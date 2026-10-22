#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Resource/include/Registration.hpp"
#include "prx/libSceAgcDriver/Submit/include/Workload.hpp"
#include <algorithm>
#include <iterator>
#include <stdexcept>

#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

int AgcDriver::SubmitMany(std::uint32_t* const* addresses, const std::uint32_t* sizes, std::uint32_t count, std::uint32_t queue) {
    if (count == 0) return 0;
    if (!addresses || !sizes) throw std::runtime_error("AGC driver: null multi-submit arrays");
    AgcDriver::GuestMemory::CheckRange(addresses, count * sizeof(*addresses), alignof(std::uint32_t*));
    AgcDriver::GuestMemory::CheckRange(sizes, count * sizeof(*sizes), alignof(std::uint32_t));
    for (std::uint32_t i = 0; i < count; ++i) {
        const Packet packet{addresses[i], sizes[i], 0, {}};
        AgcDriver::Submit(&packet, queue);
    }
    return 0;
}

namespace {

std::uint32_t PayloadDwords(std::uint32_t bytes) {
    return static_cast<std::uint32_t>((std::uint64_t{bytes} + 3) >> 2);
}

std::uint32_t WorkloadDataPacketDwords(std::uint32_t bytes) {
    if (bytes == 0) return 3;
    const std::uint32_t payload = PayloadDwords(bytes);
    return payload == 1 ? 4 : payload + 7;
}

constexpr std::uint32_t GraphicsQueueType = 0;
constexpr std::uint32_t ComputeQueueType = 1;

bool WorkloadCompute(std::uint32_t queueType) {
    if (queueType != GraphicsQueueType && queueType != ComputeQueueType)
        throw std::invalid_argument("AGC driver: workload packet for a queue type other than graphics or compute");
    return queueType == ComputeQueueType;
}

std::uint64_t WorkloadSlot(std::uint32_t streamId) {
    const auto slot = AgcDriverWorkloadStreamSlot_nid_no_patch(streamId);
    if (slot == 0) throw std::invalid_argument("AGC driver: workload packet for a stream that is not registered");
    return slot;
}

void CheckWorkloadPacket(std::uint32_t* command, std::uint32_t words) {
    if (!command) throw std::invalid_argument("AGC driver: null workload packet destination");
    AgcDriver::GuestMemory::CheckRange(command, words * sizeof(std::uint32_t), alignof(std::uint32_t), true);
}

}

extern "C" {

std::uint32_t APS5_VABI sceAgcDriverGetWaitRenderingPacketSizeInDwords() {
    return AgcDriver::RenderingWaitPacketWords;
}

std::uint32_t APS5_VABI sceAgcDriverGetSetFlipPacketSizeInDwords() {
    return AgcDriver::FlipPacketWords;
}

std::uint32_t APS5_VABI sceAgcDriverGetSetWorkloadsActivePacketSize() {
    return WorkloadDataPacketDwords(8) + 9;
}

std::uint32_t APS5_VABI sceAgcDriverGetSetWorkloadCompletePacketSize() {
    return WorkloadDataPacketDwords(0) + 9;
}

std::uint32_t APS5_VABI sceAgcDriverGetSetWorkloadStreamInactivePacketSize() {
    return WorkloadDataPacketDwords(8);
}

std::uint32_t APS5_VABI sceAgcDriverNopDataGetPacketSize(std::uint32_t bytes) {
    return PayloadDwords(bytes) + 2;
}

std::uint32_t APS5_VABI sceAgcDriverUserDataGetPacketSize(std::uint32_t bytes) {
    return WorkloadDataPacketDwords(bytes);
}

std::uint32_t APS5_VABI sceAgcDriverWaitUntilSafeForRendering(std::uint32_t** command,
    std::uint32_t capacity, std::uint32_t mode, std::uint32_t handle, int index) {
    if (!command || capacity < AgcDriver::RenderingWaitPacketWords || mode != 0 || index < 0)
        throw std::invalid_argument("AGC driver: invalid rendering wait arguments");
    AgcDriver::GuestMemory::CheckRange(command, sizeof(*command), alignof(std::uint32_t*), true);
    AgcDriver::GuestMemory::CheckRange(*command, AgcDriver::RenderingWaitPacketWords * sizeof(std::uint32_t), alignof(std::uint32_t), true);
    const std::uint32_t words[] = {AgcDriver::RenderingWaitPacketHeader, handle, static_cast<std::uint32_t>(index), mode};
    std::copy(std::begin(words), std::end(words), *command);
    *command += AgcDriver::RenderingWaitPacketWords;
    return 0;
}

std::uint32_t APS5_VABI sceAgcDriverSetFlip(std::uint32_t** command, std::uint32_t capacity, std::uint32_t queueType,
    std::uint32_t handle, std::int32_t index, std::uint32_t flipMode, std::int64_t flipArg) {
    if (!command || capacity < AgcDriver::FlipPacketWords || queueType != GraphicsQueueType)
        throw std::invalid_argument("AGC driver: invalid flip arguments");
    AgcDriver::GuestMemory::CheckRange(command, sizeof(*command), alignof(std::uint32_t*), true);
    AgcDriver::GuestMemory::CheckRange(*command, AgcDriver::FlipPacketWords * sizeof(std::uint32_t), alignof(std::uint32_t), true);
    const auto argument = static_cast<std::uint64_t>(flipArg);
    const std::uint32_t words[] = {AgcDriver::FlipPacketHeader, handle, static_cast<std::uint32_t>(index), flipMode,
        static_cast<std::uint32_t>(argument), static_cast<std::uint32_t>(argument >> 32u)};
    std::copy(std::begin(words), std::end(words), *command);
    *command += AgcDriver::FlipPacketWords;
    return 0;
}

int APS5_VABI sceAgcDriverSetWorkloadsActive(std::uint32_t* command, std::uint32_t queueType, std::uint32_t streamId,
    const std::uint32_t* workloadIds, std::uint32_t workloadCount) {
    const bool compute = WorkloadCompute(queueType);
    CheckWorkloadPacket(command, AgcDriver::Workload::ActiveWords);
    if (workloadIds && workloadCount != 0 && workloadCount <= AgcDriver::Workload::MaxWorkloadId)
        AgcDriver::GuestMemory::CheckRange(workloadIds, workloadCount * sizeof(*workloadIds), alignof(std::uint32_t));
    const auto mask = AgcDriver::Workload::ActiveMask(workloadIds, workloadCount);
    if (mask == 0) throw std::invalid_argument("AGC driver: invalid workload list");
    AgcDriver::Workload::WriteActive(command, compute, streamId, WorkloadSlot(streamId), mask);
    return 0;
}

int APS5_VABI sceAgcDriverSetWorkloadComplete(std::uint32_t* command, std::uint32_t queueType, std::uint32_t streamId, std::uint32_t workloadId) {
    const bool compute = WorkloadCompute(queueType);
    CheckWorkloadPacket(command, AgcDriver::Workload::CompleteWords);
    if (workloadId > AgcDriver::Workload::MaxWorkloadId) throw std::invalid_argument("AGC driver: workload id above 63");
    AgcDriver::Workload::WriteComplete(command, compute, streamId, workloadId, WorkloadSlot(streamId));
    return 0;
}

int APS5_VABI sceAgcDriverSubmitDcb(const Packet* packet) {
    AgcDriver::Submit(packet, 0);
    return 0;
}

int APS5_VABI sceAgcDriverSubmitMultiDcbs(std::uint32_t* const* dcbGpuAddrs, const std::uint32_t* dcbSizesInDwords, std::uint32_t count) {
    return AgcDriver::SubmitMany(dcbGpuAddrs, dcbSizesInDwords, count, 0);
}

int APS5_VABI sceAgcDriverAgrSubmitDcb(const Packet* packet) {
    AgcDriver::Submit(packet, 0);
    return 0;
}

int APS5_VABI sceAgcDriverAgrSubmitMultiDcbs(std::uint32_t* const* dcbGpuAddrs, const std::uint32_t* dcbSizesInDwords, std::uint32_t count) {
    return AgcDriver::SubmitMany(dcbGpuAddrs, dcbSizesInDwords, count, 0);
}

int APS5_VABI sceAgcDriverSetupAsyncGraphics(std::uint32_t pipeId) {
    (void)pipeId;
    return 0;
}

}
