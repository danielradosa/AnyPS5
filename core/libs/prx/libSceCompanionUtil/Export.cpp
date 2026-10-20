#include <cstdint>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {
constexpr int COMPANION_UTIL_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80AD0004u);
constexpr int COMPANION_UTIL_ERROR_INVALID_POINTER = static_cast<int>(0x80AD0006u);
constexpr int COMPANION_UTIL_ERROR_NO_EVENT = static_cast<int>(0x80AD0008u);

std::mutex g_mutex;
bool g_initialized = false;
}

extern "C" {

int APS5_VABI sceCompanionUtilGetEvent(void* pEvent) {
    if (!pEvent) return COMPANION_UTIL_ERROR_INVALID_ARGUMENT;
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return COMPANION_UTIL_ERROR_INVALID_POINTER;
    return COMPANION_UTIL_ERROR_NO_EVENT;
}

int APS5_VABI sceCompanionUtilGetRemoteOskEvent() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCompanionUtilInitialize(const void* option) {
    (void)option;
    std::lock_guard lock(g_mutex);
    if (g_initialized) throw std::logic_error("sceCompanionUtilInitialize: the library is already initialized");
    g_initialized = true;
    return 0;
}

int APS5_VABI sceCompanionUtilOptParamInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCompanionUtilTerminate() {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) throw std::logic_error("sceCompanionUtilTerminate: the library is not initialized");
    g_initialized = false;
    return 0;
}

}
