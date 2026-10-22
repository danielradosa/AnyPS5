#include "prx/libSceAgcDriver/Submit/include/CommandBuffer.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"

#include <cerrno>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::size_t QueueContextQueueOffset = 4;
constexpr std::uint32_t GraphicsQueue = 0;
constexpr std::uint32_t FirstComputeQueue = 0x20;
constexpr std::uint32_t ComputeQueueEnd = 0x58;

std::uint32_t ContextQueue(const void* queueContext, const char* function) {
    const auto* field = static_cast<const std::byte*>(queueContext) + QueueContextQueueOffset;
    AgcDriver::GuestMemory::CheckRange(field, sizeof(std::uint32_t), 1);
    std::uint32_t queue = 0;
    std::memcpy(&queue, field, sizeof(queue));
    if (queue != GraphicsQueue && (queue < FirstComputeQueue || queue >= ComputeQueueEnd))
        throw std::runtime_error(std::string(function) + ": unsupported queue " + std::to_string(queue) + " in the queue context");
    return queue;
}

}

extern "C" {

int APS5_VABI sceAgcDriverSubmitCommandBuffer(void* queue_context, const Packet* packet) {
    if (!queue_context || !packet) return EINVAL;
    AgcDriver::Submit(packet, ContextQueue(queue_context, __func__));
    return 0;
}

int APS5_VABI sceAgcDriverSubmitMultiCommandBuffers(void* queue_context, uint32_t* const* command_buffers, const uint32_t* sizes_in_dwords, uint32_t count) {
    if (count == 0) return 0;
    if (!queue_context || !command_buffers || !sizes_in_dwords) return EINVAL;
    return AgcDriver::SubmitMany(command_buffers, sizes_in_dwords, count, ContextQueue(queue_context, __func__));
}

}
