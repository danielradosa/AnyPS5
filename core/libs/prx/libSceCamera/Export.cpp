#include <cstdint>
#include <cstddef>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

struct CameraStartParameter {
    uint32_t sizeThis;
    uint32_t formatLevel[2];
    void* startOption;
};

struct CameraConfigExtension {
    uint32_t formatLevel[4];
    uint32_t resolution;
    uint32_t framerate;
    uint32_t width;
    uint32_t height;
    uint32_t reserved1;
    void* baseOption;
};

struct CameraConfig {
    uint32_t sizeThis;
    uint32_t configType;
    CameraConfigExtension configExtension[2];
};

struct CameraAttribute {
    uint32_t sizeThis;
    int32_t channel;
    uint8_t settings[104];
};

struct CameraDeviceInfo {
    uint32_t sizeThis;
    uint32_t infoRevision;
    uint32_t deviceRevision;
    uint32_t padding;
};

struct CameraFrameData {
    uint32_t sizeThis;
    uint32_t readMode;
    uint8_t framePosition[2][4][16];
    void* framePointerList[2][4];
    uint32_t frameSize[2][4];
    uint32_t status[2];
};

static_assert(sizeof(CameraStartParameter) == 24);
static_assert(sizeof(CameraConfigExtension) == 48);
static_assert(sizeof(CameraConfig) == 104);
static_assert(sizeof(CameraAttribute) == 112);
static_assert(sizeof(CameraDeviceInfo) == 16);
static_assert(offsetof(CameraFrameData, status) == 232);

namespace {

constexpr int SCE_CAMERA_ERROR_PARAM = static_cast<int>(0x802E0000);
constexpr int SCE_CAMERA_ERROR_NOT_INIT = static_cast<int>(0x802E0002);
constexpr int SCE_CAMERA_ERROR_NOT_OPEN = static_cast<int>(0x802E0004);
constexpr int SCE_CAMERA_ERROR_FORMAT_UNKNOWN = static_cast<int>(0x802E0007);
constexpr int SCE_CAMERA_ERROR_NOT_CONNECTED = static_cast<int>(0x802E0010);
constexpr int32_t SCE_USER_SERVICE_USER_ID_SYSTEM = 0xFF;
constexpr int32_t CAMERA_CHANNEL_0 = 1;
constexpr int32_t CAMERA_CHANNEL_BOTH = 3;
constexpr uint32_t CAMERA_CONFIG_TYPE1 = 1;
constexpr uint32_t CAMERA_FORMAT_YUV422 = 0;
constexpr uint32_t CAMERA_SCALE_FORMAT_Y8 = 4;
constexpr uint32_t CAMERA_RESOLUTION_1280X800 = 0;
constexpr uint32_t CAMERA_FRAMERATE_60 = 60;
constexpr uint32_t CAMERA_FORMAT_LEVEL_MAX = 0xF;
constexpr uint32_t CAMERA_DEVICE_INFO_REVISION = 1;
constexpr uint32_t CAMERA_FRAME_STATUS_INVALID = 0xFFFFFFFFu;
constexpr uint32_t CAMERA_FRAME_STATUS_VALID = 1;
constexpr uint32_t CAMERA_FRAME_DATA_MAX_SIZE = 584;

std::mutex stateMutex;
int32_t openCount = 0;

bool isOpen() {
    std::lock_guard lock(stateMutex);
    return openCount != 0;
}

}

extern "C" {

int APS5_VABI sceCameraIsAttached(int32_t index) {
    if (index != 0) return SCE_CAMERA_ERROR_PARAM;
    return 0;
}

int APS5_VABI sceCameraOpen(int32_t userId, int32_t type, int32_t index, void* param) {
    (void)param;
    if (userId != SCE_USER_SERVICE_USER_ID_SYSTEM || type != 0 || index != 0) return SCE_CAMERA_ERROR_PARAM;
    std::lock_guard lock(stateMutex);
    return ++openCount;
}

int APS5_VABI sceCameraClose(int32_t handle) {
    if (handle < 1) return SCE_CAMERA_ERROR_PARAM;
    std::lock_guard lock(stateMutex);
    if (openCount == 0) return SCE_CAMERA_ERROR_NOT_OPEN;
    --openCount;
    return 0;
}

int APS5_VABI sceCameraStart(int32_t handle, const CameraStartParameter* param) {
    if (handle < 1 || param == nullptr || param->sizeThis != sizeof(CameraStartParameter)) return SCE_CAMERA_ERROR_PARAM;
    if (!isOpen()) return SCE_CAMERA_ERROR_NOT_OPEN;
    if (param->formatLevel[0] > CAMERA_FORMAT_LEVEL_MAX || param->formatLevel[1] > CAMERA_FORMAT_LEVEL_MAX ||
        (param->formatLevel[0] | param->formatLevel[1]) == 0) {
        return SCE_CAMERA_ERROR_FORMAT_UNKNOWN;
    }
    return SCE_CAMERA_ERROR_NOT_CONNECTED;
}

int APS5_VABI sceCameraStop(int32_t handle) {
    if (handle < 1) return SCE_CAMERA_ERROR_PARAM;
    if (!isOpen()) return SCE_CAMERA_ERROR_NOT_OPEN;
    return 0;
}

int APS5_VABI sceCameraGetConfig(int32_t handle, CameraConfig* config) {
    if (handle < 1 || config == nullptr || config->sizeThis != sizeof(CameraConfig)) return SCE_CAMERA_ERROR_PARAM;
    if (!isOpen()) return SCE_CAMERA_ERROR_NOT_OPEN;
    config->configType = CAMERA_CONFIG_TYPE1;
    for (CameraConfigExtension& extension : config->configExtension) {
        extension.formatLevel[0] = CAMERA_FORMAT_YUV422;
        extension.formatLevel[1] = CAMERA_SCALE_FORMAT_Y8;
        extension.formatLevel[2] = CAMERA_SCALE_FORMAT_Y8;
        extension.formatLevel[3] = CAMERA_SCALE_FORMAT_Y8;
        extension.resolution = CAMERA_RESOLUTION_1280X800;
        extension.framerate = CAMERA_FRAMERATE_60;
        extension.width = 0;
        extension.height = 0;
        extension.reserved1 = 0;
        extension.baseOption = nullptr;
    }
    return 0;
}

int APS5_VABI sceCameraSetConfig(int32_t handle, const CameraConfig* config) {
    if (handle < 1 || config == nullptr || config->sizeThis != sizeof(CameraConfig)) return SCE_CAMERA_ERROR_PARAM;
    if (!isOpen()) return SCE_CAMERA_ERROR_NOT_OPEN;
    return SCE_CAMERA_ERROR_NOT_CONNECTED;
}

int APS5_VABI sceCameraSetAttribute(int32_t handle, const CameraAttribute* attribute) {
    if (handle < 1 || attribute == nullptr || attribute->sizeThis != sizeof(CameraAttribute) ||
        attribute->channel < CAMERA_CHANNEL_0 || attribute->channel > CAMERA_CHANNEL_BOTH) {
        return SCE_CAMERA_ERROR_PARAM;
    }
    if (!isOpen()) return SCE_CAMERA_ERROR_NOT_OPEN;
    return SCE_CAMERA_ERROR_NOT_CONNECTED;
}

int APS5_VABI sceCameraGetDeviceInfo(int32_t reserved, const CameraDeviceInfo* deviceInfo) {
    if (reserved != 0 || deviceInfo == nullptr || deviceInfo->sizeThis != sizeof(CameraDeviceInfo) ||
        deviceInfo->infoRevision != CAMERA_DEVICE_INFO_REVISION) {
        return SCE_CAMERA_ERROR_PARAM;
    }
    if (!isOpen()) return SCE_CAMERA_ERROR_NOT_INIT;
    return SCE_CAMERA_ERROR_NOT_CONNECTED;
}

int APS5_VABI sceCameraGetFrameData(int32_t handle, CameraFrameData* frameData) {
    if (frameData == nullptr) return SCE_CAMERA_ERROR_PARAM;
    frameData->status[0] = CAMERA_FRAME_STATUS_INVALID;
    frameData->status[1] = CAMERA_FRAME_STATUS_INVALID;
    if (handle < 1 || frameData->sizeThis > CAMERA_FRAME_DATA_MAX_SIZE) return SCE_CAMERA_ERROR_PARAM;
    return SCE_CAMERA_ERROR_NOT_OPEN;
}

int APS5_VABI sceCameraIsValidFrameData(int32_t handle, const CameraFrameData* frameData) {
    if (handle < 1 || frameData == nullptr || frameData->sizeThis > CAMERA_FRAME_DATA_MAX_SIZE) return SCE_CAMERA_ERROR_PARAM;
    if (!isOpen()) return SCE_CAMERA_ERROR_NOT_OPEN;
    return frameData->status[0] == CAMERA_FRAME_STATUS_VALID && frameData->status[1] == CAMERA_FRAME_STATUS_VALID ? 1 : 0;
}

}
