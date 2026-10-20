#include <atomic>
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

std::atomic<bool> g_disabled{false};

}

extern "C" {

int APS5_VABI sceScreenShotSetOverlayImage(const char* filePath, int32_t offsetX, int32_t offsetY) {
    (void)filePath;
    (void)offsetX;
    (void)offsetY;
    return 0;
}

int APS5_VABI sceScreenShotSetOverlayImageWithOrigin(const char* filePath, int32_t marginX, int32_t marginY, int32_t origin) {
    (void)filePath;
    (void)marginX;
    (void)marginY;
    (void)origin;
    return 0;
}
int APS5_VABI sceScreenShotCapture() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotDisable(void) {
    g_disabled.store(true);
    return 0;
}
int APS5_VABI sceScreenShotDisableNotification(void) {
    return 0;
}
int APS5_VABI sceScreenShotEnable(void) {
    g_disabled.store(false);
    return 0;
}
int APS5_VABI sceScreenShotEnableNotification(void) {
    return 0;
}
int APS5_VABI sceScreenShotGetAppInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotGetDrcParam() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotIsDisabled(void) {
    return g_disabled.load() ? 1 : 0;
}
int APS5_VABI sceScreenShotIsVshScreenCaptureDisabled(void) {
    return 0;
}
int APS5_VABI sceScreenShotSetDrcParam() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotSetParam(const void* param) {
    (void)param;
    return 0;
}

}
