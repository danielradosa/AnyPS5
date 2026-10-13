#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <vector>

extern "C" {
std::size_t APS5_VABI sceAudioOut2GetSpeakerArrayMemorySize(std::uint32_t, std::uint8_t, std::uint8_t);
int APS5_VABI sceAudioOut2SpeakerArrayCreate(AudioOut2SpeakerArrayHandle*, const void*, const void*);
int APS5_VABI sceAudioOut2SpeakerArrayDestroy(AudioOut2SpeakerArrayHandle);
int APS5_VABI sceAudioOut2GetSpeakerArrayCoefficients(AudioOut2SpeakerArrayHandle, AudioOut2Position, float, float*, std::uint32_t, std::uint8_t, float);
}

static void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "%s\n", message);
    std::abort();
}

template<typename TException, typename TFunction>
static bool Throws(TFunction function) {
    try {
        function();
    } catch (const TException&) {
        return true;
    }
    return false;
}

namespace {

struct VbapParams {
    const AudioOut2Position* positions;
    std::uint32_t numSpeakers;
    std::uint8_t is3d;
    void* memory;
    std::size_t memorySize;
    std::uint32_t correctionType;
    float correctionGain;
    std::uint32_t reserved[6];
};

struct AmbisonicsParams {
    std::uint32_t channelOrder;
    std::uint8_t heightAware;
    std::uint8_t pad1;
    std::uint16_t pad2;
    std::uint32_t reserved[6];
};

constexpr double tolerance = 1e-5;
constexpr double pi = 3.14159265358979323846;
const float half = static_cast<float>(std::sqrt(0.5));

class Array {
public:
    Array(const std::vector<AudioOut2Position>& positions, bool is3d, std::uint32_t correctionType = 0)
        : count(static_cast<std::uint32_t>(positions.size())), speakers(positions) {
        const std::size_t size = sceAudioOut2GetSpeakerArrayMemorySize(count, is3d ? 1 : 0, 0);
        memory.resize(size / sizeof(std::uint64_t) + 1);
        const VbapParams vbap{speakers.data(), count, static_cast<std::uint8_t>(is3d ? 1 : 0), memory.data(), size, correctionType, 0.0f, {}};
        const AmbisonicsParams ambisonics{};
        Require(sceAudioOut2SpeakerArrayCreate(&handle, &vbap, &ambisonics) == 0, "the speaker array must be created");
    }

    ~Array() { sceAudioOut2SpeakerArrayDestroy(handle); }

    std::vector<float> Gains(AudioOut2Position source) const {
        std::vector<float> gains(count, -1.0f);
        Require(sceAudioOut2GetSpeakerArrayCoefficients(handle, source, 0.0f, gains.data(), count, 0, 0.0f) == 0, "the coefficients must be computed");
        return gains;
    }

    AudioOut2SpeakerArrayHandle handle = nullptr;
    std::uint32_t count;
    std::vector<AudioOut2Position> speakers;

private:
    std::vector<std::uint64_t> memory;
};

AudioOut2Position Horizontal(double degrees) {
    const double radians = degrees * pi / 180.0;
    return {static_cast<float>(std::cos(radians)), static_cast<float>(std::sin(radians)), 0.0f};
}

bool Near(const std::vector<float>& gains, const std::vector<float>& expected) {
    if (gains.size() != expected.size()) return false;
    for (std::size_t i = 0; i < gains.size(); ++i) {
        if (std::abs(gains[i] - expected[i]) > tolerance) return false;
    }
    return true;
}

void RequirePanned(const Array& array, const std::vector<float>& gains, AudioOut2Position source, bool is3d, std::size_t maxActive) {
    double power = 0.0;
    double sum[3] = {0.0, 0.0, 0.0};
    std::size_t active = 0;
    for (std::size_t i = 0; i < gains.size(); ++i) {
        Require(gains[i] >= 0.0f, "gains must not be negative");
        power += double(gains[i]) * gains[i];
        if (gains[i] > tolerance) active++;
        const AudioOut2Position& speaker = array.speakers[i];
        const double z = is3d ? speaker.z : 0.0;
        const double length = std::sqrt(double(speaker.x) * speaker.x + double(speaker.y) * speaker.y + z * z);
        sum[0] += gains[i] * speaker.x / length;
        sum[1] += gains[i] * speaker.y / length;
        sum[2] += gains[i] * z / length;
    }
    Require(std::abs(power - 1.0) < tolerance, "gains must keep unit power");
    Require(active >= 1 && active <= maxActive, "only the enclosing speakers may play");
    const double target[3] = {source.x, source.y, is3d ? source.z : 0.0};
    const double cross[3] = {sum[1] * target[2] - sum[2] * target[1], sum[2] * target[0] - sum[0] * target[2], sum[0] * target[1] - sum[1] * target[0]};
    const double sumLength = std::sqrt(sum[0] * sum[0] + sum[1] * sum[1] + sum[2] * sum[2]);
    const double targetLength = std::sqrt(target[0] * target[0] + target[1] * target[1] + target[2] * target[2]);
    const double crossLength = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
    Require(crossLength / (sumLength * targetLength) < 1e-4, "the weighted speaker directions must point at the source");
    Require(sum[0] * target[0] + sum[1] * target[1] + sum[2] * target[2] > 0.0, "the panned image must face the source");
}

void TestStereo() {
    const Array stereo({Horizontal(30.0), Horizontal(-30.0)}, false);
    Require(Near(stereo.Gains(Horizontal(30.0)), {1.0f, 0.0f}), "a source at the left speaker plays from it alone");
    Require(Near(stereo.Gains(Horizontal(-30.0)), {0.0f, 1.0f}), "a source at the right speaker plays from it alone");
    Require(Near(stereo.Gains({1.0f, 0.0f, 0.0f}), {half, half}), "a centred source plays from both at equal power");
    Require(Near(stereo.Gains({3.0f, 0.0f, 2.0f}), {half, half}), "distance and height do not change a flat array's gains");
    const double left = std::sin(40.0 * pi / 180.0);
    const double right = std::sin(20.0 * pi / 180.0);
    const double norm = std::sqrt(left * left + right * right);
    Require(Near(stereo.Gains(Horizontal(10.0)), {static_cast<float>(left / norm), static_cast<float>(right / norm)}), "an off-centre source follows the sine law");
    Require(Near(stereo.Gains({-1.0f, 0.0f, 0.0f}), {half, half}), "a source behind plays from the two nearest speakers");
    Require(Near(stereo.Gains(Horizontal(100.0)), {1.0f, 0.0f}), "a source outside the pair plays from the nearest speaker");
    for (int degrees = -30; degrees <= 30; degrees += 5) RequirePanned(stereo, stereo.Gains(Horizontal(degrees)), Horizontal(degrees), false, 2);
    Require(Throws<std::runtime_error>([&stereo] { stereo.Gains({0.0f, 0.0f, 1.0f}); }), "a source above a flat array has no direction");
    Require(Throws<std::runtime_error>([&stereo] { stereo.Gains({0.0f, 0.0f, 0.0f}); }), "a source at the listener has no direction");
}

void TestRing() {
    const Array quad({Horizontal(45.0), Horizontal(135.0), Horizontal(-135.0), Horizontal(-45.0)}, false);
    Require(Near(quad.Gains(Horizontal(90.0)), {half, half, 0.0f, 0.0f}), "a source between two speakers plays from them");
    Require(Near(quad.Gains(Horizontal(180.0)), {0.0f, half, half, 0.0f}), "a rear source plays from the rear pair");
    Require(Near(quad.Gains(Horizontal(-45.0)), {0.0f, 0.0f, 0.0f, 1.0f}), "a source at a speaker plays from it alone");
    for (int degrees = -180; degrees < 180; degrees += 7) RequirePanned(quad, quad.Gains(Horizontal(degrees)), Horizontal(degrees), false, 2);
}

void TestSphere() {
    const Array octahedron({{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}, true);
    const float third = static_cast<float>(std::sqrt(1.0 / 3.0));
    Require(Near(octahedron.Gains({1, 1, 1}), {third, 0.0f, third, 0.0f, third, 0.0f}), "a source in a face plays from its three speakers");
    Require(Near(octahedron.Gains({0, 0, 2}), {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}), "a source at the top speaker plays from it alone");
    Require(Near(octahedron.Gains({0, -1, -1}), {0.0f, 0.0f, 0.0f, half, 0.0f, half}), "a source on an edge plays from its two speakers");
    std::mt19937 random(5);
    std::normal_distribution<float> axis(0.0f, 1.0f);
    for (int i = 0; i < 200; ++i) {
        const AudioOut2Position source{axis(random), axis(random), axis(random)};
        RequirePanned(octahedron, octahedron.Gains(source), source, true, 3);
    }
}

void TestDome() {
    const Array dome({{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {0, 0, 1}}, true);
    Require(Near(dome.Gains({1, 0, 1}), {half, 0.0f, 0.0f, 0.0f, half}), "a raised source plays between the ring and the top");
    Require(Near(dome.Gains({0.6f, 0.0f, -0.8f}), {1.0f, 0.0f, 0.0f, 0.0f, 0.0f}), "a source below the ring plays from the ring");
    Require(Near(dome.Gains({0, 0, -1}), {0.5f, 0.5f, 0.5f, 0.5f, 0.0f}), "a source straight below plays from the nearest speakers");
    std::mt19937 random(7);
    std::normal_distribution<float> axis(0.0f, 1.0f);
    for (int i = 0; i < 100; ++i) {
        const AudioOut2Position source{axis(random), axis(random), std::abs(axis(random))};
        RequirePanned(dome, dome.Gains(source), source, true, 3);
    }
}

void TestSingle() {
    const Array single({{0, 1, 0}}, true);
    Require(Near(single.Gains({1, 0, 0}), {1.0f}), "a single speaker plays every source");
}

void TestErrors() {
    const Array stereo({Horizontal(30.0), Horizontal(-30.0)}, false);
    float gains[3] = {};
    const AudioOut2Position front{1.0f, 0.0f, 0.0f};
    const auto handle = stereo.handle;
    Require(Throws<std::invalid_argument>([&] { sceAudioOut2GetSpeakerArrayCoefficients(handle, front, 0.0f, nullptr, 2, 0, 0.0f); }), "null coefficients must be refused");
    Require(Throws<std::invalid_argument>([&] { sceAudioOut2GetSpeakerArrayCoefficients(handle, front, 0.0f, gains, 3, 0, 0.0f); }), "a coefficient count other than the speaker count must be refused");
    Require(Throws<std::runtime_error>([&] { sceAudioOut2GetSpeakerArrayCoefficients(handle, front, 0.5f, gains, 2, 0, 0.0f); }), "a spread source must throw");
    Require(Throws<std::runtime_error>([&] { sceAudioOut2GetSpeakerArrayCoefficients(handle, front, 0.0f, gains, 2, 1, 1.0f); }), "a downmix spread radius must throw");
    Require(sceAudioOut2GetSpeakerArrayCoefficients(handle, front, 0.0f, gains, 2, 1, 0.0f) == 0, "height awareness without a downmix spread radius must be accepted");
    const Array corrected({Horizontal(30.0), Horizontal(-30.0)}, false, 1);
    const auto correctedHandle = corrected.handle;
    Require(Throws<std::runtime_error>([&] { sceAudioOut2GetSpeakerArrayCoefficients(correctedHandle, front, 0.0f, gains, 2, 0, 0.0f); }), "virtual speaker correction must throw");
    std::vector<std::uint64_t> memory(sceAudioOut2GetSpeakerArrayMemorySize(2, 0, 0) / sizeof(std::uint64_t) + 1);
    const AudioOut2Position positions[2] = {Horizontal(30.0), Horizontal(-30.0)};
    const VbapParams vbap{positions, 2, 0, memory.data(), memory.size() * sizeof(std::uint64_t), 0, 0.0f, {}};
    const AmbisonicsParams ambisonics{};
    AudioOut2SpeakerArrayHandle destroyed = nullptr;
    Require(sceAudioOut2SpeakerArrayCreate(&destroyed, &vbap, &ambisonics) == 0, "the speaker array must be created");
    Require(sceAudioOut2SpeakerArrayDestroy(destroyed) == 0, "the speaker array must be destroyed");
    Require(Throws<std::invalid_argument>([&] { sceAudioOut2GetSpeakerArrayCoefficients(destroyed, front, 0.0f, gains, 2, 0, 0.0f); }), "a destroyed array must be refused");
}

}

int main() {
    TestStereo();
    TestRing();
    TestSphere();
    TestDome();
    TestSingle();
    TestErrors();
    return 0;
}
