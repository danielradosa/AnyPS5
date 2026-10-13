#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "VulkanTestDevice.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;

alignas(256) constexpr std::array<std::uint32_t, 8> VertexCode{0x7e000208u, 0x7e020280u, 0x7e040280u, 0x7e0602f2u, 0xf80008cfu, 0x03020100u, 0xbf810000u, 0xbf810000u};
alignas(256) constexpr std::array<std::uint32_t, 12> PixelCode{0xf4040100u, 0xfa000000u, 0xbf8cc07fu, 0xf4000182u, 0xfa000000u, 0xbf8cc07fu, 0x7e000206u, 0x7e020280u, 0xf800180fu, 0x00010100u, 0xbf810000u, 0xbf810000u};
alignas(256) std::array<std::byte, 1024> ColorMemory{};
alignas(256) std::array<std::byte, 1024> OtherColorMemory{};
alignas(256) std::array<std::uint32_t, 4> First{std::bit_cast<std::uint32_t>(1.0f)};
alignas(256) std::array<std::uint32_t, 4> Second{std::bit_cast<std::uint32_t>(0.25f)};
alignas(256) std::array<std::uint32_t, 4> Third{std::bit_cast<std::uint32_t>(0.5f)};
alignas(256) std::array<std::uint32_t, 2> Table{};
alignas(256) std::array<std::uint32_t, 2> OtherTable{};

struct Header {
    Shader shader{};
    ShaderUserData userData{};
};

alignas(8) Header VertexHeader{};
alignas(8) Header PixelHeader{};

std::uint32_t Low(const void* pointer) {
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer));
}

std::uint32_t High(const void* pointer) {
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(pointer)) >> 32u);
}

void Register(Header& header, const std::uint32_t* code, std::size_t bytes, std::uint8_t type) {
    header.shader.file_header = 0x34333231u;
    header.shader.version = 0x18u;
    header.shader.header_size = sizeof(Header);
    header.shader.shader_size = static_cast<std::uint32_t>(bytes);
    header.shader.code = code;
    header.shader.type = type;
    header.shader.user_data = &header.userData;
    AgcDriverRegisterShader_nid_postfix(&header.shader);
}

void Append(std::vector<std::uint32_t>& commands, std::uint32_t opcode, std::initializer_list<std::uint32_t> payload) {
    commands.push_back(0xc0000000u | (static_cast<std::uint32_t>(payload.size() - 1) << 16u) | (opcode << 8u));
    commands.insert(commands.end(), payload);
}

void Target(std::vector<std::uint32_t>& commands, const void* color) {
    const auto address = reinterpret_cast<std::uintptr_t>(color);
    Append(commands, 0x69, {0x318, static_cast<std::uint32_t>(address >> 8u)});
    Append(commands, 0x69, {0x390, static_cast<std::uint32_t>(address >> 40u)});
}

void Draw(std::vector<std::uint32_t>& commands, const void* table, std::uint32_t nonce) {
    Append(commands, 0x76, {0x00c, Low(table)});
    Append(commands, 0x76, {0x00d, High(table)});
    Append(commands, 0x76, {0x00e, nonce});
    Append(commands, 0x2d, {3, 2});
}

std::vector<std::uint32_t> Commands(bool warmUp) {
    const auto vertex = reinterpret_cast<std::uintptr_t>(VertexCode.data());
    const auto pixel = reinterpret_cast<std::uintptr_t>(PixelCode.data());
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> context{
        {0x2d5, 0x2000},
        {0x1b6, 0}, {0x207, 0}, {0x200, 0}, {0x203, 0x800},
        {0x2dc, 0xaa00}, {0x2f8, 0}, {0x292, 2}, {0x293, 0},
        {0x80, 0}, {0x8d, 0}, {0x83, 0xffff}, {0x8c, 0xa},
        {0x2f9, 0x2d}, {0x313, 0x6000}, {0x30e, 0xffffffff}, {0x30f, 0xffffffff},
        {0x206, 0x43f}, {0x204, 0x80000}, {0x205, 0x240},
        {0x8e, 0xf}, {0x8f, 0xf}, {0x202, 0xcc0010},
        {0x1c4, 0}, {0x1c5, 9}, {0x1c3, 4}, {0x31c, 0x28028},
        {0x31b, 0}, {0x31d, 0}, {0x3b0, (63u << 14u) | 3u},
        {0x3b8, 0x9000000}, {0x1e0, 0},
        {0xc, 0}, {0xd, 0x40040},
        {0x81, 0x80000000}, {0x82, 0x40040},
        {0x90, 0x80000000}, {0x91, 0x40040},
        {0x94, 0x80000000}, {0x95, 0x40040},
        {0x10f, std::bit_cast<std::uint32_t>(32.0f)}, {0x110, std::bit_cast<std::uint32_t>(32.0f)},
        {0x111, std::bit_cast<std::uint32_t>(-2.0f)}, {0x112, std::bit_cast<std::uint32_t>(2.0f)},
        {0x113, std::bit_cast<std::uint32_t>(1.0f)}, {0x114, 0},
        {0xb4, 0}, {0xb5, std::bit_cast<std::uint32_t>(1.0f)},
    };
    std::vector<std::uint32_t> commands;
    Append(commands, 0x79, {0x242, 4});
    for (const auto& [offset, value] : context) Append(commands, 0x69, {offset, value});
    for (const auto& [offset, value] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{
             {0xc8, static_cast<std::uint32_t>(vertex >> 8u)}, {0xc9, static_cast<std::uint32_t>(vertex >> 40u)}, {0x8b, 1u << 1u}, {0x8c, 0},
             {0x008, static_cast<std::uint32_t>(pixel >> 8u)}, {0x009, static_cast<std::uint32_t>(pixel >> 40u)}, {0x00b, 3u << 1u}}) {
        Append(commands, 0x76, {offset, value});
    }
    Target(commands, ColorMemory.data());
    if (warmUp) {
        Draw(commands, Table.data(), 0);
        return commands;
    }
    Draw(commands, Table.data(), 1);
    Append(commands, 0x37, {0x00100200u, Low(Table.data()), High(Table.data()), Low(Second.data()), High(Second.data())});
    Draw(commands, Table.data(), 2);
    Target(commands, OtherColorMemory.data());
    Draw(commands, OtherTable.data(), 3);
    return commands;
}

void PreparedDrawsHitAndAStaleOneMisses() {
    Register(VertexHeader, VertexCode.data(), sizeof(VertexCode), 2);
    Register(PixelHeader, PixelCode.data(), sizeof(PixelCode), 1);
    Table = {Low(First.data()), High(First.data())};
    OtherTable = {Low(Third.data()), High(Third.data())};
    for (const bool warmUp : {true, false}) {
        auto commands = Commands(warmUp);
        Packet packet{commands.data(), static_cast<std::uint32_t>(commands.size()), 0, {}};
        Require(sceAgcDriverSubmitDcb(&packet) == 0, "the draw submission failed");
        AgcDriverWaitIdle_nid_postfix();
    }
    const auto counters = AgcDriver::DriverDetail::Driver::Get().DrawCacheCounters();
    const bool verify = std::getenv("APS5_VERIFY_DRAW_ENTRIES") != nullptr;
    const auto hits = std::to_string(counters.preparedHits) + " of " + std::to_string(counters.preparedLookups) + " prepared entries hit";
    Require(counters.lookups == 4, "the four draws made " + std::to_string(counters.lookups) + " draw-cache lookups");
    Require(counters.preparedHits >= 2, "the draws after the first did not hit the entries the preparation thread inserted: " + hits);
    Require(!verify || counters.verifyMismatches == 0, std::to_string(counters.verifyMismatches) + " prepared entries differed from a capture at the draw (" + hits + ")");
    Require(Table[0] == Low(Second.data()) && Table[1] == High(Second.data()), "the table write did not land");
}

}

int main() {
    try {
        {
            const auto device = OpenVulkanTestDevice();
            if (!device) return VulkanTestSkipped;
        }
        PreparedDrawsHitAndAStaleOneMisses();
        AgcDriverShutdown_nid_postfix();
        std::cout << "prepared draws hit the draw cache and the draw behind a table write did not reuse a stale preparation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        AgcDriverShutdown_nid_postfix();
        return 1;
    }
}
