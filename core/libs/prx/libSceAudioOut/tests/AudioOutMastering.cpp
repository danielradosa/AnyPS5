#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceAudioOutMasteringInit(std::uint32_t);
int APS5_VABI sceAudioOutMasteringTerm();
int APS5_VABI sceAudioOutMasteringSetParam(const AudioOut2MasteringParamsHeader*, std::uint32_t);
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
constexpr int audioOut2NotInitialized = static_cast<int>(0x80268203);
const AudioOut2MasteringParamsHeader params{1};

void TestBeforeInit() {
    Require(sceAudioOutMasteringSetParam(&params, 0) == notInitialized, "parameters before initialization must be refused");
    Require(sceAudioOutMasteringTerm() == 0, "termination without an initialization must be accepted");
    Require(sceAudioOutMasteringSetParam(&params, 0) == notInitialized, "an unmatched termination must not initialize");
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
    TestTerm();
    return 0;
}
