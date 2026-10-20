#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

struct Header {
    char* key;
    char* value;
    Header* header;
};

struct Request {
    std::int32_t method;
    char* url;
    Header* header;
    char* body;
    std::uint64_t bodySize;
};

struct Response {
    std::int32_t status;
    Header* header;
    char* body;
    std::uint64_t bodySize;
};

struct Event {
    std::int32_t event;
    std::uint8_t data[256];
};

using RequestCallback = std::int32_t(APS5_VABI*)(std::int32_t, const Request*, Response*, void*);
using BodyReceptionCallback = std::int32_t(APS5_VABI*)(std::int32_t, std::int32_t, const Request*, void*);

}

extern "C" {
int APS5_VABI sceCompanionHttpdAddHeader(const char*, const char*, Response*);
int APS5_VABI sceCompanionHttpdGet2ndScreenStatus();
int APS5_VABI sceCompanionHttpdGetEvent(Event*);
int APS5_VABI sceCompanionHttpdGetUserId(std::uint32_t, std::int32_t*);
int APS5_VABI sceCompanionHttpdInitialize(const void*);
int APS5_VABI sceCompanionHttpdInitialize2(const void*);
int APS5_VABI sceCompanionHttpdOptParamInitialize(void*);
int APS5_VABI sceCompanionHttpdRegisterRequestBodyReceptionCallback(BodyReceptionCallback, void*);
int APS5_VABI sceCompanionHttpdRegisterRequestCallback(RequestCallback, void*);
int APS5_VABI sceCompanionHttpdRegisterRequestCallback2(RequestCallback, void*);
int APS5_VABI sceCompanionHttpdSetBody(const char*, std::uint64_t, Response*);
int APS5_VABI sceCompanionHttpdSetStatus(std::int32_t, Response*);
int APS5_VABI sceCompanionHttpdStart();
int APS5_VABI sceCompanionHttpdStop();
int APS5_VABI sceCompanionHttpdTerminate();
int APS5_VABI sceCompanionHttpdUnregisterRequestBodyReceptionCallback();
int APS5_VABI sceCompanionHttpdUnregisterRequestCallback();
}

namespace {

constexpr int InvalidParam = static_cast<int>(0x80E40004u);
constexpr int NotInitialized = static_cast<int>(0x80E40006u);
constexpr int AlreadyInitialized = static_cast<int>(0x80E40007u);
constexpr int NoEvent = static_cast<int>(0x80E40008u);
constexpr int AlreadyStarted = static_cast<int>(0x80E4000Au);
constexpr int NotStarted = static_cast<int>(0x80E4000Bu);
constexpr int AlreadyRegistered = static_cast<int>(0x80E4000Cu);
constexpr int UserNotFound = static_cast<int>(0x80E4000Eu);

void Require(bool value) {
    if (!value) std::abort();
}

template <typename Call>
bool Throws(Call call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

std::int32_t APS5_VABI OnRequest(std::int32_t, const Request*, Response*, void*) {
    std::abort();
}

std::int32_t APS5_VABI OnBody(std::int32_t, std::int32_t, const Request*, void*) {
    std::abort();
}

}

int main() {
    Event event{};
    std::memset(&event, 0x5a, sizeof(event));
    std::int32_t userId = 0x5a5a;
    int param = 0;
    Require(sceCompanionHttpdGetEvent(&event) == NotInitialized);
    Require(sceCompanionHttpdGetUserId(0x0100007f, &userId) == NotInitialized);
    Require(sceCompanionHttpdStart() == NotInitialized);
    Require(sceCompanionHttpdStop() == NotInitialized);
    Require(sceCompanionHttpdTerminate() == NotInitialized);
    Require(sceCompanionHttpdRegisterRequestCallback(OnRequest, &param) == NotInitialized);
    Require(sceCompanionHttpdRegisterRequestBodyReceptionCallback(OnBody, &param) == NotInitialized);
    Require(sceCompanionHttpdUnregisterRequestCallback() == NotInitialized);
    Require(sceCompanionHttpdUnregisterRequestBodyReceptionCallback() == NotInitialized);

    std::uint8_t block[0x70];
    std::memset(block, 0x5a, sizeof(block));
    Require(sceCompanionHttpdOptParamInitialize(nullptr) == InvalidParam);
    Require(sceCompanionHttpdOptParamInitialize(block) == 0);
    for (std::size_t index = 0; index < sizeof(block); ++index) Require(block[index] == (index < 0x60 ? 0u : 0x5au));
    Require(sceCompanionHttpdInitialize(nullptr) == InvalidParam);
    Require(sceCompanionHttpdInitialize(block) == 0);
    Require(sceCompanionHttpdInitialize(block) == AlreadyInitialized);
    Require(sceCompanionHttpdTerminate() == 0);

    const std::uint8_t option[64] = {};
    Require(sceCompanionHttpdInitialize2(option) == 0);
    Require(sceCompanionHttpdInitialize(block) == AlreadyInitialized);
    Require(sceCompanionHttpdInitialize2(option) == AlreadyInitialized);

    Require(sceCompanionHttpdGetEvent(nullptr) == InvalidParam);
    Require(sceCompanionHttpdGetEvent(&event) == NoEvent);
    Require(event.event == 0x5a5a5a5a);
    Require(sceCompanionHttpdGetUserId(0x0100007f, nullptr) == InvalidParam);
    Require(sceCompanionHttpdGetUserId(0x0100007f, &userId) == UserNotFound && userId == 0x5a5a);

    Require(sceCompanionHttpdRegisterRequestCallback(nullptr, &param) == InvalidParam);
    Require(sceCompanionHttpdRegisterRequestCallback(OnRequest, &param) == 0);
    Require(sceCompanionHttpdRegisterRequestCallback(OnRequest, &param) == AlreadyRegistered);
    Require(sceCompanionHttpdRegisterRequestCallback2(OnRequest, &param) == AlreadyRegistered);
    Require(sceCompanionHttpdUnregisterRequestCallback() == 0);
    Require(sceCompanionHttpdUnregisterRequestCallback() == 0);
    Require(sceCompanionHttpdRegisterRequestCallback2(nullptr, &param) == InvalidParam);
    Require(sceCompanionHttpdRegisterRequestCallback2(OnRequest, &param) == 0);
    Require(sceCompanionHttpdRegisterRequestCallback(OnRequest, &param) == AlreadyRegistered);

    Require(sceCompanionHttpdRegisterRequestBodyReceptionCallback(nullptr, &param) == InvalidParam);
    Require(sceCompanionHttpdRegisterRequestBodyReceptionCallback(OnBody, &param) == 0);
    Require(sceCompanionHttpdRegisterRequestBodyReceptionCallback(OnBody, &param) == AlreadyRegistered);
    Require(sceCompanionHttpdUnregisterRequestBodyReceptionCallback() == 0);
    Require(sceCompanionHttpdRegisterRequestBodyReceptionCallback(OnBody, nullptr) == 0);

    Require(sceCompanionHttpdStop() == NotStarted);
    Require(sceCompanionHttpdStart() == 0);
    Require(sceCompanionHttpdStart() == AlreadyStarted);
    Require(sceCompanionHttpdGetEvent(&event) == NoEvent);
    Require(sceCompanionHttpdStop() == 0);
    Require(sceCompanionHttpdStop() == NotStarted);
    Require(sceCompanionHttpdStart() == 0);

    Response response{};
    response.header = reinterpret_cast<Header*>(&param);
    Require(sceCompanionHttpdSetStatus(200, nullptr) == InvalidParam);
    Require(sceCompanionHttpdSetStatus(404, &response) == 0);
    Require(response.status == 404 && response.header == reinterpret_cast<Header*>(&param) && response.body == nullptr);
    Require(Throws([&] { sceCompanionHttpdAddHeader("key", "value", &response); }));
    Require(Throws([&] { sceCompanionHttpdSetBody("body", 4, &response); }));
    Require(Throws([] { sceCompanionHttpdGet2ndScreenStatus(); }));

    Require(sceCompanionHttpdTerminate() == 0);
    Require(sceCompanionHttpdTerminate() == NotInitialized);
    Require(sceCompanionHttpdStop() == NotInitialized);
    Require(sceCompanionHttpdInitialize2(nullptr) == 0);
    Require(sceCompanionHttpdStop() == NotStarted);
    Require(sceCompanionHttpdRegisterRequestCallback(OnRequest, &param) == 0);
    Require(sceCompanionHttpdRegisterRequestBodyReceptionCallback(OnBody, &param) == 0);
    Require(sceCompanionHttpdTerminate() == 0);
    return 0;
}
