#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "prx/libSceCommonDialog/CommonDialog.hpp"
#include "prx/libSceUserService/UserService.hpp"

extern "C" {
int APS5_VABI sceUserServiceInitialize(const UserServiceInitParam* param);
int APS5_VABI sceUserServiceTerminate();
int APS5_VABI scePlayerSelectionDialogInitialize(void);
int APS5_VABI scePlayerSelectionDialogOpen(const PlayerSelectionDialogParam* param);
int APS5_VABI scePlayerSelectionDialogUpdateStatus(void);
int APS5_VABI scePlayerSelectionDialogGetStatus(void);
int APS5_VABI scePlayerSelectionDialogGetResult(PlayerSelectionDialogResult* result);
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
    Require(sceUserServiceInitialize(nullptr) == 0);
    PlayerSelectionDialogParam param{};
    param.base.size = sizeof(param.base);
    param.base.magic = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&param)) - 1060003575u;
    param.size = sizeof(param);
    param.userId = USER_SERVICE_INITIAL_USER_ID;
    PlayerSelectionDialogResult result{-1, 99, nullptr, 77, 55};

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

    param.size = 0;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    param.size = sizeof(param);
    param.maxPlayers = 257;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    param.maxPlayers = 1;
    param.mode = 4;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    param.mode = 0;
    param.playerCount = 1;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    PlayerSelectionDialogPlayer player{};
    param.players = &player;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    player.accountId = 1;
    player.option0 = 1;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    param.maxPlayers = 2;
    player.option1 = 2;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    player.option1 = 1;
    player.mode = 3;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    player.mode = 2;
    player.reserved[63] = 1;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    player.reserved[63] = 0;
    param.reserved[63] = 1;
    Require(scePlayerSelectionDialogOpen(&param) == static_cast<int>(0x80B8000Au));
    param.reserved[63] = 0;
    Require(scePlayerSelectionDialogOpen(&param) == 0);
    Require(scePlayerSelectionDialogGetStatus() == kStatusFinished);
    Require(scePlayerSelectionDialogUpdateStatus() == kStatusFinished);

    Require(scePlayerSelectionDialogGetResult(nullptr) == kErrArgNull);
    Require(scePlayerSelectionDialogGetResult(&result) == kResultUserCanceled);
    Require(result.result == kResultUserCanceled);
    Require(result.option == 99 && result.accountIds == nullptr && result.count == 77 && result.padding == 55);

    Require(scePlayerSelectionDialogClose() == static_cast<int>(0x80B8000Bu));
    Require(scePlayerSelectionDialogTerminate() == 0);
    Require(scePlayerSelectionDialogTerminate() == kErrNotInitialized);
    Require(scePlayerSelectionDialogGetStatus() == kStatusNone);
    Require(scePlayerSelectionDialogUpdateStatus() == kStatusNone);
    Require(scePlayerSelectionDialogClose() == kErrNotInitialized);
    Require(sceUserServiceTerminate() == 0);
}
