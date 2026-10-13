#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" {
std::int32_t APS5_VABI sceHmd2Initialize(const void* param);
std::int32_t APS5_VABI sceHmd2Terminate();
std::int32_t APS5_VABI sceHmd2Open(std::int32_t userId, std::int32_t type, std::int32_t index, const void* param);
std::int32_t APS5_VABI sceHmd2Close(std::int32_t handle);
std::int32_t APS5_VABI sceHmd2GetDeviceInformation(void* info);
std::int32_t APS5_VABI sceHmd2GetDeviceInformationByHandle(std::int32_t handle, void* info);
std::int32_t APS5_VABI sceHmd2GetFieldOfView(std::int32_t handle, void* fieldOfView);
std::int32_t APS5_VABI sceHmd2GetFieldOfViewWithoutHandle(void* fieldOfView);
std::int32_t APS5_VABI sceHmd2SetVibration(std::int32_t handle, std::int32_t strength);
std::int32_t APS5_VABI sceHmd2GazeGetResult(const void* param, void* result);
std::int32_t APS5_VABI sceHmd2GazeGetResultForFoveatedRendering();
std::int32_t APS5_VABI sceHmd2ReprojectionInitialize(const void* param, const void* reserved);
std::int32_t APS5_VABI sceHmd2ReprojectionTerminate();
std::int32_t APS5_VABI sceHmd2ReprojectionEnableVrMode(std::uint64_t mode);
std::int32_t APS5_VABI sceHmd2ReprojectionDisableVrMode();
std::int32_t APS5_VABI sceHmd2ReprojectionBeginFrame(std::uint64_t frame);
std::int32_t APS5_VABI sceHmd2ReprojectionGetPredictedDisplayTime(std::uint64_t frame, std::uint64_t* time);
std::int32_t APS5_VABI sceHmd2ReprojectionSetParam(std::uint32_t buffer, std::uint64_t frame, const void* param,
                                                   const void* trace);
std::int32_t APS5_VABI sceHmd2ReprojectionSetParamWithBuffer();
std::int32_t APS5_VABI sceHmd2ReprojectionSetRenderConfig(std::uint64_t halfRate, std::uint64_t reserved0,
                                                          std::uint64_t reserved1);
std::int32_t APS5_VABI sceHmd2ReprojectionSetAllowPositionalReprojection();
std::int32_t APS5_VABI sceHmd2ReprojectionSetTiming();
std::int32_t APS5_VABI sceHmd2ReprojectionSetMirroringOption(const void* option);
std::int32_t APS5_VABI sceHmd2ReprojectionEnableMirroring(const void* param);
std::int32_t APS5_VABI sceHmd2ReprojectionDisableMirroring();
}

namespace {

constexpr auto NotInitialized = static_cast<std::int32_t>(0x81110002);
constexpr auto ParameterNull = static_cast<std::int32_t>(0x81110008);
constexpr auto UnsupportedFeature = static_cast<std::int32_t>(0x81110016);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "HMD2: %s\n", message);
        std::abort();
    }
}

void CheckUninitialized() {
    std::uint8_t output[64];
    std::memset(output, 0xa5, sizeof(output));
    std::uint8_t untouched[sizeof(output)];
    std::memcpy(untouched, output, sizeof(output));
    std::uint8_t param[0x48]{};
    std::uint64_t time = 0x1234;

    Require(sceHmd2GetDeviceInformation(nullptr) == ParameterNull, "null device information");
    Require(sceHmd2GetDeviceInformation(output) == NotInitialized, "device information before initialization");
    Require(sceHmd2GetFieldOfViewWithoutHandle(nullptr) == ParameterNull, "null field of view without handle");
    Require(sceHmd2GetFieldOfViewWithoutHandle(output) == NotInitialized, "field of view before initialization");
    for (const std::int32_t handle : {0, 1, -1, 0x0f000000}) {
        Require(sceHmd2GetFieldOfView(handle, nullptr) == ParameterNull, "null field of view");
        Require(sceHmd2GetFieldOfView(handle, output) == NotInitialized, "handle field of view");
        Require(sceHmd2GetDeviceInformationByHandle(handle, output) == NotInitialized, "handle information");
        Require(sceHmd2Close(handle) == NotInitialized, "close");
        Require(sceHmd2SetVibration(handle, 25) == NotInitialized, "vibration");
    }
    Require(std::memcmp(output, untouched, sizeof(output)) == 0, "failed query wrote output");

    Require(sceHmd2Open(1, 0, 0, nullptr) == NotInitialized, "open before initialization");
    Require(sceHmd2GazeGetResult(param, output) == NotInitialized, "gaze result");
    Require(sceHmd2GazeGetResultForFoveatedRendering() == NotInitialized, "foveated gaze result");
    Require(sceHmd2ReprojectionInitialize(param, nullptr) == NotInitialized, "reprojection initialize");
    Require(sceHmd2ReprojectionEnableVrMode(0x2000f) == NotInitialized, "enable VR mode");
    Require(sceHmd2ReprojectionBeginFrame(1) == NotInitialized, "begin frame");
    Require(sceHmd2ReprojectionGetPredictedDisplayTime(1, &time) == NotInitialized, "predicted display time");
    Require(time == 0x1234, "failed display time query wrote output");
    Require(sceHmd2ReprojectionSetParam(0, 1, param, nullptr) == NotInitialized, "set param");
    Require(sceHmd2ReprojectionSetParamWithBuffer() == NotInitialized, "set param with buffer");
    Require(sceHmd2ReprojectionSetRenderConfig(1, 0, 0) == NotInitialized, "render config");
    Require(sceHmd2ReprojectionSetAllowPositionalReprojection() == NotInitialized, "positional reprojection");
    Require(sceHmd2ReprojectionSetTiming() == NotInitialized, "timing");
    Require(sceHmd2ReprojectionSetMirroringOption(param) == NotInitialized, "mirroring option");
    Require(sceHmd2ReprojectionEnableMirroring(param) == NotInitialized, "enable mirroring");
    Require(sceHmd2ReprojectionDisableMirroring() == NotInitialized, "disable mirroring");
    Require(sceHmd2ReprojectionDisableVrMode() == NotInitialized, "disable VR mode");
    Require(sceHmd2ReprojectionTerminate() == NotInitialized, "reprojection terminate");
    Require(sceHmd2Terminate() == NotInitialized, "terminate");
}

}

int main() {
    const std::uint8_t param[16]{};
    CheckUninitialized();
    Require(sceHmd2Initialize(param) == UnsupportedFeature, "initialization did not report the unsupported feature");
    Require(sceHmd2Initialize(nullptr) == UnsupportedFeature, "initialization without a param did not report the unsupported feature");
    CheckUninitialized();
    Require(sceHmd2Initialize(param) == UnsupportedFeature, "repeated initialization changed its result");
    CheckUninitialized();
    std::puts("HMD2 tests passed");
    return 0;
}
