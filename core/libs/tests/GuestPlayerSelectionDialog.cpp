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
int APS5_VABI scePlayerSelectionDialogParamInitialize(void* param);
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

void ParamInitialize() {
    alignas(8) std::uint8_t param[232];
    std::memset(param, 0x5A, sizeof(param));
    Require(scePlayerSelectionDialogParamInitialize(nullptr) == kErrArgNull);
    Require(scePlayerSelectionDialogParamInitialize(param) == 0);
    std::uint64_t baseSize = 0, size = 0;
    std::uint32_t magic = 0, maxSelectable = 0;
    std::int32_t userId = 0;
    std::memcpy(&baseSize, param, sizeof(baseSize));
    std::memcpy(&magic, param + 0x2C, sizeof(magic));
    std::memcpy(&size, param + 0x30, sizeof(size));
    std::memcpy(&userId, param + 0x38, sizeof(userId));
    std::memcpy(&maxSelectable, param + 0x7C, sizeof(maxSelectable));
    Require(baseSize == 48 && size == 216 && userId == -1 && maxSelectable == 256);
    Require(magic == static_cast<std::uint32_t>(0xC0D1A109u + reinterpret_cast<std::uintptr_t>(param)));
    for (std::size_t offset = 0x08; offset < 0x2C; ++offset) Require(param[offset] == 0);
    for (std::size_t offset = 0x3C; offset < 0x7C; ++offset) Require(param[offset] == 0);
    for (std::size_t offset = 0x80; offset < 216; ++offset) Require(param[offset] == 0);
    for (std::size_t offset = 216; offset < sizeof(param); ++offset) Require(param[offset] == 0x5A);
}

}

int main() {
    ParamInitialize();
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
