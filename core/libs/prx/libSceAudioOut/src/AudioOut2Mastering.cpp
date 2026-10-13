#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

static constexpr int SCE_AUDIO_OUT2_ERROR_MASTERING_INVALID_API_PARAM = static_cast<int>(0x80268201);
static constexpr int SCE_AUDIO_OUT2_ERROR_MASTERING_NOT_INITIALIZED = static_cast<int>(0x80268203);
static constexpr std::uint32_t MASTERING_OUTPUT_RECORDING = 2;

static std::mutex g_masteringLock;
static std::uint32_t g_masteringInits = 0;

extern "C" {

int APS5_VABI sceAudioOut2MasteringInit(uint32_t flags) {
    if (flags != 0) return SCE_AUDIO_OUT2_ERROR_MASTERING_INVALID_API_PARAM;
    std::lock_guard lock(g_masteringLock);
    g_masteringInits++;
    return 0;
}

int APS5_VABI sceAudioOut2MasteringTerm(void) {
    std::lock_guard lock(g_masteringLock);
    if (g_masteringInits > 0) g_masteringInits--;
    return 0;
}

int APS5_VABI sceAudioOut2MasteringGetState(AudioOut2MasteringStatesHeader* state, uint32_t output, AudioOut2UserHandle user) {
    (void)state;
    (void)output;
    (void)user;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAudioOut2MasteringSetParam(const AudioOut2MasteringParamsHeader* param, uint32_t output, uint32_t flags) {
    std::lock_guard lock(g_masteringLock);
    if (g_masteringInits == 0) return SCE_AUDIO_OUT2_ERROR_MASTERING_NOT_INITIALIZED;
    if (param == nullptr || output > MASTERING_OUTPUT_RECORDING) return SCE_AUDIO_OUT2_ERROR_MASTERING_INVALID_API_PARAM;
    if (flags != 0) throw std::runtime_error("sceAudioOut2MasteringSetParam: flags " + std::to_string(flags) + " not supported");
    return 0;
}

}
