#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" {
std::int32_t APS5_VABI sceVrTracker2QueryMemory(const void* param, void* result);
std::int32_t APS5_VABI sceVrTracker2Initialize(const void* param);
std::int32_t APS5_VABI sceVrTracker2Finalize();
std::int32_t APS5_VABI sceVrTracker2RegisterDevice(std::int32_t type, std::int32_t handle);
std::int32_t APS5_VABI sceVrTracker2UnregisterDevice(std::int32_t handle);
std::int32_t APS5_VABI sceVrTracker2GetResult(const void* param, void* result);
std::int32_t APS5_VABI sceVrTracker2ResetLocalCoordinate();
std::int32_t APS5_VABI sceVrTracker2LocateCoordinateSystem();
std::int32_t APS5_VABI sceVrTracker2GetPlayAreaOrientedBoundingBox();
}

namespace {

constexpr auto NotInit = static_cast<std::int32_t>(0x8A740001);
constexpr auto AlreadyInitialized = static_cast<std::int32_t>(0x8A740002);
constexpr auto DeviceNotRegistered = static_cast<std::int32_t>(0x8A740003);
constexpr auto ArgumentInvalid = static_cast<std::int32_t>(0x8A740006);
constexpr auto InvalidDeviceHandle = static_cast<std::int32_t>(0x8A740007);
constexpr auto NotSupported = static_cast<std::int32_t>(0x8A74001E);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "VRTRACKER2: %s\n", message);
        std::abort();
    }
}

template <std::size_t Size>
void Put32(std::uint8_t (&buffer)[Size], std::size_t offset, std::uint32_t value) {
    std::memcpy(buffer + offset, &value, sizeof(value));
}

void CheckQueryMemory() {
    std::uint8_t param[0x20]{};
    std::uint32_t result[8];
    std::memset(result, 0xa5, sizeof(result));
    std::uint32_t untouched[8];
    std::memcpy(untouched, result, sizeof(result));

    Require(sceVrTracker2QueryMemory(nullptr, result) == ArgumentInvalid, "null query param");
    Put32(param, 0, 0x20);
    Require(sceVrTracker2QueryMemory(param, nullptr) == ArgumentInvalid, "null query result");
    Put32(param, 0, 0x1c);
    Require(sceVrTracker2QueryMemory(param, result) == ArgumentInvalid, "wrong query param size");
    Put32(param, 0, 0x20);
    Put32(param, 4, 1);
    Require(sceVrTracker2QueryMemory(param, result) == ArgumentInvalid, "profile other than 0");
    Put32(param, 4, 0);
    Require(sceVrTracker2QueryMemory(param, result) == NotSupported, "query reported a memory size");
    Require(std::memcmp(result, untouched, sizeof(result)) == 0, "failed query wrote output");
}

void CheckUninitialized(std::uint8_t (&getParam)[0x20], std::uint8_t (&result)[0x400]) {
    Require(sceVrTracker2Finalize() == NotInit, "finalize without initialization");
    Require(sceVrTracker2RegisterDevice(0, 0) == NotInit, "register before initialization");
    Require(sceVrTracker2UnregisterDevice(0) == NotInit, "unregister before initialization");
    Require(sceVrTracker2GetResult(getParam, result) == NotInit, "result before initialization");
    Require(sceVrTracker2GetResult(nullptr, nullptr) == NotInit, "initialization checked after arguments");
    Require(sceVrTracker2ResetLocalCoordinate() == NotInit, "reset before initialization");
    Require(sceVrTracker2LocateCoordinateSystem() == NotInit, "locate before initialization");
    Require(sceVrTracker2GetPlayAreaOrientedBoundingBox() == NotInit, "play area before initialization");
}

}

int main() {
    static std::uint8_t work[0x1000];
    static std::uint8_t result[0x400];
    std::uint8_t getParam[0x20]{};
    Put32(getParam, 0, 0x20);
    Put32(getParam, 4, 1);
    std::uint8_t initParam[0x48]{};
    const auto workAddress = reinterpret_cast<std::uintptr_t>(work);

    CheckQueryMemory();
    CheckUninitialized(getParam, result);

    Require(sceVrTracker2Initialize(nullptr) == ArgumentInvalid, "null initialize param");
    Put32(initParam, 0, 0x40);
    std::memcpy(initParam + 0x20, &workAddress, sizeof(workAddress));
    Put32(initParam, 0x28, sizeof(work));
    Require(sceVrTracker2Initialize(initParam) == ArgumentInvalid, "wrong initialize param size");
    Put32(initParam, 0, 0x48);
    Put32(initParam, 0x28, 0);
    Require(sceVrTracker2Initialize(initParam) == ArgumentInvalid, "empty work memory");
    Put32(initParam, 0x28, sizeof(work));
    std::memset(initParam + 0x20, 0, sizeof(workAddress));
    Require(sceVrTracker2Initialize(initParam) == ArgumentInvalid, "null work memory");
    std::memcpy(initParam + 0x20, &workAddress, sizeof(workAddress));
    CheckUninitialized(getParam, result);

    for (int round = 0; round < 2; ++round) {
        Require(sceVrTracker2Initialize(initParam) == 0, "initialize failed");
        Require(sceVrTracker2Initialize(initParam) == AlreadyInitialized, "duplicate initialize");
        Require(sceVrTracker2Initialize(nullptr) == AlreadyInitialized, "duplicate checked after arguments");
        CheckQueryMemory();

        for (const std::int32_t type : {0, 1, 2}) {
            for (const std::int32_t handle : {0, 1, -1, 0x0f000000}) {
                Require(sceVrTracker2RegisterDevice(type, handle) == InvalidDeviceHandle, "registered an absent device");
            }
        }
        for (const std::int32_t handle : {0, 1, -1, 0x0f000000}) {
            Require(sceVrTracker2UnregisterDevice(handle) == ArgumentInvalid, "unregistered an unknown handle");
        }

        std::memset(result, 0xa5, sizeof(result));
        Require(sceVrTracker2GetResult(nullptr, result) == ArgumentInvalid, "null result param");
        Require(sceVrTracker2GetResult(getParam, nullptr) == ArgumentInvalid, "null result");
        Put32(getParam, 0, 0x18);
        Require(sceVrTracker2GetResult(getParam, result) == ArgumentInvalid, "wrong result param size");
        Put32(getParam, 0, 0x20);
        Require(sceVrTracker2GetResult(getParam, result) == DeviceNotRegistered, "result for an unregistered device");
        for (const auto byte : result) Require(byte == 0xa5, "failed result wrote output");

        Require(sceVrTracker2ResetLocalCoordinate() == DeviceNotRegistered, "reset without a headset");
        Require(sceVrTracker2LocateCoordinateSystem() == DeviceNotRegistered, "locate without a headset");
        Require(sceVrTracker2GetPlayAreaOrientedBoundingBox() == DeviceNotRegistered, "play area without a headset");

        Require(sceVrTracker2Finalize() == 0, "finalize failed");
        Require(sceVrTracker2Finalize() == NotInit, "duplicate finalize");
        CheckUninitialized(getParam, result);
    }
    std::puts("VRTRACKER2 tests passed");
    return 0;
}
