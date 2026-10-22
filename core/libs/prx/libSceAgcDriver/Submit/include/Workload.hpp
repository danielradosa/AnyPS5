#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_SUBMIT_INCLUDE_WORKLOAD_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_SUBMIT_INCLUDE_WORKLOAD_HPP

#include <cstdint>

namespace AgcDriver::Workload {

inline constexpr std::uint32_t ActiveWords = 18;
inline constexpr std::uint32_t CompleteWords = 12;
inline constexpr std::uint32_t StreamInactiveWords = 9;
inline constexpr std::uint32_t MaxWorkloadId = 63;

namespace Detail {

inline constexpr std::uint32_t TagRegister = 0x342u;
inline constexpr std::uint32_t TagHighRegister = 0xc343u;
inline constexpr std::uint32_t TagRegisterWriteControl = 0x06010000u;
inline constexpr std::uint32_t ActiveTag = 0xcc000000u;
inline constexpr std::uint32_t CompleteTag = 0xcd000000u;
inline constexpr std::uint32_t AtomicSwap64 = 0x67u;
inline constexpr std::uint32_t AtomicAnd64 = 0x75u;
inline constexpr std::uint32_t AtomicWaitForWriteConfirmation = 2u << 8u;
inline constexpr std::uint32_t AtomicPrefetchEngine = 1u << 30u;

constexpr std::uint32_t Header(std::uint32_t opcode, std::uint32_t words, std::uint32_t flags = 0) {
    return 0xc0000000u | ((words - 2u) << 16u) | (opcode << 8u) | flags;
}

constexpr std::uint32_t TagFlags(bool compute) {
    return compute ? 0u : 4u;
}

inline void WriteTags(std::uint32_t* packet, bool compute, std::uint32_t tag, std::uint32_t maskLow, std::uint32_t maskHigh) {
    packet[0] = Header(0x79u, 4, TagFlags(compute));
    packet[1] = TagRegister;
    packet[2] = tag;
    packet[3] = maskLow;
    packet[4] = Header(0x37u, 5, TagFlags(compute));
    packet[5] = TagRegisterWriteControl;
    packet[6] = TagHighRegister;
    packet[7] = maskHigh;
    packet[8] = 0;
}

inline void WriteSlotAtomic(std::uint32_t* packet, bool compute, std::uint32_t operation, std::uint64_t slot, std::uint64_t source) {
    packet[0] = Header(0x1eu, 9);
    packet[1] = operation | AtomicWaitForWriteConfirmation | (compute ? 0u : AtomicPrefetchEngine);
    packet[2] = static_cast<std::uint32_t>(slot);
    packet[3] = static_cast<std::uint32_t>(slot >> 32u);
    packet[4] = static_cast<std::uint32_t>(source);
    packet[5] = static_cast<std::uint32_t>(source >> 32u);
    packet[6] = 0;
    packet[7] = 0;
    packet[8] = 0;
}

}

inline std::uint64_t ActiveMask(const std::uint32_t* workloadIds, std::uint32_t workloadCount) {
    if (workloadIds == nullptr || workloadCount == 0 || workloadCount > MaxWorkloadId) return 0;
    std::uint64_t mask = 0;
    for (std::uint32_t i = 0; i < workloadCount; ++i) {
        if (workloadIds[i] > MaxWorkloadId) return 0;
        const auto bit = std::uint64_t{1} << workloadIds[i];
        if ((mask & bit) != 0) return 0;
        mask |= bit;
    }
    return mask;
}

inline void WriteActive(std::uint32_t* packet, bool compute, std::uint32_t streamId, std::uint64_t slot, std::uint64_t mask) {
    Detail::WriteTags(packet, compute, Detail::ActiveTag | streamId, static_cast<std::uint32_t>(mask), static_cast<std::uint32_t>(mask >> 32u));
    Detail::WriteSlotAtomic(packet + StreamInactiveWords, compute, Detail::AtomicSwap64, slot, mask);
}

inline void WriteComplete(std::uint32_t* packet, bool compute, std::uint32_t streamId, std::uint32_t workloadId, std::uint64_t slot) {
    packet[0] = Detail::Header(0x79u, 3, Detail::TagFlags(compute));
    packet[1] = Detail::TagRegister;
    packet[2] = Detail::CompleteTag | (workloadId << 5u) | streamId;
    Detail::WriteSlotAtomic(packet + 3, compute, Detail::AtomicAnd64, slot, ~(std::uint64_t{1} << workloadId));
}

inline void WriteStreamInactive(std::uint32_t* packet, bool compute, std::uint32_t streamId) {
    Detail::WriteTags(packet, compute, Detail::ActiveTag | streamId, 0, 0);
}

}

#endif
