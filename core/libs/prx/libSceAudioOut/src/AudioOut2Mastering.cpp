#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

static constexpr int SCE_AUDIO_OUT2_ERROR_MASTERING_INVALID_API_PARAM = static_cast<int>(0x80268201);
static constexpr int SCE_AUDIO_OUT2_ERROR_MASTERING_NOT_INITIALIZED = static_cast<int>(0x80268203);
static constexpr int SCE_AUDIO_OUT2_ERROR_MASTERING_INVALID_STATES_ID = static_cast<int>(0x80268204);
static constexpr std::uint32_t MASTERING_OUTPUT_RECORDING = 2;
static constexpr std::uint32_t MASTERING_STATES_ID_DEFAULT = 1;
static constexpr std::uint32_t MASTERING_STATES_ID_V2 = 2;
static constexpr std::uint32_t MASTERING_STATES_COMPRESSOR = 0x01010001;
static constexpr std::uint32_t MASTERING_STATES_COMPRESSOR_V2 = 0x01020001;
static constexpr std::uint32_t MASTERING_STATES_LIMITER = 0x01010003;

static_assert(sizeof(AudioOut2MasteringStates) == 0x150);
static_assert(offsetof(AudioOut2MasteringStates, compressor_states) == 0x10);
static_assert(offsetof(AudioOut2MasteringStates, limiter_states) == 0xE0);
static_assert(sizeof(AudioOut2MasteringStatesV2) == 0x190);
static_assert(offsetof(AudioOut2MasteringStatesV2, limiter_states) == 0x120);

static std::mutex g_masteringLock;
static std::uint32_t g_masteringInits = 0;

template<typename TStates>
static void writeStates(AudioOut2MasteringStatesHeader* state, std::uint32_t compressorId) {
    TStates states{};
    states.states_header = *state;
    states.compressor_states.descriptor = {compressorId, sizeof(states.compressor_states)};
    states.limiter_states.descriptor = {MASTERING_STATES_LIMITER, sizeof(states.limiter_states)};
    std::memcpy(state, &states, sizeof(states));
}

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
    (void)user;
    std::lock_guard lock(g_masteringLock);
    if (g_masteringInits == 0) return SCE_AUDIO_OUT2_ERROR_MASTERING_NOT_INITIALIZED;
    if (state == nullptr || output > MASTERING_OUTPUT_RECORDING) return SCE_AUDIO_OUT2_ERROR_MASTERING_INVALID_API_PARAM;
    switch (state->states_id) {
        case MASTERING_STATES_ID_DEFAULT:
            writeStates<AudioOut2MasteringStates>(state, MASTERING_STATES_COMPRESSOR);
            return 0;
        case MASTERING_STATES_ID_V2:
            writeStates<AudioOut2MasteringStatesV2>(state, MASTERING_STATES_COMPRESSOR_V2);
            return 0;
        default:
            return SCE_AUDIO_OUT2_ERROR_MASTERING_INVALID_STATES_ID;
    }
}

int APS5_VABI sceAudioOut2MasteringSetParam(const AudioOut2MasteringParamsHeader* param, uint32_t output, uint32_t flags) {
    std::lock_guard lock(g_masteringLock);
    if (g_masteringInits == 0) return SCE_AUDIO_OUT2_ERROR_MASTERING_NOT_INITIALIZED;
    if (param == nullptr || output > MASTERING_OUTPUT_RECORDING) return SCE_AUDIO_OUT2_ERROR_MASTERING_INVALID_API_PARAM;
    if (flags != 0) throw std::runtime_error("sceAudioOut2MasteringSetParam: flags " + std::to_string(flags) + " not supported");
    return 0;
}

}
