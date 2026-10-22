#include <cstdint>
#include <cstddef>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/Time/include/Time.hpp"

namespace {

constexpr std::int32_t VR_TRACKER_ERROR_NOT_INIT = static_cast<std::int32_t>(0x81260801);
constexpr std::int32_t VR_TRACKER_ERROR_ALREADY_INITIALIZED = static_cast<std::int32_t>(0x81260802);
constexpr std::int32_t VR_TRACKER_ERROR_DEVICE_NOT_REGISTERED = static_cast<std::int32_t>(0x81260803);
constexpr std::int32_t VR_TRACKER_ERROR_DEVICE_ALREADY_REGISTERED = static_cast<std::int32_t>(0x81260804);
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
constexpr std::uint32_t ProfileDefault = 0;
constexpr std::uint32_t Profile100 = 100;
constexpr std::uint32_t DeviceHmd = 0;
constexpr std::uint32_t DeviceGun = 3;
constexpr std::uint32_t DeviceTypes = DeviceGun + 1;
constexpr std::int32_t DefaultRegisterMode = -1;
constexpr std::int32_t MaxRegisterMode = 4;
constexpr std::int32_t NoHandle = -1;

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

struct QueryMemoryParam {
    std::uint32_t size;
    std::uint32_t profile;
    std::uint32_t reserved[6];
    CalibrationSettings calibrationSettings;
};

struct QueryMemoryResult {
    std::uint32_t size;
    std::uint32_t directMemoryOnionSize;
    std::uint32_t directMemoryOnionAlignment;
    std::uint32_t directMemoryGarlicSize;
    std::uint32_t directMemoryGarlicAlignment;
    std::uint32_t workMemorySize;
    std::uint32_t workMemoryAlignment;
    std::uint32_t reserved[9];
};

struct RecalibrateParam {
    std::uint32_t size;
    std::uint32_t deviceType;
    std::uint32_t calibrationType;
    std::uint32_t reserved[5];
};

static_assert(sizeof(CalibrationSettings) == 0x20);
static_assert(sizeof(InitParam) == 0x80 && offsetof(InitParam, calibrationSettings) == 0x28 && offsetof(InitParam, directMemoryOnion) == 0x48 && offsetof(InitParam, gpuPipeId) == 0x78);
static_assert(sizeof(QueryMemoryParam) == 0x40 && offsetof(QueryMemoryParam, calibrationSettings) == 0x20);
static_assert(sizeof(QueryMemoryResult) == 0x40);
static_assert(sizeof(RecalibrateParam) == 0x20);

std::mutex g_mutex;
bool g_initialized = false;
std::int32_t g_registered[DeviceTypes] = {NoHandle, NoHandle, NoHandle, NoHandle};

bool Initialized() {
    std::lock_guard lock(g_mutex);
    return g_initialized;
}

bool ValidCalibration(const CalibrationSettings& settings) {
    return settings.hmdPosition <= CalibrationManual && settings.padPosition <= CalibrationAuto && settings.movePosition <= CalibrationAuto && settings.gunPosition <= CalibrationAuto;
}

std::uint32_t OnionSize(const CalibrationSettings& settings) {
    return settings.movePosition == CalibrationAuto ? BaseOnionSize * 2 : BaseOnionSize;
}

std::int32_t RegisterDevice(std::uint32_t deviceType, std::int32_t handle, std::int32_t mode) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return VR_TRACKER_ERROR_NOT_INIT;
    if (deviceType > DeviceGun || mode > MaxRegisterMode) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    if (g_registered[deviceType] != NoHandle) return VR_TRACKER_ERROR_DEVICE_ALREADY_REGISTERED;
    g_registered[deviceType] = handle;
    return 0;
}

bool ValidInit(const InitParam* param) {
    if (param == nullptr || param->size != sizeof(InitParam) || !ValidCalibration(param->calibrationSettings)) return false;
    const std::uint32_t onionSize = OnionSize(param->calibrationSettings);
    if (param->directMemoryGarlic == nullptr || param->directMemoryGarlicAlignment != MemoryAlignment || param->directMemoryGarlicSize != GarlicSize) return false;
    if (param->directMemoryOnion == nullptr || param->directMemoryOnionAlignment != MemoryAlignment || param->directMemoryOnionSize != onionSize) return false;
    if (param->workMemory == nullptr || param->workMemoryAlignment != MemoryAlignment || param->workMemorySize != WorkSize) return false;
    return param->gpuPipeId >= 0 && param->gpuPipeId < ComputePipes && param->gpuQueueId >= 0 && param->gpuQueueId < QueuesPerPipe;
}

}

struct VrTrackerMemoryBlock {
    void* address;
    std::uint32_t size;
    std::uint32_t alignment;
};

struct VrTrackerInitConfig {
    std::uint32_t type;
    std::uint32_t option;
    std::uint32_t mode;
    std::uint32_t parameter;
    std::uint32_t reserved[4];
};

struct VrTrackerInitParam {
    std::uint32_t size;
    std::int32_t mode;
    std::uint32_t reserved[6];
    VrTrackerInitConfig config;
    VrTrackerMemoryBlock memory[3];
    std::uint32_t option112;
    std::uint32_t option116;
    std::uint32_t reservedTail[6];
};

static_assert(sizeof(VrTrackerMemoryBlock) == 16);
static_assert(sizeof(VrTrackerInitConfig) == 32);
static_assert(sizeof(VrTrackerInitParam) == 144);
static_assert(offsetof(VrTrackerInitParam, config) == 32);
static_assert(offsetof(VrTrackerInitParam, memory) == 64);
static_assert(offsetof(VrTrackerInitParam, option112) == 112);
struct VrTrackerGpuSubmitParam;
struct VrTrackerResult;

struct VrTrackerCpuProcessParam {
    std::uint32_t size;
    std::uint32_t mode;
    std::uint32_t argument;
    std::uint32_t reserved[5];
};

struct VrTrackerGpuWaitParam {
    std::uint32_t size;
    std::byte payload[28];
};

struct VrTrackerGetResultParam {
    std::uint32_t size;
    std::int32_t handle;
    std::uint32_t mode;
    std::uint32_t extendedResult;
    std::uint64_t time;
    std::uint32_t statusMode;
    std::uint32_t reserved0;
    std::uint32_t option32;
    std::byte outputValue[4];
    std::uint32_t option40;
    std::uint32_t reserved1[2];
    std::byte unused[4];
};

static_assert(sizeof(VrTrackerCpuProcessParam) == 32);
static_assert(offsetof(VrTrackerCpuProcessParam, mode) == 4);
static_assert(offsetof(VrTrackerCpuProcessParam, argument) == 8);
static_assert(sizeof(VrTrackerGpuWaitParam) == 32);
static_assert(sizeof(VrTrackerGetResultParam) == 56);
static_assert(offsetof(VrTrackerGetResultParam, time) == 16);
static_assert(offsetof(VrTrackerGetResultParam, statusMode) == 24);
static_assert(offsetof(VrTrackerGetResultParam, outputValue) == 36);
static_assert(offsetof(VrTrackerGetResultParam, reserved1) == 44);

extern "C" {

int APS5_VABI sceVrTrackerCpuProcess(const VrTrackerCpuProcessParam* param) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerGetResult(const VrTrackerGetResultParam* param, VrTrackerResult* result) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVrTrackerGetTime(std::uint64_t* time) {
    if (!Initialized()) return VR_TRACKER_ERROR_NOT_INIT;
    if (time == nullptr) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    *time = sceKernelGetProcessTime();
    return 0;
}

int APS5_VABI sceVrTrackerGpuSubmit(const VrTrackerGpuSubmitParam* param) {
    (void)param;
    if (!Initialized()) return VR_TRACKER_ERROR_NOT_INIT;
    return VR_TRACKER_ERROR_ARGUMENT_INVALID;
}

int APS5_VABI sceVrTrackerGpuWait(const VrTrackerGpuWaitParam* param) {
    if (!Initialized()) return VR_TRACKER_ERROR_NOT_INIT;
    if (param == nullptr || param->size != sizeof(VrTrackerGpuWaitParam)) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    return VR_TRACKER_ERROR_NOT_EXECUTE_GPU_SUBMIT;
}

std::int32_t APS5_VABI sceVrTrackerInit(const InitParam* param) {
    std::lock_guard lock(g_mutex);
    if (g_initialized) return VR_TRACKER_ERROR_ALREADY_INITIALIZED;
    if (!ValidInit(param)) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    g_initialized = true;
    return 0;
}
int APS5_VABI sceVrTrackerCpuPopMarker() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerCpuPushMarker() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerGetLiveCaptureId() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerGetPlayAreaWarningInfo(void* info) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
std::int32_t APS5_VABI sceVrTrackerGpuWaitAndCpuProcess() {
    if (!Initialized()) return VR_TRACKER_ERROR_NOT_INIT;
    return VR_TRACKER_ERROR_NOT_EXECUTE_GPU_SUBMIT;
}
int APS5_VABI sceVrTrackerNotifyEndOfCpuProcess(const void* param) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
std::int32_t APS5_VABI sceVrTrackerQueryMemory(const QueryMemoryParam* param, QueryMemoryResult* result) {
    if (param == nullptr || result == nullptr || param->size != sizeof(QueryMemoryParam) || (param->profile != ProfileDefault && param->profile != Profile100) || !ValidCalibration(param->calibrationSettings)) {
        return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    }
    result->directMemoryOnionSize = OnionSize(param->calibrationSettings);
    result->directMemoryOnionAlignment = MemoryAlignment;
    result->directMemoryGarlicSize = GarlicSize;
    result->directMemoryGarlicAlignment = MemoryAlignment;
    result->workMemorySize = WorkSize;
    result->workMemoryAlignment = MemoryAlignment;
    return 0;
}
std::int32_t APS5_VABI sceVrTrackerRecalibrate(const RecalibrateParam* param) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return VR_TRACKER_ERROR_NOT_INIT;
    if (param == nullptr || param->size != sizeof(RecalibrateParam) || param->deviceType > DeviceGun) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    if (param->deviceType == DeviceHmd || g_registered[param->deviceType] == NoHandle) return VR_TRACKER_ERROR_DEVICE_NOT_REGISTERED;
    return 0;
}
std::int32_t APS5_VABI sceVrTrackerRegisterDevice(std::uint32_t deviceType, std::int32_t handle) {
    return RegisterDevice(deviceType, handle, DefaultRegisterMode);
}
std::int32_t APS5_VABI sceVrTrackerRegisterDevice2(std::uint32_t deviceType, std::int32_t handle) {
    return RegisterDevice(deviceType, handle, DefaultRegisterMode);
}
std::int32_t APS5_VABI sceVrTrackerRegisterDeviceInternal(std::uint32_t deviceType, std::int32_t handle, std::int32_t mode, std::int32_t variant) {
    (void)variant;
    return RegisterDevice(deviceType, handle, mode);
}
int APS5_VABI sceVrTrackerResetAll() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerResetOrientationRelative(int device_type, int handle) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerSaveInternalBuffers() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerSetDeviceRejection() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
std::int32_t APS5_VABI sceVrTrackerSetDurationUntilStatusNotTracking(std::uint32_t deviceType, std::uint32_t durationCameraFrames) {
    (void)durationCameraFrames;
    if (!Initialized()) return VR_TRACKER_ERROR_NOT_INIT;
    if (deviceType > DeviceGun) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    return 0;
}
int APS5_VABI sceVrTrackerSetExtendedMode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerSetLEDBrightness() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerSetRestingMode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerStartLiveCapture() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
int APS5_VABI sceVrTrackerStopLiveCapture() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
std::int32_t APS5_VABI sceVrTrackerTerm() {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return VR_TRACKER_ERROR_NOT_INIT;
    g_initialized = false;
    for (std::int32_t& registered : g_registered) registered = NoHandle;
    return 0;
}
std::int32_t APS5_VABI sceVrTrackerUnregisterDevice(std::int32_t handle) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return VR_TRACKER_ERROR_NOT_INIT;
    if (handle < 0) return VR_TRACKER_ERROR_ARGUMENT_INVALID;
    for (std::int32_t& registered : g_registered) {
        if (registered == handle) {
            registered = NoHandle;
            return 0;
        }
    }
    return VR_TRACKER_ERROR_ARGUMENT_INVALID;
}
int APS5_VABI sceVrTrackerUpdateMotionSensorData(const void* param) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
