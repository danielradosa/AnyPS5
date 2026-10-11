#include "SceShaders.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

extern "C" int APS5_VABI sceAgcGetGsPrimPayload(std::uint32_t* payload, const Shader* gs);

namespace {

constexpr std::uint32_t Untouched = 0xdeadbeefu;

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename TAction>
void expectInvalid(TAction action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error("expected an invalid argument");
}

Shader withRegisters(ShaderRegister* registers, std::uint8_t count) {
    Shader gs{};
    gs.cx_registers = registers;
    gs.num_cx_registers = count;
    return gs;
}

std::uint32_t payloadOf(const Shader& gs) {
    std::uint32_t payload = Untouched;
    check(sceAgcGetGsPrimPayload(&payload, &gs) == 0, "payload query failed");
    return payload;
}

void testPayload() {
    check(payloadOf(withRegisters(nullptr, 0)) == 0, "a shader without CX registers has a payload");

    std::array<ShaderRegister, 3> registers{{{0x1b1u, 2u}, {0x1c2u, 2u}, {0x1ffu, 2u}}};
    check(payloadOf(withRegisters(registers.data(), 3)) == 8, "index format 2 does not give an 8-byte payload");
    registers[1].value = 0x12u;
    check(payloadOf(withRegisters(registers.data(), 3)) == 8, "bits above the index format change the payload");
    registers[1].value = 4u;
    check(payloadOf(withRegisters(registers.data(), 3)) == 0, "index format 4 gives a payload");
    registers[1].value = 0u;
    check(payloadOf(withRegisters(registers.data(), 3)) == 0, "index format 0 gives a payload");

    std::array<ShaderRegister, 2> repeated{{{0x1c2u, 4u}, {0x1c2u, 2u}}};
    check(payloadOf(withRegisters(repeated.data(), 2)) == 0, "a later index format register was read after the first");

    std::array<ShaderRegister, 3> beyond{{{0x1b1u, 0u}, {0x1ffu, 0u}, {0x1c2u, 2u}}};
    check(payloadOf(withRegisters(beyond.data(), 2)) == 0, "a register past the CX register count was read");
    check(payloadOf(withRegisters(beyond.data(), 3)) == 8, "the last CX register was not read");
}

void testInvalidArguments() {
    std::array<ShaderRegister, 1> registers{{{0x1c2u, 2u}}};
    const auto gs = withRegisters(registers.data(), 1);
    std::uint32_t payload = Untouched;
    expectInvalid([&] { sceAgcGetGsPrimPayload(nullptr, &gs); });
    expectInvalid([&] { sceAgcGetGsPrimPayload(&payload, nullptr); });
    check(payload == Untouched, "a null shader wrote the payload");
    const auto missing = withRegisters(nullptr, 1);
    expectInvalid([&] { sceAgcGetGsPrimPayload(&payload, &missing); });
}

}

int main() {
    try {
        testPayload();
        testInvalidArguments();
        std::puts("AGC GS primitive payload tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
