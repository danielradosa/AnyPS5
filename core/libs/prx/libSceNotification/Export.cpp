#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int NOTIFICATION_ERROR_BAD_MESSAGE = static_cast<int>(0x81980001u);
constexpr std::size_t MessageCapacity = 0x1800;
constexpr std::size_t IdCapacity = 0x20;

std::mutex g_bannerMutex;
bool g_bannerShown = false;

const char* Text(const std::uint8_t* bytes) {
    return reinterpret_cast<const char*>(bytes);
}

std::size_t TerminatedLength(const char* function, const char* text, std::size_t capacity, const char* what) {
    const std::size_t length = ::strnlen(text, capacity);
    if (length == capacity) {
        throw std::runtime_error(std::string(function) + ": " + what + " without a terminator in its first " + std::to_string(capacity) + " bytes");
    }
    return length;
}

}

extern "C" {

int APS5_VABI sceNotificationSend(int type, std::uint8_t flag, std::uint8_t* message) {
    (void)type;
    (void)flag;
    if (message == nullptr || *message == 0) return NOTIFICATION_ERROR_BAD_MESSAGE;
    TerminatedLength(__func__, Text(message), MessageCapacity, "a message");
    WriteLogLine(stderr, "[notification] %s\n", Text(message));
    return 0;
}

int APS5_VABI sceNotificationSendById(int type, std::uint8_t flag, std::uint8_t* id, std::uint8_t* jsonData) {
    (void)type;
    (void)flag;
    if (id == nullptr) APS5_INVALID_ARG_EX;
    TerminatedLength(__func__, Text(id), IdCapacity, "an id");
    if (jsonData == nullptr || *jsonData == 0) return NOTIFICATION_ERROR_BAD_MESSAGE;
    TerminatedLength(__func__, Text(jsonData), MessageCapacity, "a message");
    WriteLogLine(stderr, "[notification] %s: %s\n", Text(id), Text(jsonData));
    return 0;
}

int APS5_VABI sceNotificationShowPsButtonPersistentBanner(std::uint8_t* jsonParam) {
    const char* configuration = jsonParam == nullptr ? "{}" : Text(jsonParam);
    const std::lock_guard lock(g_bannerMutex);
    g_bannerShown = true;
    WriteLogLine(stderr, "[notification] PS button banner shown: %s\n", configuration);
    return 0;
}

void APS5_VABI sceNotificationHidePsButtonPersistentBanner() {
    const std::lock_guard lock(g_bannerMutex);
    if (!g_bannerShown) return;
    g_bannerShown = false;
    WriteLogLine(stderr, "[notification] PS button banner hidden\n");
}

}
