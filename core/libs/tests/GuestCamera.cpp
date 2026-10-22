#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI sceCameraIsAttached(std::int32_t index);
int APS5_VABI sceCameraOpen(std::int32_t userId, std::int32_t type, std::int32_t index, void* param);
int APS5_VABI sceCameraClose(std::int32_t handle);
int APS5_VABI sceCameraStart(std::int32_t handle, const void* param);
int APS5_VABI sceCameraStop(std::int32_t handle);
int APS5_VABI sceCameraGetConfig(std::int32_t handle, void* config);
int APS5_VABI sceCameraSetConfig(std::int32_t handle, const void* config);
int APS5_VABI sceCameraSetAttribute(std::int32_t handle, const void* attribute);
int APS5_VABI sceCameraGetDeviceInfo(std::int32_t reserved, const void* deviceInfo);
int APS5_VABI sceCameraGetFrameData(std::int32_t handle, void* frameData);
int APS5_VABI sceCameraIsValidFrameData(std::int32_t handle, const void* frameData);
}

namespace {

constexpr int ErrorParam = static_cast<int>(0x802E0000u);
constexpr int ErrorNotInit = static_cast<int>(0x802E0002u);
constexpr int ErrorNotOpen = static_cast<int>(0x802E0004u);
constexpr int ErrorFormatUnknown = static_cast<int>(0x802E0007u);
constexpr int ErrorNotConnected = static_cast<int>(0x802E0010u);
constexpr std::int32_t UserIdSystem = 0xFF;

struct StartParameter {
    std::uint32_t sizeThis;
    std::uint32_t formatLevel[2];
    void* startOption;
};

struct ConfigExtension {
    std::uint32_t formatLevel[4];
    std::uint32_t resolution;
    std::uint32_t framerate;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t reserved1;
    void* baseOption;
};

struct Config {
    std::uint32_t sizeThis;
    std::uint32_t configType;
    ConfigExtension configExtension[2];
};

struct Attribute {
    std::uint32_t sizeThis;
    std::int32_t channel;
    std::uint8_t settings[104];
};

struct DeviceInfo {
    std::uint32_t sizeThis;
    std::uint32_t infoRevision;
    std::uint32_t deviceRevision;
    std::uint32_t padding;
};

struct FrameData {
    std::uint32_t sizeThis;
    std::uint32_t readMode;
    std::uint8_t framePosition[2][4][16];
    void* framePointerList[2][4];
    std::uint32_t frameSize[2][4];
    std::uint32_t status[2];
    std::uint8_t rest[344];
};

static_assert(sizeof(StartParameter) == 24);
static_assert(sizeof(Config) == 104);
static_assert(sizeof(Attribute) == 112);
static_assert(sizeof(FrameData) == 584);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "Camera: %s\n", message);
        std::abort();
    }
}

}

int main() {
    Require(sceCameraIsAttached(0) == 0, "camera 0 must be reported as not attached");
    Require(sceCameraIsAttached(1) == ErrorParam, "camera index 1 must be rejected");
    Require(sceCameraIsAttached(-1) == ErrorParam, "a negative camera index must be rejected");

    StartParameter start{sizeof(StartParameter), {1, 0}, nullptr};
    Config config{};
    config.sizeThis = sizeof(Config);
    Attribute attribute{sizeof(Attribute), 1, {}};
    DeviceInfo info{sizeof(DeviceInfo), 1, 0, 0};
    FrameData frame{};
    frame.sizeThis = sizeof(FrameData);

    Require(sceCameraClose(1) == ErrorNotOpen, "close before open must report not open");
    Require(sceCameraClose(0) == ErrorParam, "handle 0 must be rejected first");
    Require(sceCameraStart(1, &start) == ErrorNotOpen, "start before open must report not open");
    Require(sceCameraStop(1) == ErrorNotOpen, "stop before open must report not open");
    Require(sceCameraGetConfig(1, &config) == ErrorNotOpen, "get config before open must report not open");
    Require(sceCameraSetConfig(1, &config) == ErrorNotOpen, "set config before open must report not open");
    Require(sceCameraSetAttribute(1, &attribute) == ErrorNotOpen, "set attribute before open must report not open");
    Require(sceCameraGetDeviceInfo(0, &info) == ErrorNotInit, "device info before open must report not init");
    Require(sceCameraIsValidFrameData(1, &frame) == ErrorNotOpen, "frame check before open must report not open");

    Require(sceCameraOpen(1, 0, 0, nullptr) == ErrorParam, "only the system user may open the camera");
    Require(sceCameraOpen(UserIdSystem, 1, 0, nullptr) == ErrorParam, "type 1 must be rejected");
    Require(sceCameraOpen(UserIdSystem, 0, 1, nullptr) == ErrorParam, "index 1 must be rejected");
    const int handle = sceCameraOpen(UserIdSystem, 0, 0, nullptr);
    Require(handle == 1, "the first open must return handle 1");

    Require(sceCameraStart(0, &start) == ErrorParam, "start must reject handle 0");
    Require(sceCameraStart(handle, nullptr) == ErrorParam, "start must reject a null parameter");
    StartParameter badSize = start;
    badSize.sizeThis = 16;
    Require(sceCameraStart(handle, &badSize) == ErrorParam, "start must check the parameter size");
    StartParameter noFormat{sizeof(StartParameter), {0, 0}, nullptr};
    Require(sceCameraStart(handle, &noFormat) == ErrorFormatUnknown, "start needs a format level");
    StartParameter bigFormat{sizeof(StartParameter), {0x10, 1}, nullptr};
    Require(sceCameraStart(handle, &bigFormat) == ErrorFormatUnknown, "start must reject format levels above 0xF");
    Require(sceCameraStart(handle, &start) == ErrorNotConnected, "start must report that no camera is connected");
    Require(sceCameraStop(0) == ErrorParam, "stop must reject handle 0");
    Require(sceCameraStop(handle) == 0, "stop must succeed on an open handle");

    Config badConfig{};
    badConfig.sizeThis = 8;
    Require(sceCameraGetConfig(handle, &badConfig) == ErrorParam, "get config must check the size");
    Require(sceCameraGetConfig(handle, nullptr) == ErrorParam, "get config must reject null");
    std::memset(config.configExtension, 0xA5, sizeof(config.configExtension));
    Require(sceCameraGetConfig(handle, &config) == 0, "get config must succeed on an open handle");
    Require(config.sizeThis == sizeof(Config) && config.configType == 1, "the default config is type 1");
    for (const ConfigExtension& extension : config.configExtension) {
        Require(extension.formatLevel[0] == 0 && extension.formatLevel[1] == 4 && extension.formatLevel[2] == 4 &&
                    extension.formatLevel[3] == 4,
                "the default formats are YUV422 then Y8");
        Require(extension.resolution == 0 && extension.framerate == 60, "the default is 1280x800 at 60 Hz");
        Require(extension.width == 0 && extension.height == 0 && extension.reserved1 == 0 && extension.baseOption == nullptr,
                "the default extension clears the rest");
    }
    Require(sceCameraSetConfig(handle, &badConfig) == ErrorParam, "set config must check the size");
    Require(sceCameraSetConfig(handle, &config) == ErrorNotConnected, "set config must report no camera");

    Attribute badChannel{sizeof(Attribute), 4, {}};
    Require(sceCameraSetAttribute(handle, &badChannel) == ErrorParam, "set attribute must reject channel 4");
    badChannel.channel = 0;
    Require(sceCameraSetAttribute(handle, &badChannel) == ErrorParam, "set attribute must reject channel 0");
    Require(sceCameraSetAttribute(handle, nullptr) == ErrorParam, "set attribute must reject null");
    attribute.channel = 3;
    Require(sceCameraSetAttribute(handle, &attribute) == ErrorNotConnected, "set attribute must report no camera");

    DeviceInfo badRevision{sizeof(DeviceInfo), 2, 0, 0};
    Require(sceCameraGetDeviceInfo(0, &badRevision) == ErrorParam, "device info needs revision 1");
    Require(sceCameraGetDeviceInfo(1, &info) == ErrorParam, "device info needs reserved 0");
    Require(sceCameraGetDeviceInfo(0, nullptr) == ErrorParam, "device info must reject null");
    Require(sceCameraGetDeviceInfo(0, &info) == ErrorNotConnected, "device info must report no camera");

    Require(sceCameraGetFrameData(handle, nullptr) == ErrorParam, "frame data must reject null");
    frame.status[0] = 1;
    frame.status[1] = 1;
    Require(sceCameraIsValidFrameData(handle, &frame) == 1, "frame check reports valid statuses");
    Require(sceCameraGetFrameData(0, &frame) == ErrorParam, "frame data must reject handle 0");
    Require(frame.status[0] == 0xFFFFFFFFu && frame.status[1] == 0xFFFFFFFFu, "frame data clears the statuses first");
    frame.status[0] = 1;
    frame.sizeThis = sizeof(FrameData) + 8;
    Require(sceCameraGetFrameData(handle, &frame) == ErrorParam, "frame data must reject an oversized record");
    Require(sceCameraIsValidFrameData(handle, &frame) == ErrorParam, "frame check must reject an oversized record");
    frame.sizeThis = sizeof(FrameData);
    Require(sceCameraGetFrameData(handle, &frame) == ErrorNotOpen, "no frame arrives while the camera is not started");
    Require(frame.status[0] == 0xFFFFFFFFu && frame.status[1] == 0xFFFFFFFFu, "no frame is valid");
    Require(sceCameraIsValidFrameData(handle, &frame) == 0, "frame check reports the invalid frame");
    Require(sceCameraIsValidFrameData(0, &frame) == ErrorParam, "frame check must reject handle 0");

    Require(sceCameraOpen(UserIdSystem, 0, 0, nullptr) == 2, "a second open counts up");
    Require(sceCameraClose(2) == 0, "closing one of two handles succeeds");
    Require(sceCameraStop(handle) == 0, "the library stays open while a handle remains");
    Require(sceCameraClose(handle) == 0, "closing the last handle succeeds");
    Require(sceCameraClose(handle) == ErrorNotOpen, "a further close reports not open");
    Require(sceCameraGetDeviceInfo(0, &info) == ErrorNotInit, "device info after the last close reports not init");
    return 0;
}
