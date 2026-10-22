#include "Ngs2Test.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

extern "C" {
int APS5_VABI sceNgs2RackSetUserData(uintptr_t, uintptr_t);
int APS5_VABI sceNgs2RackGetUserData(uintptr_t, uintptr_t*);
int APS5_VABI sceNgs2VoiceGetOwner(uintptr_t, uintptr_t*, std::uint32_t*);
int APS5_VABI sceNgs2VoiceGetMatrixInfo(uintptr_t, std::uint32_t, Ngs2VoiceMatrixInfo*, size_t);
int APS5_VABI sceNgs2CustomRackGetModuleInfo(uintptr_t, std::uint32_t, Ngs2CustomModuleInfo*, size_t);
int APS5_VABI sceNgs2ParseWaveformUser(Ngs2ParseReadHandler, uintptr_t, Ngs2WaveformInfo*);
int APS5_VABI sceNgs2GetWaveformFrameInfo(const Ngs2WaveformFormat*, std::uint32_t*, std::uint32_t*, std::uint32_t*, std::uint32_t*);
}

template <typename TException, typename TAction>
static bool Throws(TAction action) {
    try {
        action();
    } catch (const TException&) {
        return true;
    }
    return false;
}

static void TestRackUserData() {
    const auto system = CreateSystem();
    const auto rack = CreateRack(system, SCE_NGS2_RACK_ID_SAMPLER);
    uintptr_t userData = 0xdead;
    Require(sceNgs2RackGetUserData(rack, &userData) == SCE_NGS2_OK && userData == 0);
    Require(sceNgs2RackSetUserData(rack, 0x1234) == SCE_NGS2_OK);
    Require(sceNgs2RackGetUserData(rack, &userData) == SCE_NGS2_OK && userData == 0x1234);
    Require(sceNgs2SystemSetUserData(system, 0x99) == SCE_NGS2_OK);
    Require(sceNgs2RackGetUserData(rack, &userData) == SCE_NGS2_OK && userData == 0x1234);
    Require(sceNgs2RackSetUserData(system, 1) == SCE_NGS2_ERROR_INVALID_RACK_HANDLE);
    Require(sceNgs2RackGetUserData(0x1234, &userData) == SCE_NGS2_ERROR_INVALID_RACK_HANDLE);
    Require(Throws<std::invalid_argument>([&] { sceNgs2RackGetUserData(rack, nullptr); }));
    Require(sceNgs2RackDestroy(rack, nullptr) == SCE_NGS2_OK);
    Require(sceNgs2RackGetUserData(rack, &userData) == SCE_NGS2_ERROR_INVALID_RACK_HANDLE);
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
}

static void TestVoiceOwner() {
    const auto system = CreateSystem();
    const auto rack = CreateRack(system, SCE_NGS2_RACK_ID_SAMPLER);
    uintptr_t voice = 0;
    Require(sceNgs2RackGetVoiceHandle(rack, 3, &voice) == SCE_NGS2_OK);
    uintptr_t owner = 0;
    std::uint32_t index = 0xffffffff;
    Require(sceNgs2VoiceGetOwner(voice, &owner, &index) == SCE_NGS2_OK && owner == rack && index == 3);
    Require(sceNgs2VoiceGetOwner(Voice(rack), &owner, &index) == SCE_NGS2_OK && owner == rack && index == 0);
    Require(sceNgs2VoiceGetOwner(voice, nullptr, &index) == SCE_NGS2_ERROR_INVALID_OUT_ADDRESS);
    Require(sceNgs2VoiceGetOwner(voice, &owner, nullptr) == SCE_NGS2_ERROR_INVALID_OUT_ADDRESS);
    Require(sceNgs2VoiceGetOwner(rack, &owner, &index) == SCE_NGS2_ERROR_INVALID_VOICE_HANDLE);
    Require(sceNgs2RackDestroy(rack, nullptr) == SCE_NGS2_OK);
    Require(sceNgs2VoiceGetOwner(voice, &owner, &index) == SCE_NGS2_ERROR_INVALID_VOICE_HANDLE);
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
}

static void TestVoiceMatrixInfo() {
    const auto system = CreateSystem();
    const auto voice = Voice(CreateRack(system, SCE_NGS2_RACK_ID_SAMPLER));
    Ngs2VoiceMatrixInfo info{};
    info.num_levels = 0xffffffff;
    Require(sceNgs2VoiceGetMatrixInfo(voice, 0, &info, sizeof(info)) == SCE_NGS2_OK && info.num_levels == 0);
    const float levels[4] = {1.0f, 0.5f, 0.25f, 8.0f};
    Control(voice, SCE_NGS2_VOICE_PARAM_MATRIX_LEVELS, Ngs2VoiceMatrixLevelsParam{{}, 0, 4, levels});
    std::fill(std::begin(info.levels), std::end(info.levels), -1.0f);
    Require(sceNgs2VoiceGetMatrixInfo(voice, 0, &info, sizeof(info)) == SCE_NGS2_OK && info.num_levels == 4);
    Require(info.levels[0] == 1.0f && info.levels[1] == 0.5f && info.levels[2] == 0.25f && info.levels[3] == 4.0f);
    Require(std::all_of(info.levels + 4, std::end(info.levels), [](float level) { return level == 0.0f; }));
    Require(sceNgs2VoiceGetMatrixInfo(voice, 0, nullptr, sizeof(info)) == SCE_NGS2_ERROR_INVALID_OUT_ADDRESS);
    Require(sceNgs2VoiceGetMatrixInfo(voice, 0, &info, sizeof(info) - 4) == SCE_NGS2_ERROR_INVALID_OUT_SIZE);
    Require(sceNgs2VoiceGetMatrixInfo(system, 0, &info, sizeof(info)) == SCE_NGS2_ERROR_INVALID_VOICE_HANDLE);
    Require(Throws<std::invalid_argument>([&] { sceNgs2VoiceGetMatrixInfo(voice, 1, &info, sizeof(info)); }));
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
}

static void TestCustomModuleInfo() {
    const auto system = CreateSystem();
    Ngs2CustomSubmixerRackOption option{};
    auto& common = option.custom_rack_option.rack_option;
    common.size = sizeof(option);
    common.max_grain_samples = 512;
    common.max_voices = 1;
    common.max_matrices = 1;
    common.max_ports = 1;
    option.custom_rack_option.num_buffers = 1;
    option.custom_rack_option.num_modules = 2;
    option.custom_rack_option.module[0].module_id = SCE_NGS2_CUSTOM_MODULE_ID_FILTER;
    option.custom_rack_option.module[1].module_id = SCE_NGS2_CUSTOM_MODULE_ID_USER_FX;
    option.custom_rack_option.module[1].extra_buffer_id = 0;
    option.custom_rack_option.module[1].state_offset = 0x40;
    option.custom_rack_option.module[1].state_size = 0x20;
    option.custom_rack_option.module[1].reserved = 7;
    option.max_channels = 2;
    option.max_inputs = 1;
    Ngs2ContextBufferInfo query{};
    Require(sceNgs2RackQueryBufferSize(SCE_NGS2_RACK_ID_CUSTOM_SUBMIXER, &common, &query) == SCE_NGS2_OK);
    const auto buffer = Buffer(query);
    uintptr_t rack = 0;
    Require(sceNgs2RackCreate(system, SCE_NGS2_RACK_ID_CUSTOM_SUBMIXER, &common, &buffer, &rack) == SCE_NGS2_OK);
    Ngs2CustomModuleInfo info{};
    std::memset(&info, 0xa5, sizeof(info));
    Require(sceNgs2CustomRackGetModuleInfo(rack, 0, &info, sizeof(info)) == SCE_NGS2_OK);
    Require(info.module_id == SCE_NGS2_CUSTOM_MODULE_ID_FILTER && info.source_buffer_id == 0 && info.extra_buffer_id == 0 && info.dest_buffer_id == 0);
    Require(info.state_offset == 0 && info.state_size == 0 && info.reserved == 0 && info.reserved2 == 0);
    Require(sceNgs2CustomRackGetModuleInfo(rack, 1, &info, sizeof(info)) == SCE_NGS2_OK);
    Require(info.module_id == SCE_NGS2_CUSTOM_MODULE_ID_USER_FX && info.state_offset == 0x40 && info.state_size == 0x20 && info.reserved == 0);
    Require(sceNgs2CustomRackGetModuleInfo(rack, 0, nullptr, sizeof(info)) == SCE_NGS2_ERROR_INVALID_OUT_ADDRESS);
    Require(sceNgs2CustomRackGetModuleInfo(rack, 0, &info, sizeof(info) + 8) == SCE_NGS2_ERROR_INVALID_OUT_SIZE);
    Require(sceNgs2CustomRackGetModuleInfo(system, 0, &info, sizeof(info)) == SCE_NGS2_ERROR_INVALID_RACK_HANDLE);
    Require(Throws<std::invalid_argument>([&] { sceNgs2CustomRackGetModuleInfo(rack, 2, &info, sizeof(info)); }));
    const auto sampler = CreateRack(system, SCE_NGS2_RACK_ID_SAMPLER);
    Require(Throws<std::runtime_error>([&] { sceNgs2CustomRackGetModuleInfo(sampler, 0, &info, sizeof(info)); }));
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
}

static void Put16(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}

static void Put32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    Put16(out, value & 0xffff);
    Put16(out, value >> 16);
}

static std::vector<std::uint8_t> PcmFile(std::uint32_t channels, std::uint32_t sampleRate, std::uint32_t frames) {
    const std::uint32_t frameBytes = channels * 2;
    std::vector<std::uint8_t> file;
    file.insert(file.end(), {'R', 'I', 'F', 'F'});
    Put32(file, 36 + frames * frameBytes);
    file.insert(file.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    Put32(file, 16);
    Put16(file, 1);
    Put16(file, channels);
    Put32(file, sampleRate);
    Put32(file, sampleRate * frameBytes);
    Put16(file, frameBytes);
    Put16(file, 16);
    file.insert(file.end(), {'d', 'a', 't', 'a'});
    Put32(file, frames * frameBytes);
    file.resize(file.size() + frames * frameBytes, 0x11);
    return file;
}

struct Stream {
    std::vector<std::uint8_t> bytes;
    bool returnCount = false;
    std::int32_t failure = 0;
    std::uint32_t reads = 0;
};

static std::int32_t APS5_VABI Read(uintptr_t userData, std::uint32_t offset, void* data, size_t size) {
    auto& stream = *reinterpret_cast<Stream*>(userData);
    stream.reads++;
    if (stream.failure != 0) return stream.failure;
    if (offset > stream.bytes.size() || size > stream.bytes.size() - offset) return -1;
    std::memcpy(data, stream.bytes.data() + offset, size);
    return stream.returnCount ? static_cast<std::int32_t>(size) : 0;
}

static void TestParseWaveformUser() {
    Stream stream{PcmFile(2, 44100, 100)};
    Ngs2WaveformInfo expected{};
    Require(sceNgs2ParseWaveformData(stream.bytes.data(), stream.bytes.size(), &expected) == SCE_NGS2_OK);
    Ngs2WaveformInfo info{};
    Require(sceNgs2ParseWaveformUser(Read, reinterpret_cast<uintptr_t>(&stream), &info) == SCE_NGS2_OK && stream.reads != 0);
    Require(std::memcmp(&info, &expected, sizeof(info)) == 0);
    Require(info.format.num_channels == 2 && info.data_offset == 44 && info.data_size == 400 && info.num_samples == 100);
    stream.returnCount = true;
    std::memset(&info, 0xa5, sizeof(info));
    Require(sceNgs2ParseWaveformUser(Read, reinterpret_cast<uintptr_t>(&stream), &info) == SCE_NGS2_OK);
    Require(std::memcmp(&info, &expected, sizeof(info)) == 0);

    Stream notRiff{std::vector<std::uint8_t>(64, 0)};
    Require(sceNgs2ParseWaveformUser(Read, reinterpret_cast<uintptr_t>(&notRiff), &info) == SCE_NGS2_ERROR_UNKNOWN_WAVEFORM_FORMAT);
    Stream truncated{stream.bytes};
    truncated.bytes.resize(24);
    Require(Throws<std::runtime_error>([&] { sceNgs2ParseWaveformUser(Read, reinterpret_cast<uintptr_t>(&truncated), &info); }));
    Stream failing{stream.bytes};
    failing.failure = -5;
    Require(Throws<std::runtime_error>([&] { sceNgs2ParseWaveformUser(Read, reinterpret_cast<uintptr_t>(&failing), &info); }));
    Require(sceNgs2ParseWaveformUser(nullptr, 0, &info) == SCE_NGS2_ERROR_INVALID_WAVEFORM_DATA);
    Require(sceNgs2ParseWaveformUser(Read, reinterpret_cast<uintptr_t>(&stream), nullptr) == SCE_NGS2_ERROR_INVALID_OUT_ADDRESS);
}

static void TestWaveformFrameInfo() {
    const Ngs2WaveformFormat stereo{SCE_NGS2_WAVEFORM_TYPE_PCM_I16L, 2, 48000, 0, 0, 0};
    std::uint32_t frameSize = 0, frameSamples = 0, units = 0, delay = 0xffffffff;
    Require(sceNgs2GetWaveformFrameInfo(&stereo, &frameSize, &frameSamples, &units, &delay) == SCE_NGS2_OK);
    Require(frameSize == 4 && frameSamples == 1 && units == 1 && delay == 0);
    const Ngs2WaveformFormat surround{SCE_NGS2_WAVEFORM_TYPE_PCM_I16L, 6, 44100, 0, 0, 0};
    Require(sceNgs2GetWaveformFrameInfo(&surround, &frameSize, &frameSamples, &units, &delay) == SCE_NGS2_OK && frameSize == 12);
    Require(sceNgs2GetWaveformFrameInfo(&stereo, nullptr, &frameSamples, &units, &delay) == SCE_NGS2_ERROR_INVALID_OUT_ADDRESS);
    Require(sceNgs2GetWaveformFrameInfo(&stereo, &frameSize, &frameSamples, &units, nullptr) == SCE_NGS2_ERROR_INVALID_OUT_ADDRESS);
    Require(sceNgs2GetWaveformFrameInfo(nullptr, &frameSize, &frameSamples, &units, &delay) == SCE_NGS2_ERROR_INVALID_WAVEFORM_FORMAT);
    const Ngs2WaveformFormat noChannels{SCE_NGS2_WAVEFORM_TYPE_PCM_I16L, 0, 48000, 0, 0, 0};
    Require(sceNgs2GetWaveformFrameInfo(&noChannels, &frameSize, &frameSamples, &units, &delay) == SCE_NGS2_ERROR_INVALID_WAVEFORM_FORMAT);
    const Ngs2WaveformFormat atrac9{SCE_NGS2_WAVEFORM_TYPE_ATRAC9, 2, 48000, 0, 0, 0};
    Require(Throws<std::runtime_error>([&] { sceNgs2GetWaveformFrameInfo(&atrac9, &frameSize, &frameSamples, &units, &delay); }));
}

int main() {
    TestRackUserData();
    TestVoiceOwner();
    TestVoiceMatrixInfo();
    TestCustomModuleInfo();
    TestParseWaveformUser();
    TestWaveformFrameInfo();
    return 0;
}
