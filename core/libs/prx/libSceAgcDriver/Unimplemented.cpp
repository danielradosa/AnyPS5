#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceAgcDriverRegisterDefaultOwner(uint32_t owner) {
    (void)owner;
    return 0;
}

}
