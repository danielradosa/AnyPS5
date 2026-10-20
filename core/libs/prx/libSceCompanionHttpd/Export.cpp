#include <cstdint>
#include <cstddef>
#include <cstring>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {
constexpr int COMPANION_HTTPD_ERROR_INVALID_PARAM = static_cast<int>(0x80E40004u);
constexpr int COMPANION_HTTPD_ERROR_NOT_INITIALIZED = static_cast<int>(0x80E40006u);
constexpr int COMPANION_HTTPD_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80E40007u);
constexpr int COMPANION_HTTPD_ERROR_NO_EVENT = static_cast<int>(0x80E40008u);
constexpr int COMPANION_HTTPD_ERROR_ALREADY_STARTED = static_cast<int>(0x80E4000Au);
constexpr int COMPANION_HTTPD_ERROR_NOT_STARTED = static_cast<int>(0x80E4000Bu);
constexpr int COMPANION_HTTPD_ERROR_ALREADY_REGISTERED = static_cast<int>(0x80E4000Cu);
constexpr int COMPANION_HTTPD_ERROR_USER_NOT_FOUND = static_cast<int>(0x80E4000Eu);

struct CompanionHttpdOptParam {
    std::uint64_t reserved;
    void* workMemory;
    std::uint64_t workMemorySize;
    std::uint8_t settings[0x40];
    const void* temporaryMountPoint;
};
static_assert(sizeof(CompanionHttpdOptParam) == 0x60);

struct CompanionHttpdHeader {
    char* key;
    char* value;
    CompanionHttpdHeader* header;
};

struct CompanionHttpdRequest {
    std::int32_t method;
    char* url;
    CompanionHttpdHeader* header;
    char* body;
    std::uint64_t bodySize;
};

struct CompanionHttpdResponse {
    std::int32_t status;
    CompanionHttpdHeader* header;
    char* body;
    std::uint64_t bodySize;
};
static_assert(sizeof(CompanionHttpdResponse) == 32);

using CompanionHttpdRequestCallback = std::int32_t(APS5_VABI*)(std::int32_t, const CompanionHttpdRequest*, CompanionHttpdResponse*, void*);
using CompanionHttpdRequestBodyReceptionCallback = std::int32_t(APS5_VABI*)(std::int32_t, std::int32_t, const CompanionHttpdRequest*, void*);

template <typename Function>
struct Registration {
    Function function = nullptr;
    void* param = nullptr;
};

std::mutex g_mutex;
bool g_initialized = false;
bool g_started = false;
Registration<CompanionHttpdRequestCallback> g_request;
Registration<CompanionHttpdRequestBodyReceptionCallback> g_bodyReception;

template <typename Function>
int Register(Registration<Function>& slot, Function function, void* param) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return COMPANION_HTTPD_ERROR_NOT_INITIALIZED;
    if (!function) return COMPANION_HTTPD_ERROR_INVALID_PARAM;
    if (slot.function) return COMPANION_HTTPD_ERROR_ALREADY_REGISTERED;
    slot = {function, param};
    return 0;
}

template <typename Function>
int Unregister(Registration<Function>& slot) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return COMPANION_HTTPD_ERROR_NOT_INITIALIZED;
    slot = {};
    return 0;
}
}

extern "C" {

int APS5_VABI sceCompanionHttpdAddHeader(const char* key, const char* value, void* response) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCompanionHttpdGet2ndScreenStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCompanionHttpdGetEvent(void* pEvent) {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return COMPANION_HTTPD_ERROR_NOT_INITIALIZED;
    if (!pEvent) return COMPANION_HTTPD_ERROR_INVALID_PARAM;
    return COMPANION_HTTPD_ERROR_NO_EVENT;
}

int APS5_VABI sceCompanionHttpdGetUserId(std::uint32_t addr, std::int32_t* userId) {
    (void)addr;
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return COMPANION_HTTPD_ERROR_NOT_INITIALIZED;
    if (!userId) return COMPANION_HTTPD_ERROR_INVALID_PARAM;
    return COMPANION_HTTPD_ERROR_USER_NOT_FOUND;
}

int APS5_VABI sceCompanionHttpdInitialize(const CompanionHttpdOptParam* param) {
    if (param == nullptr) return COMPANION_HTTPD_ERROR_INVALID_PARAM;
    std::lock_guard lock(g_mutex);
    if (g_initialized) return COMPANION_HTTPD_ERROR_ALREADY_INITIALIZED;
    g_initialized = true;
    return 0;
}

int APS5_VABI sceCompanionHttpdInitialize2(const void* option) {
    (void)option;
    std::lock_guard lock(g_mutex);
    if (g_initialized) return COMPANION_HTTPD_ERROR_ALREADY_INITIALIZED;
    g_initialized = true;
    return 0;
}

int APS5_VABI sceCompanionHttpdOptParamInitialize(CompanionHttpdOptParam* param) {
    if (param == nullptr) return COMPANION_HTTPD_ERROR_INVALID_PARAM;
    std::memset(param, 0, sizeof(*param));
    return 0;
}

int APS5_VABI sceCompanionHttpdRegisterRequestBodyReceptionCallback(CompanionHttpdRequestBodyReceptionCallback function, void* param) {
    return Register(g_bodyReception, function, param);
}

int APS5_VABI sceCompanionHttpdRegisterRequestCallback(CompanionHttpdRequestCallback function, void* param) {
    return Register(g_request, function, param);
}

int APS5_VABI sceCompanionHttpdRegisterRequestCallback2(CompanionHttpdRequestCallback function, void* param) {
    return Register(g_request, function, param);
}

int APS5_VABI sceCompanionHttpdSetBody(const char* body, std::uint64_t size, void* response) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCompanionHttpdSetStatus(std::int32_t status, CompanionHttpdResponse* response) {
    if (!response) return COMPANION_HTTPD_ERROR_INVALID_PARAM;
    response->status = status;
    return 0;
}

int APS5_VABI sceCompanionHttpdStart() {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return COMPANION_HTTPD_ERROR_NOT_INITIALIZED;
    if (g_started) return COMPANION_HTTPD_ERROR_ALREADY_STARTED;
    g_started = true;
    return 0;
}

int APS5_VABI sceCompanionHttpdStop() {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return COMPANION_HTTPD_ERROR_NOT_INITIALIZED;
    if (!g_started) return COMPANION_HTTPD_ERROR_NOT_STARTED;
    g_started = false;
    return 0;
}

int APS5_VABI sceCompanionHttpdTerminate() {
    std::lock_guard lock(g_mutex);
    if (!g_initialized) return COMPANION_HTTPD_ERROR_NOT_INITIALIZED;
    g_initialized = false;
    g_started = false;
    g_request = {};
    g_bodyReception = {};
    return 0;
}

int APS5_VABI sceCompanionHttpdUnregisterRequestBodyReceptionCallback() {
    return Unregister(g_bodyReception);
}

int APS5_VABI sceCompanionHttpdUnregisterRequestCallback() {
    return Unregister(g_request);
}

}
