#include <cstdint>
#include <cstddef>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/Time/include/Time.hpp"

namespace {

constexpr std::int32_t VR_TRACKER_ERROR_NOT_INIT = static_cast<std::int32_t>(0x81260801);
constexpr std::int32_t VR_TRACKER_ERROR_ALREADY_INITIALIZED = static_cast<std::int32_t>(0x81260802);
constexpr std::int32_t VR_TRACKER_ERROR_ARGUMENT_INVALID = static_cast<std::int32_t>(0x81260806);
constexpr std::int32_t VR_TRACKER_ERROR_NOT_EXECUTE_GPU_SUBMIT = static_cast<std::int32_t>(0x81260815);

constexpr std::uint32_t MemoryAlignment = 0x10000;
constexpr std::uint32_t BaseOnionSize = 0x400000;
constexpr std::uint32_t GarlicSize = 0x3000000;
constexpr std::uint32_t WorkSize = 0x1000000;
constexpr std::uint32_t CalibrationManual = 0;
constexpr std::uint32_t CalibrationAuto = 1;
constexpr std::int32_t ComputePipes = 7;
constexpr std::int32_t QueuesPerPipe = 8;

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

static_assert(sizeof(CalibrationSettings) == 0x20);
static_assert(sizeof(InitParam) == 0x80 && offsetof(InitParam, calibrationSettings) == 0x28 && offsetof(InitParam, directMemoryOnion) == 0x48 && offsetof(InitParam, gpuPipeId) == 0x78);
static_assert(sizeof(GpuWaitParam) == 0x20);

std::mutex g_mutex;
bool g_initialized = false;

bool Initialized() {
    std::lock_guard lock(g_mutex);
    return g_initialized;
}

bool ValidCalibration(const CalibrationSettings& settings) {
    return settings.hmdPosition <= CalibrationManual && settings.padPosition <= CalibrationAuto && settings.movePosition <= CalibrationAuto && settings.gunPosition <= CalibrationAuto;
}

bool ValidInit(const InitParam* param) {
    if (param == nullptr || param->size != sizeof(InitParam) || !ValidCalibration(param->calibrationSettings)) return false;
    const std::uint32_t onionSize = param->calibrationSettings.movePosition == CalibrationAuto ? BaseOnionSize * 2 : BaseOnionSize;
    if (param->directMemoryGarlic == nullptr || param->directMemoryGarlicAlignment != MemoryAlignment || param->directMemoryGarlicSize != GarlicSize) return false;
    if (param->directMemoryOnion == nullptr || param->directMemoryOnionAlignment != MemoryAlignment || param->directMemoryOnionSize != onionSize) return false;
    if (param->workMemory == nullptr || param->workMemoryAlignment != MemoryAlignment || param->workMemorySize != WorkSize) return false;
    return param->gpuPipeId >= 0 && param->gpuPipeId < ComputePipes && param->gpuQueueId >= 0 && param->gpuQueueId < QueuesPerPipe;
}

}

extern "C" {

int APS5_VABI sceVrTrackerCpuProcess() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerGetResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceVrTrackerGetTime(std::uint64_t* time) {
    if (!Initialized()) return VR_TRACKER_ERROR_NOT_INIT;
    if (time == nullptr) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    *time = sceKernelGetProcessTime();
    return 0;
}

std::int32_t APS5_VABI sceVrTrackerGpuSubmit(const void* param) {
    (void)param;
    if (!Initialized()) return VR_TRACKER_ERROR_NOT_INIT;
    return VR_TRACKER_ERROR_ARGUMENT_INVALID;
}

std::int32_t APS5_VABI sceVrTrackerGpuWait(const GpuWaitParam* param) {
    if (!Initialized()) return VR_TRACKER_ERROR_NOT_INIT;
    if (param == nullptr || param->size != sizeof(GpuWaitParam)) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    return VR_TRACKER_ERROR_NOT_EXECUTE_GPU_SUBMIT;
}

std::int32_t APS5_VABI sceVrTrackerInit(const InitParam* param) {
    std::lock_guard lock(g_mutex);
    if (g_initialized) return VR_TRACKER_ERROR_ALREADY_INITIALIZED;
    if (!ValidInit(param)) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    g_initialized = true;
    return 0;
}

}
