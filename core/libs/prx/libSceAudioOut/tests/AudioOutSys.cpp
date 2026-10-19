#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

extern "C" {
int APS5_VABI sceAudioOutSysOpen(int userId, int mode);
int APS5_VABI sceAudioOutSysConfigureOutput(int type, std::uint32_t flags, int mode, int target, std::uint64_t option);
int APS5_VABI sceAudioOutSysClose(int handle);
int APS5_VABI sceAudioOutExOpen(int userId, std::uint32_t mode);
int APS5_VABI sceAudioOutOutput(int handle, const void* ptr);
}

static constexpr int SYSTEM_USER = 0xFF;
static constexpr int ERROR_INVALID_PORT = static_cast<int>(0x80260003u);
static constexpr int ERROR_INVALID_PORT_TYPE = static_cast<int>(0x8026000Au);
static constexpr int ERROR_INVALID_CONF_TYPE = static_cast<int>(0x8026000Cu);

static bool failed = false;

static void Require(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "failed: %s\n", what);
        failed = true;
    }
}

template <typename Call>
static bool Throws(Call call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

int main() {
    Require(Throws([] { sceAudioOutSysOpen(SYSTEM_USER, 0); }), "mode 0 throws: linear PCM opens through the Ex path");
    Require(Throws([] { sceAudioOutSysOpen(SYSTEM_USER, 6); }), "mode 6 throws");

    const int truehd = sceAudioOutSysOpen(SYSTEM_USER, 5);
    Require(truehd > 0, "mode 5 opens a TrueHD MAT port");
    const std::vector<std::int16_t> grain(1024 * 8, 0x1234);
    Require(sceAudioOutOutput(truehd, grain.data()) == 1024, "a mode 5 port takes 1024 eight-channel frames");
    Require(sceAudioOutOutput(truehd, nullptr) == 1024, "a mode 5 port drains");

    const int second = sceAudioOutSysOpen(SYSTEM_USER, 5);
    Require(second > 0 && second != truehd, "a second mode 5 port opens");

    Require(sceAudioOutSysConfigureOutput(1, 0, 5, 1, 0) == ERROR_INVALID_CONF_TYPE, "the HDMI MAT switch is refused by a PCM host output");
    Require(Throws([] { sceAudioOutSysConfigureOutput(0, 0, 5, 1, 0); }), "another output type throws");
    Require(Throws([] { sceAudioOutSysConfigureOutput(1, 1, 5, 1, 0); }), "nonzero flags throw");
    Require(Throws([] { sceAudioOutSysConfigureOutput(1, 0, 4, 1, 0); }), "another mode throws");
    Require(Throws([] { sceAudioOutSysConfigureOutput(1, 0, 5, 2, 0); }), "another target throws");

    Require(sceAudioOutSysClose(truehd) == 0, "close the first Sys port");
    Require(sceAudioOutSysClose(truehd) == ERROR_INVALID_PORT, "a closed Sys port is refused");
    const int ex = sceAudioOutExOpen(SYSTEM_USER, 0);
    Require(ex > 0, "an Ex port opens");
    Require(sceAudioOutSysClose(ex) == ERROR_INVALID_PORT_TYPE, "an Ex port is not a Sys port");
    Require(sceAudioOutSysClose(0) == ERROR_INVALID_PORT, "handle 0 is not a Sys port");
    Require(sceAudioOutSysClose(1000) == ERROR_INVALID_PORT, "an out-of-range handle is not a Sys port");
    Require(sceAudioOutSysClose(second) == 0, "close the second Sys port");
    return failed ? 1 : 0;
}
