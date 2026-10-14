#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DccMetadata.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Recorder.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Texture.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include "Recompiler.hpp"
#include "VulkanTestDevice.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

// A DCC register clear (keys 0x20) is materialized as a GPU clear of the target's image
// (Graphics/src/Draw.cpp materializeRegisterClear). While the title's 0x20 key fill is still
// recorded, the clear goes ahead without waiting only when the pending-store memo proves that fill
// is the newest writer of the keys, and the driver's uncompressed key store recorded after the clear
// must land behind the fill. This checks the recording order and the final bytes; the key-store run's
// leading barrier (Recorder::recordKeyStores) that orders the two on the GPU is not isolated here.

namespace {

using AgcDriver::Graphics::DccKeys;
using AgcDriver::Graphics::Require;
using AgcDriver::Graphics::StorageTexture;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Threads = 32;
constexpr std::uint32_t Side = 128;
constexpr std::uint32_t Format8888Unorm = 56;
constexpr std::uint32_t TileRenderTarget64KB = 0x1b;
constexpr std::uint32_t Type2D = 9;
constexpr std::size_t SurfaceBytes = 65536;
constexpr std::size_t KeyBytes = SurfaceBytes / 256;
constexpr std::size_t KeysOffset = SurfaceBytes;
constexpr std::size_t BlockBytes = 2 * 65536;

alignas(256) constexpr std::array<std::uint32_t, 8> WriteCode{
    0x7e020280, 0x7e0402f2, 0x7e0602f2, 0x7e0802f2, 0x7e0a02f2, 0xf0201f08, 0x00010200, 0xbf810000,
};

alignas(256) constexpr std::array<std::uint32_t, 6> ClearKeysCode{
    0x34020083, 0x7e040280, 0x7e060280, 0xe0741000, 0x80000201, 0xbf810000,
};

std::uint64_t AddressOf(const void* data) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(data));
}

class GuestBlock {
public:
    GuestBlock() {
#ifdef _WIN32
        block = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, BlockBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
#else
        block = static_cast<std::uint8_t*>(std::aligned_alloc(65536, BlockBytes));
#endif
        Require(block != nullptr, "register clear: cannot allocate the guest block");
        GuestAllocations::Mutation().Add(block, BlockBytes, true, true, true);
    }

    ~GuestBlock() {
        GuestAllocations::Mutation().Remove(block);
#ifdef _WIN32
        VirtualFree(block, 0, MEM_RELEASE);
#else
        std::free(block);
#endif
    }

    GuestBlock(const GuestBlock&) = delete;
    GuestBlock& operator=(const GuestBlock&) = delete;

    std::uint8_t* Data() { return block; }

private:
    std::uint8_t* block = nullptr;
};

std::array<std::uint32_t, 8> TextureDescriptor(const std::uint8_t* texels, const std::uint8_t* keys) {
    const auto address = AddressOf(texels);
    const auto meta = AddressOf(keys) >> 8u;
    return {
        static_cast<std::uint32_t>(address >> 8u),
        static_cast<std::uint32_t>((address >> 40u) & 0xffu) | (Format8888Unorm << 20u) | (((Side - 1u) & 3u) << 30u),
        ((Side - 1u) >> 2u) | ((Side - 1u) << 14u),
        0xfacu | (TileRenderTarget64KB << 20u) | (Type2D << 28u),
        0u,
        0u,
        (1u << 20u) | (1u << 21u) | (static_cast<std::uint32_t>(meta & 0xffu) << 24u),
        static_cast<std::uint32_t>(meta >> 8u),
    };
}

std::array<std::uint32_t, 4> BufferDescriptor(const void* data, std::uint32_t bytes) {
    const auto address = AddressOf(data);
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu), bytes, 0x31016facu};
}

void Dispatch(AgcDriver::VulkanDevice& device, std::span<const std::uint32_t> code, const std::vector<std::uint32_t>& userData, bool wait = true) {
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
    if (wait) device.WaitIdle();
}

std::size_t Count(const std::uint8_t* bytes, std::size_t size, std::uint8_t value) {
    return static_cast<std::size_t>(std::count(bytes, bytes + size, value));
}

void RequireKeys(const std::uint8_t* keys, std::uint8_t expected, const std::string& what) {
    const auto found = Count(keys, KeyBytes, expected);
    char message[256];
    std::snprintf(message, sizeof(message), "%s: %zu of %zu key bytes are 0x%02x (key 0 is 0x%02x)", what.c_str(), found, KeyBytes, expected, keys[0]);
    Require(found == KeyBytes, message);
}

int Run(AgcDriver::VulkanDevice& device, std::uint8_t* block) {
    auto* texels = block;
    auto* keys = block + KeysOffset;
    const auto surface = AddressOf(texels);
    const auto keyAddress = AddressOf(keys);
    std::memset(texels, 0x55, SurfaceBytes);
    std::memset(keys, 0xff, KeyBytes);
    AgcDriver::GuestMemory::BumpCollectEpoch();
    const auto texture = TextureDescriptor(texels, keys);
    std::vector<std::uint32_t> writeData(16, 0u);
    std::copy(texture.begin(), texture.end(), writeData.begin() + 4);
    std::vector<std::uint32_t> clearData(16, 0u);
    const auto keyBuffer = BufferDescriptor(keys, KeyBytes);
    std::copy(keyBuffer.begin(), keyBuffer.end(), clearData.begin());
    std::lock_guard gpu(AgcDriver::GuestMemory::GpuMutex());
    Dispatch(device, WriteCode, writeData);
    const auto image = StorageTexture::FindPending(surface, SurfaceBytes);
    Require(image != nullptr && image->Descriptor().dccAddress == keyAddress, "register clear: the written surface has no pending image under its keys");
    StorageTexture::FlushPending(surface, SurfaceBytes, nullptr, "test");
    device.WaitIdle();
    auto* recorder = AgcDriver::Graphics::Recorder::Active();
    Require(recorder != nullptr, "register clear: no active recorder");
    // The title's key fill as Driver's buffer fill makes it: noted on the images, recorded, then
    // noted in the pending-store memo.
    const auto fillRegisterKeys = [&](bool memoized) {
        const std::array<std::uint32_t, 4> pattern{0x20202020u, 0x20202020u, 0x20202020u, 0x20202020u};
        StorageTexture::NoteKeysFill(keyAddress, KeyBytes, 0x20);
        if (!device.FillBuffer(keyAddress, KeyBytes, pattern)) return false;
        if (memoized) AgcDriver::Graphics::NoteKeysFillOnGpu(keyAddress, KeyBytes, DccKeys::ClearRegister);
        return true;
    };
    if (!fillRegisterKeys(true)) {
        std::puts("skipped: the key range has no host import, so key fills are not recorded");
        return VulkanTestSkipped;
    }
    const std::array<std::uint32_t, 4> clear{0x80808080u, 0x80808080u, 0x80808080u, 0x80808080u};
    const char* refusal = nullptr;
    Require(recorder->PendingWriteOverlaps(keyAddress, KeyBytes) && keys[0] == 0xff, "the 0x20 key fill is not pending");
    Require(AgcDriver::Graphics::PendingKeysFill(keyAddress, KeyBytes) == DccKeys::ClearRegister, "the memo does not prove the pending 0x20 fill");
    Require(!image->FillClear(clear, StorageTexture::WholeImage, refusal) && refusal != nullptr && std::strcmp(refusal, "keys pending") == 0, "a clear not asking for proved keys went ahead over pending keys");
    refusal = nullptr;
    if (!image->FillClear(clear, StorageTexture::WholeImage, refusal, true)) Require(false, std::string("the clear over the proved 0x20 fill was refused: ") + (refusal != nullptr ? refusal : "?"));
    Require(recorder->PendingWriteOverlaps(keyAddress, KeyBytes) && keys[0] == 0xff, "the clear over the proved 0x20 fill waited for it");
    // materializeRegisterClear's next step, with a context that only finds the keys' import (the
    // fill above made it).
    AgcDriver::Graphics::Context probe{};
    probe.device = device.Device();
    probe.hostImportAlignment = 1;
    AgcDriver::Graphics::MarkDccUncompressed(probe, keyAddress, SurfaceBytes, KeyBytes);
    Require(keys[0] == 0xff && recorder->PendingWriteOverlaps(keyAddress, KeyBytes), "the uncompressed key store after the clear waited for the GPU");
    device.WaitIdle();
    RequireKeys(keys, 0xff, "the keys after the register clear (the uncompressed store must land after the 0x20 fill)");
    StorageTexture::FlushPending(surface, SurfaceBytes, nullptr, "test");
    device.WaitIdle();
    char message[256];
    std::snprintf(message, sizeof(message), "the register clear's write-back stored %zu of %zu texel bytes as 0x80", Count(texels, SurfaceBytes, 0x80), SurfaceBytes);
    Require(Count(texels, SurfaceBytes, 0x80) == SurfaceBytes, message);

    Require(fillRegisterKeys(true), "the second 0x20 key fill was not recorded");
    Dispatch(device, ClearKeysCode, clearData, false);
    refusal = nullptr;
    Require(!AgcDriver::Graphics::PendingKeysFill(keyAddress, KeyBytes).has_value(), "the memo proved a 0x20 fill that a kernel's key store follows");
    Require(!image->FillClear(clear, StorageTexture::WholeImage, refusal, true) && refusal != nullptr && std::strcmp(refusal, "keys pending") == 0, "a clear went ahead over a kernel's pending key store after the 0x20 fill");
    device.WaitIdle();
    // A key read drops the memo entry of the fill that has landed, so the next fill has none: the
    // value the recorder keeps for that fill proves its keys instead.
    AgcDriver::Graphics::ReadDccKeys(keyAddress, SurfaceBytes);

    Require(fillRegisterKeys(false), "the unmemoized 0x20 key fill was not recorded");
    refusal = nullptr;
    Require(AgcDriver::Graphics::PendingKeysFill(keyAddress, KeyBytes) == DccKeys::ClearRegister, "the recorded value of a 0x20 fill the memo never noted did not prove its keys");
    if (!image->FillClear(clear, StorageTexture::WholeImage, refusal, true)) Require(false, std::string("the clear over a 0x20 fill proved by its recorded value was refused: ") + (refusal != nullptr ? refusal : "?"));
    Require(recorder->PendingWriteOverlaps(keyAddress, KeyBytes), "the clear over a 0x20 fill proved by its recorded value waited for it");
    device.WaitIdle();

    // A 0x20 fill over two surfaces' keys: the uncompressed store over the first keeps the second's
    // pending answer.
    const std::array<std::uint32_t, 4> wide{0x20202020u, 0x20202020u, 0x20202020u, 0x20202020u};
    Require(device.FillBuffer(keyAddress, 2 * KeyBytes, wide), "the 0x20 fill over two key ranges was not recorded");
    AgcDriver::Graphics::NoteKeysFillOnGpu(keyAddress, 2 * KeyBytes, DccKeys::ClearRegister);
    AgcDriver::Graphics::MarkDccUncompressed(probe, keyAddress, SurfaceBytes, KeyBytes);
    Require(AgcDriver::Graphics::PendingKeysFill(keyAddress + KeyBytes, KeyBytes) == DccKeys::ClearRegister, "the uncompressed store over one surface's keys dropped the pending 0x20 fill's answer for the next surface's");
    device.WaitIdle();
    RequireKeys(keys, 0xff, "the first keys after the uncompressed store over a wider 0x20 fill");
    RequireKeys(keys + KeyBytes, 0x20, "the second keys after a wider 0x20 fill");
    return 0;
}

}

int main() {
    try {
        std::optional<GuestBlock> block;
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        block.emplace();
        const int result = Run(*device, block->Data());
        if (result == 0) std::puts("register clear over pending keys tests passed");
        return result;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
