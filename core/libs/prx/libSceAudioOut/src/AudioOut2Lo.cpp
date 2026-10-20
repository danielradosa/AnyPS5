#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "AudioOut2Internal.hpp"

extern "C" {
int APS5_VABI sceAudioOut2Initialize(void);
int APS5_VABI sceAudioOut2ContextQueryMemory(const AudioOut2ContextParam* params, size_t* memory_size);
int APS5_VABI sceAudioOut2ContextCreate(const AudioOut2ContextParam* params, void* buffer, size_t buffer_size, AudioOut2ContextHandle* ctx);
int APS5_VABI sceAudioOut2ContextDestroy(AudioOut2ContextHandle ctx);
int APS5_VABI sceAudioOut2ContextSetAttributes(AudioOut2ContextHandle ctx, const AudioOut2Attribute* attributes, uint32_t num);
int APS5_VABI sceAudioOut2ContextAdvance(AudioOut2ContextHandle ctx);
int APS5_VABI sceAudioOut2ContextPush(AudioOut2ContextHandle ctx, uint32_t blocking);
int APS5_VABI sceAudioOut2ContextGetQueueLevel(AudioOut2ContextHandle ctx, uint32_t* queue_level, uint32_t* available_queue);
int APS5_VABI sceAudioOut2PortCreate(AudioOut2ContextHandle ctx, const AudioOut2PortParam* params, AudioOut2PortHandle* port);
int APS5_VABI sceAudioOut2PortDestroy(AudioOut2PortHandle port);
int APS5_VABI sceAudioOut2PortSetAttributes(AudioOut2PortHandle port, const AudioOut2Attribute* attributes, uint32_t num);
int APS5_VABI sceAudioOut2PortGetState(AudioOut2PortHandle port, AudioOut2PortState* state);
}

static std::mutex g_loLock;
static std::unordered_set<AudioOut2ContextHandle> g_loContexts;

static void RequireLoContext(AudioOut2ContextHandle ctx, const char* function) {
    if (ctx == 0) return;
    std::lock_guard lock(g_loLock);
    if (!g_loContexts.contains(ctx)) throw std::runtime_error(std::string(function) + ": the context was not created by sceAudioOut2LoContextCreate");
}

static int RequireLoPort(AudioOut2PortHandle port, const char* function) {
    const auto* owner = AudioOut2PortContext(port);
    if (owner == nullptr) return SCE_AUDIO_OUT2_ERROR_INVALID_HANDLE;
    std::lock_guard lock(g_loLock);
    if (!g_loContexts.contains(reinterpret_cast<AudioOut2ContextHandle>(owner))) throw std::runtime_error(std::string(function) + ": the port was not created by sceAudioOut2LoPortCreate");
    return 0;
}

extern "C" {

int APS5_VABI sceAudioOut2LoInit(void) {
    return sceAudioOut2Initialize();
}

int APS5_VABI sceAudioOut2LoTerminate(void) {
    std::lock_guard lock(g_loLock);
    if (!g_loContexts.empty()) throw std::runtime_error("sceAudioOut2LoTerminate: terminating with " + std::to_string(g_loContexts.size()) + " low-latency contexts alive is not implemented");
    return 0;
}

int APS5_VABI sceAudioOut2LoContextQueryMemory(const AudioOut2ContextParam* params, size_t* memory_size) {
    return sceAudioOut2ContextQueryMemory(params, memory_size);
}

int APS5_VABI sceAudioOut2LoContextCreate(const AudioOut2ContextParam* params, void* buffer, size_t buffer_size, AudioOut2ContextHandle* ctx) {
    const int result = sceAudioOut2ContextCreate(params, buffer, buffer_size, ctx);
    if (result != 0) return result;
    std::lock_guard lock(g_loLock);
    g_loContexts.insert(*ctx);
    return 0;
}

int APS5_VABI sceAudioOut2LoContextDestroy(AudioOut2ContextHandle ctx) {
    RequireLoContext(ctx, __func__);
    {
        std::lock_guard lock(g_loLock);
        g_loContexts.erase(ctx);
    }
    return sceAudioOut2ContextDestroy(ctx);
}

int APS5_VABI sceAudioOut2LoContextSetAttributes(AudioOut2ContextHandle ctx, const AudioOut2Attribute* attributes, uint32_t num) {
    RequireLoContext(ctx, __func__);
    return sceAudioOut2ContextSetAttributes(ctx, attributes, num);
}

int APS5_VABI sceAudioOut2LoContextAdvance(AudioOut2ContextHandle ctx) {
    RequireLoContext(ctx, __func__);
    return sceAudioOut2ContextAdvance(ctx);
}

int APS5_VABI sceAudioOut2LoContextPush(AudioOut2ContextHandle ctx, uint32_t blocking) {
    RequireLoContext(ctx, __func__);
    return sceAudioOut2ContextPush(ctx, blocking);
}

int APS5_VABI sceAudioOut2LoContextGetQueueLevel(AudioOut2ContextHandle ctx, uint32_t* queue_level, uint32_t* available_queue) {
    RequireLoContext(ctx, __func__);
    return sceAudioOut2ContextGetQueueLevel(ctx, queue_level, available_queue);
}

int APS5_VABI sceAudioOut2LoPortCreate(AudioOut2ContextHandle ctx, const AudioOut2PortParam* params, AudioOut2PortHandle* port) {
    RequireLoContext(ctx, __func__);
    return sceAudioOut2PortCreate(ctx, params, port);
}

int APS5_VABI sceAudioOut2LoPortDestroy(AudioOut2PortHandle port) {
    if (const int result = RequireLoPort(port, __func__); result != 0) return result;
    return sceAudioOut2PortDestroy(port);
}

int APS5_VABI sceAudioOut2LoPortSetAttributes(AudioOut2PortHandle port, const AudioOut2Attribute* attributes, uint32_t num) {
    if (const int result = RequireLoPort(port, __func__); result != 0) return result;
    return sceAudioOut2PortSetAttributes(port, attributes, num);
}

int APS5_VABI sceAudioOut2LoPortGetState(AudioOut2PortHandle port, AudioOut2PortState* state) {
    if (const int result = RequireLoPort(port, __func__); result != 0) return result;
    return sceAudioOut2PortGetState(port, state);
}

}
