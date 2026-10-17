#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

extern "C" {
int APS5_VABI sceVideoOutOpen(int userId, int busType, int index, const void* param);
int APS5_VABI sceVideoOutClose(int handle);
int APS5_VABI sceVideoOutGetOutputStatus(int handle, VideoOutOutputStatus* status);
int APS5_VABI sceVideoOutGetOutputStatusWithVrr(int handle, void* status);
int APS5_VABI sceVideoOutGetResolutionStatus(int handle, VideoOutResolutionStatus* status);
int APS5_VABI sceVideoOutVrrPegToFixedRate(int handle, std::uint64_t arg1, std::uint64_t arg2);
int APS5_VABI sceVideoOutVrrUnpegFromFixedRate(int handle);
}

static constexpr int SYSTEM_USER = 255;
static constexpr int MAIN_BUS = 0;
static constexpr int NEVER_OPENED_HANDLE = 2;

static void Require(bool value) { if (!value) std::abort(); }

static bool Rejects(int handle, VideoOutResolutionStatus* status) {
    try {
        sceVideoOutGetResolutionStatus(handle, status);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

static bool PegRejects(int handle) {
    try {
        sceVideoOutVrrPegToFixedRate(handle, 0, 0);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

static bool UnpegRejects(int handle) {
    try {
        sceVideoOutVrrUnpegFromFixedRate(handle);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

static void RequireOutputUnchanged(int handle, const VideoOutOutputStatus& before) {
    VideoOutOutputStatus after{};
    Require(sceVideoOutGetOutputStatus(handle, &after) == 0);
    Require(after.flags == before.flags && after.dynamicRange == before.dynamicRange && after.refreshRate == before.refreshRate && after.resolution == before.resolution);
}

int main() {
    VideoOutTestEnvironment environment;
    int handle = 0;
    try {
        handle = sceVideoOutOpen(SYSTEM_USER, MAIN_BUS, 0, nullptr);
    } catch (const std::runtime_error& error) {
        if (std::getenv("ANYPS5_REQUIRE_DISPLAY") != nullptr) throw;
        std::printf("skipped, no display or Vulkan device: %s\n", error.what());
        return 77;
    }
    Require(handle > 0);

    VideoOutResolutionStatus status;
    std::memset(&status, 0xff, sizeof(status));
    Require(sceVideoOutGetResolutionStatus(handle, &status) == 0);
    Require(status.fullWidth == 1920 && status.fullHeight == 1080);
    Require(status.paneWidth == status.fullWidth && status.paneHeight == status.fullHeight);
    Require(status.screenSizeInInch == 0.0f && status.flags == 0 && status.reserved0 == 0);
    Require(status.reserved1[0] == 0 && status.reserved1[1] == 0 && status.reserved1[2] == 0);

    VideoOutOutputStatus output{};
    Require(sceVideoOutGetOutputStatus(handle, &output) == 0);
    Require(status.refreshRate == output.refreshRate);

    std::array<std::byte, 96> vrr;
    vrr.fill(std::byte{0xff});
    Require(sceVideoOutGetOutputStatusWithVrr(handle, vrr.data()) == 0);
    Require(std::memcmp(vrr.data(), &output, sizeof(output)) == 0);
    for (std::size_t offset = sizeof(output); offset < 80; ++offset) Require(vrr[offset] == std::byte{0});
    for (std::size_t offset = 80; offset < vrr.size(); ++offset) Require(vrr[offset] == std::byte{0xff});
    vrr.fill(std::byte{0xff});
    Require(sceVideoOutGetOutputStatusWithVrr(handle, nullptr) == static_cast<int>(0x80290002u));
    Require(sceVideoOutGetOutputStatusWithVrr(NEVER_OPENED_HANDLE, vrr.data()) == static_cast<int>(0x8029000Bu));
    Require(sceVideoOutGetOutputStatusWithVrr(-1, vrr.data()) == static_cast<int>(0x8029000Bu));
    for (const auto byte : vrr) Require(byte == std::byte{0xff});

    Require(sceVideoOutVrrPegToFixedRate(handle, 0, 0) == 0);
    RequireOutputUnchanged(handle, output);
    Require(sceVideoOutVrrPegToFixedRate(handle, 1, 0xffffffffffffffffull) == 0);
    RequireOutputUnchanged(handle, output);
    Require(sceVideoOutGetOutputStatusWithVrr(handle, vrr.data()) == 0 && std::memcmp(vrr.data(), &output, sizeof(output)) == 0);
    Require(sceVideoOutVrrUnpegFromFixedRate(handle) == 0);
    RequireOutputUnchanged(handle, output);
    Require(sceVideoOutVrrUnpegFromFixedRate(handle) == 0);
    for (int invalid : {0, -1, NEVER_OPENED_HANDLE}) {
        Require(PegRejects(invalid));
        Require(UnpegRejects(invalid));
    }

    Require(Rejects(handle, nullptr));
    Require(Rejects(0, &status));
    Require(Rejects(-1, &status));
    Require(Rejects(NEVER_OPENED_HANDLE, &status));

    Require(sceVideoOutClose(handle) == 0);
    Require(Rejects(handle, &status));
    Require(PegRejects(handle));
    Require(UnpegRejects(handle));
    Require(sceVideoOutGetOutputStatusWithVrr(handle, vrr.data()) == static_cast<int>(0x8029000Bu));
    LibcRunShutdown_nid_postfix();
}
