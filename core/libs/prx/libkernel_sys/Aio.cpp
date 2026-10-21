#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <list>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/File/include/File.hpp"

extern "C" {
int* APS5_VABI __error_nid_postfix();
std::int64_t APS5_VABI _fstat_nid_postfix(int d, FileStat* sb);
std::int64_t APS5_VABI pread_nid_postfix(int d, void* buf, std::size_t nbytes, std::int64_t offset);
int APS5_VABI fsync_nid_postfix(int fd);
int APS5_VABI fcntl_nid_postfix(int descriptor, int command, ...);
}

struct PosixAioSigevent {
    int notify;
    int signo;
    std::uint64_t value;
    std::uint64_t data[8];
};

struct PosixAioControlBlock {
    int fildes;
    std::int64_t offset;
    void* buf;
    std::uint64_t nbytes;
    int spare[2];
    void* spare2;
    int lioOpcode;
    int reqprio;
    std::int64_t status;
    std::int64_t error;
    void* kernelinfo;
    PosixAioSigevent sigevent;
};
static_assert(offsetof(PosixAioControlBlock, offset) == 8);
static_assert(offsetof(PosixAioControlBlock, lioOpcode) == 48);
static_assert(offsetof(PosixAioControlBlock, status) == 56);
static_assert(offsetof(PosixAioControlBlock, error) == 64);
static_assert(offsetof(PosixAioControlBlock, kernelinfo) == 72);
static_assert(offsetof(PosixAioControlBlock, sigevent) == 80);
static_assert(sizeof(PosixAioControlBlock) == 160);

namespace {

constexpr int GuestEbadf = 9;
constexpr int GuestEfault = 14;
constexpr int GuestEinval = 22;
constexpr int GuestEagain = 35;
constexpr int GuestEinprogress = 36;
constexpr int GuestEopnotsupp = 45;
constexpr int GuestGetStatusFlags = 3;
constexpr int GuestAccessMode = 3;
constexpr int GuestWriteOnly = 1;
constexpr int GuestReadOnly = 0;
constexpr int GuestOSync = 0x0080;
constexpr int SigevNone = 0;
constexpr int SigevSignal = 1;
constexpr int SigevKevent = 3;
constexpr int SigevThreadId = 4;
constexpr int MaximumSignal = 128;
constexpr std::size_t MaximumJobs = 256;
constexpr int AioNotCanceled = 2;
constexpr int AioAllDone = 3;
constexpr std::uint16_t GuestFileType = 0170000;
constexpr std::uint16_t GuestFifo = 0010000;
constexpr std::uint16_t GuestCharacterDevice = 0020000;
constexpr std::uint16_t GuestSocket = 0140000;

enum class Operation { Read, Write, Sync };

struct Job {
    PosixAioControlBlock* user;
    int fd;
    std::int64_t status = -1;
    std::int64_t error = 0;
};

struct Jobs {
    std::mutex lock;
    std::condition_variable finished;
    bool initialized = false;
    std::list<Job> pending;
    std::list<Job> done;
    std::intptr_t nextReference = 1;
};

Jobs& State() {
    static Jobs jobs;
    return jobs;
}

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

std::optional<std::chrono::nanoseconds> Timeout(const KernelTimespec* timeout, int* error) {
    *error = 0;
    if (timeout == nullptr) return std::nullopt;
    if (timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000 || timeout->tv_sec < 0) {
        *error = GuestEinval;
        return std::nullopt;
    }
    constexpr auto limit = std::numeric_limits<std::int64_t>::max() / 1000000000 - 1;
    const auto seconds = timeout->tv_sec > limit ? limit : timeout->tv_sec;
    return std::chrono::seconds(seconds) + std::chrono::nanoseconds(timeout->tv_nsec);
}

int DescriptorError(int fd, Operation operation) {
    const int saved = *__error_nid_postfix();
    const int flags = fcntl_nid_postfix(fd, GuestGetStatusFlags);
    FileStat status{};
    const bool described = flags >= 0 && _fstat_nid_postfix(fd, &status) == 0;
    *__error_nid_postfix() = saved;
    if (!described) return GuestEbadf;
    const int access = flags & GuestAccessMode;
    if (operation == Operation::Read && access == GuestWriteOnly) return GuestEbadf;
    if (operation == Operation::Write && access == GuestReadOnly) return GuestEbadf;
    const auto type = status.st_mode & GuestFileType;
    if (type == GuestCharacterDevice)
        throw std::runtime_error("aio: descriptor " + std::to_string(fd) + " is a device, and the console's disk and device AIO backends are not modelled");
    if (type == GuestSocket) {
        if (operation == Operation::Sync) return GuestEinval;
        throw std::runtime_error("aio: descriptor " + std::to_string(fd) + " is a socket, whose requests complete when data arrives, which is not modelled");
    }
    if (type == GuestFifo) return operation == Operation::Sync ? GuestEinval : -GuestEopnotsupp;
    return 0;
}

int Queue(PosixAioControlBlock* user, Operation operation) {
    if (user == nullptr) return Fail(GuestEfault);
    auto& jobs = State();
    std::unique_lock lock(jobs.lock);
    jobs.initialized = true;
    user->status = -1;
    user->error = 0;
    user->kernelinfo = reinterpret_cast<void*>(static_cast<std::intptr_t>(-1));
    if (jobs.pending.size() + jobs.done.size() >= MaximumJobs) {
        user->error = GuestEagain;
        return Fail(GuestEagain);
    }
    const PosixAioControlBlock request = *user;
    if (request.nbytes > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return Fail(GuestEinval);
    const int notify = request.sigevent.notify;
    if (notify != SigevKevent && notify != SigevSignal && notify != SigevThreadId && notify != SigevNone) {
        user->error = GuestEinval;
        return Fail(GuestEinval);
    }
    if ((notify == SigevSignal || notify == SigevThreadId) && (request.sigevent.signo <= 0 || request.sigevent.signo > MaximumSignal))
        return Fail(GuestEinval);
    int queueError = 0;
    if (const int error = DescriptorError(request.fildes, operation)) {
        if (error > 0) {
            user->error = error;
            return Fail(error);
        }
        queueError = -error;
    }
    if (operation != Operation::Sync && request.offset < 0) {
        user->error = GuestEinval;
        return Fail(GuestEinval);
    }
    user->kernelinfo = reinterpret_cast<void*>(jobs.nextReference++);
    if (notify != SigevNone)
        throw std::runtime_error("aio: completion notification " + std::to_string(notify) + " (signal, thread or kevent) is not modelled");
    user->error = GuestEinprogress;
    if (queueError != 0) {
        user->error = queueError;
        return Fail(queueError);
    }
    auto job = jobs.pending.insert(jobs.pending.end(), Job{user, request.fildes});
    lock.unlock();
    const int saved = *__error_nid_postfix();
    std::int64_t result = 0;
    if (operation == Operation::Read) {
        if (request.nbytes != 0) result = pread_nid_postfix(request.fildes, request.buf, request.nbytes, request.offset);
    } else if (operation == Operation::Write) {
        result = PosixPwrite_nid_no_patch(request.fildes, request.buf, request.nbytes, request.offset);
    } else {
        result = fsync_nid_postfix(request.fildes);
    }
    const int error = result < 0 ? *__error_nid_postfix() : 0;
    *__error_nid_postfix() = saved;
    lock.lock();
    job->status = result < 0 ? -1 : result;
    job->error = error;
    jobs.done.splice(jobs.done.end(), jobs.pending, job);
    jobs.finished.notify_all();
    return 0;
}

std::int64_t Collect(Jobs& jobs, std::list<Job>::iterator job) {
    PosixAioControlBlock* user = job->user;
    const auto status = job->status;
    const auto error = job->error;
    jobs.done.erase(job);
    user->error = error;
    user->status = status;
    if (error != 0) return Fail(static_cast<int>(error));
    return status;
}

}

extern "C" {

int APS5_VABI aio_read_nid_postfix(PosixAioControlBlock* aiocb) {
    return Queue(aiocb, Operation::Read);
}

int APS5_VABI aio_write_nid_postfix(PosixAioControlBlock* aiocb) {
    return Queue(aiocb, Operation::Write);
}

int APS5_VABI aio_fsync_nid_postfix(int op, PosixAioControlBlock* aiocb) {
    if (op != GuestOSync) return Fail(GuestEinval);
    return Queue(aiocb, Operation::Sync);
}

int APS5_VABI aio_error_nid_postfix(const PosixAioControlBlock* aiocb) {
    if (aiocb == nullptr) throw std::invalid_argument("aio_error: null control block");
    auto& jobs = State();
    const std::lock_guard lock(jobs.lock);
    if (!jobs.initialized) return GuestEinval;
    for (const auto& job : jobs.pending) {
        if (job.user == aiocb) return GuestEinprogress;
    }
    for (const auto& job : jobs.done) {
        if (job.user == aiocb) return static_cast<int>(job.error);
    }
    if (aiocb->status == -1) return static_cast<int>(aiocb->error);
    return GuestEinval;
}

std::int64_t APS5_VABI aio_return_nid_postfix(PosixAioControlBlock* aiocb) {
    auto& jobs = State();
    const std::lock_guard lock(jobs.lock);
    if (!jobs.initialized) return Fail(GuestEinval);
    for (auto job = jobs.done.begin(); job != jobs.done.end(); ++job) {
        if (job->user == aiocb) return Collect(jobs, job);
    }
    return Fail(GuestEinval);
}

int APS5_VABI aio_suspend_nid_postfix(const PosixAioControlBlock* const* list, int count, const KernelTimespec* timeout) {
    if (count < 0 || count > static_cast<int>(MaximumJobs)) return Fail(GuestEinval);
    if (count > 0 && list == nullptr) return Fail(GuestEfault);
    int error = 0;
    const auto wait = Timeout(timeout, &error);
    if (error) return Fail(error);
    auto& jobs = State();
    std::unique_lock lock(jobs.lock);
    if (!jobs.initialized) return Fail(GuestEagain);
    if (count == 0) return 0;
    const auto listed = [&](const Job& job) {
        for (int index = 0; index < count; ++index) {
            if (list[index] == job.user) return true;
        }
        return false;
    };
    const auto ready = [&] {
        for (const auto& job : jobs.done) {
            if (listed(job)) return true;
        }
        for (const auto& job : jobs.pending) {
            if (listed(job)) return false;
        }
        return true;
    };
    if (!wait) {
        jobs.finished.wait(lock, ready);
        return 0;
    }
    if (!jobs.finished.wait_for(lock, *wait, ready)) return Fail(GuestEagain);
    return 0;
}

int APS5_VABI aio_cancel_nid_postfix(int fd, PosixAioControlBlock* aiocb) {
    FileStat status{};
    const int saved = *__error_nid_postfix();
    const bool open = _fstat_nid_postfix(fd, &status) == 0;
    *__error_nid_postfix() = saved;
    if (!open) return Fail(GuestEbadf);
    auto& jobs = State();
    const std::lock_guard lock(jobs.lock);
    if (!jobs.initialized) return AioAllDone;
    if ((status.st_mode & GuestFileType) == GuestCharacterDevice)
        throw std::runtime_error("aio_cancel: descriptor " + std::to_string(fd) + " is a device, and whether the console takes it for a disk is not modelled");
    for (const auto& job : jobs.pending) {
        if (job.fd == fd && (aiocb == nullptr || job.user == aiocb)) return AioNotCanceled;
    }
    return AioAllDone;
}

std::int64_t APS5_VABI aio_waitcomplete_nid_postfix(PosixAioControlBlock** result, const KernelTimespec* timeout) {
    if (result != nullptr) *result = nullptr;
    bool poll = false;
    std::optional<std::chrono::nanoseconds> wait;
    if (timeout != nullptr && timeout->tv_sec == 0 && timeout->tv_nsec == 0) {
        poll = true;
    } else {
        int error = 0;
        wait = Timeout(timeout, &error);
        if (error) return Fail(error);
    }
    auto& jobs = State();
    std::unique_lock lock(jobs.lock);
    jobs.initialized = true;
    const auto ready = [&] { return !jobs.done.empty(); };
    if (poll) {
        if (!ready()) return Fail(GuestEagain);
    } else if (!wait) {
        jobs.finished.wait(lock, ready);
    } else if (!jobs.finished.wait_for(lock, *wait, ready)) {
        return Fail(GuestEagain);
    }
    const auto job = jobs.done.begin();
    if (result != nullptr) *result = job->user;
    return Collect(jobs, job);
}

}
