#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_SYNCHRONIZATIONSTATISTICS_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_SYNCHRONIZATIONSTATISTICS_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>

namespace AgcDriver::DriverDetail {

struct SubmissionCosts {
    std::atomic<std::uint64_t> submissions{0}, validateNs{0}, copyNs{0}, dequeueNs{0}, completeNs{0}, suspendNs{0}, suspends{0}, suspendsSkipped{0}, endSubmits{0}, endSkipped{0}, notifiesSkipped{0};
};

extern std::atomic<std::uint64_t> gpuLabels, completionLabels, noOpLabels, unlockedDrains;

extern std::atomic<std::uint64_t> labelFallbacks[5];

extern std::atomic<std::uint64_t> queuedLabels, labelGroups, immediateLabels;

extern std::atomic<std::uint64_t> packetLockRecords, packetLockSubmits, packetLockDeferred, packetSubmitDeferred, captureTryRecords, captureRetries, suspendPoints;

extern std::atomic<std::uint64_t> recordTriesSkipped;

extern std::atomic<bool> queue0Dormant;

extern std::atomic<std::uint64_t> packetLockRecordUs, packetLockSubmitUs;

enum TrySite { TryLabel = 0, TryFlush, TryReap, TryPoll, TryIdle, TryCapture, TryPollRecord, TrySites };

extern std::atomic<std::uint64_t> triesFailed[TrySites];

extern std::atomic<std::uint64_t> storesOnGpu, storesBehindCompletions, storesOnCpu, storesDrained, dmaFills, copiesToHostMemory, gdsStoresOrdered;

// Drained stores by why they could not go on the GPU: 0 a DMA_DATA into the GDS (APS5_GDS_DRAIN=1), 1 a
// destination without a host import, 2 another WriteLabelOnGpu refusal, 3 not resolvable as a store
// (misaligned, over 64 KiB, a register destination, a refused copy).
extern std::atomic<std::uint64_t> storeDrainReasons[4];

// DMA_DATA whose source has recorded GPU writes pending: index destination (0 memory, 2 GDS, 4 nowhere) + source (0
// memory, 1 GDS).
extern std::atomic<std::uint64_t> pendingSourceDmas[6];

extern std::map<std::uint32_t, std::uint64_t> drainCounts;

extern std::uint64_t drainTotal;

extern std::chrono::steady_clock::time_point lastSyncReport;

extern std::atomic<std::uint64_t> pollLabelRecords, hookLabelRecords;

SubmissionCosts& submissionCosts(std::uint32_t queue);

void reportSync();

void countLabelOutcome(int reason);

}

#endif
