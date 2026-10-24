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

int APS5_VABI sceCameraStopByHandle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetDeviceIDWithoutOpen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetAttribute(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraInitializeRegistryCalibData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetContrast(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2Finalize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetConnectedDeviceAndDriverMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2IsAttached(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetSharpness(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetTrackerMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetContrast(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetCalibDataForEve(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetCalibDataFromDevice(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetContrast(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraIsConnectedForSystem(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetExposureGain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetRegister(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetVideoSyncInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetConfigInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetAttributeInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetWhiteBalance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetMmapConnectedCount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetSaturation(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetDeviceConfig(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraAudioOpen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetAutoExposureGain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetCalibDataFromDeviceForEve(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetWhiteBalance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetVrCaptureInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetCalibDataForEve(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetAttribute(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetLensCorrection(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetCalibData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetProcessFocusByHandle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetConfig(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetExposureGain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetSharpness(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2Initialize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetHue(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraAccGetData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetAttribute(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetCalibrationData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetSaturation(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetHue(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2Stop(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetSaturation(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetAutoWhiteBalance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraAudioClose(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetProductInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetGamma(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetLensCorrection(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraChangeAppModuleState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetFrameData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetContrast(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetSaturation(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetSharpness(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetExposureGain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraStartByHandle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2Open(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2Start(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetLensCorrectionInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraOpenByModuleId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetCalibrationData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraAudioGetData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetHue(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetGamma(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetCalibData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraWaitForEveReady(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetRegistryInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraAudioGetData2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetAppModuleFocus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetDefectivePixelCancellation(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetDeviceID(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetForceActivate(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetWhiteBalance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetFieldOfView(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2IsValidFrameData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetProcessFocus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetGamma(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetConfig(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetWhiteBalance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetDeviceConfigWithoutHandle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraDeviceOpen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetUacModeInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetAutoExposureGain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetRegister(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetGamma(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraRemoveAppModuleFocus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetAutoWhiteBalance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetDefectivePixelCancellation(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2Close(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetVideoSync(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraWaitForLukeReady(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetDefectivePixelCancellationInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2GetSharpness(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetAutoWhiteBalance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetDebugStop(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCamera2SetAutoExposureGain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraIsConfigChangeDone(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetExposureGain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetVideoSync(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraAudioReset(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetAutoExposureGain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraSetAutoWhiteBalance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraGetHue(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCameraCloseByHandle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
}
