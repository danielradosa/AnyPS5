#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <source_location>
#include <span>
#include <vector>
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceGnmDriver/include/Lowering.hpp"
#ifdef _WIN32
#include <windows.h>
#endif

extern "C" {
int APS5_VABI sceGnmBeginWorkload(std::uint32_t workloadStream, std::uint64_t* workload);
int APS5_VABI sceGnmEndWorkload(std::uint64_t workload);
int APS5_VABI sceGnmCreateWorkloadStream(std::uint64_t argument, std::uint32_t* workloadStream);
int APS5_VABI sceGnmComputeWaitOnAddress(std::uint32_t* cmd, std::uint32_t size, std::uintptr_t address, std::uint32_t mask, std::uint32_t compareFunction, std::uint32_t reference);
int APS5_VABI sceGnmComputeWaitSemaphore(std::uint32_t* cmd, std::uint32_t size, std::uint64_t semaphore, std::uint32_t flags);
int APS5_VABI sceGnmDispatchDirect(std::uint32_t* cmd, std::uint32_t size, std::uint32_t threadsX, std::uint32_t threadsY, std::uint32_t threadsZ, std::uint32_t flags);
int APS5_VABI sceGnmDispatchIndirect(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t flags);
int APS5_VABI sceGnmDispatchIndirectOnMec(std::uint32_t* cmd, std::uint32_t size, std::uint64_t arguments, std::uint32_t modifier);
int APS5_VABI sceGnmDrawIndex(std::uint32_t* cmd, std::uint32_t size, std::uint32_t indexCount, std::uintptr_t indexAddress, std::uint32_t flags, std::uint32_t type);
int APS5_VABI sceGnmDrawIndexAuto(std::uint32_t* cmd, std::uint32_t size, std::uint32_t indexCount, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndexIndirect(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndexIndirectCountMulti(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint64_t countAddress, std::uint32_t stage,
    std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndexIndirectMulti(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint32_t stage, std::uint32_t vertexSgpr,
    std::uint32_t instanceSgpr, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndexOffset(std::uint32_t* cmd, std::uint32_t size, std::uint32_t indexOffset, std::uint32_t indexCount, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndirect(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t stage, std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndirectCountMulti(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint64_t countAddress, std::uint32_t stage,
    std::uint32_t vertexSgpr, std::uint32_t instanceSgpr, std::uint32_t flags);
int APS5_VABI sceGnmDrawIndirectMulti(std::uint32_t* cmd, std::uint32_t size, std::uint32_t dataOffset, std::uint32_t maxCount, std::uint32_t stage, std::uint32_t vertexSgpr,
    std::uint32_t instanceSgpr, std::uint32_t flags);
std::uint32_t APS5_VABI sceGnmDrawInitDefaultHardwareState(std::uint32_t* cmd, std::uint32_t size);
std::uint32_t APS5_VABI sceGnmDrawInitDefaultHardwareState200(std::uint32_t* cmd, std::uint32_t size);
std::uint32_t APS5_VABI sceGnmDrawInitDefaultHardwareState350(std::uint32_t* cmd, std::uint32_t size);
std::uint32_t APS5_VABI sceGnmDrawInitToDefaultContextState(std::uint32_t* cmd, std::uint32_t size);
std::uint32_t APS5_VABI sceGnmDrawInitToDefaultContextState400(std::uint32_t* cmd, std::uint32_t size);
int APS5_VABI sceGnmDrawInitToDefaultContextStateInternalCommand(std::uint32_t* cmd, std::uint32_t size);
int APS5_VABI sceGnmDrawInitToDefaultContextStateInternalSize();
std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterface();
std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterfaceForGpuDebugger();
std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterfaceForReplay();
std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterfaceForResourceRegistration();
std::uint32_t APS5_VABI sceGnmDriverInternalRetrieveGnmInterfaceForValidation();
int APS5_VABI sceGnmDriverInternalVirtualQuery();
bool APS5_VABI sceGnmDriverTraceInProgress();
int APS5_VABI sceGnmDebugModuleReset();
int APS5_VABI sceGnmDebugReset();
int APS5_VABI sceGnmFindResources();
int APS5_VABI sceGnmFindResourcesPublic();
int APS5_VABI sceGnmGetDbgGcHandle();
int APS5_VABI sceGnmGetGpuBlockStatus();
std::uint32_t APS5_VABI sceGnmGetGpuCoreClockFrequency();
int APS5_VABI sceGnmGetGpuInfoStatus();
int APS5_VABI sceGnmGetLastWaitedAddress();
int APS5_VABI sceGnmGetOffChipTessellationBufferSize();
int APS5_VABI sceGnmGetOwnerName();
int APS5_VABI sceGnmGetPhysicalCounterFromVirtualized();
std::uint32_t APS5_VABI sceGnmGetProtectionFaultTimeStamp();
int APS5_VABI sceGnmGetResourceBaseAddressAndSizeInBytes();
int APS5_VABI sceGnmGetResourceName();
int APS5_VABI sceGnmGetResourceRegistrationBuffers();
int APS5_VABI sceGnmGetResourceShaderGuid();
int APS5_VABI sceGnmGetResourceType();
int APS5_VABI sceGnmGetResourceUserData();
int APS5_VABI sceGnmGetShaderProgramBaseAddress();
int APS5_VABI sceGnmGetShaderStatus();
void APS5_VABI sceGnmGpuPaDebugEnter();
void APS5_VABI sceGnmGpuPaDebugLeave();
int APS5_VABI sceGnmInsertDingDongMarker(std::uint32_t* cmd, std::uint32_t size);
int APS5_VABI sceGnmInsertThreadTraceMarker();
int APS5_VABI sceGnmPaDisableFlipCallbacks();
int APS5_VABI sceGnmPaEnableFlipCallbacks();
int APS5_VABI sceGnmPaHeartbeat();
int APS5_VABI sceGnmQueryResourceRegistrationUserMemoryRequirements();
int APS5_VABI sceGnmRaiseUserExceptionEvent();
int APS5_VABI sceGnmRegisterGdsResource();
void APS5_VABI sceGnmRegisterGnmLiveCallbackConfig();
int APS5_VABI sceGnmRegisterOwner(void* handle, const char* name);
int APS5_VABI sceGnmRegisterOwnerForSystem();
int APS5_VABI sceGnmRegisterResource(void* resourceHandle, void* ownerHandle, const void* address, std::size_t size, const char* name, int type, std::uint64_t userData);
int APS5_VABI sceGnmRequestFlipAndSubmitDoneForWorkload(std::uint64_t workload, std::uint32_t* commands, std::uint32_t sizeInBytes, std::uint32_t handle, std::uint32_t index,
    std::uint32_t flipMode, std::int64_t flipArg);
int APS5_VABI sceGnmResetVgtControl(std::uint32_t* cmd, std::uint32_t size);
int APS5_VABI sceGnmSdmaClose();
int APS5_VABI sceGnmSdmaConstFill();
int APS5_VABI sceGnmSdmaCopyLinear();
int APS5_VABI sceGnmSdmaCopyTiled();
int APS5_VABI sceGnmSdmaCopyWindow();
int APS5_VABI sceGnmSdmaFlush();
int APS5_VABI sceGnmSdmaGetMinCmdSize();
int APS5_VABI sceGnmSdmaOpen();
int APS5_VABI sceGnmSetCsShaderWithModifier(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier);
int APS5_VABI sceGnmSetEmbeddedPsShader(std::uint32_t* cmd, std::uint32_t size, std::uint32_t shaderId, std::uint32_t modifier);
int APS5_VABI sceGnmSetEmbeddedVsShader(std::uint32_t* cmd, std::uint32_t size, std::uint32_t shaderId, std::uint32_t modifier);
int APS5_VABI sceGnmSetPsShader350(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs);
int APS5_VABI sceGnmSetResourceRegistrationUserMemory();
int APS5_VABI sceGnmSetResourceUserData();
int APS5_VABI sceGnmSetSpiEnableSqCounters();
int APS5_VABI sceGnmSetSpiEnableSqCountersForUnitInstance();
int APS5_VABI sceGnmSetVgtControl(std::uint32_t* cmd, std::uint32_t size, std::uint32_t primGroupSizeMinusOne, std::uint32_t partialVsWaveMode, std::uint32_t wdSwitchOnlyOnEopMode);
int APS5_VABI sceGnmSubmitAndFlipCommandBuffersForWorkload(std::uint32_t workload, std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs,
    const std::uint32_t* ccbBytes, std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg);
int APS5_VABI sceGnmSubmitCommandBuffersForWorkload(std::uint32_t workload, std::uint32_t count, std::uint32_t* const* dcbs, const std::uint32_t* dcbBytes, std::uint32_t* const* ccbs,
    const std::uint32_t* ccbBytes);
int APS5_VABI sceGnmUnregisterAllResourcesForOwner();
int APS5_VABI sceGnmUnregisterOwnerAndResources();
int APS5_VABI sceGnmUnregisterResource();
int APS5_VABI sceGnmUpdateGsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs);
int APS5_VABI sceGnmUpdateHsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t lsHsConfig);
int APS5_VABI sceGnmUpdatePsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs);
int APS5_VABI sceGnmUpdatePsShader350(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs);
int APS5_VABI sceGnmUpdateVsShader(std::uint32_t* cmd, std::uint32_t size, const std::uint32_t* regs, std::uint32_t modifier);
int APS5_VABI sceGnmValidateDisableDiagnostics();
int APS5_VABI sceGnmValidateDisableDiagnostics2();
int APS5_VABI sceGnmValidateDispatchCommandBuffers();
int APS5_VABI sceGnmValidateDrawCommandBuffers();
int APS5_VABI sceGnmValidateGetDiagnosticInfo();
int APS5_VABI sceGnmValidateGetDiagnostics();
int APS5_VABI sceGnmValidateGetVersion();
bool APS5_VABI sceGnmValidateOnSubmitEnabled();
int APS5_VABI sceGnmValidateResetState();
int APS5_VABI sceGnmValidationRegisterMemoryCheckCallback();
}

#ifdef _WIN32
[[gnu::section(".procpar"), gnu::used]] std::uint32_t descriptor[2]{};
#endif

namespace {

using GnmDriver::Header;
using StateWriter = std::uint32_t (APS5_VABI*)(std::uint32_t*, std::uint32_t);

constexpr int SubmissionInvalidArgument = static_cast<int>(0x80d11000u);
constexpr int SubmissionNotEnoughResources = static_cast<int>(0x80d11001u);
constexpr int ValidationNotEnabled = static_cast<int>(0x80d13fffu);
constexpr int Failure = static_cast<int>(0x8eee00ffu);
constexpr std::uint32_t Fill = 0xeeeeeeeeu;
constexpr std::uint32_t Nop2 = 0xc0011000u;
constexpr std::uint32_t Nop3 = 0xc0021000u;
constexpr std::uint32_t Nop11 = 0xc00a1000u;
constexpr std::uint32_t SetSh3 = 0xc0027600u;
constexpr std::uint32_t SetSh4 = 0xc0037600u;
constexpr std::uint32_t SetContext2 = 0xc0016900u;
constexpr std::uint32_t SetContext3 = 0xc0026900u;
constexpr std::uint32_t SetUconfig2 = 0xc0017900u;

constexpr std::uint32_t ClearContextState[]{0xc0012800u, 0x80000000u, 0x80000000u, 0xc0001200u, 0, 0xc0055800u, 0x2ec47fc0u, 0xffffffffu, 0, 0, 0, 10};

constexpr std::uint32_t Sdk100Sequence[]{
    0xc0017600, 0x216, 0xffffffff, 0xc0017600, 0x217, 0xffffffff, 0xc0017600, 0x215, 0x0, 0xc0016900,
    0x2f9, 0x2d, 0xc0016900, 0x282, 0x8, 0xc0016900, 0x280, 0x80008, 0xc0016900, 0x281,
    0xffff0000, 0xc0016900, 0x204, 0x0, 0xc0016900, 0x206, 0x43f, 0xc0016900, 0x83, 0xffff,
    0xc0016900, 0x317, 0x10, 0xc0016900, 0x2fa, 0x3f800000, 0xc0016900, 0x2fc, 0x3f800000, 0xc0016900,
    0x2fb, 0x3f800000, 0xc0016900, 0x2fd, 0x3f800000, 0xc0016900, 0x202, 0xcc0010, 0xc0016900, 0x30e,
    0xffffffff, 0xc0016900, 0x30f, 0xffffffff, 0xc0002f00, 0x1, 0xc0017600, 0x7, 0x1ff, 0xc0017600,
    0x46, 0x1ff, 0xc0017600, 0x87, 0x1ff, 0xc0017600, 0xc7, 0x1ff, 0xc0017600, 0x107,
    0x0, 0xc0017600, 0x147, 0x1ff, 0xc0016900, 0x1b1, 0x2, 0xc0016900, 0x101, 0x0,
    0xc0016900, 0x100, 0xffffffff, 0xc0016900, 0x103, 0x0, 0xc0016900, 0x284, 0x0, 0xc0016900,
    0x290, 0x0, 0xc0016900, 0x2ae, 0x0, 0xc0016900, 0x292, 0x0, 0xc0016900, 0x293,
    0x6000000, 0xc0016900, 0x2f8, 0x0, 0xc0016900, 0x2de, 0x1e9, 0xc0036900, 0x295, 0x100,
    0x100, 0x4, 0xc0017900, 0x200, 0xe0000000,
};

constexpr std::uint32_t Neo200Sequence[]{
    0xc0017600, 0x216, 0xffffffff, 0xc0017600, 0x217, 0xffffffff, 0xc0017600, 0x219, 0xffffffff, 0xc0017600,
    0x21a, 0xffffffff, 0xc0017600, 0x215, 0x0, 0xc0016900, 0x2f9, 0x2d, 0xc0016900, 0x282,
    0x8, 0xc0016900, 0x280, 0x80008, 0xc0016900, 0x281, 0xffff0000, 0xc0016900, 0x204, 0x0,
    0xc0016900, 0x206, 0x43f, 0xc0016900, 0x83, 0xffff, 0xc0016900, 0x317, 0x10, 0xc0016900,
    0x2fa, 0x3f800000, 0xc0016900, 0x2fc, 0x3f800000, 0xc0016900, 0x2fb, 0x3f800000, 0xc0016900, 0x2fd,
    0x3f800000, 0xc0016900, 0x202, 0xcc0010, 0xc0016900, 0x30e, 0xffffffff, 0xc0016900, 0x30f, 0xffffffff,
    0xc0002f00, 0x1, 0xc0017600, 0x7, 0x1701ff, 0xc0017600, 0x46, 0x1701fd, 0xc0017600, 0x87,
    0x1701ff, 0xc0017600, 0xc7, 0x1701fd, 0xc0017600, 0x107, 0x17, 0xc0017600, 0x147, 0x1701fd,
    0xc0017600, 0x47, 0x1c, 0xc0016900, 0x1b1, 0x2, 0xc0016900, 0x101, 0x0, 0xc0016900,
    0x100, 0xffffffff, 0xc0016900, 0x103, 0x0, 0xc0016900, 0x284, 0x0, 0xc0016900, 0x290,
    0x0, 0xc0016900, 0x2ae, 0x0, 0xc0016900, 0x292, 0x0, 0xc0016900, 0x293, 0x6020000,
    0xc0016900, 0x2f8, 0x0, 0xc0016900, 0x2de, 0x1e9, 0xc0026900, 0xeb, 0xff00ff00, 0xff00,
    0xc0036900, 0x295, 0x100, 0x100, 0x4, 0xc0017900, 0x200, 0xe0000000, 0xc0017900, 0x40000258,
    0x6d007f,
};

constexpr std::uint32_t Neo350Sequence[]{
    0xc0017600, 0x216, 0xffffffff, 0xc0017600, 0x217, 0xffffffff, 0xc0017600, 0x219, 0xffffffff, 0xc0017600,
    0x21a, 0xffffffff, 0xc0017600, 0x215, 0x0, 0xc0016900, 0x2f9, 0x2d, 0xc0016900, 0x282,
    0x8, 0xc0016900, 0x280, 0x80008, 0xc0016900, 0x281, 0xffff0000, 0xc0016900, 0x204, 0x0,
    0xc0016900, 0x206, 0x43f, 0xc0016900, 0x83, 0xffff, 0xc0016900, 0x317, 0x10, 0xc0016900,
    0x2fa, 0x3f800000, 0xc0016900, 0x2fc, 0x3f800000, 0xc0016900, 0x2fb, 0x3f800000, 0xc0016900, 0x2fd,
    0x3f800000, 0xc0016900, 0x202, 0xcc0010, 0xc0016900, 0x30e, 0xffffffff, 0xc0016900, 0x30f, 0xffffffff,
    0xc0002f00, 0x1, 0xc0017600, 0x7, 0x1701ff, 0xc0017600, 0x46, 0x1701fd, 0xc0017600, 0x87,
    0x1701ff, 0xc0017600, 0xc7, 0x1701fd, 0xc0017600, 0x107, 0x17, 0xc0017600, 0x147, 0x1701fd,
    0xc0017600, 0x47, 0x1c, 0xc0016900, 0x1b1, 0x2, 0xc0016900, 0x101, 0x0, 0xc0016900,
    0x100, 0xffffffff, 0xc0016900, 0x103, 0x0, 0xc0016900, 0x284, 0x0, 0xc0016900, 0x290,
    0x0, 0xc0016900, 0x2ae, 0x0, 0xc0016900, 0x102, 0x0, 0xc0016900, 0x292, 0x0,
    0xc0016900, 0x293, 0x6020000, 0xc0016900, 0x2f8, 0x0, 0xc0016900, 0x2de, 0x1e9, 0xc0026900,
    0xeb, 0xff00ff00, 0xff00, 0xc0036900, 0x295, 0x100, 0x100, 0x4, 0xc0017900, 0x200,
    0xe0000000, 0xc0017900, 0x40000258, 0x6d007f,
};

constexpr std::uint32_t NeoContextSequence[]{
    0xc0012800, 0x80000000, 0x80000000, 0xc0001200, 0x0, 0xc0002f00, 0x1, 0xc0016900, 0x102, 0x0,
    0xc0016900, 0x202, 0xcc0010, 0xc0026900, 0xeb, 0xff00ff00, 0xff00, 0xc00d1000, 0x0,
};

constexpr std::uint32_t NeoContext400Sequence[]{
    0xc0012800, 0x80000000, 0x80000000, 0xc0001200, 0x0, 0xc0016900, 0x2f9, 0x2d, 0xc0016900, 0x282,
    0x8, 0xc0016900, 0x280, 0x80008, 0xc0016900, 0x281, 0xffff0000, 0xc0016900, 0x204, 0x0,
    0xc0016900, 0x206, 0x43f, 0xc0016900, 0x83, 0xffff, 0xc0016900, 0x317, 0x10, 0xc0016900,
    0x2fa, 0x3f800000, 0xc0016900, 0x2fc, 0x3f800000, 0xc0016900, 0x2fb, 0x3f800000, 0xc0016900, 0x2fd,
    0x3f800000, 0xc0016900, 0x202, 0xcc0010, 0xc0016900, 0x30e, 0xffffffff, 0xc0016900, 0x30f, 0xffffffff,
    0xc0002f00, 0x1, 0xc0016900, 0x1b1, 0x2, 0xc0016900, 0x101, 0x0, 0xc0016900, 0x100,
    0xffffffff, 0xc0016900, 0x103, 0x0, 0xc0016900, 0x284, 0x0, 0xc0016900, 0x290, 0x0,
    0xc0016900, 0x2ae, 0x0, 0xc0016900, 0x102, 0x0, 0xc0016900, 0x292, 0x0, 0xc0016900,
    0x293, 0x6020000, 0xc0016900, 0x2f8, 0x0, 0xc0016900, 0x2de, 0x1e9, 0xc0026900, 0xeb,
    0xff00ff00, 0xff00, 0xc0036900, 0x295, 0x100, 0x100, 0x4, 0xc0017900, 0x40000258, 0x6d007f,
    0xc09a1000, 0x0,
};

void Require(bool value, std::source_location where = std::source_location::current()) {
    if (!value) {
        std::fprintf(stderr, "GnmDriverForNeoMode check failed at line %u\n", static_cast<unsigned>(where.line()));
        std::abort();
    }
}

std::vector<std::uint32_t> Buffer(std::size_t words) {
    return std::vector<std::uint32_t>(words, Fill);
}

std::vector<std::uint32_t> Words(std::initializer_list<std::uint32_t> words) {
    return words;
}

std::vector<std::uint32_t> Append(std::vector<std::uint32_t> words, std::size_t zeros) {
    words.insert(words.end(), zeros, 0u);
    return words;
}

bool Untouched(const std::vector<std::uint32_t>& buffer) {
    for (const auto word : buffer) {
        if (word != Fill) return false;
    }
    return true;
}

bool Matches(const std::uint32_t* at, std::span<const std::uint32_t> expected) {
    return std::memcmp(at, expected.data(), expected.size_bytes()) == 0;
}

void CheckHardwareState(StateWriter write, std::span<const std::uint32_t> expected) {
    auto buffer = Buffer(0x100);
    Require(write(buffer.data(), 0xff) == 0 && Untouched(buffer));
    Require(write(buffer.data(), 0x100) == 0x100);
    Require(Matches(buffer.data(), ClearContextState));
    Require(Matches(buffer.data() + 12, expected));
    const auto rest = static_cast<std::uint32_t>(0x100 - 12 - expected.size());
    Require(buffer[12 + expected.size()] == Header(0x10, rest - 1));
    for (std::size_t at = 13 + expected.size(); at < 0x100; ++at) Require(buffer[at] == 0);
}

void CheckContextState(StateWriter write, std::uint32_t words, std::span<const std::uint32_t> expected) {
    auto buffer = Buffer(words + 1);
    Require(write(buffer.data(), words - 1) == 0 && write(buffer.data(), words + 1) == 0 && Untouched(buffer));
    Require(write(buffer.data(), words) == words);
    Require(Matches(buffer.data(), expected));
    for (std::size_t at = expected.size(); at < words; ++at) Require(buffer[at] == 0);
    Require(buffer[words] == Fill);
}

void CheckDraws() {
    auto buffer = Buffer(10);
    Require(sceGnmDrawIndex(buffer.data(), 9, 0x30, 0x123456789abcull, 0, 0) == -1);
    Require(sceGnmDrawIndex(buffer.data(), 10, 0x30, 0x123456789abdull, 0, 0) == -1);
    Require(sceGnmDrawIndex(buffer.data(), 10, 0x30, 0, 0, 0) == -1);
    Require(sceGnmDrawIndex(buffer.data(), 10, 0x30, 0x123456789abcull, 2, 0) == -1);
    Require(Untouched(buffer));
    Require(sceGnmDrawIndex(buffer.data(), 10, 0x30, 0x123456789abcull, 0xa0000001u, 1) == 0);
    Require(buffer == Words({0xc0042700u, 0x30, 0x56789abcu, 0x1234, 0x30, 0xa0000000u, Nop3, 0, 0, 0}));

    buffer = Buffer(7);
    Require(sceGnmDrawIndexAuto(buffer.data(), 7, 12, 0x100) == -1 && Untouched(buffer));
    Require(sceGnmDrawIndexAuto(buffer.data(), 7, 12, 0x20000000u) == 0);
    Require(buffer == Words({0xc0012d00u, 12, 0x20000002u, Nop3, 0, 0, 0}));

    buffer = Buffer(9);
    Require(sceGnmDrawIndexIndirect(buffer.data(), 9, 0x40, 7, 0, 0, 0) == -1);
    Require(sceGnmDrawIndexIndirect(buffer.data(), 9, 0x40, 2, 0x10, 0, 0) == -1);
    Require(sceGnmDrawIndexIndirect(buffer.data(), 8, 0x40, 2, 0, 0, 0) == -1 && Untouched(buffer));
    Require(sceGnmDrawIndexIndirect(buffer.data(), 9, 0x40, 2, 3, 4, 0x40000001u) == 0);
    Require(buffer == Words({0xc0032501u, 0x40, 0x4f, 0x50, 0x40000000u, Nop3, 0, 0, 0}));

    buffer = Buffer(9);
    Require(sceGnmDrawIndirect(buffer.data(), 9, 0x20, 0, 1, 2, 0) == 0);
    Require(buffer == Words({0xc0032400u, 0x20, 1, 2, 2, Nop3, 0, 0, 0}));

    buffer = Buffer(9);
    Require(sceGnmDrawIndexOffset(buffer.data(), 8, 100, 36, 0) == -1 && Untouched(buffer));
    Require(sceGnmDrawIndexOffset(buffer.data(), 9, 100, 36, 0xe0000001u) == 0);
    Require(buffer == Words({0xc0033501u, 36, 100, 36, 0xe0000000u, Nop3, 0, 0, 0}));

    buffer = Buffer(16);
    Require(sceGnmDrawIndexIndirectCountMulti(buffer.data(), 16, 0x80, 5, 0x1000002000ull, 1, 0, 1, 1) == -1 && Untouched(buffer));
    Require(sceGnmDrawIndexIndirectCountMulti(buffer.data(), 16, 0x80, 5, 0x1000002000ull, 4, 0, 1, 1) == 0);
    Require(buffer == Words({Nop2, 0, 0, 0xc0089d01u, 0x80, 0, 0xcd, 1u << 30, 5, 0x2000, 0x10, 0x14, 0, Nop2, 0, 0}));

    buffer = Buffer(16);
    Require(sceGnmDrawIndirectCountMulti(buffer.data(), 16, 0, 7, 0, 1, 0, 0, 0x80000000u) == 0);
    Require(buffer == Words({Nop2, 0, 0, 0xc0089c00u, 0, 0, 0, 0, 7, 0, 0, 0x10, 0x80000002u, Nop2, 0, 0}));

    buffer = Buffer(11);
    Require(sceGnmDrawIndexIndirectMulti(buffer.data(), 11, 0x10, 3, 3, 2, 0, 0) == -1 && Untouched(buffer));
    Require(sceGnmDrawIndexIndirectMulti(buffer.data(), 11, 0x10, 3, 6, 2, 0, 0) == 0);
    Require(buffer == Words({0xc0053800u, 0x10, 0x14e, 0, 3, 0x14, 0, Nop3, 0, 0, 0}));

    buffer = Buffer(11);
    Require(sceGnmDrawIndirectMulti(buffer.data(), 11, 4, 9, 2, 0, 5, 1) == 0);
    Require(buffer == Words({0xc0052c01u, 4, 0, 0x51, 9, 0x10, 2, Nop3, 0, 0, 0}));
}

void CheckCompute() {
    auto buffer = Buffer(9);
    Require(sceGnmDispatchDirect(buffer.data(), 9, 0x80000000u, 1, 1, 0) == -1 && Untouched(buffer));
    Require(sceGnmDispatchDirect(buffer.data(), 9, 8, 4, 1, 0x19) == 0);
    Require(buffer == Words({0xc0031503u, 8, 4, 1, 0x19, Nop3, 0, 0, 0}));

    buffer = Buffer(7);
    Require(sceGnmDispatchIndirect(buffer.data(), 7, 0x30, 8) == 0);
    Require(buffer == Words({0xc0011602u, 0x30, 9, Nop3, 0, 0, 0}));

    buffer = Buffer(8);
    Require(sceGnmDispatchIndirectOnMec(buffer.data(), 8, 0x1234567892ull, 0) == -1);
    Require(sceGnmDispatchIndirectOnMec(buffer.data(), 8, 0, 0) == -1);
    Require(sceGnmDispatchIndirectOnMec(buffer.data(), 7, 0x1234567890ull, 0) == -1 && Untouched(buffer));
    Require(sceGnmDispatchIndirectOnMec(buffer.data(), 8, 0x1234567890ull, 0x11) == 0);
    Require(buffer == Words({0xc0021603u, 0x34567890u, 0x12, 0x11, Nop3, 0, 0, 0}));

    buffer = Buffer(14);
    Require(sceGnmComputeWaitOnAddress(buffer.data(), 13, 0x4000001006ull, 0xff, 3, 7) == -1 && Untouched(buffer));
    Require(sceGnmComputeWaitOnAddress(buffer.data(), 14, 0x4000001006ull, 0xff, 3, 7) == 0);
    Require(buffer == Words({Nop3, 0, 0, 0, 0xc0053c00u, 0x13, 0x1004, 0x40, 7, 0xff, 10, Nop2, 0, 0}));
    Require(sceGnmComputeWaitOnAddress(buffer.data(), 14, 0x1234, 0xffffffffu, 0xb, 1) == 0);
    Require(buffer == Words({Nop3, 0, 0, 0, 0xc0053c00u, 3, 0x1234, 0, 1, 0xffffffffu, 10, Nop2, 0, 0}));

    buffer = Buffer(10);
    Require(sceGnmComputeWaitSemaphore(buffer.data(), 9, 0x500000100ull, 1) == -1 && Untouched(buffer));
    Require(sceGnmComputeWaitSemaphore(buffer.data(), 10, 0x500000100ull, 1) == 0);
    Require(buffer == Words({Nop3, 0, 0, 0, 0xc0013900u, 0x100, 0xe0100005u, Nop2, 0, 0}));
    Require(sceGnmComputeWaitSemaphore(buffer.data(), 10, 0x500000100ull, 0) == 0 && buffer[6] == 0xe0000005u);
}

void CheckControl() {
    auto buffer = Buffer(4);
    Require(sceGnmInsertDingDongMarker(buffer.data(), 3) == -1 && Untouched(buffer));
    Require(sceGnmInsertDingDongMarker(buffer.data(), 4) == 0 && buffer == Words({Nop3, 0, 0, 0}));

    buffer = Buffer(3);
    Require(sceGnmResetVgtControl(buffer.data(), 4) == -1 && Untouched(buffer));
    Require(sceGnmResetVgtControl(buffer.data(), 3) == 0 && buffer == Words({SetUconfig2, 0x40000258u, 0x6d007fu}));

    buffer = Buffer(3);
    Require(sceGnmSetVgtControl(buffer.data(), 3, 0x100, 0, 0) == -1);
    Require(sceGnmSetVgtControl(buffer.data(), 3, 0x7f, 2, 0) == -1);
    Require(sceGnmSetVgtControl(buffer.data(), 3, 0x7f, 0, 2) == -1 && Untouched(buffer));
    Require(sceGnmSetVgtControl(buffer.data(), 3, 0x7f, 0, 0) == 0 && buffer == Words({SetUconfig2, 0x40000258u, 0x6d007fu}));
    Require(sceGnmSetVgtControl(buffer.data(), 3, 0x3f, 1, 0) == 0 && buffer[2] == 0x6d003fu);
    Require(sceGnmSetVgtControl(buffer.data(), 3, 0x7f, 1, 1) == 0 && buffer[2] == 0x75007fu);
}

void CheckShaders() {
    const std::uint32_t cs[]{0x1000, 0, 0x2fff, 0x92, 64, 1, 1};
    auto buffer = Buffer(25);
    Require(sceGnmSetCsShaderWithModifier(buffer.data(), 0x18, cs, 0) == -1);
    Require(sceGnmSetCsShaderWithModifier(buffer.data(), 25, cs, 1) == -1 && Untouched(buffer));
    Require(sceGnmSetCsShaderWithModifier(buffer.data(), 25, cs, 0x140) == 0);
    Require(buffer == Append(Words({SetSh3, 0x20c, 0x1000, 0, SetSh3, 0x212, 0x2d7f, 0x92, SetSh4, 0x207, 64, 1, 1, Nop11}), 11));
    Require(sceGnmSetCsShaderWithModifier(buffer.data(), 25, cs, 0) == 0 && buffer[6] == 0x2fff);

    buffer = Buffer(40);
    Require(sceGnmSetPsShader350(buffer.data(), 0x27, nullptr) == -1 && Untouched(buffer));
    Require(sceGnmSetPsShader350(buffer.data(), 40, nullptr) == 0);
    Require(buffer == Append(Words({SetSh3, 8, 0, 0, SetContext2, 0x203, 0, SetContext2, 0x8f, 0xf, 0xc01c1000u}), 29));

    const std::uint32_t ps[]{0x3000, 0, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a};
    buffer = Buffer(40);
    Require(sceGnmSetPsShader350(buffer.data(), 40, ps) == 0);
    Require(buffer == Append(Words({SetSh3, 8, 0x3000, 0, SetSh3, 10, 0x11, 0x12, SetContext3, 0x1c4, 0x13, 0x14, SetContext3, 0x1b3, 0x15, 0x16,
        SetContext2, 0x1b6, 0x17, SetContext2, 0x1b8, 0x18, SetContext2, 0x203, 0x19, SetContext2, 0x8f, 0x1a, Nop11}), 11));

    buffer = Buffer(40);
    Require(sceGnmUpdatePsShader(buffer.data(), 40, ps) == 0);
    Require(buffer == Append(Words({SetSh3, 8, 0x3000, 0, SetSh3, 10, 0x11, 0x12, Nop3, 0xc01e01c4u, 0x13, 0x14, Nop3, 0xc01e01b3u, 0x15, 0x16,
        Nop2, 0xc01e01b6u, 0x17, Nop2, 0xc01e01b8u, 0x18, Nop2, 0xc01e0203u, 0x19, Nop2, 0xc01e008fu, 0x1a, Nop11}), 11));
    Require(sceGnmUpdatePsShader(buffer.data(), 40, nullptr) == 0);
    Require(buffer == Append(Words({SetSh3, 8, 0, 0, Nop2, 0xc01e0203u, 0, 0xc01f1000u}), 32));
    Require(sceGnmUpdatePsShader350(buffer.data(), 40, nullptr) == 0);
    Require(buffer == Append(Words({SetSh3, 8, 0, 0, Nop2, 0xc01e0203u, 0, SetContext2, 0x8f, 0xf, 0xc01c1000u}), 29));
    const std::uint32_t badPs[]{0x3000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    buffer = Buffer(40);
    Require(sceGnmUpdatePsShader350(buffer.data(), 40, badPs) == -1 && sceGnmUpdatePsShader(buffer.data(), 0x27, ps) == -1 && Untouched(buffer));

    const std::uint32_t gs[]{0x2000, 0, 0x11, 0x22, 0x33, 0x44, 0x55};
    buffer = Buffer(29);
    Require(sceGnmUpdateGsShader(buffer.data(), 0x1c, gs) == -1 && sceGnmUpdateGsShader(buffer.data(), 29, nullptr) == -1 && Untouched(buffer));
    Require(sceGnmUpdateGsShader(buffer.data(), 29, gs) == 0);
    Require(buffer == Append(Words({SetSh3, 0x88, 0x2000, 0, SetSh3, 0x8a, 0x11, 0x22, Nop2, 0xc01e02e5u, 0x33, Nop2, 0xc01e029bu, 0x44, Nop2, 0xc01e02e4u, 0x55, Nop11}), 11));

    const std::uint32_t hs[]{0x4000, 0, 0x11, 0x22, 0x33, 0x44, 0x55};
    buffer = Buffer(30);
    Require(sceGnmUpdateHsShader(buffer.data(), 0x1c, hs, 9) == -1 && Untouched(buffer));
    Require(sceGnmUpdateHsShader(buffer.data(), 30, hs, 9) == 0);
    Require(buffer == Append(Words({SetSh3, 0x108, 0x4000, 0, SetSh3, 0x10a, 0x11, 0x22, Nop3, 0xc01e0286u, 0x44, 0x55, Nop2, 0xc01e02dbu, 0x33, Nop2, 0xc01e02d6u, 9, Nop11}), 11));

    const std::uint32_t vs[]{0x5000, 0, 0x0300ffffu, 0x22, 0x33, 0x44, 0x55};
    buffer = Buffer(29);
    Require(sceGnmUpdateVsShader(buffer.data(), 29, vs, 1) == -1 && sceGnmUpdateVsShader(buffer.data(), 0x1c, vs, 0) == -1 && Untouched(buffer));
    Require(sceGnmUpdateVsShader(buffer.data(), 29, vs, 0x01000140u) == 0);
    Require(buffer == Append(Words({SetSh3, 0x48, 0x5000, 0, SetSh3, 0x4a, 0x0100fd7fu, 0x22, Nop2, 0xc01e0207u, 0x55, Nop2, 0xc01e01b1u, 0x33, Nop2, 0xc01e01c3u, 0x44, Nop11}), 11));
}

std::uint64_t ProgramAddress(const std::vector<std::uint32_t>& buffer) {
    return (static_cast<std::uint64_t>(buffer[3]) << 40) | (static_cast<std::uint64_t>(buffer[2]) << 8);
}

void CheckEmbeddedShaders() {
    auto buffer = Buffer(40);
    Require(sceGnmSetEmbeddedPsShader(buffer.data(), 40, 2, 0) == Failure && Untouched(buffer));
    for (std::uint32_t id = 0; id < 2; ++id) {
        Require(sceGnmSetEmbeddedPsShader(buffer.data(), 40, id, 0) == 0);
        const auto address = ProgramAddress(buffer);
        Require(buffer[0] == SetSh3 && buffer[1] == 8 && address != 0 && (address & 0xff) == 0);
        const auto* code = reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(address));
        Require(code[0] == 0xbeeb03ffu && code[1] == 3 && code[id == 0 ? 7 : 5] == 0xbf810000u);
        const auto expected = id == 0 ? Words({SetSh3, 10, 0xc0000u, 4, SetContext3, 0x1c4, 0, 4, SetContext3, 0x1b3, 2, 2, SetContext2, 0x1b6, 0, SetContext2, 0x1b8, 0,
                                                SetContext2, 0x203, 0x10, SetContext2, 0x8f, 0xf, Nop11})
                                      : Words({SetSh3, 10, 0x2000u, 0, SetContext3, 0x1c4, 0, 2, SetContext3, 0x1b3, 2, 2, SetContext2, 0x1b6, 0, SetContext2, 0x1b8, 0,
                                                SetContext2, 0x203, 0x10, SetContext2, 0x8f, 3, Nop11});
        Require(Matches(buffer.data() + 4, expected));
        for (std::size_t at = 4 + expected.size(); at < 40; ++at) Require(buffer[at] == 0);
    }

    buffer = Buffer(29);
    Require(sceGnmSetEmbeddedVsShader(buffer.data(), 29, 1, 0) == Failure && Untouched(buffer));
    Require(sceGnmSetEmbeddedVsShader(buffer.data(), 29, 0, 0) == 0);
    const auto address = ProgramAddress(buffer);
    Require(buffer[0] == SetSh3 && buffer[1] == 0x48 && address != 0 && (address & 0xff) == 0);
    const auto* code = reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(address));
    Require(code[0] == 0xbeeb03ffu && code[1] == 7 && code[15] == 0xbf810000u);
    Require(Matches(buffer.data() + 4, Append(Words({SetSh3, 0x4a, 0xc0000u, 4, SetContext2, 0x207, 0, SetContext2, 0x1b1, 0, SetContext2, 0x1c3, 4, Nop11}), 11)));
}

void CheckWorkloads() {
    std::uint64_t workload = 7;
    Require(sceGnmBeginWorkload(3, nullptr) == 3);
    Require(sceGnmBeginWorkload(3, &workload) == 0 && workload == 1);
    Require(sceGnmBeginWorkload(0x10, &workload) == 1 && workload == 0);
    Require(sceGnmEndWorkload(0) == 2 && sceGnmEndWorkload(1) == 0 && sceGnmEndWorkload(0x0f00000000000000ull) == 0 && sceGnmEndWorkload(0x1000000000000000ull) == 2);
    std::uint32_t stream = 9;
    Require(sceGnmCreateWorkloadStream(0, &stream) == 3 && stream == 9 && sceGnmCreateWorkloadStream(0x1234, nullptr) == 3);
    Require(sceGnmCreateWorkloadStream(0x1234, &stream) == 0 && stream == 1);

    Require(sceGnmSubmitCommandBuffersForWorkload(1, 1, nullptr, nullptr, nullptr, nullptr) == SubmissionInvalidArgument);
    Require(sceGnmSubmitAndFlipCommandBuffersForWorkload(1, 0, nullptr, nullptr, nullptr, nullptr, 1, 0, 0, 0) == SubmissionInvalidArgument);
    auto flip = Buffer(GnmDriver::FlipReserveWords - 1);
    Require(sceGnmRequestFlipAndSubmitDoneForWorkload(1, flip.data(), static_cast<std::uint32_t>(flip.size() * 4), 1, 0, 0, 0) == SubmissionNotEnoughResources && Untouched(flip));
}

void CheckToolAnswers() {
    for (auto* retrieve : {sceGnmDriverInternalRetrieveGnmInterface, sceGnmDriverInternalRetrieveGnmInterfaceForGpuDebugger, sceGnmDriverInternalRetrieveGnmInterfaceForReplay,
             sceGnmDriverInternalRetrieveGnmInterfaceForResourceRegistration, sceGnmDriverInternalRetrieveGnmInterfaceForValidation})
        Require(retrieve() == 0x80000000u);
    for (auto* failing : {sceGnmDebugModuleReset, sceGnmDebugReset, sceGnmFindResources, sceGnmFindResourcesPublic, sceGnmGetOwnerName, sceGnmGetPhysicalCounterFromVirtualized,
             sceGnmGetResourceBaseAddressAndSizeInBytes, sceGnmGetResourceName, sceGnmGetResourceRegistrationBuffers, sceGnmGetResourceShaderGuid, sceGnmGetResourceType,
             sceGnmGetResourceUserData, sceGnmQueryResourceRegistrationUserMemoryRequirements, sceGnmRegisterGdsResource, sceGnmRegisterOwnerForSystem, sceGnmSdmaClose,
             sceGnmSdmaConstFill, sceGnmSdmaCopyLinear, sceGnmSdmaCopyTiled, sceGnmSdmaCopyWindow, sceGnmSdmaFlush, sceGnmSdmaGetMinCmdSize, sceGnmSdmaOpen,
             sceGnmSetResourceRegistrationUserMemory, sceGnmSetResourceUserData, sceGnmSetSpiEnableSqCounters, sceGnmSetSpiEnableSqCountersForUnitInstance,
             sceGnmUnregisterAllResourcesForOwner, sceGnmUnregisterOwnerAndResources, sceGnmUnregisterResource})
        Require(failing() == Failure);
    int owner = 0;
    Require(sceGnmRegisterOwner(&owner, "owner") == Failure && sceGnmRegisterResource(&owner, &owner, &owner, sizeof(owner), "resource", 1, 2) == Failure);
    for (auto* validate : {sceGnmValidateDisableDiagnostics, sceGnmValidateDisableDiagnostics2, sceGnmValidateDispatchCommandBuffers, sceGnmValidateDrawCommandBuffers,
             sceGnmValidateGetDiagnosticInfo, sceGnmValidateGetDiagnostics, sceGnmValidateResetState, sceGnmValidationRegisterMemoryCheckCallback})
        Require(validate() == ValidationNotEnabled);
    for (auto* zero : {sceGnmDriverInternalVirtualQuery, sceGnmGetGpuBlockStatus, sceGnmGetGpuInfoStatus, sceGnmGetLastWaitedAddress, sceGnmGetShaderProgramBaseAddress,
             sceGnmGetShaderStatus, sceGnmInsertThreadTraceMarker, sceGnmPaDisableFlipCallbacks, sceGnmPaEnableFlipCallbacks, sceGnmPaHeartbeat, sceGnmRaiseUserExceptionEvent,
             sceGnmValidateGetVersion})
        Require(zero() == 0);
    Require(!sceGnmDriverTraceInProgress() && !sceGnmValidateOnSubmitEnabled());
    Require(sceGnmGetDbgGcHandle() == -1 && sceGnmGetProtectionFaultTimeStamp() == 0);
    Require(sceGnmGetGpuCoreClockFrequency() == 911000000u && sceGnmGetOffChipTessellationBufferSize() == 0x800000);
    sceGnmGpuPaDebugEnter();
    sceGnmGpuPaDebugLeave();
    sceGnmRegisterGnmLiveCallbackConfig();
}

#ifdef _WIN32
void CheckInternalContextState() {
    alignas(8) static std::uint8_t parameters[0x40]{};
    const std::uint64_t size = sizeof(parameters);
    const std::uint32_t magic = 0x4942524f;
    std::memcpy(parameters, &size, sizeof(size));
    std::memcpy(parameters + 8, &magic, sizeof(magic));
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    descriptor[0] = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(parameters) - base);
    descriptor[1] = sizeof(parameters);
    for (const std::uint64_t sdk : {std::uint64_t{0x03508001}, std::uint64_t{0x04008001}}) {
        std::memcpy(parameters + 0x10, &sdk, sizeof(sdk));
        const bool sdk400 = sdk >= 0x4000000;
        const std::uint32_t words = sdk400 ? 0x100 : 0x20;
        Require(sceGnmDrawInitToDefaultContextStateInternalSize() == static_cast<int>(words));
        auto buffer = Buffer(words + 1);
        Require(sceGnmDrawInitToDefaultContextStateInternalCommand(buffer.data(), sdk400 ? 0x20 : 0x100) == 0 && Untouched(buffer));
        Require(sceGnmDrawInitToDefaultContextStateInternalCommand(buffer.data(), words) == static_cast<int>(words));
        Require(sdk400 ? Matches(buffer.data(), NeoContext400Sequence) : Matches(buffer.data(), NeoContextSequence));
        Require(buffer[words] == Fill);
    }
    descriptor[0] = 0xffffffffu;
}
#endif

}

int main() {
    CheckHardwareState(sceGnmDrawInitDefaultHardwareState, Sdk100Sequence);
    CheckHardwareState(sceGnmDrawInitDefaultHardwareState200, Neo200Sequence);
    CheckHardwareState(sceGnmDrawInitDefaultHardwareState350, Neo350Sequence);
    CheckContextState(sceGnmDrawInitToDefaultContextState, 0x20, NeoContextSequence);
    CheckContextState(sceGnmDrawInitToDefaultContextState400, 0x100, NeoContext400Sequence);
    CheckDraws();
    CheckCompute();
    CheckControl();
    CheckShaders();
    CheckEmbeddedShaders();
    CheckWorkloads();
    CheckToolAnswers();
#ifdef _WIN32
    CheckInternalContextState();
#endif
}
