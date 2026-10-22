#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

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

struct FiosOpenParams {
    std::uint16_t openFlags;
    std::uint16_t opFlags;
    std::uint32_t reserved;
    FiosBuffer buffer;
};

}

extern "C" {
int APS5_VABI sceFiosInitialize(const void* parameters);
void APS5_VABI sceFiosTerminate(void);
bool APS5_VABI sceFiosIsInitialized(void* outParameters);
void APS5_VABI sceFiosSuspend(void);
void APS5_VABI sceFiosResume(void);
std::uint32_t APS5_VABI sceFiosGetSuspendCount(void);
bool APS5_VABI sceFiosIsSuspended(void);
bool APS5_VABI sceFiosIsIdle(void);
void APS5_VABI sceFiosCancelAllOps(void);
void APS5_VABI sceFiosCloseAllFiles(void);
bool APS5_VABI sceFiosIsValidHandle(std::int32_t handle);
bool APS5_VABI sceFiosOpIsDone(std::int32_t op);
int APS5_VABI sceFiosOpWait(std::int32_t op);
int APS5_VABI sceFiosOpWaitUntil(std::int32_t op, std::int64_t deadline);
int APS5_VABI sceFiosOpGetError(std::int32_t op);
int APS5_VABI sceFiosOpSyncWait(std::int32_t op);
std::int64_t APS5_VABI sceFiosOpSyncWaitForIO(std::int32_t op);
int APS5_VABI sceFiosOpDelete(std::int32_t op);
void APS5_VABI sceFiosOpCancel(std::int32_t op);
bool APS5_VABI sceFiosOpIsCancelled(std::int32_t op);
const FiosOpAttr* APS5_VABI sceFiosOpGetAttr(std::int32_t op);
void APS5_VABI sceFiosOpReschedule(std::int32_t op, std::int64_t newDeadline);
const char* APS5_VABI sceFiosOpGetPath(std::int32_t op);
void* APS5_VABI sceFiosOpGetBuffer(std::int32_t op);
std::int64_t APS5_VABI sceFiosOpGetOffset(std::int32_t op);
std::int64_t APS5_VABI sceFiosOpGetRequestCount(std::int32_t op);
std::int64_t APS5_VABI sceFiosOpGetActualCount(std::int32_t op);
std::int64_t APS5_VABI sceFiosTimeIntervalFromNanoseconds(std::int64_t ns);
std::int64_t APS5_VABI sceFiosTimeIntervalToNanoseconds(std::int64_t interval);
std::int64_t APS5_VABI sceFiosTimeGetCurrent(void);
std::int64_t APS5_VABI sceFiosDateGetCurrent(void);
std::int32_t APS5_VABI sceFiosExists(const FiosOpAttr* attr, const char* path, bool* outExists);
bool APS5_VABI sceFiosExistsSync(const FiosOpAttr* attr, const char* path);
std::int32_t APS5_VABI sceFiosDirectoryExists(const FiosOpAttr* attr, const char* path, bool* outExists);
bool APS5_VABI sceFiosDirectoryExistsSync(const FiosOpAttr* attr, const char* path);
bool APS5_VABI sceFiosFileExistsSync(const FiosOpAttr* attr, const char* path);
std::int64_t APS5_VABI sceFiosFileGetSizeSync(const FiosOpAttr* attr, const char* path);
std::int32_t APS5_VABI sceFiosFileDelete(const FiosOpAttr* attr, const char* path);
int APS5_VABI sceFiosFileDeleteSync(const FiosOpAttr* attr, const char* path);
std::int32_t APS5_VABI sceFiosDirectoryCreate(const FiosOpAttr* attr, const char* path);
int APS5_VABI sceFiosDirectoryCreateSync(const FiosOpAttr* attr, const char* path);
std::int32_t APS5_VABI sceFiosDirectoryDelete(const FiosOpAttr* attr, const char* path);
int APS5_VABI sceFiosDirectoryDeleteSync(const FiosOpAttr* attr, const char* path);
std::int32_t APS5_VABI sceFiosDelete(const FiosOpAttr* attr, const char* path);
int APS5_VABI sceFiosDeleteSync(const FiosOpAttr* attr, const char* path);
std::int32_t APS5_VABI sceFiosRename(const FiosOpAttr* attr, const char* oldPath, const char* newPath);
int APS5_VABI sceFiosRenameSync(const FiosOpAttr* attr, const char* oldPath, const char* newPath);
std::int32_t APS5_VABI sceFiosFileRead(const FiosOpAttr* attr, const char* path, void* buffer, std::int64_t length, std::int64_t offset);
std::int64_t APS5_VABI sceFiosFileReadSync(const FiosOpAttr* attr, const char* path, void* buffer, std::int64_t length, std::int64_t offset);
std::int32_t APS5_VABI sceFiosFileTruncate(const FiosOpAttr* attr, const char* path, std::int64_t length);
int APS5_VABI sceFiosFileTruncateSync(const FiosOpAttr* attr, const char* path, std::int64_t length);
std::int32_t APS5_VABI sceFiosFHOpenWithMode(const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params, std::int32_t nativeMode);
int APS5_VABI sceFiosFHOpenWithModeSync(const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params, std::int32_t nativeMode);
std::int32_t APS5_VABI sceFiosFHOpen(const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params);
int APS5_VABI sceFiosFHOpenSync(const FiosOpAttr* attr, std::int32_t* outFh, const char* path, const FiosOpenParams* params);
std::int32_t APS5_VABI sceFiosFHClose(const FiosOpAttr* attr, std::int32_t fh);
int APS5_VABI sceFiosFHCloseSync(const FiosOpAttr* attr, std::int32_t fh);
std::int32_t APS5_VABI sceFiosFHRead(const FiosOpAttr* attr, std::int32_t fh, void* buffer, std::int64_t length);
std::int64_t APS5_VABI sceFiosFHReadSync(const FiosOpAttr* attr, std::int32_t fh, void* buffer, std::int64_t length);
std::int32_t APS5_VABI sceFiosFHWrite(const FiosOpAttr* attr, std::int32_t fh, const void* buffer, std::int64_t length);
std::int64_t APS5_VABI sceFiosFHWriteSync(const FiosOpAttr* attr, std::int32_t fh, const void* buffer, std::int64_t length);
std::int32_t APS5_VABI sceFiosFHPread(const FiosOpAttr* attr, std::int32_t fh, void* buffer, std::int64_t length, std::int64_t offset);
std::int64_t APS5_VABI sceFiosFHPreadSync(const FiosOpAttr* attr, std::int32_t fh, void* buffer, std::int64_t length, std::int64_t offset);
std::int32_t APS5_VABI sceFiosFHPwrite(const FiosOpAttr* attr, std::int32_t fh, const void* buffer, std::int64_t length, std::int64_t offset);
std::int64_t APS5_VABI sceFiosFHPwriteSync(const FiosOpAttr* attr, std::int32_t fh, const void* buffer, std::int64_t length, std::int64_t offset);
std::int32_t APS5_VABI sceFiosFHReadv(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt);
std::int64_t APS5_VABI sceFiosFHReadvSync(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt);
std::int32_t APS5_VABI sceFiosFHWritev(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt);
std::int64_t APS5_VABI sceFiosFHWritevSync(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt);
std::int32_t APS5_VABI sceFiosFHPreadv(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt, std::int64_t offset);
std::int64_t APS5_VABI sceFiosFHPreadvSync(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt, std::int64_t offset);
std::int32_t APS5_VABI sceFiosFHPwritev(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt, std::int64_t offset);
std::int64_t APS5_VABI sceFiosFHPwritevSync(const FiosOpAttr* attr, std::int32_t fh, const FiosBuffer* iov, int iovcnt, std::int64_t offset);
std::int32_t APS5_VABI sceFiosFHTruncate(const FiosOpAttr* attr, std::int32_t fh, std::int64_t length);
int APS5_VABI sceFiosFHTruncateSync(const FiosOpAttr* attr, std::int32_t fh, std::int64_t length);
std::int32_t APS5_VABI sceFiosFHSync(const FiosOpAttr* attr, std::int32_t fh);
int APS5_VABI sceFiosFHSyncSync(const FiosOpAttr* attr, std::int32_t fh);
std::int64_t APS5_VABI sceFiosFHSeek(std::int32_t fh, std::int64_t offset, std::int32_t whence);
std::int64_t APS5_VABI sceFiosFHTell(std::int32_t fh);
std::int64_t APS5_VABI sceFiosFHGetSize(std::int32_t fh);
const char* APS5_VABI sceFiosFHGetPath(std::int32_t fh);
std::int32_t APS5_VABI sceFiosDHOpen(const FiosOpAttr* attr, std::int32_t* outDh, const char* path, FiosBuffer buffer);
int APS5_VABI sceFiosDHOpenSync(const FiosOpAttr* attr, std::int32_t* outDh, const char* path, FiosBuffer buffer);
std::int32_t APS5_VABI sceFiosDHClose(const FiosOpAttr* attr, std::int32_t dh);
int APS5_VABI sceFiosDHCloseSync(const FiosOpAttr* attr, std::int32_t dh);
const char* APS5_VABI sceFiosDHGetPath(std::int32_t dh);
}

namespace {

constexpr int ErrorBadPath = static_cast<int>(0x80820005u);
constexpr int ErrorBadOp = static_cast<int>(0x8082000Au);
constexpr std::uint16_t OpenRead = 0x1;
constexpr std::uint16_t OpenWrite = 0x2;
constexpr std::uint16_t OpenAppend = 0x4;
constexpr std::uint16_t OpenCreate = 0x8;
constexpr std::uint16_t OpenTruncate = 0x10;

void Check(bool value, int line) {
    if (value) return;
    std::fprintf(stderr, "fios2 check failed at line %d\n", line);
    std::abort();
}
#define Require(value) Check((value), __LINE__)

template <typename TAction>
bool Throws(TAction action) {
    try {
        action();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

struct CallbackLog {
    int calls = 0;
    std::int32_t op = 0;
    std::uint8_t event = 0;
    int error = 0;
};

int APS5_VABI Record(void* context, std::int32_t op, std::uint8_t event, int error) {
    auto* log = static_cast<CallbackLog*>(context);
    ++log->calls;
    log->op = op;
    log->event = event;
    log->error = error;
    return 0;
}

std::string Contents(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

void Write(const std::filesystem::path& path, const char* text) {
    std::ofstream(path, std::ios::binary) << text;
}

}

int main() {
    const std::string root = "/fios2-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const std::filesystem::path host = ResolvePath_nid_no_patch(root.c_str());
    Require(std::filesystem::create_directories(host));
    Write(host / "data.bin", "0123456789");
    const std::string data = root + "/data.bin";
    const std::string missing = root + "/missing.bin";
    const std::string created = root + "/created.bin";
    const std::string renamed = root + "/renamed.bin";
    const std::string sub = root + "/sub";

    Require(!sceFiosIsInitialized(nullptr));
    Require(Throws([&] { sceFiosExistsSync(nullptr, data.c_str()); }));
    Require(Throws([] { sceFiosInitialize(nullptr); }));
    unsigned char parameters[128] = {};
    Require(sceFiosInitialize(parameters) == 0);
    Require(Throws([&] { sceFiosInitialize(parameters); }));
    Require(sceFiosIsInitialized(nullptr));
    Require(Throws([&] { sceFiosIsInitialized(parameters); }));

    Require(sceFiosTimeIntervalFromNanoseconds(1234) == 1234);
    Require(sceFiosTimeIntervalToNanoseconds(5678) == 5678);
    const std::int64_t before = sceFiosTimeGetCurrent();
    Require(sceFiosTimeGetCurrent() >= before);
    const std::int64_t seconds = sceFiosDateGetCurrent() / 1000000000;
    const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
    Require(seconds >= now - 5 && seconds <= now + 5);

    Require(sceFiosExistsSync(nullptr, data.c_str()));
    Require(sceFiosExistsSync(nullptr, root.c_str()));
    Require(!sceFiosExistsSync(nullptr, missing.c_str()));
    Require(sceFiosFileExistsSync(nullptr, data.c_str()));
    Require(!sceFiosFileExistsSync(nullptr, root.c_str()));
    Require(!sceFiosFileExistsSync(nullptr, missing.c_str()));
    Require(sceFiosDirectoryExistsSync(nullptr, root.c_str()));
    Require(!sceFiosDirectoryExistsSync(nullptr, data.c_str()));
    Require(!sceFiosDirectoryExistsSync(nullptr, missing.c_str()));
    Require(Throws([] { sceFiosExistsSync(nullptr, nullptr); }));

    bool exists = false;
    std::int32_t op = sceFiosExists(nullptr, data.c_str(), &exists);
    Require(exists);
    Require(sceFiosIsValidHandle(op));
    Require(sceFiosOpIsDone(op));
    Require(sceFiosOpGetError(op) == 0);
    Require(std::strcmp(sceFiosOpGetPath(op), data.c_str()) == 0);
    Require(Throws([&] { sceFiosOpGetBuffer(op); }));
    Require(Throws([&] { sceFiosOpGetOffset(op); }));
    Require(Throws([&] { sceFiosOpGetRequestCount(op); }));
    Require(Throws([&] { sceFiosOpGetActualCount(op); }));
    Require(sceFiosOpDelete(op) == 0);
    Require(!sceFiosIsValidHandle(op));
    Require(!sceFiosOpIsDone(op));
    Require(sceFiosOpGetError(op) == ErrorBadOp);
    Require(sceFiosOpWait(op) == ErrorBadOp);
    Require(sceFiosOpSyncWait(op) == ErrorBadOp);
    Require(sceFiosOpSyncWaitForIO(op) == ErrorBadOp);
    Require(sceFiosOpGetActualCount(op) == ErrorBadOp);
    Require(sceFiosOpDelete(op) == ErrorBadOp);
    Require(Throws([&] { sceFiosOpCancel(op); }));
    Require(Throws([&] { sceFiosOpIsCancelled(op); }));
    Require(Throws([&] { sceFiosOpGetAttr(op); }));
    exists = true;
    Require(sceFiosOpSyncWait(sceFiosDirectoryExists(nullptr, data.c_str(), &exists)) == 0 && !exists);
    Require(sceFiosOpSyncWait(sceFiosDirectoryExists(nullptr, root.c_str(), &exists)) == 0 && exists);

    Require(sceFiosFileGetSizeSync(nullptr, data.c_str()) == 10);
    Require(sceFiosFileGetSizeSync(nullptr, missing.c_str()) == ErrorBadPath);
    Require(Throws([&] { sceFiosFileGetSizeSync(nullptr, root.c_str()); }));

    CallbackLog log;
    FiosOpAttr attr{};
    attr.callback = Record;
    attr.callbackContext = &log;
    attr.userTag = 7;
    std::int32_t fh = -1;
    op = sceFiosFHOpen(&attr, &fh, data.c_str(), nullptr);
    Require(log.calls == 1 && log.op == op && log.event == 1 && log.error == 0);
    Require(sceFiosOpWait(op) == 0);
    Require(sceFiosOpWaitUntil(op, 0) == 0);
    Require(sceFiosOpGetAttr(op)->userTag == 7 && sceFiosOpGetAttr(op)->callbackContext == &log);
    sceFiosOpReschedule(op, 99);
    Require(sceFiosOpGetAttr(op)->deadline == 99);
    sceFiosOpCancel(op);
    Require(!sceFiosOpIsCancelled(op));
    Require(sceFiosOpWait(op) == 0);
    Require(sceFiosOpSyncWait(op) == 0);
    Require(!sceFiosIsValidHandle(op));
    Require(sceFiosIsValidHandle(fh));
    Require(std::strcmp(sceFiosFHGetPath(fh), data.c_str()) == 0);
    Require(sceFiosFHGetSize(fh) == 10);

    char buffer[32] = {};
    Require(sceFiosFHReadSync(nullptr, fh, buffer, 4) == 4 && std::memcmp(buffer, "0123", 4) == 0);
    Require(sceFiosFHTell(fh) == 4);
    op = sceFiosFHRead(nullptr, fh, buffer, 3);
    Require(sceFiosOpGetRequestCount(op) == 3);
    Require(sceFiosOpGetActualCount(op) == 3);
    Require(sceFiosOpGetOffset(op) == 4);
    Require(sceFiosOpGetBuffer(op) == buffer);
    Require(Throws([&] { sceFiosOpGetPath(op); }));
    Require(sceFiosOpSyncWaitForIO(op) == 3 && std::memcmp(buffer, "456", 3) == 0);
    Require(sceFiosFHSeek(fh, 1, 0) == 1);
    Require(sceFiosFHReadSync(nullptr, fh, buffer, 20) == 9 && std::memcmp(buffer, "123456789", 9) == 0);
    Require(sceFiosFHReadSync(nullptr, fh, buffer, 20) == 0);
    Require(sceFiosFHSeek(fh, -2, 2) == 8);
    Require(sceFiosFHPreadSync(nullptr, fh, buffer, 2, 3) == 2 && std::memcmp(buffer, "34", 2) == 0);
    Require(sceFiosFHTell(fh) == 8);
    op = sceFiosFHPread(nullptr, fh, buffer, 4, 6);
    Require(sceFiosOpGetOffset(op) == 6 && sceFiosOpGetRequestCount(op) == 4 && sceFiosOpGetActualCount(op) == 4);
    Require(sceFiosOpSyncWaitForIO(op) == 4 && std::memcmp(buffer, "6789", 4) == 0);
    char first[2] = {};
    char second[3] = {};
    const FiosBuffer pieces[2] = {{first, sizeof(first)}, {second, sizeof(second)}};
    Require(sceFiosFHSeek(fh, 0, 0) == 0);
    Require(sceFiosFHReadvSync(nullptr, fh, pieces, 2) == 5 && std::memcmp(first, "01", 2) == 0 && std::memcmp(second, "234", 3) == 0);
    op = sceFiosFHReadv(nullptr, fh, pieces, 2);
    Require(sceFiosOpGetOffset(op) == 5 && sceFiosOpGetRequestCount(op) == 5);
    Require(sceFiosOpSyncWaitForIO(op) == 5 && std::memcmp(first, "56", 2) == 0);
    Require(sceFiosFHPreadvSync(nullptr, fh, pieces, 2, 2) == 5 && std::memcmp(first, "23", 2) == 0 && std::memcmp(second, "456", 3) == 0);
    op = sceFiosFHPreadv(nullptr, fh, pieces, 2, 7);
    Require(sceFiosOpGetOffset(op) == 7 && sceFiosOpSyncWaitForIO(op) == 3 && std::memcmp(first, "78", 2) == 0);
    Require(Throws([&] { sceFiosFHReadSync(nullptr, fh, buffer, -1); }));
    Require(sceFiosFHCloseSync(nullptr, fh) == 0);
    Require(!sceFiosIsValidHandle(fh));
    Require(sceFiosFHGetPath(fh) == nullptr);
    Require(Throws([&] { sceFiosFHReadSync(nullptr, fh, buffer, 1); }));
    Require(Throws([&] { sceFiosFHCloseSync(nullptr, fh); }));
    Require(Throws([&] { sceFiosFHTell(fh); }));

    fh = 5;
    Require(sceFiosFHOpenSync(nullptr, &fh, missing.c_str(), nullptr) == ErrorBadPath && fh == 0);
    log = {};
    op = sceFiosFHOpen(&attr, &fh, missing.c_str(), nullptr);
    Require(log.calls == 1 && log.error == ErrorBadPath && fh == 0);
    Require(sceFiosOpGetError(op) == ErrorBadPath);
    Require(sceFiosOpSyncWaitForIO(op) == ErrorBadPath);
    Require(Throws([&] { sceFiosFHOpenSync(nullptr, &fh, root.c_str(), nullptr); }));
    Require(Throws([&] { sceFiosFHOpenSync(nullptr, nullptr, data.c_str(), nullptr); }));
    Require(Throws([&] { sceFiosFHClose(&attr, -1234); }));

    FiosOpenParams params{};
    params.openFlags = OpenRead | OpenWrite | OpenCreate | OpenTruncate;
    Require(sceFiosFHOpenWithModeSync(nullptr, &fh, created.c_str(), &params, 0644) == 0);
    Require(sceFiosFHWriteSync(nullptr, fh, "abcdef", 6) == 6);
    op = sceFiosFHWrite(nullptr, fh, "gh", 2);
    Require(sceFiosOpGetOffset(op) == 6 && sceFiosOpSyncWaitForIO(op) == 2);
    Require(sceFiosFHPwriteSync(nullptr, fh, "XY", 2, 1) == 2);
    op = sceFiosFHPwrite(nullptr, fh, "Z", 1, 0);
    Require(sceFiosOpGetOffset(op) == 0 && sceFiosOpSyncWaitForIO(op) == 1);
    char left[2] = {'1', '2'};
    char right[1] = {'3'};
    const FiosBuffer outputs[2] = {{left, sizeof(left)}, {right, sizeof(right)}};
    Require(sceFiosFHWritevSync(nullptr, fh, outputs, 2) == 3);
    op = sceFiosFHWritev(nullptr, fh, outputs, 2);
    Require(sceFiosOpGetOffset(op) == 11 && sceFiosOpSyncWaitForIO(op) == 3);
    Require(sceFiosFHPwritevSync(nullptr, fh, outputs, 2, 3) == 3);
    op = sceFiosFHPwritev(nullptr, fh, outputs, 1, 6);
    Require(sceFiosOpSyncWaitForIO(op) == 2);
    Require(sceFiosFHSyncSync(nullptr, fh) == 0);
    Require(sceFiosOpSyncWait(sceFiosFHSync(nullptr, fh)) == 0);
    Require(sceFiosFHGetSize(fh) == 14);
    Require(sceFiosFHTruncateSync(nullptr, fh, 10) == 0);
    Require(sceFiosFHGetSize(fh) == 10);
    Require(sceFiosOpSyncWait(sceFiosFHTruncate(nullptr, fh, 9)) == 0);
    Require(sceFiosFHGetSize(fh) == 9);
    Require(sceFiosOpSyncWait(sceFiosFHClose(nullptr, fh)) == 0);
    Require(Contents(host / "created.bin") == "ZXY123121");

    params.openFlags = OpenWrite | OpenAppend;
    Require(sceFiosFHOpenWithModeSync(nullptr, &fh, created.c_str(), &params, -1) == 0);
    Require(sceFiosFHWriteSync(nullptr, fh, "!", 1) == 1);
    Require(sceFiosFHCloseSync(nullptr, fh) == 0);
    Require(Contents(host / "created.bin") == "ZXY123121!");
    params.openFlags = OpenWrite;
    op = sceFiosFHOpenWithMode(nullptr, &fh, created.c_str(), &params, -1);
    Require(sceFiosOpSyncWait(op) == 0);
    Require(sceFiosFHWriteSync(nullptr, fh, "z", 1) == 1);
    Require(sceFiosFHCloseSync(nullptr, fh) == 0);
    Require(Contents(host / "created.bin") == "zXY123121!");
    Require(sceFiosFHOpenSync(nullptr, &fh, created.c_str(), nullptr) == 0);
    Require(Throws([&] { sceFiosFHWriteSync(nullptr, fh, "q", 1); }));
    Require(sceFiosFHCloseSync(nullptr, fh) == 0);

    Require(sceFiosFileReadSync(nullptr, data.c_str(), buffer, 3, 2) == 3 && std::memcmp(buffer, "234", 3) == 0);
    Require(sceFiosFileReadSync(nullptr, data.c_str(), buffer, 8, 7) == 3 && std::memcmp(buffer, "789", 3) == 0);
    Require(sceFiosFileReadSync(nullptr, missing.c_str(), buffer, 3, 0) == ErrorBadPath);
    op = sceFiosFileRead(nullptr, data.c_str(), buffer, 2, 5);
    Require(sceFiosOpGetOffset(op) == 5 && sceFiosOpGetRequestCount(op) == 2 && sceFiosOpGetActualCount(op) == 2);
    Require(std::strcmp(sceFiosOpGetPath(op), data.c_str()) == 0 && sceFiosOpGetBuffer(op) == buffer);
    Require(sceFiosOpSyncWaitForIO(op) == 2 && std::memcmp(buffer, "56", 2) == 0);

    Require(sceFiosFileTruncateSync(nullptr, created.c_str(), 4) == 0);
    Require(Contents(host / "created.bin") == "zXY1");
    Require(sceFiosOpSyncWait(sceFiosFileTruncate(nullptr, created.c_str(), 3)) == 0);
    Require(Contents(host / "created.bin") == "zXY");
    Require(sceFiosFileTruncateSync(nullptr, missing.c_str(), 1) == ErrorBadPath);

    Require(sceFiosRenameSync(nullptr, created.c_str(), renamed.c_str()) == 0);
    Require(!sceFiosExistsSync(nullptr, created.c_str()) && sceFiosFileExistsSync(nullptr, renamed.c_str()));
    Require(sceFiosRenameSync(nullptr, missing.c_str(), created.c_str()) == ErrorBadPath);
    Require(sceFiosOpSyncWait(sceFiosRename(nullptr, renamed.c_str(), created.c_str())) == 0);
    Require(sceFiosFileDeleteSync(nullptr, created.c_str()) == 0);
    Require(sceFiosFileDeleteSync(nullptr, created.c_str()) == ErrorBadPath);
    Write(host / "created.bin", "x");
    Require(sceFiosOpSyncWait(sceFiosFileDelete(nullptr, created.c_str())) == 0);
    Require(!std::filesystem::exists(host / "created.bin"));

    Require(sceFiosDirectoryCreateSync(nullptr, sub.c_str()) == 0);
    Require(sceFiosDirectoryExistsSync(nullptr, sub.c_str()));
    Require(Throws([&] { sceFiosDirectoryCreateSync(nullptr, sub.c_str()); }));
    Require(Throws([&] { sceFiosDirectoryCreateSync(nullptr, (root + "/absent/child").c_str()); }));
    std::int32_t dh = -1;
    Require(sceFiosDHOpenSync(nullptr, &dh, sub.c_str(), FiosBuffer{nullptr, 0}) == 0);
    Require(sceFiosIsValidHandle(dh));
    Require(std::strcmp(sceFiosDHGetPath(dh), sub.c_str()) == 0);
    Require(sceFiosDHCloseSync(nullptr, dh) == 0);
    Require(!sceFiosIsValidHandle(dh) && sceFiosDHGetPath(dh) == nullptr);
    Require(Throws([&] { sceFiosDHCloseSync(nullptr, dh); }));
    Require(sceFiosDHOpenSync(nullptr, &dh, missing.c_str(), FiosBuffer{nullptr, 0}) == ErrorBadPath && dh == 0);
    Require(Throws([&] { sceFiosDHOpenSync(nullptr, &dh, data.c_str(), FiosBuffer{nullptr, 0}); }));
    log = {};
    op = sceFiosDHOpen(&attr, &dh, root.c_str(), FiosBuffer{buffer, sizeof(buffer)});
    Require(log.calls == 1 && log.op == op && sceFiosOpSyncWait(op) == 0);
    Require(sceFiosOpSyncWait(sceFiosDHClose(nullptr, dh)) == 0);
    Require(sceFiosDirectoryDeleteSync(nullptr, sub.c_str()) == 0);
    Require(sceFiosDirectoryDeleteSync(nullptr, sub.c_str()) == ErrorBadPath);
    Require(sceFiosOpSyncWait(sceFiosDirectoryCreate(nullptr, sub.c_str())) == 0);
    Require(sceFiosOpSyncWait(sceFiosDirectoryDelete(nullptr, sub.c_str())) == 0);
    Require(!std::filesystem::exists(host / "sub"));

    Require(sceFiosDirectoryCreateSync(nullptr, sub.c_str()) == 0);
    Write(host / "doomed.bin", "x");
    Require(sceFiosDeleteSync(nullptr, (root + "/doomed.bin").c_str()) == 0);
    Require(sceFiosDeleteSync(nullptr, sub.c_str()) == 0);
    Require(sceFiosDeleteSync(nullptr, sub.c_str()) == ErrorBadPath);
    Require(sceFiosOpSyncWait(sceFiosDelete(nullptr, sub.c_str())) == ErrorBadPath);
    Require(!std::filesystem::exists(host / "sub") && !std::filesystem::exists(host / "doomed.bin"));

    Require(!sceFiosIsSuspended() && sceFiosGetSuspendCount() == 0);
    sceFiosSuspend();
    sceFiosSuspend();
    Require(sceFiosIsSuspended() && sceFiosGetSuspendCount() == 2);
    Require(Throws([&] { sceFiosExistsSync(nullptr, data.c_str()); }));
    sceFiosResume();
    sceFiosResume();
    Require(!sceFiosIsSuspended() && sceFiosGetSuspendCount() == 0);
    Require(Throws([] { sceFiosResume(); }));
    Require(sceFiosIsIdle());
    op = sceFiosExists(nullptr, data.c_str(), nullptr);
    sceFiosCancelAllOps();
    Require(sceFiosOpIsDone(op) && !sceFiosOpIsCancelled(op) && sceFiosOpSyncWait(op) == 0);

    Require(sceFiosFHOpenSync(nullptr, &fh, data.c_str(), nullptr) == 0);
    std::int32_t other = -1;
    Require(sceFiosFHOpenSync(nullptr, &other, data.c_str(), nullptr) == 0 && other != fh);
    sceFiosCloseAllFiles();
    Require(!sceFiosIsValidHandle(fh) && !sceFiosIsValidHandle(other));

    Require(sceFiosFHOpenSync(nullptr, &fh, data.c_str(), nullptr) == 0);
    Require(sceFiosDHOpenSync(nullptr, &dh, root.c_str(), FiosBuffer{nullptr, 0}) == 0);
    op = sceFiosExists(nullptr, data.c_str(), nullptr);
    sceFiosTerminate();
    Require(!sceFiosIsInitialized(nullptr));
    Require(!sceFiosIsValidHandle(fh) && !sceFiosIsValidHandle(dh) && !sceFiosIsValidHandle(op));
    Require(Throws([] { sceFiosTerminate(); }));
    Require(Throws([] { sceFiosSuspend(); }));
    Require(Throws([] { sceFiosIsIdle(); }));
    Require(Throws([] { sceFiosCloseAllFiles(); }));
    Require(sceFiosInitialize(parameters) == 0);
    sceFiosTerminate();

    std::filesystem::remove_all(host);
    return 0;
}
