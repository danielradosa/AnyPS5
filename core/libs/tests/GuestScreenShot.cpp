#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceScreenShotSetOverlayImage(const char* filePath, std::int32_t offsetX, std::int32_t offsetY);
int APS5_VABI sceScreenShotSetOverlayImageWithOrigin(const char* filePath, std::int32_t marginX, std::int32_t marginY, std::int32_t origin);
int APS5_VABI sceScreenShotDisable(void);
int APS5_VABI sceScreenShotEnable(void);
int APS5_VABI sceScreenShotIsDisabled(void);
int APS5_VABI sceScreenShotDisableNotification(void);
int APS5_VABI sceScreenShotEnableNotification(void);
int APS5_VABI sceScreenShotIsVshScreenCaptureDisabled(void);
int APS5_VABI sceScreenShotSetParam(const void* param);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    Require(sceScreenShotSetOverlayImage("/app0/overlay.png", 0, 0) == 0);
    Require(sceScreenShotSetOverlayImage("/app0/overlay.png", 32, 16) == 0);
    Require(sceScreenShotSetOverlayImageWithOrigin("/app0/overlay.png", 8, 8, 0) == 0);
    Require(sceScreenShotSetOverlayImageWithOrigin("/app0/overlay.png", 8, 8, 3) == 0);
    Require(sceScreenShotSetOverlayImage(nullptr, 0, 0) == 0);

    Require(sceScreenShotIsDisabled() == 0);
    Require(sceScreenShotDisable() == 0);
    Require(sceScreenShotIsDisabled() == 1);
    Require(sceScreenShotDisable() == 0);
    Require(sceScreenShotIsDisabled() == 1);
    Require(sceScreenShotEnable() == 0);
    Require(sceScreenShotIsDisabled() == 0);
    Require(sceScreenShotEnable() == 0);
    Require(sceScreenShotIsDisabled() == 0);

    Require(sceScreenShotDisableNotification() == 0);
    Require(sceScreenShotEnableNotification() == 0);
    Require(sceScreenShotIsVshScreenCaptureDisabled() == 0);
    Require(sceScreenShotDisable() == 0);
    Require(sceScreenShotIsVshScreenCaptureDisabled() == 0);
    Require(sceScreenShotEnable() == 0);

    unsigned char param[64] = {};
    Require(sceScreenShotSetParam(param) == 0);
    Require(sceScreenShotIsDisabled() == 0);
}
