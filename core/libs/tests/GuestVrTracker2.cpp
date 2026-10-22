#include "prx/libc/include/general/VabiMacros.hpp"
#include "ProcessParametersFixture.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
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
std::int32_t APS5_VABI sceVrTracker2GetCoordinateSystem();
std::int32_t APS5_VABI sceVrTracker2GetPlayAreaBoundaryGeometry();
std::int32_t APS5_VABI sceVrTracker2SetCoordinateSystem();
std::int32_t APS5_VABI VrTracker2NotInitialized();
std::int32_t APS5_VABI VrTracker2ClearWideResult(void* result);
std::int32_t APS5_VABI VrTracker2ClearNarrowResult(void* result);
std::int32_t APS5_VABI VrTracker2CheckSecondThenReportNotInitialized(const void* first, const void* second);
std::int32_t APS5_VABI VrTracker2Accept();
std::int32_t APS5_VABI VrTracker2FillDefaults(void* output);
std::int32_t APS5_VABI VrTracker2ClearWord(void* output);
}

namespace {

alignas(16) std::array<std::byte, 0x40> process{};

constexpr auto NotInit = static_cast<std::int32_t>(0x8A740001);
constexpr auto AlreadyInitialized = static_cast<std::int32_t>(0x8A740002);
constexpr auto DeviceNotRegistered = static_cast<std::int32_t>(0x8A740003);
constexpr auto ArgumentInvalid = static_cast<std::int32_t>(0x8A740006);
constexpr auto InvalidDeviceHandle = static_cast<std::int32_t>(0x8A740007);

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

std::array<std::uint8_t, 32> Filled() {
    std::array<std::uint8_t, 32> buffer{};
    buffer.fill(0xAA);
    return buffer;
}

bool AllEqual(const std::uint8_t* begin, const std::uint8_t* end, std::uint8_t value) {
    for (; begin != end; ++begin)
        if (*begin != value) return false;
    return true;
}

template <typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void CheckQueryMemory() {
    std::array<std::uint8_t, 40> parameter{};
    parameter[0] = 0x20;
    parameter.back() = 0xAA;
    std::array<std::uint8_t, 40> requirement{};
    requirement.fill(0xAA);
    Require(sceVrTracker2QueryMemory(parameter.data(), requirement.data()) == 0, "query return");
    const std::array<std::uint8_t, 12> fields{0x20, 0, 0, 0, 0, 0, 0, 0x02, 0, 0, 0x01, 0};
    Require(std::memcmp(requirement.data(), fields.data(), fields.size()) == 0, "query fields");
    Require(AllEqual(requirement.data() + 12, requirement.data() + 32, 0) && AllEqual(requirement.data() + 32, requirement.data() + 40, 0xAA), "query writes 32 bytes");
    Require(sceVrTracker2QueryMemory(nullptr, requirement.data()) == ArgumentInvalid, "query null parameter");
    Require(sceVrTracker2QueryMemory(parameter.data(), nullptr) == ArgumentInvalid, "query null requirement");
    for (std::size_t offset = 4; offset < 32; ++offset) {
        auto rejected = parameter;
        rejected[offset] = 1;
        requirement.fill(0xAA);
        Require(sceVrTracker2QueryMemory(rejected.data(), requirement.data()) == ArgumentInvalid, "query reserved byte");
        Require(AllEqual(requirement.data(), requirement.data() + 40, 0xAA), "query rejection leaves output");
    }
    parameter[0] = 0x21;
    Require(sceVrTracker2QueryMemory(parameter.data(), requirement.data()) == ArgumentInvalid, "query size");
}

void CheckQueryMemoryModes() {
    std::array<std::uint8_t, 40> parameter{};
    parameter[0] = 0x20;
    std::array<std::uint8_t, 40> requirement{};
    const std::uint64_t processSize = process.size();
    const std::uint32_t magic = 0x4942524f;
    std::memcpy(process.data(), &processSize, sizeof(processSize));
    std::memcpy(process.data() + 8, &magic, sizeof(magic));
    Require(ProcessParametersFixture::Install(process.data(), process.size()), "process parameters");
    for (const std::uint32_t version : {0u, 0xffffffu, 0x1000000u, 0x7000000u}) {
        std::memcpy(process.data() + 0x10, &version, sizeof(version));
        for (const std::uint32_t mode : {1000u, 1001u}) {
            std::memcpy(parameter.data() + 4, &mode, sizeof(mode));
            requirement.fill(0xAA);
            const auto result = sceVrTracker2QueryMemory(parameter.data(), requirement.data());
            if (version < 0x1000000) {
                Require(result == ArgumentInvalid, "query SDK restriction");
                Require(AllEqual(requirement.data(), requirement.data() + requirement.size(), 0xAA), "SDK rejection preserves output");
            } else {
                Require(result == 0, "query alternate mode");
                std::uint32_t memorySize;
                std::memcpy(&memorySize, requirement.data() + 4, sizeof(memorySize));
                Require(memorySize == (mode == 1000 ? 7u << 20 : 3u << 20), "query alternate memory size");
                Require(AllEqual(requirement.data() + 12, requirement.data() + 32, 0), "query alternate reserved fields");
                Require(AllEqual(requirement.data() + 32, requirement.data() + 40, 0xAA), "query alternate output boundary");
            }
        }
    }
}

void CheckNidExportsUninitialized() {
    Require(VrTracker2NotInitialized() == NotInit, "plain export");

    auto wide = Filled();
    Require(VrTracker2ClearWideResult(wide.data()) == NotInit, "wide result return");
    Require(AllEqual(wide.data(), wide.data() + 24, 0) && AllEqual(wide.data() + 24, wide.data() + 32, 0xAA), "wide result clears 24 bytes");
    Require(VrTracker2ClearWideResult(nullptr) == ArgumentInvalid, "wide result null");

    auto narrow = Filled();
    Require(VrTracker2ClearNarrowResult(narrow.data()) == NotInit, "narrow result return");
    Require(AllEqual(narrow.data(), narrow.data() + 16, 0) && AllEqual(narrow.data() + 16, narrow.data() + 32, 0xAA), "narrow result clears 16 bytes");
    Require(VrTracker2ClearNarrowResult(nullptr) == ArgumentInvalid, "narrow result null");

    auto second = Filled();
    Require(VrTracker2CheckSecondThenReportNotInitialized(nullptr, second.data()) == NotInit, "second checked, first null");
    Require(VrTracker2CheckSecondThenReportNotInitialized(second.data(), nullptr) == ArgumentInvalid, "second null");
    Require(AllEqual(second.data(), second.data() + 32, 0xAA), "second untouched");
}

void CheckNidExportsInitialized() {
    auto buffer = Filled();
    Require(Throws([] { VrTracker2NotInitialized(); }), "plain export after initialization");
    Require(Throws([&] { VrTracker2ClearWideResult(buffer.data()); }), "wide result after initialization");
    Require(Throws([&] { VrTracker2ClearNarrowResult(buffer.data()); }), "narrow result after initialization");
    Require(Throws([&] { VrTracker2CheckSecondThenReportNotInitialized(buffer.data(), buffer.data()); }), "second check after initialization");
    Require(AllEqual(buffer.data(), buffer.data() + 32, 0xAA), "initialized calls wrote output");
    Require(Throws([] { sceVrTracker2GetCoordinateSystem(); }), "coordinate system after initialization");
    Require(Throws([] { sceVrTracker2GetPlayAreaBoundaryGeometry(); }), "boundary geometry after initialization");
    Require(Throws([] { sceVrTracker2SetCoordinateSystem(); }), "set coordinate system after initialization");
}

void CheckInitializationFree() {
    Require(VrTracker2Accept() == 0, "accepting export");

    auto defaults = Filled();
    Require(VrTracker2FillDefaults(defaults.data()) == 0, "defaults return");
    const std::array<std::uint8_t, 16> expected{0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00};
    Require(std::memcmp(defaults.data(), expected.data(), expected.size()) == 0, "defaults layout");
    Require(AllEqual(defaults.data() + 16, defaults.data() + 32, 0xAA), "defaults stop at 16 bytes");
    Require(Throws([] { VrTracker2FillDefaults(nullptr); }), "defaults null");

    auto word = Filled();
    Require(VrTracker2ClearWord(word.data()) == 0, "clear word return");
    Require(AllEqual(word.data(), word.data() + 4, 0) && AllEqual(word.data() + 4, word.data() + 32, 0xAA), "clear word clears 4 bytes");
    Require(Throws([] { VrTracker2ClearWord(nullptr); }), "clear word null");
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
    Require(sceVrTracker2GetCoordinateSystem() == NotInit, "coordinate system before initialization");
    Require(sceVrTracker2GetPlayAreaBoundaryGeometry() == NotInit, "boundary geometry before initialization");
    Require(sceVrTracker2SetCoordinateSystem() == NotInit, "set coordinate system before initialization");
    CheckNidExportsUninitialized();
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
    CheckInitializationFree();

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
        CheckNidExportsInitialized();
        CheckInitializationFree();

        Require(sceVrTracker2Finalize() == 0, "finalize failed");
        Require(sceVrTracker2Finalize() == NotInit, "duplicate finalize");
        CheckUninitialized(getParam, result);
    }
    CheckQueryMemoryModes();
    std::puts("VRTRACKER2 tests passed");
    return 0;
}
