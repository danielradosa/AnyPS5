#include "ProcessParametersFixture.hpp"
#include "prx/libSceHmd/Hmd.hpp"
#include "prx/libSceHmd2/Hmd2.hpp"
#include "prx/libSceUserService/UserService.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

extern "C" {
std::int32_t APS5_VABI sceHmd2GazeGetResult(const void* param, void* result);
std::int32_t APS5_VABI sceHmd2GazeGetResultForFoveatedRendering();
std::int32_t APS5_VABI sceHmd2ReprojectionTerminate();
std::int32_t APS5_VABI sceHmd2ReprojectionDisableVrMode();
std::int32_t APS5_VABI sceHmd2ReprojectionBeginFrame(std::uint64_t frame);
std::int32_t APS5_VABI sceHmd2ReprojectionGetPredictedDisplayTime(std::uint64_t frame, std::uint64_t* time);
std::int32_t APS5_VABI sceHmd2ReprojectionSetParam(std::uint32_t buffer, std::uint64_t frame, const void* param,
                                                   const void* trace);
std::int32_t APS5_VABI sceHmd2ReprojectionSetParamWithBuffer();
std::int32_t APS5_VABI sceHmd2ReprojectionSetRenderConfig(std::uint64_t halfRate, std::uint64_t reserved0,
                                                          std::uint64_t reserved1);
std::int32_t APS5_VABI sceHmd2ReprojectionSetMirroringOption(const void* option);
std::int32_t APS5_VABI sceHmd2ReprojectionEnableMirroring(const void* param);
std::int32_t APS5_VABI sceHmd2ReprojectionDisableMirroring();
std::int32_t APS5_VABI sceHmd2GazeActivateGazeTracking();
std::int32_t APS5_VABI sceHmd2SetIpdOffset();
std::int32_t APS5_VABI sceHmd2GazeSetEnabledEye();
std::int32_t APS5_VABI sceHmd2GazeSetUserCalibrationPoint();
std::int32_t APS5_VABI sceHmd2GazeStartUserCalibration();
std::int32_t APS5_VABI sceHmd2GazeStopUserCalibration();
std::int32_t APS5_VABI sceHmd2GazeGetCalibrationData();
std::int32_t APS5_VABI sceHmd2GazeSetUserCalibrationData();
std::int32_t APS5_VABI sceHmd2GazeObjectSelectionInitialize();
std::int32_t APS5_VABI sceHmd2GazeObjectSelectionTerminate();
std::int32_t APS5_VABI sceHmd2ReprojectionSetUserEventStart();
std::int32_t APS5_VABI sceHmd2ReprojectionSetUserEventEnd();
std::int32_t APS5_VABI sceHmd2ReprojectionClearUserEventStart();
std::int32_t APS5_VABI sceHmd2ReprojectionClearUserEventEnd();
std::int32_t APS5_VABI sceHmd2ReprojectionGetCurrentOutput();
std::int32_t APS5_VABI sceHmd2GazeGetDeviceStatus(const void* status);
std::int32_t APS5_VABI sceHmd2GazeGetUserCalibrationStatus(const void* status);
std::int32_t APS5_VABI sceHmd2SetFieldOfView(const void* fieldOfView);
std::int32_t APS5_VABI sceHmd2GazeObjectSelectionQueryMemory(const void* first, const void* second);
std::int32_t APS5_VABI sceHmd2GazeObjectSelectionGetSelectedObject(const void* first, const void* second);
std::int32_t APS5_VABI sceHmd2GazeGetCalibrationDataMaxSize(const void* first, const void* second);
std::int32_t APS5_VABI sceHmd2GazeInsertRawDataWithHandle(const void* first, const void* second);
int APS5_VABI sceHmd2ReprojectionGetStatus();
}
namespace {

alignas(16) std::array<std::byte, 0x40> process{};

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

    Require(sceHmd2GazeGetResult(param, output) == Hmd2::NotInitialized, "gaze result");
    Require(std::memcmp(output, untouched, sizeof(output)) == 0, "failed query wrote output");
    Require(sceHmd2GazeGetResultForFoveatedRendering() == Hmd2::NotInitialized, "foveated gaze result");
    Require(sceHmd2ReprojectionBeginFrame(1) == Hmd2::NotInitialized, "begin frame");
    Require(sceHmd2ReprojectionGetPredictedDisplayTime(1, &time) == Hmd2::NotInitialized, "predicted display time");
    Require(time == 0x1234, "failed display time query wrote output");
    Require(sceHmd2ReprojectionSetParam(0, 1, param, nullptr) == Hmd2::NotInitialized, "set param");
    Require(sceHmd2ReprojectionSetParamWithBuffer() == Hmd2::NotInitialized, "set param with buffer");
    Require(sceHmd2ReprojectionSetRenderConfig(1, 0, 0) == Hmd2::NotInitialized, "render config");
    Require(sceHmd2ReprojectionSetMirroringOption(param) == Hmd2::NotInitialized, "mirroring option");
    Require(sceHmd2ReprojectionEnableMirroring(param) == Hmd2::NotInitialized, "enable mirroring");
    Require(sceHmd2ReprojectionDisableMirroring() == Hmd2::NotInitialized, "disable mirroring");
    Require(sceHmd2ReprojectionDisableVrMode() == Hmd2::NotInitialized, "disable VR mode");
    Require(sceHmd2ReprojectionTerminate() == Hmd2::NotInitialized, "reprojection terminate");
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

void TestReprojection() {
    Require(sceHmd2ReprojectionEnableVrMode(Hmd2::OutputMode89_91Hz) == Hmd2::ReprojectionNotInitialized,
            "enabling VR mode without a session");
    Require(sceHmd2ReprojectionSetTiming(3000) == Hmd2::ReprojectionNotInitialized, "setting the timing without a session");
    const Hmd2::SizeAlign sizeAlign = sceHmd2ReprojectionQueryBufferSizeAlign();
    Require(sizeAlign.size == Hmd2::ReprojectionBufferSize && sizeAlign.align == Hmd2::ReprojectionBufferAlign,
            "the reprojection work size did not answer the console's requirement");
    alignas(Hmd2::ReprojectionBufferAlign) static std::uint8_t work[Hmd2::ReprojectionBufferSize]{};
    std::uint8_t display[64]{};
    std::uint8_t seeThrough[64]{};
    Hmd2::ReprojectionInitializeParam initParam{};
    Require(sceHmd2ReprojectionInitialize(nullptr) == Hmd2::ParameterNull, "initialization without a param");
    Require(sceHmd2ReprojectionInitialize(&initParam) == Hmd2::ParameterNull, "initialization without buffers");
    initParam.reprojectionBuffer = work;
    Require(sceHmd2ReprojectionInitialize(&initParam) == Hmd2::ParameterNull, "initialization without a display buffer");
    initParam.displayBuffer = display;
    initParam.seeThroughBuffer = seeThrough;
    bool threw = false;
    try {
        sceHmd2ReprojectionInitialize(&initParam);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "initialization with a see-through buffer did not throw");
    initParam.seeThroughBuffer = nullptr;
    initParam.reprojectionTiming = 0;
    Require(sceHmd2ReprojectionInitialize(&initParam) == 0, "initialization did not succeed");
    const auto* state = reinterpret_cast<const Hmd2::ReprojectionState*>(work);
    Require(state->outputMode == 0 && state->timingUs == 3000, "the initial state did not carry the default timing");
    Require(sceHmd2ReprojectionInitialize(&initParam) == Hmd2::ReprojectionAlreadyInitialized, "a second initialization");
    Require(sceHmd2ReprojectionSetTiming(2500) == 0, "setting the timing did not succeed");
    Require(state->timingUs == 2500, "the timing was not recorded in the work buffer");
    Require(sceHmd2ReprojectionSetTiming(2001) == 0 && state->timingUs == 2100, "the timing was not rounded up to hundreds");
    threw = false;
    try {
        sceHmd2ReprojectionSetTiming(1999);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "a timing below the range did not throw");
    threw = false;
    try {
        sceHmd2ReprojectionSetTiming(7001);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "a timing above the range did not throw");
    Require(sceHmd2ReprojectionEnableVrMode(0x2000C) == Hmd2::VideoOutNoDevice, "enabling VR mode did not report no device");
    Require(sceHmd2ReprojectionEnableVrMode(0x2000F) == Hmd2::VideoOutNoDevice, "enabling the second VR mode did not report no device");
    Require(sceHmd2ReprojectionEnableVrMode(0) == Hmd2::VideoOutInvalidValue, "an unknown output mode did not report invalid value");
    Require(state->outputMode == 0, "the output mode changed without a VR runtime");
    Require(sceHmd2ReprojectionSetAllowPositionalReprojection(6, 0x100000001, 1) == Hmd2::ParameterInvalid,
            "a positional setting above the range did not report invalid parameter");
    Require(sceHmd2ReprojectionSetAllowPositionalReprojection(-1, 0x100000001, 1) == Hmd2::ParameterInvalid,
            "a negative positional setting did not report invalid parameter");
    Hmd2::SizeAlign positional = sceHmd2ReprojectionQueryBufferSizeAlign();
    Require(positional.size == Hmd2::ReprojectionBufferSize && positional.align == Hmd2::ReprojectionBufferAlign,
            "a rejected positional setting changed the work-size answer");
    Require(sceHmd2ReprojectionSetAllowPositionalReprojection(2, 0x100000001, 1) == 0,
            "the positional setting did not succeed");
    positional = sceHmd2ReprojectionQueryBufferSizeAlign();
    Require(positional.size == Hmd2::ReprojectionPositionalBufferSize + 2 * Hmd2::ReprojectionPositionalBufferSizeStep
                && positional.align == Hmd2::ReprojectionPositionalBufferAlign,
            "the positional work size did not follow the setting");
    Require(sceHmd2ReprojectionSetAllowPositionalReprojection(1, 0, 0) == 0,
            "the lowest positional setting did not succeed");
    positional = sceHmd2ReprojectionQueryBufferSizeAlign();
    Require(positional.size == Hmd2::ReprojectionPositionalBufferSize + Hmd2::ReprojectionPositionalBufferSizeStep
                && positional.align == Hmd2::ReprojectionPositionalBufferAlign,
            "the lowest positional work size did not answer the console's requirement");
    Require(sceHmd2ReprojectionSetAllowPositionalReprojection(5, 0, 0) == 0,
            "the highest positional setting did not succeed");
    positional = sceHmd2ReprojectionQueryBufferSizeAlign();
    Require(positional.size == Hmd2::ReprojectionPositionalBufferSize + 5 * Hmd2::ReprojectionPositionalBufferSizeStep
                && positional.align == Hmd2::ReprojectionPositionalBufferAlign,
            "the highest positional work size did not answer the console's requirement");
    Require(sceHmd2ReprojectionSetAllowPositionalReprojection(0, 0x100000001, 1) == 0,
            "clearing the positional setting did not succeed");
    const Hmd2::SizeAlign cleared = sceHmd2ReprojectionQueryBufferSizeAlign();
    Require(cleared.size == Hmd2::ReprojectionBufferSize && cleared.align == Hmd2::ReprojectionBufferAlign,
            "clearing the positional setting did not restore the work-size answer");
}

}

void SizeQueriesReturnTheConsoleSizes() {
    const auto display = sceHmd2ReprojectionQueryDisplayBufferSizeAlign();
    Require(display.size == 0x6000000 && display.align == 0x10000, "display buffer size");
    const auto seeThrough = sceHmd2ReprojectionQuerySeeThroughBufferSizeAlign();
    Require(seeThrough.size == 0x1106000 && seeThrough.align == 0x100, "see-through buffer size");
    const auto mirroring = sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign();
    Require(mirroring.size == 0x5FA0000 && mirroring.align == 0x10000, "mirroring display buffer size");
    const auto work = sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign();
    Require(work.size == 0x4000 && work.align == 0x100, "mirroring work memory size");
}

void MeasuredAnswersBeforeInitialization() {
    std::array<std::uint8_t, 0x4000> zeroed{};
    const void* buffer = zeroed.data();
    Require(sceHmd2GazeActivateGazeTracking() == 0, "gaze activation");
    Require(sceHmd2SetIpdOffset() == 0, "IPD offset");
    Require(sceHmd2GazeSetEnabledEye() == Hmd2::Error0004, "enabled eye");
    Require(sceHmd2GazeSetUserCalibrationPoint() == Hmd2::Error0004, "calibration point");
    Require(sceHmd2GazeStartUserCalibration() == Hmd2::Error0004, "start calibration");
    Require(sceHmd2GazeStopUserCalibration() == Hmd2::Error0004, "stop calibration");
    Require(sceHmd2GazeGetCalibrationData() == Hmd2::ParameterNull, "get calibration data");
    Require(sceHmd2GazeSetUserCalibrationData() == Hmd2::ParameterNull, "set calibration data");
    Require(sceHmd2GazeObjectSelectionInitialize() == Hmd2::ParameterNull, "object selection initialize");
    Require(sceHmd2GazeObjectSelectionTerminate() == Hmd2::Error002C, "object selection terminate");
    Require(sceHmd2ReprojectionSetUserEventStart() == Hmd2::ReprojectionNotInitialized, "user event start");
    Require(sceHmd2ReprojectionSetUserEventEnd() == Hmd2::ReprojectionNotInitialized, "user event end");
    Require(sceHmd2ReprojectionClearUserEventStart() == Hmd2::ReprojectionNotInitialized, "clear user event start");
    Require(sceHmd2ReprojectionClearUserEventEnd() == Hmd2::ReprojectionNotInitialized, "clear user event end");
    Require(sceHmd2ReprojectionGetCurrentOutput() == Hmd2::ReprojectionNotInitialized, "current output");
    Require(sceHmd2GazeGetDeviceStatus(nullptr) == Hmd2::ParameterNull, "null gaze device status");
    Require(sceHmd2GazeGetDeviceStatus(buffer) == Hmd2::Error0004, "gaze device status");
    Require(sceHmd2GazeGetUserCalibrationStatus(nullptr) == Hmd2::ParameterNull, "null calibration status");
    Require(sceHmd2GazeGetUserCalibrationStatus(buffer) == Hmd2::Error0004, "calibration status");
    Require(sceHmd2SetFieldOfView(nullptr) == Hmd2::ParameterNull, "null field of view");
    Require(sceHmd2SetFieldOfView(buffer) == 0, "set field of view");
    Require(sceHmd2GazeObjectSelectionQueryMemory(nullptr, buffer) == Hmd2::ParameterNull, "object selection memory, null first");
    Require(sceHmd2GazeObjectSelectionQueryMemory(buffer, nullptr) == Hmd2::ParameterNull, "object selection memory, null second");
    Require(sceHmd2GazeObjectSelectionQueryMemory(buffer, buffer) == Hmd2::ParameterInvalid, "object selection memory");
    Require(sceHmd2GazeObjectSelectionGetSelectedObject(nullptr, buffer) == Hmd2::ParameterNull, "selected object, null first");
    Require(sceHmd2GazeObjectSelectionGetSelectedObject(buffer, nullptr) == Hmd2::ParameterNull, "selected object, null second");
    Require(sceHmd2GazeObjectSelectionGetSelectedObject(buffer, buffer) == Hmd2::ParameterInvalid, "selected object");
    Require(sceHmd2GazeGetCalibrationDataMaxSize(nullptr, buffer) == Hmd2::ParameterInvalid, "calibration size, null first");
    Require(sceHmd2GazeGetCalibrationDataMaxSize(buffer, nullptr) == Hmd2::ParameterNull, "calibration size, null second");
    Require(sceHmd2GazeInsertRawDataWithHandle(nullptr, buffer) == Hmd2::InvalidHandle, "raw data, null first");
    Require(sceHmd2GazeInsertRawDataWithHandle(buffer, nullptr) == Hmd2::ParameterNull, "raw data, null second");
    for (const auto byte : zeroed) Require(byte == 0, "a measured answer wrote its argument");
}

template <typename F>
void RequireRuntimeError(F f, const char* message) {
    bool threw = false;
    try {
        f();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, message);
}

void UnmeasuredWithReprojectionSession() {
    RequireRuntimeError([] { sceHmd2ReprojectionSetUserEventStart(); }, "user event start with a session did not throw");
    RequireRuntimeError([] { sceHmd2ReprojectionSetUserEventEnd(); }, "user event end with a session did not throw");
    RequireRuntimeError([] { sceHmd2ReprojectionClearUserEventStart(); }, "clear user event start with a session did not throw");
    RequireRuntimeError([] { sceHmd2ReprojectionClearUserEventEnd(); }, "clear user event end with a session did not throw");
    RequireRuntimeError([] { sceHmd2ReprojectionGetCurrentOutput(); }, "current output with a session did not throw");
    Require(sceHmd2GazeActivateGazeTracking() == 0, "gaze activation depends on the reprojection session");
}

void SizeQueriesReturnTheConsoleSizes() {
    const auto display = sceHmd2ReprojectionQueryDisplayBufferSizeAlign();
    Require(display.size == 0x6000000 && display.align == 0x10000, "display buffer size");
    const auto seeThrough = sceHmd2ReprojectionQuerySeeThroughBufferSizeAlign();
    Require(seeThrough.size == 0x1106000 && seeThrough.align == 0x100, "see-through buffer size");
    const auto mirroring = sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign();
    Require(mirroring.size == 0x5FA0000 && mirroring.align == 0x10000, "mirroring display buffer size");
    const auto work = sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign();
    Require(work.size == 0x4000 && work.align == 0x100, "mirroring work memory size");
}

int main() {
    SizeQueriesReturnTheConsoleSizes();
    InstallProcessParameters();
    CheckUninitialized();
    TestBeforeInitialize();
    TestWithoutHeadset();
    CheckUninitialized();
    MeasuredAnswersBeforeInitialization();
    TestReprojection();
    UnmeasuredWithReprojectionSession();
    RequireRuntimeError([] { sceHmd2ReprojectionGetStatus(); }, "reprojection status did not throw");
    std::puts("HMD2 tests passed");
    return 0;
}
