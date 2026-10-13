#include "prx/libSceAgcDriver/Submit/include/CommandBuffer.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"

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
    if (queueContext == nullptr) throw std::runtime_error(std::string(function) + ": null queue context");
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

int APS5_VABI sceAgcDriverSubmitCommandBuffer(void* queueContext, const Packet* packet) {
    AgcDriver::Submit(packet, ContextQueue(queueContext, __func__));
    return 0;
}

int APS5_VABI sceAgcDriverSubmitMultiCommandBuffers(void* queueContext, uint32_t* const* commandBuffers, const uint32_t* sizesInDwords, uint32_t count) {
    if (count == 0) return 0;
    return AgcDriver::SubmitMany(commandBuffers, sizesInDwords, count, ContextQueue(queueContext, __func__));
}

}
