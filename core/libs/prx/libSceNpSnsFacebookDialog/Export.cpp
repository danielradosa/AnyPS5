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

}
