#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <atomic>
#include <cstdio>
#include <iterator>
#include <stdexcept>
#include <string>

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

int APS5_VABI sceAjmDecWVorbisCreateHeaderPacket(void) {
    NotImplemented_nid_no_patch(__func__);
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
    return (instance_id >> 14) & 0x1Fu;
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
