#include "prx/libc/include/general/VabiMacros.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
int APS5_VABI sceAudioOutOpen(int, int, int, std::uint32_t, std::uint32_t, std::uint32_t);
int APS5_VABI sceAudioOutClose(int);
int APS5_VABI sceAudioOutOutput(int, const void*);
int APS5_VABI sceAudioOutGetLastOutputTime(int, std::uint64_t*);
int APS5_VABI sceAudioOutExOpen(int, int);
int APS5_VABI sceAudioOutExClose(int);
int APS5_VABI sceAudioOutExConfigureOutput(int, std::uint32_t, int, int, std::uint64_t);
int APS5_VABI sceAudioOutSysOpen(int, int);
int APS5_VABI sceAudioOutSysClose(int);
int APS5_VABI sceAudioOutSysConfigureOutput(int, std::uint32_t, int, int, std::uint64_t);
}

static void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "%s\n", message);
    std::abort();
}

static void SetEnvironment(const char* name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

template<typename TFunction>
static bool ThrowsRuntimeError(TFunction function) {
    try {
        function();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

namespace {

constexpr int systemUser = 0xFF;
constexpr int modeAc3 = 0;
constexpr int modeAac = 1;
constexpr int modeDts = 2;
constexpr int modeEac3 = 3;
constexpr int modeMat = 5;
constexpr int modePcm = 0xFF;
constexpr int targetHdmi = 1;
constexpr int targetDefault = 0xFF;
constexpr int sysTypeHdmi = 1;
constexpr int portsMax = 32;
constexpr int invalidPort = static_cast<int>(0x80260003);
constexpr int portFull = static_cast<int>(0x80260005);
constexpr int invalidPortType = static_cast<int>(0x8026000A);
constexpr int invalidConfType = static_cast<int>(0x8026000C);

void TestConfigureOutput() {
    Require(sceAudioOutExConfigureOutput(0, 0, modePcm, targetDefault, 0) == 0, "restoring PCM output must succeed");
    Require(sceAudioOutExConfigureOutput(0, 0, modePcm, targetHdmi, 0) == 0, "restoring PCM output on HDMI must succeed");
    for (const int mode : {modeAc3, modeAac, modeDts, modeEac3}) {
        Require(sceAudioOutExConfigureOutput(0, 0, mode, targetDefault, 0) == invalidConfType, "a bitstream mode must be refused by a PCM host output");
        Require(sceAudioOutExConfigureOutput(0, 0, mode, targetHdmi, 0) == invalidConfType, "a bitstream mode on HDMI must be refused by a PCM host output");
    }
    Require(ThrowsRuntimeError([] { sceAudioOutExConfigureOutput(0, 0, 4, targetDefault, 0); }), "an unknown mode must throw");
    Require(ThrowsRuntimeError([] { sceAudioOutExConfigureOutput(0, 0, modeAc3, 2, 0); }), "an unknown target must throw");
    Require(ThrowsRuntimeError([] { sceAudioOutExConfigureOutput(1, 0, modePcm, targetDefault, 0); }), "a nonzero first argument must throw");
    Require(ThrowsRuntimeError([] { sceAudioOutExConfigureOutput(0, 1, modePcm, targetDefault, 0); }), "nonzero flags must throw");
    Require(ThrowsRuntimeError([] { sceAudioOutExConfigureOutput(0, 0, modePcm, targetDefault, 1); }), "a nonzero option must throw");

    Require(sceAudioOutSysConfigureOutput(sysTypeHdmi, 0, modePcm, targetDefault, 0) == 0, "restoring PCM output through the system call must succeed");
    Require(sceAudioOutSysConfigureOutput(sysTypeHdmi, 0, modeMat, targetHdmi, 0) == invalidConfType, "MAT output must be refused by a PCM host output");
    Require(ThrowsRuntimeError([] { sceAudioOutSysConfigureOutput(sysTypeHdmi, 0, modeAc3, targetHdmi, 0); }), "a mode outside the system table must throw");
    Require(ThrowsRuntimeError([] { sceAudioOutSysConfigureOutput(0, 0, modePcm, targetDefault, 0); }), "an output type other than HDMI must throw");
}

void TestOpenAndOutput(const std::filesystem::path& deviceFile) {
    Require(ThrowsRuntimeError([] { sceAudioOutExOpen(systemUser, 4); }), "an unknown bitstream mode must throw");
    Require(ThrowsRuntimeError([] { sceAudioOutExOpen(systemUser, modeMat); }), "the system MAT mode is not an extended mode");
    Require(ThrowsRuntimeError([] { sceAudioOutSysOpen(systemUser, modeAc3); }), "a mode outside the system table must throw");

    const int ac3 = sceAudioOutExOpen(systemUser, modeAc3);
    const int eac3 = sceAudioOutExOpen(systemUser, modeEac3);
    const int mat = sceAudioOutSysOpen(systemUser, modeMat);
    Require(ac3 > 0 && eac3 > 0 && mat > 0 && ac3 != eac3 && eac3 != mat && ac3 != mat, "bitstream ports must open with distinct handles");

    const std::vector<std::uint8_t> burst(1024 * 16, 0x5A);
    std::uint64_t time = 1;
    Require(sceAudioOutGetLastOutputTime(ac3, &time) == 0 && time == 0, "a bitstream port that never output must report 0");
    Require(sceAudioOutOutput(ac3, burst.data()) == 256, "an AC-3 port takes 256 frames per output");
    Require(sceAudioOutGetLastOutputTime(ac3, &time) == 0 && time != 0, "an output must set the last output time");
    Require(sceAudioOutOutput(eac3, burst.data()) == 1024, "an E-AC-3 port takes 1024 frames per output");
    Require(sceAudioOutOutput(mat, burst.data()) == 1024, "a MAT port takes 1024 frames per output");

    const auto start = std::chrono::steady_clock::now();
    for (int grain = 0; grain < 30; ++grain) Require(sceAudioOutOutput(ac3, burst.data()) == 256, "repeated output must be accepted");
    Require(sceAudioOutOutput(ac3, nullptr) == 256, "waiting for the queue must succeed");
    const auto elapsed = std::chrono::steady_clock::now() - start;
    Require(elapsed >= std::chrono::milliseconds(150) && elapsed < std::chrono::seconds(2), "a bitstream port must be paced at its frame rate");
    Require(!std::filesystem::exists(deviceFile), "bitstream bursts must not reach the host device");

    Require(sceAudioOutSysClose(ac3) == invalidPortType, "the system close must refuse an extended port");
    Require(sceAudioOutExClose(mat) == invalidPortType, "the extended close must refuse a system port");
    Require(sceAudioOutExClose(ac3) == 0, "the extended close must close an extended port");
    Require(sceAudioOutExClose(ac3) == invalidPort, "a closed port must be refused");
    Require(sceAudioOutOutput(ac3, burst.data()) == invalidPort, "a closed port must not take output");
    Require(sceAudioOutSysClose(mat) == 0, "the system close must close a system port");
    Require(sceAudioOutSysClose(mat) == invalidPort, "a closed system port must be refused");
    Require(sceAudioOutClose(eac3) == 0, "the plain close must close an extended port");
    Require(sceAudioOutExClose(0) == invalidPort && sceAudioOutSysClose(portsMax + 1) == invalidPort, "handles outside the table must be refused");

    std::vector<int> handles;
    for (int port = 0; port < portsMax; ++port) {
        const int handle = sceAudioOutExOpen(systemUser, modeDts);
        Require(handle > 0, "every free port must open");
        handles.push_back(handle);
    }
    Require(sceAudioOutExOpen(systemUser, modeAac) == portFull, "a full port table must be reported");
    Require(sceAudioOutSysOpen(systemUser, modeMat) == portFull, "a full port table must be reported to the system open");
    for (const int handle : handles) Require(sceAudioOutExClose(handle) == 0, "every port must close");

    const int pcm = sceAudioOutOpen(0x10000000, 0, 0, 256, 48000, 1);
    Require(pcm > 0, "a PCM port must open");
    Require(sceAudioOutExClose(pcm) == invalidPortType && sceAudioOutSysClose(pcm) == invalidPortType, "the bitstream closes must refuse a PCM port");
    Require(sceAudioOutClose(pcm) == 0, "the PCM port must close");
}

}

int main() {
    const auto deviceFile = std::filesystem::temp_directory_path() / ("anyps5_audio_out_ex-" + std::to_string(std::random_device{}()) + ".raw");
    std::filesystem::remove(deviceFile);
    SetEnvironment("SDL_AUDIODRIVER", "disk");
    SetEnvironment("SDL_DISKAUDIOFILE", deviceFile.string());
    TestConfigureOutput();
    TestOpenAndOutput(deviceFile);
    std::filesystem::remove(deviceFile);
    return 0;
}
