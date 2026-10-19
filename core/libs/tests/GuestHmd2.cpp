#include "ProcessParametersFixture.hpp"
#include "prx/libSceHmd/Hmd.hpp"
#include "prx/libSceHmd2/Hmd2.hpp"
#include "prx/libSceUserService/UserService.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <stdexcept>

struct Hmd2SizeAlign {
    std::uint64_t size;
    std::uint64_t align;
};

extern "C" {
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
Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionQueryBufferSizeAlign();
Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionQueryDisplayBufferSizeAlign();
Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionQuerySeeThroughBufferSizeAlign();
Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign();
Hmd2SizeAlign APS5_VABI sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign();
int APS5_VABI sceHmd2GazeActivateGazeTracking();
int APS5_VABI sceHmd2ReprojectionGetCurrentOutput();
int APS5_VABI sceHmd2SetFieldOfView();
}
namespace {

alignas(16) std::array<std::byte, 0x40> process{};

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "HMD2: %s\n", message);
        std::abort();
    }
}

template <typename F>
void RequireThrows(F f, const char* message) {
    bool threw = false;
    try {
        f();
    } catch (const std::exception&) {
        threw = true;
    }
    Require(threw, message);
}

void CheckSizeQueries() {
    RequireThrows([] { sceHmd2ReprojectionQueryBufferSizeAlign(); }, "reprojection buffer size");
    RequireThrows([] { sceHmd2ReprojectionQueryDisplayBufferSizeAlign(); }, "display buffer size");
    RequireThrows([] { sceHmd2ReprojectionQuerySeeThroughBufferSizeAlign(); }, "see-through buffer size");
    RequireThrows([] { sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign(); }, "mirroring work memory size");
    RequireThrows([] { sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign(); }, "mirroring display buffer size");
}

void CheckUninitialized() {
    std::uint8_t output[64];
    std::memset(output, 0xa5, sizeof(output));
    std::uint8_t untouched[sizeof(output)];
    std::memcpy(untouched, output, sizeof(output));
    std::uint8_t param[0x48]{};
    std::uint64_t time = 0x1234;

    Require(sceHmd2GazeGetResult(param, output) == Hmd2::NotInitialized, "gaze result");
    Require(std::memcmp(output, untouched, sizeof(output)) == 0, "failed query wrote output");
    Require(sceHmd2GazeGetResultForFoveatedRendering() == Hmd2::NotInitialized, "foveated gaze result");
    Require(sceHmd2ReprojectionInitialize(param, nullptr) == Hmd2::NotInitialized, "reprojection initialize");
    Require(sceHmd2ReprojectionEnableVrMode(0x2000f) == Hmd2::NotInitialized, "enable VR mode");
    Require(sceHmd2ReprojectionBeginFrame(1) == Hmd2::NotInitialized, "begin frame");
    Require(sceHmd2ReprojectionGetPredictedDisplayTime(1, &time) == Hmd2::NotInitialized, "predicted display time");
    Require(time == 0x1234, "failed display time query wrote output");
    Require(sceHmd2ReprojectionSetParam(0, 1, param, nullptr) == Hmd2::NotInitialized, "set param");
    Require(sceHmd2ReprojectionSetParamWithBuffer() == Hmd2::NotInitialized, "set param with buffer");
    Require(sceHmd2ReprojectionSetRenderConfig(1, 0, 0) == Hmd2::NotInitialized, "render config");
    Require(sceHmd2ReprojectionSetAllowPositionalReprojection() == Hmd2::NotInitialized, "positional reprojection");
    Require(sceHmd2ReprojectionSetTiming() == Hmd2::NotInitialized, "timing");
    Require(sceHmd2ReprojectionSetMirroringOption(param) == Hmd2::NotInitialized, "mirroring option");
    Require(sceHmd2ReprojectionEnableMirroring(param) == Hmd2::NotInitialized, "enable mirroring");
    Require(sceHmd2ReprojectionDisableMirroring() == Hmd2::NotInitialized, "disable mirroring");
    Require(sceHmd2ReprojectionDisableVrMode() == Hmd2::NotInitialized, "disable VR mode");
    Require(sceHmd2ReprojectionTerminate() == Hmd2::NotInitialized, "reprojection terminate");
    CheckSizeQueries();
}
void InstallProcessParameters() {
    const std::uint64_t size = process.size();
    const std::uint32_t magic = 0x4942524f;
    std::memcpy(process.data(), &size, sizeof(size));
    std::memcpy(process.data() + 8, &magic, sizeof(magic));
    Require(ProcessParametersFixture::Install(process.data(), process.size()), "process parameters were not installed");
}

void TestBeforeInitialize() {
    Hmd2::DeviceInformation info;
    std::memset(&info, 0xcc, sizeof(info));
    Require(sceHmd2GetDeviceInformation(&info) == Hmd2::NotInitialized, "device information before initialization");
    const Hmd2::DeviceInformation cleared{};
    Require(std::memcmp(&info, &cleared, sizeof(info)) == 0, "device information was not cleared before the initialization check");
    Require(sceHmd2GetDeviceInformation(nullptr) == Hmd2::ParameterNull, "null device information before initialization");
    Hmd2::FieldOfView fieldOfView{};
    Require(sceHmd2GetFieldOfViewWithoutHandle(&fieldOfView) == Hmd2::NotInitialized, "field of view before initialization");
    Require(sceHmd2GetFieldOfView(0, &fieldOfView) == Hmd2::InvalidHandle, "field of view of handle 0 before initialization");
    Require(sceHmd2GetDeviceInformationByHandle(0, &info) == Hmd2::NotInitialized, "device information of handle 0 before initialization");
    Require(sceHmd2Open(USER_SERVICE_INITIAL_USER_ID, 0, 0, nullptr) == Hmd2::NotInitialized, "open before initialization");
    Require(sceHmd2Close(0) == Hmd2::NotInitialized, "close before initialization");
    Require(sceHmd2Terminate() == Hmd2::NotInitialized, "terminate before initialization");
    Require(sceHmd2SetVibration(0, 0) == 0, "vibration of the unopened handle 0 failed");
}

void TestWithoutHeadset() {
    const std::uint8_t param[16]{};
    Require(sceHmd2Initialize(param) == Hmd::UnsupportedFeature, "initialization without a headset did not fail");
    Require(sceHmd2Initialize(nullptr) == Hmd::UnsupportedFeature, "initialization without a param did not fail as unsupported");
    Require(sceHmd2Open(USER_SERVICE_INITIAL_USER_ID, 0, 0, nullptr) == Hmd2::NotInitialized, "open without a headset did not fail");
    Hmd2::DeviceInformation info;
    Require(sceHmd2GetDeviceInformation(&info) == Hmd2::NotInitialized, "device information without a headset");
    Require(sceHmd2Terminate() == Hmd2::NotInitialized, "terminate after the failed initialization");
}

}

int main() {
    InstallProcessParameters();
    CheckUninitialized();
    TestBeforeInitialize();
    TestWithoutHeadset();
    CheckUninitialized();
    bool threw = false;
    try {
        sceHmd2GazeActivateGazeTracking();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "gaze activation did not throw");
    threw = false;
    try {
        sceHmd2ReprojectionGetCurrentOutput();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "reprojection get current output did not throw");
    threw = false;
    try {
        sceHmd2SetFieldOfView();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "set field of view did not throw");
    std::puts("HMD2 tests passed");
    return 0;
}
