#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
std::int32_t APS5_VABI sceBluetoothHidInit();
std::int32_t APS5_VABI sceBluetoothHidRegisterCallback(void* callback, std::int32_t flags, void* param);
std::int32_t APS5_VABI sceBluetoothHidUnregisterCallback();
std::int32_t APS5_VABI sceBluetoothHidRegisterDevice(std::uint16_t vendorId, std::uint16_t productId);
std::int32_t APS5_VABI sceBluetoothHidUnregisterDevice(std::uint16_t vendorId, std::uint16_t productId);
std::int32_t APS5_VABI sceBluetoothHidGetDeviceName(std::uint64_t handle, void* name);
std::int32_t APS5_VABI sceBluetoothHidGetInputReport(void* report, std::uint32_t reportId);
std::int32_t APS5_VABI sceBluetoothHidSetOutputReport(std::uint64_t handle, std::uint32_t reportId, void* data, std::uint64_t length);
std::int32_t APS5_VABI sceBluetoothHidGetDeviceInfo(std::uint32_t deviceId, void* info);
std::int32_t APS5_VABI sceBluetoothHidDisconnectDevice(std::uint32_t deviceId);
std::int32_t APS5_VABI sceBluetoothHidGetFeatureReport(void* report, std::uint32_t reportId);
std::int32_t APS5_VABI sceBluetoothHidSetFeatureReport(std::uint64_t handle, std::uint32_t reportId, void* data, std::uint64_t length);
std::int32_t APS5_VABI sceBluetoothHidGetReportDescriptor(std::uint64_t handle, void* buffer, std::uint64_t length);
std::int32_t APS5_VABI sceBluetoothHidInterruptOutput(std::uint64_t handle, std::uint32_t reportId, void* data, std::uint64_t length);
std::int32_t APS5_VABI sceBluetoothHidParamInitialize(void* param, std::uint64_t value);
int APS5_VABI sceBluetoothHidDebugGetVersion();
int APS5_VABI sceBluetoothHidThreadParamInitialize();
}

namespace {

int callbackCalls = 0;

void APS5_VABI Callback() {
    ++callbackCalls;
}

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "BluetoothHid: %s\n", message);
        std::abort();
    }
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

}

int main() {
    Require(sceBluetoothHidInit() == 0, "initialization failed");
    std::uint8_t param[16];
    std::memset(param, 0xcd, sizeof(param));
    Require(sceBluetoothHidParamInitialize(param, 0x1122334455667788ull) == 0, "param initialization failed");
    std::uint64_t value = 0;
    std::uint32_t cleared = 1;
    std::memcpy(&value, param, sizeof(value));
    std::memcpy(&cleared, param + 8, sizeof(cleared));
    Require(value == 0x1122334455667788ull && cleared == 0, "param initialization did not store the value and clear the word after it");
    Require(param[12] == 0xcd && param[13] == 0xcd && param[14] == 0xcd && param[15] == 0xcd, "param initialization wrote past twelve bytes");
    bool threwParam = false;
    try {
        sceBluetoothHidParamInitialize(nullptr, 0);
    } catch (const std::invalid_argument&) {
        threwParam = true;
    }
    Require(threwParam, "a null param was accepted");
    Require(sceBluetoothHidParamInitialize(param, 0) == 0, "param initialization failed");
    Require(sceBluetoothHidRegisterCallback(reinterpret_cast<void*>(&Callback), 0, param) == 0, "callback registration failed");
    Require(sceBluetoothHidRegisterDevice(0x44f, 0x1000) == 0, "device registration failed");
    Require(callbackCalls == 0, "a device connection was reported");
    bool threw = false;
    try {
        sceBluetoothHidRegisterCallback(nullptr, 0, param);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw, "a null callback was accepted");
    std::uint8_t buffer[64]{};
    RequireRuntimeError([&] { sceBluetoothHidGetInputReport(buffer, 1); }, "an input report without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidGetDeviceName(1, buffer); }, "a device name without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidSetOutputReport(1, 1, buffer, sizeof(buffer)); }, "an output report without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidGetDeviceInfo(1, buffer); }, "device info without a device did not throw");
    RequireRuntimeError([] { sceBluetoothHidDisconnectDevice(1); }, "a disconnect without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidGetFeatureReport(buffer, 1); }, "a feature report read without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidSetFeatureReport(1, 1, buffer, sizeof(buffer)); }, "a feature report write without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidGetReportDescriptor(1, buffer, sizeof(buffer)); }, "a report descriptor without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidInterruptOutput(1, 1, buffer, sizeof(buffer)); }, "an interrupt output without a device did not throw");
    for (const std::uint8_t byte : buffer) Require(byte == 0, "a call without a device wrote the caller's buffer");
    RequireRuntimeError([] { sceBluetoothHidDebugGetVersion(); }, "debug get version did not throw");
    RequireRuntimeError([] { sceBluetoothHidThreadParamInitialize(); }, "thread param initialization did not throw");
    Require(sceBluetoothHidUnregisterDevice(0x44f, 0x1000) == 0, "device unregistration failed");
    Require(sceBluetoothHidUnregisterCallback() == 0, "callback unregistration failed");
    std::puts("BluetoothHid tests passed");
    return 0;
}
