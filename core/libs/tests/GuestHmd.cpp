#include "prx/libSceHmd/Hmd.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "HMD: %s\n", message);
        std::abort();
    }
}

void CheckDisconnected() {
    Hmd::DeviceStatus status = Hmd::DeviceStatus::Ready;
    Require(sceHmdInternalGetDeviceStatus(&status) == 0, "status query failed");
    Require(status == Hmd::DeviceStatus::NotDetected, "headset reported connected");
    Require(sceHmdInternalGetDeviceStatus(nullptr) == Hmd::ParameterNull, "null status");
    Require(sceHmdInternalMmapIsConnect() == 0, "connection reported present");
}

void CheckQueries() {
    Require(sceHmdReprojectionQueryGarlicBuffAlign() == 0x100, "garlic alignment");
    Require(sceHmdReprojectionQueryGarlicBuffSize() == 0x100000, "garlic size");
    Require(sceHmdReprojectionQueryOnionBuffAlign() == 0x100, "onion alignment");
    Require(sceHmdReprojectionQueryOnionBuffSize() == 0x810, "onion size");
    Require(sceHmdDistortionGetWorkMemoryAlign() == 0x400, "distortion alignment");
    Require(sceHmdDistortionGetWorkMemorySize() == 0x20000, "distortion size");
    Require(sceHmdGetDistortionWorkMemoryAlign() == 0x400, "distortion alignment alias");
    Require(sceHmdGetDistortionWorkMemorySize() == 0x20000, "distortion size alias");
    Require(sceHmdDistortionGetCompoundEyeCorrectionCommand() == Hmd::ParameterInvalid,
            "compound eye command");
}

void CheckHandleQueries(bool initialized) {
    Hmd::EyeOffset left;
    Hmd::EyeOffset right;
    Hmd::FieldOfView fieldOfView;
    unsigned char sensor[64];
    unsigned char assy[64];
    std::memset(&left, 0xa5, sizeof(left));
    std::memset(&right, 0xa5, sizeof(right));
    std::memset(&fieldOfView, 0xa5, sizeof(fieldOfView));
    std::memset(sensor, 0xa5, sizeof(sensor));
    std::memset(assy, 0xa5, sizeof(assy));
    const auto leftBefore = left;
    const auto rightBefore = right;
    const auto fieldOfViewBefore = fieldOfView;
    unsigned char untouched[64];
    std::memset(untouched, 0xa5, sizeof(untouched));
    const std::int32_t handleAnswer = initialized ? Hmd::InvalidHandle : Hmd::NotInitialized;

    for (const auto handle : {0, -1, 1, 0x0f000000}) {
        Require(sceHmdGet2DEyeOffset(handle, &left, &right) == handleAnswer, "eye offset answer");
        Require(sceHmdGet2DEyeOffset(handle, nullptr, nullptr) == handleAnswer, "null eye offset answer");
        Require(sceHmdGetInertialSensorData(handle, sensor, 1) == handleAnswer, "sensor answer");
        Require(sceHmdGetInertialSensorData(handle, nullptr, 0) == handleAnswer, "null sensor answer");
        Require(sceHmdGetFieldOfView(handle, &fieldOfView) == Hmd::InvalidHandle, "field of view answer");
        Require(sceHmdGetFieldOfView(handle, nullptr) == Hmd::ParameterNull, "null field of view");
    }
    Require(sceHmdGetAssyError(nullptr) == Hmd::ParameterNull, "null assy error");
    Require(sceHmdGetAssyError(assy) ==
                (initialized ? Hmd::DeviceDisconnected : Hmd::NotInitialized), "assy error answer");
    Require(std::memcmp(&left, &leftBefore, sizeof(left)) == 0, "left eye offset written");
    Require(std::memcmp(&right, &rightBefore, sizeof(right)) == 0, "right eye offset written");
    Require(std::memcmp(&fieldOfView, &fieldOfViewBefore, sizeof(fieldOfView)) == 0,
            "field of view written");
    Require(std::memcmp(sensor, untouched, sizeof(sensor)) == 0, "sensor data written");
    Require(std::memcmp(assy, untouched, sizeof(assy)) == 0, "assy error written");
}

}

int main() {
    Hmd::InitializeParam param{};
    Hmd::DeviceInformation info;
    std::memset(&info, 0xa5, sizeof(info));
    const auto untouched = info;

    CheckDisconnected();
    CheckQueries();
    CheckHandleQueries(false);
    Require(sceHmdTerminate() == Hmd::NotInitialized, "terminate before initialize");
    Require(sceHmdOpen(1, 0, 0, nullptr) == Hmd::NotInitialized, "open before initialize");
    Require(sceHmdClose(0) == Hmd::NotInitialized, "close before initialize");
    Require(sceHmdGetDeviceInformation(nullptr) == Hmd::ParameterNull, "null information");
    Require(sceHmdGetDeviceInformation(&info) == Hmd::NotInitialized, "query before initialize");
    Require(std::memcmp(&info, &untouched, sizeof(info)) == 0, "failed query modified output");
    Require(sceHmdGetDeviceInformationByHandle(0, &info) == Hmd::NotInitialized,
            "handle query before initialize");
    Require(sceHmdInitialize(nullptr) == Hmd::ParameterNull, "null initialize");
    Require(sceHmdInitialize315(nullptr) == Hmd::ParameterNull, "null initialize315");
    param.reserved0 = &param;
    Require(sceHmdInitialize(&param) == Hmd::UnsupportedFeature, "distortion unsupported");
    Require(sceHmdInitialize315(&param) == Hmd::UnsupportedFeature, "distortion315 unsupported");
    param.reserved0 = nullptr;

    for (const auto initialize : {sceHmdInitialize, sceHmdInitialize315}) {
        Require(initialize(&param) == 0, "initialize failed");
        Require(sceHmdInitialize(&param) == Hmd::AlreadyInitialized, "duplicate initialize");
        Require(sceHmdInitialize315(&param) == Hmd::AlreadyInitialized, "shared initialization");
        Require(sceHmdInitialize(nullptr) == Hmd::AlreadyInitialized, "duplicate precedence");
        CheckDisconnected();
        Require(sceHmdGetDeviceInformation(&info) == 0, "device query failed");
        Hmd::DeviceInformation expected{};
        expected.status = Hmd::DeviceStatus::NotDetected;
        expected.userId = -1;
        Require(std::memcmp(&info, &expected, sizeof(info)) == 0, "incorrect absent-device output");
        Require(sceHmdGetDeviceInformation(nullptr) == Hmd::ParameterNull, "null device query");

        Require(sceHmdOpen(-1, 0, 0, nullptr) == Hmd::ParameterInvalid, "invalid user");
        Require(sceHmdOpen(0xff, 0, 0, nullptr) == Hmd::ParameterInvalid, "system user");
        Require(sceHmdOpen(1, 1, 0, nullptr) == Hmd::ParameterInvalid, "invalid type");
        Require(sceHmdOpen(1, 0, 1, nullptr) == Hmd::ParameterInvalid, "invalid index");
        Require(sceHmdOpen(1, 0, 0, reinterpret_cast<Hmd::OpenParam*>(&param)) ==
                    Hmd::ParameterInvalid, "non-null reserved open parameter");
        Require(sceHmdOpen(1, 0, 0, nullptr) == Hmd::DeviceDisconnected, "open faked a handle");
        Require(sceHmdOpen(1, 0, 0, nullptr) == Hmd::DeviceDisconnected, "failed open created state");
        for (const auto handle : {0, -1, 1, 0x0f000000}) {
            Require(sceHmdClose(handle) == Hmd::InvalidHandle, "close accepted invalid handle");
            Require(sceHmdGetDeviceInformationByHandle(handle, &info) == Hmd::InvalidHandle,
                    "query accepted invalid handle");
            Require(std::memcmp(&info, &expected, sizeof(info)) == 0, "invalid handle modified output");
        }
        Require(sceHmdGetDeviceInformationByHandle(0, nullptr) == Hmd::InvalidHandle,
                "null query accepted invalid handle");
        CheckQueries();
        CheckHandleQueries(true);
        Require(sceHmdTerminate() == 0, "terminate failed");
        Require(sceHmdTerminate() == Hmd::NotInitialized, "duplicate terminate");
        Require(sceHmdGetDeviceInformation(&info) == Hmd::NotInitialized, "query after terminate");
        CheckDisconnected();
        CheckHandleQueries(false);
    }
}
