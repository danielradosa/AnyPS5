#include "prx/libSceAgc/Cb/include/Dispatch.hpp"

#include "prx/libSceAgc/Command/include/Packet.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::uint32_t SetShRegister = 0x76u;
constexpr std::uint32_t DispatchDirect = 0x15u;
constexpr std::uint32_t ComputeNumThreadX = 0x207u;
constexpr std::uint32_t ComputePgmLo = 0x20cu;
constexpr std::uint32_t ComputePgmRsrc1 = 0x212u;
constexpr std::uint32_t ComputePgmRsrc3 = 0x228u;
constexpr std::uint32_t ComputeUserData0 = 0x240u;
constexpr std::uint32_t MemsetGroupThreads = 64u;
constexpr std::uint32_t MemsetVgprBlocks = 1u;
constexpr std::uint32_t MemsetUserSgprs = 8u;
constexpr std::uint32_t MemsetGroupIdX = 0x80u;
constexpr std::uint32_t MemsetBlockBytes = 16u;
constexpr std::uint32_t MemsetBufferFormat = 0x4bfacu;
constexpr std::uint32_t MemsetPacketDwords = 32u;

alignas(256) constexpr std::uint32_t MemsetKernel[] = {0xd7460004u, 0x04010c08u, 0x7e000204u, 0x7e020205u, 0x7e040206u, 0x7e060207u, 0xe01c2000u, 0x80000004u, 0xbf810000u};

}

extern "C" {

std::uint32_t* APS5_VABI sceAgcCbDispatch(CommandBuffer* buf, std::uint32_t threadGroupX, std::uint32_t threadGroupY, std::uint32_t threadGroupZ, std::uint32_t modifier) {
    Agc::Command::CheckBits(modifier, 0xa038u | 0x41u, __func__);
    return Agc::Command::Emit(buf, 0x15u, {threadGroupX, threadGroupY, threadGroupZ, modifier | 0x41u}, __func__);
}

uint32_t APS5_VABI sceAgcCbDispatchGetSize(void) {
    return 20;
}

std::uint32_t* APS5_VABI sceAgcCbMemsetExclusive(CommandBuffer* buf, std::uint64_t destination, const void* pattern, std::uint64_t sizeInBytes) {
    Agc::Command::Require(pattern != nullptr, __func__, "null pattern");
    Agc::Command::CheckAddress(destination, MemsetBlockBytes, __func__);
    Agc::Command::CheckBits(destination, 0xffffffffffffull, __func__);
    Agc::Command::Require(sizeInBytes % MemsetBlockBytes == 0, __func__, "size is not a multiple of 16 bytes");
    Agc::Command::Require(sizeInBytes / MemsetBlockBytes <= 0xffffffffu, __func__, "more than 2^32 - 1 blocks");
    const auto kernel = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(MemsetKernel));
    Agc::Command::Require((kernel >> 48u) == 0, __func__, "memset kernel outside the 48-bit GPU address range");
    std::uint32_t words[4];
    std::memcpy(words, pattern, sizeof(words));
    const auto blocks = static_cast<std::uint32_t>(sizeInBytes / MemsetBlockBytes);
    const auto groups = static_cast<std::uint32_t>((static_cast<std::uint64_t>(blocks) + MemsetGroupThreads - 1u) / MemsetGroupThreads);
    const std::uint32_t program[] = {static_cast<std::uint32_t>(kernel >> 8u), static_cast<std::uint32_t>(kernel >> 40u) & 0xffu};
    const std::uint32_t resources[] = {MemsetVgprBlocks, (MemsetUserSgprs << 1u) | MemsetGroupIdX};
    const std::uint32_t resources3[] = {0u};
    const std::uint32_t threads[] = {MemsetGroupThreads, 1u, 1u};
    const std::uint32_t userData[] = {
        static_cast<std::uint32_t>(destination), (static_cast<std::uint32_t>(destination >> 32u) & 0xffffu) | (MemsetBlockBytes << 16u), blocks, MemsetBufferFormat,
        words[0], words[1], words[2], words[3], 0u};
    Agc::Command::Reserve(buf, MemsetPacketDwords, __func__);
    auto* first = Agc::Command::WriteRegisterRange(buf, SetShRegister, ComputePgmLo, program, 2, __func__);
    Agc::Command::WriteRegisterRange(buf, SetShRegister, ComputePgmRsrc1, resources, 2, __func__);
    Agc::Command::WriteRegisterRange(buf, SetShRegister, ComputePgmRsrc3, resources3, 1, __func__);
    Agc::Command::WriteRegisterRange(buf, SetShRegister, ComputeNumThreadX, threads, 3, __func__);
    Agc::Command::WriteRegisterRange(buf, SetShRegister, ComputeUserData0, userData, 9, __func__);
    Agc::Command::Emit(buf, DispatchDirect, {groups, 1u, 1u, 0x41u}, __func__);
    return first;
}

}
