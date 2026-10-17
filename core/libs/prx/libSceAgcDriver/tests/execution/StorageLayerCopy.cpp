#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Recorder.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ShaderResources.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Texture.hpp"
#include "prx/libSceAgcDriver/Graphics/include/UnitShadow.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "prx/libc/include/GuestWriteWatch.hpp"
#include "Recompiler.hpp"
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
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;
using AgcDriver::Graphics::StorageTexture;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Threads = 32;
// Over 1 MiB in all, as the title's pair: no generation baseline is kept for it.
constexpr std::uint32_t Side = 512;
constexpr std::uint32_t Format32UInt = 20;
constexpr std::uint32_t TileLinear = 0;
constexpr std::uint32_t Type2D = 9;
constexpr std::uint32_t Type2DArray = 13;
constexpr std::size_t LayerBytes = std::size_t{Side} * Side * 4u;
constexpr std::size_t BlockBytes = 2 * LayerBytes;
constexpr std::uint32_t Initial0 = 0x5a5a5a5au;
constexpr std::uint32_t Initial1 = 0x6b6b6b6bu;

alignas(256) constexpr std::array<std::uint32_t, 5> StoreCode{
    0x7e020280, 0x7e04020c, 0xf0201108, 0x00010200, 0xbf810000,
};

alignas(256) constexpr std::array<std::uint32_t, 9> LoadSlice0Code{
    0x7e020280, 0x7e040280, 0xf0001128, 0x00010300, 0x34080082, 0xbf8c3f70, 0xe0701000, 0x80000304, 0xbf810000,
};

alignas(256) constexpr std::array<std::uint32_t, 9> LoadSlice1Code{
    0x7e020280, 0x7e040281, 0xf0001128, 0x00010300, 0x34080082, 0xbf8c3f70, 0xe0701000, 0x80000304, 0xbf810000,
};

alignas(256) std::array<std::uint32_t, Threads> Output{};

std::uint64_t AddressOf(const void* data) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(data));
}

std::string Hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08x", value);
    return text;
}

class GuestBlock {
public:
    explicit GuestBlock(bool watched) {
#ifdef _WIN32
        // A shared section, as the title's memory: its import is a separate alias, so the guest view
        // keeps its write stamps (a direct import is compared instead).
        block = static_cast<std::uint8_t*>(GuestArena::GuestArenaAllocate_nid_postfix(BlockBytes, 65536));
        Require(block != nullptr, "layer copy: cannot reserve the guest block");
        const auto section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_EXECUTE_READWRITE, 0, BlockBytes, nullptr);
        Require(section != nullptr, "layer copy: cannot create the shared section");
        GuestArena::GuestArenaMap_nid_postfix(block, BlockBytes, section, 0, PAGE_READWRITE);
        CloseHandle(section);
        static_cast<void>(watched);
#else
        constexpr std::uintptr_t alignment = 65536;
        void* mapped = mmap(nullptr, BlockBytes + alignment, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapped != MAP_FAILED) {
            const auto begin = reinterpret_cast<std::uintptr_t>(mapped);
            const auto aligned = (begin + alignment - 1) & ~(alignment - 1);
            if (aligned != begin) munmap(mapped, aligned - begin);
            if (aligned + BlockBytes != begin + BlockBytes + alignment) munmap(reinterpret_cast<void*>(aligned + BlockBytes), begin + alignment - aligned);
            block = reinterpret_cast<std::uint8_t*>(aligned);
            if (watched) GuestWriteWatch::GuestWriteWatchRegister_nid_postfix(block, BlockBytes);
        }
#endif
        Require(block != nullptr, "layer copy: cannot allocate the guest block");
        GuestAllocations::Mutation().Add(block, BlockBytes, true, true, true);
    }

    ~GuestBlock() {
        GuestAllocations::Mutation().Remove(block);
#ifdef _WIN32
        GuestArena::GuestArenaReset_nid_postfix(block, BlockBytes);
        GuestArena::GuestArenaRelease_nid_postfix(block, BlockBytes);
#else
        munmap(block, BlockBytes);
        GuestWriteWatch::GuestWriteWatchUnregister_nid_postfix(block, BlockBytes);
#endif
    }

    GuestBlock(const GuestBlock&) = delete;
    GuestBlock& operator=(const GuestBlock&) = delete;

    std::uint8_t* Data() { return block; }

private:
    std::uint8_t* block = nullptr;
};

std::array<std::uint32_t, 8> TextureDescriptor(const std::uint8_t* texels, std::uint32_t type, std::uint32_t lastSlice) {
    const auto address = AddressOf(texels);
    return {
        static_cast<std::uint32_t>(address >> 8u),
        static_cast<std::uint32_t>((address >> 40u) & 0xffu) | (Format32UInt << 20u) | (((Side - 1u) & 3u) << 30u),
        ((Side - 1u) >> 2u) | ((Side - 1u) << 14u),
        0xfacu | (TileLinear << 20u) | (type << 28u),
        lastSlice,
        0u,
        0u,
        0u,
    };
}

std::array<std::uint32_t, 4> BufferDescriptor(const void* data, std::uint32_t bytes) {
    const auto address = AddressOf(data);
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu), bytes, 0x31016facu};
}

void Dispatch(AgcDriver::VulkanDevice& device, std::span<const std::uint32_t> code, const std::vector<std::uint32_t>& userData) {
    const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{reinterpret_cast<std::uintptr_t>(code.data()), std::as_bytes(code)}}};
    const ShaderRecompiler::ShaderComputeStageInfo compute{{Threads, 1, 1}, 0u, {false, false, false}, false, 1};
    ShaderRecompiler::RecompileRequest request{
        {ShaderStage::Compute, reinterpret_cast<std::uintptr_t>(code.data()), code, 0, {}},
        {32, 0, userData, compute, std::nullopt, std::nullopt, memory},
        device.Target(),
        {0, 0, 0, 128}
    };
    request.useCache = false;
    const auto result = ShaderRecompiler::Recompile(request);
    device.Dispatch(result, 1, 1, 1, {}, reinterpret_cast<std::uintptr_t>(code.data()));
    device.WaitIdle();
}

// The one-layer image A: 32 texels of row 0 take `value`.
void Store(AgcDriver::VulkanDevice& device, const std::uint8_t* texels, std::uint32_t value) {
    std::vector<std::uint32_t> userData(16, 0u);
    const auto texture = TextureDescriptor(texels, Type2D, 0u);
    std::copy(texture.begin(), texture.end(), userData.begin() + 4);
    userData[12] = value;
    Dispatch(device, StoreCode, userData);
}

// Row 0 of one slice of the two-layer array image B into Output.
void Load(AgcDriver::VulkanDevice& device, const std::uint8_t* texels, std::uint32_t slice) {
    Output.fill(0xdeadbeefu);
    std::vector<std::uint32_t> userData(16, 0u);
    const auto buffer = BufferDescriptor(Output.data(), static_cast<std::uint32_t>(Output.size() * 4u));
    const auto texture = TextureDescriptor(texels, Type2DArray, 1u);
    std::copy(buffer.begin(), buffer.end(), userData.begin());
    std::copy(texture.begin(), texture.end(), userData.begin() + 4);
    Dispatch(device, slice == 0 ? std::span<const std::uint32_t>(LoadSlice0Code) : std::span<const std::uint32_t>(LoadSlice1Code), userData);
}

void RequireOutput(std::uint32_t first, std::uint32_t rest, const std::string& what) {
    for (std::uint32_t x = 0; x < Threads; ++x) {
        const auto expected = x == 0 ? first : rest;
        Require(Output[x] == expected, what + ": texel (" + std::to_string(x) + ", 0) reads " + Hex(Output[x]) + ", expected " + Hex(expected));
    }
}

void RequireMemory(const std::uint8_t* texels, std::uint32_t first, std::uint32_t rest, const std::string& what) {
    const auto* words = reinterpret_cast<const std::uint32_t*>(texels);
    for (std::size_t index = 0; index < BlockBytes / 4u; ++index) {
        const auto expected = index >= LayerBytes / 4u ? Initial1 : index == 0 ? first : index < Threads ? rest : Initial0;
        Require(words[index] == expected, what + ": guest word " + std::to_string(index) + " holds " + Hex(words[index]) + ", expected " + Hex(expected));
    }
}

void Run(AgcDriver::VulkanDevice& device, std::uint8_t* texels, bool tracked) {
    const auto surface = AddressOf(texels);
    std::fill_n(reinterpret_cast<std::uint32_t*>(texels), LayerBytes / 4u, Initial0);
    std::fill_n(reinterpret_cast<std::uint32_t*>(texels + LayerBytes), LayerBytes / 4u, Initial1);
    AgcDriver::GuestMemory::CollectWrites(surface, BlockBytes);
    AgcDriver::GuestMemory::BumpCollectEpoch();
    std::lock_guard gpu(AgcDriver::GuestMemory::GpuMutex());
    Load(device, texels, 0);
    RequireOutput(Initial0, Initial0, "B's layer 0 before any store");
    // B's first view over A's pending results makes B's storage image (A stored, B uploaded whole).
    Store(device, texels, 0x11111111u);
    Require(StorageTexture::FindPending(surface, LayerBytes) != nullptr, "the store into A left no pending image");
    Load(device, texels, 0);
    RequireOutput(0x11111111u, 0x11111111u, "B's layer 0 after A's first store");
    const bool shadowed = tracked && AgcDriver::GuestMemory::Watched(surface, BlockBytes) && AgcDriver::Graphics::UnitShadowEnabled() && std::getenv("APS5_NO_LAYER_SHADOW") == nullptr;
    // B's upload read A's results from the unit shadow: nothing was published into guest memory.
    if (shadowed && std::getenv("APS5_NO_UPLOAD_SHADOW") == nullptr) Require(reinterpret_cast<const std::uint32_t*>(texels)[0] == Initial0, "B's upload published A's shadowed results into guest memory");
    // With B cached, A's next results reach B's layer 0 through A's store, then B's refresh: on the device.
    Store(device, texels, 0x22222222u);
    Require(StorageTexture::FindPending(surface, LayerBytes) != nullptr, "the second store into A left no pending image");
    const auto copies = StorageTexture::LayerCopies();
    Load(device, texels, 0);
    RequireOutput(0x22222222u, 0x22222222u, "B's layer 0 after A's second store");
    tracked = tracked && AgcDriver::GuestMemory::Watched(surface, BlockBytes);
    if (!tracked) std::puts("guest memory has no write stamps: the layer copy is not checked");
    else if (std::getenv("APS5_NO_LAYER_COPY") != nullptr) Require(StorageTexture::LayerCopies() == copies, "APS5_NO_LAYER_COPY=1 still copied the layer");
    else Require(StorageTexture::LayerCopies() == copies + 1, "B's refresh read layer 0 back from guest memory instead of copying A's stored image");
    Load(device, texels, 1);
    RequireOutput(Initial1, Initial1, "B's layer 1 after the layer copy");
    if (shadowed) {
        // A's second store went into the import's unit shadow (B took the layer on the device): the
        // guest bytes get it only when a host-facing consumer publishes.
        Require(AgcDriver::Graphics::AnyShadowedOverlaps(surface, LayerBytes), "A's linear store did not go into the unit shadow");
        Require(reinterpret_cast<const std::uint32_t*>(texels)[0] != 0x22222222u, "A's linear store reached guest memory without a publish");
        std::puts("A's linear store stays in the unit shadow until a publish");
    }
    StorageTexture::FlushPending(surface, BlockBytes, nullptr, "test");
    device.WaitIdle();
    RequireMemory(texels, 0x22222222u, 0x22222222u, "after A's store");
    // A CPU store over the layer after A's store wins: B reads the layer back from memory.
    Store(device, texels, 0x33333333u);
    StorageTexture::FlushPending(surface, LayerBytes, nullptr, "test");
    device.WaitIdle();
    reinterpret_cast<std::uint32_t*>(texels)[0] = 0x77777777u;
    // The next packet's collect sees the store (the title bumps the epoch per packet).
    AgcDriver::GuestMemory::BumpCollectEpoch();
    const auto before = StorageTexture::LayerCopies();
    Load(device, texels, 0);
    RequireOutput(0x77777777u, 0x33333333u, "B's layer 0 after a CPU store over A's stored results");
    Require(StorageTexture::LayerCopies() == before, "B copied A's image over a layer the CPU wrote after A's store");
    RequireMemory(texels, 0x77777777u, 0x33333333u, "after the CPU store");
    if (tracked && std::getenv("APS5_NO_LAYER_COPY") == nullptr) {
        // A's store made earlier, in a finished batch: the layer is still copied.
        Store(device, texels, 0x44444444u);
        StorageTexture::FlushPending(surface, LayerBytes, nullptr, "test");
        device.WaitIdle();
        AgcDriver::GuestMemory::BumpCollectEpoch();
        auto copied = StorageTexture::LayerCopies();
        Load(device, texels, 0);
        RequireOutput(0x44444444u, 0x44444444u, "B's layer 0 after A's earlier store");
        Require(StorageTexture::LayerCopies() == copied + 1, "B read layer 0 back although A's earlier store still holds it");
        // A writer stamped only by its completion (a BDA table's stores) recorded after A's store: no copy.
        if (auto* recorder = AgcDriver::Graphics::Recorder::Active()) {
            Store(device, texels, 0x55555555u);
            StorageTexture::FlushPending(surface, LayerBytes, nullptr, "test");
            recorder->OnComplete([] {});
            copied = StorageTexture::LayerCopies();
            Load(device, texels, 0);
            RequireOutput(0x55555555u, 0x55555555u, "B's layer 0 behind a completion-stamped writer");
            Require(StorageTexture::LayerCopies() == copied, "B copied A's image past a writer only its completion stamps");
        } else {
            std::puts("no recorder: the completion-stamped writer case is not checked");
        }
    }
    AgcDriver::Graphics::ClearCachedTextures(device.Device());
}

}

int main() {
    try {
        const bool watched = AgcDriver::GuestMemory::WriteWatched();
        GuestBlock block(watched);
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        const bool tracked = watched && AgcDriver::GuestMemory::Watched(AddressOf(block.Data()), BlockBytes);
        Require(!watched || tracked, "layer copy: the shared guest block is not write-watched");
        Run(*device, block.Data(), tracked);
        std::puts("storage layer copy tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
