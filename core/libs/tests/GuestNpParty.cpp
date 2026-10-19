#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceNpPartyGetState(std::uint16_t* state);
}

static void Require(bool value) {
    if (!value) std::abort();
}

int main() {
    std::uint16_t state = 0xFFFF;
    Require(sceNpPartyGetState(&state) == 0);
    Require(state == 2);
    bool threw = false;
    try {
        sceNpPartyGetState(nullptr);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw);
    return 0;
}
