#include "prx/libc/include/general/VabiMacros.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
int APS5_VABI sceAudioOutExOpen(int userId, std::uint32_t mode);
int APS5_VABI sceAudioOutExConfigureOutput(int control, std::uint64_t flags, std::uint32_t mode, std::uint32_t target, std::uint64_t option);
int APS5_VABI sceAudioOutExClose(int handle);
int APS5_VABI sceAudioOutOutput(int handle, const void* ptr);
int APS5_VABI sceAudioOutOpen(int userId, int type, int index, std::uint32_t len, std::uint32_t freq, std::uint32_t param);
int APS5_VABI sceAudioOutClose(int handle);
int APS5_VABI sceAudioOutGetLastOutputTime(int handle, std::uint64_t* outputTime);
int APS5_VABI sceAudioOutSysOpen(int userId, int mode);
int APS5_VABI sceAudioOutSysClose(int handle);
int APS5_VABI sceAudioOutSysConfigureOutput(int type, std::uint32_t flags, int mode, int target, std::uint64_t option);
}

static constexpr int SYSTEM_USER = 0xFF;
static constexpr int ERROR_EX_INVALID_MODE = static_cast<int>(0x80260015u);
static constexpr int ERROR_EX_INVALID_CONTROL = static_cast<int>(0x80268009u);
static constexpr int ERROR_INVALID_PORT = static_cast<int>(0x80260003u);
static constexpr int ERROR_PORT_FULL = static_cast<int>(0x80260005u);
static constexpr int ERROR_INVALID_PORT_TYPE = static_cast<int>(0x8026000Au);
static constexpr int ERROR_INVALID_CONF_TYPE = static_cast<int>(0x8026000Cu);
static constexpr int MODE_AC3 = 0;
static constexpr int MODE_MAT = 5;
static constexpr int MODE_PCM = 0xFF;
static constexpr int TARGET_HDMI = 1;
static constexpr int TARGET_DEFAULT = 0xFF;
static constexpr int SYS_TYPE_HDMI = 1;
static constexpr int PORTS_MAX = 32;

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

static void SetEnvironment(const char* name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

static void TestSystemPorts(const std::filesystem::path& deviceFile) {
    Require(sceAudioOutSysConfigureOutput(SYS_TYPE_HDMI, 0, MODE_PCM, TARGET_DEFAULT, 0) == 0, "restoring PCM output through the system call must succeed");
    Require(sceAudioOutSysConfigureOutput(SYS_TYPE_HDMI, 0, MODE_MAT, TARGET_HDMI, 0) == ERROR_INVALID_CONF_TYPE, "MAT output must be refused by a PCM host output");
    Require(Throws([] { sceAudioOutSysConfigureOutput(SYS_TYPE_HDMI, 0, MODE_AC3, TARGET_HDMI, 0); }), "a mode outside the system table must throw");
    Require(Throws([] { sceAudioOutSysConfigureOutput(0, 0, MODE_PCM, TARGET_DEFAULT, 0); }), "an output type other than HDMI must throw");
    Require(Throws([] { sceAudioOutSysOpen(SYSTEM_USER, MODE_AC3); }), "a mode outside the system table must throw on open");

    const int ac3 = sceAudioOutExOpen(SYSTEM_USER, MODE_AC3);
    const int mat = sceAudioOutSysOpen(SYSTEM_USER, MODE_MAT);
    Require(ac3 > 0 && mat > 0 && ac3 != mat, "bitstream ports must open with distinct handles");

    const std::vector<std::uint8_t> burst(1024 * 16, 0x5A);
    std::uint64_t time = 1;
    Require(sceAudioOutGetLastOutputTime(mat, &time) == 0 && time == 0, "a system port that never output must report 0");
    Require(sceAudioOutOutput(mat, burst.data()) == 1024, "a MAT port takes 1024 frames per output");
    Require(sceAudioOutGetLastOutputTime(mat, &time) == 0 && time != 0, "an output must set the last output time");

    const auto start = std::chrono::steady_clock::now();
    for (int grain = 0; grain < 30; ++grain) Require(sceAudioOutOutput(mat, burst.data()) == 1024, "repeated output must be accepted");
    Require(sceAudioOutOutput(mat, nullptr) == 1024, "waiting for the queue must succeed");
    const auto elapsed = std::chrono::steady_clock::now() - start;
    Require(elapsed >= std::chrono::milliseconds(150) && elapsed < std::chrono::seconds(2), "a system port must be paced at its frame rate");
    Require(!std::filesystem::exists(deviceFile), "bitstream bursts must not reach the host device");

    Require(sceAudioOutSysClose(ac3) == ERROR_INVALID_PORT_TYPE, "the system close must refuse an extended port");
    Require(Throws([mat] { sceAudioOutExClose(mat); }), "the extended close must refuse a system port");
    Require(sceAudioOutSysClose(mat) == 0, "the system close must close a system port");
    Require(sceAudioOutSysClose(mat) == ERROR_INVALID_PORT, "a closed system port must be refused");
    Require(sceAudioOutSysClose(PORTS_MAX + 1) == ERROR_INVALID_PORT, "handles outside the table must be refused");

    std::vector<int> handles;
    for (int port = 1; port < PORTS_MAX; ++port) {
        const int handle = sceAudioOutExOpen(SYSTEM_USER, 2);
        Require(handle > 0, "every free port must open");
        handles.push_back(handle);
    }
    Require(sceAudioOutSysOpen(SYSTEM_USER, MODE_MAT) == ERROR_PORT_FULL, "a full port table must be reported to the system open");
    for (const int handle : handles) Require(sceAudioOutExClose(handle) == 0, "every port must close");
    Require(sceAudioOutExClose(ac3) == 0, "the extended port must close");

    const int pcm = sceAudioOutOpen(0x10000000, 0, 0, 256, 48000, 1);
    Require(pcm > 0, "a PCM port must open");
    Require(sceAudioOutSysClose(pcm) == ERROR_INVALID_PORT_TYPE, "the system close must refuse a PCM port");
    Require(sceAudioOutClose(pcm) == 0, "the PCM port must close");
}

int main() {
    const auto deviceFile = std::filesystem::temp_directory_path() / ("anyps5_audio_out_ex-" + std::to_string(std::random_device{}()) + ".raw");
    std::filesystem::remove(deviceFile);
    SetEnvironment("SDL_AUDIODRIVER", "disk");
    SetEnvironment("SDL_DISKAUDIOFILE", deviceFile.string());
    Require(sceAudioOutExOpen(SYSTEM_USER, 5) == ERROR_EX_INVALID_MODE, "mode 5 (Linear PCM) is refused");
    Require(sceAudioOutExOpen(SYSTEM_USER, 7) == ERROR_EX_INVALID_MODE, "mode 7 is outside mask 0x61F");
    Require(sceAudioOutExOpen(SYSTEM_USER, 11) == ERROR_EX_INVALID_MODE, "mode 11 is above 10");

    const int ac3 = sceAudioOutExOpen(SYSTEM_USER, 0);
    Require(ac3 > 0, "mode 0 opens a port");
    const std::vector<std::int16_t> burst(256 * 2, 0x1234);
    Require(sceAudioOutOutput(ac3, burst.data()) == 256, "a mode 0 port takes 256 frames");
    Require(sceAudioOutOutput(ac3, nullptr) == 256, "a mode 0 port drains");

    const int eac3 = sceAudioOutExOpen(SYSTEM_USER, 4);
    Require(eac3 > 0 && eac3 != ac3, "mode 4 opens a second port");
    const std::vector<std::int16_t> wide(1024 * 2, 0);
    Require(sceAudioOutOutput(eac3, wide.data()) == 1024, "a mode 4 port takes 1024 frames");

    Require(sceAudioOutExConfigureOutput(1, 0, 0, 1, 0) == ERROR_EX_INVALID_CONTROL, "a nonzero first argument is refused");
    Require(sceAudioOutExConfigureOutput(0, 0, 0, 1, 0) == 0, "mode 0 to HDMI");
    Require(sceAudioOutExConfigureOutput(0, 0, 0, 0xFF, 0) == 0, "mode 0 to the default target");
    Require(sceAudioOutExConfigureOutput(0, 0, 0xFF, 0xFF, 0) == 0, "the reset");
    Require(Throws([] { sceAudioOutExConfigureOutput(0, 0, 11, 1, 0); }), "an unknown mode throws");
    Require(Throws([] { sceAudioOutExConfigureOutput(0, 0, 0, 4, 0); }), "an unknown target throws");
    Require(Throws([] { sceAudioOutExConfigureOutput(0, 1, 0, 1, 0); }), "nonzero flags throw");

    Require(sceAudioOutExClose(ac3) == 0, "close the mode 0 port");
    Require(Throws([ac3] { sceAudioOutExClose(ac3); }), "a closed port throws");
    Require(sceAudioOutExClose(eac3) == 0, "close the mode 4 port");

    TestSystemPorts(deviceFile);
    std::filesystem::remove(deviceFile);
    return failed ? 1 : 0;
}
