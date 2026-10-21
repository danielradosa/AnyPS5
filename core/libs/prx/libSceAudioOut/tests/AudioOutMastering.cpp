#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceAudioOutMasteringInit(std::uint32_t);
int APS5_VABI sceAudioOutMasteringTerm();
int APS5_VABI sceAudioOutMasteringSetParam(const AudioOut2MasteringParamsHeader*, std::uint32_t);
int APS5_VABI sceAudioOutMasteringGetState(AudioOut2MasteringStatesHeader*);
int APS5_VABI sceAudioOut2MasteringInit(std::uint32_t);
int APS5_VABI sceAudioOut2MasteringTerm();
int APS5_VABI sceAudioOut2MasteringSetParam(const void*, std::uint32_t, std::uint32_t);
}

static void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "%s\n", message);
    std::abort();
}

template<typename TFunction>
static bool ThrowsRuntimeError(TFunction function) {
    try {
        function();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

namespace {

constexpr int invalidApiParam = static_cast<int>(0x80260201);
constexpr int notInitialized = static_cast<int>(0x80260203);
constexpr int invalidStatesId = static_cast<int>(0x80260204);
constexpr int audioOut2NotInitialized = static_cast<int>(0x80268203);
const AudioOut2MasteringParamsHeader params{1};

void TestBeforeInit() {
    Require(sceAudioOutMasteringSetParam(&params, 0) == notInitialized, "parameters before initialization must be refused");
    Require(sceAudioOutMasteringTerm() == 0, "termination without an initialization must be accepted");
    Require(sceAudioOutMasteringSetParam(&params, 0) == notInitialized, "an unmatched termination must not initialize");
    AudioOut2MasteringStatesHeader header{1};
    Require(sceAudioOutMasteringGetState(&header) == notInitialized, "states before initialization must be refused");
}

struct StatesBuffer {
    AudioOut2MasteringStates states;
    unsigned char after[16];
};

void TestGetState() {
    Require(sceAudioOutMasteringGetState(nullptr) == invalidApiParam, "a null states block must be refused");
    StatesBuffer buffer;
    std::memset(&buffer, 0xA5, sizeof(buffer));
    buffer.states.states_header.states_id = 1;
    Require(sceAudioOutMasteringGetState(&buffer.states.states_header) == 0, "the default states must be returned");
    Require(buffer.states.states_header.states_id == 1, "the states id must be kept");
    Require(buffer.states.reserved[0] == 0 && buffer.states.reserved[1] == 0 && buffer.states.reserved[2] == 0, "the reserved words must be cleared");
    Require(buffer.states.compressor_states.descriptor.id == 0x01010001 && buffer.states.compressor_states.descriptor.size == 0xD0, "the compressor descriptor must be written");
    Require(buffer.states.limiter_states.descriptor.id == 0x01010003 && buffer.states.limiter_states.descriptor.size == 0x70, "the limiter descriptor must be written");
    Require(buffer.states.compressor_states.input_rms[0][0] == 0.0f && buffer.states.compressor_states.compression_coeff[2][7] == 0.0f, "the compressor meters must read 0");
    Require(buffer.states.limiter_states.input_peak[0] == 0.0f && buffer.states.limiter_states.gain_peak[7] == 0.0f, "the limiter meters must read 0");
    for (unsigned char byte : buffer.after) Require(byte == 0xA5, "nothing past the default states may be written");

    StatesBuffer untouched;
    std::memset(&untouched, 0xA5, sizeof(untouched));
    untouched.states.states_header.states_id = 3;
    StatesBuffer expected = untouched;
    Require(sceAudioOutMasteringGetState(&untouched.states.states_header) == invalidStatesId, "an unknown states id must be refused");
    Require(std::memcmp(&untouched, &expected, sizeof(untouched)) == 0, "a refused states id must leave the block alone");
    untouched.states.states_header.states_id = 2;
    Require(ThrowsRuntimeError([&] { sceAudioOutMasteringGetState(&untouched.states.states_header); }), "states id 2 is not modelled for this call");
}

void TestInit() {
    Require(sceAudioOutMasteringInit(2) == invalidApiParam, "flag 2 must be refused");
    Require(sceAudioOutMasteringInit(0x80000000u) == invalidApiParam, "a high flag must be refused");
    Require(sceAudioOutMasteringInit(3) == invalidApiParam, "unknown flags must be refused with the known one");
    Require(ThrowsRuntimeError([] { sceAudioOutMasteringInit(1); }), "the compressor-first flag is not modelled");
    Require(sceAudioOutMasteringSetParam(&params, 0) == notInitialized, "a refused initialization must not initialize");
    Require(sceAudioOutMasteringInit(0) == 0, "flags 0 must be accepted");
}

void TestSetParam() {
    Require(sceAudioOutMasteringSetParam(&params, 0) == 0, "mastering parameters must be accepted");
    Require(sceAudioOutMasteringSetParam(nullptr, 0) == invalidApiParam, "null parameters must be refused");
    Require(ThrowsRuntimeError([] { sceAudioOutMasteringSetParam(&params, 1); }), "flags must throw");
    Require(sceAudioOut2MasteringSetParam(&params, 0, 0) == audioOut2NotInitialized, "the AudioOut2 mastering stays separate");
    Require(sceAudioOut2MasteringInit(0) == 0 && sceAudioOut2MasteringTerm() == 0, "the AudioOut2 mastering must start and stop on its own");
    Require(sceAudioOutMasteringSetParam(&params, 0) == 0, "stopping the AudioOut2 mastering must not stop this one");
}

void TestTerm() {
    Require(sceAudioOutMasteringInit(0) == 0, "a second initialization must be accepted");
    Require(sceAudioOutMasteringTerm() == 0, "the first termination must be accepted");
    Require(sceAudioOutMasteringSetParam(&params, 0) == 0, "one initialization must remain");
    Require(sceAudioOutMasteringTerm() == 0, "the second termination must be accepted");
    Require(sceAudioOutMasteringSetParam(&params, 0) == notInitialized, "parameters after the last termination must be refused");
}

}

int main() {
    TestBeforeInit();
    TestInit();
    TestSetParam();
    TestGetState();
    TestTerm();
    return 0;
}
