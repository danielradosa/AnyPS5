#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace {

struct CalibrationSettings {
    std::uint32_t hmdPosition;
    std::uint32_t padPosition;
    std::uint32_t movePosition;
    std::uint32_t gunPosition;
    std::uint32_t reserved[4];
};

struct InitParam {
    std::uint32_t size;
    std::uint32_t profile;
    std::uint32_t executionMode;
    std::int32_t hmdThreadPriority;
    std::int32_t padThreadPriority;
    std::int32_t moveThreadPriority;
    std::int32_t gunThreadPriority;
    std::int32_t reserved;
    std::uint64_t cpuMask;
    CalibrationSettings calibrationSettings;
    void* directMemoryOnion;
    std::uint32_t directMemoryOnionSize;
    std::uint32_t directMemoryOnionAlignment;
    void* directMemoryGarlic;
    std::uint32_t directMemoryGarlicSize;
    std::uint32_t directMemoryGarlicAlignment;
    void* workMemory;
    std::uint32_t workMemorySize;
    std::uint32_t workMemoryAlignment;
    std::int32_t gpuPipeId;
    std::int32_t gpuQueueId;
};

struct GpuWaitParam {
    std::uint32_t size;
    std::uint32_t reserved[7];
};

static_assert(sizeof(InitParam) == 0x80 && offsetof(InitParam, gpuPipeId) == 0x78);

constexpr std::int32_t NotInit = static_cast<std::int32_t>(0x81260801);
constexpr std::int32_t AlreadyInitialized = static_cast<std::int32_t>(0x81260802);
constexpr std::int32_t ArgumentInvalid = static_cast<std::int32_t>(0x81260806);
constexpr std::int32_t NotExecuteGpuSubmit = static_cast<std::int32_t>(0x81260815);

void Require(bool value) { if (!value) std::abort(); }

InitParam ValidParam() {
    InitParam param{};
    param.size = sizeof(InitParam);
    param.directMemoryOnion = reinterpret_cast<void*>(std::uintptr_t{0x10000000});
    param.directMemoryOnionSize = 0x400000;
    param.directMemoryOnionAlignment = 0x10000;
    param.directMemoryGarlic = reinterpret_cast<void*>(std::uintptr_t{0x20000000});
    param.directMemoryGarlicSize = 0x3000000;
    param.directMemoryGarlicAlignment = 0x10000;
    param.workMemory = reinterpret_cast<void*>(std::uintptr_t{0x30000000});
    param.workMemorySize = 0x1000000;
    param.workMemoryAlignment = 0x10000;
    param.gpuPipeId = 6;
    param.gpuQueueId = 7;
    return param;
}

}

extern "C" {
std::int32_t APS5_VABI sceVrTrackerInit(const InitParam* param);
std::int32_t APS5_VABI sceVrTrackerGetTime(std::uint64_t* time);
std::int32_t APS5_VABI sceVrTrackerGpuSubmit(const void* param);
std::int32_t APS5_VABI sceVrTrackerGpuWait(const GpuWaitParam* param);
}

int main() {
    std::uint64_t time = 0;
    const GpuWaitParam wait{sizeof(GpuWaitParam), {}};
    const std::uint8_t submit[0x100] = {};
    Require(sceVrTrackerGetTime(&time) == NotInit);
    Require(sceVrTrackerGpuSubmit(submit) == NotInit);
    Require(sceVrTrackerGpuWait(&wait) == NotInit);

    Require(sceVrTrackerInit(nullptr) == ArgumentInvalid);
    InitParam param = ValidParam();
    param.size = sizeof(InitParam) - 8;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    param = ValidParam();
    param.calibrationSettings.hmdPosition = 1;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    param = ValidParam();
    param.calibrationSettings.gunPosition = 2;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    param = ValidParam();
    param.calibrationSettings.movePosition = 1;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    param.directMemoryOnionSize = 0x800000;
    param.directMemoryGarlic = nullptr;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    param = ValidParam();
    param.workMemoryAlignment = 0x1000;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    param = ValidParam();
    param.directMemoryGarlicSize = 0x1000000;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    param = ValidParam();
    param.gpuPipeId = 7;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    param = ValidParam();
    param.gpuQueueId = 8;
    Require(sceVrTrackerInit(&param) == ArgumentInvalid);
    Require(sceVrTrackerGetTime(&time) == NotInit);

    param = ValidParam();
    param.calibrationSettings.movePosition = 1;
    param.directMemoryOnionSize = 0x800000;
    Require(sceVrTrackerInit(&param) == 0);
    Require(sceVrTrackerInit(&param) == AlreadyInitialized);
    Require(sceVrTrackerGetTime(nullptr) == ArgumentInvalid);
    Require(sceVrTrackerGetTime(&time) == 0);
    std::uint64_t later = 0;
    Require(sceVrTrackerGetTime(&later) == 0 && later >= time);
    Require(sceVrTrackerGpuSubmit(submit) == ArgumentInvalid);
    Require(sceVrTrackerGpuWait(nullptr) == ArgumentInvalid);
    const GpuWaitParam wrong{sizeof(GpuWaitParam) + 4, {}};
    Require(sceVrTrackerGpuWait(&wrong) == ArgumentInvalid);
    Require(sceVrTrackerGpuWait(&wait) == NotExecuteGpuSubmit);
}
