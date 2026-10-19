#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI scePlayerSelectionDialogInitialize(void);
int APS5_VABI scePlayerSelectionDialogOpen(const void* param);
int APS5_VABI scePlayerSelectionDialogUpdateStatus(void);
int APS5_VABI scePlayerSelectionDialogGetStatus(void);
int APS5_VABI scePlayerSelectionDialogGetResult(void* result);
int APS5_VABI scePlayerSelectionDialogClose(void);
int APS5_VABI scePlayerSelectionDialogTerminate(void);
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

struct DialogResult {
    std::int32_t result;
    std::int32_t userAction;
    std::uint64_t* playerList;
    std::uint32_t playerListLength;
    std::uint8_t reserved[36];
};
static_assert(sizeof(DialogResult) == 56);

}

int main() {
    std::int64_t param = 0;
    DialogResult result;
    std::memset(&result, 0xFF, sizeof(result));

    Require(scePlayerSelectionDialogTerminate() == kErrNotInitialized);
    Require(scePlayerSelectionDialogOpen(&param) == kErrNotInitialized);
    Require(scePlayerSelectionDialogClose() == kErrNotInitialized);
    Require(scePlayerSelectionDialogGetResult(&result) == kErrNotInitialized);
    Require(scePlayerSelectionDialogGetStatus() == kStatusNone);
    Require(scePlayerSelectionDialogUpdateStatus() == kStatusNone);

    Require(scePlayerSelectionDialogInitialize() == 0);
    Require(scePlayerSelectionDialogInitialize() == kErrAlreadyInitialized);
    Require(scePlayerSelectionDialogGetStatus() == kStatusInitialized);
    Require(scePlayerSelectionDialogUpdateStatus() == kStatusInitialized);

    Require(scePlayerSelectionDialogOpen(nullptr) == kErrArgNull);
    Require(scePlayerSelectionDialogGetResult(&result) == kErrNotFinished);

    Require(scePlayerSelectionDialogOpen(&param) == 0);
    Require(scePlayerSelectionDialogGetStatus() == kStatusFinished);
    Require(scePlayerSelectionDialogUpdateStatus() == kStatusFinished);

    Require(scePlayerSelectionDialogGetResult(nullptr) == kErrArgNull);
    Require(scePlayerSelectionDialogGetResult(&result) == 0);
    Require(result.result == 0);
    Require(result.userAction == kResultUserCanceled);
    Require(result.playerList == nullptr);
    Require(result.playerListLength == 0);
    Require(result.reserved[0] == 0xFF);

    Require(scePlayerSelectionDialogClose() == 0);
    Require(scePlayerSelectionDialogTerminate() == 0);
    Require(scePlayerSelectionDialogTerminate() == kErrNotInitialized);
    Require(scePlayerSelectionDialogGetStatus() == kStatusNone);
    Require(scePlayerSelectionDialogUpdateStatus() == kStatusNone);
    Require(scePlayerSelectionDialogClose() == kErrNotInitialized);
}
