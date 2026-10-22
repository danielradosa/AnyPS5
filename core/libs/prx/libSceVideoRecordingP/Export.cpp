#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <limits>
#include <mutex>
#include <stdexcept>

struct VideoRecordingContext;

struct VideoRecordingQueryParam {
    std::uint64_t size;
    std::uint32_t reserved8;
    std::int32_t mode;
    std::uint8_t reserved16[12];
    std::uint32_t option;
    std::uint8_t reserved32[32];
};

static_assert(sizeof(VideoRecordingQueryParam) == 64);
static_assert(offsetof(VideoRecordingQueryParam, mode) == 12);
static_assert(offsetof(VideoRecordingQueryParam, option) == 28);


struct VideoRecordingSupportInfo {
    std::uint8_t data[18];
};

static_assert(sizeof(VideoRecordingSupportInfo) == 18);

struct VideoRecordingParam {
    std::uint64_t size;
    std::int32_t threadPriority;
    std::uint32_t duration;
    std::uint32_t mode;
    std::int32_t outputMode;
    std::uint32_t option;
    std::uint32_t format;
};

struct VideoRecordingParam2 {
    std::uint64_t size;
    std::uint64_t affinityMask;
    std::int32_t threadPriority;
    std::uint32_t duration;
    std::uint32_t mode;
    std::int32_t outputMode;
    std::uint32_t option;
    std::uint32_t format;
};

static_assert(sizeof(VideoRecordingParam) == 32);
static_assert(offsetof(VideoRecordingParam, duration) == 12);
static_assert(offsetof(VideoRecordingParam, format) == 28);
static_assert(sizeof(VideoRecordingParam2) == 40);
static_assert(offsetof(VideoRecordingParam2, threadPriority) == 16);
static_assert(offsetof(VideoRecordingParam2, duration) == 20);
static_assert(offsetof(VideoRecordingParam2, format) == 36);

namespace {
constexpr int32_t SCE_VIDEO_RECORDING_ERROR_INVALID_STATE = static_cast<int32_t>(0x80A80006);
constexpr int32_t SCE_VIDEO_RECORDING_ERROR_UNSUPPORTED = static_cast<int32_t>(0x80A80008);
constexpr int32_t SCE_VIDEO_RECORDING_STATUS_NONE = 0;
constexpr int32_t SCE_VIDEO_RECORDING_STATUS_RUNNING = 1;
constexpr int32_t SCE_VIDEO_RECORDING_STATUS_PAUSED = 2;
std::mutex g_statusMutex;
int32_t g_status = SCE_VIDEO_RECORDING_STATUS_NONE;

unsigned char ClampByte(int value) {
    return static_cast<unsigned char>(value < 0 ? 0 : value > 255 ? 255 : value);
}

int FloorDivide256(int value) {
    return value >= 0 ? value / 256 : -((-value + 255) / 256);
}

unsigned char Luma(int red, int green, int blue) {
    return ClampByte(FloorDivide256(66 * red + 129 * green + 25 * blue + 128) + 16);
}

unsigned char ChromaU(int red, int green, int blue) {
    return ClampByte(FloorDivide256(-38 * red - 74 * green + 112 * blue + 128) + 128);
}

unsigned char ChromaV(int red, int green, int blue) {
    return ClampByte(FloorDivide256(112 * red - 94 * green - 18 * blue + 128) + 128);
}
}

extern "C" {

void APS5_VABI _sceVideoRecordingQueryParam_nid_postfix(VideoRecordingQueryParam* param) {
 (void)param;
 NotImplemented_nid_no_patch("_sceVideoRecordingQueryParam");
}

int APS5_VABI sceVideoRecordingClose(int discard) {
    (void)discard;
    std::lock_guard lock(g_statusMutex);
    if (g_status == SCE_VIDEO_RECORDING_STATUS_NONE) return SCE_VIDEO_RECORDING_ERROR_INVALID_STATE;
    g_status = SCE_VIDEO_RECORDING_STATUS_NONE;
    return 0;
}

int APS5_VABI sceVideoRecordingCloseByHandle(VideoRecordingContext* handle, int discard) {
 (void)handle;
 (void)discard;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingCopyBGRAtoNV12(void* dst, const void* src, uint32_t width, uint32_t height) {
    if (dst == nullptr || src == nullptr || width == 0 || height == 0) APS5_INVALID_ARG_EX;
    const auto maxSize = std::numeric_limits<std::size_t>::max();
    if (static_cast<std::size_t>(width) > maxSize / height) APS5_INVALID_ARG_EX;
    const auto pixelCount = static_cast<std::size_t>(width) * height;
    if (pixelCount > maxSize / 4) APS5_INVALID_ARG_EX;
    const auto ySize = pixelCount;
    const auto chromaWidth = static_cast<std::size_t>(width / 2 + width % 2);
    const auto chromaHeight = static_cast<std::size_t>(height / 2 + height % 2);
    if (chromaWidth > maxSize / chromaHeight || chromaWidth * chromaHeight > maxSize / 2) APS5_INVALID_ARG_EX;
    const auto uvSize = chromaWidth * chromaHeight * 2;
    if (ySize > maxSize - uvSize) APS5_INVALID_ARG_EX;

    const auto* input = static_cast<const unsigned char*>(src);
    auto* output = static_cast<unsigned char*>(dst);
    auto* uv = output + ySize;
    for (std::size_t row = 0; row < height; ++row) {
        for (std::size_t column = 0; column < width; ++column) {
            const auto* pixel = input + (row * width + column) * 4;
            output[row * width + column] = Luma(pixel[2], pixel[1], pixel[0]);
        }
    }
    for (std::size_t row = 0; row < chromaHeight; ++row) {
        for (std::size_t column = 0; column < chromaWidth; ++column) {
            int red = 0;
            int green = 0;
            int blue = 0;
            int samples = 0;
            for (std::size_t dy = 0; dy < 2; ++dy) {
                const auto sourceRow = row * 2 + dy;
                if (sourceRow >= height) continue;
                for (std::size_t dx = 0; dx < 2; ++dx) {
                    const auto sourceColumn = column * 2 + dx;
                    if (sourceColumn >= width) continue;
                    const auto* pixel = input + (sourceRow * width + sourceColumn) * 4;
                    blue += pixel[0];
                    green += pixel[1];
                    red += pixel[2];
                    ++samples;
                }
            }
            red = (red + samples / 2) / samples;
            green = (green + samples / 2) / samples;
            blue = (blue + samples / 2) / samples;
            const auto offset = (row * chromaWidth + column) * 2;
            uv[offset] = ChromaU(red, green, blue);
            uv[offset + 1] = ChromaV(red, green, blue);
        }
    }
    return 0;
}

int APS5_VABI sceVideoRecordingGetInfo(std::int32_t info, void*, std::size_t) {
    switch (info) {
    case 0xa001:
    case 0xa002:
    case 0xa003:
    case 0xa005:
    case 0xa00a:
    case 0xa00b:
    case 0xa00c:
        return static_cast<int>(0x80a80006);
    default:
        return static_cast<int>(0x80a80003);
    }
}

int APS5_VABI sceVideoRecordingGetInfoByHandle(VideoRecordingContext* handle, int32_t info, void* data, size_t size) {
 (void)handle;
 (void)info;
 (void)data;
 (void)size;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingGetStatus(void) {
    std::lock_guard lock(g_statusMutex);
    return g_status;
}

int APS5_VABI sceVideoRecordingGetSupportInfo(VideoRecordingSupportInfo* info) {
 (void)info;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingGetVideoOutMode(std::uint32_t* mode) {
 (void)mode;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingInit(void) {
 return 0;
}

int APS5_VABI sceVideoRecordingInitialize(void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingOpen(const char* path, const VideoRecordingParam* param, void* heap, int heapSize) {
    (void)heap;
    (void)heapSize;
    if (path == nullptr || param == nullptr) APS5_INVALID_ARG_EX;
    return SCE_VIDEO_RECORDING_ERROR_UNSUPPORTED;
}

int APS5_VABI sceVideoRecordingOpen2(const char* path, const VideoRecordingParam2* param, void* heap, int heapSize) {
    (void)heap;
    (void)heapSize;
    if (path == nullptr || param == nullptr) APS5_INVALID_ARG_EX;
    return SCE_VIDEO_RECORDING_ERROR_UNSUPPORTED;
}

std::int64_t APS5_VABI sceVideoRecordingQueryHddSize2(const VideoRecordingParam2* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingQueryMemSize(const VideoRecordingParam* param) {
    if (param == nullptr) APS5_INVALID_ARG_EX;
    return SCE_VIDEO_RECORDING_ERROR_UNSUPPORTED;
}

int APS5_VABI sceVideoRecordingQueryMemSize2(const VideoRecordingParam2* param) {
    if (param == nullptr) APS5_INVALID_ARG_EX;
    return SCE_VIDEO_RECORDING_ERROR_UNSUPPORTED;
}

int APS5_VABI sceVideoRecordingRestModeProcessGetStatus(void* status) {
 (void)status;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingRestModeProcessStart(void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingSendCommand(int cmd, const void* param) {
 (void)cmd;
 (void)param;
 return 0;
}

int APS5_VABI sceVideoRecordingSetInfo(std::int32_t info, const void* data, std::size_t size) {
    switch (info) {
    case 0x8002:
    case 0x8003:
        if (!data || size < sizeof(float)) return static_cast<int>(0x80a80003);
        return static_cast<int>(0x80a80002);
    case 0xa004:
    case 0xa006:
        return static_cast<int>(0x80a80006);
    default:
        return static_cast<int>(0x80a80003);
    }
}

int APS5_VABI sceVideoRecordingStart(void) {
    std::lock_guard lock(g_statusMutex);
    if (g_status == SCE_VIDEO_RECORDING_STATUS_NONE) return SCE_VIDEO_RECORDING_ERROR_INVALID_STATE;
    g_status = SCE_VIDEO_RECORDING_STATUS_RUNNING;
    return 0;
}

int APS5_VABI sceVideoRecordingStop(void) {
    std::lock_guard lock(g_statusMutex);
    if (g_status != SCE_VIDEO_RECORDING_STATUS_RUNNING) return SCE_VIDEO_RECORDING_ERROR_INVALID_STATE;
    g_status = SCE_VIDEO_RECORDING_STATUS_PAUSED;
    return 0;
}

int APS5_VABI sceVideoRecordingTerm(void) {
 return 0;
}

APS5_EXPORT("iQS6DUtLybE", videoRecordingUnknown_iQS6DUtLybE);
int APS5_VABI videoRecordingUnknown_iQS6DUtLybE(void) {
 NotImplemented_nid_no_patch("iQS6DUtLybE");
 return 0;
}

}
