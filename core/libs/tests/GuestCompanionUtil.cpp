#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>

extern "C" {
int APS5_VABI sceCompanionUtilGetEvent();
int APS5_VABI sceCompanionUtilGetRemoteOskEvent();
int APS5_VABI sceCompanionUtilInitialize();
int APS5_VABI sceCompanionUtilOptParamInitialize();
int APS5_VABI sceCompanionUtilTerminate();
}

namespace {

constexpr int NoEvent = static_cast<int>(0x80AD0008u);

void Require(bool value) {
    if (!value) std::abort();
}

}

int main() {
    Require(sceCompanionUtilGetEvent() == NoEvent);
    Require(sceCompanionUtilTerminate() == 0);
    Require(sceCompanionUtilOptParamInitialize() == 0);
    Require(sceCompanionUtilInitialize() == 0);
    Require(sceCompanionUtilInitialize() == 0);
    Require(sceCompanionUtilGetEvent() == NoEvent);
    Require(sceCompanionUtilGetRemoteOskEvent() == 0);
    Require(sceCompanionUtilTerminate() == 0);
    Require(sceCompanionUtilGetEvent() == NoEvent);
    return 0;
}
