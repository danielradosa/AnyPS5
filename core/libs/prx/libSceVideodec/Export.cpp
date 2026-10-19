#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t VIDEODEC_ERROR_STRUCT_SIZE = static_cast<std::int32_t>(0x80C10002);
constexpr std::int32_t VIDEODEC_ERROR_HANDLE = static_cast<std::int32_t>(0x80C10003);
constexpr std::int32_t VIDEODEC_ERROR_AU_SIZE = static_cast<std::int32_t>(0x80C10009);
constexpr std::int32_t VIDEODEC_ERROR_AU_POINTER = static_cast<std::int32_t>(0x80C1000A);
constexpr std::int32_t VIDEODEC_ERROR_FRAME_BUFFER_SIZE = static_cast<std::int32_t>(0x80C1000B);
constexpr std::int32_t VIDEODEC_ERROR_FRAME_BUFFER_POINTER = static_cast<std::int32_t>(0x80C1000C);
constexpr std::int32_t VIDEODEC_ERROR_ARGUMENT_POINTER = static_cast<std::int32_t>(0x80C1000F);
constexpr std::uint32_t Videodec2CodecAvc = 1;
constexpr std::uint32_t Videodec2ResourceCompute = 1;
constexpr std::uint32_t CodecTypeAvc = 0;
constexpr std::uint64_t CtrlVersion = 1;

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

static_assert(sizeof(ConfigInfo) == 0x28 && sizeof(ResourceInfo) == 0x38 && sizeof(Ctrl) == 0x18 && sizeof(FrameBuffer) == 0x18);
static_assert(sizeof(AvcInfo) == 0x24 && sizeof(PictureInfo) == 0x70 && offsetof(PictureInfo, ptsData) == 0x20 && offsetof(PictureInfo, codec) == 0x30);
static_assert(sizeof(InputData) == 0x30);

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

struct Videodec2Crop {
    std::uint32_t left;
    std::uint32_t right;
    std::uint32_t top;
    std::uint32_t bottom;
};

struct Videodec2PictureHeader {
    std::uint64_t pts;
    std::uint64_t dts;
    std::uint64_t attached;
    bool keyFrame;
    bool hevcIdr;
    std::uint32_t profile;
    std::uint32_t level;
    Videodec2Crop crop;
};

static_assert(sizeof(Videodec2Config) == 0x28 && sizeof(Videodec2OutputInfo) == 0x38);

}

extern "C" {
int APS5_VABI sceVideodec2QueryDecoderMemoryInfo_nid_postfix(const Videodec2Config* config, Videodec2Memory* memory);
int APS5_VABI sceVideodec2CreateDecoder_nid_postfix(const Videodec2Config* config, const Videodec2Memory* memory, std::uint64_t* handle);
int APS5_VABI sceVideodec2DeleteDecoder_nid_postfix(std::uint64_t handle);
int APS5_VABI sceVideodec2Decode_nid_postfix(std::uint64_t handle, const InputData* input, Videodec2FrameBuffer* frame, Videodec2OutputInfo* output);
int APS5_VABI sceVideodec2Flush_nid_postfix(std::uint64_t handle, Videodec2FrameBuffer* frame, Videodec2OutputInfo* output);
int APS5_VABI sceVideodec2Reset_nid_postfix(std::uint64_t handle);
void Videodec2PictureHeader_nid_no_patch(std::uint64_t handle, const void* frameBuffer, Videodec2PictureHeader* header);
}

namespace {

struct Decoder {
    std::mutex mutex;
    std::uint64_t handle = 0;
    std::vector<std::uint8_t> scratch;
};

std::mutex g_decodersMutex;
std::set<Decoder*> g_decoders;

std::uint32_t AlignUp(std::uint32_t value, std::uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

void WorstCaseDimensions(const ConfigInfo& config, std::int32_t& width, std::int32_t& height) {
    if (config.maxFrameWidth > 0 && config.maxFrameHeight > 0) {
        width = config.maxFrameWidth;
        height = config.maxFrameHeight;
        return;
    }
    width = config.maxLevel >= 150 ? 3840 : 1920;
    height = config.maxLevel >= 150 ? 2160 : 1080;
}

std::unique_lock<std::mutex> Lock(const Ctrl* ctrl, Decoder*& decoder) {
    std::lock_guard registry(g_decodersMutex);
    decoder = static_cast<Decoder*>(ctrl->handle);
    if (decoder == nullptr || !g_decoders.contains(decoder)) {
        decoder = nullptr;
        return {};
    }
    return std::unique_lock(decoder->mutex);
}

std::int32_t Deliver(Decoder& decoder, const Videodec2OutputInfo& output, FrameBuffer& frame, PictureInfo& picture) {
    if (!output.isValid) return 0;
    if (output.framePitchInBytes != output.framePitch) throw std::runtime_error("Videodec: output deeper than 8 bits is not implemented");
    Videodec2PictureHeader header{};
    Videodec2PictureHeader_nid_no_patch(decoder.handle, output.frameBuffer, &header);
    const auto& crop = header.crop;
    const std::uint32_t width = output.frameWidth - crop.left - crop.right;
    const std::uint32_t height = output.frameHeight - crop.top - crop.bottom;
    const std::uint32_t pitch = AlignUp(width, 64);
    const std::uint32_t alignedHeight = AlignUp(height, 16);
    if (frame.frameBuffer == nullptr) return VIDEODEC_ERROR_FRAME_BUFFER_POINTER;
    if (frame.frameBufferSize < static_cast<std::uint64_t>(pitch) * alignedHeight * 3 / 2) return VIDEODEC_ERROR_FRAME_BUFFER_SIZE;
    const auto* source = decoder.scratch.data();
    const std::size_t sourcePitch = output.framePitchInBytes;
    const auto* sourceChroma = source + sourcePitch * output.frameHeight;
    auto* luma = static_cast<std::uint8_t*>(frame.frameBuffer);
    auto* chroma = luma + static_cast<std::size_t>(pitch) * alignedHeight;
    for (std::uint32_t row = 0; row < alignedHeight; ++row) {
        const std::uint32_t from = std::min(row, height - 1) + crop.top;
        std::memcpy(luma + static_cast<std::size_t>(row) * pitch, source + from * sourcePitch + crop.left, width);
    }
    for (std::uint32_t row = 0; row < alignedHeight / 2; ++row) {
        const std::uint32_t from = std::min(row, height / 2 - 1) + crop.top / 2;
        std::memcpy(chroma + static_cast<std::size_t>(row) * pitch, sourceChroma + from * sourcePitch + crop.left, width);
    }
    picture.isValid = 1;
    picture.codecType = CodecTypeAvc;
    picture.frameWidth = AlignUp(width, 16);
    picture.framePitch = pitch;
    picture.frameHeight = alignedHeight;
    picture.isErrorPic = output.isErrorFrame ? 1 : 0;
    picture.ptsData = header.pts;
    picture.attachedData = header.attached;
    std::memset(&picture.codec, 0, sizeof(picture.codec));
    picture.codec.avc.frameCropRightOffset = pitch - width;
    picture.codec.avc.frameCropBottomOffset = alignedHeight - height;
    return 0;
}

}

extern "C" {

std::int32_t APS5_VABI sceVideodecCreateDecoder(const ConfigInfo* pCfgInfoIn, const ResourceInfo* pRsrcInfoIn, Ctrl* pCtrlOut) {
    if (pCfgInfoIn == nullptr || pRsrcInfoIn == nullptr || pCtrlOut == nullptr) return VIDEODEC_ERROR_ARGUMENT_POINTER;
    if (pCfgInfoIn->thisSize != sizeof(ConfigInfo) || pRsrcInfoIn->thisSize != sizeof(ResourceInfo)) return VIDEODEC_ERROR_STRUCT_SIZE;
    std::int32_t width = 0;
    std::int32_t height = 0;
    WorstCaseDimensions(*pCfgInfoIn, width, height);
    const Videodec2Config config{sizeof(Videodec2Config), Videodec2ResourceCompute, Videodec2CodecAvc, pCfgInfoIn->profile, pCfgInfoIn->maxLevel, width, height, pCfgInfoIn->maxDpbFrameCount, 1};
    Videodec2Memory memory{sizeof(Videodec2Memory)};
    sceVideodec2QueryDecoderMemoryInfo_nid_postfix(&config, &memory);
    auto decoder = std::make_unique<Decoder>();
    decoder->scratch.resize(memory.maxFrameBufferSize);
    if (const int result = sceVideodec2CreateDecoder_nid_postfix(&config, &memory, &decoder->handle); result != 0) return result;
    std::lock_guard registry(g_decodersMutex);
    pCtrlOut->thisSize = sizeof(Ctrl);
    pCtrlOut->handle = decoder.get();
    pCtrlOut->version = CtrlVersion;
    g_decoders.insert(decoder.release());
    return 0;
}

std::int32_t APS5_VABI sceVideodecDecode(Ctrl* pCtrlIn, const InputData* pInputDataIn, FrameBuffer* pFrameBufferInOut, PictureInfo* pPictureInfoOut) {
    if (pCtrlIn == nullptr || pInputDataIn == nullptr || pFrameBufferInOut == nullptr || pPictureInfoOut == nullptr) return VIDEODEC_ERROR_ARGUMENT_POINTER;
    if (pCtrlIn->thisSize != sizeof(Ctrl) || pFrameBufferInOut->thisSize != sizeof(FrameBuffer)) return VIDEODEC_ERROR_STRUCT_SIZE;
    Decoder* decoder = nullptr;
    const auto lock = Lock(pCtrlIn, decoder);
    if (decoder == nullptr) return VIDEODEC_ERROR_HANDLE;
    pPictureInfoOut->isValid = 0;
    if (pInputDataIn->auData == nullptr) return VIDEODEC_ERROR_AU_POINTER;
    if (pInputDataIn->auSize == 0) return VIDEODEC_ERROR_AU_SIZE;
    Videodec2FrameBuffer frame{sizeof(Videodec2FrameBuffer), decoder->scratch.data(), decoder->scratch.size(), false};
    Videodec2OutputInfo output{sizeof(Videodec2OutputInfo)};
    if (const int result = sceVideodec2Decode_nid_postfix(decoder->handle, pInputDataIn, &frame, &output); result != 0) return result;
    return Deliver(*decoder, output, *pFrameBufferInOut, *pPictureInfoOut);
}

std::int32_t APS5_VABI sceVideodecDeleteDecoder(Ctrl* pCtrlIn) {
    if (pCtrlIn == nullptr) return VIDEODEC_ERROR_ARGUMENT_POINTER;
    std::unique_ptr<Decoder> decoder;
    {
        std::lock_guard registry(g_decodersMutex);
        auto* found = static_cast<Decoder*>(pCtrlIn->handle);
        if (found == nullptr || g_decoders.erase(found) == 0) return VIDEODEC_ERROR_HANDLE;
        decoder.reset(found);
    }
    std::lock_guard lock(decoder->mutex);
    return sceVideodec2DeleteDecoder_nid_postfix(decoder->handle);
}

std::int32_t APS5_VABI sceVideodecFlush(Ctrl* pCtrlIn, FrameBuffer* pFrameBufferInOut, PictureInfo* pPictureInfoOut) {
    if (pCtrlIn == nullptr || pFrameBufferInOut == nullptr || pPictureInfoOut == nullptr) return VIDEODEC_ERROR_ARGUMENT_POINTER;
    if (pFrameBufferInOut->thisSize != sizeof(FrameBuffer) || pPictureInfoOut->thisSize != sizeof(PictureInfo)) return VIDEODEC_ERROR_STRUCT_SIZE;
    Decoder* decoder = nullptr;
    const auto lock = Lock(pCtrlIn, decoder);
    if (decoder == nullptr) return VIDEODEC_ERROR_HANDLE;
    pPictureInfoOut->isValid = 0;
    Videodec2FrameBuffer frame{sizeof(Videodec2FrameBuffer), decoder->scratch.data(), decoder->scratch.size(), false};
    Videodec2OutputInfo output{sizeof(Videodec2OutputInfo)};
    if (const int result = sceVideodec2Flush_nid_postfix(decoder->handle, &frame, &output); result != 0) return result;
    return Deliver(*decoder, output, *pFrameBufferInOut, *pPictureInfoOut);
}

std::int32_t APS5_VABI sceVideodecReset(Ctrl* pCtrlIn) {
    if (pCtrlIn == nullptr) return VIDEODEC_ERROR_ARGUMENT_POINTER;
    Decoder* decoder = nullptr;
    const auto lock = Lock(pCtrlIn, decoder);
    if (decoder == nullptr) return VIDEODEC_ERROR_HANDLE;
    return sceVideodec2Reset_nid_postfix(decoder->handle);
}

}
