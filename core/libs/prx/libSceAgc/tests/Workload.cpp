#include "SceTypes.hpp"
#include "prx/libSceAgcDriver/Resource/include/Registration.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <vector>

extern "C" {
std::uint32_t* APS5_VABI sceAgcDcbSetWorkloadsActive(CommandBuffer*, std::uint32_t, const std::uint32_t*, std::uint32_t);
std::uint32_t* APS5_VABI sceAgcDcbSetWorkloadComplete(CommandBuffer*, std::uint32_t, std::uint32_t);
std::uint32_t* APS5_VABI sceAgcDcbSetWorkloadStreamInactive(CommandBuffer*, std::uint32_t);
std::uint32_t* APS5_VABI sceAgcAcbSetWorkloadsActive(CommandBuffer*, std::uint32_t, const std::uint32_t*, std::uint32_t);
std::uint32_t* APS5_VABI sceAgcAcbSetWorkloadComplete(CommandBuffer*, std::uint32_t, std::uint32_t);
std::uint32_t* APS5_VABI sceAgcAcbSetWorkloadStreamInactive(CommandBuffer*, std::uint32_t);
int APS5_VABI sceAgcDriverRegisterWorkloadStream(std::uint32_t, const void*);
int APS5_VABI sceAgcDriverUnregisterWorkloadStream(std::uint32_t);
}

namespace {

constexpr std::uint32_t Sentinel = 0xabcdef01u;
constexpr int InvalidValue = static_cast<int>(0x8A6C0033u);
constexpr int InvalidArgument = static_cast<int>(0x8A6C0035u);
constexpr int NotRegistered = static_cast<int>(0x8A6C003Au);
const std::array<std::uint8_t, 32> Descriptor{'s', 't', 'r', 'e', 'a', 'm'};

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename TAction>
void expectFailure(TAction action) {
    try {
        action();
    } catch (const std::runtime_error&) {
        return;
    }
    throw std::runtime_error("expected the builder to fail");
}

struct Storage {
    std::array<std::uint32_t, 64> words{};
    CommandBuffer buffer{words.data(), words.data() + words.size(), words.data(), words.data() + words.size(), nullptr, nullptr, 0};

    Storage() {
        words.fill(Sentinel);
    }

    void limitTo(std::uint32_t count) {
        buffer.cursor_down = buffer.cursor_up + count;
    }

    bool untouched() const {
        return buffer.cursor_up == words.data() && std::all_of(words.begin(), words.end(), [](std::uint32_t word) { return word == Sentinel; });
    }
};

template <std::size_t N>
void expectPacket(const std::uint32_t* packet, const Storage& storage, const std::array<std::uint32_t, N>& expected, const char* message) {
    check(packet == storage.words.data() && storage.buffer.cursor_up == packet + N, message);
    check(std::equal(expected.begin(), expected.end(), packet), message);
    check(std::all_of(storage.words.begin() + N, storage.words.end(), [](std::uint32_t word) { return word == Sentinel; }), message);
}

std::uint32_t low(std::uint64_t value) { return static_cast<std::uint32_t>(value); }
std::uint32_t high(std::uint64_t value) { return static_cast<std::uint32_t>(value >> 32u); }

void expectNothingWritten(std::uint32_t streamId) {
    const std::array<std::uint32_t, 1> ids{1};
    const std::array builders{
        +[](CommandBuffer* buffer, std::uint32_t stream, const std::uint32_t* workloads) { return sceAgcDcbSetWorkloadsActive(buffer, stream, workloads, 1); },
        +[](CommandBuffer* buffer, std::uint32_t stream, const std::uint32_t*) { return sceAgcDcbSetWorkloadComplete(buffer, stream, 1); },
        +[](CommandBuffer* buffer, std::uint32_t stream, const std::uint32_t*) { return sceAgcDcbSetWorkloadStreamInactive(buffer, stream); },
        +[](CommandBuffer* buffer, std::uint32_t stream, const std::uint32_t* workloads) { return sceAgcAcbSetWorkloadsActive(buffer, stream, workloads, 1); },
        +[](CommandBuffer* buffer, std::uint32_t stream, const std::uint32_t*) { return sceAgcAcbSetWorkloadComplete(buffer, stream, 1); },
        +[](CommandBuffer* buffer, std::uint32_t stream, const std::uint32_t*) { return sceAgcAcbSetWorkloadStreamInactive(buffer, stream); },
    };
    for (const auto builder : builders) {
        Storage storage;
        check(builder(&storage.buffer, streamId, ids.data()) == nullptr && storage.untouched(), "a builder for an unregistered or invalid stream wrote a packet");
    }
}

void testUnregisteredStreams() {
    for (const auto stream : {0u, 1u, 7u, 31u, 32u, 0x11111111u}) expectNothingWritten(stream);
}

void testRegistration() {
    check(sceAgcDriverRegisterWorkloadStream(0, Descriptor.data()) == InvalidValue, "stream 0 registered");
    check(sceAgcDriverRegisterWorkloadStream(32, Descriptor.data()) == InvalidValue, "stream 32 registered");
    check(sceAgcDriverRegisterWorkloadStream(5, nullptr) == InvalidArgument, "a null stream descriptor registered");
    check(AgcDriverWorkloadStreamSlot_nid_no_patch(5) == 0, "a failed registration made the stream usable");
    check(sceAgcDriverRegisterWorkloadStream(5, Descriptor.data()) == 0, "stream 5 did not register");
    check(sceAgcDriverRegisterWorkloadStream(5, Descriptor.data()) == InvalidValue, "stream 5 registered twice");
    check(sceAgcDriverRegisterWorkloadStream(5, nullptr) == InvalidValue, "a duplicate registration was not refused before the descriptor check");
    check(sceAgcDriverUnregisterWorkloadStream(0) == InvalidValue && sceAgcDriverUnregisterWorkloadStream(32) == InvalidValue, "an invalid stream unregistered");
    check(sceAgcDriverUnregisterWorkloadStream(6) == NotRegistered, "an unregistered stream unregistered");
    check(sceAgcDriverRegisterWorkloadStream(4, Descriptor.data()) == 0 && sceAgcDriverRegisterWorkloadStream(6, Descriptor.data()) == 0, "streams 4 and 6 did not register");
    const auto slot4 = AgcDriverWorkloadStreamSlot_nid_no_patch(4);
    const auto slot5 = AgcDriverWorkloadStreamSlot_nid_no_patch(5);
    const auto slot6 = AgcDriverWorkloadStreamSlot_nid_no_patch(6);
    check(slot4 != 0 && slot4 % 8 == 0 && slot5 == slot4 + 8 && slot6 == slot5 + 8, "stream slots are not consecutive 8-byte slots");
    check(*reinterpret_cast<const std::uint64_t*>(slot5) == ~0ull, "an unused stream slot is not all ones");
    check(sceAgcDriverUnregisterWorkloadStream(5) == 0, "stream 5 did not unregister");
    check(sceAgcDriverUnregisterWorkloadStream(5) == NotRegistered, "stream 5 unregistered twice");
    check(AgcDriverWorkloadStreamSlot_nid_no_patch(5) == 0, "an unregistered stream kept its slot");
    expectNothingWritten(5);
    check(sceAgcDriverRegisterWorkloadStream(5, Descriptor.data()) == 0 && AgcDriverWorkloadStreamSlot_nid_no_patch(5) == slot5, "a stream registered again moved its slot");
    for (const auto stream : {4u, 5u, 6u}) check(sceAgcDriverUnregisterWorkloadStream(stream) == 0, "stream did not unregister");
}

void testDrawBuilders(std::uint64_t slot) {
    const std::array<std::uint32_t, 1> ids{1};
    Storage active;
    expectPacket(sceAgcDcbSetWorkloadsActive(&active.buffer, 1, ids.data(), 1), active, std::array<std::uint32_t, 18>{
        0xc0027904u, 0x342u, 0xcc000001u, 2u, 0xc0033704u, 0x06010000u, 0xc343u, 0u, 0u,
        0xc0071e00u, 0x40000267u, low(slot), high(slot), 2u, 0u, 0u, 0u, 0u}, "DCB SetWorkloadsActive bytes differ from OpenAGC's");
    Storage complete;
    expectPacket(sceAgcDcbSetWorkloadComplete(&complete.buffer, 1, 1), complete, std::array<std::uint32_t, 12>{
        0xc0017904u, 0x342u, 0xcd000021u, 0xc0071e00u, 0x40000275u, low(slot), high(slot), 0xfffffffdu, 0xffffffffu, 0u, 0u, 0u}, "DCB SetWorkloadComplete bytes differ from OpenAGC's");
    Storage inactive;
    expectPacket(sceAgcDcbSetWorkloadStreamInactive(&inactive.buffer, 1), inactive, std::array<std::uint32_t, 9>{
        0xc0027904u, 0x342u, 0xcc000001u, 0u, 0xc0033704u, 0x06010000u, 0xc343u, 0u, 0u}, "DCB SetWorkloadStreamInactive bytes differ from OpenAGC's");
}

void testComputeBuilders(std::uint64_t slot) {
    const std::array<std::uint32_t, 2> ids{1, 63};
    Storage active;
    expectPacket(sceAgcAcbSetWorkloadsActive(&active.buffer, 1, ids.data(), 2), active, std::array<std::uint32_t, 18>{
        0xc0027900u, 0x342u, 0xcc000001u, 2u, 0xc0033700u, 0x06010000u, 0xc343u, 0x80000000u, 0u,
        0xc0071e00u, 0x267u, low(slot), high(slot), 2u, 0x80000000u, 0u, 0u, 0u}, "ACB SetWorkloadsActive bytes differ from OpenAGC's");
    Storage complete;
    expectPacket(sceAgcAcbSetWorkloadComplete(&complete.buffer, 1, 63), complete, std::array<std::uint32_t, 12>{
        0xc0017900u, 0x342u, 0xcd0007e1u, 0xc0071e00u, 0x275u, low(slot), high(slot), 0xffffffffu, 0x7fffffffu, 0u, 0u, 0u}, "ACB SetWorkloadComplete bytes differ from OpenAGC's");
    Storage inactive;
    expectPacket(sceAgcAcbSetWorkloadStreamInactive(&inactive.buffer, 1), inactive, std::array<std::uint32_t, 9>{
        0xc0027900u, 0x342u, 0xcc000001u, 0u, 0xc0033700u, 0x06010000u, 0xc343u, 0u, 0u}, "ACB SetWorkloadStreamInactive bytes differ from OpenAGC's");
}

void testValidation() {
    std::vector<std::uint32_t> all(64);
    std::iota(all.begin(), all.end(), 0u);
    const auto refused = [](auto build) {
        Storage storage;
        check(build(&storage.buffer) == nullptr && storage.untouched(), "an invalid workload list wrote a packet");
    };
    for (const auto active : {sceAgcDcbSetWorkloadsActive, sceAgcAcbSetWorkloadsActive}) {
        const std::array<std::uint32_t, 2> repeated{3, 3};
        const std::array<std::uint32_t, 2> outOfRange{3, 64};
        refused([&](CommandBuffer* buffer) { return active(buffer, 1, repeated.data(), 2); });
        refused([&](CommandBuffer* buffer) { return active(buffer, 1, outOfRange.data(), 2); });
        refused([&](CommandBuffer* buffer) { return active(buffer, 1, all.data(), 0); });
        refused([&](CommandBuffer* buffer) { return active(buffer, 1, all.data(), 64); });
        refused([&](CommandBuffer* buffer) { return active(buffer, 1, nullptr, 1); });
        Storage storage;
        const auto* packet = active(&storage.buffer, 1, all.data() + 1, 63);
        check(packet != nullptr && packet[3] == 0xfffffffeu && packet[13] == 0xfffffffeu && packet[14] == 0xffffffffu, "63 workloads did not form the full mask");
        Storage first;
        check(active(&first.buffer, 1, all.data(), 1)[13] == 1u, "workload 0 is not bit 0");
        Storage shortBuffer;
        shortBuffer.limitTo(17);
        expectFailure([&] { active(&shortBuffer.buffer, 1, all.data(), 1); });
        check(shortBuffer.untouched(), "a failed SetWorkloadsActive wrote words");
    }
    for (const auto complete : {sceAgcDcbSetWorkloadComplete, sceAgcAcbSetWorkloadComplete}) {
        refused([&](CommandBuffer* buffer) { return complete(buffer, 1, 64); });
        Storage storage;
        check(complete(&storage.buffer, 1, 0)[7] == 0xfffffffeu, "workload 0 complete does not clear bit 0");
    }
}

}

int main() {
    try {
        testUnregisteredStreams();
        testRegistration();
        check(sceAgcDriverRegisterWorkloadStream(1, Descriptor.data()) == 0, "stream 1 did not register");
        const auto slot = AgcDriverWorkloadStreamSlot_nid_no_patch(1);
        check(slot != 0 && *reinterpret_cast<const std::uint64_t*>(slot) == 0, "stream 1's slot is not the zeroed query slot");
        testDrawBuilders(slot);
        testComputeBuilders(slot);
        testValidation();
        check(sceAgcDriverUnregisterWorkloadStream(1) == 0, "stream 1 did not unregister");
        expectNothingWritten(1);
        std::puts("AGC workload stream builder tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
