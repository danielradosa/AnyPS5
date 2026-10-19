#include <atomic>
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t SCE_VR_TRACKER2_ERROR_NOT_INIT = static_cast<std::int32_t>(0x8A740001);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_ALREADY_INITIALIZED = static_cast<std::int32_t>(0x8A740002);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_DEVICE_NOT_REGISTERED = static_cast<std::int32_t>(0x8A740003);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID = static_cast<std::int32_t>(0x8A740006);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_INVALID_DEVICE_HANDLE = static_cast<std::int32_t>(0x8A740007);
constexpr std::int32_t SCE_VR_TRACKER2_ERROR_NOT_SUPPORTED = static_cast<std::int32_t>(0x8A74001E);

struct QueryMemoryParam {
    std::uint32_t size;
    std::uint32_t profile;
    std::uint8_t reserved[24];
};

struct InitializeParam {
    std::uint32_t size;
    std::uint8_t reserved0[28];
    void* workMemory;
    std::uint32_t workMemorySize;
    std::uint32_t workMemoryAlignment;
    std::uint8_t reserved1[24];
};

struct GetResultParam {
    std::uint32_t size;
    std::int32_t handle;
    std::uint8_t reserved0[8];
    std::uint64_t time;
    std::uint8_t reserved1[8];
};

static_assert(sizeof(QueryMemoryParam) == 0x20);
static_assert(sizeof(InitializeParam) == 0x48);
static_assert(offsetof(InitializeParam, workMemory) == 0x20);
static_assert(offsetof(InitializeParam, workMemorySize) == 0x28);
static_assert(sizeof(GetResultParam) == 0x20);
static_assert(offsetof(GetResultParam, time) == 0x10);

std::atomic<bool> initialized{false};

std::int32_t NoDevice() {
    return initialized.load() ? SCE_VR_TRACKER2_ERROR_DEVICE_NOT_REGISTERED : SCE_VR_TRACKER2_ERROR_NOT_INIT;
}

}

extern "C" {

std::int32_t APS5_VABI sceVrTracker2Finalize() {
    return initialized.exchange(false) ? 0 : SCE_VR_TRACKER2_ERROR_NOT_INIT;
}

int APS5_VABI sceVrTracker2GetCoordinateSystem() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceVrTracker2GetPlayAreaBoundaryGeometry() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceVrTracker2ResetLocalCoordinate() {
    return NoDevice();
}

std::int32_t APS5_VABI sceVrTracker2GetPlayAreaOrientedBoundingBox() {
    return NoDevice();
}

std::int32_t APS5_VABI sceVrTracker2GetResult(const GetResultParam* param, void* result) {
    if (!initialized.load()) return SCE_VR_TRACKER2_ERROR_NOT_INIT;
    if (!param || !result || param->size != sizeof(GetResultParam)) return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    return SCE_VR_TRACKER2_ERROR_DEVICE_NOT_REGISTERED;
}

std::int32_t APS5_VABI sceVrTracker2Initialize(const InitializeParam* param) {
    if (initialized.load()) return SCE_VR_TRACKER2_ERROR_ALREADY_INITIALIZED;
    if (!param || param->size != sizeof(InitializeParam) || !param->workMemory || param->workMemorySize == 0)
        return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    return initialized.exchange(true) ? SCE_VR_TRACKER2_ERROR_ALREADY_INITIALIZED : 0;
}

std::int32_t APS5_VABI sceVrTracker2LocateCoordinateSystem() {
    return NoDevice();
}

std::int32_t APS5_VABI sceVrTracker2QueryMemory(const QueryMemoryParam* param, void* result) {
    if (!param || !result || param->size != sizeof(QueryMemoryParam) || param->profile != 0)
        return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
    return SCE_VR_TRACKER2_ERROR_NOT_SUPPORTED;
}

std::int32_t APS5_VABI sceVrTracker2RegisterDevice(std::int32_t type, std::int32_t handle) {
    (void)type;
    (void)handle;
    if (!initialized.load()) return SCE_VR_TRACKER2_ERROR_NOT_INIT;
    return SCE_VR_TRACKER2_ERROR_INVALID_DEVICE_HANDLE;
}

int APS5_VABI sceVrTracker2SetCoordinateSystem() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceVrTracker2UnregisterDevice(std::int32_t handle) {
    (void)handle;
    if (!initialized.load()) return SCE_VR_TRACKER2_ERROR_NOT_INIT;
    return SCE_VR_TRACKER2_ERROR_ARGUMENT_INVALID;
}

}
