#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

struct BluetoothHidEvent;
struct BluetoothHidDeviceInfo;
struct BluetoothHidDeviceName;
struct BluetoothHidThreadParam {
    std::int32_t priority;
    std::uint32_t padding;
    std::uint64_t affinityMask;
    std::uint32_t reserved;
    std::uint32_t tailPadding;
};
struct BluetoothHidParam {
    const BluetoothHidThreadParam* threadParam;
    std::uint32_t reserved;
    std::uint32_t padding;
};
using BluetoothHidCallback = void (APS5_VABI *)(const BluetoothHidEvent*, std::int32_t, void*);

extern "C" {
std::int32_t APS5_VABI sceBluetoothHidInit();
std::int32_t APS5_VABI sceBluetoothHidRegisterCallback(BluetoothHidCallback callback, void* userData, const BluetoothHidParam* param);
std::int32_t APS5_VABI sceBluetoothHidUnregisterCallback();
std::int32_t APS5_VABI sceBluetoothHidRegisterDevice(std::uint16_t vendorId, std::uint16_t productId);
std::int32_t APS5_VABI sceBluetoothHidUnregisterDevice(std::uint16_t vendorId, std::uint16_t productId);
int APS5_VABI sceBluetoothHidGetDeviceName(std::uint64_t, BluetoothHidDeviceName*);
int APS5_VABI sceBluetoothHidGetInputReport(std::uint64_t, std::uint8_t);
int APS5_VABI sceBluetoothHidSetOutputReport(std::uint64_t, std::uint8_t, const void*, std::size_t);
int APS5_VABI sceBluetoothHidGetDeviceInfo(std::int32_t, BluetoothHidDeviceInfo*);
int APS5_VABI sceBluetoothHidDisconnectDevice(std::int32_t);
int APS5_VABI sceBluetoothHidGetFeatureReport(std::uint64_t, std::uint8_t);
int APS5_VABI sceBluetoothHidSetFeatureReport(std::uint64_t, std::uint8_t, const void*, std::size_t);
int APS5_VABI sceBluetoothHidGetReportDescriptor(std::uint64_t, void*, std::size_t);
int APS5_VABI sceBluetoothHidInterruptOutput(std::uint64_t, std::uint8_t, const void*, std::size_t);
int APS5_VABI sceBluetoothHidParamInitialize(BluetoothHidParam*, const BluetoothHidThreadParam*);
int APS5_VABI sceBluetoothHidDebugGetVersion();
int APS5_VABI sceBluetoothHidThreadParamInitialize(BluetoothHidThreadParam*, std::int32_t, std::uint64_t);
}

namespace {

int callbackCalls = 0;

void APS5_VABI Callback(const BluetoothHidEvent*, std::int32_t, void*) {
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
    constexpr auto notInitialized = static_cast<std::int32_t>(0x81540004u);
    constexpr auto invalidArgument = static_cast<std::int32_t>(0x81540001u);
    constexpr auto noCallback = static_cast<std::int32_t>(0x81540006u);
    Require(sceBluetoothHidRegisterCallback(nullptr, nullptr, nullptr) == notInitialized, "initialization check order");
    Require(sceBluetoothHidRegisterDevice(1, 2) == notInitialized, "registration before initialization");
    Require(sceBluetoothHidUnregisterDevice(1, 2) == notInitialized, "unregistration before initialization");
    Require(sceBluetoothHidUnregisterCallback() == notInitialized, "callback removal before initialization");
    Require(sceBluetoothHidInit() == 0, "initialization failed");
    Require(sceBluetoothHidInit() == 0, "repeated initialization failed");
    Require(sceBluetoothHidRegisterDevice(1, 2) == noCallback, "registration without callback");
    Require(sceBluetoothHidUnregisterCallback() == noCallback, "removing absent callback");
    BluetoothHidParam param;
    std::memset(&param, 0xcd, sizeof(param));
    const BluetoothHidThreadParam initialThreadParam{};
    Require(sceBluetoothHidParamInitialize(&param, &initialThreadParam) == 0, "param initialization failed");
    Require(param.threadParam == &initialThreadParam && param.reserved == 0, "param initialization did not store the thread param and clear the word after it");
    Require(param.padding == 0xcdcdcdcdu, "param initialization wrote past twelve bytes");
    bool threwParam = false;
    try {
        sceBluetoothHidParamInitialize(nullptr, nullptr);
    } catch (const std::invalid_argument&) {
        threwParam = true;
    }
    Require(threwParam, "a null param was accepted");
    Require(sceBluetoothHidParamInitialize(&param, nullptr) == 0, "param initialization failed");
    Require(sceBluetoothHidRegisterCallback(&Callback, &callbackCalls, &param) == 0, "callback registration failed");
    Require(sceBluetoothHidRegisterDevice(0x44f, 0x1000) == 0, "device registration failed");
    Require(callbackCalls == 0, "a device connection was reported");
    Require(sceBluetoothHidRegisterCallback(nullptr, nullptr, &param) == invalidArgument, "null callback validation");
    param.reserved = 1;
    Require(sceBluetoothHidRegisterCallback(&Callback, nullptr, &param) == invalidArgument, "reserved field validation");
    param.reserved = 0;
    BluetoothHidThreadParam threadParam{};
    threadParam.reserved = 1;
    param.threadParam = &threadParam;
    Require(sceBluetoothHidRegisterCallback(&Callback, nullptr, &param) == invalidArgument, "thread reserved field validation");
    Require(sceBluetoothHidRegisterCallback(&Callback, nullptr, nullptr) == 0, "optional parameters rejected");
    std::uint8_t buffer[64]{};
    RequireRuntimeError([] { sceBluetoothHidGetInputReport(1, 1); }, "an input report without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidGetDeviceName(1, reinterpret_cast<BluetoothHidDeviceName*>(buffer)); }, "a device name without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidSetOutputReport(1, 1, buffer, sizeof(buffer)); }, "an output report without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidGetDeviceInfo(1, reinterpret_cast<BluetoothHidDeviceInfo*>(buffer)); }, "device info without a device did not throw");
    RequireRuntimeError([] { sceBluetoothHidDisconnectDevice(1); }, "a disconnect without a device did not throw");
    RequireRuntimeError([] { sceBluetoothHidGetFeatureReport(1, 1); }, "a feature report read without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidSetFeatureReport(1, 1, buffer, sizeof(buffer)); }, "a feature report write without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidGetReportDescriptor(1, buffer, sizeof(buffer)); }, "a report descriptor without a device did not throw");
    RequireRuntimeError([&] { sceBluetoothHidInterruptOutput(1, 1, buffer, sizeof(buffer)); }, "an interrupt output without a device did not throw");
    for (const std::uint8_t byte : buffer) Require(byte == 0, "a call without a device wrote the caller's buffer");
    Require(sceBluetoothHidDebugGetVersion() == 0x1e, "debug get version is not the firmware's 0x1e");
    RequireRuntimeError([] {
        BluetoothHidThreadParam blankThreadParam{};
        sceBluetoothHidThreadParamInitialize(&blankThreadParam, 0, 0);
    }, "thread param initialization did not throw");
    Require(sceBluetoothHidUnregisterDevice(0x44f, 0x1000) == 0, "device unregistration failed");
    Require(sceBluetoothHidUnregisterCallback() == 0, "callback unregistration failed");
    std::puts("BluetoothHid tests passed");
    return 0;
}
