#include "VulkanTestDevice.hpp"
#include "SceTypes.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Submit/include/Acb.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" std::uint32_t* APS5_VABI sceAgcCbMemsetExclusive(CommandBuffer* buf, std::uint64_t destination, const void* pattern, std::uint64_t sizeInBytes);

namespace {

constexpr std::uint32_t Blocks = 100;
constexpr std::uint32_t Untouched = 0xa5a5a5a5u;
constexpr std::uint32_t Unwritten = 0xcdcdcdcdu;

alignas(256) std::array<std::uint32_t, 512> Destination{};
alignas(16) constexpr std::array<std::uint32_t, 4> Pattern{0x11223344u, 0x55667788u, 0x99aabbccu, 0xddeeff00u};

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint64_t Address(const void* pointer) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(pointer));
}

CommandBuffer Buffer(std::vector<std::uint32_t>& storage) {
    return {storage.data(), storage.data() + storage.size(), storage.data(), storage.data() + storage.size(), nullptr, nullptr, 0};
}

bool Throws(CommandBuffer& buffer, std::uint64_t destination, const void* pattern, std::uint64_t sizeInBytes) {
    auto* before = buffer.cursor_up;
    try {
        sceAgcCbMemsetExclusive(&buffer, destination, pattern, sizeInBytes);
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find("sceAgcCbMemsetExclusive") != std::string::npos && buffer.cursor_up == before;
    }
    return false;
}

void CheckPackets(const std::vector<std::uint32_t>& words, std::uint64_t destination, std::uint32_t blocks, std::uint32_t groups) {
    Require(words[0] == 0xc0027600u && words[1] == 0x20cu, "the first packet does not set COMPUTE_PGM_LO and COMPUTE_PGM_HI");
    const auto kernel = (static_cast<std::uint64_t>(words[2]) << 8u) | (static_cast<std::uint64_t>(words[3] & 0xffu) << 40u);
    Require(kernel != 0 && words[3] <= 0xffu, "the memset kernel address is not a 48-bit GPU address");
    const auto* code = reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(kernel));
    Require(code[0] == 0xd7460004u && code[6] == 0xe01c2000u && code[8] == 0xbf810000u, "COMPUTE_PGM does not point at the memset kernel");
    Require(words[4] == 0xc0027600u && words[5] == 0x212u && words[6] == 1u && words[7] == 0x90u, "COMPUTE_PGM_RSRC1 and RSRC2 are wrong");
    Require(words[8] == 0xc0017600u && words[9] == 0x228u && words[10] == 0, "COMPUTE_PGM_RSRC3 is wrong");
    Require(words[11] == 0xc0037600u && words[12] == 0x207u && words[13] == 64 && words[14] == 1 && words[15] == 1, "COMPUTE_NUM_THREAD is wrong");
    Require(words[16] == 0xc0097600u && words[17] == 0x240u, "the user data packet does not start at COMPUTE_USER_DATA_0");
    Require(words[18] == static_cast<std::uint32_t>(destination) && words[19] == ((static_cast<std::uint32_t>(destination >> 32u) & 0xffffu) | (16u << 16u)), "the buffer descriptor does not hold the destination and a 16-byte stride");
    Require(words[20] == blocks && words[21] == 0x4bfacu, "the buffer descriptor does not hold the block count and format");
    for (std::uint32_t i = 0; i < 4; ++i) Require(words[22 + i] == Pattern[i], "the user data does not hold the pattern");
    Require(words[26] == 0, "COMPUTE_USER_DATA_8 is not 0");
    Require(words[27] == 0xc0031500u && words[28] == groups && words[29] == 1 && words[30] == 1 && words[31] == 0x41u, "the dispatch is wrong");
}

void CheckBuilder() {
    const auto destination = Address(Destination.data());
    std::vector<std::uint32_t> storage(40, Unwritten);
    auto buffer = Buffer(storage);
    Require(sceAgcCbMemsetExclusive(&buffer, destination, Pattern.data(), Blocks * 16u) == storage.data(), "the memset does not return its first packet");
    Require(buffer.cursor_up == storage.data() + 32, "the memset does not write 32 dwords");
    CheckPackets(storage, destination, Blocks, 2);
    for (std::size_t i = 32; i < storage.size(); ++i) Require(storage[i] == Unwritten, "the memset writes past its 32 dwords");

    std::vector<std::uint32_t> empty(32, Unwritten);
    auto emptyBuffer = Buffer(empty);
    Require(sceAgcCbMemsetExclusive(&emptyBuffer, destination + 4096u, Pattern.data(), 0) == empty.data(), "an empty memset does not return its first packet");
    CheckPackets(empty, destination + 4096u, 0, 0);
    std::vector<std::uint32_t> exact(32, Unwritten);
    auto exactBuffer = Buffer(exact);
    sceAgcCbMemsetExclusive(&exactBuffer, destination, Pattern.data(), 128u * 16u);
    CheckPackets(exact, destination, 128, 2);

    std::vector<std::uint32_t> refused(64, Unwritten);
    auto refusedBuffer = Buffer(refused);
    Require(Throws(refusedBuffer, destination, nullptr, 16), "a null pattern is accepted");
    Require(Throws(refusedBuffer, 0, Pattern.data(), 16), "a null destination is accepted");
    Require(Throws(refusedBuffer, destination + 4u, Pattern.data(), 16), "a misaligned destination is accepted");
    Require(Throws(refusedBuffer, 1ull << 48u, Pattern.data(), 16), "a destination above 48 bits is accepted");
    Require(Throws(refusedBuffer, destination, Pattern.data(), 24), "a size that is not a multiple of 16 is accepted");
    Require(Throws(refusedBuffer, destination, Pattern.data(), (1ull << 36u) + 16u), "more than 2^32 - 1 blocks are accepted");
    std::vector<std::uint32_t> tight(31, Unwritten);
    auto tightBuffer = Buffer(tight);
    Require(Throws(tightBuffer, destination, Pattern.data(), 16), "a buffer with 31 free dwords is accepted");
    for (const auto word : tight) Require(word == Unwritten, "a refused memset writes to the buffer");
}

void Fill(bool compute) {
    Destination.fill(Untouched);
    std::vector<std::uint32_t> words(32);
    auto buffer = Buffer(words);
    sceAgcCbMemsetExclusive(&buffer, Address(Destination.data()), Pattern.data(), Blocks * 16u);
    Packet packet{words.data(), static_cast<std::uint32_t>(words.size()), 0, {}};
    Require((compute ? sceAgcDriverSubmitAcb(0x20, &packet) : sceAgcDriverSubmitDcb(&packet)) == 0, "the memset submission failed");
    AgcDriverWaitIdle_nid_postfix();
    for (std::uint32_t i = 0; i < Blocks * 4u; ++i) Require(Destination[i] == Pattern[i % 4u], "dword " + std::to_string(i) + " was not filled with the pattern");
    for (std::size_t i = Blocks * 4u; i < Destination.size(); ++i) Require(Destination[i] == Untouched, "dword " + std::to_string(i) + " past the size was written");
}

}

int main() {
    try {
        CheckBuilder();
        auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        device.reset();
        Fill(true);
        Fill(false);
        std::cout << "memset exclusive tests passed\n";
        AgcDriverShutdown_nid_postfix();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        try {
            AgcDriverShutdown_nid_postfix();
        } catch (const std::exception& shutdownError) {
            std::cerr << shutdownError.what() << '\n';
        }
        return 1;
    }
}
