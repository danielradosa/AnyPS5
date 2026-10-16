#include "prx/libSceAgc/DcbFlow/include/Control.hpp"

#include "prx/libSceAgc/Command/include/Control.hpp"
#include "prx/libSceAgc/Command/include/Memory.hpp"
#include "prx/libSceAgc/Command/include/Packet.hpp"
#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


extern "C" {

std::uint32_t* APS5_VABI sceAgcDcbAtomicGds_0900(CommandBuffer* buf, std::uint32_t engine, std::uint32_t operation, std::uint32_t completion, std::uint32_t dataMode, std::uint32_t autoIncrementBytes, std::uint16_t base, std::uint16_t size, std::uint32_t offsets, std::uint32_t destination, std::uint64_t source0, std::uint64_t source1) {
    const std::uint32_t waitForCompletion = completion == 1u && (operation & 0x20u) == 0 ? 1u : 0u;
    auto* packet = Agc::Command::Allocate(buf, 11, __func__);
    packet[0] = Agc::Command::Header(0x1du, 11);
    packet[1] = ((engine & 3u) << 30u) | (operation & 0xffu) | ((operation << 7u) & 0x10000u) | (waitForCompletion << 17u) | ((operation & 0x20u) << 13u) | ((operation & 0x40u) << 14u);
    packet[2] = (autoIncrementBytes & 0x3fu) | ((dataMode & 1u) << 8u);
    packet[3] = base;
    packet[4] = size;
    packet[5] = offsets & 0x00ff00ffu;
    packet[6] = destination;
    packet[7] = static_cast<std::uint32_t>(source0);
    packet[8] = static_cast<std::uint32_t>(source0 >> 32u);
    packet[9] = static_cast<std::uint32_t>(source1);
    packet[10] = static_cast<std::uint32_t>(source1 >> 32u);
    return packet;
}

std::uint32_t* APS5_VABI sceAgcDcbAtomicGds(CommandBuffer* buf, std::uint32_t engine, std::uint32_t operation, std::uint32_t completion, std::uint32_t dataMode, std::uint32_t autoIncrementBytes, std::uint16_t base, std::uint16_t size, std::uint32_t offsets, std::uint32_t destination, std::uint64_t source0, std::uint64_t source1) {
    return sceAgcDcbAtomicGds_0900(buf, engine, operation, completion, dataMode, autoIncrementBytes, base, size, offsets, destination, source0, source1);
}

uint32_t* APS5_VABI sceAgcDcbJump(CommandBuffer* buf, uint8_t mode, uint8_t cache_policy, const uint32_t* target, uint32_t size_in_dwords) {
    return Agc::Command::WriteJump(buf, mode, cache_policy, target, size_in_dwords, __func__);
}

std::uint32_t APS5_VABI sceAgcDcbJumpGetSize() {
    return 16;
}

std::uint32_t* APS5_VABI sceAgcDcbResetQueue(CommandBuffer* buf, std::uint32_t op, std::uint32_t state) {
    Agc::Command::CheckBits(op, 0xfffu, __func__);
    Agc::Command::CheckBits(state, 0xfu, __func__);
    return Agc::Command::Emit(buf, 0x12u, {state}, __func__);
}

std::uint32_t* APS5_VABI sceAgcDcbClearState(CommandBuffer* buf, std::uint32_t command) {
    return Agc::Command::Emit(buf, 0x12u, {command & 0xfu}, __func__);
}

uint32_t* APS5_VABI sceAgcDcbRewind(CommandBuffer* buf, uint32_t initial_state) {
    return Agc::Command::WriteRewind(buf, initial_state, __func__);
}

uint32_t APS5_VABI sceAgcDcbRewindGetSize(void) {
    return 8;
}

uint32_t* APS5_VABI sceAgcDcbWaitUntilSafeForRendering(CommandBuffer* buf, uint32_t video_out_handle, uint32_t display_buffer_index) {
    auto* packet = Agc::Command::Allocate(buf, AgcDriver::RenderingWaitPacketWords, __func__);
    packet[0] = AgcDriver::RenderingWaitPacketHeader;
    packet[1] = video_out_handle;
    packet[2] = display_buffer_index;
    packet[3] = 0;
    return packet;
}

}
