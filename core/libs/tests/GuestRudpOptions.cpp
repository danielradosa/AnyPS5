#include "SceTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
using GuestRudpEventHandler = void(APS5_VABI*)(int, int, int, void*);
int APS5_VABI sceRudpInit_nid_postfix(void*, int);
int APS5_VABI sceRudpEnd();
int APS5_VABI sceRudpCreateContext(GuestRudpEventHandler, void*, int*);
int APS5_VABI sceRudpTerminate(int);
int APS5_VABI sceRudpSetOption(int, int, const void*, std::size_t);
int APS5_VABI sceRudpGetOption(int, int, void*, std::size_t);
int APS5_VABI sceRudpInitiate(int, const void*, std::uint32_t, std::uint16_t);
int APS5_VABI sceRudpWrite(int, const void*, std::uint32_t, int);
int APS5_VABI sceRudpGetRemoteInfo(int, void*, std::uint32_t*, std::uint16_t*);
int APS5_VABI sceRudpNetReceived(int, const std::uint8_t*, std::size_t, const void*, std::uint32_t);
int APS5_VABI sceRudpGetStatus(void*, std::size_t);
int APS5_VABI sceRudpEnableInternalIOThread2(std::uint32_t, std::uint32_t, std::uint64_t);
}

static void Require(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "rudp options check failed at line %d\n", line);
        std::abort();
    }
}
#define CHECK(value) Require((value), __LINE__)

static void APS5_VABI Handler(int, int, int, void*) {}

static std::int32_t Option(int context, int option) {
    std::int32_t value = 0x5a5a5a5a;
    CHECK(sceRudpGetOption(context, option, &value, sizeof(value)) == 0);
    return value;
}

int main() {
    constexpr int notInitialized = static_cast<int>(0x80770001u);
    constexpr int invalidContext = static_cast<int>(0x80770003u);
    constexpr int invalidArgument = static_cast<int>(0x80770004u);
    constexpr int invalidOption = static_cast<int>(0x80770005u);
    constexpr int connReset = static_cast<int>(0x80770009u);
    constexpr int connRefused = static_cast<int>(0x8077000Au);
    constexpr int notAcceptable = static_cast<int>(0x80770011u);
    constexpr int invalidSocket = static_cast<int>(0x80770019u);
    constexpr int maxPayload = 1;
    constexpr int sendBuffer = 2;
    constexpr int deliveryCritical = 5;
    constexpr int nonblock = 7;
    constexpr int connectionTimeout = 9;
    constexpr int lastError = 14;
    constexpr int keepAliveTimeout = 19;
    constexpr std::size_t statusSize = 0xF8;
    alignas(16) static std::array<unsigned char, 0x10000> pool{};

    const std::uint8_t datagram[24] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
    const std::uint8_t from[16] = {16, 2, 0x0e, 0x4a, 127, 0, 0, 1};
    std::int32_t value = 0;
    CHECK(sceRudpEnableInternalIOThread2(0x4000, 700, 1) == notInitialized);
    CHECK(sceRudpGetOption(0, maxPayload, &value, sizeof(value)) == notInitialized);
    CHECK(sceRudpNetReceived(3, datagram, sizeof(datagram), from, sizeof(from)) == notInitialized);
    std::uint32_t addressLength = 16;
    std::uint16_t vport = 0x5a5a;
    std::uint8_t address[16] = {};
    CHECK(sceRudpGetRemoteInfo(0, address, &addressLength, &vport) == notInitialized);

    CHECK(sceRudpInit_nid_postfix(pool.data(), static_cast<int>(pool.size())) == 0);
    CHECK(sceRudpEnableInternalIOThread2(0x4000, 700, 1) == 0);

    int context = -1;
    CHECK(sceRudpCreateContext(Handler, nullptr, &context) == 0);
    CHECK(Option(context, maxPayload) == 1346);
    CHECK(Option(context, sendBuffer) == 65536);
    CHECK(Option(context, deliveryCritical) == 1);
    CHECK(Option(context, nonblock) == 1);
    CHECK(Option(context, connectionTimeout) == 60000);
    CHECK(Option(context, lastError) == 0);
    CHECK(Option(context, keepAliveTimeout) == 0);

    CHECK(sceRudpGetOption(context + 100, maxPayload, &value, sizeof(value)) == invalidContext);
    CHECK(sceRudpGetOption(context, 0, &value, sizeof(value)) == invalidOption);
    CHECK(sceRudpGetOption(context, 12, &value, sizeof(value)) == invalidOption);
    CHECK(sceRudpGetOption(context, 20, &value, sizeof(value)) == invalidOption);
    CHECK(sceRudpGetOption(context, maxPayload, nullptr, sizeof(value)) == invalidArgument);
    CHECK(sceRudpGetOption(context, maxPayload, &value, 2) == invalidArgument);
    CHECK(sceRudpGetOption(context, maxPayload, &value, 8) == invalidArgument);

    const std::int32_t payload = 1000;
    CHECK(sceRudpSetOption(context, maxPayload, &payload, sizeof(payload)) == 0);
    CHECK(Option(context, maxPayload) == 1000);
    const std::int32_t blocking = 0;
    CHECK(sceRudpSetOption(context, nonblock, &blocking, sizeof(blocking)) == 0);
    CHECK(Option(context, nonblock) == 0);
    CHECK(sceRudpSetOption(context + 100, nonblock, &blocking, sizeof(blocking)) == invalidContext);
    CHECK(sceRudpSetOption(context, 13, &blocking, sizeof(blocking)) == invalidOption);
    CHECK(Option(context, lastError) == invalidOption);
    CHECK(sceRudpSetOption(context, nonblock, nullptr, sizeof(blocking)) == invalidArgument);
    CHECK(Option(context, lastError) == invalidArgument);
    CHECK(sceRudpSetOption(context, nonblock, &blocking, 1) == invalidArgument);
    CHECK(Option(context, nonblock) == 0);

    const std::uint8_t peer[16] = {16, 2, 0x0e, 0x4a, 127, 0, 0, 1};
    CHECK(sceRudpInitiate(context, peer, sizeof(peer), 1) == connRefused);
    CHECK(Option(context, lastError) == connRefused);
    CHECK(sceRudpWrite(context, datagram, sizeof(datagram), 0) == connReset);
    CHECK(Option(context, lastError) == connReset);

    CHECK(sceRudpGetRemoteInfo(-1, address, &addressLength, &vport) == invalidArgument);
    CHECK(sceRudpGetRemoteInfo(context, nullptr, &addressLength, &vport) == invalidArgument);
    CHECK(sceRudpGetRemoteInfo(context, address, nullptr, &vport) == invalidArgument);
    CHECK(sceRudpGetRemoteInfo(context, address, &addressLength, nullptr) == invalidArgument);
    CHECK(sceRudpGetRemoteInfo(context + 100, address, &addressLength, &vport) == invalidContext);
    CHECK(sceRudpGetRemoteInfo(context, address, &addressLength, &vport) == notAcceptable);
    CHECK(addressLength == 16 && vport == 0x5a5a);

    std::array<unsigned char, statusSize> status{};
    CHECK(sceRudpGetStatus(status.data(), status.size()) == 0);
    std::uint64_t receivedBytes = 1;
    std::uint32_t receivedPackets = 1;
    std::memcpy(&receivedBytes, status.data() + 0x08, sizeof(receivedBytes));
    std::memcpy(&receivedPackets, status.data() + 0x14, sizeof(receivedPackets));
    CHECK(receivedBytes == 0 && receivedPackets == 0);

    CHECK(sceRudpNetReceived(-1, datagram, sizeof(datagram), from, sizeof(from)) == invalidSocket);
    CHECK(sceRudpNetReceived(3, nullptr, sizeof(datagram), from, sizeof(from)) == invalidArgument);
    CHECK(sceRudpNetReceived(3, datagram, 0, from, sizeof(from)) == invalidArgument);
    CHECK(sceRudpNetReceived(3, datagram, sizeof(datagram), nullptr, sizeof(from)) == invalidArgument);
    CHECK(sceRudpNetReceived(3, datagram, sizeof(datagram), from, 8) == invalidArgument);
    CHECK(sceRudpNetReceived(3, datagram, sizeof(datagram), from, sizeof(from)) == 0);
    CHECK(sceRudpNetReceived(3, datagram, 10, from, sizeof(from)) == 0);
    CHECK(sceRudpGetStatus(status.data(), status.size()) == 0);
    std::memcpy(&receivedBytes, status.data() + 0x08, sizeof(receivedBytes));
    std::memcpy(&receivedPackets, status.data() + 0x14, sizeof(receivedPackets));
    CHECK(receivedBytes == sizeof(datagram) + 10 && receivedPackets == 2);

    CHECK(sceRudpTerminate(context) == 0);
    CHECK(sceRudpGetOption(context, maxPayload, &value, sizeof(value)) == invalidContext);
    CHECK(sceRudpCreateContext(Handler, nullptr, &context) == 0);
    CHECK(Option(context, maxPayload) == 1346);
    CHECK(Option(context, lastError) == 0);
    CHECK(sceRudpEnd() == 0);

    CHECK(sceRudpInit_nid_postfix(pool.data(), static_cast<int>(pool.size())) == 0);
    CHECK(sceRudpGetStatus(status.data(), status.size()) == 0);
    std::memcpy(&receivedBytes, status.data() + 0x08, sizeof(receivedBytes));
    std::memcpy(&receivedPackets, status.data() + 0x14, sizeof(receivedPackets));
    CHECK(receivedBytes == 0 && receivedPackets == 0);
    CHECK(sceRudpEnd() == 0);
    return 0;
}
