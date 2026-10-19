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

}
