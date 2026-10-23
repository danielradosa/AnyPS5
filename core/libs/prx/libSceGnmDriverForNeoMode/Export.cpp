#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceGnmDriver/include/Commands.hpp"
#include "prx/libSceGnmDriver/include/Lowering.hpp"
#include "prx/libSceGnmDriverForNeoMode/include/InitSequences.hpp"

extern "C" {

int APS5_VABI sceGnmSubmitCommandBuffers(std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs, const std::uint32_t* ccbBytes);
int APS5_VABI sceGnmSubmitAndFlipCommandBuffers(std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs, const std::uint32_t* ccbBytes,
    std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg);
int APS5_VABI sceGnmRequestFlipAndSubmitDone(std::uint32_t* commands, std::uint32_t sizeInBytes, std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg);
int APS5_VABI sceKernelGetCompiledSdkVersion_nid_postfix(std::int32_t* version);

}

namespace {

constexpr std::uint32_t InitPacketWords = 0x100;

template <std::size_t Words>
int WriteHardwareState(std::uint32_t* cmdbuf, std::uint32_t size, const std::array<std::uint32_t, Words>& state) {
    if (size < InitPacketWords) return 0;
    auto* at = std::copy(GnmNeo::ClearState.begin(), GnmNeo::ClearState.end(), cmdbuf);
    at = std::copy(state.begin(), state.end(), at);
    const auto padding = static_cast<std::uint32_t>(cmdbuf + InitPacketWords - at - 1);
    at[0] = 0xc0001000u | ((padding - 1) << 16);
    at[1] = 0;
    return InitPacketWords;
}

template <std::size_t Words>
int WriteContextState(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t packetWords, const std::array<std::uint32_t, Words>& state) {
    if (size != packetWords) return 0;
    std::copy(state.begin(), state.end(), cmdbuf);
    return static_cast<int>(packetWords);
}

constexpr int CommandFailed = -1;
constexpr std::uint32_t OpNop = 0x10;
constexpr std::uint32_t NeoDrawFlags = 0xe0000000u;
constexpr std::uint32_t AutoIndex = 2;

constexpr std::uint32_t Header(std::uint32_t opcode, std::uint32_t bodyWords, bool compute = false, bool predicated = false) {
    return 0xc0000000u | ((bodyWords - 1) << 16) | (opcode << 8) | (compute ? 2u : 0u) | (predicated ? 1u : 0u);
}

std::uint32_t* Nop(std::uint32_t* at, std::uint32_t bodyWords) {
    at[0] = Header(OpNop, bodyWords);
    at[1] = 0;
    return at + 1 + bodyWords;
}

constexpr std::uint32_t ShaderStageCount = 7;
constexpr std::uint32_t StageVs = 2;
constexpr std::uint32_t StageEs = 4;
constexpr std::uint32_t StageLs = 6;
constexpr std::uint32_t IndirectSgprBase[ShaderStageCount] = {0, 0, 0x4c, 0, 0xcc, 0, 0x14c};
constexpr std::uint32_t DrawIndirectArgsBytes = 0x10;
constexpr std::uint32_t DrawIndexedIndirectArgsBytes = 0x14;

std::uint32_t IndirectSgpr(std::uint32_t stage, std::uint32_t offset) {
    return offset == 0 ? 0 : (offset & 0xffffu) + IndirectSgprBase[stage];
}

bool ValidIndirect(std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr) {
    return stage < ShaderStageCount && vertexSgpr < 0x10 && instanceSgpr < 0x10;
}

bool IndexedIndirectStage(std::uint32_t stage) {
    return stage == StageVs || stage == StageEs || stage == StageLs;
}

constexpr std::uint32_t OpSetUconfigReg = 0x79;
constexpr std::uint32_t IaMultiVgtParam = 0x40000258u;


using GnmDriver::Emit;
using GnmDriver::OpSetContextReg;
using GnmDriver::OpSetShReg;
using GnmDriver::SetRegisters;

constexpr std::uint32_t RegisterUpdate = 0xc01e0000u;
constexpr std::uint32_t VsModifierMask = 0xfcfffc3fu;

constexpr int GNM_ERROR_VALIDATION_NOT_ENABLED = static_cast<int>(0x80d13fffu);
constexpr int GNM_ERROR_FAILURE = static_cast<int>(0x8eee00ffu);
constexpr std::uint32_t GNM_INTERFACE_UNAVAILABLE = 0x80000000u;
constexpr std::uint32_t NeoGpuCoreClockHz = 911000000u;
constexpr int OffChipTessellationBufferBytes = 0x800000;
constexpr std::uint32_t WorkloadStreamCount = 0x10;
constexpr int WorkloadInvalidArgument = 3;
constexpr int WorkloadEndInvalid = 2;

alignas(256) constexpr std::uint32_t EmbeddedPs0[]{
    0xbeeb03ffu, 0x00000003u, 0x7e000280u, 0x5e000100u, 0xbf800000u, 0xf8001c0fu, 0x00000000u, 0xbf810000u,
    0x5362724fu, 0x07726468u, 0x00002043u, 0u, 0xb0a45b2bu, 0x1d39766du, 0x72044b7bu, 0x0000000fu,
    0x0fe000f0u, 0u, 0xc0000u, 4u, 0u, 4u, 2u, 2u, 0u, 0u, 0x10u, 0xfu, 0xcu, 0u, 0u, 0u,
};

alignas(256) constexpr std::uint32_t EmbeddedPs1[]{
    0xbeeb03ffu, 0x00000003u, 0x7e040280u, 0xf8001803u, 0x02020202u, 0xbf810000u,
    0x5362724fu, 0x07726468u, 0x00001841u, 0x04080002u, 0x98b9cb94u, 0u, 0x6f130734u, 0x0000000fu,
    0x0fe000f2u, 0u, 0x2000u, 0u, 0u, 2u, 2u, 2u, 0u, 0u, 0x10u, 3u, 0xcu,
};

alignas(256) constexpr std::uint32_t EmbeddedVs0[]{
    0xbeeb03ffu, 0x00000007u, 0x36020081u, 0x34020281u, 0x360000c2u, 0x4a0202c1u, 0x4a0000c1u, 0x7e020b01u,
    0x7e000b00u, 0x7e040280u, 0x7e0602f2u, 0xf80008cfu, 0x03020001u, 0xf800020fu, 0x03030303u, 0xbf810000u,
    0x5362724fu, 0x07726468u, 0x00004047u, 0u, 0x47f8c29fu, 0x9b2da5cfu, 0xff7c5b7du, 0x00000017u,
    0x0fe000f1u, 0u, 0x000c0000u, 4u, 0u, 4u, 0u, 7u,
};

constexpr std::array<std::uint32_t, 10> EmbeddedPs0Registers{0xc0000u, 4, 0, 4, 2, 2, 0, 0, 0x10, 0xf};
constexpr std::array<std::uint32_t, 10> EmbeddedPs1Registers{0x2000u, 0, 0, 2, 2, 2, 0, 0, 0x10, 3};

std::uint32_t* Update(std::uint32_t* at, std::uint32_t reg, std::initializer_list<std::uint32_t> values) {
    *at = GnmDriver::Header(OpNop, static_cast<std::uint32_t>(values.size()) + 1);
    at[1] = RegisterUpdate | reg;
    at += 2;
    for (const auto value : values) *at++ = value;
    return at;
}

bool UsesSdk400ContextState() {
    std::int32_t version = 0;
    if (sceKernelGetCompiledSdkVersion_nid_postfix(&version) != 0) return false;
    return static_cast<std::uint32_t>(version) >= GnmDriver::Sdk400;
}

int UpdatePsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, bool clearShaderMask) {
    if (!cmd || size <= 0x27) return -1;
    if (!regs) {
        cmd = Emit(cmd, OpSetShReg, {8, 0, 0});
        cmd = Update(cmd, 0x203, {0});
        if (clearShaderMask) {
            cmd = Emit(cmd, OpSetContextReg, {0x8f, 0xf});
            GnmDriver::Nop(cmd, 0x1d);
        } else {
            GnmDriver::Nop(cmd, 0x20);
        }
        return 0;
    }
    if (regs[1] != 0) return -1;
    cmd = Emit(cmd, OpSetShReg, {8, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {10, regs[2], regs[3]});
    cmd = Update(cmd, 0x1c4, {regs[4], regs[5]});
    cmd = Update(cmd, 0x1b3, {regs[6], regs[7]});
    cmd = Update(cmd, 0x1b6, {regs[8]});
    cmd = Update(cmd, 0x1b8, {regs[9]});
    cmd = Update(cmd, 0x203, {regs[10]});
    cmd = Update(cmd, 0x8f, {regs[11]});
    GnmDriver::Nop(cmd, 11);
    return 0;
}

} // namespace

extern "C" {

int APS5_VABI sceGnmBeginWorkload(std::uint32_t workloadStream, std::uint64_t* workload) {
    if (!workload) return WorkloadInvalidArgument;
    *workload = workloadStream < WorkloadStreamCount ? 1 : 0;
    return workloadStream < WorkloadStreamCount ? 0 : 1;
}

int APS5_VABI sceGnmComputeWaitOnAddress(std::uint32_t* cmdbuf, std::uint32_t size, std::uint64_t address, std::uint32_t mask, std::uint32_t compare, std::uint32_t reference) {
    if (!cmdbuf || size != 14) return CommandFailed;
    auto* at = Nop(cmdbuf, 3);
    const bool memory = address > 0xffff;
    at[0] = Header(0x3c, 6);
    at[1] = (memory ? 1u << 4 : 0u) | (compare & 7);
    at[2] = static_cast<std::uint32_t>(address & (memory ? 0xfffffffcu : 0xffffu));
    at[3] = static_cast<std::uint32_t>(address >> 32);
    at[4] = reference;
    at[5] = mask;
    at[6] = 10;
    Nop(at + 7, 2);
    return 0;
}

int APS5_VABI sceGnmComputeWaitSemaphore(std::uint32_t* cmdbuf, std::uint32_t size, std::uint64_t semaphoreAddress, std::uint32_t flags) {
    if (!cmdbuf || size != 10) return CommandFailed;
    auto* at = Nop(cmdbuf, 3);
    at[0] = Header(0x39, 2);
    at[1] = static_cast<std::uint32_t>(semaphoreAddress);
    at[2] = (flags == 1 ? 0x100000u : 0u) - 0x20000000u + static_cast<std::uint32_t>(semaphoreAddress >> 32);
    Nop(at + 3, 2);
    return 0;
}

int APS5_VABI sceGnmCreateWorkloadStream(std::uint64_t argument, std::uint32_t* workloadStream) {
    if (argument == 0 || !workloadStream) return WorkloadInvalidArgument;
    *workloadStream = 1;
    return 0;
}

int APS5_VABI sceGnmDebugModuleReset() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmDebugReset() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmDestroyWorkloadStream(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmDingDongForWorkload(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmDispatchDirect(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t threadsX, std::uint32_t threadsY, std::uint32_t threadsZ, std::uint32_t flags) {
    if (!cmdbuf || size != 9 || static_cast<std::int32_t>(threadsX | threadsY | threadsZ) < 0) return CommandFailed;
    cmdbuf[0] = Header(0x15, 4, true, flags & 1);
    cmdbuf[1] = threadsX;
    cmdbuf[2] = threadsY;
    cmdbuf[3] = threadsZ;
    cmdbuf[4] = (flags & 0x18) + 1;
    Nop(cmdbuf + 5, 3);
    return 0;
}

int APS5_VABI sceGnmDispatchIndirect(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t flags) {
    if (!cmdbuf || size != 7) return CommandFailed;
    cmdbuf[0] = Header(0x16, 2, true, flags & 1);
    cmdbuf[1] = dataOffset;
    cmdbuf[2] = (flags & 0x18) + 1;
    Nop(cmdbuf + 3, 3);
    return 0;
}

int APS5_VABI sceGnmDispatchIndirectOnMec(std::uint32_t* cmdbuf, std::uint32_t size, std::uint64_t args, std::uint32_t modifier) {
    if (!cmdbuf || size != 8 || args == 0 || (args & 3) != 0) return CommandFailed;
    cmdbuf[0] = Header(0x16, 3, true, modifier & 1);
    cmdbuf[1] = static_cast<std::uint32_t>(args);
    cmdbuf[2] = static_cast<std::uint32_t>(args >> 32);
    cmdbuf[3] = (modifier & 0x18) | 1;
    cmdbuf[4] = Header(OpNop, 3);
    cmdbuf[5] = 0;
    return 0;
}

int APS5_VABI sceGnmDrawIndex(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t indexCount, std::uint64_t indexAddress, std::uint32_t flags, std::uint32_t) {
    if (!cmdbuf || size != 10 || indexAddress == 0 || (indexAddress & 1) != 0 || (flags & 0x1ffffffeu) != 0) return CommandFailed;
    cmdbuf[0] = Header(0x27, 5);
    cmdbuf[1] = indexCount;
    cmdbuf[2] = static_cast<std::uint32_t>(indexAddress);
    cmdbuf[3] = static_cast<std::uint32_t>(indexAddress >> 32);
    cmdbuf[4] = indexCount;
    cmdbuf[5] = flags & NeoDrawFlags;
    Nop(cmdbuf + 6, 3);
    return 0;
}

int APS5_VABI sceGnmDrawIndexAuto(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t indexCount, std::uint32_t flags) {
    if (!cmdbuf || size != 7 || (flags & 0x1ffffffeu) != 0) return CommandFailed;
    cmdbuf[0] = Header(0x2d, 2);
    cmdbuf[1] = indexCount;
    cmdbuf[2] = (flags & NeoDrawFlags) | AutoIndex;
    Nop(cmdbuf + 3, 3);
    return 0;
}

int APS5_VABI sceGnmDrawIndexIndirect(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags) {
    if (!cmdbuf || size != 9 || !ValidIndirect(stage, vertexSgpr, instanceSgpr)) return CommandFailed;
    cmdbuf[0] = Header(0x25, 4, false, flags & 1);
    cmdbuf[1] = dataOffset;
    cmdbuf[2] = IndirectSgpr(stage, vertexSgpr);
    cmdbuf[3] = IndirectSgpr(stage, instanceSgpr);
    cmdbuf[4] = flags & NeoDrawFlags;
    Nop(cmdbuf + 5, 3);
    return 0;
}

int APS5_VABI sceGnmDrawIndexIndirectCountMulti(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint64_t countAddress, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags) {
    if (!cmdbuf || size != 0x10 || vertexSgpr >= 0x10 || instanceSgpr >= 0x10 || !IndexedIndirectStage(stage)) return CommandFailed;
    auto* at = Nop(cmdbuf, 2);
    at[0] = Header(0x9d, 9, false, flags & 1);
    at[1] = dataOffset;
    at[2] = IndirectSgpr(stage, vertexSgpr);
    at[3] = IndirectSgpr(stage, instanceSgpr);
    at[4] = countAddress != 0 ? 1u << 30 : 0;
    at[5] = maxCount;
    at[6] = static_cast<std::uint32_t>(countAddress);
    at[7] = static_cast<std::uint32_t>(countAddress >> 32);
    at[8] = DrawIndexedIndirectArgsBytes;
    at[9] = flags & NeoDrawFlags;
    Nop(at + 10, 2);
    return 0;
}

int APS5_VABI sceGnmDrawIndexIndirectMulti(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags) {
    if (!cmdbuf || size != 11 || vertexSgpr >= 0x10 || instanceSgpr >= 0x10 || !IndexedIndirectStage(stage)) return CommandFailed;
    cmdbuf[0] = Header(0x38, 6, false, flags & 1);
    cmdbuf[1] = dataOffset;
    cmdbuf[2] = IndirectSgpr(stage, vertexSgpr);
    cmdbuf[3] = IndirectSgpr(stage, instanceSgpr);
    cmdbuf[4] = maxCount;
    cmdbuf[5] = DrawIndexedIndirectArgsBytes;
    cmdbuf[6] = flags & NeoDrawFlags;
    Nop(cmdbuf + 7, 3);
    return 0;
}

int APS5_VABI sceGnmDrawIndexMultiInstanced(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmDrawIndexOffset(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t indexOffset, std::uint32_t indexCount, std::uint32_t flags) {
    if (!cmdbuf || size != 9) return CommandFailed;
    cmdbuf[0] = Header(0x35, 4, false, flags & 1);
    cmdbuf[1] = indexCount;
    cmdbuf[2] = indexOffset;
    cmdbuf[3] = indexCount;
    cmdbuf[4] = flags & NeoDrawFlags;
    Nop(cmdbuf + 5, 3);
    return 0;
}

int APS5_VABI sceGnmDrawIndirect(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags) {
    if (!cmdbuf || size != 9 || !ValidIndirect(stage, vertexSgpr, instanceSgpr)) return CommandFailed;
    cmdbuf[0] = Header(0x24, 4, false, flags & 1);
    cmdbuf[1] = dataOffset;
    cmdbuf[2] = IndirectSgpr(stage, vertexSgpr);
    cmdbuf[3] = IndirectSgpr(stage, instanceSgpr);
    cmdbuf[4] = (flags & NeoDrawFlags) | AutoIndex;
    Nop(cmdbuf + 5, 3);
    return 0;
}

int APS5_VABI sceGnmDrawIndirectCountMulti(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint64_t countAddress, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags) {
    if (!cmdbuf || size != 0x10 || !ValidIndirect(stage, vertexSgpr, instanceSgpr)) return CommandFailed;
    auto* at = Nop(cmdbuf, 2);
    at[0] = Header(0x9c, 9, false, flags & 1);
    at[1] = dataOffset;
    at[2] = IndirectSgpr(stage, vertexSgpr);
    at[3] = IndirectSgpr(stage, instanceSgpr);
    at[4] = countAddress != 0 ? 1u << 30 : 0;
    at[5] = maxCount;
    at[6] = static_cast<std::uint32_t>(countAddress);
    at[7] = static_cast<std::uint32_t>(countAddress >> 32);
    at[8] = DrawIndirectArgsBytes;
    at[9] = (flags & NeoDrawFlags) | AutoIndex;
    Nop(at + 10, 2);
    return 0;
}

int APS5_VABI sceGnmDrawIndirectMulti(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags) {
    if (!cmdbuf || size != 11 || !ValidIndirect(stage, vertexSgpr, instanceSgpr)) return CommandFailed;
    cmdbuf[0] = Header(0x2c, 6, false, flags & 1);
    cmdbuf[1] = dataOffset;
    cmdbuf[2] = IndirectSgpr(stage, vertexSgpr);
    cmdbuf[3] = IndirectSgpr(stage, instanceSgpr);
    cmdbuf[4] = maxCount;
    cmdbuf[5] = DrawIndirectArgsBytes;
    cmdbuf[6] = (flags & NeoDrawFlags) | AutoIndex;
    Nop(cmdbuf + 7, 3);
    return 0;
}

int APS5_VABI sceGnmDrawInitDefaultHardwareState(std::uint32_t* cmdbuf, std::uint32_t size) {
    return WriteHardwareState(cmdbuf, size, GnmNeo::HardwareState);
}

int APS5_VABI sceGnmDrawInitDefaultHardwareState200(std::uint32_t* cmdbuf, std::uint32_t size) {
    return WriteHardwareState(cmdbuf, size, GnmNeo::HardwareState200);
}

int APS5_VABI sceGnmDrawInitDefaultHardwareState350(std::uint32_t* cmdbuf, std::uint32_t size) {
    return WriteHardwareState(cmdbuf, size, GnmNeo::HardwareState350);
}

int APS5_VABI sceGnmDrawInitToDefaultContextState(std::uint32_t* cmdbuf, std::uint32_t size) {
    return WriteContextState(cmdbuf, size, 0x20, GnmNeo::ContextState);
}

int APS5_VABI sceGnmDrawInitToDefaultContextState400(std::uint32_t* cmdbuf, std::uint32_t size) {
    return WriteContextState(cmdbuf, size, 0x100, GnmNeo::ContextState400);
}

int APS5_VABI sceGnmDrawInitToDefaultContextStateInternalCommand(std::uint32_t* cmd, std::uint32_t size) {
    return static_cast<int>(GnmDriver::DrawInitToNeoContextState(cmd, size, UsesSdk400ContextState()));
}

int APS5_VABI sceGnmDrawInitToDefaultContextStateInternalSize() {
    return static_cast<int>(UsesSdk400ContextState() ? GnmDriver::ContextState400Words : GnmDriver::ContextStateWords);
}

int APS5_VABI sceGnmDrawOpaqueAuto(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterface() {
    return GNM_INTERFACE_UNAVAILABLE;
}

std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterfaceForGpuDebugger() {
    return GNM_INTERFACE_UNAVAILABLE;
}

std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterfaceForReplay() {
    return GNM_INTERFACE_UNAVAILABLE;
}

std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterfaceForResourceRegistration() {
    return GNM_INTERFACE_UNAVAILABLE;
}

std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterfaceForValidation() {
    return GNM_INTERFACE_UNAVAILABLE;
}

int APS5_VABI sceGnmDriverInternalVirtualQuery() {
    return 0;
}

bool APS5_VABI sceGnmDriverTraceInProgress() {
    return false;
}

int APS5_VABI sceGnmEndWorkload(std::uint64_t workload) {
    if (workload == 0) return WorkloadEndInvalid;
    return ((workload >> 56) & 0xffu) >= WorkloadStreamCount ? WorkloadEndInvalid : 0;
}

int APS5_VABI sceGnmFindResources() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmFindResourcesPublic() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmGetCoredumpAddress(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmGetCoredumpMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmGetCoredumpProtectionFaultTimestamp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmGetDbgGcHandle() {
    return -1;
}

int APS5_VABI sceGnmGetGpuBlockStatus() {
    return 0;
}

std::uint32_t APS5_VABI sceGnmGetGpuCoreClockFrequency() {
    return NeoGpuCoreClockHz;
}

int APS5_VABI sceGnmGetGpuInfoStatus() {
    return 0;
}

int APS5_VABI sceGnmGetLastWaitedAddress() {
    return 0;
}

int APS5_VABI sceGnmGetNumTcaUnits(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmGetOffChipTessellationBufferSize() {
    return OffChipTessellationBufferBytes;
}

int APS5_VABI sceGnmGetOwnerName() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmGetPhysicalCounterFromVirtualized() {
    return GNM_ERROR_FAILURE;
}

std::uint32_t APS5_VABI sceGnmGetProtectionFaultTimeStamp() {
    return 0;
}

int APS5_VABI sceGnmGetResourceBaseAddressAndSizeInBytes() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmGetResourceName() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmGetResourceRegistrationBuffers() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmGetResourceShaderGuid() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmGetResourceType() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmGetResourceUserData() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmGetShaderProgramBaseAddress() {
    return 0;
}

int APS5_VABI sceGnmGetShaderStatus() {
    return 0;
}

void APS5_VABI sceGnmGpuPaDebugEnter() {
}

void APS5_VABI sceGnmGpuPaDebugLeave() {
}

int APS5_VABI sceGnmInsertDingDongMarker(std::uint32_t* cmdbuf, std::uint32_t size) {
    if (!cmdbuf || size != 4) return CommandFailed;
    cmdbuf[0] = Header(OpNop, 3);
    cmdbuf[1] = 0;
    cmdbuf[2] = 0;
    cmdbuf[3] = 0;
    return 0;
}

int APS5_VABI sceGnmInsertThreadTraceMarker() {
    return 0;
}

int APS5_VABI sceGnmIsCoredumpValid(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmLogicalTcaUnitToPhysical(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmMapComputeQueueWithPriority(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmPaDisableFlipCallbacks() {
    return 0;
}

int APS5_VABI sceGnmPaEnableFlipCallbacks() {
    return 0;
}

int APS5_VABI sceGnmPaHeartbeat() {
    return 0;
}

int APS5_VABI sceGnmQueryResourceRegistrationUserMemoryRequirements() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmRaiseUserExceptionEvent() {
    return 0;
}

int APS5_VABI sceGnmRegisterGdsResource() {
    return GNM_ERROR_FAILURE;
}

void APS5_VABI sceGnmRegisterGnmLiveCallbackConfig() {
}

int APS5_VABI sceGnmRegisterOwner(void* handle, const char* name) {
    static_cast<void>(handle);
    static_cast<void>(name);
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmRegisterOwnerForSystem() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmRegisterResource(void* resourceHandle, void* ownerHandle, const void* address, std::size_t size, const char* name, int type, std::uint64_t userData) {
    static_cast<void>(resourceHandle);
    static_cast<void>(ownerHandle);
    static_cast<void>(address);
    static_cast<void>(size);
    static_cast<void>(name);
    static_cast<void>(type);
    static_cast<void>(userData);
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmRequestFlipAndSubmitDoneForWorkload(std::uint64_t workload, std::uint32_t* commands, std::uint32_t sizeInBytes, std::uint32_t handle, std::uint32_t index,
    std::uint32_t flipMode, std::int64_t flipArg) {
    static_cast<void>(workload);
    return sceGnmRequestFlipAndSubmitDone(commands, sizeInBytes, handle, index, flipMode, flipArg);
}

int APS5_VABI sceGnmResetVgtControl(std::uint32_t* cmdbuf, std::uint32_t size) {
    if (!cmdbuf || size != 3) return CommandFailed;
    cmdbuf[0] = Header(OpSetUconfigReg, 2);
    cmdbuf[1] = IaMultiVgtParam;
    cmdbuf[2] = 0x6d007fu;
    return 0;
}

int APS5_VABI sceGnmSdmaClose() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSdmaConstFill() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSdmaCopyLinear() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSdmaCopyTiled() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSdmaCopyWindow() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSdmaFlush() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSdmaGetMinCmdSize() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSdmaOpen() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSetCsShaderWithModifier(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier) {
    return GnmDriver::SetCsShader(cmd, size, regs, modifier);
}

int APS5_VABI sceGnmSetEmbeddedPsShader(std::uint32_t* cmd, std::uint32_t size, std::uint32_t shaderId, std::uint32_t modifier) {
    static_cast<void>(size);
    static_cast<void>(modifier);
    if (shaderId > 1) return GNM_ERROR_FAILURE;
    const auto address = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(shaderId == 0 ? EmbeddedPs0 : EmbeddedPs1));
    const auto& regs = shaderId == 0 ? EmbeddedPs0Registers : EmbeddedPs1Registers;
    cmd = Emit(cmd, OpSetShReg, {8, static_cast<std::uint32_t>(address >> 8), static_cast<std::uint32_t>(address >> 40)});
    cmd = Emit(cmd, OpSetShReg, {10, regs[0], regs[1]});
    cmd = Emit(cmd, OpSetContextReg, {0x1c4, regs[2], regs[3]});
    cmd = Emit(cmd, OpSetContextReg, {0x1b3, regs[4], regs[5]});
    cmd = SetRegisters(cmd, OpSetContextReg, {{0x1b6, regs[6]}, {0x1b8, regs[7]}, {0x203, regs[8]}, {0x8f, regs[9]}});
    GnmDriver::Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmSetEmbeddedVsShader(std::uint32_t* cmd, std::uint32_t size, std::uint32_t shaderId, std::uint32_t modifier) {
    static_cast<void>(size);
    static_cast<void>(modifier);
    if (shaderId != 0) return GNM_ERROR_FAILURE;
    const auto address = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(EmbeddedVs0));
    cmd = Emit(cmd, OpSetShReg, {0x48, static_cast<std::uint32_t>(address >> 8), static_cast<std::uint32_t>(address >> 40)});
    cmd = Emit(cmd, OpSetShReg, {0x4a, 0xc0000u, 4});
    cmd = SetRegisters(cmd, OpSetContextReg, {{0x207, 0}, {0x1b1, 0}, {0x1c3, 4}});
    GnmDriver::Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmSetPsShader350(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs) {
    return GnmDriver::SetPsShader(cmd, size, regs, true);
}

int APS5_VABI sceGnmSetResourceRegistrationUserMemory() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSetResourceUserData() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSetSpiEnableSqCounters() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSetSpiEnableSqCountersForUnitInstance() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmSetVgtControl(std::uint32_t* cmdbuf, std::uint32_t size, std::uint32_t primGroupSizeMinusOne, std::uint32_t partialVsWaveMode, std::uint32_t switchOnlyOnEop) {
    if (!cmdbuf || size != 3 || primGroupSizeMinusOne >= 0x100 || (switchOnlyOnEop | partialVsWaveMode) >= 2) return CommandFailed;
    const std::uint32_t switchOnEop = static_cast<std::uint32_t>(switchOnlyOnEop != 0) << 20;
    const std::uint32_t switchOnEoi = static_cast<std::uint32_t>(switchOnlyOnEop == 0) << 19;
    const std::uint32_t value = switchOnlyOnEop != 0
        ? ((partialVsWaveMode & 1) << 16) | primGroupSizeMinusOne | switchOnEop | switchOnEoi | 0x40000u
        : (primGroupSizeMinusOne & 0x1cffffu) | switchOnEop | switchOnEoi | 0x50000u;
    cmdbuf[0] = Header(OpSetUconfigReg, 2);
    cmdbuf[1] = IaMultiVgtParam;
    cmdbuf[2] = value | 0x600000u;
    return 0;
}

int APS5_VABI sceGnmSetWaveLimitMultiplier(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmSetWaveLimitMultipliers(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmSubmitAndFlipCommandBuffersForWorkload(std::uint32_t workload, std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs,
    const std::uint32_t* ccbBytes, std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg) {
    static_cast<void>(workload);
    return sceGnmSubmitAndFlipCommandBuffers(count, dcbs, dcbBytes, ccbs, ccbBytes, handle, index, flipMode, flipArg);
}

int APS5_VABI sceGnmSubmitCommandBuffersForWorkload(std::uint32_t workload, std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs,
    const std::uint32_t* ccbBytes) {
    static_cast<void>(workload);
    return sceGnmSubmitCommandBuffers(count, dcbs, dcbBytes, ccbs, ccbBytes);
}

int APS5_VABI sceGnmUnregisterAllResourcesForOwner() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmUnregisterOwnerAndResources() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmUnregisterResource() {
    return GNM_ERROR_FAILURE;
}

int APS5_VABI sceGnmUpdateGsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs) {
    if (!cmd || size < 0x1d || !regs || regs[1] != 0) return -1;
    cmd = Emit(cmd, OpSetShReg, {0x88, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x8a, regs[2], regs[3]});
    cmd = Update(cmd, 0x2e5, {regs[4]});
    cmd = Update(cmd, 0x29b, {regs[5]});
    cmd = Update(cmd, 0x2e4, {regs[6]});
    GnmDriver::Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmUpdateHsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t lsHsConfig) {
    if (!cmd || size <= 0x1c || !regs || regs[1] != 0) return -1;
    cmd = Emit(cmd, OpSetShReg, {0x108, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x10a, regs[2], regs[3]});
    cmd = Update(cmd, 0x286, {regs[5], regs[6]});
    cmd = Update(cmd, 0x2db, {regs[4]});
    cmd = Update(cmd, 0x2d6, {lsHsConfig});
    GnmDriver::Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmUpdatePsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs) {
    return UpdatePsShader(cmd, size, regs, false);
}

int APS5_VABI sceGnmUpdatePsShader350(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs) {
    return UpdatePsShader(cmd, size, regs, true);
}

int APS5_VABI sceGnmUpdateVsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier) {
    if (!cmd || size <= 0x1c || !regs || (modifier & VsModifierMask) != 0 || regs[1] != 0) return -1;
    const std::uint32_t rsrc1 = modifier == 0 ? regs[2] : (regs[2] & VsModifierMask) | modifier;
    cmd = Emit(cmd, OpSetShReg, {0x48, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x4a, rsrc1, regs[3]});
    cmd = Update(cmd, 0x207, {regs[6]});
    cmd = Update(cmd, 0x1b1, {regs[4]});
    cmd = Update(cmd, 0x1c3, {regs[5]});
    GnmDriver::Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmValidateDisableDiagnostics() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

int APS5_VABI sceGnmValidateDisableDiagnostics2() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

int APS5_VABI sceGnmValidateDispatchCommandBuffers() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

int APS5_VABI sceGnmValidateDrawCommandBuffers() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

int APS5_VABI sceGnmValidateGetDiagnosticInfo() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

int APS5_VABI sceGnmValidateGetDiagnostics() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

int APS5_VABI sceGnmValidateGetVersion() {
    return 0;
}

bool APS5_VABI sceGnmValidateOnSubmitEnabled() {
    return false;
}

int APS5_VABI sceGnmValidateResetState() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

int APS5_VABI sceGnmValidationRegisterMemoryCheckCallback() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

}
