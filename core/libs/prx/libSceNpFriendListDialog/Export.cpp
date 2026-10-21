#include <cstdint>
#include <cstddef>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int COMMON_DIALOG_STATUS_NONE = 0;
constexpr int COMMON_DIALOG_STATUS_INITIALIZED = 1;
constexpr int COMMON_DIALOG_STATUS_RUNNING = 2;
constexpr int COMMON_DIALOG_STATUS_FINISHED = 3;
constexpr int COMMON_DIALOG_ERROR_NOT_INITIALIZED = static_cast<int>(0x80B80003u);
constexpr int COMMON_DIALOG_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80B80004u);
constexpr int COMMON_DIALOG_ERROR_INVALID_STATE = static_cast<int>(0x80B80006u);
constexpr int COMMON_DIALOG_ERROR_NOT_RUNNING = static_cast<int>(0x80B8000Bu);
constexpr int COMMON_DIALOG_ERROR_ARG_NULL = static_cast<int>(0x80B8000Du);

std::mutex g_mutex;
int g_status = COMMON_DIALOG_STATUS_NONE;

}

extern "C" {

int APS5_VABI sceNpFriendListDialogClose(void) {
    std::lock_guard lock(g_mutex);
    if (g_status == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    if (g_status != COMMON_DIALOG_STATUS_RUNNING) return COMMON_DIALOG_ERROR_NOT_RUNNING;
    g_status = COMMON_DIALOG_STATUS_FINISHED;
    return 0;
}

int APS5_VABI sceNpFriendListDialogGetResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpFriendListDialogGetResultA() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpFriendListDialogGetStatus(void) {
    std::lock_guard lock(g_mutex);
    return g_status;
}

int APS5_VABI sceNpFriendListDialogInitialize(void) {
    std::lock_guard lock(g_mutex);
    if (g_status != COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_ALREADY_INITIALIZED;
    g_status = COMMON_DIALOG_STATUS_INITIALIZED;
    return 0;
}

int APS5_VABI sceNpFriendListDialogOpen() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpFriendListDialogOpenA(const void* param) {
    std::lock_guard lock(g_mutex);
    if (g_status != COMMON_DIALOG_STATUS_INITIALIZED && g_status != COMMON_DIALOG_STATUS_FINISHED) return COMMON_DIALOG_ERROR_INVALID_STATE;
    if (!param) return COMMON_DIALOG_ERROR_ARG_NULL;
    g_status = COMMON_DIALOG_STATUS_FINISHED;
    return 0;
}

int APS5_VABI sceNpFriendListDialogTerminate(void) {
    std::lock_guard lock(g_mutex);
    if (g_status == COMMON_DIALOG_STATUS_NONE) return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    g_status = COMMON_DIALOG_STATUS_NONE;
    return 0;
}

int APS5_VABI sceNpFriendListDialogUpdateStatus(void) {
    std::lock_guard lock(g_mutex);
    return g_status;
}

}
