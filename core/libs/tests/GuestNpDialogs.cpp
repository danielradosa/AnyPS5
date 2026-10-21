#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>

extern "C" {
int APS5_VABI sceNpFriendListDialogInitialize(void);
int APS5_VABI sceNpFriendListDialogOpenA(const void* param);
int APS5_VABI sceNpFriendListDialogUpdateStatus(void);
int APS5_VABI sceNpFriendListDialogGetStatus(void);
int APS5_VABI sceNpFriendListDialogClose(void);
int APS5_VABI sceNpFriendListDialogTerminate(void);
int APS5_VABI sceNpSnsFacebookDialogClose(void);
int APS5_VABI sceNpSnsFacebookDialogGetStatus(void);
}

namespace {

constexpr int STATUS_NONE = 0;
constexpr int STATUS_INITIALIZED = 1;
constexpr int STATUS_FINISHED = 3;
constexpr int NOT_INITIALIZED = static_cast<int>(0x80B80003u);
constexpr int ALREADY_INITIALIZED = static_cast<int>(0x80B80004u);
constexpr int INVALID_STATE = static_cast<int>(0x80B80006u);
constexpr int NOT_RUNNING = static_cast<int>(0x80B8000Bu);
constexpr int ARG_NULL = static_cast<int>(0x80B8000Du);

bool Check(bool condition, const char* message) {
    if (!condition) std::fprintf(stderr, "NpDialogs: %s\n", message);
    return condition;
}

bool CheckFriendList() {
    const unsigned char param[128] = {};
    return Check(sceNpFriendListDialogGetStatus() == STATUS_NONE, "the friend list dialog starts uninitialized")
        && Check(sceNpFriendListDialogOpenA(param) == INVALID_STATE, "open before initialize is INVALID_STATE")
        && Check(sceNpFriendListDialogClose() == NOT_INITIALIZED, "close before initialize is NOT_INITIALIZED")
        && Check(sceNpFriendListDialogTerminate() == NOT_INITIALIZED, "terminate before initialize is NOT_INITIALIZED")
        && Check(sceNpFriendListDialogInitialize() == 0, "initialize succeeds")
        && Check(sceNpFriendListDialogInitialize() == ALREADY_INITIALIZED, "a second initialize is ALREADY_INITIALIZED")
        && Check(sceNpFriendListDialogUpdateStatus() == STATUS_INITIALIZED, "the status is INITIALIZED")
        && Check(sceNpFriendListDialogOpenA(nullptr) == ARG_NULL, "a null parameter is ARG_NULL")
        && Check(sceNpFriendListDialogOpenA(param) == 0, "open succeeds")
        && Check(sceNpFriendListDialogUpdateStatus() == STATUS_FINISHED, "the dialog finishes at once")
        && Check(sceNpFriendListDialogGetStatus() == STATUS_FINISHED, "GetStatus reports FINISHED")
        && Check(sceNpFriendListDialogClose() == NOT_RUNNING, "close after the dialog finished is NOT_RUNNING")
        && Check(sceNpFriendListDialogOpenA(param) == 0, "a finished dialog can be opened again")
        && Check(sceNpFriendListDialogTerminate() == 0, "terminate succeeds")
        && Check(sceNpFriendListDialogGetStatus() == STATUS_NONE, "terminate returns to NONE")
        && Check(sceNpFriendListDialogTerminate() == NOT_INITIALIZED, "a second terminate is NOT_INITIALIZED");
}

bool CheckFacebook() {
    return Check(sceNpSnsFacebookDialogGetStatus() == STATUS_NONE, "the Facebook dialog is never initialized")
        && Check(sceNpSnsFacebookDialogClose() == NOT_INITIALIZED, "closing the Facebook dialog is NOT_INITIALIZED");
}

}

int main() {
    if (!CheckFriendList() || !CheckFacebook()) return 1;
    std::puts("NpDialogs tests passed");
    return 0;
}
