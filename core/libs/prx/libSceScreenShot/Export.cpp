#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

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
int APS5_VABI sceScreenShotDisable() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotDisableNotification() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotEnable() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotEnableNotification() {
    NotImplemented_nid_no_patch(__func__);
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
int APS5_VABI sceScreenShotIsDisabled() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotIsVshScreenCaptureDisabled() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotSetDrcParam() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceScreenShotSetParam() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
