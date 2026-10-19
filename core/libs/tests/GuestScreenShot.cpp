#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI sceScreenShotSetOverlayImage(const char* filePath, std::int32_t offsetX, std::int32_t offsetY);
int APS5_VABI sceScreenShotSetOverlayImageWithOrigin(const char* filePath, std::int32_t marginX, std::int32_t marginY, std::int32_t origin);
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
}
