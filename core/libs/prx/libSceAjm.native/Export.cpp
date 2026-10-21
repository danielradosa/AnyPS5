#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>

struct AjmDecWVorbisHeaderPacketParam {
    std::uint64_t size;
    std::uint32_t channels;
    std::uint32_t sampleRate;
    std::int32_t bytesPerSecond;
    std::int32_t blockSize0Log2;
    std::int32_t blockSize1Log2;
    std::uint32_t reserved;
    const std::uint8_t* setupPacket;
    std::uint64_t setupPacketSize;
    std::uint8_t* output;
    std::uint64_t outputSize;
    std::uint64_t writtenSize;
};
static_assert(sizeof(AjmDecWVorbisHeaderPacketParam) == 0x48);

namespace {

constexpr int AJM_WVORBIS_ERROR_INVALID_PARAMETER = static_cast<int>(0x8A6E0001);
constexpr int AJM_WVORBIS_ERROR_INVALID_SIZE = static_cast<int>(0x8A6E0002);
constexpr int AJM_WVORBIS_ERROR_OUTPUT_TOO_SMALL = static_cast<int>(0x8A6E0003);
constexpr int AJM_WVORBIS_ERROR_INVALID_SETUP = static_cast<int>(0x8A6E0004);
constexpr std::size_t WVORBIS_LENGTH_TABLE_SIZE = 10;
constexpr std::size_t VORBIS_IDENTIFICATION_SIZE = 30;
constexpr std::size_t VORBIS_COMMENT_SIZE = 16;
constexpr std::size_t WVORBIS_SETUP_MIN_SIZE = 9;
constexpr std::size_t WVORBIS_SETUP_PART1_LENGTH_OFFSET = 7;
constexpr std::int32_t VORBIS_BLOCK_SIZE_MIN = 64;
constexpr std::int32_t VORBIS_BLOCK_SIZE_MAX = 8192;
constexpr std::uint8_t VORBIS_MAGIC[] {'v', 'o', 'r', 'b', 'i', 's'};

std::int32_t WVorbisBlockSize(std::int32_t log2) {
    return static_cast<std::int32_t>(1u << (static_cast<std::uint32_t>(log2) & 31u));
}

std::uint8_t* PutLittle(std::uint8_t* out, std::uint32_t value, std::size_t bytes) {
    for (std::size_t i = 0; i < bytes; ++i) out[i] = static_cast<std::uint8_t>(value >> (8 * i));
    return out + bytes;
}

std::uint8_t* PutVorbisHeaderStart(std::uint8_t* out, std::uint8_t type) {
    *out++ = type;
    std::memcpy(out, VORBIS_MAGIC, sizeof(VORBIS_MAGIC));
    return out + sizeof(VORBIS_MAGIC);
}

}

extern "C" {

const char* APS5_VABI sceAjmStrError(int error) {
 static constexpr const char* ErrorNames[] = {
  "SCE_AJM_ERROR_UNKNOWN",
  "SCE_AJM_ERROR_INVALID_CONTEXT",
  "SCE_AJM_ERROR_INVALID_INSTANCE",
  "SCE_AJM_ERROR_INVALID_BATCH",
  "SCE_AJM_ERROR_INVALID_PARAMETER",
  "SCE_AJM_ERROR_OUT_OF_MEMORY",
  "SCE_AJM_ERROR_OUT_OF_RESOURCES",
  "SCE_AJM_ERROR_CODEC_NOT_SUPPORTED",
  "SCE_AJM_ERROR_CODEC_ALREADY_REGISTERED",
  "SCE_AJM_ERROR_CODEC_NOT_REGISTERED",
  "SCE_AJM_ERROR_WRONG_REVISION_FLAG",
  "SCE_AJM_ERROR_FLAG_NOT_SUPPORTED",
  "SCE_AJM_ERROR_BUSY",
  "SCE_AJM_ERROR_BAD_PRIORITY",
  "SCE_AJM_ERROR_IN_PROGRESS",
  "SCE_AJM_ERROR_RETRY",
  "SCE_AJM_ERROR_MALFORMED_BATCH",
  "SCE_AJM_ERROR_JOB_CREATION",
  "SCE_AJM_ERROR_INVALID_OPCODE",
  "SCE_AJM_ERROR_PRIORITY_VIOLATION",
  "SCE_AJM_ERROR_BUFFER_TOO_BIG",
  "SCE_AJM_ERROR_INVALID_ADDRESS",
  "SCE_AJM_ERROR_CANCELLED",
 };
 constexpr std::uint32_t FirstError = 0x80930001u;
 const std::uint32_t code = static_cast<std::uint32_t>(error);
 if (code < FirstError || code - FirstError >= std::size(ErrorNames)) {
  char hex[11];
  std::snprintf(hex, sizeof(hex), "0x%08X", static_cast<unsigned>(code));
  throw std::runtime_error(std::string("sceAjmStrError: ") + hex + " is not an AJM error code");
 }
 return ErrorNames[code - FirstError];
}

int APS5_VABI sceAjmDecWVorbisCreateHeaderPacket(AjmDecWVorbisHeaderPacketParam* param) {
    if (param == nullptr) return AJM_WVORBIS_ERROR_INVALID_PARAMETER;
    if (param->size != sizeof(AjmDecWVorbisHeaderPacketParam)) return AJM_WVORBIS_ERROR_INVALID_SIZE;
    if (param->bytesPerSecond < 0) return AJM_WVORBIS_ERROR_INVALID_PARAMETER;
    if (WVorbisBlockSize(param->blockSize0Log2) < VORBIS_BLOCK_SIZE_MIN) return AJM_WVORBIS_ERROR_INVALID_PARAMETER;
    if (param->blockSize1Log2 < param->blockSize0Log2 || WVorbisBlockSize(param->blockSize1Log2) > VORBIS_BLOCK_SIZE_MAX)
        return AJM_WVORBIS_ERROR_INVALID_PARAMETER;
    if (param->setupPacket == nullptr) return AJM_WVORBIS_ERROR_INVALID_PARAMETER;
    if (param->setupPacketSize < WVORBIS_SETUP_MIN_SIZE) return AJM_WVORBIS_ERROR_INVALID_SETUP;
    if (param->output == nullptr) return AJM_WVORBIS_ERROR_INVALID_PARAMETER;
    constexpr std::size_t headers = WVORBIS_LENGTH_TABLE_SIZE + VORBIS_IDENTIFICATION_SIZE + VORBIS_COMMENT_SIZE;
    if (param->outputSize < headers || param->setupPacketSize > param->outputSize - headers) return AJM_WVORBIS_ERROR_OUTPUT_TOO_SMALL;

    const std::uint8_t* setup = param->setupPacket;
    const auto part1 = static_cast<std::uint16_t>(setup[WVORBIS_SETUP_PART1_LENGTH_OFFSET] | setup[WVORBIS_SETUP_PART1_LENGTH_OFFSET + 1] << 8);
    std::uint8_t* out = param->output;
    out = PutLittle(out, 0, 2);
    out = PutLittle(out, VORBIS_IDENTIFICATION_SIZE, 2);
    out = PutLittle(out, VORBIS_COMMENT_SIZE, 2);
    out = PutLittle(out, part1, 2);
    out = PutLittle(out, static_cast<std::uint16_t>(param->setupPacketSize) - part1, 2);

    out = PutVorbisHeaderStart(out, 1);
    out = PutLittle(out, 0, 4);
    out = PutLittle(out, param->channels, 1);
    out = PutLittle(out, param->sampleRate, 4);
    out = PutLittle(out, 0, 4);
    out = PutLittle(out, static_cast<std::uint32_t>(param->bytesPerSecond) * 8u, 4);
    out = PutLittle(out, 0, 4);
    *out++ = static_cast<std::uint8_t>((param->blockSize0Log2 & 0xF) | (param->blockSize1Log2 & 0xF) << 4);
    *out++ = 1;

    out = PutVorbisHeaderStart(out, 3);
    out = PutLittle(out, 0, 4);
    out = PutLittle(out, 0, 4);
    *out++ = 1;

    std::memcpy(out, setup, param->setupPacketSize);
    param->writtenSize = headers + param->setupPacketSize;
    return 0;
}

int APS5_VABI sceAjmDecWVorbisCreateSetupPacket(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAjmBatchGetLog() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAjmInstanceAvailable() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

uint32_t APS5_VABI sceAjmInstanceCodecType(uint32_t instance_id) {
    return instance_id >> 14;
}

int APS5_VABI sceAjmInstanceExtend(uint32_t context, uint32_t codec, uint64_t flags, uint32_t instance) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAjmInstanceSwitch(uint32_t context, uint32_t codec, uint64_t flags, uint32_t instance) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
