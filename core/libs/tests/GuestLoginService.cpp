#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdlib>

extern "C" {
int APS5_VABI sceLoginServiceInitialize(void);
int APS5_VABI sceLoginServiceTerminate(void);
int APS5_VABI sceLoginServiceRequestDevices(void* param);
}

static void Require(bool value) {
    if (!value) std::abort();
}

int main() {
    Require(sceLoginServiceInitialize() == 0);
    Require(sceLoginServiceTerminate() == 0);
    unsigned char buffer[0x80]{};
    Require(sceLoginServiceRequestDevices(buffer) == 0);
}
