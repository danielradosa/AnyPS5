#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
int APS5_VABI sceCompanionUtilGetEvent(void*);
int APS5_VABI sceCompanionUtilInitialize(const void*);
int APS5_VABI sceCompanionUtilTerminate();
}

namespace {

constexpr int InvalidArgument = static_cast<int>(0x80AD0004u);
constexpr int InvalidPointer = static_cast<int>(0x80AD0006u);
constexpr int NoEvent = static_cast<int>(0x80AD0008u);

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

}

int main() {
    std::uint8_t event[0x104];
    std::memset(event, 0x5a, sizeof(event));
    Require(sceCompanionUtilGetEvent(nullptr) == InvalidArgument);
    Require(sceCompanionUtilGetEvent(event) == InvalidPointer);
    Require(Throws([] { sceCompanionUtilTerminate(); }));

    const std::uint8_t option[64] = {};
    Require(sceCompanionUtilInitialize(option) == 0);
    Require(Throws([&] { sceCompanionUtilInitialize(option); }));
    Require(sceCompanionUtilGetEvent(nullptr) == InvalidArgument);
    Require(sceCompanionUtilGetEvent(event) == NoEvent);
    for (const auto byte : event) Require(byte == 0x5a);
    Require(sceCompanionUtilTerminate() == 0);

    Require(sceCompanionUtilGetEvent(event) == InvalidPointer);
    Require(sceCompanionUtilInitialize(nullptr) == 0);
    Require(sceCompanionUtilGetEvent(event) == NoEvent);
    Require(sceCompanionUtilTerminate() == 0);
    return 0;
}
