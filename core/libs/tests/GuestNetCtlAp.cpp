#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" {
int APS5_VABI sceNetCtlApInit(void);
void APS5_VABI sceNetCtlApTerm(void);
int APS5_VABI sceNetCtlApCheckCallback(void);
int APS5_VABI sceNetCtlApClearEvent(int eventType);
int APS5_VABI sceNetCtlApRegisterCallback(void (APS5_VABI* func)(int, void*), void* arg, int* cid);
int APS5_VABI sceNetCtlApUnregisterCallback(int cid);
int APS5_VABI sceNetCtlApGetResult(int eventType, int* errorCode);
int APS5_VABI sceNetCtlApGetInfo(void* info);
int APS5_VABI sceNetCtlApGetState(int* state);
int APS5_VABI sceNetCtlApStop(void);
int APS5_VABI sceNetCtlApGetConnectInfo(const char* optStr, void* connectInfo);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr int kErrNotStarted = static_cast<int>(0x8041298F);
constexpr int kErrInvalidType = static_cast<int>(0x80412990);
constexpr int kErrInvalidAddr = static_cast<int>(0x80412991);
constexpr int kErrNotInitialized = static_cast<int>(0x80412994);
constexpr int kErrCallbackMax = static_cast<int>(0x80412995);
constexpr int kErrInvalidId = static_cast<int>(0x80412996);
constexpr int kErrIdNotFound = static_cast<int>(0x80412997);
constexpr int kErrInvalidSize = static_cast<int>(0x80412998);
constexpr int kErrInvalidOptStr = static_cast<int>(0x80412901);

int g_events = 0;

void APS5_VABI CountEvent(int, void*) { ++g_events; }

template <std::size_t TSize>
void SetSize(std::uint8_t (&block)[TSize], std::uint64_t size) {
    std::memset(block, 0x5a, sizeof(block));
    std::memcpy(block, &size, sizeof(size));
}

template <std::size_t TSize>
bool Untouched(const std::uint8_t (&block)[TSize], std::uint64_t size) {
    std::uint64_t stored = 0;
    std::memcpy(&stored, block, sizeof(stored));
    if (stored != size) return false;
    for (std::size_t i = sizeof(stored); i < TSize; ++i) {
        if (block[i] != 0x5a) return false;
    }
    return true;
}

}

int main() {
    int state = -1;
    int errorCode = -1;
    int cid = -1;
    std::uint8_t info[152];
    std::uint8_t connectInfo[184];
    SetSize(info, sizeof(info));
    SetSize(connectInfo, sizeof(connectInfo));

    Require(sceNetCtlApCheckCallback() == kErrNotInitialized);
    Require(sceNetCtlApClearEvent(1) == kErrNotInitialized);
    Require(sceNetCtlApRegisterCallback(CountEvent, nullptr, &cid) == kErrNotInitialized && cid == -1);
    Require(sceNetCtlApUnregisterCallback(0) == kErrNotInitialized);
    Require(sceNetCtlApGetResult(1, &errorCode) == kErrNotInitialized && errorCode == -1);
    Require(sceNetCtlApGetInfo(info) == kErrNotInitialized);
    Require(sceNetCtlApGetState(&state) == kErrNotInitialized && state == -1);
    Require(sceNetCtlApStop() == kErrNotInitialized);
    Require(sceNetCtlApGetConnectInfo("", connectInfo) == kErrNotInitialized);

    Require(sceNetCtlApInit() == 0);
    Require(sceNetCtlApInit() == 0);

    Require(sceNetCtlApGetState(nullptr) == kErrInvalidAddr);
    Require(sceNetCtlApGetState(&state) == 0 && state == 0);

    Require(sceNetCtlApRegisterCallback(nullptr, nullptr, &cid) == kErrInvalidAddr && cid == -1);
    Require(sceNetCtlApRegisterCallback(CountEvent, nullptr, nullptr) == kErrInvalidAddr);
    int cids[8] = {};
    for (int i = 0; i < 8; ++i) {
        Require(sceNetCtlApRegisterCallback(CountEvent, &cids[i], &cids[i]) == 0 && cids[i] == i);
    }
    Require(sceNetCtlApRegisterCallback(CountEvent, nullptr, &cid) == kErrCallbackMax && cid == -1);
    Require(sceNetCtlApUnregisterCallback(-1) == kErrInvalidId);
    Require(sceNetCtlApUnregisterCallback(8) == kErrInvalidId);
    Require(sceNetCtlApUnregisterCallback(5) == 0);
    Require(sceNetCtlApUnregisterCallback(5) == kErrIdNotFound);
    Require(sceNetCtlApRegisterCallback(CountEvent, nullptr, &cid) == 0 && cid == 5);
    Require(sceNetCtlApCheckCallback() == 0);
    Require(g_events == 0);

    for (const int type : {0, 5, -1}) {
        Require(sceNetCtlApClearEvent(type) == kErrInvalidType);
        Require(sceNetCtlApGetResult(type, &errorCode) == kErrInvalidType && errorCode == -1);
    }
    Require(sceNetCtlApGetResult(1, nullptr) == kErrInvalidAddr);
    for (int type = 1; type <= 4; ++type) {
        Require(sceNetCtlApClearEvent(type) == 0);
        errorCode = -1;
        Require(sceNetCtlApGetResult(type, &errorCode) == 0 && errorCode == 0);
    }

    Require(sceNetCtlApGetInfo(nullptr) == kErrInvalidAddr);
    SetSize(info, 151);
    Require(sceNetCtlApGetInfo(info) == kErrInvalidSize && Untouched(info, 151));
    SetSize(info, sizeof(info));
    Require(sceNetCtlApGetInfo(info) == kErrNotStarted && Untouched(info, sizeof(info)));

    Require(sceNetCtlApGetConnectInfo("", nullptr) == kErrInvalidAddr);
    SetSize(connectInfo, 152);
    Require(sceNetCtlApGetConnectInfo("", connectInfo) == kErrInvalidSize && Untouched(connectInfo, 152));
    SetSize(connectInfo, sizeof(connectInfo));
    Require(sceNetCtlApGetConnectInfo("123456789", connectInfo) == kErrInvalidOptStr);
    Require(sceNetCtlApGetConnectInfo("12345678", connectInfo) == kErrNotStarted);
    Require(sceNetCtlApGetConnectInfo(nullptr, connectInfo) == kErrNotStarted && Untouched(connectInfo, sizeof(connectInfo)));

    Require(sceNetCtlApStop() == kErrNotStarted);
    Require(sceNetCtlApCheckCallback() == 0);
    Require(g_events == 0);
    Require(sceNetCtlApGetState(&state) == 0 && state == 0);

    sceNetCtlApTerm();
    Require(sceNetCtlApCheckCallback() == kErrNotInitialized);
    Require(sceNetCtlApInit() == 0);
    for (int i = 0; i < 8; ++i) {
        Require(sceNetCtlApUnregisterCallback(i) == kErrIdNotFound);
    }
    Require(sceNetCtlApRegisterCallback(CountEvent, nullptr, &cid) == 0 && cid == 0);
    Require(sceNetCtlApUnregisterCallback(0) == 0);
    sceNetCtlApTerm();
    sceNetCtlApTerm();
    Require(sceNetCtlApStop() == kErrNotInitialized);
}
