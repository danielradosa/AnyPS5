#include "SceTypes.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>

extern "C" {
std::size_t APS5_VABI sceAudio3dGetSpeakerArrayMemorySize(std::uint32_t num_speakers, std::uint8_t is_3d);
int APS5_VABI sceAudio3dCreateSpeakerArray(Audio3dSpeakerArrayHandle* handle, const Audio3dSpeakerArrayParameters* parameters);
int APS5_VABI sceAudio3dDeleteSpeakerArray(Audio3dSpeakerArrayHandle handle);
int APS5_VABI sceAudio3dGetSpeakerArrayMixCoefficients(Audio3dSpeakerArrayHandle handle, Audio3dPosition pos, float spread, float* coefficients, std::uint32_t num_coefficients);
int APS5_VABI sceAudio3dGetSpeakerArrayMixCoefficients2(Audio3dSpeakerArrayHandle handle, Audio3dPosition pos, float spread, float* coefficients, std::uint32_t num_coefficients, std::uint8_t height_aware, float downmix_spread_radius);
std::size_t APS5_VABI sceAudioOut2GetSpeakerArrayMemorySize(std::uint32_t num_speakers, std::uint8_t is_3d, std::uint8_t is_ambisonics);
}

static_assert(sizeof(Audio3dSpeakerArrayParameters) == 0x20);
static_assert(offsetof(Audio3dSpeakerArrayParameters, is_3d) == 0x0C);
static_assert(offsetof(Audio3dSpeakerArrayParameters, buffer) == 0x10);
static_assert(offsetof(Audio3dSpeakerArrayParameters, size) == 0x18);

namespace {

constexpr int INVALID_PARAMETER = static_cast<int>(0x80EA0004);
const float half = static_cast<float>(std::sqrt(0.5));
const float third = static_cast<float>(std::sqrt(1.0 / 3.0));

void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "%s\n", message);
    std::abort();
}

template <typename F>
void RequireThrows(F f, const char* message) {
    bool threw = false;
    try {
        f();
    } catch (const std::exception&) {
        threw = true;
    }
    Require(threw, message);
}

bool Near(const std::vector<float>& gains, const std::vector<float>& expected) {
    if (gains.size() != expected.size()) return false;
    for (std::size_t i = 0; i < gains.size(); ++i) {
        if (std::abs(gains[i] - expected[i]) > 1e-5f) return false;
    }
    return true;
}

Audio3dPosition Horizontal(double degrees) {
    const double radians = degrees * 3.14159265358979323846 / 180.0;
    return {static_cast<float>(std::cos(radians)), static_cast<float>(std::sin(radians)), 0.0f};
}

struct Memory {
    explicit Memory(std::size_t size) : words(size / sizeof(std::uint64_t) + 1), size(size) {}
    void* Data() { return words.data(); }
    std::vector<std::uint64_t> words;
    std::size_t size;
};

std::vector<float> Gains(Audio3dSpeakerArrayHandle handle, std::uint32_t count, Audio3dPosition source) {
    std::vector<float> gains(count, -1.0f);
    Require(sceAudio3dGetSpeakerArrayMixCoefficients(handle, source, 0.0f, gains.data(), count) == 0, "the mix coefficients must be computed");
    return gains;
}

void CheckMemorySize() {
    Require(sceAudio3dGetSpeakerArrayMemorySize(4, 1) == sceAudioOut2GetSpeakerArrayMemorySize(4, 1, 0), "memory size forwards to AudioOut2 without ambisonics");
    Require(sceAudio3dGetSpeakerArrayMemorySize(1, 0) > 0, "one speaker needs memory");
    Require(sceAudio3dGetSpeakerArrayMemorySize(16, 1) >= sceAudio3dGetSpeakerArrayMemorySize(1, 0), "sixteen speakers need at least as much memory as one");
    RequireThrows([] { sceAudio3dGetSpeakerArrayMemorySize(0, 0); }, "the size of an array without speakers must throw");
    RequireThrows([] { sceAudio3dGetSpeakerArrayMemorySize(17, 0); }, "the size of an array above 16 speakers must throw");
}

void CheckCreateParameters() {
    const Audio3dPosition positions[2] = {Horizontal(30.0), Horizontal(-30.0)};
    Memory memory(sceAudio3dGetSpeakerArrayMemorySize(2, 0));
    int sentinel = 0;
    Audio3dSpeakerArrayHandle handle = &sentinel;
    const Audio3dSpeakerArrayParameters valid{positions, 2, 0, memory.Data(), memory.size};
    Require(sceAudio3dCreateSpeakerArray(nullptr, &valid) == INVALID_PARAMETER, "a null handle must be refused");
    Require(sceAudio3dCreateSpeakerArray(&handle, nullptr) == INVALID_PARAMETER, "null parameters must be refused");
    auto rejects = [&](auto change, const char* message) {
        Audio3dSpeakerArrayParameters parameters = valid;
        change(parameters);
        Require(sceAudio3dCreateSpeakerArray(&handle, &parameters) == INVALID_PARAMETER, message);
        Require(handle == &sentinel, "a refused array must not write the handle");
    };
    rejects([](auto& p) { p.speaker_positions = nullptr; }, "null speaker positions must be refused");
    rejects([](auto& p) { p.num_speakers = 0; }, "an array without speakers must be refused");
    rejects([](auto& p) { p.num_speakers = 17; }, "an array above 16 speakers must be refused");
    auto throws = [&](auto change, const char* message) {
        Audio3dSpeakerArrayParameters parameters = valid;
        change(parameters);
        RequireThrows([&] { sceAudio3dCreateSpeakerArray(&handle, &parameters); }, message);
    };
    throws([](auto& p) { p.buffer = nullptr; }, "an array without a buffer must throw");
    throws([](auto& p) { p.size -= 1; }, "a buffer below the memory size must throw");
}

void CheckStereo() {
    const Audio3dPosition positions[2] = {Horizontal(30.0), Horizontal(-30.0)};
    Memory memory(sceAudio3dGetSpeakerArrayMemorySize(2, 0));
    const Audio3dSpeakerArrayParameters parameters{positions, 2, 0, memory.Data(), memory.size};
    Audio3dSpeakerArrayHandle handle = nullptr;
    Require(sceAudio3dCreateSpeakerArray(&handle, &parameters) == 0 && handle != nullptr, "a stereo array must be created");
    Require(Near(Gains(handle, 2, Horizontal(30.0)), {1.0f, 0.0f}), "a source at the left speaker plays from it alone");
    Require(Near(Gains(handle, 2, {1.0f, 0.0f, 0.0f}), {half, half}), "a centred source plays from both at equal power");
    Require(Near(Gains(handle, 2, {2.0f, 0.0f, 3.0f}), {half, half}), "a flat array ignores distance and height");
    float gains[3] = {-1.0f, -1.0f, -1.0f};
    const Audio3dPosition front{1.0f, 0.0f, 0.0f};
    Require(sceAudio3dGetSpeakerArrayMixCoefficients(handle, front, 0.0f, nullptr, 2) == INVALID_PARAMETER, "null coefficients must be refused");
    Require(sceAudio3dGetSpeakerArrayMixCoefficients(handle, front, 0.0f, gains, 0) == INVALID_PARAMETER, "zero coefficients must be refused");
    Require(sceAudio3dGetSpeakerArrayMixCoefficients(handle, front, 0.0f, gains, 3) == INVALID_PARAMETER, "a count other than the speaker count must be refused");
    Require(sceAudio3dGetSpeakerArrayMixCoefficients(nullptr, front, 0.0f, gains, 2) == INVALID_PARAMETER, "a null handle must be refused");
    Require(sceAudio3dGetSpeakerArrayMixCoefficients(&handle, front, 0.0f, gains, 2) == INVALID_PARAMETER, "a handle that was never created must be refused");
    Require(gains[0] == -1.0f && gains[1] == -1.0f, "a refused query must not write coefficients");
    RequireThrows([&] { sceAudio3dGetSpeakerArrayMixCoefficients(handle, front, 0.5f, gains, 2); }, "a spread source must throw");
    Require(sceAudio3dGetSpeakerArrayMixCoefficients2(handle, Horizontal(-30.0), 0.0f, gains, 2, 1, 0.0f) == 0, "height awareness without a radius must be accepted");
    Require(Near({gains[0], gains[1]}, {0.0f, 1.0f}), "mix coefficients 2 pans like mix coefficients");
    Require(sceAudio3dGetSpeakerArrayMixCoefficients2(handle, front, 0.0f, gains, 3, 0, 0.0f) == INVALID_PARAMETER, "mix coefficients 2 checks the count");
    Require(sceAudio3dGetSpeakerArrayMixCoefficients2(nullptr, front, 0.0f, gains, 2, 0, 0.0f) == INVALID_PARAMETER, "mix coefficients 2 checks the handle");
    RequireThrows([&] { sceAudio3dGetSpeakerArrayMixCoefficients2(handle, front, 0.0f, gains, 2, 1, 1.0f); }, "a downmix spread radius must throw");
    Require(sceAudio3dDeleteSpeakerArray(handle) == 0, "the array must be deleted");
    Require(sceAudio3dDeleteSpeakerArray(handle) == INVALID_PARAMETER, "a deleted array must not be deleted again");
    Require(sceAudio3dGetSpeakerArrayMixCoefficients(handle, front, 0.0f, gains, 2) == INVALID_PARAMETER, "a deleted array must be refused");
    Require(sceAudio3dDeleteSpeakerArray(nullptr) == INVALID_PARAMETER, "a null handle must not be deleted");
    Audio3dSpeakerArrayHandle again = nullptr;
    Require(sceAudio3dCreateSpeakerArray(&again, &parameters) == 0, "the buffer must take a new array");
    Require(Near(Gains(again, 2, Horizontal(-30.0)), {0.0f, 1.0f}), "the new array must pan");
    Require(sceAudio3dDeleteSpeakerArray(again) == 0, "the new array must be deleted");
}

void CheckSphere() {
    const Audio3dPosition positions[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    Memory memory(sceAudio3dGetSpeakerArrayMemorySize(6, 1));
    const Audio3dSpeakerArrayParameters parameters{positions, 6, 1, memory.Data(), memory.size};
    Audio3dSpeakerArrayHandle handle = nullptr;
    Require(sceAudio3dCreateSpeakerArray(&handle, &parameters) == 0, "a 3D array must be created");
    Require(Near(Gains(handle, 6, {1, 1, 1}), {third, 0.0f, third, 0.0f, third, 0.0f}), "a 3D array pans between three speakers");
    Require(Near(Gains(handle, 6, {0, 0, 2}), {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}), "a 3D array keeps the height");
    Require(sceAudio3dDeleteSpeakerArray(handle) == 0, "the 3D array must be deleted");
}

}

int main() {
    CheckMemorySize();
    CheckCreateParameters();
    CheckStereo();
    CheckSphere();
}
