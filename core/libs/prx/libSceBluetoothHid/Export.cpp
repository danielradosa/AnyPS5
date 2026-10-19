#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

#include <string>

extern "C" {

std::int32_t APS5_VABI sceBluetoothHidInit() {
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidRegisterCallback(void* callback, std::int32_t flags, void* param) {
    (void)flags;
    (void)param;
    if (callback == nullptr) throw std::invalid_argument(std::string(__func__) + ": null callback");
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidUnregisterCallback() {
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidRegisterDevice(std::uint16_t vendorId, std::uint16_t productId) {
    (void)vendorId;
    (void)productId;
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidUnregisterDevice(std::uint16_t vendorId, std::uint16_t productId) {
    (void)vendorId;
    (void)productId;
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidGetDeviceName(std::uint64_t handle, void* name) {
    (void)handle;
    (void)name;
    throw std::runtime_error(std::string(__func__) + ": no Bluetooth HID device is connected");
}

std::int32_t APS5_VABI sceBluetoothHidGetInputReport(void* report, std::uint32_t reportId) {
    (void)report;
    (void)reportId;
    throw std::runtime_error(std::string(__func__) + ": no Bluetooth HID device is connected");
}

std::int32_t APS5_VABI sceBluetoothHidSetOutputReport(std::uint64_t handle, std::uint32_t reportId, void* data, std::uint64_t length) {
    (void)handle;
    (void)reportId;
    (void)data;
    (void)length;
    throw std::runtime_error(std::string(__func__) + ": no Bluetooth HID device is connected");
}

}
