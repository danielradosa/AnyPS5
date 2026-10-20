#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI sceRudpInit_nid_postfix(void*, int);
int APS5_VABI sceRudpEnd();
int APS5_VABI sceRudpProcessEvents();
int APS5_VABI sceRudpNetFlush();
int APS5_VABI sceRudpGetMaxSegmentSize(std::uint16_t*);
int APS5_VABI sceRudpSetMaxSegmentSize(std::uint16_t);
int APS5_VABI sceRudpCreateContext(void (APS5_VABI*)(int, int, int, void*), void*, int*);
int APS5_VABI sceRudpTerminate(int);
int APS5_VABI sceRudpNetReceived(int, const std::uint8_t*, std::size_t, const void*, std::uint32_t);
}

static void Require(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "rudp check failed at line %d\n", line);
        std::abort();
    }
}
#define CHECK(value) Require((value), __LINE__)

int main() {
    constexpr int notInitialized = static_cast<int>(0x80770001);
    constexpr int invalidArgument = static_cast<int>(0x80770004);
    std::uint16_t segmentSize = 0x5a5a;
    CHECK(sceRudpProcessEvents() == notInitialized);
    CHECK(sceRudpNetFlush() == notInitialized);
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == notInitialized);
    CHECK(segmentSize == 0x5a5a);
    CHECK(sceRudpGetMaxSegmentSize(nullptr) == notInitialized);

    alignas(8) static unsigned char pool[0xC58];
    CHECK(sceRudpInit_nid_postfix(pool, sizeof(pool)) == 0);
    CHECK(sceRudpProcessEvents() == 0);
    CHECK(sceRudpNetFlush() == 0);
    CHECK(sceRudpGetMaxSegmentSize(nullptr) == invalidArgument);
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == 0);
    CHECK(segmentSize == 1410);
    std::uint16_t segment[3] = {0, 0, 0x5a5a};
    CHECK(sceRudpGetMaxSegmentSize(&segment[1]) == 0);
    CHECK(segment[0] == 0 && segment[1] == 1410 && segment[2] == 0x5a5a);
    CHECK(sceRudpEnd() == 0);

    segmentSize = 0x5a5a;
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == notInitialized);
    CHECK(segmentSize == 0x5a5a);

    const std::uint8_t datagram[4] = {1, 2, 3, 4};
    const std::uint8_t from[16] = {16, 2, 0x0e, 0x4a, 127, 0, 0, 1};
    CHECK(sceRudpNetReceived(3, datagram, sizeof(datagram), from, sizeof(from)) == notInitialized);

    constexpr int notAcceptable = static_cast<int>(0x80770011);
    CHECK(sceRudpSetMaxSegmentSize(1200) == notInitialized);
    CHECK(sceRudpInit_nid_postfix(pool, sizeof(pool)) == 0);
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == 0 && segmentSize == 1410);
    CHECK(sceRudpSetMaxSegmentSize(1200) == 0);
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == 0 && segmentSize == 1200);
    int context = -1;
    CHECK(sceRudpCreateContext(nullptr, nullptr, &context) == 0);
    CHECK(sceRudpSetMaxSegmentSize(1000) == notAcceptable);
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == 0 && segmentSize == 1200);
    CHECK(sceRudpTerminate(context) == 0);
    CHECK(sceRudpSetMaxSegmentSize(1300) == 0);
    CHECK(sceRudpEnd() == 0);
    CHECK(sceRudpInit_nid_postfix(pool, sizeof(pool)) == 0);
    CHECK(sceRudpGetMaxSegmentSize(&segmentSize) == 0 && segmentSize == 1300);
    CHECK(sceRudpSetMaxSegmentSize(1410) == 0);
    CHECK(sceRudpEnd() == 0);
    return 0;
}
