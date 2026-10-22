#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

static constexpr int SCE_AUDIO_OUT_ERROR_MASTERING_INVALID_API_PARAM = static_cast<int>(0x80260201);
static constexpr int SCE_AUDIO_OUT_ERROR_MASTERING_NOT_INITIALIZED = static_cast<int>(0x80260203);
static constexpr int SCE_AUDIO_OUT_ERROR_MASTERING_INVALID_STATES_ID = static_cast<int>(0x80260204);
static constexpr std::uint32_t MASTERING_FLAG_SWAP_EQ_COMPRESSOR = 1;
static constexpr std::uint32_t MASTERING_STATES_ID_DEFAULT = 1;
static constexpr std::uint32_t MASTERING_STATES_ID_V2 = 2;
static constexpr std::uint32_t MASTERING_STATES_COMPRESSOR = 0x01010001;
static constexpr std::uint32_t MASTERING_STATES_LIMITER = 0x01010003;

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

int APS5_VABI sceAudioOutMasteringGetState(AudioOut2MasteringStatesHeader* state) {
    std::lock_guard lock(g_masteringLock);
    if (g_masteringInits == 0) return SCE_AUDIO_OUT_ERROR_MASTERING_NOT_INITIALIZED;
    if (state == nullptr) return SCE_AUDIO_OUT_ERROR_MASTERING_INVALID_API_PARAM;
    if (state->states_id == MASTERING_STATES_ID_V2) throw std::runtime_error("sceAudioOutMasteringGetState: states id 2 is not known for this call");
    if (state->states_id != MASTERING_STATES_ID_DEFAULT) return SCE_AUDIO_OUT_ERROR_MASTERING_INVALID_STATES_ID;
    AudioOut2MasteringStates states{};
    states.states_header = *state;
    states.compressor_states.descriptor = {MASTERING_STATES_COMPRESSOR, sizeof(states.compressor_states)};
    states.limiter_states.descriptor = {MASTERING_STATES_LIMITER, sizeof(states.limiter_states)};
    std::memcpy(state, &states, sizeof(states));
    return 0;
}

}
