#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI sceMoveClose(std::int32_t handle);
int APS5_VABI sceMoveGetExtensionPortInfo(std::int32_t handle, void* data);
int APS5_VABI sceMoveReadStateLatest(std::int32_t handle, void* data);
int APS5_VABI sceMoveReadStateRecent(std::int32_t handle, std::int64_t timestamp, void* data, std::int32_t* count);
int APS5_VABI sceMoveResetLightSphere(std::int32_t handle);
int APS5_VABI sceMoveSetLightSphere(std::int32_t handle, std::uint8_t red, std::uint8_t green, std::uint8_t blue);
}

namespace {

constexpr int notInitialized = static_cast<int>(0x80EE0001u);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "Move: %s\n", message);
        std::abort();
    }
}

}

int main() {
    std::array<std::uint8_t, 0x80> data;
    data.fill(0xa5);
    const auto untouched = data;
    std::int32_t count = 7;
    for (const std::int32_t handle : {0, 1, 0x30b0100, -1}) {
        Require(sceMoveReadStateLatest(handle, nullptr) == notInitialized, "latest read with no buffer is not NOT_INIT");
        Require(sceMoveReadStateLatest(handle, data.data()) == notInitialized, "latest read is not NOT_INIT");
        Require(sceMoveReadStateRecent(handle, -1, nullptr, nullptr) == notInitialized, "recent read with bad arguments is not NOT_INIT");
        Require(sceMoveReadStateRecent(handle, 0, data.data(), &count) == notInitialized, "recent read is not NOT_INIT");
        Require(sceMoveGetExtensionPortInfo(handle, nullptr) == notInitialized, "extension port info with no buffer is not NOT_INIT");
        Require(sceMoveGetExtensionPortInfo(handle, data.data()) == notInitialized, "extension port info is not NOT_INIT");
        Require(sceMoveSetLightSphere(handle, 0xff, 0x40, 0x00) == notInitialized, "set light sphere is not NOT_INIT");
        Require(sceMoveResetLightSphere(handle) == notInitialized, "reset light sphere is not NOT_INIT");
        Require(sceMoveClose(handle) == notInitialized, "close is not NOT_INIT");
    }
    Require(data == untouched, "a call wrote the buffer");
    Require(count == 7, "a call wrote the count");
}
