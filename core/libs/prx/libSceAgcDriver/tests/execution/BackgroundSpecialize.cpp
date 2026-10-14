#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "prx/libc/include/GuestWriteWatch.hpp"
#include "VulkanTestDevice.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;
using AgcDriver::DriverDetail::SpecializeCount;
using AgcDriver::DriverDetail::SpecializeDropped;
using AgcDriver::DriverDetail::SpecializeFailed;
using AgcDriver::DriverDetail::SpecializeNotSpecialized;
using AgcDriver::DriverDetail::SpecializeParked;
using AgcDriver::DriverDetail::SpecializePublished;
using AgcDriver::DriverDetail::SpecializeQueued;
using AgcDriver::DriverDetail::SpecializeStale;

constexpr std::size_t Block = 65536;
constexpr std::size_t ColorBytes = 1024;
constexpr std::byte Kept{0x40};
std::byte* ColorMemory = nullptr;

alignas(256) constexpr std::array<std::uint32_t, 16> VertexCode{
    0x7e000d05u, 0x080200f4u, 0x10020300u, 0x100202f4u, 0x060202f3u, 0x060400f3u, 0x10040500u, 0x060404f3u,
    0x7e060280u, 0x7e0802f2u, 0xf80008cfu, 0x04030201u, 0xbf810000u, 0xbf810000u, 0xbf810000u, 0xbf810000u,
};
alignas(256) constexpr std::array<std::uint32_t, 8> PixelCode{0x7e000200u, 0x7e020280u, 0xf800180fu, 0x00010100u, 0xbf810000u, 0xbf810000u, 0xbf810000u, 0xbf810000u};
alignas(16) std::array<std::uint32_t, 4> IndirectArguments{3, 1, 0, 0};

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

std::byte* AllocateWatched(std::size_t bytes) {
#ifdef _WIN32
    void* block = GuestArena::GuestArenaAllocate_nid_postfix(bytes, Block);
    GuestArena::GuestArenaCommit_nid_postfix(block, bytes, PAGE_READWRITE, bytes);
#else
    void* raw = mmap(nullptr, bytes + Block, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED) throw std::runtime_error("cannot map the watched block");
    const auto begin = reinterpret_cast<std::uintptr_t>(raw);
    const auto aligned = (begin + Block - 1) & ~static_cast<std::uintptr_t>(Block - 1);
    if (aligned != begin) munmap(raw, aligned - begin);
    if (aligned + bytes != begin + bytes + Block) munmap(reinterpret_cast<void*>(aligned + bytes), begin + Block - aligned);
    void* block = reinterpret_cast<void*>(aligned);
    GuestWriteWatch::GuestWriteWatchRegister_nid_postfix(block, bytes);
#endif
    return static_cast<std::byte*>(block);
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

std::vector<std::uint32_t> Commands(bool indirect) {
    const auto vertex = reinterpret_cast<std::uintptr_t>(VertexCode.data());
    const auto pixel = reinterpret_cast<std::uintptr_t>(PixelCode.data());
    const auto color = reinterpret_cast<std::uintptr_t>(ColorMemory);
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
        {0x318, static_cast<std::uint32_t>(color >> 8u)}, {0x390, static_cast<std::uint32_t>(color >> 40u)},
    };
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> shader{
        {0xc8, static_cast<std::uint32_t>(vertex >> 8u)}, {0xc9, static_cast<std::uint32_t>(vertex >> 40u)}, {0x8b, 1u << 1u}, {0x8c, 0},
        {0x008, static_cast<std::uint32_t>(pixel >> 8u)}, {0x009, static_cast<std::uint32_t>(pixel >> 40u)}, {0x00b, 1u << 1u}, {0x00c, std::bit_cast<std::uint32_t>(1.0f)},
    };
    std::vector<std::uint32_t> commands;
    Append(commands, 0x79, {0x242, 4});
    for (const auto& [offset, value] : context) Append(commands, 0x69, {offset, value});
    for (const auto& [offset, value] : shader) Append(commands, 0x76, {offset, value});
    if (!indirect) {
        Append(commands, 0x2d, {3, 2});
        return commands;
    }
    Append(commands, 0x79, {0x24a, 0});
    Append(commands, 0x11, {1, Low(IndirectArguments.data()), High(IndirectArguments.data())});
    Append(commands, 0x24, {0, 0x8c, 0, 2});
    return commands;
}

std::vector<std::byte> Draw(bool indirect, const std::string& what) {
    std::fill_n(ColorMemory, ColorBytes, Kept);
    auto commands = Commands(indirect);
    Packet packet{commands.data(), static_cast<std::uint32_t>(commands.size()), 0, {}};
    Require(sceAgcDriverSubmitDcb(&packet) == 0, what + ": the submission failed");
    AgcDriverWaitIdle_nid_postfix();
    std::vector<std::byte> colors(ColorBytes);
    AgcDriver::GuestMemory::Read(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ColorMemory)), colors);
    return colors;
}

void RequireSame(const std::vector<std::byte>& colors, const std::vector<std::byte>& reference, const std::string& what) {
    for (std::size_t index = 0; index < reference.size(); ++index) {
        Require(colors[index] == reference[index], what + ": byte " + std::to_string(index) + " is " + std::to_string(std::to_integer<unsigned>(colors[index])) + ", the registration draw wrote " + std::to_string(std::to_integer<unsigned>(reference[index])));
    }
}

void SpecializedVariantsDrawLikeTheRegistrationDraw() {
    auto& driver = AgcDriver::DriverDetail::Driver::Get();
    ColorMemory = AllocateWatched(Block);
    Register(VertexHeader, VertexCode.data(), sizeof(VertexCode), 2);
    Register(PixelHeader, PixelCode.data(), sizeof(PixelCode), 1);
    const auto reference = Draw(false, "the first draw");
    const auto kept = static_cast<std::size_t>(std::count(reference.begin(), reference.end(), Kept));
    Require(kept != 0 && kept != reference.size(), "the first draw covered " + std::to_string(reference.size() - kept) + " of " + std::to_string(reference.size()) + " bytes, expected an edge through the target");
    RequireSame(Draw(false, "the second draw"), reference, "the second draw, a hit on the registration variants");
    driver.WaitSpecializations();
    const auto specialized = driver.DrawSpecializeCounters();
    const auto count = [&](SpecializeCount which) { return std::to_string(specialized.counts[which]); };
    const auto summary = count(SpecializeParked) + " parked, " + count(SpecializeQueued) + " queued, " + count(SpecializeDropped) + " dropped, " + count(SpecializePublished) + " published, " + count(SpecializeStale) + " stale, " + count(SpecializeFailed) + " failed, " + count(SpecializeNotSpecialized) + " not specialized";
    Require(specialized.counts[SpecializeFailed] == 0 && specialized.counts[SpecializeNotSpecialized] == 0 && specialized.counts[SpecializeStale] == 0, "a background specialization did not publish: " + summary);
    Require(specialized.specializedVariants == 2, "the draw cache holds " + std::to_string(specialized.specializedVariants) + " specialized variants with their registration result, expected the vertex and the pixel stage's (" + summary + ")");
    const auto before = driver.DrawCacheCounters();
    RequireSame(Draw(false, "the third draw"), reference, "the third draw, on the specialized variants");
    const auto after = driver.DrawCacheCounters();
    Require(after.specializedStageHits - before.specializedStageHits == 2, "the third draw hit " + std::to_string(after.specializedStageHits - before.specializedStageHits) + " specialized stage variants, expected 2");
    RequireSame(Draw(true, "the indirect draw"), reference, "an indirect draw over the specialized variants (CPU path: the base vertex is a vertex user word)");
    const auto indirect = driver.DrawCacheCounters();
    Require(indirect.specializedStageHits - after.specializedStageHits == 2, "the indirect draw hit " + std::to_string(indirect.specializedStageHits - after.specializedStageHits) + " specialized stage variants, expected 2");
    Require(driver.DrawSpecializeCounters().specializedVariants == 2, "the indirect draw replaced a specialized variant");
}

}

int main() {
    try {
        Require(std::getenv("APS5_BACKGROUND_SPECIALIZE") != nullptr, "the background specialization test must run with APS5_BACKGROUND_SPECIALIZE set");
        {
            const auto device = OpenVulkanTestDevice();
            if (!device || !device->GraphicsPipelineLibraries()) return VulkanTestSkipped;
        }
        SpecializedVariantsDrawLikeTheRegistrationDraw();
        AgcDriverShutdown_nid_postfix();
        std::cout << "draws on background-specialized variants, direct and indirect, wrote the registration draw's pixels\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        AgcDriverShutdown_nid_postfix();
        return 1;
    }
}
