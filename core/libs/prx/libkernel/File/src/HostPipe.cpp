#include "prx/libkernel/File/include/HostPipe.hpp"
#include "prx/libkernel/File/include/File.hpp"

#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
struct PipeIoStatus {
    void* status;
    ULONG_PTR information;
};

struct PipeLocalInformation {
    ULONG type;
    ULONG configuration;
    ULONG maximumInstances;
    ULONG currentInstances;
    ULONG inboundQuota;
    ULONG readDataAvailable;
    ULONG outboundQuota;
    ULONG writeQuotaAvailable;
    ULONG state;
    ULONG end;
};

using QueryInformationFile = LONG(NTAPI*)(HANDLE, PipeIoStatus*, void*, ULONG, int);
constexpr int FileAccessInformationClass = 8;
constexpr int FilePipeLocalInformationClass = 24;
constexpr ULONG PipeClosingState = 4;
#endif

}

namespace File {

std::optional<HostPipeState> QueryHostPipe(int descriptor) {
#ifdef _WIN32
    if (!DescriptorIsOpen_nid_no_patch(descriptor)) return std::nullopt;
    const auto handle = reinterpret_cast<HANDLE>(::_get_osfhandle(descriptor));
    if (handle == INVALID_HANDLE_VALUE || ::GetFileType(handle) != FILE_TYPE_PIPE) return std::nullopt;
    static const auto query = reinterpret_cast<QueryInformationFile>(
        reinterpret_cast<void*>(::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationFile")));
    if (query == nullptr) throw std::runtime_error("QueryHostPipe: NtQueryInformationFile is unavailable");
    PipeIoStatus status{};
    ACCESS_MASK access = 0;
    PipeLocalInformation pipe{};
    if (query(handle, &status, &access, sizeof(access), FileAccessInformationClass) < 0 ||
        query(handle, &status, &pipe, sizeof(pipe), FilePipeLocalInformationClass) < 0)
        throw std::runtime_error("QueryHostPipe: querying pipe descriptor " + std::to_string(descriptor) + " failed");
    HostPipeState state;
    state.readEnd = (access & FILE_READ_DATA) != 0;
    state.writeEnd = (access & FILE_WRITE_DATA) != 0;
    state.available = pipe.readDataAvailable;
    state.space = pipe.writeQuotaAvailable;
    state.peerClosed = pipe.state == PipeClosingState;
    return state;
#else
    struct stat status{};
    if (::fstat(descriptor, &status) != 0 || !S_ISFIFO(status.st_mode)) return std::nullopt;
    const int flags = ::fcntl(descriptor, F_GETFL);
    int queued = 0;
    if (flags < 0 || ::ioctl(descriptor, FIONREAD, &queued) != 0)
        throw std::runtime_error("QueryHostPipe: querying pipe descriptor " + std::to_string(descriptor) + " failed, errno=" + std::to_string(errno));
    HostPipeState state;
    state.readEnd = (flags & O_ACCMODE) != O_WRONLY;
    state.writeEnd = (flags & O_ACCMODE) != O_RDONLY;
    const int capacity = ::fcntl(descriptor, F_GETPIPE_SZ);
    state.available = queued;
    state.space = capacity > queued ? capacity - queued : 0;
    pollfd entry{descriptor, static_cast<short>(state.readEnd ? POLLIN : POLLOUT), 0};
    if (::poll(&entry, 1, 0) < 0)
        throw std::runtime_error("QueryHostPipe: polling pipe descriptor " + std::to_string(descriptor) + " failed, errno=" + std::to_string(errno));
    state.peerClosed = (entry.revents & (POLLHUP | POLLERR)) != 0;
    return state;
#endif
}

}
