#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/GuestBufferMemory.hpp"
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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <span>
#include <string>
#include <string_view>

namespace {

using AgcDriver::Graphics::Require;
using ShaderRecompiler::ShaderStage;

constexpr std::size_t BlockBytes = 65536;
constexpr std::size_t CounterOffset = 256;
constexpr std::uint32_t Threads = 32;
constexpr std::array<std::uint32_t, 3> Groups{2u, 3u, 1u};

alignas(256) constexpr std::array<std::uint32_t, 6> WriterCode{0x7e020282u, 0x7e040283u, 0x7e060281u, 0xe07c0000u, 0x80000100u, 0xbf810000u};
alignas(256) constexpr std::array<std::uint32_t, 4> CounterCode{0x7e020281u, 0xe0c80000u, 0x80000100u, 0xbf810000u};

class GuestBlock {
public:
    GuestBlock() {
#ifdef _WIN32
        block = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, BlockBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
#else
        block = static_cast<std::uint8_t*>(std::aligned_alloc(BlockBytes, BlockBytes));
#endif
        Require(block != nullptr, "fault-only indirect args: cannot allocate the guest block");
        std::memset(block, 0, BlockBytes);
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

    std::uint64_t Address() const { return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(block)); }

private:
    std::uint8_t* block = nullptr;
};

void SetEnvironment(const char* name, const char* value) {
#ifdef _WIN32
    Require(_putenv_s(name, value) == 0, std::string("fault-only indirect args: cannot set ") + name);
#else
    Require(setenv(name, value, 1) == 0, std::string("fault-only indirect args: cannot set ") + name);
#endif
}

ShaderRecompiler::RecompileResult Compile(std::span<const std::uint32_t> code, std::uint64_t address, const ShaderRecompiler::SpirvTarget& target) {
    const std::array<std::uint32_t, 4> userData{static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu), 16u, 0x31016facu};
    const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{reinterpret_cast<std::uintptr_t>(code.data()), std::as_bytes(code)}}};
    const ShaderRecompiler::ShaderComputeStageInfo compute{{Threads, 1, 1}, 0u, {false, false, false}, false, 1};
    ShaderRecompiler::RecompileRequest request{
        {ShaderStage::Compute, reinterpret_cast<std::uintptr_t>(code.data()), code, 0, {}},
        {32u, 0, userData, compute, std::nullopt, std::nullopt, memory},
        target,
        {0, 0, 0, 128}
    };
    request.useCache = false;
    return ShaderRecompiler::Recompile(request);
}

std::uint32_t Word(GuestBlock& guest, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, guest.Data() + offset, sizeof(value));
    return value;
}

}

int main(int argc, char** argv) {
    try {
        const bool listAll = argc > 1 && std::string_view(argv[1]) == "all";
        SetEnvironment("APS5_LOOP_GUARD", "1000");
        SetEnvironment("APS5_NO_OPPORTUNISTIC_REAP", "1");
        if (listAll) SetEnvironment("APS5_LIST_ALL_COMPLETION_WRITERS", "1");
        const auto mode = std::string(listAll ? " (APS5_LIST_ALL_COMPLETION_WRITERS=1)" : "");
        GuestBlock guest;
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        const auto writer = Compile(WriterCode, guest.Address(), device->Target());
        const auto counter = Compile(CounterCode, guest.Address() + CounterOffset, device->Target());
        const auto faults = [](const ShaderRecompiler::RecompileResult& compiled) {
            return std::count_if(compiled.bindings.begin(), compiled.bindings.end(), [](const auto& binding) { return binding.role == ShaderRecompiler::DescriptorRole::FaultBuffer; });
        };
        Require(faults(writer) == 1, "fault-only indirect args: the loop guard gave the argument writer no fault buffer, so it has no completion work and the test proves nothing");
        int cpuReason = -1;
        {
            std::lock_guard lock(AgcDriver::GuestMemory::GpuMutex());
            device->Dispatch(writer, 1u, 1u, 1u, {}, reinterpret_cast<std::uintptr_t>(WriterCode.data()));
            AgcDriver::Graphics::Context probe{};
            probe.device = device->Device();
            probe.hostImportAlignment = 1;
            if (!AgcDriver::Graphics::HostImportCovers(probe, guest.Address(), 16)) {
                device->WaitIdle();
                std::puts("skipped, the device does not import guest memory");
                return VulkanTestSkipped;
            }
            cpuReason = device->DispatchIndirect(counter, guest.Address(), {}, reinterpret_cast<std::uintptr_t>(CounterCode.data())).cpuReason;
            device->WaitIdle();
        }
        for (std::size_t index = 0; index < Groups.size(); ++index) {
            Require(Word(guest, index * 4u) == Groups[index], "fault-only indirect args" + mode + ": group count " + std::to_string(index) + " is " + std::to_string(Word(guest, index * 4u)) + ", expected " + std::to_string(Groups[index]));
        }
        const auto expected = Groups[0] * Groups[1] * Groups[2] * Threads;
        Require(Word(guest, CounterOffset) == expected, "fault-only indirect args" + mode + ": the indirect dispatch counted " + std::to_string(Word(guest, CounterOffset)) + " lanes, expected " + std::to_string(expected));
        if (listAll) {
            Require(cpuReason == 2, "fault-only indirect args" + mode + ": a listed fault-only writer of the arguments did not send them to the CPU (reason " + std::to_string(cpuReason) + ")");
        } else {
            Require(cpuReason == 0, "fault-only indirect args: an in-place fault-only writer of the arguments sent the indirect dispatch to the CPU (reason " + std::to_string(cpuReason) + "), as if its completion stored them");
        }
        std::puts(listAll ? "fault-only indirect args tests passed (all writers listed)" : "fault-only indirect args tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
