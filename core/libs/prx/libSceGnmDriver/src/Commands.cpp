#include "prx/libSceGnmDriver/include/Commands.hpp"

#include "prx/libSceGnmDriver/include/Lowering.hpp"

namespace GnmDriver {

namespace {

constexpr std::uint32_t AllUnits = 0xffffffffu;
constexpr std::uint32_t ContextControlLoad = 0x80000000u;
constexpr std::uint32_t ContextControlShadow = 0x80000000u;

bool Neo(HardwareState state) {
    return state == HardwareState::Neo200 || state == HardwareState::Neo350;
}

std::uint32_t* ContextDefaults(std::uint32_t* at) {
    return SetRegisters(at, OpSetContextReg, {{0x2f9, 0x2d}, {0x282, 8}, {0x280, 0x80008}, {0x281, 0xffff0000u}, {0x204, 0}, {0x206, 0x43f}, {0x83, 0xffff},
        {0x317, 0x10}, {0x2fa, 0x3f800000u}, {0x2fc, 0x3f800000u}, {0x2fb, 0x3f800000u}, {0x2fd, 0x3f800000u}, {0x202, 0xcc0010}, {0x30e, 0xffffffffu}, {0x30f, 0xffffffffu}});
}

std::uint32_t* ContextTail(std::uint32_t* at, bool clearsIndexOffset, std::uint32_t scModeCntl1) {
    at = SetRegisters(at, OpSetContextReg, {{0x1b1, 2}, {0x101, 0}, {0x100, 0xffffffffu}, {0x103, 0}, {0x284, 0}, {0x290, 0}, {0x2ae, 0}});
    if (clearsIndexOffset) at = SetRegisters(at, OpSetContextReg, {{0x102, 0}});
    return SetRegisters(at, OpSetContextReg, {{0x292, 0}, {0x293, scModeCntl1}, {0x2f8, 0}, {0x2de, 0x1e9}});
}

std::uint32_t* NeoContextExtras(std::uint32_t* at) {
    return Emit(at, OpSetContextReg, {0xeb, 0xff00ff00u, 0xff00u});
}

std::uint32_t* ResetContext(std::uint32_t* at) {
    at = Emit(at, OpContextControl, {ContextControlLoad, ContextControlShadow});
    return Emit(at, OpClearState, {0});
}

}

std::uint32_t* SetRegisters(std::uint32_t* at, std::uint32_t opcode, std::initializer_list<RegisterValue> values) {
    for (const auto& [reg, value] : values) at = Emit(at, opcode, {reg, value});
    return at;
}

std::uint32_t* DefaultState(std::uint32_t* at, HardwareState state) {
    const bool neo = Neo(state);
    at = SetRegisters(at, OpSetShReg, {{0x216, AllUnits}, {0x217, AllUnits}});
    if (neo) at = SetRegisters(at, OpSetShReg, {{0x219, AllUnits}, {0x21a, AllUnits}});
    at = SetRegisters(at, OpSetShReg, {{0x215, 0}});
    at = ContextDefaults(at);
    at = Emit(at, OpNumInstances, {1});
    if (neo) {
        at = SetRegisters(at, OpSetShReg, {{7, 0x1701ff}, {0x46, 0x1701fd}, {0x87, 0x1701ff}, {0xc7, 0x1701fd}, {0x107, 0x17}, {0x147, 0x1701fd}, {0x47, 0x1c}});
    } else {
        at = SetRegisters(at, OpSetShReg, {{7, 0x1ff}, {0x46, 0x1ff}, {0x87, 0x1ff}, {0xc7, 0x1ff}, {0x107, 0}, {0x147, 0x1ff}});
    }
    at = ContextTail(at, state == HardwareState::Neo350, state == HardwareState::Sdk100 ? 0x6000000u : 0x6020000u);
    if (neo) at = NeoContextExtras(at);
    at = Emit(at, OpSetContextReg, {0x295, 0x100, 0x100, 4});
    at = Emit(at, OpSetUconfigReg, {0x200, 0xe0000000u});
    if (neo) at = Emit(at, OpSetUconfigReg, {NeoVgtParamRegister, NeoVgtParamDefault});
    return at;
}

std::uint32_t DrawInitDefaultHardwareState(std::uint32_t* cmd, std::uint32_t size, HardwareState state) {
    if (size < HwInitPacketWords) return 0;
    auto* at = ResetContext(cmd);
    at = Emit(at, OpAcquireMem, {0x2ec47fc0u, 0xffffffffu, 0, 0, 0, 10});
    at = DefaultState(at, state);
    Nop(at, static_cast<std::uint32_t>(cmd + HwInitPacketWords - at) - 1);
    return HwInitPacketWords;
}

std::uint32_t DrawInitToNeoContextState(std::uint32_t* cmd, std::uint32_t size, bool sdk400) {
    const auto words = sdk400 ? ContextState400Words : ContextStateWords;
    if (size != words) return 0;
    auto* at = ResetContext(cmd);
    if (sdk400) {
        at = ContextDefaults(at);
        at = Emit(at, OpNumInstances, {1});
        at = ContextTail(at, true, 0x6020000u);
        at = NeoContextExtras(at);
        at = Emit(at, OpSetContextReg, {0x295, 0x100, 0x100, 4});
        at = Emit(at, OpSetUconfigReg, {NeoVgtParamRegister, NeoVgtParamDefault});
    } else {
        at = Emit(at, OpNumInstances, {1});
        at = SetRegisters(at, OpSetContextReg, {{0x102, 0}, {0x202, 0xcc0010}});
        at = NeoContextExtras(at);
    }
    Nop(at, static_cast<std::uint32_t>(cmd + words - at) - 1);
    return words;
}

int SetPsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, bool clearShaderMask) {
    if (!cmd || size <= 0x27) return -1;
    if (!regs) {
        cmd = Emit(cmd, OpSetShReg, {8, 0, 0});
        cmd = Emit(cmd, OpSetContextReg, {0x203, 0});
        if (clearShaderMask) {
            cmd = Emit(cmd, OpSetContextReg, {0x8f, 0xf});
            Nop(cmd, 0x1d);
        } else {
            Nop(cmd, 0x20);
        }
        return 0;
    }
    if (regs[1] != 0) return -1;
    cmd = Emit(cmd, OpSetShReg, {8, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {10, regs[2], regs[3]});
    cmd = Emit(cmd, OpSetContextReg, {0x1c4, regs[4], regs[5]});
    cmd = Emit(cmd, OpSetContextReg, {0x1b3, regs[6], regs[7]});
    cmd = SetRegisters(cmd, OpSetContextReg, {{0x1b6, regs[8]}, {0x1b8, regs[9]}, {0x203, regs[10]}, {0x8f, regs[11]}});
    Nop(cmd, 11);
    return 0;
}

int SetCsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier) {
    constexpr std::uint32_t ModifierMask = 0xfffffc3fu;
    if (!cmd || size <= 0x18 || !regs || (modifier & ModifierMask) != 0 || regs[1] != 0) return -1;
    const std::uint32_t rsrc1 = modifier == 0 ? regs[2] : (regs[2] & ModifierMask) | modifier;
    cmd = Emit(cmd, OpSetShReg, {0x20c, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x212, rsrc1, regs[3]});
    cmd = Emit(cmd, OpSetShReg, {0x207, regs[4], regs[5], regs[6]});
    Nop(cmd, 11);
    return 0;
}

}
