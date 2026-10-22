#include "SceTypes.hpp"
#include "ProcessParametersFixture.hpp"
#include <array>
#include "prx/libc/include/general/VabiMacros.hpp"
#include <atomic>
#include <chrono>
#include <clocale>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

extern "C" {
int APS5_VABI sceHttpUriParse(SceHttpUriElement*, const char*, void*, std::size_t*, std::size_t);
int APS5_VABI sceHttpUriCopy(SceHttpUriElement*, const SceHttpUriElement*, void*, std::size_t, std::size_t*);
int APS5_VABI sceHttpUriMerge(char*, const char*, const char*, std::size_t*, std::size_t, std::uint32_t);
int APS5_VABI sceHttpSetInflateGZIPEnabled(int, int);
int APS5_VABI sceHttpSetCookieEnabled(int, int);
int APS5_VABI sceHttpSetRequestStatusCallback(int, void (APS5_VABI *)(int, int, void*), void*);
int APS5_VABI sceHttpSendRequest(int, const void*, std::size_t);
int APS5_VABI sceHttpUriBuild(char*, std::size_t*, std::size_t, const SceHttpUriElement*, std::uint32_t);
int APS5_VABI sceHttpUriEscape(char*, std::size_t*, std::size_t, const char*);
int APS5_VABI sceHttpUriUnescape(char*, std::size_t*, std::size_t, const char*);
int APS5_VABI sceHttpUriSweepPath(char*, const char*, std::size_t);
int APS5_VABI sceHttpCreateEpoll(int, HttpEpollHandle*);
int APS5_VABI sceHttpDestroyEpoll(int, HttpEpollHandle);
int APS5_VABI sceHttpWaitRequest(HttpEpollHandle, HttpNBEvent*, int, int);
int APS5_VABI sceHttpAbortWaitRequest(HttpEpollHandle);
int APS5_VABI sceHttpInit_nid_postfix(int, int, std::uint64_t);
int APS5_VABI sceHttpTerm_nid_postfix(int);
int APS5_VABI sceHttpAuthCacheFlush(int);
int APS5_VABI sceHttpCookieFlush(int);
int APS5_VABI sceHttpReadData(int, void*, std::size_t);
int APS5_VABI sceHttpCreateRequest2(int, const char*, const char*, std::uint64_t);
int APS5_VABI sceHttpsEnableOption(int, std::uint32_t);
int APS5_VABI sceHttpsLoadCert(int, int, void*, void*, void*);
int APS5_VABI sceHttpGetLastErrno(int, int*);
int APS5_VABI sceHttpSetResponseHeaderMaxSize(int, std::uint64_t);
int APS5_VABI sceHttpSetRecvBlockSize(int, std::uint32_t);
int APS5_VABI sceHttpRedirectCacheFlush(int);
int APS5_VABI sceHttpsUnloadCert(int);
int APS5_VABI sceHttpsSetSslVersion(int, int);
int APS5_VABI sceHttpsGetSslError(int, int*, std::uint32_t*);
int APS5_VABI sceHttpSetRedirectCallback(int, HttpRedirectCallback, void*);
int APS5_VABI sceHttpSetCookieRecvCallback(int, HttpCookieRecvCallback, void*);
int APS5_VABI sceHttpSetAuthInfoCallback(int, HttpAuthInfoCallback, void*);
int APS5_VABI sceHttpParseStatusLine(const char*, std::size_t, std::int32_t*, std::int32_t*, std::int32_t*, const char**, std::size_t*);
struct HttpMemoryPoolStats {
    std::uint64_t poolSize;
    std::uint64_t peakUsedSize;
    std::uint64_t usedSize;
};
struct HttpCookieStats {
    std::uint64_t usedBytes;
    std::uint32_t cookieCount;
    std::uint32_t reserved;
    std::uint64_t peakUsedBytes;
    std::uint32_t peakCookieCount;
    std::uint32_t evictedCookieCount;
};
int APS5_VABI sceHttpCreateTemplate(int, const char*, int, int);
int APS5_VABI sceHttpCreateConnectionWithURL(int, const char*, int);
int APS5_VABI sceHttpCreateConnection(int, const char*, const char*, std::uint16_t, int);
int APS5_VABI sceHttpCreateRequestWithURL(int, int, const char*, std::uint64_t);
int APS5_VABI sceHttpDeleteTemplate(int);
int APS5_VABI sceHttpDeleteConnection(int);
int APS5_VABI sceHttpDeleteRequest(int);
int APS5_VABI sceHttpSetAutoRedirect(int, int);
int APS5_VABI sceHttpGetAutoRedirect(int, int*);
int APS5_VABI sceHttpSetNonblock(int, int);
int APS5_VABI sceHttpGetNonblock(int, int*);
int APS5_VABI sceHttpTrySetNonblock(int, int);
int APS5_VABI sceHttpTryGetNonblock(int, int*);
int APS5_VABI sceHttpSetAcceptEncodingGZIPEnabled(int, int);
int APS5_VABI sceHttpGetAcceptEncodingGZIPEnabled(int, int*);
int APS5_VABI sceHttpSetDefaultAcceptEncodingGZIPEnabled(int, int);
int APS5_VABI sceHttpSetEpoll(int, HttpEpollHandle, void*);
int APS5_VABI sceHttpUnsetEpoll(int);
int APS5_VABI sceHttpGetEpoll(int, HttpEpollHandle*, void**);
int APS5_VABI sceHttpGetMemoryPoolStats(int, HttpMemoryPoolStats*);
int APS5_VABI sceHttpGetCookieStats(int, HttpCookieStats*);
int APS5_VABI sceHttpSetCookieMaxNum(int, std::uint32_t);
int APS5_VABI sceHttpSetCookieMaxNumPerDomain(int, std::uint32_t);
int APS5_VABI sceHttpSetCookieMaxSize(int, std::uint32_t);
int APS5_VABI sceHttpSetCookieTotalMaxSize(int, std::uint32_t);
int APS5_VABI sceHttpSetCookieSendCallback(int, int (APS5_VABI *)(int, const char*, const char*, void*), void*);
int APS5_VABI sceHttpSetHttp09Enabled(int, int);
int APS5_VABI sceHttpSetDelayBuildRequestEnabled(int, int);
int APS5_VABI sceHttpCacheRedirectedConnectionEnabled(int, int);
int APS5_VABI sceHttpsEnableOptionPrivate(int, std::uint32_t);
int APS5_VABI sceHttpsDisableOptionPrivate(int, std::uint32_t);
int APS5_VABI sceHttpSetProxy(int, int, int, const char*, std::uint16_t);
int APS5_VABI sceHttpAbortRequestForce(int);
int APS5_VABI sceHttpAddRequestHeader(int, const char*, const char*, std::uint32_t);
int APS5_VABI sceHttpRemoveRequestHeader(int, const char*);
}

alignas(16) static std::array<std::byte, 0x40> processParameters{};

static void Require(bool value) { if (!value) std::abort(); }

static bool Equal(const char* left, const char* right) { return std::strcmp(left, right) == 0; }

static void CheckUriParseCharacters() {
    char pool[256];
    SceHttpUriElement element{};
    Require(sceHttpUriParse(&element, "HtTpS://AZaz09-._/path", pool, nullptr, sizeof(pool)) == 0);
    Require(Equal(element.scheme, "HtTpS") && Equal(element.hostname, "AZaz09-._") && element.port == 443);
    Require(sceHttpUriParse(&element, "aZ09+.-://host/", pool, nullptr, sizeof(pool)) == 0);
    Require(Equal(element.scheme, "aZ09+.-") && element.port == 0);
    for (unsigned value = 128; value <= 255; ++value) {
        const std::string byte(1, static_cast<char>(value));
        const std::string urls[] = {
            "http://" + byte + "/", "http://[" + byte + "]/", byte + "://host/",
            "a" + byte + "://host/", "http://host:" + byte + "/"
        };
        for (const auto& url : urls) {
            std::size_t required = 0;
            Require(sceHttpUriParse(nullptr, url.c_str(), nullptr, &required, 0) == static_cast<int>(0x80433060));
            Require(sceHttpUriParse(&element, url.c_str(), pool, nullptr, sizeof(pool)) == static_cast<int>(0x80433060));
        }
    }
}

static void CheckUriEscapeBytes() {
    constexpr char unreserved[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    constexpr char hex[] = "0123456789ABCDEF";
    for (unsigned value = 1; value <= 255; ++value) {
        const char input[] = {static_cast<char>(value), '\0'};
        char expected[] = {'%', hex[value >> 4], hex[value & 15], '\0'};
        if (std::strchr(unreserved, input[0])) {
            expected[0] = input[0];
            expected[1] = '\0';
        }
        const auto size = std::strlen(expected) + 1;
        std::size_t required = 0;
        Require(sceHttpUriEscape(nullptr, &required, 0, input) == 0 && required == size);
        char output[5] = {'Z', 'Z', 'Z', 'Z', 'Z'};
        Require(sceHttpUriEscape(output, &required, size - 1, input) == static_cast<int>(0x80431022));
        for (char byte : output) Require(byte == 'Z');
        Require(sceHttpUriEscape(output, &required, size, input) == 0 && required == size);
        Require(Equal(output, expected) && output[size] == 'Z');
    }
}

static int APS5_VABI AuthInfo(int, int, const char*, char*, char*, int, std::uint8_t**, std::uint64_t*, int*, void*) { std::abort(); }

static void APS5_VABI RequestStatusCallback(int, int, void* argument) {
    ++*static_cast<int*>(argument);
}

static int APS5_VABI CookieSend(int, const char*, const char*, void*) { std::abort(); }

static int Flag(int (APS5_VABI *get)(int, int*), int id) {
    int value = -1;
    Require(get(id, &value) == 0 && (value == 0 || value == 1));
    return value;
}

static void CheckSettings() {
    constexpr int beforeInit = static_cast<int>(0x80431001);
    constexpr int invalidId = static_cast<int>(0x80431100);
    constexpr int invalidValue = static_cast<int>(0x804311FE);
    int value = -1;
    HttpEpollHandle eh = nullptr;
    void* argument = nullptr;
    HttpMemoryPoolStats pool{};
    HttpCookieStats cookies{};

    const int early = sceHttpCreateTemplate(1, "agent", 2, 1);
    Require(early > 0);
    Require(sceHttpGetAutoRedirect(early, &value) == beforeInit);
    Require(sceHttpGetNonblock(early, &value) == beforeInit);
    Require(sceHttpTryGetNonblock(early, &value) == beforeInit);
    Require(sceHttpGetAcceptEncodingGZIPEnabled(early, &value) == beforeInit);
    Require(sceHttpGetEpoll(early, &eh, &argument) == beforeInit);
    Require(sceHttpGetMemoryPoolStats(1, &pool) == beforeInit);
    Require(sceHttpGetCookieStats(1, &cookies) == beforeInit);
    Require(sceHttpSetDefaultAcceptEncodingGZIPEnabled(1, 1) == beforeInit);
    Require(sceHttpSetCookieMaxNum(1, 300) == beforeInit);
    Require(sceHttpSetCookieMaxNumPerDomain(1, 50) == beforeInit);
    Require(sceHttpSetCookieMaxSize(1, 4096) == beforeInit);
    Require(sceHttpSetCookieTotalMaxSize(1, 0x10000) == beforeInit);
    Require(sceHttpSetAutoRedirect(early, 0) == beforeInit);
    Require(sceHttpAddRequestHeader(early, "X-Early", "1", 1) == 0);
    Require(sceHttpRemoveRequestHeader(early, "X-Early") == beforeInit);

    const int context = sceHttpInit_nid_postfix(0, 0, 0x24000);
    Require(context > 0);
    Require(sceHttpGetMemoryPoolStats(-1, &pool) == invalidId);
    Require(sceHttpGetMemoryPoolStats(early, &pool) == invalidId);
    Require(sceHttpGetMemoryPoolStats(context, nullptr) == invalidValue);
    std::memset(&pool, 0x5a, sizeof(pool));
    Require(sceHttpGetMemoryPoolStats(context, &pool) == 0);
    Require(pool.poolSize == 0x24000 && pool.peakUsedSize == 0 && pool.usedSize == 0);
    Require(sceHttpGetCookieStats(-1, &cookies) == invalidId);
    Require(sceHttpGetCookieStats(context, nullptr) == invalidValue);
    std::memset(&cookies, 0x5a, sizeof(cookies));
    Require(sceHttpGetCookieStats(context, &cookies) == 0);
    const auto* cookieBytes = reinterpret_cast<const unsigned char*>(&cookies);
    for (std::size_t index = 0; index < sizeof(cookies); ++index) Require(cookieBytes[index] == 0);
    for (const int id : {context, -1}) {
        const int expected = id == context ? 0 : invalidId;
        Require(sceHttpSetCookieMaxNum(id, 300) == expected);
        Require(sceHttpSetCookieMaxNumPerDomain(id, 50) == expected);
        Require(sceHttpSetCookieMaxSize(id, 4096) == expected);
        Require(sceHttpSetCookieTotalMaxSize(id, 0x10000) == expected);
    }

    Require(Flag(sceHttpGetAutoRedirect, early) == 1);
    const int tmpl = sceHttpCreateTemplate(context, "agent", 2, 1);
    Require(sceHttpGetAutoRedirect(tmpl, nullptr) == invalidValue);
    Require(sceHttpGetNonblock(tmpl, nullptr) == invalidValue);
    Require(sceHttpGetAcceptEncodingGZIPEnabled(tmpl, nullptr) == invalidValue);
    Require(sceHttpGetAutoRedirect(-1, &value) == invalidId);
    Require(sceHttpGetAutoRedirect(context, &value) == invalidId);
    Require(sceHttpGetNonblock(context, &value) == invalidId);
    Require(Flag(sceHttpGetAutoRedirect, tmpl) == 1);
    Require(Flag(sceHttpGetNonblock, tmpl) == 0 && Flag(sceHttpTryGetNonblock, tmpl) == 0);
    Require(Flag(sceHttpGetAcceptEncodingGZIPEnabled, tmpl) == 0);
    Require(sceHttpGetEpoll(tmpl, nullptr, &argument) == invalidValue);
    Require(sceHttpGetEpoll(-1, &eh, &argument) == invalidId);
    eh = reinterpret_cast<HttpEpollHandle>(&value);
    argument = &value;
    Require(sceHttpGetEpoll(tmpl, &eh, &argument) == 0 && eh == nullptr && argument == nullptr);

    constexpr int notFound = static_cast<int>(0x80431025);
    Require(sceHttpRemoveRequestHeader(early, "x-early") == 0);
    Require(sceHttpRemoveRequestHeader(early, "X-Early") == notFound);
    Require(sceHttpAddRequestHeader(tmpl, "X-Test", "1", 1) == 0);
    Require(sceHttpAddRequestHeader(tmpl, "x-TEST", "2", 1) == 0);
    Require(sceHttpAddRequestHeader(tmpl, "X-Mode", "1", 0) == 0);
    Require(sceHttpAddRequestHeader(tmpl, "X-Mode", "2", 0) == 0);
    Require(sceHttpAddRequestHeader(tmpl, "X-Bad", "1", 2) == 0);
    Require(sceHttpAddRequestHeader(tmpl, nullptr, "1", 1) == 0);
    Require(sceHttpRemoveRequestHeader(tmpl, nullptr) == notFound);
    Require(sceHttpRemoveRequestHeader(-1, "X-Test") == invalidId);
    Require(sceHttpRemoveRequestHeader(context, "X-Test") == invalidId);
    Require(sceHttpRemoveRequestHeader(tmpl, "X-Bad") == notFound);
    Require(sceHttpRemoveRequestHeader(tmpl, "X-Tes") == notFound);
    Require(sceHttpRemoveRequestHeader(tmpl, "x-test") == 0);
    Require(sceHttpRemoveRequestHeader(tmpl, "X-Test") == notFound);
    Require(sceHttpRemoveRequestHeader(tmpl, "X-MODE") == 0);
    Require(sceHttpRemoveRequestHeader(tmpl, "X-Mode") == notFound);
    Require(sceHttpAddRequestHeader(tmpl, "X-Template", "1", 1) == 0);

    HttpEpollHandle epoll = nullptr;
    Require(sceHttpCreateEpoll(context, &epoll) == 0);
    int marker = 0;
    Require(sceHttpSetAutoRedirect(tmpl, 0) == 0);
    Require(sceHttpTrySetNonblock(tmpl, 1) == 0);
    Require(sceHttpSetAcceptEncodingGZIPEnabled(tmpl, 2) == 0);
    Require(sceHttpSetEpoll(tmpl, epoll, &marker) == 0);
    Require(Flag(sceHttpGetAutoRedirect, tmpl) == 0 && Flag(sceHttpGetNonblock, tmpl) == 1);
    Require(Flag(sceHttpGetAcceptEncodingGZIPEnabled, tmpl) == 1);
    Require(sceHttpGetEpoll(tmpl, &eh, nullptr) == 0 && eh == epoll);
    Require(sceHttpGetEpoll(tmpl, &eh, &argument) == 0 && eh == epoll && argument == &marker);

    const int connection = sceHttpCreateConnectionWithURL(tmpl, "http://example.com/", 1);
    Require(sceHttpRemoveRequestHeader(connection, "X-Template") == notFound);
    Require(sceHttpRemoveRequestHeader(tmpl, "X-Template") == 0);
    Require(Flag(sceHttpGetAutoRedirect, connection) == 0 && Flag(sceHttpTryGetNonblock, connection) == 1);
    Require(Flag(sceHttpGetAcceptEncodingGZIPEnabled, connection) == 1);
    Require(sceHttpGetEpoll(connection, &eh, &argument) == 0 && eh == epoll && argument == &marker);
    Require(sceHttpSetNonblock(connection, 0) == 0 && sceHttpSetAutoRedirect(connection, 1) == 0);
    Require(Flag(sceHttpGetNonblock, connection) == 0 && Flag(sceHttpGetNonblock, tmpl) == 1);
    Require(Flag(sceHttpGetAutoRedirect, connection) == 1 && Flag(sceHttpGetAutoRedirect, tmpl) == 0);
    const int request = sceHttpCreateRequestWithURL(connection, 0, "http://example.com/a", 0);
    Require(Flag(sceHttpGetNonblock, request) == 0 && Flag(sceHttpGetAutoRedirect, request) == 1);
    Require(sceHttpUnsetEpoll(request) == 0);
    Require(sceHttpGetEpoll(request, &eh, &argument) == 0 && eh == nullptr && argument == nullptr);
    Require(sceHttpGetEpoll(connection, &eh, &argument) == 0 && eh == epoll);
    Require(sceHttpAbortRequestForce(request) == 0);
    Require(sceHttpDeleteRequest(request) == 0);
    Require(sceHttpGetAutoRedirect(request, &value) == invalidId);
    Require(sceHttpGetEpoll(request, &eh, &argument) == invalidId);
    const int plain = sceHttpCreateConnection(tmpl, "example.com", "http", 80, 1);
    Require(Flag(sceHttpGetNonblock, plain) == 1);
    Require(sceHttpDeleteConnection(plain) == 0 && sceHttpDeleteConnection(connection) == 0);
    Require(sceHttpGetNonblock(connection, &value) == invalidId);
    Require(sceHttpDeleteTemplate(tmpl) == 0);
    Require(sceHttpGetAcceptEncodingGZIPEnabled(tmpl, &value) == invalidId);

    Require(sceHttpSetDefaultAcceptEncodingGZIPEnabled(-1, 1) == invalidId);
    Require(sceHttpSetDefaultAcceptEncodingGZIPEnabled(context, 1) == 0);
    const int compressed = sceHttpCreateTemplate(context, "agent", 2, 1);
    Require(Flag(sceHttpGetAcceptEncodingGZIPEnabled, compressed) == 1);
    Require(Flag(sceHttpGetAcceptEncodingGZIPEnabled, early) == 0);
    Require(sceHttpSetDefaultAcceptEncodingGZIPEnabled(context, 0) == 0);
    Require(Flag(sceHttpGetAcceptEncodingGZIPEnabled, compressed) == 1);
    Require(Flag(sceHttpGetAcceptEncodingGZIPEnabled, sceHttpCreateTemplate(context, "agent", 2, 1)) == 0);

    Require(sceHttpSetHttp09Enabled(compressed, 1) == 0);
    Require(sceHttpSetDelayBuildRequestEnabled(compressed, 1) == 0);
    Require(sceHttpCacheRedirectedConnectionEnabled(compressed, 0) == 0);
    Require(sceHttpSetCookieSendCallback(compressed, CookieSend, &marker) == 0);
    Require(sceHttpsEnableOptionPrivate(compressed, 0x400) == 0);
    Require(sceHttpsDisableOptionPrivate(compressed, 0x400) == 0);
    Require(sceHttpSetProxy(compressed, 1, 1, nullptr, 8080) == invalidValue);
    Require(sceHttpSetProxy(compressed, 1, 1, "proxy.example", 8080) == 0);

    Require(sceHttpDestroyEpoll(context, epoll) == 0);
    Require(sceHttpTerm_nid_postfix(context) == 0);
    Require(sceHttpGetAutoRedirect(compressed, &value) == beforeInit);
    Require(sceHttpGetMemoryPoolStats(context, &pool) == beforeInit);
}

int main() {
    const std::uint64_t processSize = processParameters.size();
    const std::uint32_t processMagic = 0x4942524f;
    std::uint32_t sdkVersion = 0x1000000;
    std::memcpy(processParameters.data(), &processSize, sizeof(processSize));
    std::memcpy(processParameters.data() + 8, &processMagic, sizeof(processMagic));
    std::memcpy(processParameters.data() + 16, &sdkVersion, sizeof(sdkVersion));
    Require(ProcessParametersFixture::Install(processParameters.data(), processParameters.size()));
    for (const std::uint32_t version : {0u, 0xffffffu, 0x1000000u}) {
        std::memcpy(processParameters.data() + 16, &version, sizeof(version));
        for (unsigned value = 1; value < 256; ++value) {
            if (value == '%') continue;
            const char input[]{static_cast<char>(value), 0};
            char output[]{'x', 'x'};
            std::size_t required = 123;
            const bool allowed = version >= 0x1000000 || (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') || value == '*' || value == '-' || value == '.' || value == '_' || value == '+';
            Require(sceHttpUriUnescape(output, &required, sizeof(output), input) == (allowed ? 0 : static_cast<int>(0x80433060)));
            Require(allowed ? output[0] == input[0] && output[1] == 0 && required == 2 : output[0] == 'x' && output[1] == 'x' && required == 123);
        }
        char output[2]{};
        Require(sceHttpUriUnescape(output, nullptr, sizeof(output), "%7E") == 0 && output[0] == '~');
    }
    std::memcpy(processParameters.data() + 16, &sdkVersion, sizeof(sdkVersion));
    CheckSettings();
    int callbackCount = 0;
    const auto request = sceHttpCreateRequest2(1, "GET", "/", 0);
    Require(sceHttpSetRequestStatusCallback(request, RequestStatusCallback, &callbackCount) == 0);
    Require(sceHttpSendRequest(request, nullptr, 0) == static_cast<int>(0x80431063u));
    Require(callbackCount == 0);
    Require(sceHttpSetRequestStatusCallback(request, nullptr, nullptr) == 0);
    Require(sceHttpSetRequestStatusCallback(-1, RequestStatusCallback, &callbackCount) == static_cast<int>(0x80431100u));
    constexpr int outOfMemory = static_cast<int>(0x80431022);
    constexpr int invalidValue = static_cast<int>(0x804311FE);
    constexpr int invalidUrl = static_cast<int>(0x80433060);
    constexpr int network = static_cast<int>(0x80431063);
    constexpr std::uint32_t buildAll = 0xFF;

    const char* url = "https://user:secret@example.com:8443/a/./b/../c?x=1&y=2#top";
    std::size_t required = 0;
    Require(sceHttpUriParse(nullptr, url, nullptr, &required, 0) == 0);
    Require(required == 6 + 5 + 7 + 12 + 5 + 9 + 5);

    char pool[256];
    SceHttpUriElement element;
    Require(sceHttpUriParse(&element, url, pool, nullptr, required - 1) == outOfMemory);
    Require(sceHttpUriParse(&element, url, pool, nullptr, required) == 0);
    Require(element.opaque == 0);
    Require(Equal(element.scheme, "https") && Equal(element.username, "user") && Equal(element.password, "secret"));
    Require(Equal(element.hostname, "example.com") && element.port == 8443);
    Require(Equal(element.path, "/a/c") && Equal(element.query, "?x=1&y=2") && Equal(element.fragment, "#top"));

    char built[256];
    Require(sceHttpUriBuild(nullptr, &required, 0, &element, buildAll) == 0);
    Require(required == std::strlen("https://user:secret@example.com:8443/a/c?x=1&y=2#top") + 1);
    Require(sceHttpUriBuild(built, nullptr, required - 1, &element, buildAll) == outOfMemory);
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, buildAll) == 0);
    Require(Equal(built, "https://user:secret@example.com:8443/a/c?x=1&y=2#top"));
    Require(sceHttpUriBuild(nullptr, &required, 0, &element, 0x01) == 0);
    Require(required == sizeof("https://"));
    std::memset(built, 'Z', sizeof(built));
    Require(sceHttpUriBuild(built, nullptr, required - 1, &element, 0x01) == outOfMemory);
    for (char c : built) Require(c == 'Z');
    Require(sceHttpUriBuild(built, nullptr, required, &element, 0x01) == 0);
    Require(Equal(built, "https://") && built[required] == 'Z');
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, 0) == 0);
    Require(Equal(built, "//"));
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, 0x02) == 0);
    Require(Equal(built, "//example.com"));
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, 0x08 | 0x40) == 0);
    Require(Equal(built, "///a/c?x=1&y=2"));

    Require(sceHttpUriParse(&element, "HTTP://Example.com/", pool, nullptr, sizeof(pool)) == 0);
    Require(element.port == 80 && Equal(element.username, "") && Equal(element.query, ""));
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, buildAll) == 0);
    Require(Equal(built, "HTTP://Example.com/"));

    Require(sceHttpUriParse(&element, "http://[::1]:8080/index.html", pool, nullptr, sizeof(pool)) == 0);
    Require(Equal(element.hostname, "::1") && element.port == 8080 && Equal(element.path, "/index.html"));
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, buildAll) == 0);
    Require(Equal(built, "http://::1:8080/index.html"));

    Require(sceHttpUriParse(&element, "mailto:someone@example.com", pool, nullptr, sizeof(pool)) == 0);
    Require(element.opaque != 0 && element.port == 0);
    Require(Equal(element.username, "someone") && Equal(element.hostname, "example.com"));
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, buildAll) == 0);
    Require(Equal(built, "mailto:someone@example.com"));
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, 0x01) == 0);
    Require(Equal(built, "mailto:"));
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), &element, 0) == 0);
    Require(Equal(built, ""));

    Require(sceHttpUriParse(&element, "/path/only?q", pool, nullptr, sizeof(pool)) == 0);
    Require(Equal(element.scheme, "") && Equal(element.hostname, "") && Equal(element.path, "/path/only"));

    Require(sceHttpUriParse(&element, nullptr, pool, nullptr, sizeof(pool)) == invalidUrl);
    Require(sceHttpUriParse(nullptr, url, nullptr, nullptr, 0) == invalidValue);
    Require(sceHttpUriParse(nullptr, "http://host:65536/", nullptr, &required, 0) == invalidUrl);
    Require(sceHttpUriParse(nullptr, "http://host:80a/", nullptr, &required, 0) == invalidUrl);
    Require(sceHttpUriParse(nullptr, "http://[::1/", nullptr, &required, 0) == invalidUrl);
    Require(sceHttpUriParse(nullptr, "http://bad host/", nullptr, &required, 0) == invalidUrl);
    Require(sceHttpUriBuild(built, nullptr, sizeof(built), nullptr, buildAll) == invalidUrl);
    Require(sceHttpUriBuild(nullptr, nullptr, 0, &element, buildAll) == invalidValue);

    {
        SceHttpUriElement bounded{};
        bounded.opaque = 2;
        char scheme[33];
        char hostname[256];
        std::memset(scheme, 's', sizeof(scheme));
        std::memset(hostname, 'h', sizeof(hostname));
        bounded.scheme = scheme;
        bounded.hostname = hostname;
        char output[512];
        Require(sceHttpUriBuild(output, &required, sizeof(output), &bounded, 3) == 0);
        Require(required == 32 + 1 + 2 + 255 + 1);
        Require(output[32] == ':' && output[33] == '/' && output[34] == '/');
        Require(output[required - 1] == '\0');
        scheme[0] = '\0';
        Require(sceHttpUriBuild(output, &required, sizeof(output), &bounded, 1) == 0);
        Require(required == 4 && Equal(output, "//"));
        char prefix[] = "HTTPS-extra";
        bounded.scheme = prefix;
        bounded.port = 443;
        Require(sceHttpUriBuild(output, &required, sizeof(output), &bounded, 4) == 0);
        Require(required == 3 && Equal(output, "//"));
    }

    char escaped[64];
    Require(sceHttpUriEscape(nullptr, &required, 0, "a b/~\xC3\xA9") == 0);
    Require(required == std::strlen("a%20b%2F~%C3%A9") + 1);
    Require(sceHttpUriEscape(escaped, nullptr, required - 1, "a b/~\xC3\xA9") == outOfMemory);
    Require(sceHttpUriEscape(escaped, nullptr, sizeof(escaped), "a b/~\xC3\xA9") == 0);
    Require(Equal(escaped, "a%20b%2F~%C3%A9"));
    Require(sceHttpUriEscape(escaped, nullptr, sizeof(escaped), nullptr) == invalidValue);
    const std::string previousCtype = std::setlocale(LC_CTYPE, nullptr);
    Require(std::setlocale(LC_CTYPE, "C") != nullptr);
    CheckUriEscapeBytes();
    CheckUriParseCharacters();
#ifdef _WIN32
    Require(std::setlocale(LC_CTYPE, ".1252") != nullptr);
    CheckUriEscapeBytes();
    CheckUriParseCharacters();
#else
    for (const char* locale : {"en_US.ISO8859-1", "en_US.iso88591", "en_US.UTF-8"}) {
        if (!std::setlocale(LC_CTYPE, locale)) continue;
        CheckUriEscapeBytes();
        CheckUriParseCharacters();
    }
#endif
    Require(std::setlocale(LC_CTYPE, previousCtype.c_str()) != nullptr);

    constexpr const char* unescapeCases[][2] = {
        {"", ""}, {"plain+text", "plain+text"}, {"a%20b%2F~%c3%a9", "a b/~\xC3\xA9"},
        {"%41%4a%4F%ff", "AJO\xFF"}, {"%2520", "%20"}
    };
    for (const auto& row : unescapeCases) {
        const std::size_t size = std::strlen(row[1]) + 1;
        Require(sceHttpUriUnescape(nullptr, &required, 0, row[0]) == 0 && required == size);
        std::memset(escaped, 'Z', sizeof(escaped));
        required = 123;
        Require(sceHttpUriUnescape(escaped, &required, size - 1, row[0]) == outOfMemory && required == 123);
        Require(std::memcmp(escaped, row[1], size - 1) == 0 && escaped[size - 1] == 'Z');
        Require(sceHttpUriUnescape(escaped, nullptr, size, row[0]) == 0);
        Require(Equal(escaped, row[1]) && escaped[size] == 'Z');
    }
    for (const char* malformed : {"%", "%1", "%1g", "%gg", "%+1", "%-1", "% 1", "abc%", "%%41", "%4%41"}) {
        std::memset(escaped, 'Z', sizeof(escaped));
        required = 77;
        Require(sceHttpUriUnescape(escaped, &required, sizeof(escaped), malformed) == invalidUrl && required == 77 && escaped[0] == 'Z');
        Require(sceHttpUriUnescape(nullptr, &required, 0, malformed) == invalidUrl && required == 77);
    }
    Require(sceHttpUriUnescape(escaped, &required, sizeof(escaped), "a%00b") == 0);
    Require(required == 4 && std::memcmp(escaped, "a\0b", 4) == 0);
    std::strcpy(escaped, "%41%2f%2520");
    Require(sceHttpUriUnescape(escaped, &required, sizeof(escaped), escaped) == 0);
    Require(Equal(escaped, "A/%20") && required == 6);
    Require(sceHttpUriUnescape(nullptr, nullptr, 0, "valid") == invalidValue);
    required = 123;
    Require(sceHttpUriUnescape(escaped, &required, sizeof(escaped), nullptr) == invalidValue);
    Require(required == 123 && Equal(escaped, "A/%20"));

    for (const char* malformed : {"%", "%1", "%1g", "%gg", "%+1", "%-1", "prefix%4", "prefix%xy"}) {
        std::memset(escaped, 'Z', sizeof(escaped));
        required = 123;
        Require(sceHttpUriUnescape(escaped, &required, sizeof(escaped), malformed) == static_cast<int>(0x80433060));
        Require(required == 123);
        for (char value : escaped) Require(value == 'Z');
    }
    std::memset(escaped, 'Z', sizeof(escaped));
    Require(sceHttpUriUnescape(escaped, &required, 2, "a%20b") == outOfMemory);
    Require(required == 123 && escaped[0] == 'a' && escaped[1] == ' ' && escaped[2] == 'Z');

    char bytes[256], encodedBytes[766], decodedBytes[256];
    for (std::size_t i = 1; i < 256; ++i) bytes[i - 1] = static_cast<char>(i);
    bytes[255] = '\0';
    Require(sceHttpUriEscape(encodedBytes, nullptr, sizeof(encodedBytes), bytes) == 0);
    Require(sceHttpUriUnescape(decodedBytes, &required, sizeof(decodedBytes), encodedBytes) == 0);
    Require(required == sizeof(bytes) && std::memcmp(bytes, decodedBytes, sizeof(bytes)) == 0);

    const char* base = "http://foo.com/foo/index.html";
    const std::size_t baseMergeSize = 5 + 1 + 1 + 8 + 16 + 1 + 1 + 2;
    char merged[512];
    auto merge = [&](const char* mergeBase, const char* relative, const char* expected) {
        std::memset(merged, 'Z', sizeof(merged));
        return sceHttpUriMerge(merged, mergeBase, relative, &required, sizeof(merged), 0) == 0 && Equal(merged, expected);
    };
    Require(merge(base, "./default.html", "http://foo.com/foo/./default.html"));
    Require(required == baseMergeSize + 2 * (29 + 14));
    Require(merge(base, "../sibling.html", "http://foo.com/foo/../sibling.html"));
    Require(merge(base, "", "http://foo.com/foo/"));
    Require(merge(base, "/root.html", "http://foo.com/root.html"));
    Require(merge(base, "a?q=1#f", "http://foo.com/foo/a?q=1#f"));
    Require(merge("https://u:p@foo.com:8443/a/b?x=1#top", "c", "https://u:p@foo.com:8443/a/c"));
    Require(merge("http://foo.com", "x", "http://foo.com/x"));
    Require(merge("http://foo.com:80/", "x", "http://foo.com/x"));
    Require(merge("http://foo.com/a/b", "mailto:x", "http://foo.com/a/mailto:x"));
    Require(merge(base, "a%20b.html", "http://foo.com/foo/a%20b.html"));
    Require(merge(base, "~user/x", "http://foo.com/foo/~user/x"));
    Require(merge(base, "file(1).png", "http://foo.com/foo/file(1).png"));
    Require(merge(base, "a+b=c;d!e", "http://foo.com/foo/a+b=c;d!e"));

    Require(merge(base, "http://bar.com/other", "http://bar.com/other") && required == 21);
    const std::size_t absoluteSize = baseMergeSize + 2 * (29 + 20);
    for (std::size_t i = 21; i < absoluteSize; ++i) Require(merged[i] == '\0');
    Require(merged[absoluteSize] == 'Z');
    Require(merge(base, "//bar.com/x", "//bar.com/x") && required == 12);

    required = 0;
    Require(sceHttpUriMerge(nullptr, base, "./default.html", &required, 0, 0) == 0);
    Require(required == baseMergeSize + 2 * (29 + 14));
    Require(sceHttpUriMerge(nullptr, base, "http://bar.com/other", &required, 0, 0) == 0);
    Require(required == absoluteSize);
    Require(sceHttpUriMerge(nullptr, base, "x", nullptr, 0, 0) == 0);
    std::memset(merged, 'Z', sizeof(merged));
    Require(sceHttpUriMerge(merged, base, "./default.html", &required, baseMergeSize + 2 * (29 + 14) - 1, 0) == outOfMemory);
    Require(required == baseMergeSize + 2 * (29 + 14) && merged[0] == 'Z');
    Require(sceHttpUriMerge(merged, base, "http://bar.com/other", &required, absoluteSize - 1, 0) == outOfMemory);
    Require(merged[0] == 'Z');
    Require(sceHttpUriMerge(merged, base, "./default.html", nullptr, baseMergeSize + 2 * (29 + 14), 0) == 0);
    Require(Equal(merged, "http://foo.com/foo/./default.html"));

    required = 123;
    Require(sceHttpUriMerge(merged, nullptr, "./x", &required, sizeof(merged), 0) == invalidValue);
    Require(sceHttpUriMerge(merged, base, nullptr, &required, sizeof(merged), 0) == invalidValue);
    Require(sceHttpUriMerge(merged, base, "./x", &required, sizeof(merged), 1) == invalidValue);
    Require(sceHttpUriMerge(nullptr, nullptr, nullptr, &required, 0, 1) == invalidValue);
    Require(sceHttpUriMerge(merged, "http://bad host/", "./x", &required, sizeof(merged), 0) == invalidUrl);
    Require(sceHttpUriMerge(merged, base, "http://bad host/", &required, sizeof(merged), 0) == invalidUrl);
    Require(required == 123);

    constexpr int beforeInit = static_cast<int>(0x80431001);
    constexpr int aborted = static_cast<int>(0x80431080);
    constexpr int invalidId = static_cast<int>(0x80431100);
    const auto flushCaches = [](int id, int expected) {
        Require(sceHttpAuthCacheFlush(id) == expected);
        Require(sceHttpCookieFlush(id) == expected);
    };
    flushCaches(0, beforeInit);
    flushCaches(-1, beforeInit);
    flushCaches(request, beforeInit);
    HttpEpollHandle epoll = nullptr;
    HttpNBEvent events[2]{};
    Require(sceHttpCreateEpoll(1, &epoll) == beforeInit);
    Require(sceHttpAbortWaitRequest(nullptr) == beforeInit);
    Require(sceHttpTerm_nid_postfix(1) == beforeInit);
    const int context = sceHttpInit_nid_postfix(0, 0, 0x10000);
    Require(context > 0);
    flushCaches(context, 0);
    flushCaches(context, 0);
    flushCaches(0, invalidId);
    flushCaches(-1, invalidId);
    flushCaches(INT32_MAX, invalidId);
    flushCaches(request, invalidId);
    Require(sceHttpCreateEpoll(-1, &epoll) == invalidId);
    Require(sceHttpCreateEpoll(context, nullptr) == invalidValue);
    Require(sceHttpCreateEpoll(context, &epoll) == 0 && epoll != nullptr);
    Require(sceHttpWaitRequest(epoll, events, 2, 0) == 0);
    Require(sceHttpWaitRequest(epoll, events, 2, 1000) == 0);
    Require(sceHttpWaitRequest(epoll, nullptr, 2, 0) == invalidValue);
    Require(sceHttpWaitRequest(epoll, events, 0, 0) == invalidValue);
    Require(sceHttpWaitRequest(nullptr, events, 2, 0) == invalidValue);
    const auto unknownEpoll = reinterpret_cast<HttpEpollHandle>(&events);
    Require(sceHttpAbortWaitRequest(nullptr) == invalidValue);
    Require(sceHttpAbortWaitRequest(unknownEpoll) == invalidId);
    Require(sceHttpWaitRequest(unknownEpoll, events, 2, -1) == invalidId);
    Require(sceHttpDestroyEpoll(context, nullptr) == invalidValue);
    Require(sceHttpDestroyEpoll(context, unknownEpoll) == invalidId);
    Require(sceHttpDestroyEpoll(-1, epoll) == invalidId);

    auto waitWokenBy = [&](auto wake) {
        std::atomic<bool> started{false};
        std::atomic<int> result{1};
        std::thread waiter([&] {
            started = true;
            result = sceHttpWaitRequest(epoll, events, 2, -1);
        });
        while (!started) std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        Require(result == 1);
        wake();
        waiter.join();
        return result.load();
    };
    Require(waitWokenBy([&] { Require(sceHttpAbortWaitRequest(epoll) == 0); }) == aborted);
    Require(sceHttpWaitRequest(epoll, events, 2, 0) == aborted);
    Require(sceHttpWaitRequest(epoll, events, 2, -1) == aborted);
    Require(sceHttpDestroyEpoll(context, epoll) == 0);
    Require(sceHttpAbortWaitRequest(epoll) == invalidId);
    Require(sceHttpWaitRequest(epoll, events, 2, 0) == invalidId);
    Require(sceHttpDestroyEpoll(context, epoll) == invalidId);
    HttpEpollHandle next = nullptr;
    Require(sceHttpCreateEpoll(context, &next) == 0 && next != nullptr && next != epoll);
    Require(sceHttpAbortWaitRequest(epoll) == invalidId);
    Require(sceHttpWaitRequest(next, events, 2, 0) == 0);
    Require(sceHttpDestroyEpoll(context, next) == 0);

    Require(sceHttpCreateEpoll(context, &epoll) == 0);
    Require(waitWokenBy([&] { Require(sceHttpDestroyEpoll(context, epoll) == 0); }) == 0);

    Require(sceHttpCreateEpoll(context, &epoll) == 0);
    const int otherContext = sceHttpInit_nid_postfix(0, 0, 0x10000);
    Require(otherContext > 0 && otherContext != context);
    flushCaches(otherContext, 0);
    Require(sceHttpTerm_nid_postfix(otherContext) == 0);
    flushCaches(otherContext, invalidId);
    flushCaches(context, 0);
    Require(sceHttpTerm_nid_postfix(otherContext) == invalidId);
    Require(sceHttpWaitRequest(epoll, events, 2, 0) == 0);
    Require(waitWokenBy([&] { Require(sceHttpTerm_nid_postfix(context) == 0); }) == 0);
    Require(sceHttpWaitRequest(epoll, events, 2, 0) == beforeInit);
    Require(sceHttpTerm_nid_postfix(context) == beforeInit);
    flushCaches(context, beforeInit);
    const int newContext = sceHttpInit_nid_postfix(0, 0, 0x10000);
    Require(newContext > 0 && newContext != context && newContext != otherContext);
    flushCaches(context, invalidId);
    flushCaches(otherContext, invalidId);
    flushCaches(newContext, 0);
    Require(sceHttpTerm_nid_postfix(newContext) == 0);
    flushCaches(newContext, beforeInit);

    char data[16];
    Require(sceHttpReadData(1, data, sizeof(data)) == network);

    Require(sceHttpCreateRequest2(1, "GET", "/", 0) > 0);
    Require(sceHttpSetInflateGZIPEnabled(1, 0) == 0);
    Require(sceHttpSetInflateGZIPEnabled(1, 1) == 0);
    Require(sceHttpSetInflateGZIPEnabled(1, 2) == invalidValue);
    Require(sceHttpSetInflateGZIPEnabled(1, -1) == invalidValue);
    Require(sceHttpSetCookieEnabled(1, 2) == invalidValue);
    Require(sceHttpSetCookieEnabled(1, -1) == invalidValue);
    Require(sceHttpReadData(1, data, sizeof(data)) == network);
    Require(sceHttpsEnableOption(1, 0) == 0);
    Require(sceHttpsLoadCert(1, 0, nullptr, nullptr, nullptr) == 0);
    Require(sceHttpsUnloadCert(1) == 0);
    Require(sceHttpSetResponseHeaderMaxSize(1, 8192) == 0);
    Require(sceHttpSetRecvBlockSize(1, 0x4000) == 0);
    Require(sceHttpRedirectCacheFlush(1) == 0);
    int authUserArg = 0;
    Require(sceHttpSetAuthInfoCallback(1, AuthInfo, &authUserArg) == 0);
    Require(sceHttpReadData(1, data, sizeof(data)) == network);
    int httpErrno = -1;
    Require(sceHttpGetLastErrno(1, &httpErrno) == 0);
    Require(httpErrno == 0);
    Require(sceHttpGetLastErrno(1, nullptr) == invalidValue);
    Require(sceHttpsSetSslVersion(1, 0) == 0);
    Require(sceHttpSetRedirectCallback(1, nullptr, nullptr) == 0);
    Require(sceHttpSetCookieRecvCallback(1, nullptr, nullptr) == 0);
    int sslError = -1;
    std::uint32_t sslDetail = 0xFFFFFFFFu;
    Require(sceHttpsGetSslError(1, &sslError, &sslDetail) == 0);
    Require(sslError == 0 && sslDetail == 0);
    Require(sceHttpsGetSslError(1, nullptr, &sslDetail) == invalidValue);
    Require(sceHttpsGetSslError(1, &sslError, nullptr) == invalidValue);

    constexpr int parseInvalidResponse = static_cast<int>(0x80432060);
    constexpr int parseInvalidValue = static_cast<int>(0x804321FE);
    std::int32_t major = -1;
    std::int32_t minor = -1;
    std::int32_t code = -1;
    const char* phrase = nullptr;
    std::size_t phraseLength = 0;
    auto parse = [&](const char* line, std::size_t length) {
        return sceHttpParseStatusLine(line, length, &major, &minor, &code, &phrase, &phraseLength);
    };

    const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    Require(parse(response, std::strlen(response)) == 17);
    Require(major == 1 && minor == 1 && code == 200);
    Require(phrase == response + 12 && phraseLength == 3 && std::strncmp(phrase, " OK", 3) == 0);

    const char* lineFeedOnly = "HTTP/10.25 404 Not Found\n";
    Require(parse(lineFeedOnly, std::strlen(lineFeedOnly)) == 25);
    Require(major == 10 && minor == 25 && code == 404);
    Require(phrase == lineFeedOnly + 14 && phraseLength == 10);

    const char* noPhrase = "HTTP/2.0 204\r\n";
    Require(parse(noPhrase, std::strlen(noPhrase)) == 14);
    Require(major == 2 && minor == 0 && code == 204);
    Require(phrase == noPhrase + 12 && phraseLength == 0);

    phrase = nullptr;
    phraseLength = 0;
    Require(parse(nullptr, 17) == parseInvalidResponse);
    Require(sceHttpParseStatusLine(response, 17, nullptr, &minor, &code, &phrase, &phraseLength) == parseInvalidValue);
    Require(sceHttpParseStatusLine(response, 17, &major, &minor, &code, &phrase, nullptr) == parseInvalidValue);
    major = -1;
    minor = -1;
    Require(parse("HTTX/1.1 200 OK\n", 16) == parseInvalidResponse);
    Require(major == 0 && minor == 0);
    Require(parse("HTTP/1.1", 7) == parseInvalidResponse);
    Require(parse("HTTP/123", 8) == parseInvalidResponse);
    Require(parse("HTTP/x.1 200 OK\n", 16) == parseInvalidResponse);
    Require(parse("HTTP/1 200 OK\n", 14) == parseInvalidResponse);
    Require(parse("HTTP/1. 200 OK\n", 15) == parseInvalidResponse);
    Require(parse("HTTP/1.1/200 OK\n", 16) == parseInvalidResponse);
    Require(parse("HTTP/1.1 2x0 OK\n", 16) == parseInvalidResponse);
    Require(parse("HTTP/1.1 20", 11) == parseInvalidResponse);
    Require(parse(response, 15) == parseInvalidResponse);
    Require(parse(response, 16) == parseInvalidResponse);
    Require(phrase == nullptr && phraseLength == 0);
    auto sweep = [](const char* src, const char* expected) {
        char swept[128];
        std::memset(swept, 'x', sizeof(swept));
        return sceHttpUriSweepPath(swept, src, std::strlen(src) + 1) == 0 && Equal(swept, expected);
    };
    Require(sceHttpUriSweepPath(nullptr, nullptr, 0) == 0);
    Require(sceHttpUriSweepPath(nullptr, "/foo", 5) == invalidValue);
    char sweptPath[16];
    Require(sceHttpUriSweepPath(sweptPath, nullptr, 5) == invalidValue);
    Require(sweep("foo/../bar", "foo/../bar"));
    Require(sweep("/foo/../bar", "/bar"));
    Require(sweep("/foo/./bar", "/foo/bar"));
    Require(sweep("/foo/.", "/foo/."));
    Require(sweep("/foo/..", "/foo/.."));
    Require(sweep("/foo/bar/../foo/././../../../test/index.html", "/test/index.html"));
    Require(sweep("/", "/"));
    Require(sweep("", ""));
    Require(sweep("/a/b/c/../../d/", "/a/d/"));
    Require(sweep("/../a", "/a"));
    Require(sweep("/a/..b/.c", "/a/..b/.c"));
    Require(sceHttpUriSweepPath(sweptPath, "/a/b/../c", 6) == 0 && Equal(sweptPath, "/a/b/"));
    Require(sceHttpUriSweepPath(sweptPath, "/ab/c", 3) == 0 && Equal(sweptPath, "/a"));
    Require(sceHttpUriSweepPath(sweptPath, "/a/./b", 5) == 0 && Equal(sweptPath, "/a/."));
    Require(sceHttpUriSweepPath(sweptPath, "/a/b/../c", 8) == 0 && Equal(sweptPath, "/a/"));
    Require(sceHttpUriSweepPath(sweptPath, "/a/b/../c", 5) == 0 && Equal(sweptPath, "/a/b"));
    Require(sweep("/a/b/..", "/a/b/.."));
    Require(sweep("/a/b/./.", "/a/b/."));
    char sweptByte[3] = {'x', 'x', 'x'};
    Require(sceHttpUriSweepPath(sweptByte, "/a", 1) == 0 && sweptByte[0] == '/' && sweptByte[1] == '\0' && sweptByte[2] == 'x');
    sweptByte[0] = 'x';
    Require(sceHttpUriSweepPath(sweptByte, "", 1) == 0 && sweptByte[0] == '\0');

    {
        SceHttpUriElement source{};
        char parsedPool[256];
        Require(sceHttpUriParse(&source, "http://user:pw@example.com:8080/a/b?q=1#frag", parsedPool, nullptr, sizeof(parsedPool)) == 0);
        SceHttpUriElement copy{};
        char pool[256];
        std::size_t needed = 0;
        Require(sceHttpUriCopy(nullptr, &source, nullptr, 0, &needed) == 0 && needed == 5 + 12 + 5 + 5 + 3);
        Require(sceHttpUriCopy(&copy, &source, pool, sizeof(pool), nullptr) == 0);
        Require(std::strcmp(copy.scheme, "http") == 0 && std::strcmp(copy.hostname, "example.com") == 0 && std::strcmp(copy.path, "/a/b") == 0);
        Require(std::strcmp(copy.username, "user") == 0 && std::strcmp(copy.password, "pw") == 0 && copy.port == 8080);
        Require(copy.query == nullptr && copy.fragment == nullptr && copy.opaque == 0);
        Require(sceHttpUriCopy(&copy, &source, pool, needed - 1, nullptr) == static_cast<int>(0x80431022));
        Require(copy.scheme == nullptr && copy.hostname == nullptr && pool[0] == 0);
        Require(sceHttpUriCopy(&copy, &source, pool, needed, &needed) == 0 && needed == 30);
        Require(sceHttpUriCopy(&copy, nullptr, pool, sizeof(pool), nullptr) == static_cast<int>(0x80433060));
        Require(sceHttpUriCopy(nullptr, &source, nullptr, 0, nullptr) == static_cast<int>(0x804311FE));
        SceHttpUriElement empty{};
        Require(sceHttpUriCopy(&copy, &empty, pool, sizeof(pool), &needed) == 0 && needed == 5 && copy.scheme != nullptr && copy.scheme[0] == 0);
    }
}
