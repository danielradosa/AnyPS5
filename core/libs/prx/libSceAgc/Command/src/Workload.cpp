#include "prx/libSceAgc/Command/include/Workload.hpp"

#include "prx/libSceAgcDriver/Resource/include/Registration.hpp"
#include "prx/libSceAgcDriver/Submit/include/Workload.hpp"

namespace Agc::Command {

std::uint32_t* WriteWorkloadsActive(CommandBuffer* buffer, bool compute, std::uint32_t streamId, const std::uint32_t* workloadIds, std::uint32_t workloadCount, const char* function) {
    const auto mask = AgcDriver::Workload::ActiveMask(workloadIds, workloadCount);
    if (mask == 0) return nullptr;
    const auto slot = AgcDriverWorkloadStreamSlot_nid_no_patch(streamId);
    if (slot == 0) return nullptr;
    auto* packet = Allocate(buffer, AgcDriver::Workload::ActiveWords, function);
    AgcDriver::Workload::WriteActive(packet, compute, streamId, slot, mask);
    return packet;
}

std::uint32_t* WriteWorkloadComplete(CommandBuffer* buffer, bool compute, std::uint32_t streamId, std::uint32_t workloadId, const char* function) {
    const auto slot = AgcDriverWorkloadStreamSlot_nid_no_patch(streamId);
    if (slot == 0 || workloadId > AgcDriver::Workload::MaxWorkloadId) return nullptr;
    auto* packet = Allocate(buffer, AgcDriver::Workload::CompleteWords, function);
    AgcDriver::Workload::WriteComplete(packet, compute, streamId, workloadId, slot);
    return packet;
}

std::uint32_t* WriteWorkloadStreamInactive(CommandBuffer* buffer, bool compute, std::uint32_t streamId, const char* function) {
    if (AgcDriverWorkloadStreamSlot_nid_no_patch(streamId) == 0) return nullptr;
    auto* packet = Allocate(buffer, AgcDriver::Workload::StreamInactiveWords, function);
    AgcDriver::Workload::WriteStreamInactive(packet, compute, streamId);
    return packet;
}

}
