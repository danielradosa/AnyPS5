#include "prx/libSceAgc/Command/include/Workload.hpp"

#include "prx/libSceAgcDriver/Resource/include/Registration.hpp"

namespace Agc::Command {

namespace {

constexpr std::uint32_t MaxWorkloadId = 63;
constexpr std::uint32_t TagRegister = 0x342u;
constexpr std::uint32_t TagHighRegister = 0xc343u;
constexpr std::uint32_t TagRegisterWriteControl = 0x06010000u;
constexpr std::uint32_t ActiveTag = 0xcc000000u;
constexpr std::uint32_t CompleteTag = 0xcd000000u;
constexpr std::uint32_t AtomicSwap64 = 0x67u;
constexpr std::uint32_t AtomicAnd64 = 0x75u;
constexpr std::uint32_t AtomicWaitForWriteConfirmation = 2u << 8u;
constexpr std::uint32_t AtomicPrefetchEngine = 1u << 30u;

std::uint32_t tagFlags(bool compute) {
    return compute ? 0u : 4u;
}

void writeTags(std::uint32_t* packet, bool compute, std::uint32_t tag, std::uint32_t maskLow, std::uint32_t maskHigh) {
    packet[0] = Header(0x79u, 4, tagFlags(compute));
    packet[1] = TagRegister;
    packet[2] = tag;
    packet[3] = maskLow;
    packet[4] = Header(0x37u, 5, tagFlags(compute));
    packet[5] = TagRegisterWriteControl;
    packet[6] = TagHighRegister;
    packet[7] = maskHigh;
    packet[8] = 0;
}

void writeSlotAtomic(std::uint32_t* packet, bool compute, std::uint32_t operation, std::uint64_t slot, std::uint64_t source) {
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

std::uint32_t* WriteWorkloadsActive(CommandBuffer* buffer, bool compute, std::uint32_t streamId, const std::uint32_t* workloadIds, std::uint32_t workloadCount, const char* function) {
    if (workloadIds == nullptr || workloadCount == 0 || workloadCount > MaxWorkloadId) return nullptr;
    const auto slot = AgcDriverWorkloadStreamSlot_nid_no_patch(streamId);
    if (slot == 0) return nullptr;
    std::uint64_t mask = 0;
    for (std::uint32_t i = 0; i < workloadCount; ++i) {
        if (workloadIds[i] > MaxWorkloadId) return nullptr;
        const auto bit = std::uint64_t{1} << workloadIds[i];
        if ((mask & bit) != 0) return nullptr;
        mask |= bit;
    }
    auto* packet = Allocate(buffer, 18, function);
    writeTags(packet, compute, ActiveTag | streamId, static_cast<std::uint32_t>(mask), static_cast<std::uint32_t>(mask >> 32u));
    writeSlotAtomic(packet + 9, compute, AtomicSwap64, slot, mask);
    return packet;
}

std::uint32_t* WriteWorkloadComplete(CommandBuffer* buffer, bool compute, std::uint32_t streamId, std::uint32_t workloadId, const char* function) {
    const auto slot = AgcDriverWorkloadStreamSlot_nid_no_patch(streamId);
    if (slot == 0 || workloadId > MaxWorkloadId) return nullptr;
    auto* packet = Allocate(buffer, 12, function);
    packet[0] = Header(0x79u, 3, tagFlags(compute));
    packet[1] = TagRegister;
    packet[2] = CompleteTag | (workloadId << 5u) | streamId;
    writeSlotAtomic(packet + 3, compute, AtomicAnd64, slot, ~(std::uint64_t{1} << workloadId));
    return packet;
}

std::uint32_t* WriteWorkloadStreamInactive(CommandBuffer* buffer, bool compute, std::uint32_t streamId, const char* function) {
    if (AgcDriverWorkloadStreamSlot_nid_no_patch(streamId) == 0) return nullptr;
    auto* packet = Allocate(buffer, 9, function);
    writeTags(packet, compute, ActiveTag | streamId, 0, 0);
    return packet;
}

}
