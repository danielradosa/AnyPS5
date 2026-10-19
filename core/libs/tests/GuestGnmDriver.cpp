#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceGnmDriver/include/Lowering.hpp"
#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <vector>

extern "C" {
int APS5_VABI sceGnmSetGsRingSizes(std::uint32_t esgsRingSize, std::uint32_t gsvsRingSize);
int APS5_VABI sceGnmSetupMipStatsReport(void* outputBuffer, std::uint32_t sizeInBytes, std::uint8_t intervalsBetweenReports, std::uint8_t numReportsBeforeReset, std::uint32_t mipStatsResetForce);
int APS5_VABI sceGnmRequestFlipAndSubmitDone(std::uint32_t* commands, std::uint32_t sizeInBytes, std::uint32_t handle, std::uint32_t index, std::uint32_t flipMode, std::int64_t flipArg);
}

namespace {

using GnmDriver::Header;

constexpr int GsRingSizeInvalid = 0x08000000;
constexpr int MipStatsInvalidBuffer = static_cast<int>(0x80000001u);
constexpr int MipStatsInvalidSize = static_cast<int>(0x80000002u);
constexpr int SubmissionNotEnoughResources = static_cast<int>(0x80d11001u);
constexpr std::uint32_t OpNop = 0x10;
constexpr std::uint32_t OpWriteData = 0x37;
constexpr std::uint32_t OpEventWriteEop = 0x47;
constexpr std::uint32_t OpEventWriteEos = 0x48;
constexpr std::uint32_t OpReleaseMem = 0x49;
constexpr std::uint32_t OpDmaData = 0x50;
constexpr std::uint32_t OpIncrementCeCounter = 0x84;
constexpr std::uint32_t OpIncrementDeCounter = 0x85;
constexpr std::uint32_t OpWaitOnCeCounter = 0x86;
constexpr std::uint32_t OpWaitOnDeCounterDiff = 0x88;

void Require(bool value) { if (!value) std::abort(); }

bool Throws(const std::function<void()>& call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void Packet(std::vector<std::uint32_t>& words, std::uint32_t opcode, std::initializer_list<std::uint32_t> body) {
    words.push_back(Header(opcode, static_cast<std::uint32_t>(body.size())));
    words.insert(words.end(), body);
}

std::vector<std::uint32_t> Words(std::initializer_list<std::uint32_t> words) {
    return words;
}

void CheckEndOfShader() {
    GnmDriver::EngineCounters counters;
    std::vector<std::uint32_t> dcb;
    Packet(dcb, OpEventWriteEos, {0x0514, 0x1000, (2u << 29) | 0x12, 0xabcd});
    Require(GnmDriver::MergeEngines(dcb, {}, counters) == Words({Header(OpReleaseMem, 7), 0x0514, 1u << 29, 0x1000, 0x12, 0xabcd, 0, 0}));
    dcb.clear();
    Packet(dcb, OpEventWriteEos, {0x2514, 0x2000, (1u << 29) | 0x34, (2u << 16) | 0x40});
    Require(GnmDriver::MergeEngines(dcb, {}, counters) == Words({Header(OpReleaseMem, 7), 0x0514, 0, 0, 0, 0, 0, 0, Header(OpDmaData, 6), (1u << 31) | (1u << 29), 0x40, 0, 0x2000, 0x34, 8}));
    dcb.clear();
    Packet(dcb, OpEventWriteEos, {0x0514, 0x2000, (1u << 29) | 0x34, 0x40});
    Require(Throws([&] { GnmDriver::MergeEngines(dcb, {}, counters); }));
    dcb.clear();
    Packet(dcb, OpEventWriteEos, {0x0514, 0x2000, 0x34, 0x40});
    Require(Throws([&] { GnmDriver::MergeEngines(dcb, {}, counters); }));
}

void CheckEngines() {
    std::vector<std::uint32_t> ccb;
    Packet(ccb, OpIncrementCeCounter, {0});
    Packet(ccb, OpWaitOnDeCounterDiff, {1});
    Packet(ccb, OpNop, {0x58});
    Packet(ccb, OpIncrementCeCounter, {0});
    std::vector<std::uint32_t> dcb;
    Packet(dcb, OpWaitOnCeCounter, {0});
    Packet(dcb, OpNop, {0xa});
    Packet(dcb, OpIncrementDeCounter, {0});
    Packet(dcb, OpWaitOnCeCounter, {0});
    Packet(dcb, OpNop, {0xb});
    Packet(dcb, OpIncrementDeCounter, {0});
    GnmDriver::EngineCounters counters;
    Require(GnmDriver::MergeEngines(dcb, ccb, counters) == Words({Header(OpNop, 1), 0xa, Header(OpNop, 1), 0x58, Header(OpNop, 1), 0xb}));
    Require(counters.constant == 2 && counters.draw == 2);

    ccb.clear();
    Packet(ccb, OpWaitOnDeCounterDiff, {1});
    Packet(ccb, OpNop, {0x58});
    Packet(ccb, OpIncrementCeCounter, {0});
    Packet(ccb, OpIncrementCeCounter, {0});
    dcb.clear();
    Packet(dcb, OpIncrementDeCounter, {0});
    Packet(dcb, OpWaitOnCeCounter, {0});
    Packet(dcb, OpNop, {0xb});
    counters = {};
    Require(GnmDriver::MergeEngines(dcb, ccb, counters) == Words({Header(OpNop, 1), 0x58, Header(OpNop, 1), 0xb}));
    Require(counters.constant == 2 && counters.draw == 1);

    ccb.clear();
    Packet(ccb, OpIncrementCeCounter, {0});
    Packet(ccb, OpWaitOnDeCounterDiff, {1});
    Packet(ccb, OpNop, {0x58});
    dcb.clear();
    Packet(dcb, OpNop, {0xa});
    counters = {};
    Require(Throws([&] { GnmDriver::MergeEngines(dcb, ccb, counters); }));
}

std::vector<std::uint32_t> Reservation(std::uint32_t kind) {
    std::vector<std::uint32_t> reserve(GnmDriver::FlipReserveWords, 0);
    reserve[0] = GnmDriver::PrepareFlipHeader;
    reserve[1] = kind;
    reserve[2] = 0x3003;
    reserve[3] = 0x44;
    reserve[4] = 0x55;
    reserve[5] = 0x14;
    reserve[6] = 0x3;
    return reserve;
}

void CheckFlipCommon(const std::vector<std::uint32_t>& reserve) {
    Require(reserve[0] == Header(OpWriteData, 4) && reserve[1] == 0x500 && reserve[2] == 0x9000 && reserve[3] == 0x1 && reserve[4] == 1);
    Require(reserve[5] == AgcDriver::FlipPacketHeader && reserve[6] == 2 && reserve[7] == 1 && reserve[8] == 0 && reserve[9] == 0x89abcdefu && reserve[10] == 0x01234567u);
}

void CheckFlips() {
    constexpr std::uintptr_t label = 0x100009000ull;
    constexpr std::int64_t argument = 0x0123456789abcdefll;
    auto reserve = Reservation(GnmDriver::PrepareFlip);
    GnmDriver::PatchFlip(reserve.data(), label, 2, 1, 0, argument);
    CheckFlipCommon(reserve);
    Require(reserve[11] == Header(OpNop, GnmDriver::FlipReserveWords - 12));

    reserve = Reservation(GnmDriver::PrepareFlipLabel);
    GnmDriver::PatchFlip(reserve.data(), label, 2, 1, 0, argument);
    CheckFlipCommon(reserve);
    Require(reserve[11] == Header(OpWriteData, 4) && reserve[12] == 0x500 && reserve[13] == 0x3000 && reserve[14] == 0x44 && reserve[15] == 0x55);

    reserve = Reservation(GnmDriver::PrepareFlipInterrupt);
    GnmDriver::PatchFlip(reserve.data(), label, 2, 1, 0, argument);
    CheckFlipCommon(reserve);
    Require(reserve[11] == Header(OpEventWriteEop, 5) && reserve[12] == 0x3514 && reserve[13] == 0 && reserve[14] == 0x01000000u && reserve[15] == 0 && reserve[16] == 0);
    Require(reserve[17] == Header(OpNop, GnmDriver::FlipReserveWords - 18));
    GnmDriver::EngineCounters counters;
    auto merged = GnmDriver::MergeEngines(reserve, {}, counters);
    Require(merged.size() == GnmDriver::FlipReserveWords + 2);
    Require(std::vector<std::uint32_t>(merged.begin() + 11, merged.begin() + 19) == Words({Header(OpReleaseMem, 7), 0x0514, 0x01000000u, 0, 0, 0, 0, 0}));

    reserve = Reservation(GnmDriver::PrepareFlipInterruptLabel);
    GnmDriver::PatchFlip(reserve.data(), label, 2, 1, 0, argument);
    CheckFlipCommon(reserve);
    Require(reserve[11] == Header(OpEventWriteEop, 5) && reserve[12] == 0x3514 && reserve[13] == 0x3000 && reserve[14] == (0x22000000u | 0x44) && reserve[15] == 0x55 && reserve[16] == 0);
    merged = GnmDriver::MergeEngines(reserve, {}, counters);
    Require(std::vector<std::uint32_t>(merged.begin() + 11, merged.begin() + 19) == Words({Header(OpReleaseMem, 7), 0x0514, 0x22000000u, 0x3000, 0x44, 0x55, 0, 0}));

    reserve = Reservation(0x68750779u);
    Require(Throws([&] { GnmDriver::PatchFlip(reserve.data(), label, 2, 1, 0, argument); }));
    reserve = Reservation(GnmDriver::PrepareFlip);
    reserve[0] = Header(OpNop, GnmDriver::FlipReserveWords - 1);
    Require(Throws([&] { GnmDriver::PatchFlip(reserve.data(), label, 2, 1, 0, argument); }));
}

}

int main() {
    Require(sceGnmSetGsRingSizes(0x400000, 0x400000) == 0);
    Require(sceGnmSetGsRingSizes(0x800000, 0x500000) == 0);
    Require(sceGnmSetGsRingSizes(0x300000, 0x400000) == GsRingSizeInvalid);
    Require(sceGnmSetGsRingSizes(0x400000, 0x900000) == GsRingSizeInvalid);
    Require(sceGnmSetGsRingSizes(0x480000, 0x400000) == GsRingSizeInvalid);
    Require(sceGnmSetGsRingSizes(0x400000, 0x400001) == GsRingSizeInvalid);
    Require(sceGnmSetGsRingSizes(0, 0) == GsRingSizeInvalid);

    std::vector<std::uint8_t> storage(0x1840 + 0x40, 0xcd);
    const auto base = (reinterpret_cast<std::uintptr_t>(storage.data()) + 0x3f) & ~std::uintptr_t{0x3f};
    auto* buffer = reinterpret_cast<std::uint8_t*>(base);
    Require(sceGnmSetupMipStatsReport(nullptr, 0x1040, 1, 1, 0) == MipStatsInvalidBuffer);
    Require(sceGnmSetupMipStatsReport(buffer + 4, 0x1040, 1, 1, 0) == MipStatsInvalidBuffer);
    Require(sceGnmSetupMipStatsReport(buffer, 0x1000, 1, 1, 0) == MipStatsInvalidSize);
    Require(sceGnmSetupMipStatsReport(buffer, 0x1080, 1, 1, 0) == MipStatsInvalidSize);
    Require(sceGnmSetupMipStatsReport(buffer, 0x840, 1, 1, 0) == MipStatsInvalidSize);
    Require(buffer[0] == 0xcd && buffer[0x103f] == 0xcd);
    Require(sceGnmSetupMipStatsReport(buffer, 0x1840, 4, 2, 1) == 0);
    std::uint32_t header = 0;
    std::memcpy(&header, buffer, sizeof(header));
    Require(header == 0x40);
    for (std::uint32_t at = 4; at < 0x1840; ++at) Require(buffer[at] == 0);
    Require(buffer[0x1840] == 0xcd);
    Require(sceGnmSetupMipStatsReport(buffer, 0x1040, 1, 1, 0) == 0);

    std::vector<std::uint32_t> flip(GnmDriver::FlipReserveWords - 1, 0xeeeeeeeeu);
    Require(sceGnmRequestFlipAndSubmitDone(flip.data(), static_cast<std::uint32_t>(flip.size() * 4), 1, 0, 0, 0) == SubmissionNotEnoughResources);
    Require(flip[0] == 0xeeeeeeeeu);

    CheckEndOfShader();
    CheckEngines();
    CheckFlips();
}
