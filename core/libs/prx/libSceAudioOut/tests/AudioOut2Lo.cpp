#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>

extern "C" {
int APS5_VABI sceAudioOut2ContextResetParam(AudioOut2ContextParam*);
int APS5_VABI sceAudioOut2ContextQueryMemory(const AudioOut2ContextParam*, std::size_t*);
int APS5_VABI sceAudioOut2ContextCreate(const AudioOut2ContextParam*, void*, std::size_t, AudioOut2ContextHandle*);
int APS5_VABI sceAudioOut2ContextDestroy(AudioOut2ContextHandle);
int APS5_VABI sceAudioOut2PortCreate(AudioOut2ContextHandle, const AudioOut2PortParam*, AudioOut2PortHandle*);
int APS5_VABI sceAudioOut2PortDestroy(AudioOut2PortHandle);
int APS5_VABI sceAudioOut2LoInit();
int APS5_VABI sceAudioOut2LoTerminate();
int APS5_VABI sceAudioOut2LoContextQueryMemory(const AudioOut2ContextParam*, std::size_t*);
int APS5_VABI sceAudioOut2LoContextCreate(const AudioOut2ContextParam*, void*, std::size_t, AudioOut2ContextHandle*);
int APS5_VABI sceAudioOut2LoContextDestroy(AudioOut2ContextHandle);
int APS5_VABI sceAudioOut2LoContextSetAttributes(AudioOut2ContextHandle, const AudioOut2Attribute*, std::uint32_t);
int APS5_VABI sceAudioOut2LoContextAdvance(AudioOut2ContextHandle);
int APS5_VABI sceAudioOut2LoContextPush(AudioOut2ContextHandle, std::uint32_t);
int APS5_VABI sceAudioOut2LoContextGetQueueLevel(AudioOut2ContextHandle, std::uint32_t*, std::uint32_t*);
int APS5_VABI sceAudioOut2LoPortCreate(AudioOut2ContextHandle, const AudioOut2PortParam*, AudioOut2PortHandle*);
int APS5_VABI sceAudioOut2LoPortDestroy(AudioOut2PortHandle);
int APS5_VABI sceAudioOut2LoPortSetAttributes(AudioOut2PortHandle, const AudioOut2Attribute*, std::uint32_t);
int APS5_VABI sceAudioOut2LoPortGetState(AudioOut2PortHandle, AudioOut2PortState*);
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

constexpr int invalidArgument = static_cast<int>(0x80260502);
constexpr int invalidHandle = static_cast<int>(0x80260503);
constexpr std::uint32_t queueDepth = 4;
constexpr std::uint32_t grain = 256;

AudioOut2ContextParam Params() {
    AudioOut2ContextParam params{};
    Require(sceAudioOut2ContextResetParam(&params) == 0, "the context defaults must be available");
    params.queue_depth = queueDepth;
    params.num_grains = grain;
    return params;
}

AudioOut2PortParam StereoFloat() {
    AudioOut2PortParam params{};
    params.data_format = 2u << 8;
    params.sampling_freq = 48000;
    return params;
}

void TestMemory() {
    const auto params = Params();
    std::size_t lo = 0;
    std::size_t regular = 1;
    Require(sceAudioOut2LoContextQueryMemory(&params, &lo) == 0 && sceAudioOut2ContextQueryMemory(&params, &regular) == 0, "the memory queries must succeed");
    Require(lo == regular && lo != 0, "a low-latency context needs the memory of a context");
    Require(sceAudioOut2LoContextQueryMemory(nullptr, &lo) == invalidArgument, "null parameters must be refused");
    Require(sceAudioOut2LoContextQueryMemory(&params, nullptr) == invalidArgument, "a null size must be refused");
}

void TestContextAndPorts() {
    const auto params = Params();
    AudioOut2ContextHandle context = 0;
    Require(sceAudioOut2LoContextCreate(nullptr, nullptr, 0, &context) == invalidArgument && context == 0, "null parameters must be refused");
    std::vector<std::uint8_t> memory(0x10000);
    Require(sceAudioOut2LoContextCreate(&params, memory.data(), memory.size(), &context) == 0 && context != 0, "a low-latency context must be created");
    AudioOut2ContextHandle regular = 0;
    Require(sceAudioOut2ContextCreate(&params, memory.data(), memory.size(), &regular) == 0 && regular != 0, "a context must be created");

    Require(sceAudioOut2LoContextSetAttributes(context, nullptr, 0) == 0, "no attributes must be accepted");
    Require(sceAudioOut2LoContextSetAttributes(context, nullptr, 1) == invalidArgument, "a null attribute list must be refused");
    Require(sceAudioOut2LoContextSetAttributes(0, nullptr, 0) == invalidHandle, "a null context must be refused");

    const auto portParams = StereoFloat();
    AudioOut2PortHandle port = 0;
    Require(sceAudioOut2LoPortCreate(0, &portParams, &port) == invalidHandle, "a port on a null context must be refused");
    Require(sceAudioOut2LoPortCreate(context, nullptr, &port) == invalidArgument, "null port parameters must be refused");
    Require(sceAudioOut2LoPortCreate(context, &portParams, &port) == 0 && port != 0, "a low-latency port must be created");
    AudioOut2PortHandle second = 0;
    Require(sceAudioOut2LoPortCreate(context, &portParams, &second) == 0 && second != port, "a second low-latency port must be created");
    AudioOut2PortHandle regularPort = 0;
    Require(sceAudioOut2PortCreate(regular, &portParams, &regularPort) == 0, "a port must be created");

    std::vector<float> pcm(grain * 2, 0.25f);
    const void* data = pcm.data();
    const AudioOut2Attribute attribute{0, 0, &data, sizeof(data)};
    Require(sceAudioOut2LoPortSetAttributes(port, &attribute, 1) == 0, "the port must take its PCM buffer");
    Require(sceAudioOut2LoPortSetAttributes(port, nullptr, 1) == invalidArgument, "a null attribute list must be refused");
    AudioOut2PortState state{};
    Require(sceAudioOut2LoPortGetState(port, &state) == 0 && state.num_channels == 2 && state.output == 1, "the port state must report the stereo main output");
    Require(sceAudioOut2LoPortGetState(port, nullptr) == invalidArgument, "a null state must be refused");
    Require(sceAudioOut2LoPortGetState(0x7FFF, &state) == invalidHandle, "an unknown port must be refused");

    std::uint32_t level = 0xFFFFFFFF;
    std::uint32_t available = 0xFFFFFFFF;
    Require(sceAudioOut2LoContextGetQueueLevel(context, &level, &available) == 0 && level + available == queueDepth, "the queue must be reported");
    Require(sceAudioOut2LoContextPush(context, 1) == 0, "a blocking push must succeed");
    Require(sceAudioOut2LoContextAdvance(context) == 0, "an advance must succeed");
    Require(sceAudioOut2LoContextPush(context, 1) == 0, "a push after an advance must succeed");
    Require(sceAudioOut2LoContextGetQueueLevel(context, &level, &available) == 0 && level + available == queueDepth, "the queue must still be reported");
    Require(sceAudioOut2LoContextPush(0, 1) == invalidHandle && sceAudioOut2LoContextAdvance(0) == invalidHandle, "a null context must be refused");
    Require(sceAudioOut2LoContextGetQueueLevel(0, &level, &available) == invalidHandle, "a null context must be refused");

    Require(ThrowsRuntimeError([&] { sceAudioOut2LoContextPush(regular, 1); }), "a regular context must not be pushed as a low-latency one");
    Require(ThrowsRuntimeError([&] { sceAudioOut2LoContextAdvance(regular); }), "a regular context must not be advanced as a low-latency one");
    Require(ThrowsRuntimeError([&] { sceAudioOut2LoContextGetQueueLevel(regular, &level, &available); }), "a regular context must not be polled as a low-latency one");
    Require(ThrowsRuntimeError([&] { sceAudioOut2LoContextSetAttributes(regular, nullptr, 0); }), "a regular context must not take low-latency attributes");
    Require(ThrowsRuntimeError([&] { sceAudioOut2LoPortCreate(regular, &portParams, &port); }), "a low-latency port must not open on a regular context");
    Require(ThrowsRuntimeError([&] { sceAudioOut2LoPortGetState(regularPort, &state); }), "a regular port must not be read as a low-latency one");
    Require(ThrowsRuntimeError([&] { sceAudioOut2LoPortDestroy(regularPort); }), "a regular port must not be destroyed as a low-latency one");
    Require(ThrowsRuntimeError([&] { sceAudioOut2LoContextDestroy(regular); }), "a regular context must not be destroyed as a low-latency one");
    Require(ThrowsRuntimeError([] { sceAudioOut2LoTerminate(); }), "terminating with a live low-latency context is not modelled");

    Require(sceAudioOut2LoPortDestroy(port) == 0, "the low-latency port must be destroyed");
    Require(sceAudioOut2LoPortDestroy(port) == invalidHandle, "a destroyed port must be refused");
    Require(sceAudioOut2LoContextDestroy(context) == 0, "the low-latency context must be destroyed");
    Require(sceAudioOut2LoPortGetState(second, &state) == invalidHandle, "destroying the context must close its ports");
    Require(ThrowsRuntimeError([&] { sceAudioOut2LoContextPush(context, 1); }), "a destroyed low-latency context must not be pushed");
    Require(sceAudioOut2LoContextDestroy(0) == invalidHandle, "a null context must be refused");
    Require(sceAudioOut2PortDestroy(regularPort) == 0 && sceAudioOut2ContextDestroy(regular) == 0, "the regular objects must be destroyed");
}

}

int main() {
    Require(sceAudioOut2LoInit() == 0, "the low-latency service must start");
    TestMemory();
    TestContextAndPorts();
    Require(sceAudioOut2LoTerminate() == 0, "the low-latency service must stop without live contexts");
    return 0;
}
