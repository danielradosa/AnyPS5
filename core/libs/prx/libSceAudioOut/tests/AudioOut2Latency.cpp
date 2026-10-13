#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>

extern "C" {
int APS5_VABI sceAudioOut2Initialize();
int APS5_VABI sceAudioOut2Set3DLatency(int, std::uint32_t);
int APS5_VABI sceAudioOut2MasteringInit(std::uint32_t);
int APS5_VABI sceAudioOut2MasteringTerm();
int APS5_VABI sceAudioOut2MasteringSetParam(const void*, std::uint32_t, std::uint32_t);
int APS5_VABI sceAudioOut2EnableChat();
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

constexpr int systemUser = 0xFF;
constexpr int user = 0x10000000;

void TestSet3DLatency() {
    Require(sceAudioOut2Set3DLatency(systemUser, 2) == 0, "latency 2 for the system user must be accepted");
    Require(sceAudioOut2Set3DLatency(systemUser, 2) == 0, "latency 2 must be accepted again");
    Require(sceAudioOut2Set3DLatency(systemUser, 1) == 0, "latency 1 for the system user must be accepted");
    Require(ThrowsRuntimeError([] { sceAudioOut2Set3DLatency(systemUser, 0); }), "latency 0 must throw");
    Require(ThrowsRuntimeError([] { sceAudioOut2Set3DLatency(systemUser, 3); }), "latency 3 must throw");
    Require(ThrowsRuntimeError([] { sceAudioOut2Set3DLatency(user, 2); }), "a user other than the system user must throw");
}

constexpr int masteringInvalidApiParam = static_cast<int>(0x80268201);
constexpr int masteringNotInitialized = static_cast<int>(0x80268203);
constexpr std::uint32_t outputMain = 0;
constexpr std::uint32_t outputHeadphone = 1;
constexpr std::uint32_t outputRecording = 2;
const std::uint32_t masteringParams[4] = {1u, 0u, 0u, 0u};

void TestMasteringBeforeInit() {
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

void TestMasteringTerm() {
    Require(sceAudioOut2MasteringInit(0) == 0, "a second initialization must be accepted");
    Require(sceAudioOut2MasteringTerm() == 0, "termination must be accepted");
    Require(sceAudioOut2MasteringSetParam(masteringParams, outputMain, 0) == 0, "one initialization is still outstanding");
    Require(sceAudioOut2MasteringTerm() == 0, "termination must be accepted for each initialization");
    Require(sceAudioOut2MasteringSetParam(masteringParams, outputMain, 0) == masteringNotInitialized, "the last termination must end the mastering");
    Require(sceAudioOut2MasteringTerm() == 0, "an extra termination must be accepted");
}

void TestEnableChat() {
    Require(sceAudioOut2EnableChat() == 0, "enabling chat must be accepted");
    Require(sceAudioOut2EnableChat() == 0, "enabling chat again must be accepted");
}

}

int main() {
    Require(sceAudioOut2Initialize() == 0, "initialization must succeed");
    TestSet3DLatency();
    TestMasteringBeforeInit();
    TestMasteringInit();
    TestMasteringSetParam();
    TestMasteringTerm();
    TestEnableChat();
    return 0;
}
