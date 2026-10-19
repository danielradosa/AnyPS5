#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI sceMoveInit(void);
int APS5_VABI sceMoveTerm(void);
int APS5_VABI sceMoveOpen(std::int32_t userId, std::int32_t type, std::int32_t index);
int APS5_VABI sceMoveClose(std::int32_t handle);
int APS5_VABI sceMoveGetDeviceInfo(std::int32_t handle, void* info);
int APS5_VABI sceMoveGetExtensionPortInfo(std::int32_t handle, void* data);
int APS5_VABI sceMoveReadStateLatest(std::int32_t handle, void* data);
int APS5_VABI sceMoveReadStateRecent(std::int32_t handle, std::int64_t timestamp, void* data, std::int32_t* count);
int APS5_VABI sceMoveResetLightSphere(std::int32_t handle);
int APS5_VABI sceMoveSetLightSphere(std::int32_t handle, std::uint8_t red, std::uint8_t green, std::uint8_t blue);
int APS5_VABI sceMoveSetVibration(std::int32_t handle, std::uint8_t motor);
}

namespace {

constexpr int noController = 1;
constexpr int notInitialized = static_cast<int>(0x80EE0001u);
constexpr int alreadyInitialized = static_cast<int>(0x80EE0002u);
constexpr int invalidArgument = static_cast<int>(0x80EE0003u);
constexpr std::int32_t user = 0x10000000;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "Move: %s\n", message);
        std::abort();
    }
}

void CheckNotInitialized(std::int32_t handle, void* data, std::int32_t* count) {
    Require(sceMoveTerm() == notInitialized, "term before init is not NOT_INIT");
    Require(sceMoveOpen(user, 0, 0) == notInitialized, "open before init is not NOT_INIT");
    Require(sceMoveReadStateLatest(handle, nullptr) == notInitialized, "latest read with no buffer is not NOT_INIT");
    Require(sceMoveReadStateLatest(handle, data) == notInitialized, "latest read is not NOT_INIT");
    Require(sceMoveReadStateRecent(handle, -1, nullptr, nullptr) == notInitialized, "recent read with bad arguments is not NOT_INIT");
    Require(sceMoveReadStateRecent(handle, 0, data, count) == notInitialized, "recent read is not NOT_INIT");
    Require(sceMoveGetDeviceInfo(handle, nullptr) == notInitialized, "device info with no buffer is not NOT_INIT");
    Require(sceMoveGetDeviceInfo(handle, data) == notInitialized, "device info is not NOT_INIT");
    Require(sceMoveGetExtensionPortInfo(handle, nullptr) == notInitialized, "extension port info with no buffer is not NOT_INIT");
    Require(sceMoveGetExtensionPortInfo(handle, data) == notInitialized, "extension port info is not NOT_INIT");
    Require(sceMoveSetVibration(handle, 0xff) == notInitialized, "set vibration is not NOT_INIT");
    Require(sceMoveSetLightSphere(handle, 0xff, 0x40, 0x00) == notInitialized, "set light sphere is not NOT_INIT");
    Require(sceMoveResetLightSphere(handle) == notInitialized, "reset light sphere is not NOT_INIT");
    Require(sceMoveClose(handle) == notInitialized, "close is not NOT_INIT");
}

}

int main() {
    std::array<std::uint8_t, 0x80> data;
    data.fill(0xa5);
    const auto untouched = data;
    std::int32_t count = 7;
    for (const std::int32_t handle : {0, 1, 0x30b0100, -1}) CheckNotInitialized(handle, data.data(), &count);

    Require(sceMoveInit() == 0, "init failed");
    Require(sceMoveInit() == alreadyInitialized, "second init is not ALREADY_INIT");
    const int first = sceMoveOpen(user, 0, 0);
    const int second = sceMoveOpen(user, 0, 1);
    Require(first == 0x30b0100 && second == 0x30b0200, "open does not hand out handles from 0x30b0100 in steps of 0x100");
    for (const std::int32_t handle : {first, second}) {
        Require(sceMoveGetDeviceInfo(handle, nullptr) == invalidArgument, "device info with no buffer is not INVALID_ARG");
        Require(sceMoveGetDeviceInfo(handle, data.data()) == noController, "device info finds a controller");
        Require(sceMoveReadStateLatest(handle, nullptr) == invalidArgument, "latest read with no buffer is not INVALID_ARG");
        Require(sceMoveReadStateLatest(handle, data.data()) == noController, "latest read finds a controller");
        Require(sceMoveReadStateRecent(handle, -1, data.data(), &count) == invalidArgument, "recent read with a negative timestamp is not INVALID_ARG");
        Require(sceMoveReadStateRecent(handle, 0, nullptr, &count) == invalidArgument, "recent read with no buffer is not INVALID_ARG");
        Require(sceMoveReadStateRecent(handle, 0, data.data(), nullptr) == invalidArgument, "recent read with no count is not INVALID_ARG");
        Require(sceMoveReadStateRecent(handle, 0, data.data(), &count) == noController, "recent read finds a controller");
        Require(sceMoveGetExtensionPortInfo(handle, nullptr) == invalidArgument, "extension port info with no buffer is not INVALID_ARG");
        Require(sceMoveGetExtensionPortInfo(handle, data.data()) == noController, "extension port info finds a controller");
        Require(sceMoveSetVibration(handle, 0xff) == noController, "set vibration finds a controller");
        Require(sceMoveSetLightSphere(handle, 0xff, 0x40, 0x00) == noController, "set light sphere finds a controller");
        Require(sceMoveResetLightSphere(handle) == 0, "reset light sphere failed");
        Require(sceMoveClose(handle) == 0, "close failed");
    }
    Require(data == untouched, "a call wrote the buffer");
    Require(count == 7, "a call wrote the count");

    Require(sceMoveTerm() == 0, "term failed");
    CheckNotInitialized(first, data.data(), &count);
    Require(sceMoveInit() == 0, "init after term failed");
    Require(sceMoveOpen(user, 0, 0) == 0x30b0300, "open after term does not continue the handle sequence");
    Require(sceMoveTerm() == 0, "second term failed");
}
