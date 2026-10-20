#ifndef CORE_LIBS_PRX_LIBSCEHMD_HMD_HPP
#define CORE_LIBS_PRX_LIBSCEHMD_HMD_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>

namespace Hmd {

inline constexpr std::int32_t AlreadyInitialized = static_cast<std::int32_t>(0x81110001);
inline constexpr std::int32_t NotInitialized = static_cast<std::int32_t>(0x81110002);
inline constexpr std::int32_t InvalidHandle = static_cast<std::int32_t>(0x81110003);
inline constexpr std::int32_t DeviceDisconnected = static_cast<std::int32_t>(0x81110004);
inline constexpr std::int32_t ParameterNull = static_cast<std::int32_t>(0x81110008);
inline constexpr std::int32_t ParameterInvalid = static_cast<std::int32_t>(0x81110009);
inline constexpr std::int32_t UnsupportedFeature = static_cast<std::int32_t>(0x81110016);

enum class DeviceStatus : std::uint32_t {
    Ready = 0,
    NotReady = 1,
    NotDetected = 2,
    HmuDisconnected = 3,
};

struct InitializeParam {
    void* reserved0;
    std::uint8_t reserved[8];
};

struct OpenParam;

struct DeviceInformation {
    DeviceStatus status;
    std::int32_t userId;
    std::uint8_t reserved0[4];
    std::uint32_t panelWidth;
    std::uint32_t panelHeight;
    std::uint16_t latency90Hz;
    std::uint16_t latency120Hz;
    std::uint8_t hmuMount;
    std::uint8_t reserved1[7];
};

struct FieldOfView {
    float tanOut;
    float tanIn;
    float tanTop;
    float tanBottom;
};

struct EyeOffset {
    float offsetX;
    float offsetY;
    float offsetZ;
    std::uint8_t reserved[20];
};

static_assert(sizeof(InitializeParam) == 16);
static_assert(sizeof(DeviceInformation) == 32);
static_assert(offsetof(DeviceInformation, userId) == 4);
static_assert(offsetof(DeviceInformation, panelWidth) == 12);
static_assert(offsetof(DeviceInformation, latency90Hz) == 20);
static_assert(offsetof(DeviceInformation, hmuMount) == 24);
static_assert(sizeof(FieldOfView) == 16);
static_assert(sizeof(EyeOffset) == 32);

}

extern "C" {
std::int32_t APS5_VABI sceHmdInitialize(const Hmd::InitializeParam* param);
std::int32_t APS5_VABI sceHmdInitialize315(const Hmd::InitializeParam* param);
std::int32_t APS5_VABI sceHmdTerminate();
std::int32_t APS5_VABI sceHmdOpen(std::int32_t userId, std::int32_t type, std::int32_t index,
                                Hmd::OpenParam* param);
std::int32_t APS5_VABI sceHmdClose(std::int32_t handle);
std::int32_t APS5_VABI sceHmdGetDeviceInformation(Hmd::DeviceInformation* info);
std::int32_t APS5_VABI sceHmdGetDeviceInformationByHandle(std::int32_t handle,
                                                        Hmd::DeviceInformation* info);
std::int32_t APS5_VABI sceHmdInternalGetDeviceStatus(Hmd::DeviceStatus* status);
std::int32_t APS5_VABI sceHmdInternalMmapIsConnect();
std::int32_t APS5_VABI sceHmdGet2DEyeOffset(std::int32_t handle, Hmd::EyeOffset* leftOffset,
                                          Hmd::EyeOffset* rightOffset);
std::int32_t APS5_VABI sceHmdGetFieldOfView(std::int32_t handle, Hmd::FieldOfView* fieldOfView);
std::int32_t APS5_VABI sceHmdGetInertialSensorData(std::int32_t handle, void* data, std::int32_t unknown);
std::int32_t APS5_VABI sceHmdGetAssyError(void* data);
std::uint32_t APS5_VABI sceHmdReprojectionQueryGarlicBuffAlign();
std::size_t APS5_VABI sceHmdReprojectionQueryGarlicBuffSize();
std::uint32_t APS5_VABI sceHmdReprojectionQueryOnionBuffAlign();
std::size_t APS5_VABI sceHmdReprojectionQueryOnionBuffSize();
std::int32_t APS5_VABI sceHmdDistortionGetWorkMemoryAlign();
std::int32_t APS5_VABI sceHmdDistortionGetWorkMemorySize();
std::int32_t APS5_VABI sceHmdGetDistortionWorkMemoryAlign();
std::int32_t APS5_VABI sceHmdGetDistortionWorkMemorySize();
std::int32_t APS5_VABI sceHmdDistortionGetCompoundEyeCorrectionCommand();
}

#endif
