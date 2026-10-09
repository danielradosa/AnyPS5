#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <atomic>
#include <cstdio>
#include <iterator>
#include <stdexcept>
#include <string>

// Entry points this library does not implement: reported before the unimplemented-call exception so an
// APS5_TRACE_AJM run shows which one a title reached.
static void AjmStub(const char* name) {
    std::fprintf(stderr, "[ajm] unimplemented %s called\n", name);
    NotImplemented_nid_no_patch(name);
}

extern "C" {

int APS5_VABI sceAjmBatchJobEncode(AjmBatchInfo* info, uint32_t instance, const void* pcm_input, size_t pcm_input_size, void* bitstream_output, size_t bitstream_output_size, void* result) {
 (void)info;
 (void)instance;
 (void)pcm_input;
 (void)pcm_input_size;
 (void)bitstream_output;
 (void)bitstream_output_size;
 (void)result;
 AjmStub(__func__);
 return 0;
}

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

}
