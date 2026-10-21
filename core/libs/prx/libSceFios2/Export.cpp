#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include <chrono>
#include <cstdio>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr int ErrorBadPath = static_cast<int>(0x80820005u);
constexpr int ErrorBadOp = static_cast<int>(0x8082000Au);
constexpr int KernelErrorNoEntry = static_cast<int>(0x80020002u);
constexpr std::int32_t HandleInvalid = 0;
constexpr std::uint8_t OpEventComplete = 1;
constexpr std::uint16_t OpenReadOnly = 0x1;
constexpr std::uint16_t OpenWriteOnly = 0x2;
constexpr std::uint16_t OpenAppend = 0x4;
constexpr std::uint16_t OpenCreate = 0x8;
constexpr std::uint16_t OpenTruncate = 0x10;
constexpr std::uint16_t OpenUncached = 0x1000;
constexpr std::int32_t DefaultCreateMode = 0x1ff;
constexpr std::uint16_t DirectoryCreateMode = 0x1ff;

struct FiosBuffer {
    void* pointer;
    std::size_t length;
};

using FiosOpCallback = int (APS5_VABI*)(void* context, std::int32_t op, std::uint8_t event, int error);

struct FiosOpAttr {
    std::int64_t deadline;
    FiosOpCallback callback;
    void* callbackContext;
    std::uint32_t priorityAndFlags;
    std::uint32_t userTag;
    void* userPointer;
    void* reserved;
};
static_assert(sizeof(FiosOpAttr) == 48);

struct FiosOpenParams {
    std::uint16_t openFlags;
    std::uint16_t opFlags;
    std::uint32_t reserved;
    FiosBuffer buffer;
};
static_assert(sizeof(FiosOpenParams) == 24);

struct Operation {
    FiosOpAttr attr{};
    int error = 0;
    bool transfer = false;
    std::int64_t requested = 0;
    std::int64_t actual = 0;
    std::optional<std::int64_t> offset;
    bool hasBuffer = false;
    void* buffer = nullptr;
    bool hasPath = false;
    std::string path;
};

struct FileHandle {
    int descriptor;
    std::string path;
};

struct DirectoryHandle {
    std::string path;
};

std::mutex g_mutex;
bool g_initialized = false;
std::uint32_t g_suspendCount = 0;
std::int32_t g_nextHandle = 1;
std::unordered_map<std::int32_t, Operation> g_operations;
std::unordered_map<std::int32_t, FileHandle> g_files;
std::unordered_map<std::int32_t, DirectoryHandle> g_directories;

[[noreturn]] void ThrowUnknown(const char* function, const std::string& what) {
    throw std::logic_error(std::string(function) + ": " + what + ", and the console's libSceFios2 answer for it is unknown");
}

void RequireInitialized(const char* function) {
    if (!g_initialized) ThrowUnknown(function, "libSceFios2 is not initialized");
}

void RequireRunning(const char* function) {
    RequireInitialized(function);
    if (g_suspendCount != 0) ThrowUnknown(function, "I/O is suspended (holding the op until sceFiosResume is not modelled)");
}

const char* RequirePath(const char* function, const char* path) {
    if (path == nullptr) ThrowUnknown(function, "the path is null");
    return path;
}

std::int32_t AllocateHandle() {
    for (;;) {
        const std::int32_t handle = g_nextHandle;
        g_nextHandle = g_nextHandle == std::numeric_limits<std::int32_t>::max() ? 1 : g_nextHandle + 1;
        if (!g_operations.contains(handle) && !g_files.contains(handle) && !g_directories.contains(handle)) return handle;
    }
}

FileHandle& RequireFile(const char* function, std::int32_t fh) {
    const auto found = g_files.find(fh);
    if (found == g_files.end()) ThrowUnknown(function, "file handle " + std::to_string(fh) + " is not open");
    return found->second;
}

Operation* FindOperation(std::int32_t op) {
    const auto found = g_operations.find(op);
    return found == g_operations.end() ? nullptr : &found->second;
}

Operation& RequireOperation(const char* function, std::int32_t op) {
    Operation* operation = FindOperation(op);
    if (operation == nullptr) ThrowUnknown(function, "op " + std::to_string(op) + " does not exist");
    return *operation;
}

std::string KernelCode(std::int64_t result) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08X", static_cast<unsigned>(result));
    return text;
}

int PathResult(const char* function, std::int64_t result) {
    if (result >= 0) return 0;
    if (result == KernelErrorNoEntry) return ErrorBadPath;
    ThrowUnknown(function, "the kernel returned " + KernelCode(result));
}

std::int64_t TransferResult(const char* function, std::int64_t result) {
    if (result < 0) ThrowUnknown(function, "the kernel returned " + KernelCode(result));
    return result;
}

std::size_t RequireLength(const char* function, std::int64_t length) {
    if (length < 0) ThrowUnknown(function, "the length " + std::to_string(length) + " is negative");
    return static_cast<std::size_t>(length);
}

std::int64_t VectorLength(const char* function, const FiosBuffer* iov, int iovcnt) {
    if (iovcnt < 0 || (iov == nullptr && iovcnt != 0)) ThrowUnknown(function, "the buffer vector is invalid");
    std::int64_t total = 0;
    for (int index = 0; index < iovcnt; ++index) total += static_cast<std::int64_t>(iov[index].length);
    return total;
}

enum class PathKind { Missing, File, Directory };

PathKind Classify(const char* function, const char* path) {
    FileStat status{};
    const int result = sceKernelStat(path, &status);
    if (result == KernelErrorNoEntry) return PathKind::Missing;
    if (result < 0) ThrowUnknown(function, std::string("the kernel returned ") + KernelCode(result) + " for " + path);
    return (status.st_mode & 0170000) == 0040000 ? PathKind::Directory : PathKind::File;
}

void TrackPath(Operation& operation, const char* path) {
    operation.hasPath = true;
    operation.path = path;
}

void TrackTransfer(Operation& operation, void* buffer, std::int64_t requested, std::optional<std::int64_t> offset) {
    operation.transfer = true;
    operation.requested = requested;
    operation.offset = offset;
    operation.hasBuffer = true;
    operation.buffer = buffer;
}

int OpenFlags(const FiosOpenParams* params) {
    const std::uint16_t flags = params != nullptr ? params->openFlags : OpenReadOnly;
    int native = SCE_KERNEL_O_RDONLY;
    if ((flags & (OpenReadOnly | OpenWriteOnly)) == OpenWriteOnly) native = SCE_KERNEL_O_WRONLY;
    if ((flags & (OpenReadOnly | OpenWriteOnly)) == (OpenReadOnly | OpenWriteOnly)) native = SCE_KERNEL_O_RDWR;
    if (flags & OpenAppend) native |= SCE_KERNEL_O_APPEND;
    if (flags & OpenCreate) native |= SCE_KERNEL_O_CREAT;
    if (flags & OpenTruncate) native |= SCE_KERNEL_O_TRUNC;
    if (flags & OpenUncached) native |= SCE_KERNEL_O_DIRECT;
    return native;
}

template <typename Work>
std::int32_t Submit(const char* function, const FiosOpAttr* attr, Work work) {
    std::int32_t handle;
    FiosOpAttr completion{};
    int error;
    {
        std::lock_guard lock(g_mutex);
        RequireRunning(function);
        Operation operation;
        if (attr != nullptr) operation.attr = *attr;
        work(operation);
        handle = AllocateHandle();
        completion = operation.attr;
        error = operation.error;
        g_operations.emplace(handle, std::move(operation));
    }
    if (completion.callback != nullptr) completion.callback(completion.callbackContext, handle, OpEventComplete, error);
    return handle;
}

std::int32_t OpenFile(const char* function, const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params, std::int32_t nativeMode) {
    if (outFh == nullptr) ThrowUnknown(function, "the file handle output is null");
    RequirePath(function, path);
    return Submit(function, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        if (Classify(function, path) == PathKind::Directory) ThrowUnknown(function, std::string(path) + " is a directory");
        const int flags = OpenFlags(params);
        const std::uint16_t mode = (flags & SCE_KERNEL_O_CREAT) ? static_cast<std::uint16_t>(nativeMode == -1 ? DefaultCreateMode : nativeMode) : 0;
        const int descriptor = sceKernelOpen(path, flags, mode);
        operation.error = PathResult(function, descriptor);
        if (operation.error != 0) {
            *outFh = HandleInvalid;
            return;
        }
        const std::int32_t fh = AllocateHandle();
        g_files.emplace(fh, FileHandle{descriptor, path});
        *outFh = fh;
    });
}

}

extern "C" {

std::int64_t APS5_VABI sceFiosOpSyncWaitForIO(std::int32_t op);
int APS5_VABI sceFiosOpSyncWait(std::int32_t op);

int APS5_VABI sceKernelFstat(int d, FileStat* sb);
int APS5_VABI sceKernelFsync(int fd);
int APS5_VABI sceKernelFtruncate(int d, std::int64_t length);
int APS5_VABI sceKernelMkdir(const char* path, std::uint16_t mode);
int APS5_VABI sceKernelRmdir(const char* path);
int APS5_VABI sceKernelRename(const char* from, const char* to);
std::int64_t APS5_VABI sceKernelPread(int d, void* buf, std::size_t nbytes, std::int64_t offset);
std::int64_t APS5_VABI sceKernelPwrite(int d, const void* buf, std::size_t nbytes, std::int64_t offset);
std::int64_t APS5_VABI sceKernelReadv(int d, const FiosBuffer* iov, int iovcnt);
std::int64_t APS5_VABI sceKernelWritev(int d, const FiosBuffer* iov, int iovcnt);
std::int64_t APS5_VABI sceKernelPreadv(int d, const FiosBuffer* iov, int iovcnt, std::int64_t offset);
std::int64_t APS5_VABI sceKernelPwritev(int d, const FiosBuffer* iov, int iovcnt, std::int64_t offset);

int APS5_VABI sceFiosInitialize(const void* parameters) {
    std::lock_guard lock(g_mutex);
    if (parameters == nullptr) ThrowUnknown(__func__, "the parameters are null");
    if (g_initialized) ThrowUnknown(__func__, "libSceFios2 is already initialized");
    g_initialized = true;
    g_suspendCount = 0;
    return 0;
}

void APS5_VABI sceFiosTerminate(void) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    for (const auto& file : g_files) sceKernelClose(file.second.descriptor);
    g_files.clear();
    g_directories.clear();
    g_operations.clear();
    g_suspendCount = 0;
    g_initialized = false;
}

bool APS5_VABI sceFiosIsInitialized(void* outParameters) {
    std::lock_guard lock(g_mutex);
    if (outParameters != nullptr && g_initialized) ThrowUnknown(__func__, "copying the parameters back needs the SceFiosParams layout");
    return g_initialized;
}

void APS5_VABI sceFiosSuspend(void) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    ++g_suspendCount;
}

void APS5_VABI sceFiosResume(void) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    if (g_suspendCount == 0) ThrowUnknown(__func__, "I/O is not suspended");
    --g_suspendCount;
}

std::uint32_t APS5_VABI sceFiosGetSuspendCount(void) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    return g_suspendCount;
}

bool APS5_VABI sceFiosIsSuspended(void) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    return g_suspendCount != 0;
}

bool APS5_VABI sceFiosIsIdle(void) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    return true;
}

void APS5_VABI sceFiosCancelAllOps(void) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
}

void APS5_VABI sceFiosCloseAllFiles(void) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    for (const auto& file : g_files) sceKernelClose(file.second.descriptor);
    g_files.clear();
}

bool APS5_VABI sceFiosIsValidHandle(std::int32_t handle) {
    std::lock_guard lock(g_mutex);
    return g_operations.contains(handle) || g_files.contains(handle) || g_directories.contains(handle);
}

bool APS5_VABI sceFiosOpIsDone(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    return FindOperation(op) != nullptr;
}

int APS5_VABI sceFiosOpWait(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    const Operation* operation = FindOperation(op);
    return operation == nullptr ? ErrorBadOp : operation->error;
}

int APS5_VABI sceFiosOpWaitUntil(std::int32_t op, std::int64_t deadline) {
    (void)deadline;
    return sceFiosOpWait(op);
}

int APS5_VABI sceFiosOpGetError(std::int32_t op) {
    return sceFiosOpWait(op);
}

int APS5_VABI sceFiosOpSyncWait(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    const auto found = g_operations.find(op);
    if (found == g_operations.end()) return ErrorBadOp;
    const int error = found->second.error;
    g_operations.erase(found);
    return error;
}

std::int64_t APS5_VABI sceFiosOpSyncWaitForIO(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    const auto found = g_operations.find(op);
    if (found == g_operations.end()) return ErrorBadOp;
    const Operation& operation = found->second;
    const std::int64_t result = operation.error != 0 || !operation.transfer ? operation.error : operation.actual;
    g_operations.erase(found);
    return result;
}

int APS5_VABI sceFiosOpDelete(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    return g_operations.erase(op) != 0 ? 0 : ErrorBadOp;
}

void APS5_VABI sceFiosOpCancel(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    RequireOperation(__func__, op);
}

bool APS5_VABI sceFiosOpIsCancelled(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    RequireOperation(__func__, op);
    return false;
}

const FiosOpAttr* APS5_VABI sceFiosOpGetAttr(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    return &RequireOperation(__func__, op).attr;
}

void APS5_VABI sceFiosOpReschedule(std::int32_t op, std::int64_t newDeadline) {
    std::lock_guard lock(g_mutex);
    RequireOperation(__func__, op).attr.deadline = newDeadline;
}

const char* APS5_VABI sceFiosOpGetPath(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    const Operation& operation = RequireOperation(__func__, op);
    if (!operation.hasPath) ThrowUnknown(__func__, "op " + std::to_string(op) + " has no path argument");
    return operation.path.c_str();
}

void* APS5_VABI sceFiosOpGetBuffer(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    const Operation& operation = RequireOperation(__func__, op);
    if (!operation.hasBuffer) ThrowUnknown(__func__, "op " + std::to_string(op) + " has no single data buffer");
    return operation.buffer;
}

std::int64_t APS5_VABI sceFiosOpGetOffset(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    const Operation& operation = RequireOperation(__func__, op);
    if (!operation.offset) ThrowUnknown(__func__, "op " + std::to_string(op) + " has no offset");
    return *operation.offset;
}

std::int64_t APS5_VABI sceFiosOpGetRequestCount(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    const Operation& operation = RequireOperation(__func__, op);
    if (!operation.transfer) ThrowUnknown(__func__, "op " + std::to_string(op) + " transfers no data");
    return operation.requested;
}

std::int64_t APS5_VABI sceFiosOpGetActualCount(std::int32_t op) {
    std::lock_guard lock(g_mutex);
    const Operation* operation = FindOperation(op);
    if (operation == nullptr) return ErrorBadOp;
    if (!operation->transfer) ThrowUnknown(__func__, "op " + std::to_string(op) + " transfers no data");
    return operation->actual;
}

std::int64_t APS5_VABI sceFiosTimeIntervalFromNanoseconds(std::int64_t ns) {
    return ns;
}

std::int64_t APS5_VABI sceFiosTimeIntervalToNanoseconds(std::int64_t interval) {
    return interval;
}

std::int64_t APS5_VABI sceFiosTimeGetCurrent(void) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::int64_t APS5_VABI sceFiosDateGetCurrent(void) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::int32_t APS5_VABI sceFiosExists(const FiosOpAttr* attr, const char* path, bool* outExists) {
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        const bool exists = Classify(__func__, path) != PathKind::Missing;
        if (outExists != nullptr) *outExists = exists;
    });
}

bool APS5_VABI sceFiosExistsSync(const FiosOpAttr* attr, const char* path) {
    bool exists = false;
    sceFiosOpSyncWait(sceFiosExists(attr, path, &exists));
    return exists;
}

std::int32_t APS5_VABI sceFiosDirectoryExists(const FiosOpAttr* attr, const char* path, bool* outExists) {
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        const bool exists = Classify(__func__, path) == PathKind::Directory;
        if (outExists != nullptr) *outExists = exists;
    });
}

bool APS5_VABI sceFiosDirectoryExistsSync(const FiosOpAttr* attr, const char* path) {
    bool exists = false;
    sceFiosOpSyncWait(sceFiosDirectoryExists(attr, path, &exists));
    return exists;
}

bool APS5_VABI sceFiosFileExistsSync(const FiosOpAttr* attr, const char* path) {
    RequirePath(__func__, path);
    bool exists = false;
    sceFiosOpSyncWait(Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        exists = Classify(__func__, path) == PathKind::File;
    }));
    return exists;
}

std::int64_t APS5_VABI sceFiosFileGetSizeSync(const FiosOpAttr* attr, const char* path) {
    RequirePath(__func__, path);
    std::int64_t size = 0;
    const int error = sceFiosOpSyncWait(Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        FileStat status{};
        operation.error = PathResult(__func__, sceKernelStat(path, &status));
        if (operation.error != 0) return;
        if ((status.st_mode & 0170000) == 0040000) ThrowUnknown(__func__, std::string(path) + " is a directory");
        size = status.st_size;
    }));
    return error != 0 ? error : size;
}

std::int32_t APS5_VABI sceFiosFileDelete(const FiosOpAttr* attr, const char* path) {
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        operation.error = PathResult(__func__, sceKernelUnlink(path));
    });
}

int APS5_VABI sceFiosFileDeleteSync(const FiosOpAttr* attr, const char* path) {
    return sceFiosOpSyncWait(sceFiosFileDelete(attr, path));
}

std::int32_t APS5_VABI sceFiosDirectoryCreate(const FiosOpAttr* attr, const char* path) {
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        const int result = sceKernelMkdir(path, DirectoryCreateMode);
        if (result == KernelErrorNoEntry) ThrowUnknown(__func__, std::string("a parent directory of ") + path + " is missing (whether it is created is not known)");
        operation.error = PathResult(__func__, result);
    });
}

int APS5_VABI sceFiosDirectoryCreateSync(const FiosOpAttr* attr, const char* path) {
    return sceFiosOpSyncWait(sceFiosDirectoryCreate(attr, path));
}

std::int32_t APS5_VABI sceFiosDirectoryDelete(const FiosOpAttr* attr, const char* path) {
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        operation.error = PathResult(__func__, sceKernelRmdir(path));
    });
}

int APS5_VABI sceFiosDirectoryDeleteSync(const FiosOpAttr* attr, const char* path) {
    return sceFiosOpSyncWait(sceFiosDirectoryDelete(attr, path));
}

std::int32_t APS5_VABI sceFiosDelete(const FiosOpAttr* attr, const char* path) {
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        switch (Classify(__func__, path)) {
        case PathKind::Missing: operation.error = ErrorBadPath; break;
        case PathKind::Directory: operation.error = PathResult(__func__, sceKernelRmdir(path)); break;
        case PathKind::File: operation.error = PathResult(__func__, sceKernelUnlink(path)); break;
        }
    });
}

int APS5_VABI sceFiosDeleteSync(const FiosOpAttr* attr, const char* path) {
    return sceFiosOpSyncWait(sceFiosDelete(attr, path));
}

std::int32_t APS5_VABI sceFiosRename(const FiosOpAttr* attr, const char* oldPath, const char* newPath) {
    RequirePath(__func__, oldPath);
    RequirePath(__func__, newPath);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, oldPath);
        operation.error = PathResult(__func__, sceKernelRename(oldPath, newPath));
    });
}

int APS5_VABI sceFiosRenameSync(const FiosOpAttr* attr, const char* oldPath, const char* newPath) {
    return sceFiosOpSyncWait(sceFiosRename(attr, oldPath, newPath));
}

std::int32_t APS5_VABI sceFiosFileRead(const FiosOpAttr* attr, const char* path, void* buffer, std::int64_t length, std::int64_t offset) {
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        TrackTransfer(operation, buffer, length, offset);
        const std::size_t size = RequireLength(__func__, length);
        const int descriptor = sceKernelOpen(path, SCE_KERNEL_O_RDONLY, 0);
        operation.error = PathResult(__func__, descriptor);
        if (operation.error != 0) return;
        const std::int64_t result = sceKernelPread(descriptor, buffer, size, offset);
        sceKernelClose(descriptor);
        operation.actual = TransferResult(__func__, result);
    });
}

std::int64_t APS5_VABI sceFiosFileReadSync(const FiosOpAttr* attr, const char* path, void* buffer, std::int64_t length, std::int64_t offset) {
    return sceFiosOpSyncWaitForIO(sceFiosFileRead(attr, path, buffer, length, offset));
}

std::int32_t APS5_VABI sceFiosFileTruncate(const FiosOpAttr* attr, const char* path, std::int64_t length) {
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        RequireLength(__func__, length);
        const int descriptor = sceKernelOpen(path, SCE_KERNEL_O_WRONLY, 0);
        operation.error = PathResult(__func__, descriptor);
        if (operation.error != 0) return;
        const int result = sceKernelFtruncate(descriptor, length);
        sceKernelClose(descriptor);
        TransferResult(__func__, result);
    });
}

int APS5_VABI sceFiosFileTruncateSync(const FiosOpAttr* attr, const char* path, std::int64_t length) {
    return sceFiosOpSyncWait(sceFiosFileTruncate(attr, path, length));
}

std::int32_t APS5_VABI sceFiosFHOpenWithMode(const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params, std::int32_t nativeMode) {
    return OpenFile(__func__, attr, outFh, path, params, nativeMode);
}

int APS5_VABI sceFiosFHOpenWithModeSync(const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params, std::int32_t nativeMode) {
    return sceFiosOpSyncWait(sceFiosFHOpenWithMode(attr, outFh, path, params, nativeMode));
}

std::int32_t APS5_VABI sceFiosFHOpen(const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params) {
    return OpenFile(__func__, attr, outFh, path, params, -1);
}

int APS5_VABI sceFiosFHOpenSync(const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params) {
    return sceFiosOpSyncWait(sceFiosFHOpen(attr, outFh, path, params));
}

std::int32_t APS5_VABI sceFiosFHClose(const FiosOpAttr* attr, std::int32_t fh) {
    return Submit(__func__, attr, [&](Operation& operation) {
        FileHandle& file = RequireFile(__func__, fh);
        TransferResult(__func__, sceKernelClose(file.descriptor));
        g_files.erase(fh);
        operation.error = 0;
    });
}

int APS5_VABI sceFiosFHCloseSync(const FiosOpAttr* attr, std::int32_t fh) {
    return sceFiosOpSyncWait(sceFiosFHClose(attr, fh));
}

std::int32_t APS5_VABI sceFiosFHRead(const FiosOpAttr* attr, std::int32_t fh, void* buffer, std::int64_t length) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        const std::size_t size = RequireLength(__func__, length);
        TrackTransfer(operation, buffer, length, TransferResult(__func__, sceKernelLseek(file.descriptor, 0, 1)));
        operation.actual = TransferResult(__func__, sceKernelRead(file.descriptor, buffer, size));
    });
}

std::int64_t APS5_VABI sceFiosFHReadSync(const FiosOpAttr* attr, std::int32_t fh, void* buffer, std::int64_t length) {
    return sceFiosOpSyncWaitForIO(sceFiosFHRead(attr, fh, buffer, length));
}

std::int32_t APS5_VABI sceFiosFHWrite(const FiosOpAttr* attr, std::int32_t fh, const void* buffer, std::int64_t length) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        const std::size_t size = RequireLength(__func__, length);
        TrackTransfer(operation, const_cast<void*>(buffer), length, TransferResult(__func__, sceKernelLseek(file.descriptor, 0, 1)));
        operation.actual = TransferResult(__func__, sceKernelWrite(file.descriptor, buffer, size));
    });
}

std::int64_t APS5_VABI sceFiosFHWriteSync(const FiosOpAttr* attr, std::int32_t fh, const void* buffer, std::int64_t length) {
    return sceFiosOpSyncWaitForIO(sceFiosFHWrite(attr, fh, buffer, length));
}

std::int32_t APS5_VABI sceFiosFHPread(const FiosOpAttr* attr, std::int32_t fh, void* buffer, std::int64_t length, std::int64_t offset) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        const std::size_t size = RequireLength(__func__, length);
        TrackTransfer(operation, buffer, length, offset);
        operation.actual = TransferResult(__func__, sceKernelPread(file.descriptor, buffer, size, offset));
    });
}

std::int64_t APS5_VABI sceFiosFHPreadSync(const FiosOpAttr* attr, std::int32_t fh, void* buffer, std::int64_t length, std::int64_t offset) {
    return sceFiosOpSyncWaitForIO(sceFiosFHPread(attr, fh, buffer, length, offset));
}

std::int32_t APS5_VABI sceFiosFHPwrite(const FiosOpAttr* attr, std::int32_t fh, const void* buffer, std::int64_t length, std::int64_t offset) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        const std::size_t size = RequireLength(__func__, length);
        TrackTransfer(operation, const_cast<void*>(buffer), length, offset);
        operation.actual = TransferResult(__func__, sceKernelPwrite(file.descriptor, buffer, size, offset));
    });
}

std::int64_t APS5_VABI sceFiosFHPwriteSync(const FiosOpAttr* attr, std::int32_t fh, const void* buffer, std::int64_t length, std::int64_t offset) {
    return sceFiosOpSyncWaitForIO(sceFiosFHPwrite(attr, fh, buffer, length, offset));
}

std::int32_t APS5_VABI sceFiosFHReadv(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        operation.transfer = true;
        operation.requested = VectorLength(__func__, iov, iovcnt);
        operation.offset = TransferResult(__func__, sceKernelLseek(file.descriptor, 0, 1));
        operation.actual = TransferResult(__func__, sceKernelReadv(file.descriptor, iov, iovcnt));
    });
}

std::int64_t APS5_VABI sceFiosFHReadvSync(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt) {
    return sceFiosOpSyncWaitForIO(sceFiosFHReadv(attr, fh, iov, iovcnt));
}

std::int32_t APS5_VABI sceFiosFHWritev(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        operation.transfer = true;
        operation.requested = VectorLength(__func__, iov, iovcnt);
        operation.offset = TransferResult(__func__, sceKernelLseek(file.descriptor, 0, 1));
        operation.actual = TransferResult(__func__, sceKernelWritev(file.descriptor, iov, iovcnt));
    });
}

std::int64_t APS5_VABI sceFiosFHWritevSync(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt) {
    return sceFiosOpSyncWaitForIO(sceFiosFHWritev(attr, fh, iov, iovcnt));
}

std::int32_t APS5_VABI sceFiosFHPreadv(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt, std::int64_t offset) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        operation.transfer = true;
        operation.requested = VectorLength(__func__, iov, iovcnt);
        operation.offset = offset;
        operation.actual = TransferResult(__func__, sceKernelPreadv(file.descriptor, iov, iovcnt, offset));
    });
}

std::int64_t APS5_VABI sceFiosFHPreadvSync(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt, std::int64_t offset) {
    return sceFiosOpSyncWaitForIO(sceFiosFHPreadv(attr, fh, iov, iovcnt, offset));
}

std::int32_t APS5_VABI sceFiosFHPwritev(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt, std::int64_t offset) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        operation.transfer = true;
        operation.requested = VectorLength(__func__, iov, iovcnt);
        operation.offset = offset;
        operation.actual = TransferResult(__func__, sceKernelPwritev(file.descriptor, iov, iovcnt, offset));
    });
}

std::int64_t APS5_VABI sceFiosFHPwritevSync(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt, std::int64_t offset) {
    return sceFiosOpSyncWaitForIO(sceFiosFHPwritev(attr, fh, iov, iovcnt, offset));
}

std::int32_t APS5_VABI sceFiosFHTruncate(const FiosOpAttr* attr, std::int32_t fh, std::int64_t length) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        RequireLength(__func__, length);
        TransferResult(__func__, sceKernelFtruncate(file.descriptor, length));
        operation.error = 0;
    });
}

int APS5_VABI sceFiosFHTruncateSync(const FiosOpAttr* attr, std::int32_t fh, std::int64_t length) {
    return sceFiosOpSyncWait(sceFiosFHTruncate(attr, fh, length));
}

std::int32_t APS5_VABI sceFiosFHSync(const FiosOpAttr* attr, std::int32_t fh) {
    return Submit(__func__, attr, [&](Operation& operation) {
        const FileHandle& file = RequireFile(__func__, fh);
        TransferResult(__func__, sceKernelFsync(file.descriptor));
        operation.error = 0;
    });
}

int APS5_VABI sceFiosFHSyncSync(const FiosOpAttr* attr, std::int32_t fh) {
    return sceFiosOpSyncWait(sceFiosFHSync(attr, fh));
}

std::int64_t APS5_VABI sceFiosFHSeek(std::int32_t fh, std::int64_t offset, std::int32_t whence) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    return TransferResult(__func__, sceKernelLseek(RequireFile(__func__, fh).descriptor, offset, whence));
}

std::int64_t APS5_VABI sceFiosFHTell(std::int32_t fh) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    return TransferResult(__func__, sceKernelLseek(RequireFile(__func__, fh).descriptor, 0, 1));
}

std::int64_t APS5_VABI sceFiosFHGetSize(std::int32_t fh) {
    std::lock_guard lock(g_mutex);
    RequireInitialized(__func__);
    FileStat status{};
    TransferResult(__func__, sceKernelFstat(RequireFile(__func__, fh).descriptor, &status));
    return status.st_size;
}

const char* APS5_VABI sceFiosFHGetPath(std::int32_t fh) {
    std::lock_guard lock(g_mutex);
    const auto found = g_files.find(fh);
    return found == g_files.end() ? nullptr : found->second.path.c_str();
}

std::int32_t APS5_VABI sceFiosDHOpen(const FiosOpAttr* attr, std::int32_t* outDh, const char* path, FiosBuffer buffer) {
    (void)buffer;
    if (outDh == nullptr) ThrowUnknown(__func__, "the directory handle output is null");
    RequirePath(__func__, path);
    return Submit(__func__, attr, [&](Operation& operation) {
        TrackPath(operation, path);
        switch (Classify(__func__, path)) {
        case PathKind::Missing:
            operation.error = ErrorBadPath;
            *outDh = HandleInvalid;
            return;
        case PathKind::File:
            ThrowUnknown(__func__, std::string(path) + " is not a directory");
        case PathKind::Directory:
            break;
        }
        const std::int32_t dh = AllocateHandle();
        g_directories.emplace(dh, DirectoryHandle{path});
        *outDh = dh;
    });
}

int APS5_VABI sceFiosDHOpenSync(const FiosOpAttr* attr, std::int32_t* outDh, const char* path, FiosBuffer buffer) {
    return sceFiosOpSyncWait(sceFiosDHOpen(attr, outDh, path, buffer));
}

std::int32_t APS5_VABI sceFiosDHClose(const FiosOpAttr* attr, std::int32_t dh) {
    return Submit(__func__, attr, [&](Operation& operation) {
        if (g_directories.erase(dh) == 0) ThrowUnknown(__func__, "directory handle " + std::to_string(dh) + " is not open");
        operation.error = 0;
    });
}

int APS5_VABI sceFiosDHCloseSync(const FiosOpAttr* attr, std::int32_t dh) {
    return sceFiosOpSyncWait(sceFiosDHClose(attr, dh));
}

const char* APS5_VABI sceFiosDHGetPath(std::int32_t dh) {
    std::lock_guard lock(g_mutex);
    const auto found = g_directories.find(dh);
    return found == g_directories.end() ? nullptr : found->second.path.c_str();
}

int APS5_VABI sceFiosArchiveGetDecompressorThreadCount() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveGetMountBufferSize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveGetMountBufferSizeSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveMount() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveMountSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveMountWithOrder() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveMountWithOrderSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveSetDecompressorThreadCount() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveUnmount() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosArchiveUnmountSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCacheContainsFileRangeSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCacheContainsFileSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCacheFlushFileRangeSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCacheFlushFileSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCacheFlushSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCachePrefetchFH() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCachePrefetchFHRange() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCachePrefetchFHRangeSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCachePrefetchFHSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCachePrefetchFile() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCachePrefetchFileRange() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCachePrefetchFileRangeSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosCachePrefetchFileSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosClearTimeStamps() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDHRead() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDHReadSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDLLInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDLLTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDateFromComponents() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDateToComponents() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDeallocatePassthruFH() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDebugDumpDH() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDebugDumpDate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDebugDumpError() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDebugDumpFH() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDebugDumpOp() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDirectoryCreateWithMode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosDirectoryCreateWithModeSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFHGetOpenParams() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFHStat() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFHStatSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFHToFileno() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFileExists() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFileGetSize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFileWrite() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFileWriteSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosFilenoToFH() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosGetAllDHs() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosGetAllFHs() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosGetAllOps() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosGetDefaultOpAttr() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosGetGlobalDefaultOpAttr() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosGetThreadDefaultOpAttr() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosIOFilterAdd() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosIOFilterCache() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosIOFilterGetInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosIOFilterPsarcDearchiver() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosIOFilterRemove() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosOpRescheduleWithPriority() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosOpSetBuffer() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosOverlayAdd() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosOverlayGetInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosOverlayGetList() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosOverlayModify() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosOverlayRemove() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosOverlayResolveSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosPathNormalize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosPathcmp() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosPathncmp() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosPrintTimeStamps() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosPrintf() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosResolve() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosResolveSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosSaveTimeStamp() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosSetGlobalDefaultOpAttr() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosSetThreadDefaultOpAttr() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosShutdownAndCancelOps() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosStat() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosStatSync() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosTraceTimestamp() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosUpdateParameters() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceFiosVprintf() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
