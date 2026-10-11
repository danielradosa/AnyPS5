#include <cstdint>
#include <stdexcept>
#include <string>
#include <prx/libc/include/General.hpp>

#include "SceShaders.hpp"
#include "prx/libSceAgc/Shader/include/ShaderConstants.hpp"

extern "C" {

int APS5_VABI sceAgcGetGsPrimPayload(std::uint32_t* payload, const Shader* gs) {
    if (payload == nullptr || gs == nullptr) APS5_INVALID_ARG_EX;
    *payload = 0;
    if (gs->num_cx_registers == 0) return 0;
    if (gs->cx_registers == nullptr) APS5_INVALID_ARG_EX;
    for (std::uint32_t i = 0; i < gs->num_cx_registers; ++i) {
        if (gs->cx_registers[i].offset != ShaderRegs::SPI_SHADER_IDX_FORMAT) continue;
        if ((gs->cx_registers[i].value & 0xfu) == 2u) *payload = 8;
        break;
    }
    return 0;
}

}
