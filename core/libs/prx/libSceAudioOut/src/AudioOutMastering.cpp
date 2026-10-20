#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

static constexpr int SCE_AUDIO_OUT_ERROR_MASTERING_INVALID_API_PARAM = static_cast<int>(0x80260201);
static constexpr int SCE_AUDIO_OUT_ERROR_MASTERING_NOT_INITIALIZED = static_cast<int>(0x80260203);
static constexpr std::uint32_t MASTERING_FLAG_SWAP_EQ_COMPRESSOR = 1;

static std::mutex g_masteringLock;
static std::uint32_t g_masteringInits = 0;

extern "C" {

int APS5_VABI sceAudioOutMasteringInit(uint32_t flags) {
    if ((flags & ~MASTERING_FLAG_SWAP_EQ_COMPRESSOR) != 0) return SCE_AUDIO_OUT_ERROR_MASTERING_INVALID_API_PARAM;
    if (flags != 0) throw std::runtime_error("sceAudioOutMasteringInit: flag 1 (compressor before the equaliser) is not supported");
    std::lock_guard lock(g_masteringLock);
    g_masteringInits++;
    return 0;
}

int APS5_VABI sceAudioOutMasteringTerm(void) {
    std::lock_guard lock(g_masteringLock);
    if (g_masteringInits > 0) g_masteringInits--;
    return 0;
}

int APS5_VABI sceAudioOutMasteringSetParam(const AudioOut2MasteringParamsHeader* param, uint32_t flags) {
    std::lock_guard lock(g_masteringLock);
    if (g_masteringInits == 0) return SCE_AUDIO_OUT_ERROR_MASTERING_NOT_INITIALIZED;
    if (param == nullptr) return SCE_AUDIO_OUT_ERROR_MASTERING_INVALID_API_PARAM;
    if (flags != 0) throw std::runtime_error("sceAudioOutMasteringSetParam: flags " + std::to_string(flags) + " not supported");
    return 0;
}

}
