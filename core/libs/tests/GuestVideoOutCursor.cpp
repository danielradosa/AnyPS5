#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceVideoOutOpen(int userId, int busType, int index, const void* param);
int APS5_VABI sceVideoOutClose(int handle);
int APS5_VABI sceVideoOutCursorEnable(std::int32_t handle, std::int32_t index, const void* address);
int APS5_VABI sceVideoOutCursorDisable(std::int32_t handle, std::int32_t index);
int APS5_VABI sceVideoOutCursorSetImageAddress(std::int32_t handle, std::int32_t index, const void* address);
int APS5_VABI sceVideoOutCursorSetPosition(std::int32_t handle, std::int32_t index, std::uint32_t x, std::uint32_t y);
int APS5_VABI sceVideoOutCursorSetHotSpot(std::int32_t handle, std::int32_t index, std::uint32_t hotX, std::uint32_t hotY);
}

static constexpr int SYSTEM_USER = 255;
static constexpr int MAIN_BUS = 0;
static constexpr int NEVER_OPENED_HANDLE = 2;

static void Require(bool value) { if (!value) std::abort(); }

static bool Rejects(const std::string& error, const std::function<int()>& call) {
    try {
        call();
    } catch (const std::runtime_error& thrown) {
        return std::string(thrown.what()).find(error) != std::string::npos;
    }
    return false;
}

int main() {
    static const std::uint32_t images[2][64 * 64] = {};
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
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_HANDLE", [] { return sceVideoOutCursorEnable(NEVER_OPENED_HANDLE, 0, images[0]); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_HANDLE", [] { return sceVideoOutCursorDisable(0, 0); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_HANDLE", [] { return sceVideoOutCursorSetPosition(-1, 0, 0, 0); }));

    Require(sceVideoOutCursorEnable(handle, 0, images[0]) == 0);
    Require(sceVideoOutCursorSetHotSpot(handle, 0, 0, 0) == 0);
    Require(sceVideoOutCursorSetPosition(handle, 0, 928, 508) == 0);
    Require(sceVideoOutCursorSetImageAddress(handle, 0, images[1]) == 0);
    Require(sceVideoOutCursorSetPosition(handle, 0, 0, 0) == 0);
    Require(sceVideoOutCursorEnable(handle, 1, images[1]) == 0);
    Require(sceVideoOutCursorSetHotSpot(handle, 1, 31, 31) == 0);
    Require(sceVideoOutCursorDisable(handle, 1) == 0);
    Require(sceVideoOutCursorDisable(handle, 0) == 0);

    Require(Rejects("VIDEO_OUT_ERROR_INVALID_ADDRESS", [handle] { return sceVideoOutCursorEnable(handle, 0, nullptr); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_ADDRESS", [handle] { return sceVideoOutCursorSetImageAddress(handle, 0, nullptr); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_INDEX", [handle] { return sceVideoOutCursorEnable(handle, 2, images[0]); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_INDEX", [handle] { return sceVideoOutCursorDisable(handle, -1); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_INDEX", [handle] { return sceVideoOutCursorSetImageAddress(handle, 2, images[0]); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_INDEX", [handle] { return sceVideoOutCursorSetPosition(handle, 2, 0, 0); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_INDEX", [handle] { return sceVideoOutCursorSetHotSpot(handle, 2, 0, 0); }));

    Require(sceVideoOutClose(handle) == 0);
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_HANDLE", [handle] { return sceVideoOutCursorEnable(handle, 0, images[0]); }));
    Require(Rejects("VIDEO_OUT_ERROR_INVALID_HANDLE", [handle] { return sceVideoOutCursorSetHotSpot(handle, 0, 0, 0); }));
    LibcRunShutdown_nid_postfix();
}
