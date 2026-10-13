#include "prx/libkernel/Equeue/Equeue.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/File/include/DirectoryDescriptor.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include "prx/libkernel/File/include/HostPipe.hpp"
#include "prx/libkernel/File/include/RandomDevice.hpp"
#include "prx/libkernel/File/include/SharedMemory.hpp"
#include "prx/libkernel/Socket/include/SocketRuntime.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

constexpr int GUEST_EBADF = 9;
constexpr int GUEST_ENODEV = 19;
constexpr int GUEST_EINVAL = 22;
constexpr int GUEST_EPIPE = 32;
constexpr std::uint32_t NOTE_LOWAT = 0x0001;
constexpr std::uint32_t NOTE_FILE_POLL = 0x0002;
constexpr std::int64_t PIPE_ATOMIC_BYTES = 512;

enum class DescriptorKind { Socket, Kqueue, File, Directory, Pipe, Terminal };

struct Readiness {
    bool ready = false;
    std::int64_t data = 0;
    bool eof = false;
};

struct DescriptorWatch {
    int descriptor = -1;
    DescriptorKind kind = DescriptorKind::File;
    bool write = false;
    std::uint32_t notes = 0;
    std::int64_t mark = 0;
    std::weak_ptr<KernelEqueuePrivate> queue;
    Readiness polled;
    Readiness reported;
    bool consumed = false;
};

int Classify(int descriptor, DescriptorKind& kind) {
    if (GuestSockets::IsOpen(descriptor)) {
        kind = DescriptorKind::Socket;
        return 0;
    }
    if (!DescriptorIsOpen_nid_no_patch(descriptor)) {
        return GUEST_EBADF;
    }
    if (IsKqueueDescriptor(descriptor)) {
        kind = DescriptorKind::Kqueue;
        return 0;
    }
    if (File::IsRandomDevice(descriptor)) {
        return GUEST_ENODEV;
    }
#ifdef _WIN32
    if (File::DirectoryDescriptorPath(descriptor)) {
        kind = DescriptorKind::Directory;
        return 0;
    }
    switch (::GetFileType(reinterpret_cast<HANDLE>(::_get_osfhandle(descriptor)))) {
    case FILE_TYPE_DISK:
        kind = DescriptorKind::File;
        break;
    case FILE_TYPE_PIPE:
        kind = DescriptorKind::Pipe;
        return 0;
    case FILE_TYPE_CHAR:
        if (descriptor > 2) {
            return GUEST_ENODEV;
        }
        kind = DescriptorKind::Terminal;
        return 0;
    default:
        throw std::runtime_error("EVFILT_READ/EVFILT_WRITE: descriptor " + std::to_string(descriptor) + " has an unknown host type");
    }
#else
    struct stat status {};
    if (::fstat(descriptor, &status) != 0) {
        return GUEST_EBADF;
    }
    if (S_ISDIR(status.st_mode)) {
        kind = DescriptorKind::Directory;
        return 0;
    }
    if (S_ISFIFO(status.st_mode)) {
        kind = DescriptorKind::Pipe;
        return 0;
    }
    if (S_ISCHR(status.st_mode)) {
        if (descriptor > 2) {
            return GUEST_ENODEV;
        }
        kind = DescriptorKind::Terminal;
        return 0;
    }
    if (!S_ISREG(status.st_mode)) {
        throw std::runtime_error("EVFILT_READ/EVFILT_WRITE: descriptor " + std::to_string(descriptor) + " has an unsupported host type");
    }
    kind = DescriptorKind::File;
#endif
    return File::IsSharedMemoryDescriptor_nid_no_patch(descriptor) ? GUEST_EINVAL : 0;
}

Readiness SocketReadiness(const DescriptorWatch& watch) {
    Readiness state;
    if (!GuestSockets::Ready(watch.descriptor, watch.write, &state.data, &state.eof)) {
        return {};
    }
    state.ready = state.eof || (watch.notes & NOTE_LOWAT) == 0 || state.data >= watch.mark;
    return state;
}

Readiness KqueueReadiness(const DescriptorWatch& watch) {
    const auto inner = watch.queue.lock();
    if (!inner) {
        return {};
    }
    int count = 0;
    if (!inner->CountPending(&count, false)) {
        return watch.polled;
    }
    return {count > 0, count, false};
}

Readiness FileReadiness(const DescriptorWatch& watch) {
    if (watch.write) {
        return {true, 0, false};
    }
#ifdef _WIN32
    const std::int64_t size = ::_filelengthi64(watch.descriptor);
    const std::int64_t offset = ::_telli64(watch.descriptor);
#else
    struct stat status {};
    const std::int64_t size = ::fstat(watch.descriptor, &status) == 0 ? static_cast<std::int64_t>(status.st_size) : -1;
    const std::int64_t offset = ::lseek(watch.descriptor, 0, SEEK_CUR);
#endif
    if (size < 0 || offset < 0) {
        return {};
    }
    const std::int64_t data = size - offset;
    return {data != 0 || (watch.notes & NOTE_FILE_POLL) != 0, data, false};
}

Readiness DirectoryReadiness(const DescriptorWatch& watch) {
    if (watch.write) {
        return {true, 0, false};
    }
    const auto unread = File::DirectoryDescriptorUnread(watch.descriptor);
    if (!unread) {
        return {};
    }
    return {*unread != 0 || (watch.notes & NOTE_FILE_POLL) != 0, *unread, false};
}

Readiness PipeReadiness(const DescriptorWatch& watch) {
    const auto pipe = File::QueryHostPipe(watch.descriptor);
    if (!pipe) {
        return {};
    }
    if (!watch.write) {
        const std::int64_t data = pipe->readEnd ? pipe->available : 0;
        return {pipe->peerClosed || data > 0, data, pipe->peerClosed};
    }
    if (pipe->peerClosed) {
        return {true, 0, true};
    }
    const std::int64_t data = pipe->writeEnd ? pipe->space : PIPE_ATOMIC_BYTES;
    return {data >= PIPE_ATOMIC_BYTES, data, false};
}

Readiness TerminalReadiness(const DescriptorWatch& watch) {
    if (!watch.write || watch.descriptor == 0) {
        return {};
    }
    return {true, 0, false};
}

Readiness Evaluate(const DescriptorWatch& watch) {
    switch (watch.kind) {
    case DescriptorKind::Socket:
        return SocketReadiness(watch);
    case DescriptorKind::Kqueue:
        return KqueueReadiness(watch);
    case DescriptorKind::File:
        return FileReadiness(watch);
    case DescriptorKind::Directory:
        return DirectoryReadiness(watch);
    case DescriptorKind::Pipe:
        return PipeReadiness(watch);
    case DescriptorKind::Terminal:
        return TerminalReadiness(watch);
    }
    return {};
}

bool PollDescriptor(KernelEqueueEvent* event) {
    auto& watch = *static_cast<DescriptorWatch*>(event->filter.data);
    const Readiness state = Evaluate(watch);
    watch.polled = state;
    event->event.data = static_cast<intptr_t>(state.data);
    event->event.flags = static_cast<uint16_t>(state.eof ? (event->event.flags | EV_EOF) : (event->event.flags & ~EV_EOF));
    if (!state.ready) {
        watch.reported.ready = false;
        return false;
    }
    if ((event->event.flags & EV_CLEAR) == 0) {
        return true;
    }
    if (watch.consumed && watch.reported.ready && watch.reported.data == state.data && watch.reported.eof == state.eof) {
        return false;
    }
    watch.consumed = false;
    return true;
}

bool Sliced(const DescriptorWatch& watch, uint16_t flags) {
    return watch.kind != DescriptorKind::Socket || (flags & EV_CLEAR) != 0 || (watch.notes & NOTE_LOWAT) != 0;
}

void ResetDescriptor(KernelEqueueEvent* event) {
    auto& watch = *static_cast<DescriptorWatch*>(event->filter.data);
    event->triggered = false;
    if ((event->event.flags & EV_CLEAR) == 0) {
        return;
    }
    watch.consumed = true;
    watch.reported = watch.polled;
    event->event.data = 0;
    event->event.fflags = 0;
}

}

int MakeDescriptorEvent(int descriptor, int16_t filter, uint16_t flags, uint32_t fflags, int64_t data, void* udata, KernelEqueueEvent& event) {
    DescriptorKind kind = DescriptorKind::File;
    if (const int error = Classify(descriptor, kind)) {
        return error;
    }
    const bool write = filter == EVFILT_WRITE;
    auto watch = std::make_shared<DescriptorWatch>();
    watch->descriptor = descriptor;
    watch->kind = kind;
    watch->write = write;
    watch->notes = fflags;
    watch->mark = data;
    if (kind == DescriptorKind::Kqueue) {
        if (write) {
            return GUEST_EINVAL;
        }
        watch->queue = FindKqueueDescriptor(descriptor);
    }
    if (kind == DescriptorKind::Pipe && write) {
        const auto pipe = File::QueryHostPipe(descriptor);
        if (pipe && pipe->peerClosed) {
            return GUEST_EPIPE;
        }
    }
    event = KernelEqueueEvent{};
    event.event.ident = static_cast<uintptr_t>(descriptor);
    event.event.filter = filter;
    event.event.flags = flags;
    event.event.udata = udata;
    event.filter.data = watch.get();
    event.filter.owner = watch;
    event.filter.pollFunc = PollDescriptor;
    event.filter.resetFunc = ResetDescriptor;
    event.filter.sliced = Sliced(*watch, flags);
    return 0;
}

void SetDescriptorWatch(KernelEqueueEvent& event, uint32_t fflags, int64_t data) {
    if (event.filter.pollFunc != PollDescriptor) {
        return;
    }
    auto& watch = *static_cast<DescriptorWatch*>(event.filter.data);
    watch.notes = fflags;
    watch.mark = data;
    event.filter.sliced = Sliced(watch, event.event.flags);
}
