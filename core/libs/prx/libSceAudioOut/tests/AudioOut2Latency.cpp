#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

extern "C" {
int APS5_VABI sceAudioOut2Initialize();
int APS5_VABI sceAudioOut2LoInit();
int APS5_VABI sceAudioOut2LoTerminate();
int APS5_VABI sceAudioOut2Set3DLatency(int, std::uint32_t);
int APS5_VABI sceAudioOut2MasteringInit(std::uint32_t);
int APS5_VABI sceAudioOut2MasteringTerm();
int APS5_VABI sceAudioOut2MasteringSetParam(const void*, std::uint32_t, std::uint32_t);
int APS5_VABI sceAudioOut2MasteringGetState(void*, std::uint32_t, std::uintptr_t);
int APS5_VABI sceAudioOut2SetSystemDebugState(const void*);
int APS5_VABI sceAudioOut2EnableChat();
int APS5_VABI sceAudioOut2DisableChat();
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

static_assert(sizeof(AudioOut2MasteringCompressorStates) == 208);
static_assert(sizeof(AudioOut2MasteringCompressorStatesV2) == 272);
static_assert(sizeof(AudioOut2MasteringLimiterStates) == 112);
static_assert(sizeof(AudioOut2MasteringStates) == 336);
static_assert(sizeof(AudioOut2MasteringStatesV2) == 400);

constexpr int systemUser = 0xFF;
constexpr int user = 0x10000000;
constexpr int errorInvalidParam = static_cast<int>(0x80268001);
constexpr int errorInvalidArgument = static_cast<int>(0x80260502);

void TestSet3DLatency() {
    Require(sceAudioOut2Set3DLatency(systemUser, 0) == 0, "output 0 for the system user must be accepted");
    Require(sceAudioOut2Set3DLatency(systemUser, 1) == 0, "output 1 for the system user must be accepted");
    Require(sceAudioOut2Set3DLatency(systemUser, 2) == 0, "output 2 for the system user must be accepted");
    Require(sceAudioOut2Set3DLatency(systemUser, 2) == 0, "output 2 must be accepted again");
    Require(sceAudioOut2Set3DLatency(user, 1) == 0, "output 1 for another user must be accepted");
    Require(sceAudioOut2Set3DLatency(systemUser, 3) == errorInvalidParam, "output 3 must be refused");
    Require(sceAudioOut2Set3DLatency(systemUser, 0xFFFFFFFFu) == errorInvalidParam, "a large output must be refused");
}

constexpr int masteringInvalidApiParam = static_cast<int>(0x80268201);
constexpr int masteringNotInitialized = static_cast<int>(0x80268203);
constexpr std::uint32_t outputMain = 0;
constexpr std::uint32_t outputHeadphone = 1;
constexpr std::uint32_t outputRecording = 2;
const std::uint32_t masteringParams[4] = {1u, 0u, 0u, 0u};
constexpr int masteringInvalidStatesId = static_cast<int>(0x80268204);

using StatesBuffer = std::array<std::uint8_t, 0x200>;

StatesBuffer StatesRequest(std::uint32_t statesId) {
    StatesBuffer buffer;
    buffer.fill(0xA5);
    std::memcpy(buffer.data(), &statesId, sizeof(statesId));
    return buffer;
}

std::uint32_t Word(const StatesBuffer& buffer, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, buffer.data() + offset, sizeof(value));
    return value;
}

void RequireStates(const StatesBuffer& buffer, std::uint32_t statesId, std::size_t size, std::uint32_t compressorId, std::size_t compressorSize, std::size_t limiter) {
    Require(Word(buffer, 0) == statesId, "the states id must be kept");
    Require(Word(buffer, 0x10) == compressorId && Word(buffer, 0x14) == compressorSize, "the compressor descriptor must follow the header");
    Require(Word(buffer, limiter) == 0x01010003 && Word(buffer, limiter + 4) == 0x70, "the limiter descriptor must follow the compressor");
    for (std::size_t offset = 4; offset < size; offset += 4) {
        if (offset == 0x10 || offset == 0x14 || offset == limiter || offset == limiter + 4) continue;
        Require(Word(buffer, offset) == 0, "reserved fields and meters must read 0");
    }
    for (std::size_t offset = size; offset < buffer.size(); ++offset) Require(buffer[offset] == 0xA5, "nothing past the states block may be written");
}

void TestMasteringBeforeInit() {
    StatesBuffer states = StatesRequest(1);
    Require(sceAudioOut2MasteringGetState(states.data(), outputMain, 0) == masteringNotInitialized, "states before initialization must be refused");
    Require(states == StatesRequest(1), "refused states must not be written");
    Require(sceAudioOut2MasteringSetParam(masteringParams, outputMain, 0) == masteringNotInitialized, "parameters before initialization must be refused");
    Require(sceAudioOut2MasteringTerm() == 0, "termination without an initialization must be accepted");
    Require(sceAudioOut2MasteringSetParam(masteringParams, outputMain, 0) == masteringNotInitialized, "an unmatched termination must not initialize");
}

void TestMasteringInit() {
    Require(sceAudioOut2MasteringInit(1) == masteringInvalidApiParam, "flags 1 must be refused");
    Require(sceAudioOut2MasteringInit(0x80000000u) == masteringInvalidApiParam, "a high flag must be refused");
    Require(sceAudioOut2MasteringSetParam(masteringParams, outputMain, 0) == masteringNotInitialized, "a refused initialization must not initialize");
    Require(sceAudioOut2MasteringInit(0) == 0, "flags 0 must be accepted");
}

void TestMasteringSetParam() {
    for (const std::uint32_t output : {outputMain, outputHeadphone, outputRecording}) {
        Require(sceAudioOut2MasteringSetParam(masteringParams, output, 0) == 0, "mastering parameters must be accepted for each output");
    }
    Require(sceAudioOut2MasteringSetParam(masteringParams, outputRecording + 1, 0) == masteringInvalidApiParam, "an unknown output must be refused");
    Require(sceAudioOut2MasteringSetParam(nullptr, outputMain, 0) == masteringInvalidApiParam, "null mastering parameters must be refused");
    Require(ThrowsRuntimeError([] { sceAudioOut2MasteringSetParam(masteringParams, outputMain, 1); }), "unknown flags must throw");
}

void TestMasteringGetState() {
    for (const std::uint32_t output : {outputMain, outputHeadphone, outputRecording}) {
        StatesBuffer states = StatesRequest(1);
        Require(sceAudioOut2MasteringGetState(states.data(), output, 0) == 0, "default states must be reported for each output");
        RequireStates(states, 1, 0x150, 0x01010001, 0xD0, 0xE0);
    }
    StatesBuffer v2 = StatesRequest(2);
    Require(sceAudioOut2MasteringGetState(v2.data(), outputMain, 0) == 0, "version 2 states must be reported");
    RequireStates(v2, 2, 0x190, 0x01020001, 0x110, 0x120);
    for (const std::uint32_t statesId : {0u, 3u}) {
        StatesBuffer states = StatesRequest(statesId);
        Require(sceAudioOut2MasteringGetState(states.data(), outputMain, 0) == masteringInvalidStatesId, "an unknown states id must be refused");
        Require(states == StatesRequest(statesId), "refused states must not be written");
    }
    StatesBuffer states = StatesRequest(1);
    Require(sceAudioOut2MasteringGetState(states.data(), outputRecording + 1, 0) == masteringInvalidApiParam, "an unknown output must be refused");
    Require(states == StatesRequest(1), "refused states must not be written");
    Require(sceAudioOut2MasteringGetState(nullptr, outputMain, 0) == masteringInvalidApiParam, "null states must be refused");
}

void TestMasteringTerm() {
    Require(sceAudioOut2MasteringInit(0) == 0, "a second initialization must be accepted");
    Require(sceAudioOut2MasteringTerm() == 0, "termination must be accepted");
    Require(sceAudioOut2MasteringSetParam(masteringParams, outputMain, 0) == 0, "one initialization is still outstanding");
    Require(sceAudioOut2MasteringTerm() == 0, "termination must be accepted for each initialization");
    Require(sceAudioOut2MasteringSetParam(masteringParams, outputMain, 0) == masteringNotInitialized, "the last termination must end the mastering");
    Require(sceAudioOut2MasteringTerm() == 0, "an extra termination must be accepted");
}

void TestSetSystemDebugState() {
    const std::uint64_t param[4] = {1, 0, 0, 0};
    Require(sceAudioOut2SetSystemDebugState(reinterpret_cast<const void*>(param)) == 0, "a debug state must be accepted");
    Require(sceAudioOut2SetSystemDebugState(reinterpret_cast<const void*>(param)) == 0, "a debug state must be accepted again");
    Require(sceAudioOut2SetSystemDebugState(nullptr) == errorInvalidArgument, "a null debug state is refused");
}

void TestEnableChat() {
    Require(sceAudioOut2EnableChat() == 0, "enabling chat must be accepted");
    Require(sceAudioOut2EnableChat() == 0, "enabling chat again must be accepted");
    Require(sceAudioOut2DisableChat() == 0, "disabling chat must be accepted");
    Require(sceAudioOut2DisableChat() == 0, "disabling chat again must be accepted");
    Require(sceAudioOut2EnableChat() == 0, "enabling chat after disabling it must be accepted");
}

void TestLowLatencyService() {
    Require(sceAudioOut2LoInit() == 0, "starting the low-latency service must be accepted");
    Require(sceAudioOut2LoInit() == 0, "starting the low-latency service again must be accepted");
    Require(sceAudioOut2LoTerminate() == 0, "stopping the low-latency service without low-latency contexts must be accepted");
}

}

int main() {
    Require(sceAudioOut2Initialize() == 0, "initialization must succeed");
    TestSet3DLatency();
    TestMasteringBeforeInit();
    TestMasteringInit();
    TestMasteringSetParam();
    TestMasteringGetState();
    TestMasteringTerm();
    TestSetSystemDebugState();
    TestEnableChat();
    TestLowLatencyService();
    return 0;
}
