#include "VulkanTestDevice.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver.hpp"
#include "prx/libSceAgcDriver/Submit/include/Acb.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t CounterOffset = 0x20;
constexpr std::uint32_t ShadowOffset = 0x24;
constexpr std::uint32_t GatedOffset = 0x30;
constexpr std::uint32_t RaceOffset = 0x40;
constexpr std::uint32_t SlotOffset = 0x50;
constexpr std::uint32_t ChainOffset = 0x60;
constexpr std::uint32_t ChainLinks = 8;
constexpr std::uint32_t Untouched = 0xdeadbeefu;
constexpr std::uint32_t Rounds = 4;
constexpr std::uint32_t RaceAppends = 16;
constexpr std::array<std::uint32_t, 2> Queues{0x20, 0x28};

constexpr std::array<std::uint32_t, 10> AppendCode(std::uint32_t offset) {
    return {0xf4080200u, 0xfa000000u, 0xbefc03ffu, 0x0000ffffu, 0xd8fa0000u | offset, 0x00000000u, 0xbf8cc07fu, 0xe0700000u, 0x80020000u, 0xbf810000u};
}

alignas(256) std::array<std::uint32_t, 10> CounterCode = AppendCode(CounterOffset);
alignas(256) std::array<std::uint32_t, 10> ShadowCode = AppendCode(ShadowOffset);
alignas(256) std::array<std::uint32_t, 10> GatedCode = AppendCode(GatedOffset);
alignas(256) std::array<std::uint32_t, 10> RaceCode = AppendCode(RaceOffset);
alignas(256) std::array<std::uint32_t, 10> ChainCode = AppendCode(ChainOffset);
alignas(256) std::array<std::uint32_t, 7> ValueCode{0xf4080200u, 0xfa000000u, 0xbf8cc07fu, 0x7e000202u, 0xe0700000u, 0x80020000u, 0xbf810000u};

struct Cell {
    alignas(256) std::array<std::uint32_t, 4> value{};
    alignas(256) std::array<std::uint32_t, 4> descriptor{};
};

struct Sequence {
    Cell appendA;
    Cell counterA;
    Cell staged;
    Cell appendB;
    Cell appendShadow;
    Cell counterB;
    Cell shadow;
    alignas(256) volatile std::uint32_t done = 0;
};

struct Gate {
    Cell appendA;
    Cell appendB;
    Cell counterB;
    Cell appendC;
    Cell counterC;
    alignas(256) volatile std::uint32_t opened = 0;
    alignas(256) volatile std::uint32_t answered = 0;
    alignas(256) volatile std::uint32_t done = 0;
};

struct Race {
    std::array<Cell, RaceAppends> appends;
    std::array<Cell, RaceAppends> slots;
    Cell counter;
    Cell slot;
    alignas(256) volatile std::uint32_t appended = 0;
    alignas(256) volatile std::uint32_t stored = 0;
    alignas(256) volatile std::uint32_t done = 0;
};

struct Chain {
    std::array<Cell, ChainLinks> appends;
    Cell counter;
    alignas(256) volatile std::uint32_t done = 0;
};

std::array<Sequence, Rounds> Sequences;
std::array<Chain, Rounds> Chains;
std::array<Gate, Rounds> Gates;
std::array<Race, Rounds> Races;

std::uint32_t Low(const volatile void* pointer) {
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer));
}

std::uint32_t High(const volatile void* pointer) {
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(pointer)) >> 32u);
}

std::array<std::uint32_t, 4> Descriptor(const void* pointer, std::uint32_t bytes) {
    return {Low(pointer), High(pointer) & 0xffffu, bytes, 0x31016facu};
}

void Prepare(Cell& cell) {
    cell.value.fill(Untouched);
    cell.descriptor = Descriptor(cell.value.data(), sizeof(cell.value));
}

void SetShaderRegister(std::vector<std::uint32_t>& words, std::uint32_t offset, std::uint32_t value) {
    words.insert(words.end(), {0xc0017600u, offset, value});
}

void Dispatch(std::vector<std::uint32_t>& words, const void* code, std::initializer_list<std::uint32_t> userData) {
    const auto address = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(code));
    SetShaderRegister(words, 0x20c, static_cast<std::uint32_t>(address >> 8u));
    SetShaderRegister(words, 0x20d, static_cast<std::uint32_t>(address >> 40u) & 0xffu);
    for (const std::uint32_t offset : {0x207u, 0x208u, 0x209u}) SetShaderRegister(words, offset, 1);
    SetShaderRegister(words, 0x212, 0);
    SetShaderRegister(words, 0x213, static_cast<std::uint32_t>(userData.size()) << 1u);
    std::uint32_t offset = 0x240;
    for (const auto value : userData) SetShaderRegister(words, offset++, value);
    words.insert(words.end(), {0xc0031500u, 1, 1, 1, 0x8041u});
}

void Append(std::vector<std::uint32_t>& words, const std::array<std::uint32_t, 10>& code, const Cell& cell) {
    Dispatch(words, code.data(), {Low(cell.descriptor.data()), High(cell.descriptor.data())});
}

void StoreValue(std::vector<std::uint32_t>& words, const Cell& cell, std::uint32_t value) {
    Dispatch(words, ValueCode.data(), {Low(cell.descriptor.data()), High(cell.descriptor.data()), value});
}

void DmaData(std::vector<std::uint32_t>& words, std::uint32_t control, std::uint32_t sourceLow, std::uint32_t sourceHigh, std::uint32_t destinationLow, std::uint32_t destinationHigh) {
    words.insert(words.end(), {0xc0055000u, control, sourceLow, sourceHigh, destinationLow, destinationHigh, 4u});
}

void SetGds(std::vector<std::uint32_t>& words, std::uint32_t offset, std::uint32_t value) {
    DmaData(words, 0x40100000u, value, 0, offset, 0);
}

void MemoryToGds(std::vector<std::uint32_t>& words, const Cell& source, std::uint32_t offset) {
    DmaData(words, 0x60100000u, Low(source.value.data()), High(source.value.data()), offset, 0);
}

void GdsToGds(std::vector<std::uint32_t>& words, std::uint32_t from, std::uint32_t to) {
    DmaData(words, 0x20100000u, from, 0, to, 0);
}

void GdsToMemory(std::vector<std::uint32_t>& words, std::uint32_t offset, const Cell& target) {
    DmaData(words, 0x20000000u, offset, 0, Low(target.value.data()), High(target.value.data()));
}

void WriteData(std::vector<std::uint32_t>& words, volatile std::uint32_t* target, std::uint32_t value) {
    words.insert(words.end(), {0xc0033700u, 0x00100200u, Low(target), High(target), value});
}

void WaitEqual(std::vector<std::uint32_t>& words, volatile std::uint32_t* target, std::uint32_t value) {
    words.insert(words.end(), {0xc0053c00u, 0x13u, Low(target), High(target), value, 0xffffffffu, 0x19u});
}

void Submit(std::uint32_t queue, std::vector<std::uint32_t>& words) {
    Packet packet{words.data(), static_cast<std::uint32_t>(words.size()), 0, {}};
    Require(sceAgcDriverSubmitAcb(queue, &packet) == 0, "compute submit failed");
}

void Await(const volatile std::uint32_t& flag, const std::string& what) {
    const auto start = std::chrono::steady_clock::now();
    while (flag == 0) {
        Require(std::chrono::steady_clock::now() - start < std::chrono::seconds(10), what + ": the submissions did not finish");
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    AgcDriverWaitIdle_nid_postfix();
}

void Expect(const Cell& cell, std::uint32_t expected, const std::string& what) {
    Require(cell.value[0] == expected, what + " is " + std::to_string(cell.value[0]) + ", expected " + std::to_string(expected));
}

void RunSequence(std::uint32_t round) {
    auto& sequence = Sequences[round];
    const auto queue = Queues[round % 2u];
    const auto startA = 1000u + 100u * round;
    const auto startB = 50000u + 1000u * round;
    for (auto* cell : {&sequence.appendA, &sequence.counterA, &sequence.staged, &sequence.appendB, &sequence.appendShadow, &sequence.counterB, &sequence.shadow}) Prepare(*cell);
    std::vector<std::uint32_t> words;
    SetGds(words, CounterOffset, startA);
    Append(words, CounterCode, sequence.appendA);
    GdsToMemory(words, CounterOffset, sequence.counterA);
    StoreValue(words, sequence.staged, startB);
    MemoryToGds(words, sequence.staged, CounterOffset);
    Append(words, CounterCode, sequence.appendB);
    GdsToGds(words, CounterOffset, ShadowOffset);
    Append(words, ShadowCode, sequence.appendShadow);
    GdsToMemory(words, CounterOffset, sequence.counterB);
    GdsToMemory(words, ShadowOffset, sequence.shadow);
    WriteData(words, &sequence.done, 1);
    Submit(queue, words);
    const auto what = "sequence " + std::to_string(round) + " on queue " + std::to_string(queue);
    Await(sequence.done, what);
    Expect(sequence.appendA, startA, what + ": the append behind an immediate DMA_DATA into the GDS returned");
    Expect(sequence.counterA, startA + 1u, what + ": the GDS counter copied out after that append");
    Expect(sequence.staged, startB, what + ": the value dispatch stored");
    Expect(sequence.appendB, startB, what + ": the append behind a DMA_DATA into the GDS from the value dispatch's output returned");
    Expect(sequence.appendShadow, startB + 1u, what + ": the append on the GDS-to-GDS copy of the counter returned");
    Expect(sequence.counterB, startB + 1u, what + ": the GDS counter copied out at the end");
    Expect(sequence.shadow, startB + 2u, what + ": the GDS copy copied out at the end");
}

void RunGate(std::uint32_t round) {
    auto& gate = Gates[round];
    const auto first = Queues[round % 2u];
    const auto second = Queues[(round + 1u) % 2u];
    const auto startA = 7000u + 10u * round;
    const auto startB = 90000u + 1000u * round;
    for (auto* cell : {&gate.appendA, &gate.appendB, &gate.counterB, &gate.appendC, &gate.counterC}) Prepare(*cell);
    std::vector<std::uint32_t> before;
    SetGds(before, GatedOffset, startA);
    Append(before, GatedCode, gate.appendA);
    WriteData(before, &gate.opened, 1);
    std::vector<std::uint32_t> across;
    WaitEqual(across, &gate.opened, 1);
    SetGds(across, GatedOffset, startB);
    Append(across, GatedCode, gate.appendB);
    GdsToMemory(across, GatedOffset, gate.counterB);
    WriteData(across, &gate.answered, 1);
    std::vector<std::uint32_t> after;
    WaitEqual(after, &gate.answered, 1);
    Append(after, GatedCode, gate.appendC);
    GdsToMemory(after, GatedOffset, gate.counterC);
    WriteData(after, &gate.done, 1);
    Submit(first, before);
    Submit(second, across);
    Submit(first, after);
    const auto what = "gate " + std::to_string(round) + " from queue " + std::to_string(first) + " to queue " + std::to_string(second);
    Await(gate.done, what);
    Expect(gate.appendA, startA, what + ": the first queue's append returned");
    Expect(gate.appendB, startB, what + ": the second queue's append, behind its DMA_DATA into the GDS, returned");
    Expect(gate.counterB, startB + 1u, what + ": the GDS counter the second queue copied out");
    Expect(gate.appendC, startB + 1u, what + ": the first queue's append behind the second queue's label returned");
    Expect(gate.counterC, startB + 2u, what + ": the GDS counter the first queue copied out at the end");
}

std::uint32_t ChainValue(std::uint32_t round, std::uint32_t link) {
    return 300000u + 10000u * round + 100u * link;
}

void RunBackToBack(std::uint32_t round) {
    auto& chain = Chains[round];
    const auto queue = Queues[round % 2u];
    for (auto& cell : chain.appends) Prepare(cell);
    Prepare(chain.counter);
    std::vector<std::uint32_t> words;
    for (std::uint32_t link = 0; link < ChainLinks; ++link) {
        SetGds(words, ChainOffset, ChainValue(round, link));
        Append(words, ChainCode, chain.appends[link]);
    }
    GdsToMemory(words, ChainOffset, chain.counter);
    WriteData(words, &chain.done, 1);
    Submit(queue, words);
    const auto what = "back-to-back " + std::to_string(round) + " on queue " + std::to_string(queue);
    Await(chain.done, what);
    for (std::uint32_t link = 0; link < ChainLinks; ++link) Expect(chain.appends[link], ChainValue(round, link), what + ": append " + std::to_string(link) + ", recorded right before the next DMA_DATA into the GDS, returned");
    Expect(chain.counter, ChainValue(round, ChainLinks - 1u) + 1u, what + ": the GDS counter copied out at the end");
}

std::uint32_t SlotValue(std::uint32_t round, std::uint32_t store) {
    return 0x00c0de00u + round * 0x100u + store;
}

void RunRace(std::uint32_t round) {
    auto& race = Races[round];
    const auto start = 200000u + 1000u * round;
    for (auto& cell : race.appends) Prepare(cell);
    for (auto& cell : race.slots) Prepare(cell);
    Prepare(race.counter);
    Prepare(race.slot);
    std::vector<std::uint32_t> appends;
    SetGds(appends, RaceOffset, start);
    for (const auto& cell : race.appends) Append(appends, RaceCode, cell);
    WriteData(appends, &race.appended, 1);
    std::vector<std::uint32_t> stores;
    for (std::uint32_t store = 0; store < RaceAppends; ++store) {
        SetGds(stores, SlotOffset, SlotValue(round, store));
        GdsToMemory(stores, SlotOffset, race.slots[store]);
    }
    WriteData(stores, &race.stored, 1);
    Submit(Queues[0], appends);
    Submit(Queues[1], stores);
    const auto what = "race " + std::to_string(round);
    Await(race.appended, what + " appends");
    Await(race.stored, what + " stores");
    std::vector<std::uint32_t> finish;
    GdsToMemory(finish, RaceOffset, race.counter);
    GdsToMemory(finish, SlotOffset, race.slot);
    WriteData(finish, &race.done, 1);
    Submit(Queues[0], finish);
    Await(race.done, what + " read-back");
    std::vector<std::uint32_t> returned;
    for (const auto& cell : race.appends) returned.push_back(cell.value[0]);
    std::sort(returned.begin(), returned.end());
    for (std::uint32_t append = 0; append < RaceAppends; ++append) Require(returned[append] == start + append, what + ": the appends returned " + std::to_string(returned[append]) + " where " + std::to_string(start + append) + " was due, while the other queue stored into the GDS");
    for (std::uint32_t store = 0; store < RaceAppends; ++store) Expect(race.slots[store], SlotValue(round, store), what + ": the GDS slot copied out right after DMA_DATA " + std::to_string(store) + " stored into it, while the other queue appended");
    Expect(race.counter, start + RaceAppends, what + ": the GDS counter after every append");
    Expect(race.slot, SlotValue(round, RaceAppends - 1u), what + ": the GDS slot after every store");
}

}

int main(int argc, char** argv) {
    try {
        auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        device.reset();
        const bool race = argc > 1 && std::string(argv[1]) == "race";
        for (std::uint32_t round = 0; round < Rounds; ++round) RunBackToBack(round);
        for (std::uint32_t round = 0; round < Rounds; ++round) RunSequence(round);
        for (std::uint32_t round = 0; round < Rounds; ++round) RunGate(round);
        if (race) {
            for (std::uint32_t round = 0; round < Rounds; ++round) RunRace(round);
        }
        std::cout << "gds dma store order test passed" << (race ? " with the cross-queue race" : "") << (std::getenv("APS5_GDS_DRAIN") != nullptr ? " (GDS stores drained)" : "") << '\n';
        AgcDriverShutdown_nid_postfix();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        try {
            AgcDriverShutdown_nid_postfix();
        } catch (const std::exception& shutdownError) {
            std::cerr << shutdownError.what() << '\n';
        }
        return 1;
    }
}
