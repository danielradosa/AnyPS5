#include "prx/libSceAgc/Command/include/Packet.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

extern "C" {
std::uint32_t* APS5_VABI sceAgcDcbAtomicGds_0900(CommandBuffer*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint16_t, std::uint16_t, std::uint32_t, std::uint32_t, std::uint64_t, std::uint64_t);
std::uint32_t* APS5_VABI sceAgcDcbAtomicGds(CommandBuffer*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint16_t, std::uint16_t, std::uint32_t, std::uint32_t, std::uint64_t, std::uint64_t);
std::uint32_t APS5_VABI sceAgcDcbAtomicGdsGetSize();
std::uint32_t APS5_VABI sceAgcAcbAtomicGdsGetSize();
}

namespace {

constexpr std::uint32_t Sentinel = 0xabcdef01u;

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename TAction>
void expectFailure(TAction action) {
    try {
        action();
    } catch (const std::runtime_error&) {
        return;
    }
    throw std::runtime_error("expected invalid input to fail");
}

struct Storage {
    std::array<std::uint32_t, 32> words{};
    CommandBuffer buffer{words.data(), words.data() + words.size(), words.data(), words.data() + words.size(), nullptr, nullptr, 0};

    Storage() {
        words.fill(Sentinel);
    }

    void limitTo(std::uint32_t count) {
        buffer.cursor_down = buffer.cursor_up + count;
    }
};

std::uint32_t* write(CommandBuffer* buffer, std::uint32_t a1, std::uint32_t a2, std::uint32_t a3, std::uint32_t a4, std::uint32_t a5) {
    return sceAgcDcbAtomicGds_0900(buffer, a1, a2, a3, a4, a5, 0, 0, 0, 0, 0, 0);
}

void testConsoleControlWords() {
    struct Vector {
        std::uint64_t a1, a2, a3, a4, a5;
        std::uint32_t word1, word2;
    };
    const std::array<Vector, 3> vectors{{
        {0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u, 0x55555555u, 0x40050022u, 0x15u},
        {1, 2, 3, 4, 5, 0x40000002u, 0x5u},
        {0x1111111122222222ull, 0x3333333344444444ull, 0, 0, 0, 0x80100044u, 0},
    }};
    for (const auto& vector : vectors) {
        Storage storage;
        auto* packet = write(&storage.buffer, static_cast<std::uint32_t>(vector.a1), static_cast<std::uint32_t>(vector.a2), static_cast<std::uint32_t>(vector.a3), static_cast<std::uint32_t>(vector.a4), static_cast<std::uint32_t>(vector.a5));
        check(packet == storage.words.data(), "packet does not start at the cursor");
        check(packet[0] == 0xc0091d00u, "ATOMIC_GDS header mismatch");
        check(packet[1] == vector.word1 && packet[2] == vector.word2, "control words differ from the console's");
        check(storage.buffer.cursor_up == packet + 11, "packet is not 11 dwords");
        check(std::all_of(storage.words.begin() + 11, storage.words.end(), [](std::uint32_t word) { return word == Sentinel; }), "packet overwrote following words");
    }
}

void testStackArguments() {
    Storage storage;
    auto* packet = sceAgcDcbAtomicGds_0900(&storage.buffer, 1, 0x01, 0, 0, 0, 0x1234, 0x5678, 0xabcdef12u, 0x9abcdef0u, 0x0102030405060708ull, 0x1112131415161718ull);
    const std::array<std::uint32_t, 11> expected{0xc0091d00u, 0x40000001u, 0, 0x1234u, 0x5678u, 0x00cd0012u, 0x9abcdef0u, 0x05060708u, 0x01020304u, 0x15161718u, 0x11121314u};
    check(std::equal(expected.begin(), expected.end(), packet), "stack arguments are not placed in dwords 3 to 10");
}

std::array<std::uint32_t, 11> packetFor(std::uint32_t a1, std::uint32_t a2, std::uint32_t a3, std::uint32_t a4, std::uint32_t a5) {
    Storage storage;
    const auto* packet = write(&storage.buffer, a1, a2, a3, a4, a5);
    std::array<std::uint32_t, 11> words{};
    std::copy(packet, packet + words.size(), words.begin());
    return words;
}

void testControlFields() {
    check(packetFor(0, 0x01, 1, 0, 0)[1] == 0x00020001u, "mode 1 with a non-returning operation does not wait for completion");
    check(packetFor(0, 0x21, 1, 0, 0)[1] == 0x00040021u, "a returning operation does not read or still waits for completion");
    check(packetFor(0, 0x01, 2, 0, 0)[1] == 0x00000001u, "a mode other than 1 waits for completion");
    const auto wide = packetFor(0, 0x41, 0, 1, 0x7f);
    check(wide[1] == 0x00100041u && wide[2] == 0x13fu, "64-bit read control, data mode or auto-increment mismatch");
    check(packetFor(3, 0x200, 0, 0, 0)[1] == 0xc0010000u, "engine or compare-swap bit mismatch");
}

void testUnsuffixedForm() {
    Storage versioned;
    Storage unsuffixed;
    const auto* expected = sceAgcDcbAtomicGds_0900(&versioned.buffer, 0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u, 0x55555555u, 0x1234, 0x5678, 0xabcdef12u, 0x9abcdef0u, 0x0102030405060708ull, 0x1112131415161718ull);
    auto* packet = sceAgcDcbAtomicGds(&unsuffixed.buffer, 0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u, 0x55555555u, 0x1234, 0x5678, 0xabcdef12u, 0x9abcdef0u, 0x0102030405060708ull, 0x1112131415161718ull);
    check(packet == unsuffixed.words.data() && unsuffixed.buffer.cursor_up == packet + 11, "unsuffixed builder does not write 11 dwords at the cursor");
    check(packet[1] == 0x40050022u && packet[2] == 0x15u, "unsuffixed builder control words differ from the console's");
    check(std::equal(expected, expected + 11, packet), "unsuffixed and _0900 builders write different packets");
}

void testSizeAndBounds() {
    check(sceAgcDcbAtomicGdsGetSize() == 44, "DCB ATOMIC_GDS size is not 44 bytes");
    check(sceAgcAcbAtomicGdsGetSize() == 44, "ACB ATOMIC_GDS size is not 44 bytes");

    Storage exact;
    exact.limitTo(11);
    check(write(&exact.buffer, 1, 2, 3, 4, 5) == exact.words.data() && exact.buffer.cursor_up == exact.buffer.cursor_down, "packet does not fill the queried size");

    Storage shortBuffer;
    shortBuffer.limitTo(10);
    const auto before = shortBuffer.words;
    expectFailure([&] { write(&shortBuffer.buffer, 1, 2, 3, 4, 5); });
    check(shortBuffer.words == before && shortBuffer.buffer.cursor_up == shortBuffer.words.data(), "failed write modified the buffer");

    expectFailure([] { write(nullptr, 1, 2, 3, 4, 5); });
}

}

int main() {
    try {
        testConsoleControlWords();
        testStackArguments();
        testControlFields();
        testUnsuffixedForm();
        testSizeAndBounds();
        std::puts("AGC atomic GDS tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
