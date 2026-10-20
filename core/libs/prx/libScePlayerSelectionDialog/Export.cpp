#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int COMMON_DIALOG_STATUS_NONE = 0;
constexpr int COMMON_DIALOG_STATUS_INITIALIZED = 1;
constexpr int COMMON_DIALOG_STATUS_RUNNING = 2;
constexpr int COMMON_DIALOG_STATUS_FINISHED = 3;
constexpr int COMMON_DIALOG_ERROR_NOT_INITIALIZED = static_cast<int>(0x80B80003u);
constexpr int COMMON_DIALOG_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80B80004u);
constexpr int COMMON_DIALOG_ERROR_NOT_FINISHED = static_cast<int>(0x80B80005u);
constexpr int COMMON_DIALOG_ERROR_BUSY = static_cast<int>(0x80B80007u);
constexpr int COMMON_DIALOG_ERROR_ARG_NULL = static_cast<int>(0x80B8000Du);
constexpr int COMMON_DIALOG_RESULT_USER_CANCELED = 1;

struct PlayerSelectionDialogResult {
 std::int32_t result;
 std::int32_t userAction;
 std::uint64_t* playerList;
 std::uint32_t playerListLength;
};
static_assert(offsetof(PlayerSelectionDialogResult, userAction) == 4);
static_assert(offsetof(PlayerSelectionDialogResult, playerList) == 8);
static_assert(offsetof(PlayerSelectionDialogResult, playerListLength) == 16);

constexpr std::uint32_t COMMON_DIALOG_MAGIC_NUMBER = 0xC0D1A109u;
constexpr std::int32_t USER_SERVICE_USER_ID_INVALID = -1;
constexpr std::uint32_t PLAYER_SELECTION_DIALOG_MAX_SELECTABLE = 256;

struct CommonDialogBaseParam {
 std::size_t size;
 std::uint8_t reserved[36];
 std::uint32_t magic;
};
static_assert(sizeof(CommonDialogBaseParam) == 48);

struct PlayerSelectionDialogParam {
 CommonDialogBaseParam baseParam;
 std::size_t size;
 std::int32_t userId;
 char dialogTitle[64];
 std::uint32_t maxSelectable;
 std::uint32_t behaviorOptions;
 std::uint64_t* initialPlayerStatusList;
 std::uint32_t initialPlayerStatusListLength;
 std::uint8_t reserved[64];
};
static_assert(offsetof(PlayerSelectionDialogParam, size) == 0x30);
static_assert(offsetof(PlayerSelectionDialogParam, userId) == 0x38);
static_assert(offsetof(PlayerSelectionDialogParam, maxSelectable) == 0x7C);
static_assert(offsetof(PlayerSelectionDialogParam, initialPlayerStatusList) == 0x88);
static_assert(sizeof(PlayerSelectionDialogParam) == 216);

std::atomic<int> g_status{COMMON_DIALOG_STATUS_NONE};

}

extern "C" {

int APS5_VABI scePlayerSelectionDialogInitialize(void) {
 int expected = COMMON_DIALOG_STATUS_NONE;
 if (!g_status.compare_exchange_strong(expected, COMMON_DIALOG_STATUS_INITIALIZED)) return COMMON_DIALOG_ERROR_ALREADY_INITIALIZED;
 return 0;
}

int APS5_VABI scePlayerSelectionDialogOpen(const void* param) {
 const int status = g_status.load();
 if (status == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
 if (status == COMMON_DIALOG_STATUS_RUNNING) return COMMON_DIALOG_ERROR_BUSY;
 if (param == nullptr) return COMMON_DIALOG_ERROR_ARG_NULL;
 g_status = COMMON_DIALOG_STATUS_FINISHED;
 return 0;
}

int APS5_VABI scePlayerSelectionDialogUpdateStatus(void) {
 return g_status.load();
}

int APS5_VABI scePlayerSelectionDialogGetStatus(void) {
 return g_status.load();
}

int APS5_VABI scePlayerSelectionDialogGetResult(void* result) {
 const int status = g_status.load();
 if (status == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
 if (result == nullptr) return COMMON_DIALOG_ERROR_ARG_NULL;
 if (status != COMMON_DIALOG_STATUS_FINISHED) return COMMON_DIALOG_ERROR_NOT_FINISHED;
 auto* out = static_cast<PlayerSelectionDialogResult*>(result);
 out->result = 0;
 out->userAction = COMMON_DIALOG_RESULT_USER_CANCELED;
 out->playerList = nullptr;
 out->playerListLength = 0;
 return 0;
}

int APS5_VABI scePlayerSelectionDialogClose(void) {
 if (g_status.load() == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
 g_status = COMMON_DIALOG_STATUS_FINISHED;
 return 0;
}

int APS5_VABI scePlayerSelectionDialogParamInitialize(void* param) {
 if (param == nullptr) return COMMON_DIALOG_ERROR_ARG_NULL;
 PlayerSelectionDialogParam init;
 std::memset(&init, 0, sizeof(init));
 init.baseParam.size = sizeof(CommonDialogBaseParam);
 init.baseParam.magic = static_cast<std::uint32_t>(COMMON_DIALOG_MAGIC_NUMBER + reinterpret_cast<std::uintptr_t>(static_cast<std::uint8_t*>(param) + offsetof(PlayerSelectionDialogParam, baseParam)));
 init.size = sizeof(PlayerSelectionDialogParam);
 init.userId = USER_SERVICE_USER_ID_INVALID;
 init.maxSelectable = PLAYER_SELECTION_DIALOG_MAX_SELECTABLE;
 std::memcpy(param, &init, sizeof(init));
 return 0;
}

int APS5_VABI scePlayerSelectionDialogParamInitialize(void* param) {
 if (param == nullptr) return COMMON_DIALOG_ERROR_ARG_NULL;
 NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI scePlayerSelectionDialogTerminate(void) {
 if (g_status.exchange(COMMON_DIALOG_STATUS_NONE) == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
 return 0;
}

}
