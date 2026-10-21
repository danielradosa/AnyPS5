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

static_assert(sizeof(InitParam) == 0x80 && offsetof(InitParam, gpuPipeId) == 0x78);
static_assert(sizeof(QueryMemoryParam) == 0x40 && sizeof(QueryMemoryResult) == 0x40 && sizeof(RecalibrateParam) == 0x20);

constexpr std::int32_t NotInit = static_cast<std::int32_t>(0x81260801);
constexpr std::int32_t AlreadyInitialized = static_cast<std::int32_t>(0x81260802);
constexpr std::int32_t DeviceNotRegistered = static_cast<std::int32_t>(0x81260803);
constexpr std::int32_t DeviceAlreadyRegistered = static_cast<std::int32_t>(0x81260804);
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
std::int32_t APS5_VABI sceVrTrackerGpuWaitAndCpuProcess();
std::int32_t APS5_VABI sceVrTrackerQueryMemory(const QueryMemoryParam* param, QueryMemoryResult* result);
std::int32_t APS5_VABI sceVrTrackerRecalibrate(const RecalibrateParam* param);
std::int32_t APS5_VABI sceVrTrackerRegisterDevice(std::uint32_t deviceType, std::int32_t handle);
std::int32_t APS5_VABI sceVrTrackerRegisterDevice2(std::uint32_t deviceType, std::int32_t handle);
std::int32_t APS5_VABI sceVrTrackerRegisterDeviceInternal(std::uint32_t deviceType, std::int32_t handle, std::int32_t mode, std::int32_t variant);
std::int32_t APS5_VABI sceVrTrackerSetDurationUntilStatusNotTracking(std::uint32_t deviceType, std::uint32_t durationCameraFrames);
std::int32_t APS5_VABI sceVrTrackerTerm();
std::int32_t APS5_VABI sceVrTrackerUnregisterDevice(std::int32_t handle);
}

int main() {
    std::uint64_t time = 0;
    const GpuWaitParam wait{sizeof(GpuWaitParam), {}};
    const std::uint8_t submit[0x100] = {};
    Require(sceVrTrackerGetTime(&time) == NotInit);
    Require(sceVrTrackerGpuSubmit(submit) == NotInit);
    Require(sceVrTrackerGpuWait(&wait) == NotInit);
    Require(sceVrTrackerGpuWaitAndCpuProcess() == NotInit);
    Require(sceVrTrackerTerm() == NotInit);
    Require(sceVrTrackerRegisterDevice(1, 0x100) == NotInit);
    Require(sceVrTrackerUnregisterDevice(0x100) == NotInit);
    Require(sceVrTrackerSetDurationUntilStatusNotTracking(0, 30) == NotInit);
    const RecalibrateParam recalibrateMove{sizeof(RecalibrateParam), 2, 0, {}};
    Require(sceVrTrackerRecalibrate(&recalibrateMove) == NotInit);

    QueryMemoryParam query{};
    query.size = sizeof(QueryMemoryParam);
    QueryMemoryResult memory{};
    memory.size = 0xAAAAAAAAu;
    Require(sceVrTrackerQueryMemory(nullptr, &memory) == ArgumentInvalid);
    Require(sceVrTrackerQueryMemory(&query, nullptr) == ArgumentInvalid);
    query.profile = 50;
    Require(sceVrTrackerQueryMemory(&query, &memory) == ArgumentInvalid);
    query.profile = 100;
    query.calibrationSettings.hmdPosition = 1;
    Require(sceVrTrackerQueryMemory(&query, &memory) == ArgumentInvalid);
    query.calibrationSettings.hmdPosition = 0;
    query.size = sizeof(QueryMemoryParam) + 4;
    Require(sceVrTrackerQueryMemory(&query, &memory) == ArgumentInvalid);
    query.size = sizeof(QueryMemoryParam);
    Require(sceVrTrackerQueryMemory(&query, &memory) == 0);
    Require(memory.directMemoryOnionSize == 0x400000 && memory.directMemoryOnionAlignment == 0x10000);
    Require(memory.directMemoryGarlicSize == 0x3000000 && memory.directMemoryGarlicAlignment == 0x10000);
    Require(memory.workMemorySize == 0x1000000 && memory.workMemoryAlignment == 0x10000 && memory.size == 0xAAAAAAAAu);
    query.profile = 0;
    query.calibrationSettings.movePosition = 1;
    Require(sceVrTrackerQueryMemory(&query, &memory) == 0 && memory.directMemoryOnionSize == 0x800000);

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
    Require(sceVrTrackerGpuWaitAndCpuProcess() == NotExecuteGpuSubmit);

    Require(sceVrTrackerSetDurationUntilStatusNotTracking(4, 30) == ArgumentInvalid);
    Require(sceVrTrackerSetDurationUntilStatusNotTracking(3, 30) == 0);
    Require(sceVrTrackerRegisterDevice(4, 0x100) == ArgumentInvalid);
    Require(sceVrTrackerRegisterDeviceInternal(1, 0x100, 5, 0) == ArgumentInvalid);
    Require(sceVrTrackerRecalibrate(nullptr) == ArgumentInvalid);
    const RecalibrateParam recalibrateBadType{sizeof(RecalibrateParam), 4, 0, {}};
    Require(sceVrTrackerRecalibrate(&recalibrateBadType) == ArgumentInvalid);
    const RecalibrateParam recalibrateWrongSize{sizeof(RecalibrateParam) - 4, 2, 0, {}};
    Require(sceVrTrackerRecalibrate(&recalibrateWrongSize) == ArgumentInvalid);
    Require(sceVrTrackerRecalibrate(&recalibrateMove) == DeviceNotRegistered);
    Require(sceVrTrackerRegisterDevice(2, 0x30B0100) == 0);
    Require(sceVrTrackerRegisterDevice2(2, 0x30B0200) == DeviceAlreadyRegistered);
    Require(sceVrTrackerRecalibrate(&recalibrateMove) == 0);
    Require(sceVrTrackerRegisterDevice2(1, 0x10100) == 0);
    Require(sceVrTrackerRegisterDeviceInternal(3, 0x20100, 4, 1) == 0);
    Require(sceVrTrackerRegisterDevice(0, 0x40100) == 0);
    const RecalibrateParam recalibrateHmd{sizeof(RecalibrateParam), 0, 0, {}};
    Require(sceVrTrackerRecalibrate(&recalibrateHmd) == DeviceNotRegistered);
    Require(sceVrTrackerUnregisterDevice(-1) == ArgumentInvalid);
    Require(sceVrTrackerUnregisterDevice(0x777) == ArgumentInvalid);
    Require(sceVrTrackerUnregisterDevice(0x30B0100) == 0);
    Require(sceVrTrackerUnregisterDevice(0x30B0100) == ArgumentInvalid);
    Require(sceVrTrackerRecalibrate(&recalibrateMove) == DeviceNotRegistered);
    Require(sceVrTrackerRegisterDevice(2, 0x30B0200) == 0);

    Require(sceVrTrackerTerm() == 0);
    Require(sceVrTrackerTerm() == NotInit);
    Require(sceVrTrackerGetTime(&time) == NotInit);
    Require(sceVrTrackerInit(&param) == 0);
    Require(sceVrTrackerRegisterDevice(1, 0x10100) == 0);
    Require(sceVrTrackerUnregisterDevice(0x30B0200) == ArgumentInvalid);
    Require(sceVrTrackerTerm() == 0);
}
