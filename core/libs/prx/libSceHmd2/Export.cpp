#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t SCE_HMD2_ERROR_NOT_INITIALIZED = static_cast<std::int32_t>(0x81110002);
constexpr std::int32_t SCE_HMD2_ERROR_PARAMETER_NULL = static_cast<std::int32_t>(0x81110008);
constexpr std::int32_t SCE_HMD2_ERROR_UNSUPPORTED_FEATURE = static_cast<std::int32_t>(0x81110016);

struct Hmd2SizeAlign {
    std::uint64_t size;
    std::uint64_t align;
};
static_assert(sizeof(Hmd2SizeAlign) == 16);

}

extern "C" {

Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionQuerySeeThroughBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

std::int32_t APS5_VABI sceHmd2Initialize(const void* param) {
    (void)param;
    return SCE_HMD2_ERROR_UNSUPPORTED_FEATURE;
}

std::int32_t APS5_VABI sceHmd2GetFieldOfView(std::int32_t handle, void* fieldOfView) {
    (void)handle;
    if (!fieldOfView) return SCE_HMD2_ERROR_PARAMETER_NULL;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2GetDeviceInformationByHandle(std::int32_t handle, void* info) {
    (void)handle;
    (void)info;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2Terminate() {
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2Close(std::int32_t handle) {
    (void)handle;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2GazeGetResult(const void* param, void* result) {
    (void)param;
    (void)result;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2GazeGetResultForFoveatedRendering() {
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2GetDeviceInformation(void* info) {
    if (!info) return SCE_HMD2_ERROR_PARAMETER_NULL;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2GetFieldOfViewWithoutHandle(void* fieldOfView) {
    if (!fieldOfView) return SCE_HMD2_ERROR_PARAMETER_NULL;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2Open(std::int32_t userId, std::int32_t type, std::int32_t index, const void* param) {
    (void)userId;
    (void)type;
    (void)index;
    (void)param;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionBeginFrame(std::uint64_t frame) {
    (void)frame;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionDisableMirroring() {
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionDisableVrMode() {
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionEnableMirroring(const void* param) {
    (void)param;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionEnableVrMode(std::uint64_t mode) {
    (void)mode;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

std::int32_t APS5_VABI sceHmd2ReprojectionGetPredictedDisplayTime(std::uint64_t frame, std::uint64_t* time) {
    (void)frame;
    (void)time;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionInitialize(const void* param, const void* reserved) {
    (void)param;
    (void)reserved;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionQueryBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionQueryDisplayBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return {};
}

std::int32_t APS5_VABI sceHmd2ReprojectionSetAllowPositionalReprojection() {
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionSetMirroringOption(const void* option) {
    (void)option;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionSetParam(std::uint32_t buffer, std::uint64_t frame, const void* param,
                                                   const void* trace) {
    (void)buffer;
    (void)frame;
    (void)param;
    (void)trace;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionSetParamWithBuffer() {
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionSetRenderConfig(std::uint64_t halfRate, std::uint64_t reserved0,
                                                          std::uint64_t reserved1) {
    (void)halfRate;
    (void)reserved0;
    (void)reserved1;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionSetTiming() {
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2ReprojectionTerminate() {
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

std::int32_t APS5_VABI sceHmd2SetVibration(std::int32_t handle, std::int32_t strength) {
    (void)handle;
    (void)strength;
    return SCE_HMD2_ERROR_NOT_INITIALIZED;
}

}
