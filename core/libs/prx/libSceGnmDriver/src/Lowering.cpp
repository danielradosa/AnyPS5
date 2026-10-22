#include "prx/libSceGnmDriver/include/Lowering.hpp"
#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace GnmDriver {

namespace {

constexpr std::uint32_t OpSurfaceSync = 0x43;
constexpr std::uint32_t OpLoadConstRam = 0x80;
constexpr std::uint32_t OpWriteConstRam = 0x81;
constexpr std::uint32_t OpDumpConstRam = 0x83;
constexpr std::uint32_t OpIncrementCeCounter = 0x84;
constexpr std::uint32_t OpIncrementDeCounter = 0x85;
constexpr std::uint32_t OpWaitOnCeCounter = 0x86;
constexpr std::uint32_t OpWaitOnDeCounterDiff = 0x88;
constexpr std::uint32_t DmaRegisterDestination = 1u << 27;
constexpr std::uint32_t DmaNoDestinationIncrement = 1u << 29;
constexpr std::uint32_t DmaSourceGds = 1u << 29;
constexpr std::uint32_t DmaCpSync = 1u << 31;
constexpr std::uint32_t CpDmaReadTagsBytes = 0xc08bu * 4;
constexpr std::uint32_t EventControlMask = 0xf3fu;
constexpr std::uint32_t EosGdsStore = 1;
constexpr std::uint32_t EosStoreData = 2;
constexpr std::uint32_t ReleaseMemData32 = 1u << 29;
constexpr std::uint32_t EopInterruptOnly = 0x01000000u;
constexpr std::uint32_t EopData32InterruptAfterWrite = 0x22000000u;
constexpr std::uint32_t EopEventIndex = 0x500u;

std::span<const std::uint32_t> PacketAt(std::span<const std::uint32_t> words, std::size_t at, const char* stream) {
    const auto header = words[at];
    if ((header >> 30) != 3) throw std::runtime_error(std::string("GnmDriver: unsupported PM4 packet type in the ") + stream);
    const std::size_t total = ((header >> 16) & 0x3fffu) + 2;
    if (total > words.size() - at) throw std::runtime_error(std::string("GnmDriver: truncated packet in the ") + stream);
    return words.subspan(at, total);
}

std::uint32_t Opcode(std::uint32_t header) {
    return (header >> 8) & 0xffu;
}

void Append(std::vector<std::uint32_t>& out, std::initializer_list<std::uint32_t> words) {
    out.insert(out.end(), words);
}

void Lower(std::vector<std::uint32_t>& out, std::span<const std::uint32_t> packet) {
    switch (Opcode(packet[0])) {
        case OpEventWriteEop:
            if (packet.size() != 6) throw std::runtime_error("GnmDriver: invalid EVENT_WRITE_EOP size");
            Append(out, {Header(OpReleaseMem, 7), packet[1] & EventControlMask, packet[3] & 0xe3000000u, packet[2], packet[3] & 0xffffu, packet[4], packet[5], 0});
            return;
        case OpEventWriteEos: {
            if (packet.size() != 5) throw std::runtime_error("GnmDriver: invalid EVENT_WRITE_EOS size");
            const auto command = packet[3] >> 29;
            if (command == EosStoreData) {
                Append(out, {Header(OpReleaseMem, 7), packet[1] & EventControlMask, ReleaseMemData32, packet[2], packet[3] & 0xffffu, packet[4], 0, 0});
                return;
            }
            if (command != EosGdsStore) throw std::runtime_error("GnmDriver: EVENT_WRITE_EOS command " + std::to_string(command) + " is not implemented");
            const auto dwords = packet[4] >> 16;
            if (dwords == 0) throw std::runtime_error("GnmDriver: EVENT_WRITE_EOS GDS store of no dwords");
            Append(out, {Header(OpReleaseMem, 7), packet[1] & EventControlMask, 0, 0, 0, 0, 0, 0});
            Append(out, {Header(OpDmaData, 6), DmaCpSync | DmaSourceGds, packet[4] & 0xffffu, 0, packet[2], packet[3] & 0xffffu, dwords * 4});
            return;
        }
        case OpSurfaceSync:
            if (packet.size() != 5) throw std::runtime_error("GnmDriver: invalid SURFACE_SYNC size");
            Append(out, {Header(OpAcquireMem, 6), packet[1], packet[2], 0, packet[3], 0, packet[4]});
            return;
        case OpNop:
            out.insert(out.end(), packet.begin(), packet.end());
            if (packet.size() >= 2 && (packet[1] & 0xffff0000u) == 0x68750000u) out[out.size() - packet.size() + 1] = 0;
            return;
        case OpDmaData:
            if (packet.size() == 7 && (packet[6] & DmaRegisterDestination) && (packet[6] & DmaNoDestinationIncrement) && packet[4] == CpDmaReadTagsBytes && packet[5] == 0) {
                const auto at = out.size();
                out.resize(at + packet.size());
                Nop(&out[at], static_cast<std::uint32_t>(packet.size()) - 1);
                return;
            }
            out.insert(out.end(), packet.begin(), packet.end());
            return;
        default:
            out.insert(out.end(), packet.begin(), packet.end());
    }
}

}

std::vector<std::uint32_t> MergeEngines(std::span<const std::uint32_t> dcb, std::span<const std::uint32_t> ccb, EngineCounters& counters) {
    std::vector<std::uint32_t> out;
    out.reserve(dcb.size() + ccb.size());
    std::size_t constant = 0;
    const auto runConstant = [&](bool untilAhead) {
        while (constant < ccb.size() && !(untilAhead && counters.constant > counters.draw)) {
            const auto packet = PacketAt(ccb, constant, "constant command buffer");
            switch (Opcode(packet[0])) {
                case OpIncrementCeCounter:
                    ++counters.constant;
                    break;
                case OpWaitOnDeCounterDiff:
                    if (packet.size() != 2) throw std::runtime_error("GnmDriver: invalid WAIT_ON_DE_COUNTER_DIFF size");
                    if (static_cast<std::int64_t>(counters.constant - counters.draw) >= static_cast<std::int64_t>(packet[1])) return;
                    break;
                case OpLoadConstRam: case OpWriteConstRam: case OpDumpConstRam: case OpNop:
                    Lower(out, packet);
                    break;
                default:
                    throw std::runtime_error("GnmDriver: constant engine packet " + std::to_string(Opcode(packet[0])) + " is not implemented");
            }
            constant += packet.size();
        }
    };
    for (std::size_t draw = 0; draw < dcb.size();) {
        const auto packet = PacketAt(dcb, draw, "draw command buffer");
        draw += packet.size();
        switch (Opcode(packet[0])) {
            case OpWaitOnCeCounter:
                runConstant(true);
                if (counters.constant <= counters.draw) throw std::runtime_error("GnmDriver: the draw engine waits on a constant engine counter that is never incremented");
                break;
            case OpIncrementDeCounter:
                ++counters.draw;
                break;
            default:
                Lower(out, packet);
        }
    }
    runConstant(false);
    if (constant < ccb.size()) throw std::runtime_error("GnmDriver: the constant engine waits on draw engine progress that the submission never makes");
    return out;
}

void PatchFlip(std::uint32_t* reserve, std::uintptr_t bufferLabel, std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg) {
    if (reserve[0] != PrepareFlipHeader) throw std::runtime_error("GnmDriver: the command buffer does not end with a flip reservation");
    const std::uint32_t kind = reserve[1];
    const std::uint32_t labelLow = reserve[2];
    const std::uint32_t labelHigh = reserve[3];
    const std::uint32_t labelValue = reserve[4];
    const std::uint32_t eventType = reserve[5];
    const std::uint32_t cacheAction = reserve[6];
    if (kind != PrepareFlip && kind != PrepareFlipLabel && kind != PrepareFlipInterrupt && kind != PrepareFlipInterruptLabel) {
        throw std::runtime_error("GnmDriver: unknown flip reservation kind " + std::to_string(kind));
    }
    auto* at = Emit(reserve, OpWriteData, {WriteDataToMemory, static_cast<std::uint32_t>(bufferLabel), static_cast<std::uint32_t>(bufferLabel >> 32), 1});
    const auto argument = static_cast<std::uint64_t>(flipArg);
    *at++ = AgcDriver::FlipPacketHeader;
    *at++ = handle;
    *at++ = index;
    *at++ = flipMode;
    *at++ = static_cast<std::uint32_t>(argument);
    *at++ = static_cast<std::uint32_t>(argument >> 32);
    const std::uint32_t eventControl = (eventType & 0x3fu) + EopEventIndex + (cacheAction & 0x3fu) * 0x1000u;
    if (kind == PrepareFlipLabel) at = Emit(at, OpWriteData, {WriteDataToMemory, labelLow & ~3u, labelHigh, labelValue});
    if (kind == PrepareFlipInterruptLabel) at = Emit(at, OpEventWriteEop, {eventControl, labelLow & ~3u, (labelHigh & 0xffffu) | EopData32InterruptAfterWrite, labelValue, 0});
    if (kind == PrepareFlipInterrupt) at = Emit(at, OpEventWriteEop, {eventControl, 0, EopInterruptOnly, 0, 0});
    Nop(at, static_cast<std::uint32_t>(reserve + FlipReserveWords - at) - 1);
}

}
