#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <mutex>
#include <stdexcept>
#include <string>
#include <span>
#include <utility>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"
#include "prx/libSceAgcDriver/Eq/include/Event.hpp"
#include "prx/libSceVideoOut/include/Buffer.hpp"
#include "prx/libSceGnmDriver/include/Lowering.hpp"

namespace {

using GnmDriver::Emit;
using GnmDriver::FlipReserveWords;
using GnmDriver::Header;
using GnmDriver::Nop;
using GnmDriver::OpAcquireMem;
using GnmDriver::OpNop;
using GnmDriver::OpWriteData;
using GnmDriver::PrepareFlip;
using GnmDriver::PrepareFlipHeader;
using GnmDriver::WriteDataToMemory;

constexpr std::uint32_t OpClearState = 0x12;
constexpr std::uint32_t OpContextControl = 0x28;
constexpr std::uint32_t OpNumInstances = 0x2f;
constexpr std::uint32_t OpWaitRegMem = 0x3c;
constexpr std::uint32_t OpSetContextReg = 0x69;
constexpr std::uint32_t OpSetShReg = 0x76;
constexpr std::uint32_t OpSetUconfigReg = 0x79;
constexpr std::uint32_t GfxEndOfPipeEvent = 0x40;

constexpr std::uint32_t HwInitPacketWords = 0x100;
constexpr std::uint32_t MarkerPush = 0x68750001u;
constexpr std::uint32_t MarkerPop = 0x68750002u;
constexpr std::uint32_t MarkerSet = 0x68750003u;
constexpr std::uint32_t MarkerColorPush = 0x6875000eu;
constexpr std::uint32_t WaitMemoryEqual = 0x13u;
constexpr std::uint32_t MaxCommandBufferBytes = 0x3ffffc;
constexpr std::int16_t EvfiltGraphicsCore = -14;

constexpr int GNM_ERROR_SUBMISSION_FAILED_INVALID_ARGUMENT = static_cast<int>(0x80d11000u);
constexpr int GNM_ERROR_SUBMISSION_NOT_ENOUGH_RESOURCES = static_cast<int>(0x80d11001u);
constexpr int GNM_ERROR_VALIDATION_NOT_ENABLED = static_cast<int>(0x80d13fffu);
constexpr int GNM_ERROR_CAPTURE_RAZOR_NOT_LOADED = static_cast<int>(0x80d15001u);
constexpr int GNM_ERROR_CAPTURE_FAILED_INTERNAL = static_cast<int>(0x80d1500fu);
constexpr int GNM_GS_RING_SIZE_INVALID = 0x08000000;
constexpr int GNM_MIP_STATS_INVALID_BUFFER = static_cast<int>(0x80000001u);
constexpr int GNM_MIP_STATS_INVALID_SIZE = static_cast<int>(0x80000002u);
constexpr std::uint32_t MinGsRingSize = 0x400000;
constexpr std::uint32_t MaxGsRingSize = 0x800000;
constexpr std::uint32_t GsRingAlignment = 0x100000;
constexpr std::uint32_t MipStatsHeaderBytes = 0x40;
constexpr std::uint32_t MinMipStatsBytes = 0x1040;

using RegisterValue = std::pair<std::uint32_t, std::uint32_t>;

std::uint32_t* SetRegisters(std::uint32_t* at, std::uint32_t opcode, std::initializer_list<RegisterValue> values) {
    for (const auto& [reg, value] : values) at = Emit(at, opcode, {reg, value});
    return at;
}

std::uint32_t* DefaultState(std::uint32_t* at, std::uint32_t scModeCntl1) {
    at = SetRegisters(at, OpSetShReg, {{0x216, 0xffffffffu}, {0x217, 0xffffffffu}, {0x215, 0}});
    at = SetRegisters(at, OpSetContextReg, {{0x2f9, 0x2d}, {0x282, 8}, {0x280, 0x80008}, {0x281, 0xffff0000u}, {0x204, 0}, {0x206, 0x43f}, {0x83, 0xffff}, {0x317, 0x10},
        {0x2fa, 0x3f800000u}, {0x2fc, 0x3f800000u}, {0x2fb, 0x3f800000u}, {0x2fd, 0x3f800000u}, {0x202, 0xcc0010}, {0x30e, 0xffffffffu}, {0x30f, 0xffffffffu}});
    at = Emit(at, OpNumInstances, {1});
    at = SetRegisters(at, OpSetShReg, {{7, 0x1ff}, {0x46, 0x1ff}, {0x87, 0x1ff}, {0xc7, 0x1ff}, {0x107, 0}, {0x147, 0x1ff}});
    at = SetRegisters(at, OpSetContextReg, {{0x1b1, 2}, {0x101, 0}, {0x100, 0xffffffffu}, {0x103, 0}, {0x284, 0}, {0x290, 0}, {0x2ae, 0}, {0x292, 0}, {0x293, scModeCntl1},
        {0x2f8, 0}, {0x2de, 0x1e9}});
    at = Emit(at, OpSetContextReg, {0x295, 0x100, 0x100, 4});
    return Emit(at, OpSetUconfigReg, {0x200, 0xe0000000u});
}

struct FrameStart {
    std::uint32_t words[HwInitPacketWords]{};
    std::uint32_t size = 0;
    FrameStart() {
        auto* at = Emit(words, OpClearState, {0});
        size = static_cast<std::uint32_t>(DefaultState(at, 0x6000000) - words);
    }
};

std::mutex g_submitMutex;
bool g_frameStarted = false;

int CheckSubmission(std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs, const std::uint32_t* ccbBytes) {
    if (count != 0 && (!dcbs || !dcbBytes || (ccbBytes && !ccbs))) return GNM_ERROR_SUBMISSION_FAILED_INVALID_ARGUMENT;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (dcbBytes[i] == 0 || dcbBytes[i] > MaxCommandBufferBytes || (ccbBytes && ccbBytes[i] > MaxCommandBufferBytes)) return GNM_ERROR_SUBMISSION_FAILED_INVALID_ARGUMENT;
    }
    return 0;
}

GnmDriver::EngineCounters g_counters;

void SubmitLocked(std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs, const std::uint32_t* ccbBytes) {
    static FrameStart frameStart;
    std::vector<std::vector<std::uint32_t>> merged;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto constantWords = ccbBytes && ccbs[i] ? ccbBytes[i] / 4 : 0;
        merged.push_back(GnmDriver::MergeEngines({dcbs[i], dcbBytes[i] / 4}, {ccbs ? ccbs[i] : nullptr, constantWords}, g_counters));
    }
    if (!g_frameStarted) {
        static const bool gcn = (AgcDriverUseGcnShaders_nid_postfix(), true);
        static_cast<void>(gcn);
        AgcDriverSubmitGraphics_nid_postfix(frameStart.words, frameStart.size);
        g_frameStarted = true;
    }
    for (auto& commands : merged) AgcDriverSubmitGraphics_nid_postfix(commands.data(), static_cast<std::uint32_t>(commands.size()));
}

std::uintptr_t BufferLabel(std::uint32_t handle, std::uint32_t index) {
    return (VideoOutBufferLabelAddress_nid_postfix(static_cast<int>(handle)) + index * sizeof(std::uint64_t)) & ~std::uintptr_t{3};
}

std::uint32_t MarkerWords(const char* marker, std::size_t extra) {
    const auto length = std::strlen(marker);
    return static_cast<std::uint32_t>(((length + 8 + extra) >> 2) + ((length + 0xc + extra) >> 3) * 2);
}

void WriteMarker(std::uint32_t* at, std::uint32_t kind, const char* marker, std::uint32_t words, const std::uint32_t* color) {
    const auto length = std::strlen(marker) + 1;
    *at = Header(OpNop, words + 1);
    at[1] = kind;
    auto* text = reinterpret_cast<std::uint8_t*>(at + 2);
    std::memset(text, 0, words * 4);
    std::memcpy(text, marker, length);
    if (color) std::memcpy(text + length + 8, color, sizeof(*color));
}

}

extern "C" {

int APS5_VABI sceGnmSetPsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs) {
    if (!cmd || size <= 0x27) return -1;
    if (!regs) {
        cmd = Emit(cmd, OpSetShReg, {8, 0, 0});
        cmd = Emit(cmd, OpSetContextReg, {0x203, 0});
        Nop(cmd, 0x20);
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

int APS5_VABI sceGnmSetVsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier) {
    if (!cmd || size <= 0x1c || !regs || (modifier & 0xfcfffc3fu) || regs[1] != 0) return -1;
    const std::uint32_t rsrc1 = modifier == 0 ? regs[2] : (regs[2] & 0xfcfffc3fu) | modifier;
    cmd = Emit(cmd, OpSetShReg, {0x48, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x4a, rsrc1, regs[3]});
    cmd = SetRegisters(cmd, OpSetContextReg, {{0x207, regs[6]}, {0x1b1, regs[4]}, {0x1c3, regs[5]}});
    Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmSetCsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs) {
    if (!cmd || size <= 0x18 || !regs || regs[1] != 0) return -1;
    cmd = Emit(cmd, OpSetShReg, {0x20c, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x212, regs[2], regs[3]});
    cmd = Emit(cmd, OpSetShReg, {0x207, regs[4], regs[5], regs[6]});
    Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmSetEsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier) {
    if (!cmd || size < 0x14 || !regs || (modifier & 0xfcfffc3fu) || regs[1] != 0) return -1;
    const std::uint32_t rsrc1 = modifier == 0 ? regs[2] : (regs[2] & 0xfcfffc3fu) | modifier;
    cmd = Emit(cmd, OpSetShReg, {0xc8, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0xca, rsrc1, regs[3]});
    Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmSetGsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs) {
    if (!cmd || size < 0x1d || !regs || regs[1] != 0) return -1;
    cmd = Emit(cmd, OpSetShReg, {0x88, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x8a, regs[2], regs[3]});
    cmd = SetRegisters(cmd, OpSetContextReg, {{0x2e5, regs[4]}, {0x29b, regs[5]}, {0x2e4, regs[6]}});
    Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmSetHsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t lsHsConfig) {
    if (!cmd || size < 0x1e || !regs || regs[1] != 0) return -1;
    cmd = Emit(cmd, OpSetShReg, {0x108, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x10a, regs[2], regs[3]});
    cmd = Emit(cmd, OpSetContextReg, {0x286, regs[5], regs[6]});
    cmd = SetRegisters(cmd, OpSetContextReg, {{0x2db, regs[4]}, {0x2d6, lsHsConfig}});
    Nop(cmd, 11);
    return 0;
}

int APS5_VABI sceGnmSetLsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier) {
    if (!cmd || size < 0x17 || !regs) return -1;
    const std::uint32_t mask = (modifier & 0xfffffc3fu) == 0 ? 0xfffffc3fu : 0xfcfffc3fu;
    if ((modifier & mask) || regs[1] != 0) return -1;
    const std::uint32_t rsrc1 = modifier == 0 ? regs[2] : (regs[2] & mask) | modifier;
    cmd = Emit(cmd, OpSetShReg, {0x148, regs[0], 0});
    cmd = Emit(cmd, OpSetShReg, {0x14b, regs[3]});
    cmd = Emit(cmd, OpSetShReg, {0x14a, rsrc1, regs[3]});
    Nop(cmd, 11);
    return 0;
}

std::uint32_t APS5_VABI sceGnmDrawInitDefaultHardwareState175(std::uint32_t* cmd, std::uint32_t size) {
    if (size < HwInitPacketWords) return 0;
    auto* at = Emit(cmd, OpContextControl, {0x80000000u, 0x80000000u});
    at = Emit(at, OpClearState, {0});
    at = Emit(at, OpAcquireMem, {0x2ec47fc0u, 0xffffffffu, 0, 0, 0, 10});
    at = DefaultState(at, 0x6020000);
    Nop(at, static_cast<std::uint32_t>(cmd + HwInitPacketWords - at) - 1);
    return HwInitPacketWords;
}

std::uint32_t APS5_VABI sceGnmDispatchInitDefaultHardwareState(std::uint32_t* cmd, std::uint32_t size) {
    if (size < HwInitPacketWords) return 0;
    auto* at = Emit(cmd, OpSetShReg, {0x216, 0xffffffffu}, true);
    at = Emit(at, OpSetShReg, {0x217, 0xffffffffu}, true);
    at = Emit(at, OpSetShReg, {0x215, 0x170}, true);
    at = Emit(at, OpAcquireMem, {0x28000000u, 0, 0, 0, 0, 10});
    Nop(at, static_cast<std::uint32_t>(cmd + HwInitPacketWords - at) - 1);
    return HwInitPacketWords;
}

int APS5_VABI sceGnmInsertPushMarker(std::uint32_t* cmd, std::uint32_t size, const char* marker) {
    if (!cmd || !marker) return -1;
    const auto words = MarkerWords(marker, 0);
    if (words + 2 != size) return -1;
    WriteMarker(cmd, MarkerPush, marker, words, nullptr);
    return 0;
}

int APS5_VABI sceGnmInsertSetMarker(std::uint32_t* cmd, std::uint32_t size, const char* marker) {
    if (!cmd || !marker) return -1;
    const auto words = MarkerWords(marker, 0);
    if (words + 2 != size) return -1;
    WriteMarker(cmd, MarkerSet, marker, words, nullptr);
    return 0;
}

int APS5_VABI sceGnmInsertPushColorMarker(std::uint32_t* cmd, std::uint32_t size, const char* marker, std::uint32_t color) {
    if (!cmd || !marker) return -1;
    const auto words = MarkerWords(marker, 4);
    if (words + 2 != size) return -1;
    WriteMarker(cmd, MarkerColorPush, marker, words, &color);
    return 0;
}

int APS5_VABI sceGnmInsertPopMarker(std::uint32_t* cmd, std::uint32_t size) {
    if (!cmd || size != 6) return -1;
    Emit(cmd, OpNop, {MarkerPop, 0, 0, 0, 0});
    return 0;
}

int APS5_VABI sceGnmInsertSetColorMarker() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmInsertWaitFlipDone(std::uint32_t* cmd, std::uint32_t size, std::uint32_t handle, std::uint32_t index) {
    if (size != 7) return -1;
    const auto label = BufferLabel(handle, index);
    Emit(cmd, OpWaitRegMem, {WaitMemoryEqual, static_cast<std::uint32_t>(label), static_cast<std::uint32_t>(label >> 32), 0, 0xffffffffu, 10});
    return 0;
}

int APS5_VABI sceGnmSubmitCommandBuffers(std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs, const std::uint32_t* ccbBytes) {
    if (!dcbs || !dcbBytes) return GNM_ERROR_SUBMISSION_FAILED_INVALID_ARGUMENT;
    if (const int error = CheckSubmission(count, dcbs, dcbBytes, ccbs, ccbBytes)) return error;
    std::lock_guard lock(g_submitMutex);
    SubmitLocked(count, dcbs, dcbBytes, ccbs, ccbBytes);
    return 0;
}

int APS5_VABI sceGnmSubmitAndFlipCommandBuffers(std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs, const std::uint32_t* ccbBytes,
    std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg) {
    if (count == 0) return GNM_ERROR_SUBMISSION_FAILED_INVALID_ARGUMENT;
    if (const int error = CheckSubmission(count, dcbs, dcbBytes, ccbs, ccbBytes)) return error;
    if (dcbBytes[count - 1] < FlipReserveWords * 4) throw std::runtime_error("GnmDriver: the last command buffer has no room for a flip reservation");
    std::lock_guard lock(g_submitMutex);
    GnmDriver::PatchFlip(dcbs[count - 1] + dcbBytes[count - 1] / 4 - FlipReserveWords, BufferLabel(handle, index), handle, index, flipMode, flipArg);
    SubmitLocked(count, dcbs, dcbBytes, ccbs, ccbBytes);
    return 0;
}

int APS5_VABI sceGnmSubmitDone() {
    std::lock_guard lock(g_submitMutex);
    g_frameStarted = false;
    return 0;
}

int APS5_VABI sceGnmAreSubmitsAllowed() {
    return 1;
}

void APS5_VABI sceGnmFlushGarlic() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

int APS5_VABI sceGnmAddEqEvent(KernelEqueue eq, std::uint64_t id, void* udata) {
    if (id != GfxEndOfPipeEvent) NotImplemented_nid_no_patch("sceGnmAddEqEvent for a compute queue event");
    return AgcDriverAddQueueEqEvent_nid_postfix(eq, 0, static_cast<std::uintptr_t>(id), udata);
}

int APS5_VABI sceGnmDeleteEqEvent(KernelEqueue eq, std::uint64_t id) {
    return AgcDriverDeleteQueueEqEvent_nid_postfix(eq, static_cast<std::uintptr_t>(id));
}

int APS5_VABI sceGnmGetEqEventType(const KernelEvent* ev) {
    if (!ev || ev->filter != EvfiltGraphicsCore) throw std::runtime_error(std::string(__func__) + ": not a graphics event");
    return static_cast<int>(ev->ident);
}

bool APS5_VABI sceGnmIsUserPaEnabled() {
    return false;
}

std::uint32_t APS5_VABI sceGnmLogicalCuMaskToPhysicalCuMask(std::int64_t, std::uint32_t logicalMask) {
    return logicalMask;
}

bool APS5_VABI sceGnmDriverCaptureInProgress() {
    return false;
}

int APS5_VABI sceGnmDriverTriggerCapture() {
    return GNM_ERROR_CAPTURE_RAZOR_NOT_LOADED;
}

int APS5_VABI sceGnmValidateCommandBuffers() {
    return GNM_ERROR_VALIDATION_NOT_ENABLED;
}

bool APS5_VABI sceRazorIsLoaded() {
    return false;
}

int APS5_VABI sceRazorCaptureImmediate() {
    return GNM_ERROR_CAPTURE_FAILED_INTERNAL;
}

int APS5_VABI sceRazorCaptureSinceLastFlip() {
    return GNM_ERROR_CAPTURE_FAILED_INTERNAL;
}

int APS5_VABI sceRazorCaptureCommandBuffersOnlyImmediate() {
    return GNM_ERROR_CAPTURE_FAILED_INTERNAL;
}

int APS5_VABI sceRazorCaptureCommandBuffersOnlySinceLastFlip() {
    return GNM_ERROR_CAPTURE_FAILED_INTERNAL;
}

int APS5_VABI sceGnmDebugHardwareStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmDingDong() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmDisableMipStatsReport() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmGetEqTimeStamp() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmGetTheTessellationFactorRingBufferBaseAddress() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmLogicalCuIndexToPhysicalCuIndex() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmMapComputeQueue() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmRequestFlipAndSubmitDone(std::uint32_t* commands, std::uint32_t sizeInBytes, std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg) {
    if (sizeInBytes < FlipReserveWords * 4) return GNM_ERROR_SUBMISSION_NOT_ENOUGH_RESOURCES;
    if (!commands) return GNM_ERROR_SUBMISSION_FAILED_INVALID_ARGUMENT;
    std::lock_guard lock(g_submitMutex);
    commands[0] = PrepareFlipHeader;
    commands[1] = PrepareFlip;
    GnmDriver::PatchFlip(commands, BufferLabel(handle, index), handle, index, flipMode, flipArg);
    std::uint32_t* const dcbs[] = {commands};
    const std::uint32_t bytes = FlipReserveWords * 4;
    SubmitLocked(1, dcbs, &bytes, nullptr, nullptr);
    g_frameStarted = false;
    return 0;
}

int APS5_VABI sceGnmRequestMipStatsReportAndReset() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceGnmSetGsRingSizes(std::uint32_t esgsRingSize, std::uint32_t gsvsRingSize) {
    for (const auto size : {esgsRingSize, gsvsRingSize}) {
        if (size < MinGsRingSize || size > MaxGsRingSize || (size & (GsRingAlignment - 1)) != 0) return GNM_GS_RING_SIZE_INVALID;
    }
    return 0;
}

int APS5_VABI sceGnmSetupMipStatsReport(void* outputBuffer, std::uint32_t sizeInBytes, std::uint8_t intervalsBetweenReports, std::uint8_t numReportsBeforeReset, std::uint32_t mipStatsResetForce) {
    static_cast<void>(intervalsBetweenReports);
    static_cast<void>(numReportsBeforeReset);
    static_cast<void>(mipStatsResetForce);
    if (outputBuffer == nullptr || (reinterpret_cast<std::uintptr_t>(outputBuffer) & 0x3fu) != 0) return GNM_MIP_STATS_INVALID_BUFFER;
    if (sizeInBytes < MinMipStatsBytes || (sizeInBytes & 0x7ffu) != MipStatsHeaderBytes) return GNM_MIP_STATS_INVALID_SIZE;
    std::memset(outputBuffer, 0, sizeInBytes);
    *static_cast<std::uint32_t*>(outputBuffer) = MipStatsHeaderBytes;
    return 0;
}

int APS5_VABI sceGnmUnmapComputeQueue() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
