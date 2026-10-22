#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI scePs2EmuMenuDialogInitialize(void);
int APS5_VABI scePs2EmuMenuDialogOpen(const void* param);
int APS5_VABI scePs2EmuMenuDialogUpdateStatus(void);
int APS5_VABI scePs2EmuMenuDialogGetStatus(void);
int APS5_VABI scePs2EmuMenuDialogGetResult(void* result);
int APS5_VABI scePs2EmuMenuDialogClose(void);
int APS5_VABI scePs2EmuMenuDialogTerminate(void);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr int kStatusNone = 0;
constexpr int kStatusInitialized = 1;
constexpr int kStatusFinished = 3;
constexpr int kErrNotInitialized = static_cast<int>(0x80B80003);
constexpr int kErrAlreadyInitialized = static_cast<int>(0x80B80004);
constexpr int kErrNotFinished = static_cast<int>(0x80B80005);
constexpr int kErrArgNull = static_cast<int>(0x80B8000D);
constexpr int kResultUserCanceled = 1;

}

int main() {
    std::int64_t param = 0;
    std::int32_t result[2] = {-1, -1};

    Require(scePs2EmuMenuDialogTerminate() == kErrNotInitialized);
    Require(scePs2EmuMenuDialogOpen(&param) == kErrNotInitialized);
    Require(scePs2EmuMenuDialogClose() == kErrNotInitialized);
    Require(scePs2EmuMenuDialogGetResult(result) == kErrNotInitialized);
    Require(scePs2EmuMenuDialogGetStatus() == kStatusNone);
    Require(scePs2EmuMenuDialogUpdateStatus() == kStatusNone);

    Require(scePs2EmuMenuDialogInitialize() == 0);
    Require(scePs2EmuMenuDialogInitialize() == kErrAlreadyInitialized);
    Require(scePs2EmuMenuDialogGetStatus() == kStatusInitialized);
    Require(scePs2EmuMenuDialogUpdateStatus() == kStatusInitialized);

    Require(scePs2EmuMenuDialogOpen(nullptr) == kErrArgNull);
    Require(scePs2EmuMenuDialogGetResult(result) == kErrNotFinished);

    Require(scePs2EmuMenuDialogOpen(&param) == 0);
    Require(scePs2EmuMenuDialogGetStatus() == kStatusFinished);
    Require(scePs2EmuMenuDialogUpdateStatus() == kStatusFinished);

    Require(scePs2EmuMenuDialogGetResult(nullptr) == kErrArgNull);
    Require(scePs2EmuMenuDialogGetResult(result) == 0);
    Require(result[0] == kResultUserCanceled);
    Require(result[1] == -1);

    Require(scePs2EmuMenuDialogClose() == 0);
    Require(scePs2EmuMenuDialogTerminate() == 0);
    Require(scePs2EmuMenuDialogTerminate() == kErrNotInitialized);
    Require(scePs2EmuMenuDialogGetStatus() == kStatusNone);
    Require(scePs2EmuMenuDialogUpdateStatus() == kStatusNone);

    Require(scePs2EmuMenuDialogInitialize() == 0);
    Require(scePs2EmuMenuDialogUpdateStatus() == kStatusInitialized);
    Require(scePs2EmuMenuDialogClose() == 0);
    Require(scePs2EmuMenuDialogUpdateStatus() == kStatusFinished);
    result[0] = -1;
    Require(scePs2EmuMenuDialogGetResult(result) == 0);
    Require(result[0] == kResultUserCanceled);
    Require(scePs2EmuMenuDialogTerminate() == 0);
}
