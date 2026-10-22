#include "prx/libSceAgc/Misc/include/Suspend.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceAgcSuspendPoint(void) {
    AgcDriverSuspendPoint_nid_postfix();
    return 0;
}

int APS5_VABI sceAgcSetSubmitMode(std::uint32_t mode) {
    if (mode != 0) throw std::runtime_error(std::string(__func__) + ": submit mode other than 0");
    return 0;
}

}
