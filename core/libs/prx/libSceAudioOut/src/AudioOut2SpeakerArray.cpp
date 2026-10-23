#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::uint32_t MaxSpeakers = 32;
constexpr std::uint32_t MaxTriplets = MaxSpeakers * (MaxSpeakers - 1) * (MaxSpeakers - 2) / 6;
constexpr std::uint32_t AmbisonicsChannelFlag = 0x40;
constexpr std::uint32_t MaxAmbisonicsOrder = 5;
constexpr std::uint32_t VbapCorrectionNone = 0;
constexpr double Tolerance = 1e-6;
constexpr double Pi = 3.14159265358979323846;

struct SpeakerArray {
    std::uint32_t magic;
    std::uint32_t numSpeakers;
    AudioOut2Position directions[MaxSpeakers];
    std::uint32_t correctionType;
    std::uint8_t is3d;
    std::uint8_t faces[(MaxTriplets + 7) / 8];
};
constexpr std::uint32_t SpeakerArrayMagic = 0x53504b41;

SpeakerArray& Array(AudioOut2SpeakerArrayHandle handle) {
    auto* array = static_cast<SpeakerArray*>(handle);
    if (array == nullptr || array->magic != SpeakerArrayMagic) throw std::invalid_argument("invalid speaker array handle");
    return *array;
}

struct Vector {
    double x;
    double y;
    double z;
};

Vector operator-(const Vector& a, const Vector& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
double Dot(const Vector& a, const Vector& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vector Cross(const Vector& a, const Vector& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

Vector Unit(const AudioOut2Position& position, bool keepHeight) {
    const Vector v{position.x, position.y, keepHeight ? position.z : 0.0};
    const double length = std::sqrt(Dot(v, v));
    return length > Tolerance ? Vector{v.x / length, v.y / length, v.z / length} : Vector{0.0, 0.0, 0.0};
}

bool IsZero(const Vector& v) { return v.x == 0.0 && v.y == 0.0 && v.z == 0.0; }

template<typename TVisit>
void ForEachTriplet(std::uint32_t count, TVisit visit) {
    std::uint32_t index = 0;
    for (std::uint32_t a = 0; a < count; ++a) {
        for (std::uint32_t b = a + 1; b < count; ++b) {
            for (std::uint32_t c = b + 1; c < count; ++c) visit(index++, a, b, c);
        }
    }
}

bool FacesListener(const SpeakerArray& array, std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    const Vector la = Unit(array.directions[a], true);
    const Vector lb = Unit(array.directions[b], true);
    const Vector lc = Unit(array.directions[c], true);
    if (IsZero(la) || IsZero(lb) || IsZero(lc) || std::abs(Dot(la, Cross(lb, lc))) < Tolerance) return false;
    const Vector normal = Cross(lb - la, lc - la);
    const double listenerSide = -Dot(normal, la);
    for (std::uint32_t other = 0; other < array.numSpeakers; ++other) {
        if (other == a || other == b || other == c) continue;
        const Vector lo = Unit(array.directions[other], true);
        if (IsZero(lo)) continue;
        if (Dot(normal, lo - la) * listenerSide < -Tolerance) return false;
    }
    return true;
}

void FindFaces(SpeakerArray& array) {
    ForEachTriplet(array.numSpeakers, [&array](std::uint32_t index, std::uint32_t a, std::uint32_t b, std::uint32_t c) {
        if (FacesListener(array, a, b, c)) array.faces[index / 8] |= static_cast<std::uint8_t>(1u << (index % 8));
    });
}

void Normalize(double* gains, std::uint32_t count) {
    double power = 0.0;
    for (std::uint32_t i = 0; i < count; ++i) power += gains[i] * gains[i];
    const double scale = 1.0 / std::sqrt(power);
    for (std::uint32_t i = 0; i < count; ++i) gains[i] *= scale;
}

bool PanTriplet(const SpeakerArray& array, const Vector& source, double* gains) {
    double best = -Tolerance;
    bool found = false;
    ForEachTriplet(array.numSpeakers, [&](std::uint32_t index, std::uint32_t a, std::uint32_t b, std::uint32_t c) {
        if ((array.faces[index / 8] & (1u << (index % 8))) == 0) return;
        const Vector la = Unit(array.directions[a], true);
        const Vector lb = Unit(array.directions[b], true);
        const Vector lc = Unit(array.directions[c], true);
        const double det = Dot(la, Cross(lb, lc));
        double triplet[3] = {Dot(source, Cross(lb, lc)) / det, Dot(source, Cross(lc, la)) / det, Dot(source, Cross(la, lb)) / det};
        Normalize(triplet, 3);
        const double smallest = std::min({triplet[0], triplet[1], triplet[2]});
        if (smallest <= best) return;
        best = smallest;
        found = true;
        std::fill(gains, gains + array.numSpeakers, 0.0);
        gains[a] = triplet[0];
        gains[b] = triplet[1];
        gains[c] = triplet[2];
    });
    return found;
}

double Azimuth(const Vector& v) {
    const double azimuth = std::atan2(v.y, v.x);
    return azimuth < 0.0 ? azimuth + 2.0 * Pi : azimuth;
}

bool PanPair(const SpeakerArray& array, const AudioOut2Position& position, double* gains) {
    const Vector source = Unit(position, false);
    if (IsZero(source)) return false;
    std::uint32_t order[MaxSpeakers];
    std::uint32_t count = 0;
    for (std::uint32_t speaker = 0; speaker < array.numSpeakers; ++speaker) {
        if (!IsZero(Unit(array.directions[speaker], false))) order[count++] = speaker;
    }
    if (count < 2) return false;
    std::sort(order, order + count, [&array](std::uint32_t a, std::uint32_t b) {
        return Azimuth(Unit(array.directions[a], false)) < Azimuth(Unit(array.directions[b], false));
    });
    const double target = Azimuth(source);
    for (std::uint32_t k = 0; k < count; ++k) {
        const std::uint32_t a = order[k];
        const std::uint32_t b = order[(k + 1) % count];
        const double from = Azimuth(Unit(array.directions[a], false));
        const double span = std::fmod(Azimuth(Unit(array.directions[b], false)) - from + 2.0 * Pi, 2.0 * Pi);
        const double offset = std::fmod(target - from + 2.0 * Pi, 2.0 * Pi);
        if (span < Tolerance || span > Pi - Tolerance || offset > span + Tolerance) continue;
        double pair[2] = {std::max(0.0, std::sin(span - offset)), std::max(0.0, std::sin(offset))};
        Normalize(pair, 2);
        std::fill(gains, gains + array.numSpeakers, 0.0);
        gains[a] = pair[0];
        gains[b] = pair[1];
        return true;
    }
    return false;
}

void PanNearest(const SpeakerArray& array, const AudioOut2Position& position, double* gains) {
    const bool keepHeight = array.is3d != 0;
    const Vector source = Unit(position, keepHeight);
    double closest = 2.0 * Pi;
    double distance[MaxSpeakers];
    for (std::uint32_t speaker = 0; speaker < array.numSpeakers; ++speaker) {
        const Vector direction = Unit(array.directions[speaker], keepHeight);
        distance[speaker] = IsZero(direction) ? 2.0 * Pi : std::acos(std::clamp(Dot(source, direction), -1.0, 1.0));
        closest = std::min(closest, distance[speaker]);
    }
    for (std::uint32_t speaker = 0; speaker < array.numSpeakers; ++speaker) {
        gains[speaker] = distance[speaker] <= closest + Tolerance ? 1.0 : 0.0;
    }
    Normalize(gains, array.numSpeakers);
}

float Sn3d(std::uint32_t acn, const AudioOut2Position& direction) {
    const int n = static_cast<int>(std::sqrt(static_cast<double>(acn)));
    const int m = static_cast<int>(acn) - n * n - n;
    const int am = std::abs(m);
    const double length = std::sqrt(double(direction.x) * direction.x + double(direction.y) * direction.y + double(direction.z) * direction.z);
    const double sinEl = length > 0 ? direction.z / length : 0;
    const double cosEl = std::sqrt(std::max(0.0, 1 - sinEl * sinEl));
    const double azimuth = std::atan2(direction.y, direction.x);
    double legendre = 1;
    for (int i = 1; i <= am; ++i) legendre *= (2 * i - 1) * cosEl;
    if (n > am) {
        double previous = legendre;
        double current = sinEl * (2 * am + 1) * legendre;
        for (int l = am + 2; l <= n; ++l) {
            const double next = (sinEl * (2 * l - 1) * current - (l + am - 1) * previous) / (l - am);
            previous = current;
            current = next;
        }
        legendre = current;
    }
    double ratio = 1;
    for (int i = n - am + 1; i <= n + am; ++i) ratio /= i;
    const double norm = std::sqrt((m == 0 ? 1.0 : 2.0) * ratio);
    return static_cast<float>(norm * legendre * (m < 0 ? std::sin(am * azimuth) : std::cos(am * azimuth)));
}

}

extern "C" {

int APS5_VABI sceAudioOut2SpeakerArrayCreate(AudioOut2SpeakerArrayHandle* handle, const void* vbap_params, const void* ambi_params) {
    static_assert(offsetof(AudioOut2SpeakerArrayParam, vbap_correction) == 0x20);
    const auto* params = static_cast<const AudioOut2SpeakerArrayParam*>(vbap_params);
    if (handle == nullptr || params == nullptr || params->speaker_positions == nullptr ||
        params->num_speakers == 0 || params->num_speakers > MaxSpeakers || params->buffer == nullptr ||
        params->size < sizeof(SpeakerArray) || reinterpret_cast<std::uintptr_t>(params->buffer) % alignof(SpeakerArray) != 0) {
        APS5_INVALID_ARG_EX;
    }
    auto* array = new (params->buffer) SpeakerArray{SpeakerArrayMagic, params->num_speakers, {}, params->vbap_correction.type, params->is_3d, {}};
    std::copy_n(params->speaker_positions, params->num_speakers, array->directions);
    if (array->is3d != 0) FindFaces(*array);
    *handle = array;
    return 0;
}

int APS5_VABI sceAudioOut2SpeakerArrayDestroy(AudioOut2SpeakerArrayHandle handle) {
    Array(handle).magic = 0;
    return 0;
}

int APS5_VABI sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients(AudioOut2SpeakerArrayHandle handle, uint32_t ambisonics_channel, float* coefficients, uint32_t num_coefficients) {
    const auto& array = Array(handle);
    if (coefficients == nullptr || num_coefficients != array.numSpeakers) APS5_INVALID_ARG_EX;
    const std::uint32_t acn = ambisonics_channel & ~AmbisonicsChannelFlag;
    if ((ambisonics_channel & AmbisonicsChannelFlag) == 0 || acn >= (MaxAmbisonicsOrder + 1) * (MaxAmbisonicsOrder + 1)) {
        throw std::invalid_argument("unsupported ambisonics channel " + std::to_string(ambisonics_channel));
    }
    const auto order = static_cast<std::uint32_t>(std::sqrt(static_cast<double>(acn)));
    const std::uint32_t decodable = std::max<std::uint32_t>(1, (array.numSpeakers - 1) / 2);
    for (std::uint32_t speaker = 0; speaker < array.numSpeakers; ++speaker) {
        coefficients[speaker] = order > decodable ? 0.0f : Sn3d(acn, array.directions[speaker]) / static_cast<float>(array.numSpeakers);
    }
    return 0;
}

int APS5_VABI sceAudioOut2GetSpeakerArrayCoefficients(AudioOut2SpeakerArrayHandle handle, AudioOut2Position pos, float spread, float* coefficients, uint32_t num_coefficients, uint8_t height_aware, float downmix_spread_radius) {
    (void)height_aware;
    const auto& array = Array(handle);
    if (coefficients == nullptr || num_coefficients != array.numSpeakers) APS5_INVALID_ARG_EX;
    if (spread != 0.0f || downmix_spread_radius != 0.0f || array.correctionType != VbapCorrectionNone) {
        throw std::runtime_error(std::string(__func__) + ": spread " + std::to_string(spread) + ", downmix spread radius " + std::to_string(downmix_spread_radius) +
                                 " and VBAP correction " + std::to_string(array.correctionType) + " not supported");
    }
    const bool is3d = array.is3d != 0;
    if (IsZero(Unit(pos, is3d))) throw std::runtime_error(std::string(__func__) + ": the source position has no direction");
    double gains[MaxSpeakers] = {};
    if (array.numSpeakers == 1) {
        gains[0] = 1.0;
    } else if (!(is3d && PanTriplet(array, Unit(pos, true), gains)) && !PanPair(array, pos, gains)) {
        PanNearest(array, pos, gains);
    }
    for (std::uint32_t speaker = 0; speaker < array.numSpeakers; ++speaker) coefficients[speaker] = static_cast<float>(gains[speaker]);
    return 0;
}

size_t APS5_VABI sceAudioOut2GetSpeakerArrayMemorySize(uint32_t num_speakers, uint8_t is_3d, uint8_t is_ambisonics) {
    (void)is_3d;
    (void)is_ambisonics;
    if (num_speakers == 0 || num_speakers > MaxSpeakers) APS5_INVALID_ARG_EX;
    return sizeof(SpeakerArray);
}

int APS5_VABI sceAudioOut2GetSpeakerInfo(AudioOut2SpeakerInfo* info, uint32_t flags) {
    (void)flags;
    if (!info) return static_cast<int>(0x80260502);
    constexpr std::uint8_t SpeakerTypeTelevision = 0;
    constexpr std::uint32_t FrontLeftAndRight = 0x3;
    constexpr AudioOut2SpeakerAngle NoSpeaker = {-360, -360};
    *info = AudioOut2SpeakerInfo{};
    info->type = SpeakerTypeTelevision;
    info->available_bits = FrontLeftAndRight;
    for (auto& angle : info->speaker_angle) angle = NoSpeaker;
    info->speaker_angle[0] = {30, 0};
    info->speaker_angle[1] = {-30, 0};
    return 0;
}

}
