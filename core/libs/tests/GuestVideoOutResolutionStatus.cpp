#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
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
    Require(after.flags == 0 && after.refreshRate == before.refreshRate && after.resolution == before.resolution);
}

int main() {
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

    Require(sceVideoOutVrrPegToFixedRate(handle, 0, 0) == 0);
    RequireOutputUnchanged(handle, output);
    Require(sceVideoOutVrrPegToFixedRate(handle, 1, 0xffffffffffffffffull) == 0);
    RequireOutputUnchanged(handle, output);
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
    LibcRunShutdown_nid_postfix();
}
