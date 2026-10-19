#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "prx/libc/include/General.hpp"
#include "H264Fixture.hpp"

namespace {

struct ConfigInfo {
    std::uint64_t thisSize;
    std::uint32_t codecType;
    std::uint32_t profile;
    std::uint32_t maxLevel;
    std::int32_t maxFrameWidth;
    std::int32_t maxFrameHeight;
    std::int32_t maxDpbFrameCount;
    std::uint64_t videodecFlags;
};

struct ResourceInfo {
    std::uint64_t thisSize;
    std::uint64_t cpuMemorySize;
    void* cpuMemory;
    std::uint64_t cpuGpuMemorySize;
    void* cpuGpuMemory;
    std::uint64_t maxFrameBufferSize;
    std::uint32_t frameBufferAlignment;
};

struct Ctrl {
    std::uint64_t thisSize;
    void* handle;
    std::uint64_t version;
};

struct FrameBuffer {
    std::uint64_t thisSize;
    void* frameBuffer;
    std::uint64_t frameBufferSize;
};

struct AvcInfo {
    std::uint32_t numUnitsInTick;
    std::uint32_t timeScale;
    std::uint8_t fixedFrameRateFlag;
    std::uint8_t aspectRatioIdc;
    std::uint16_t sarWidth;
    std::uint16_t sarHeight;
    std::uint8_t colourPrimaries;
    std::uint8_t transferCharacteristics;
    std::uint8_t matrixCoefficients;
    std::uint8_t videoFullRangeFlag;
    std::uint32_t frameCropLeftOffset;
    std::uint32_t frameCropRightOffset;
    std::uint32_t frameCropTopOffset;
    std::uint32_t frameCropBottomOffset;
};

union CodecInfo {
    std::uint8_t reserved[64];
    AvcInfo avc;
};

struct PictureInfo {
    std::uint64_t thisSize;
    std::uint32_t isValid;
    std::uint32_t codecType;
    std::uint32_t frameWidth;
    std::uint32_t framePitch;
    std::uint32_t frameHeight;
    std::uint32_t isErrorPic;
    std::uint64_t ptsData;
    std::uint64_t attachedData;
    CodecInfo codec;
};

struct InputData {
    std::uint64_t thisSize;
    const std::uint8_t* auData;
    std::uint64_t auSize;
    std::uint64_t ptsData;
    std::uint64_t dtsData;
    std::uint64_t attachedData;
};

struct Videodec2Config {
    std::uint64_t thisSize;
    std::uint32_t resourceType;
    std::uint32_t codecType;
    std::uint32_t profile;
    std::uint32_t maxLevel;
    std::int32_t maxFrameWidth;
    std::int32_t maxFrameHeight;
    std::int32_t maxDpbFrameCount;
    std::uint32_t decodePipelineDepth;
};

struct Videodec2Memory {
    std::uint64_t thisSize;
    std::uint64_t cpuMemorySize;
    void* cpuMemory;
    std::uint64_t gpuMemorySize;
    void* gpuMemory;
    std::uint64_t cpuGpuMemorySize;
    void* cpuGpuMemory;
    std::uint64_t maxFrameBufferSize;
    std::uint32_t frameBufferAlignment;
};

struct Videodec2FrameBuffer {
    std::uint64_t thisSize;
    void* frameBuffer;
    std::uint64_t frameBufferSize;
    bool isAccepted;
};

struct Videodec2OutputInfo {
    std::uint64_t thisSize;
    bool isValid;
    bool isErrorFrame;
    std::uint8_t pictureCount;
    std::uint32_t codecType;
    std::uint32_t frameWidth;
    std::uint32_t framePitch;
    std::uint32_t frameHeight;
    void* frameBuffer;
    std::uint64_t frameBufferSize;
    std::uint32_t frameFormat;
    std::uint32_t framePitchInBytes;
};

static_assert(sizeof(ConfigInfo) == 0x28 && sizeof(ResourceInfo) == 0x38 && sizeof(Ctrl) == 0x18 && sizeof(PictureInfo) == 0x70);

}

extern "C" {
std::int32_t APS5_VABI sceVideodecCreateDecoder(const ConfigInfo* config, const ResourceInfo* resource, Ctrl* ctrl);
std::int32_t APS5_VABI sceVideodecDecode(Ctrl* ctrl, const InputData* input, FrameBuffer* frame, PictureInfo* picture);
std::int32_t APS5_VABI sceVideodecDeleteDecoder(Ctrl* ctrl);
std::int32_t APS5_VABI sceVideodecFlush(Ctrl* ctrl, FrameBuffer* frame, PictureInfo* picture);
std::int32_t APS5_VABI sceVideodecReset(Ctrl* ctrl);
int APS5_VABI sceVideodec2QueryDecoderMemoryInfo_nid_postfix(const Videodec2Config* config, Videodec2Memory* memory);
int APS5_VABI sceVideodec2CreateDecoder_nid_postfix(const Videodec2Config* config, const Videodec2Memory* memory, std::uint64_t* handle);
int APS5_VABI sceVideodec2DeleteDecoder_nid_postfix(std::uint64_t handle);
int APS5_VABI sceVideodec2Decode_nid_postfix(std::uint64_t handle, const InputData* input, Videodec2FrameBuffer* frame, Videodec2OutputInfo* output);
int APS5_VABI sceVideodec2Flush_nid_postfix(std::uint64_t handle, Videodec2FrameBuffer* frame, Videodec2OutputInfo* output);
}

namespace {

using namespace H264Fixture;

constexpr std::int32_t ErrorStructSize = static_cast<std::int32_t>(0x80C10002);
constexpr std::int32_t ErrorHandle = static_cast<std::int32_t>(0x80C10003);
constexpr std::int32_t ErrorAuSize = static_cast<std::int32_t>(0x80C10009);
constexpr std::int32_t ErrorAuPointer = static_cast<std::int32_t>(0x80C1000A);
constexpr std::int32_t ErrorFrameBufferSize = static_cast<std::int32_t>(0x80C1000B);
constexpr std::int32_t ErrorArgumentPointer = static_cast<std::int32_t>(0x80C1000F);
constexpr std::uint32_t Pitch = 64;
constexpr std::uint32_t AlignedHeight = 48;
constexpr std::size_t FrameBytes = Pitch * AlignedHeight * 3 / 2;

void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct Picture {
    std::uint64_t pts = 0;
    std::uint64_t attached = 0;
    std::vector<std::uint8_t> luma;
    std::vector<std::uint8_t> chroma;
};

std::vector<Picture> referencePictures(const std::vector<std::vector<std::uint8_t>>& units) {
    const Videodec2Config config{sizeof(Videodec2Config), 1, 1, 100, 42, static_cast<std::int32_t>(Width), static_cast<std::int32_t>(Height), 4, 1};
    Videodec2Memory memory{sizeof(Videodec2Memory)};
    check(sceVideodec2QueryDecoderMemoryInfo_nid_postfix(&config, &memory) == 0, "reference memory query failed");
    std::uint64_t handle = 0;
    check(sceVideodec2CreateDecoder_nid_postfix(&config, &memory, &handle) == 0, "reference decoder creation failed");
    std::vector<std::uint8_t> buffer(memory.maxFrameBufferSize);
    std::vector<Picture> pictures;
    const auto take = [&](const Videodec2OutputInfo& output, std::uint64_t pts, std::uint64_t attached) {
        if (!output.isValid) return false;
        Picture picture{pts, attached};
        const auto* chroma = buffer.data() + static_cast<std::size_t>(output.framePitchInBytes) * output.frameHeight;
        for (std::uint32_t row = 0; row < Height; ++row) picture.luma.insert(picture.luma.end(), buffer.data() + row * output.framePitchInBytes, buffer.data() + row * output.framePitchInBytes + Width);
        for (std::uint32_t row = 0; row < Height / 2; ++row) picture.chroma.insert(picture.chroma.end(), chroma + row * output.framePitchInBytes, chroma + row * output.framePitchInBytes + Width);
        pictures.push_back(std::move(picture));
        return true;
    };
    for (std::size_t unit = 0; unit < units.size(); ++unit) {
        const InputData input{sizeof(InputData), units[unit].data(), units[unit].size(), 1000 + unit, unit, 0xa0 + unit};
        Videodec2FrameBuffer frame{sizeof(Videodec2FrameBuffer), buffer.data(), buffer.size(), false};
        Videodec2OutputInfo output{sizeof(Videodec2OutputInfo)};
        check(sceVideodec2Decode_nid_postfix(handle, &input, &frame, &output) == 0, "reference decode failed");
        if (output.isValid) take(output, 1000 + DisplayOrderUnits[pictures.size()], 0xa0 + DisplayOrderUnits[pictures.size()]);
    }
    for (;;) {
        Videodec2FrameBuffer frame{sizeof(Videodec2FrameBuffer), buffer.data(), buffer.size(), false};
        Videodec2OutputInfo output{sizeof(Videodec2OutputInfo)};
        check(sceVideodec2Flush_nid_postfix(handle, &frame, &output) == 0, "reference flush failed");
        if (!output.isValid || pictures.size() >= DisplayOrderUnits.size()) break;
        take(output, 1000 + DisplayOrderUnits[pictures.size()], 0xa0 + DisplayOrderUnits[pictures.size()]);
    }
    check(sceVideodec2DeleteDecoder_nid_postfix(handle) == 0, "reference decoder deletion failed");
    return pictures;
}

void checkPicture(const PictureInfo& info, const std::vector<std::uint8_t>& frame, const Picture& reference) {
    check(info.isValid == 1 && info.codecType == 0 && info.isErrorPic == 0, "picture info flags differ");
    check(info.frameWidth == Width && info.framePitch == Pitch && info.frameHeight == AlignedHeight, "picture geometry differs");
    check(info.codec.avc.frameCropLeftOffset == 0 && info.codec.avc.frameCropRightOffset == Pitch - Width && info.codec.avc.frameCropTopOffset == 0 && info.codec.avc.frameCropBottomOffset == AlignedHeight - Height, "picture crop differs");
    check(info.ptsData == reference.pts && info.attachedData == reference.attached, "pictures out of display order");
    const auto* chroma = frame.data() + Pitch * AlignedHeight;
    for (std::uint32_t row = 0; row < AlignedHeight; ++row) {
        const auto source = row < Height ? row : Height - 1;
        check(std::memcmp(frame.data() + row * Pitch, reference.luma.data() + source * Width, Width) == 0, "luma row " + std::to_string(row) + " differs from the reference");
    }
    for (std::uint32_t row = 0; row < AlignedHeight / 2; ++row) {
        const auto source = row < Height / 2 ? row : Height / 2 - 1;
        check(std::memcmp(chroma + row * Pitch, reference.chroma.data() + source * Width, Width) == 0, "chroma row " + std::to_string(row) + " differs from the reference");
    }
}

void testDecode() {
    const auto units = accessUnits(false);
    const auto reference = referencePictures(units);
    check(reference.size() == PictureHashes.size(), "the reference decode lost pictures");

    const ConfigInfo config{sizeof(ConfigInfo), 0, 100, 42, static_cast<std::int32_t>(Width), static_cast<std::int32_t>(Height), 4, 0};
    const ResourceInfo resource{sizeof(ResourceInfo)};
    Ctrl ctrl{};
    check(sceVideodecCreateDecoder(nullptr, &resource, &ctrl) == ErrorArgumentPointer, "a null config was accepted");
    check(sceVideodecCreateDecoder(&config, &resource, nullptr) == ErrorArgumentPointer, "a null ctrl was accepted");
    ConfigInfo wrongConfig = config;
    wrongConfig.thisSize = sizeof(ConfigInfo) - 8;
    check(sceVideodecCreateDecoder(&wrongConfig, &resource, &ctrl) == ErrorStructSize, "a short config was accepted");
    check(sceVideodecCreateDecoder(&config, &resource, &ctrl) == 0 && ctrl.thisSize == sizeof(Ctrl) && ctrl.handle != nullptr && ctrl.version == 1, "decoder creation failed");

    std::vector<std::uint8_t> frameBytes(FrameBytes);
    FrameBuffer frame{sizeof(FrameBuffer), frameBytes.data(), frameBytes.size()};
    PictureInfo info{sizeof(PictureInfo)};
    const InputData empty{sizeof(InputData), nullptr, 16, 0, 0, 0};
    check(sceVideodecDecode(&ctrl, &empty, &frame, &info) == ErrorAuPointer, "a null access unit was accepted");
    const InputData zero{sizeof(InputData), units[0].data(), 0, 0, 0, 0};
    check(sceVideodecDecode(&ctrl, &zero, &frame, &info) == ErrorAuSize, "an empty access unit was accepted");
    check(sceVideodecDecode(&ctrl, &zero, nullptr, &info) == ErrorArgumentPointer, "a null frame buffer was accepted");
    FrameBuffer wrongFrame{sizeof(FrameBuffer) + 8, frameBytes.data(), frameBytes.size()};
    check(sceVideodecDecode(&ctrl, &zero, &wrongFrame, &info) == ErrorStructSize, "a frame buffer of the wrong size was accepted");

    std::size_t pictures = 0;
    for (std::size_t unit = 0; unit < units.size(); ++unit) {
        const InputData input{sizeof(InputData), units[unit].data(), units[unit].size(), 1000 + unit, unit, 0xa0 + unit};
        info = PictureInfo{sizeof(PictureInfo)};
        info.isValid = 7;
        check(sceVideodecDecode(&ctrl, &input, &frame, &info) == 0, "decode failed");
        if (info.isValid == 0) continue;
        check(pictures < reference.size(), "more pictures than access units");
        checkPicture(info, frameBytes, reference[pictures]);
        ++pictures;
    }
    for (;;) {
        info = PictureInfo{sizeof(PictureInfo)};
        info.isValid = 7;
        check(sceVideodecFlush(&ctrl, &frame, &info) == 0, "flush failed");
        if (info.isValid == 0) break;
        check(pictures < reference.size(), "more pictures than access units after a flush");
        checkPicture(info, frameBytes, reference[pictures]);
        ++pictures;
    }
    check(pictures == reference.size(), "pictures were lost");
    PictureInfo wrongInfo{sizeof(PictureInfo) - 8};
    check(sceVideodecFlush(&ctrl, &frame, &wrongInfo) == ErrorStructSize, "a short picture info was accepted by flush");

    check(sceVideodecReset(&ctrl) == 0, "reset failed");
    std::vector<std::uint8_t> small(FrameBytes - 1);
    FrameBuffer smallFrame{sizeof(FrameBuffer), small.data(), small.size()};
    std::int32_t smallResult = 0;
    for (std::size_t unit = 0; unit < units.size() && smallResult == 0; ++unit) {
        const InputData input{sizeof(InputData), units[unit].data(), units[unit].size(), 1000 + unit, unit, 0xa0 + unit};
        smallResult = sceVideodecDecode(&ctrl, &input, &smallFrame, &info);
    }
    check(smallResult == ErrorFrameBufferSize, "a frame buffer smaller than the picture was accepted");

    check(sceVideodecDeleteDecoder(&ctrl) == 0, "delete failed");
    check(sceVideodecDeleteDecoder(&ctrl) == ErrorHandle, "a deleted decoder was deleted again");
    check(sceVideodecReset(&ctrl) == ErrorHandle, "a deleted decoder was reset");
    check(sceVideodecDecode(&ctrl, &zero, &frame, &info) == ErrorHandle, "a deleted decoder decoded");
    check(sceVideodecFlush(&ctrl, &frame, &info) == ErrorHandle, "a deleted decoder flushed");
    Ctrl none{sizeof(Ctrl), nullptr, 1};
    check(sceVideodecDeleteDecoder(&none) == ErrorHandle, "a null handle was deleted");
    check(sceVideodecReset(nullptr) == ErrorArgumentPointer, "a null ctrl was reset");
}

}

int main() {
    try {
        testDecode();
        std::puts("Videodec tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
