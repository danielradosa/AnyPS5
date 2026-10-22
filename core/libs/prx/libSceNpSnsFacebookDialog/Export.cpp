#include <cstddef>
#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int COMMON_DIALOG_STATUS_NONE = 0;
constexpr int COMMON_DIALOG_ERROR_NOT_INITIALIZED = static_cast<int>(0x80B80003u);

}

extern "C" {

int APS5_VABI sceNpSnsFacebookDialogClose(void) {
    return COMMON_DIALOG_ERROR_NOT_INITIALIZED;
}

int APS5_VABI sceNpSnsFacebookDialogGetStatus(void) {
    return COMMON_DIALOG_STATUS_NONE;
}

int APS5_VABI sceNpSnsFacebookDialogGetResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpSnsFacebookDialogInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpSnsFacebookDialogOpen() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpSnsFacebookDialogTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceNpSnsFacebookDialogUpdateStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
