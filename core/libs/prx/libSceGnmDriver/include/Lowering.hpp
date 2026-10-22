#ifndef CORE_LIBS_PRX_LIBSCEGNMDRIVER_INCLUDE_LOWERING_HPP
#define CORE_LIBS_PRX_LIBSCEGNMDRIVER_INCLUDE_LOWERING_HPP

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <vector>

namespace GnmDriver {

inline constexpr std::uint32_t OpNop = 0x10;
inline constexpr std::uint32_t OpWriteData = 0x37;
inline constexpr std::uint32_t OpEventWriteEop = 0x47;
inline constexpr std::uint32_t OpEventWriteEos = 0x48;
inline constexpr std::uint32_t OpReleaseMem = 0x49;
inline constexpr std::uint32_t OpDmaData = 0x50;
inline constexpr std::uint32_t OpAcquireMem = 0x58;
inline constexpr std::uint32_t WriteDataToMemory = 0x500u;
inline constexpr std::uint32_t FlipReserveWords = 64;
inline constexpr std::uint32_t PrepareFlipHeader = 0xc03e1000u;
inline constexpr std::uint32_t PrepareFlip = 0x68750777u;
inline constexpr std::uint32_t PrepareFlipLabel = 0x68750778u;
inline constexpr std::uint32_t PrepareFlipInterrupt = 0x68750780u;
inline constexpr std::uint32_t PrepareFlipInterruptLabel = 0x68750781u;

constexpr std::uint32_t Header(std::uint32_t opcode, std::uint32_t bodyWords, bool compute = false) {
    return 0xc0000000u | ((bodyWords - 1) << 16) | (opcode << 8) | (compute ? 2u : 0u);
}

inline std::uint32_t* Emit(std::uint32_t* at, std::uint32_t opcode, std::initializer_list<std::uint32_t> body, bool compute = false) {
    *at++ = Header(opcode, static_cast<std::uint32_t>(body.size()), compute);
    for (const auto word : body) *at++ = word;
    return at;
}

inline std::uint32_t* Nop(std::uint32_t* at, std::uint32_t bodyWords) {
    if (bodyWords == 0) throw std::logic_error("GnmDriver: empty NOP");
    *at = Header(OpNop, bodyWords);
    std::fill_n(at + 1, bodyWords, 0u);
    return at + 1 + bodyWords;
}

struct EngineCounters {
    std::uint64_t constant = 0;
    std::uint64_t draw = 0;
};

std::vector<std::uint32_t> MergeEngines(std::span<const std::uint32_t> dcb, std::span<const std::uint32_t> ccb, EngineCounters& counters);
void PatchFlip(std::uint32_t* reserve, std::uintptr_t bufferLabel, std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg);

}

#endif
