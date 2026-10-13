#include "SceTypes.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <thread>

extern "C" {
int APS5_VABI sceAgcDriverSetTFRing(const volatile void* base, uint32_t size);
int APS5_VABI sceAgcDriverGetTFRing(uintptr_t* base, uint32_t* size);
int APS5_VABI sceAgcDriverSetHsOffchipParam(uint16_t first, uint16_t second);
int APS5_VABI sceAgcDriverGetHsOffchipParam(uint16_t* first, uint16_t* second);
}

namespace {

constexpr uint32_t Canary = 0xa5a5a5a5;

struct RingOutput {
    uintptr_t base = ~uintptr_t{0};
    uint32_t size = 0;
    uint32_t canary = Canary;
};

void Require(bool value) {
    if (!value) std::abort();
}

template<typename TAction>
bool Rejects(TAction action) {
    try { action(); }
    catch (const std::invalid_argument&) { return true; }
    return false;
}

void RequireRing(uintptr_t expectedBase, uint32_t expectedSize) {
    RingOutput output;
    Require(sceAgcDriverGetTFRing(&output.base, &output.size) == 0);
    Require(output.base == expectedBase && output.size == expectedSize && output.canary == Canary);
}

struct OffchipOutput {
    uint16_t first = 0xffff;
    uint16_t second = 0xffff;
    uint32_t canary = Canary;
};

void RequireOffchip(uint16_t expectedFirst, uint16_t expectedSecond) {
    OffchipOutput output;
    Require(sceAgcDriverGetHsOffchipParam(&output.first, &output.second) == 0);
    Require(output.first == expectedFirst && output.second == expectedSecond && output.canary == Canary);
}

}

int main() {
    alignas(256) static uint8_t ring[0x20000];
    const auto ringBase = reinterpret_cast<uintptr_t>(ring);
    RequireRing(0xff0000000, 0x20000);
    Require(sceAgcDriverSetTFRing(ring, 0x1b000) == 0);
    RequireRing(ringBase, 0x1b000);
    std::thread([&] { RequireRing(ringBase, 0x1b000); }).join();
    std::thread([] { Require(sceAgcDriverSetTFRing(ring + 0x1000, 0x2000) == 0); }).join();
    RequireRing(ringBase + 0x1000, 0x2000);
    Require(Rejects([] { sceAgcDriverSetTFRing(nullptr, 0x2000); }));
    Require(Rejects([] { sceAgcDriverSetTFRing(ring, 0); }));
    RequireRing(ringBase + 0x1000, 0x2000);
    RingOutput output;
    Require(Rejects([&] { sceAgcDriverGetTFRing(nullptr, &output.size); }));
    Require(Rejects([&] { sceAgcDriverGetTFRing(&output.base, nullptr); }));

    RequireOffchip(0, 0);
    Require(sceAgcDriverSetHsOffchipParam(0, 255) == 0);
    RequireOffchip(0, 255);
    std::thread([] { RequireOffchip(0, 255); }).join();
    std::thread([] { Require(sceAgcDriverSetHsOffchipParam(1, 0xffff) == 0); }).join();
    RequireOffchip(1, 0xffff);
    OffchipOutput offchip;
    Require(Rejects([&] { sceAgcDriverGetHsOffchipParam(nullptr, &offchip.second); }));
    Require(Rejects([&] { sceAgcDriverGetHsOffchipParam(&offchip.first, nullptr); }));
    Require(offchip.first == 0xffff && offchip.second == 0xffff);
    RequireOffchip(1, 0xffff);
    RequireRing(ringBase + 0x1000, 0x2000);
}
