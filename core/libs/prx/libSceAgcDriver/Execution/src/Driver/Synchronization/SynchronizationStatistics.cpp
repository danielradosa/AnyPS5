#include "prx/libSceAgcDriver/Execution/include/ProfileOutput.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Synchronization/SynchronizationStatistics.hpp"
#include "prx/libSceAgcDriver/Execution/include/Pm4.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Recorder.hpp"
#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

namespace AgcDriver::DriverDetail {

std::atomic<std::uint64_t> gpuLabels{0}, completionLabels{0}, noOpLabels{0}, unlockedDrains{0};

std::atomic<std::uint64_t> labelFallbacks[5] = {};

std::atomic<std::uint64_t> queuedLabels{0}, labelGroups{0}, immediateLabels{0};

std::atomic<std::uint64_t> packetLockRecords{0}, packetLockSubmits{0}, packetLockDeferred{0}, packetSubmitDeferred{0}, captureTryRecords{0}, captureRetries{0}, suspendPoints{0};

std::atomic<std::uint64_t> recordTriesSkipped{0};

std::atomic<bool> queue0Dormant{false};

std::atomic<std::uint64_t> packetLockRecordUs{0}, packetLockSubmitUs{0};

std::atomic<std::uint64_t> triesFailed[TrySites] = {};

std::atomic<std::uint64_t> storesOnGpu{0}, storesBehindCompletions{0}, storesOnCpu{0}, storesDrained{0}, dmaFills{0}, copiesToHostMemory{0}, gdsStoresOrdered{0}, gdsStoresOnGpu{0};

std::atomic<std::uint64_t> storeDrainReasons[4] = {};

std::atomic<std::uint64_t> pendingSourceDmas[6] = {};

std::map<std::uint32_t, std::uint64_t> drainCounts;

std::uint64_t drainTotal = 0;

std::chrono::steady_clock::time_point lastSyncReport = std::chrono::steady_clock::now();

std::atomic<std::uint64_t> pollLabelRecords{0}, hookLabelRecords{0};

namespace {

struct DmaChainKey {
    std::uint32_t queue, opcode, source, destination;
    std::uint64_t from, to;
    std::uint32_t bytes;
    auto operator<=>(const DmaChainKey&) const = default;
};

struct DmaChainCounts {
    std::uint64_t count = 0, pendingSource = 0, pendingDestination = 0;
};

std::mutex dmaChainMutex;
std::map<DmaChainKey, DmaChainCounts> dmaChain;

bool dmaChainEnabled() {
    static const bool enabled = std::getenv("APS5_PROFILE_DMA_CHAIN") != nullptr;
    return enabled;
}

void reportDmaChain() {
    if (!dmaChainEnabled()) return;
    std::vector<std::pair<DmaChainKey, DmaChainCounts>> entries;
    {
        std::lock_guard lock(dmaChainMutex);
        entries.assign(dmaChain.begin(), dmaChain.end());
        dmaChain.clear();
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.second.count > b.second.count; });
    static constexpr const char* selects[4] = {"mem", "gds", "data", "mem3"};
    std::string report = "[dmachain] " + std::to_string(entries.size()) + " distinct (10 s; queue packet source -> destination bytes x count, pending source/destination):";
    char text[256];
    for (std::size_t index = 0; index < entries.size() && index < 48; ++index) {
        const auto& [key, counts] = entries[index];
        if (key.opcode == 0x50) {
            std::snprintf(text, sizeof(text), " [0x%x DMA %s:%llx -> %s:%llx %u B x%llu p%llu/%llu]", key.queue, selects[key.source & 3u], static_cast<unsigned long long>(key.from), key.destination == Pm4::DmaDestinationNowhere ? "none" : selects[key.destination & 3u], static_cast<unsigned long long>(key.to), key.bytes, static_cast<unsigned long long>(counts.count), static_cast<unsigned long long>(counts.pendingSource), static_cast<unsigned long long>(counts.pendingDestination));
        } else {
            std::snprintf(text, sizeof(text), " [0x%x WAIT%s mem:%llx x%llu p%llu]", key.queue, key.opcode == 0x3c ? "" : "64", static_cast<unsigned long long>(key.from), static_cast<unsigned long long>(counts.count), static_cast<unsigned long long>(counts.pendingSource));
        }
        report += text;
    }
    AgcDriver::ProfilePrint_nid_no_patch("%s\n", report.c_str());
}

}

void noteDmaChain(std::uint32_t queue, std::uint32_t opcode, std::span<const std::uint32_t> packet) {
    if (!dmaChainEnabled()) return;
    DmaChainKey key{};
    key.queue = queue;
    key.opcode = opcode;
    const auto pending = Graphics::Recorder::PendingWriteSnapshot();
    bool pendingSource = false, pendingDestination = false;
    if (opcode == 0x50 && packet.size() >= 7) {
        key.source = Pm4::DmaSource(packet);
        key.destination = Pm4::DmaDestination(packet);
        key.bytes = static_cast<std::uint32_t>(Pm4::DmaBytes(packet));
        const auto span = std::max<std::size_t>(key.bytes, 1);
        if (key.source == Pm4::DmaSourceData) key.from = packet[2];
        else {
            key.from = key.source == Pm4::DmaSelectGds ? packet[2] : packet[2] | (static_cast<std::uint64_t>(packet[3]) << 32u);
            pendingSource = Graphics::Recorder::SnapshotOverlaps(pending.get(), key.source == Pm4::DmaSelectGds ? Pm4::GdsAddress() + key.from : key.from, span);
        }
        if (key.destination != Pm4::DmaDestinationNowhere) {
            key.to = key.destination == Pm4::DmaSelectGds ? packet[4] : packet[4] | (static_cast<std::uint64_t>(packet[5]) << 32u);
            pendingDestination = Graphics::Recorder::SnapshotOverlaps(pending.get(), key.destination == Pm4::DmaSelectGds ? Pm4::GdsAddress() + key.to : key.to, span);
        }
    } else if ((opcode == 0x3c || opcode == 0x93) && packet.size() >= 4 && (packet[1] & 0x10u) != 0) {
        key.from = (packet[2] & ~3u) | (static_cast<std::uint64_t>(packet[3]) << 32u);
        pendingSource = Graphics::Recorder::SnapshotOverlaps(pending.get(), key.from, opcode == 0x93 ? 8 : 4);
    } else {
        return;
    }
    std::lock_guard lock(dmaChainMutex);
    auto& counts = dmaChain[key];
    ++counts.count;
    counts.pendingSource += pendingSource ? 1 : 0;
    counts.pendingDestination += pendingDestination ? 1 : 0;
}

SubmissionCosts& submissionCosts(std::uint32_t queue) {
    static std::array<SubmissionCosts, 64> costs;
    return costs[queue == 0 ? 0 : std::min<std::uint32_t>(queue - 0x20 + 1, 63)];
}

void reportSync() {
    std::string report;
    for (const auto& [code, count] : drainCounts) report += " " + (code == 0xffffu ? std::string("flip") : Pm4::Name(code << 8u)) + "=" + std::to_string(count);
    AgcDriver::ProfilePrint_nid_no_patch("[sync] %llu device drains by packet:%s (%llu waited without the GPU mutex); %llu labels written on the GPU, %llu deferred behind completions, %llu no-op, fallbacks: idle %llu, completions %llu, not imported %llu, undecodable %llu; %llu labels queued per worker, recorded in %llu groups, %llu recorded at once; stores: %llu recorded on the GPU, %llu behind completions, %llu on the CPU (idle), %llu synced (drained: GDS %llu, not imported %llu, refused %llu, not a store %llu), %llu DMA fills stored as buffer fills, into the GDS: %llu recorded on the GPU, %llu on the CPU behind its GPU work; DMA_DATA over a pending source (memory/GDS source): to memory %llu/%llu, to the GDS %llu/%llu, nowhere %llu/%llu; %llu copies into host memory synced on their source\n", static_cast<unsigned long long>(drainTotal), report.c_str(), static_cast<unsigned long long>(unlockedDrains.load()), static_cast<unsigned long long>(gpuLabels.load()), static_cast<unsigned long long>(completionLabels.load()), static_cast<unsigned long long>(noOpLabels.load()), static_cast<unsigned long long>(labelFallbacks[1].load()), static_cast<unsigned long long>(labelFallbacks[2].load()), static_cast<unsigned long long>(labelFallbacks[3].load()), static_cast<unsigned long long>(labelFallbacks[4].load()), static_cast<unsigned long long>(queuedLabels.load()), static_cast<unsigned long long>(labelGroups.load()), static_cast<unsigned long long>(immediateLabels.load()), static_cast<unsigned long long>(storesOnGpu.load()), static_cast<unsigned long long>(storesBehindCompletions.load()), static_cast<unsigned long long>(storesOnCpu.load()), static_cast<unsigned long long>(storesDrained.load()), static_cast<unsigned long long>(storeDrainReasons[0].load()), static_cast<unsigned long long>(storeDrainReasons[1].load()), static_cast<unsigned long long>(storeDrainReasons[2].load()), static_cast<unsigned long long>(storeDrainReasons[3].load()), static_cast<unsigned long long>(dmaFills.load()), static_cast<unsigned long long>(gdsStoresOnGpu.load()), static_cast<unsigned long long>(gdsStoresOrdered.load()), static_cast<unsigned long long>(pendingSourceDmas[0].load()), static_cast<unsigned long long>(pendingSourceDmas[1].load()), static_cast<unsigned long long>(pendingSourceDmas[2].load()), static_cast<unsigned long long>(pendingSourceDmas[3].load()), static_cast<unsigned long long>(pendingSourceDmas[4].load()), static_cast<unsigned long long>(pendingSourceDmas[5].load()), static_cast<unsigned long long>(copiesToHostMemory.load()));
    reportDmaChain();
}

void countLabelOutcome(int reason) {
    if (reason == 0) ++gpuLabels;
    else if (reason == 5) ++completionLabels;
    else ++labelFallbacks[std::min(reason, 4)];
}

}
