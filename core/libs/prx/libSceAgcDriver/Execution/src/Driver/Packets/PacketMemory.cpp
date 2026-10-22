#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Diagnostics.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Draw/DrawPipeline.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Synchronization/SynchronizationStatistics.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Synchronization/DeferredLabels.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/Pm4.hpp"
#include "prx/libSceAgcDriver/Eq/include/Event.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DepthSurface.hpp"
#include "prx/libSceAgcDriver/Graphics/include/GuestBufferMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Recorder.hpp"
#include "prx/libSceAgcDriver/Graphics/include/MultisampleTarget.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Texture.hpp"
#include "prx/libSceAgcDriver/Graphics/include/UnitShadow.hpp"
#include "prx/libSceAgcDriver/Resource/include/Registration.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace AgcDriver::DriverDetail {

bool Driver::preparePacketMemory(const Submission& submission, QueueState& queue, std::span<const std::uint32_t> packet, std::uint32_t header, std::uint32_t opcode, bool& wroteOnGpu, bool& endOfPipeInterrupt, bool& interruptDeferred, bool& drawPacket, bool& sampleDump) {
    static const bool drainAll = std::getenv("APS5_DRAIN_ALL") != nullptr;
    static const bool profile = std::getenv("APS5_PROFILE_DRAW") != nullptr;
    wroteOnGpu = false;

    bool orderedAlready = false;

    const auto interruptSelect = opcode == 0x49 ? (packet[2] >> 24u) & 7u : 0u;
    endOfPipeInterrupt = interruptSelect != 0 && interruptSelect != 3;
    interruptDeferred = false;
    if (!drainAll && endOfPipeInterrupt) {
        const auto label = Pm4::DecodeLabelWrite(packet);
        const bool storesNothing = (packet[2] >> 29u) == 0 || (packet[3] | (static_cast<std::uint64_t>(packet[4]) << 32u)) == 0;
        if (label.has_value() || storesNothing) {
            // A GPU label write orders nothing the CPU wrote into this queue (later waits and
            // submissions bump); keeping the epoch lets the draws until then reuse their page
            // walks. Debug aid: APS5_LABEL_EPOCH=1 bumps here as before.
            static const bool labelEpoch = std::getenv("APS5_LABEL_EPOCH") != nullptr;
            if (labelEpoch) bumpEpoch(&EpochBumps::drains);
            GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Label);
            std::lock_guard gpuLock(GuestMemory::GpuMutex());
            const auto localDevice = device.Load();
            recordDeferredLabels(localDevice.get(), submission.queue);
            const bool workOpen = Graphics::Recorder::RecordedWorkSinceSubmit() != 0;
            int reason = localDevice != nullptr ? 0 : 1;
            if (label.has_value()) {
                const auto bytes = label->Bytes();
                const auto stamp = ++eventSerial;
                reason = localDevice != nullptr ? localDevice->WriteLabelOnGpu(label->address, bytes, stamp, submission.queue) : 4;
                if (reason == 1) GuestMemory::Write(label->address, bytes, 4);
                if (reason == 0 || reason == 1 || reason == 5) {
                    noteLabelStore(label->address, bytes, stamp, submission.queue);
                    Graphics::Recorder::CloseLabelGroup(GuestMemory::TrackerGeneration());
                }
                countLabelOutcome(reason);
                ++immediateLabels;
            } else {
                ++noOpLabels;
            }
            if (reason == 0 || reason == 5) {
                const auto queueId = submission.queue;
                interruptDeferred = localDevice->AfterRecordedWork([this, queueId] { deliverGuestEopInterrupt(queueId); }, submission.queue == 0);
                if (interruptDeferred && workOpen) localDevice->SubmitRecorded(submission.queue == 0);
                wroteOnGpu = true;
            } else if (reason == 1) {
                wroteOnGpu = true;
            }
        }
    }
    if (opcode == 0x50 && packet.size() >= 7) {
        if (Pm4::DmaSource(packet) == 0 || Pm4::DmaSource(packet) == 3) Graphics::RequireNoMultisampleTarget(packet[2] | (static_cast<std::uint64_t>(packet[3]) << 32u), packet[6] & 0x3ffffffu, "a DMA copy");
        if (Pm4::DmaSource(packet) == 2 && Pm4::DmaDestination(packet) != 1) {
            const auto address = packet[4] | (static_cast<std::uint64_t>(packet[5]) << 32u);
            Graphics::NoteDepthMetadataFill(address, packet[6] & 0x3ffffffu, packet[2]);
            Graphics::NoteColorMetadataFill(address, packet[6] & 0x3ffffffu, packet[2]);
        }
    }
    if (!drainAll && !endOfPipeInterrupt && (opcode == 0x49 || opcode == 0x37)) {
        if (const auto label = Pm4::DecodeLabelWrite(packet)) {
            const auto bytes = label->Bytes();
            if (DeferLabels() && bytes.size() <= DeferredLabel::Capacity && bytes.size() % 4 == 0 && label->address % 4 == 0) {

                auto& deferred = deferredLabels();
                if (deferred.labels.empty()) deferred.since = std::chrono::steady_clock::now();
                auto& entry = deferred.labels.emplace_back();
                entry.address = label->address;
                entry.size = bytes.size();
                std::memcpy(entry.bytes.data(), bytes.data(), bytes.size());
                ++queuedLabels;
                wroteOnGpu = true;
            } else {
                GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Label);
                std::lock_guard gpuLock(GuestMemory::GpuMutex());
                const auto localDevice = device.Load();

                recordDeferredLabels(localDevice.get(), submission.queue);
                const auto stamp = ++eventSerial;
                const auto reason = localDevice != nullptr ? localDevice->WriteLabelOnGpu(label->address, bytes, stamp, submission.queue) : 4;
                wroteOnGpu = reason == 0 || reason == 5;
                if (reason == 1) {

                    GuestMemory::Write(label->address, bytes, 4);
                    wroteOnGpu = true;
                }
                if (wroteOnGpu) noteLabelStore(label->address, bytes, stamp, submission.queue);
                Graphics::Recorder::CloseLabelGroup(GuestMemory::TrackerGeneration());
                countLabelOutcome(reason);
                ++immediateLabels;
            }
        } else if (opcode == 0x49 ? ((packet[2] >> 29u) == 0 || (packet[3] | (static_cast<std::uint64_t>(packet[4]) << 32u)) == 0) : ((packet[2] | (static_cast<std::uint64_t>(packet[3]) << 32u)) == 0 || Pm4::IsTagMarker(packet))) {
            orderedAlready = true;
            ++noOpLabels;
        } else {
            ++labelFallbacks[4];
        }
    }

    // Profile only: a DMA_DATA whose memory or GDS source has recorded GPU writes pending (its CPU read below or in
    // Execute waits for them, unless a GDS readback becomes a transfer), by destination (memory, GDS, nowhere) and source (memory, GDS).
    if (profile && opcode == 0x50 && Pm4::DmaSource(packet) != 2) {
        const bool fromGds = Pm4::DmaSource(packet) == Pm4::DmaSelectGds;
        const auto source = fromGds ? Pm4::GdsAddress() + packet[2] : packet[2] | (static_cast<std::uint64_t>(packet[3]) << 32u);
        const auto pending = Graphics::Recorder::PendingWriteSnapshot();
        if (Graphics::Recorder::SnapshotOverlaps(pending.get(), source, std::max<std::size_t>(packet[6] & 0x3ffffffu, 1))) {
            const auto destination = Pm4::DmaDestination(packet);
            ++pendingSourceDmas[(destination == Pm4::DmaSelectGds ? 2 : destination == 2 ? 4 : 0) + (fromGds ? 1 : 0)];
        }
    }

    if (profile) noteDmaChain(submission.queue, opcode, packet);

    static const bool cpuStores = std::getenv("APS5_CPU_STORES") != nullptr;
    if (!drainAll && !cpuStores && (opcode == 0x40 || opcode == 0x50 || opcode == 0x83)) {
        constexpr std::size_t gpuStoreLimit = 65536;
        bool drained = true;
        int storeReason = -1;
        // A DMA_DATA from the GDS into memory while GDS users are still pending: first a transfer out of the GDS's host
        // import recorded behind every GDS user recorded before it (VulkanDevice::CopyBuffer path 1, as the larger memory
        // copies below), instead of the CPU read below whose flush hook waits for those users. With nothing pending the
        // CPU read is immediate and its known-value store keeps later waits on the destination answerable. Debug aid:
        // APS5_GDS_CPU_READBACKS=1 reads on the CPU.
        static const bool gdsCpuReadbacks = std::getenv("APS5_GDS_CPU_READBACKS") != nullptr;
        if (opcode == 0x50 && !gdsCpuReadbacks) {
            const auto readback = Pm4::DecodeGdsReadback(packet);
            if (readback.has_value() && GuestMemory::Accessible(reinterpret_cast<const void*>(readback->destination), readback->bytes, true)) {
                GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Copy);
                std::lock_guard gpuLock(GuestMemory::GpuMutex());
                if (const auto localDevice = device.Load(); localDevice != nullptr && localDevice->StoresPendingOver(readback->source, readback->bytes)) {
                    recordDeferredLabels(localDevice.get(), submission.queue);
                    // The destination's writer is foreign to the shaders' write evidence (as a CPU copy's): its readers sync.
                    const auto outcome = localDevice->CopyBuffer(readback->destination, readback->source, readback->bytes, 0, std::numeric_limits<std::size_t>::max(), 0, 0, submission.queue, [&](std::span<const std::byte>, std::uint64_t) { noteForeignWriter(readback->destination, readback->destination + readback->bytes, submission.queue); });
                    if (outcome.path == 1) {
                        // The GDS stays noted as written until the transfer ran, as a GDS binding notes it: a CPU store
                        // into it (Execute's fallback) waits for the transfer instead of landing before it reads. The
                        // transfer is recorded work, so the submission end submits it.
                        if (auto* recorder = Graphics::Recorder::Active()) recorder->NotePendingWrite(readback->source, readback->bytes);
                        Graphics::Recorder::CountRecordedWork();
                        ++gdsReadbacksOnGpu;
                        wroteOnGpu = true;
                        drained = false;
                    }
                }
            }
        }
        const auto copy = opcode == 0x50 ? Pm4::DecodeMemoryCopy(packet) : std::nullopt;
        const auto copyBehindPendingSource = [&] {
            if (!copy.has_value() || copy->bytes > gpuStoreLimit || !GuestMemory::Accessible(reinterpret_cast<const void*>(copy->source), copy->bytes) || !GuestMemory::Accessible(reinterpret_cast<const void*>(copy->destination), copy->bytes, true)) return false;
            const auto end = copy->source + copy->bytes;
            const std::array<std::pair<std::uint64_t, std::uint64_t>, 1> range{{{copy->source, end}}};
            const auto& queued = deferredLabels().labels;
            const bool labelQueued = std::any_of(queued.begin(), queued.end(), [&](const DeferredLabel& label) { return label.address < end && copy->source < label.address + label.size; });
            if (!labelQueued && !Graphics::Recorder::SnapshotWriteOverlaps(copy->source, copy->bytes) && !Graphics::StorageTexture::AnyPendingOverlaps(range) && !Graphics::AnyShadowedOverlaps(copy->source, copy->bytes)) return false;
            GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Copy);
            std::lock_guard gpuLock(GuestMemory::GpuMutex());
            const auto localDevice = device.Load();
            if (localDevice == nullptr) return false;
            recordDeferredLabels(localDevice.get(), submission.queue);
            if (!localDevice->StoresPendingOver(copy->source, copy->bytes) || !localDevice->GpuCopyPossible(copy->destination, copy->source, copy->bytes)) return false;
            const auto outcome = localDevice->CopyBuffer(copy->destination, copy->source, copy->bytes, 0, std::numeric_limits<std::size_t>::max(), 0, 0, submission.queue, [](std::span<const std::byte>, std::uint64_t) {});
            if (outcome.path != 1 && outcome.path != 3) throw std::runtime_error("AGC driver: a DMA_DATA copy of up to 64 KiB that GpuCopyPossible accepted took copy path " + std::to_string(outcome.path));
            return true;
        };
        if (drained && copyBehindPendingSource()) {
            ++copiesBehindPendingSources;
            wroteOnGpu = true;
            drained = false;
        } else if (const auto store = drained ? Pm4::ResolveStore(packet, queue, gpuStoreLimit) : std::optional<Pm4::StoreWrite>{}) {
            const auto bytes = store->Bytes();
            if (bytes.empty()) {

                orderedAlready = true;
                drained = false;
            } else {

                GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Label);
                std::lock_guard gpuLock(GuestMemory::GpuMutex());
                const auto localDevice = device.Load();

                Graphics::StorageTexture::FlushPending(store->address, bytes.size(), nullptr, "packet store", Graphics::PublishScope::PartialUnits);

                recordDeferredLabels(localDevice.get(), submission.queue);
                const auto stamp = ++eventSerial;
                const auto reason = localDevice != nullptr ? localDevice->WriteLabelOnGpu(store->address, bytes, stamp, submission.queue) : 4;
                storeReason = reason;
                if (reason == 0 || reason == 1 || reason == 5) noteLabelStore(store->address, bytes, stamp, submission.queue);
                if (reason == 0 || reason == 5) {
                    if (reason == 0) ++storesOnGpu;
                    else ++storesBehindCompletions;
                    wroteOnGpu = true;
                    drained = false;
                } else if (reason == 1) {
                    GuestMemory::Write(store->address, bytes, 1);
                    ++storesOnCpu;
                    wroteOnGpu = true;
                    drained = false;
                }
                Graphics::Recorder::CloseLabelGroup(GuestMemory::TrackerGeneration());
            }
        }
        if (drained && opcode == 0x50) {
            if (copy.has_value() && copy->bytes > gpuStoreLimit && GuestMemory::Accessible(reinterpret_cast<const void*>(copy->source), copy->bytes) && GuestMemory::Accessible(reinterpret_cast<const void*>(copy->destination), copy->bytes, true)) {
                GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Copy);
                std::lock_guard gpuLock(GuestMemory::GpuMutex());
                if (const auto localDevice = device.Load()) {
                    recordDeferredLabels(localDevice.get(), submission.queue);
                    const auto outcome = localDevice->CopyBuffer(copy->destination, copy->source, copy->bytes, 0, std::numeric_limits<std::size_t>::max(), 0, 0, submission.queue, [](std::span<const std::byte>, std::uint64_t) {});
                    if (outcome.path == 1 || outcome.path == 3) {
                        wroteOnGpu = true;
                        drained = false;
                    } else if (outcome.path == 2 && !Graphics::RegisteredReadableCovers(copy->destination, copy->bytes) && !localDevice->StoresPendingOver(copy->destination, copy->bytes)) {
                        Graphics::StorageTexture::FlushPending(copy->source, copy->bytes, nullptr, "buffer copy source", Graphics::PublishScope::Whole);
                        if (auto* recorder = Graphics::Recorder::Active()) recorder->SyncThrough(copy->source, copy->bytes);
                        ++copiesToHostMemory;
                        orderedAlready = true;
                        drained = false;
                    }
                }
            }
        }
        if (drained && opcode == 0x50 && Pm4::DmaSource(packet) == 2 && Pm4::DmaDestination(packet) != 1) {
            const auto destination = packet[4] | (static_cast<std::uint64_t>(packet[5]) << 32u);
            const std::size_t bytes = packet[6] & 0x3ffffffu;
            if (bytes != 0 && destination % 16 == 0 && bytes % 16 == 0 && GuestMemory::Accessible(reinterpret_cast<const void*>(destination), bytes, true)) {
                if (const auto localDevice = device.Load()) {
                    storeBufferFill(submission.queue, destination, bytes, {packet[2], packet[2], packet[2], packet[2]}, localDevice, false, false);
                    ++dmaFills;
                    wroteOnGpu = true;
                    drained = false;
                }
            }
        }
        // A DMA_DATA into the emulated GDS: Pm4::Execute stores it on the CPU through GuestMemory::Write,
        // whose flush hook waits for the recorded GPU work noted over the GDS. Every GDS binding notes
        // the whole GDS as written (ShaderResources: AddWritable), readers included, as does a GDS readback
        // transfer (below), so that work is every recorded GDS user of any queue: its copy-backs land before the store and its copy-ins
        // read before it. Nothing else reaches the GDS (its address is the driver's own), so the device
        // drain that waited for every queue's newest batch orders nothing more. Debug aid:
        // APS5_GDS_DRAIN=1 drains as before.
        static const bool gdsDrain = std::getenv("APS5_GDS_DRAIN") != nullptr;
        const bool toGds = opcode == 0x50 && Pm4::DmaDestination(packet) == Pm4::DmaSelectGds;
        // First as a GPU store into the GDS's host import, recorded into the open batch like a label (Recorder::
        // RecordStore orders it after every GDS user recorded before and before every one recorded after; its note
        // makes CPU readers of the GDS wait), never behind completions (reason 5). Debug aid: APS5_GDS_CPU_STORES=1
        // stores every one on the CPU behind the hook.
        static const bool gdsCpuStores = std::getenv("APS5_GDS_CPU_STORES") != nullptr;
        // A DMA_DATA from memory into the GDS whose source still has recorded GPU writes pending (a GDS readback
        // transferred above, a shader's result): a transfer into the GDS's host import behind them, instead of
        // ResolveGdsStore's CPU read that waits for them. Astro's queue 0 copies GDS 0x4 -> memory -> GDS 0x24 every frame;
        // with the readback on the GPU that read waited ~100 ms for the open batch (play1015q). The GDS is noted written
        // by the transfer, so its users and CPU stores order behind it. Debug aid: APS5_GDS_CPU_LOADS=1 reads on the CPU.
        static const bool gdsCpuLoads = std::getenv("APS5_GDS_CPU_LOADS") != nullptr;
        if (drained && toGds && !gdsDrain && !gdsCpuStores && !gdsCpuLoads) {
            const auto load = Pm4::DecodeGdsLoad(packet);
            if (load.has_value() && GuestMemory::Accessible(reinterpret_cast<const void*>(load->source), load->bytes)) {
                GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Copy);
                std::lock_guard gpuLock(GuestMemory::GpuMutex());
                if (const auto localDevice = device.Load(); localDevice != nullptr && localDevice->StoresPendingOver(load->source, load->bytes)) {
                    recordDeferredLabels(localDevice.get(), submission.queue);
                    const auto outcome = localDevice->CopyBuffer(load->destination, load->source, load->bytes, 0, std::numeric_limits<std::size_t>::max(), 0, 0, submission.queue, [](std::span<const std::byte>, std::uint64_t) {});
                    if (outcome.path == 1) {
                        Graphics::Recorder::CountRecordedWork();
                        ++gdsLoadsOnGpu;
                        wroteOnGpu = true;
                        drained = false;
                    }
                }
            }
        }
        if (drained && toGds && !gdsDrain && !gdsCpuStores) {
            if (const auto store = Pm4::ResolveGdsStore(packet, gpuStoreLimit)) {
                const auto bytes = store->Bytes();
                GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Label);
                std::lock_guard gpuLock(GuestMemory::GpuMutex());
                const auto localDevice = device.Load();
                recordDeferredLabels(localDevice.get(), submission.queue);
                const auto stamp = ++eventSerial;
                const auto reason = localDevice != nullptr ? localDevice->WriteLabelOnGpu(store->address, bytes, stamp, submission.queue, true, false) : 4;
                if (reason == 0 || reason == 1) noteLabelStore(store->address, bytes, stamp, submission.queue);
                if (reason == 0) {
                    ++gdsStoresOnGpu;
                    wroteOnGpu = true;
                    drained = false;
                } else if (reason == 1) {
                    GuestMemory::Write(store->address, bytes, 1);
                    ++storesOnCpu;
                    wroteOnGpu = true;
                    drained = false;
                }
                Graphics::Recorder::CloseLabelGroup(GuestMemory::TrackerGeneration());
            }
        }
        if (drained && toGds && !gdsDrain) {
            orderedAlready = true;
            drained = false;
            ++gdsStoresOrdered;
        }
        if (drained) {
            ++storesDrained;
            ++storeDrainReasons[toGds ? 0 : storeReason == 3 ? 1 : storeReason >= 0 ? 2 : 3];
        }
    }

    static const bool drawDrain = std::getenv("APS5_DRAW_DRAIN") != nullptr;
    drawPacket = Pm4::DrawOpcode(opcode);
    sampleDump = Pm4::SampleDump(packet);
    // APS5_SYNTHETIC_OCCLUSION=1: every occlusion query reads "visible" (a counter that grows by 1 per DB and dump,
    // as KytyPS5 publishes it): no host queries, no segments, no drain. The values go out as deferred labels, in queue
    // order with the game's own GPU stores to the result buffer (a CPU store would wait for those).
    static const bool syntheticOcclusion = std::getenv("APS5_SYNTHETIC_OCCLUSION") != nullptr;
    if (sampleDump && syntheticOcclusion && DeferLabels()) {
        static std::atomic<std::uint64_t> counter{0};
        const std::uint64_t value = (1ull << 63u) | counter.fetch_add(1, std::memory_order_relaxed);
        const std::uint64_t address = Pm4::SampleDumpAddress(packet);
        auto& deferred = deferredLabels();
        if (deferred.labels.empty()) deferred.since = std::chrono::steady_clock::now();
        for (std::uint64_t db = 0; db < Pm4::SampleDumpBlocks; ++db) {
            auto& entry = deferred.labels.emplace_back();
            entry.address = address + db * Pm4::SampleDumpStride;
            entry.size = Pm4::SampleDumpBlockBytes;
            std::memcpy(entry.bytes.data(), &value, sizeof(value));
        }
        queuedLabels += Pm4::SampleDumpBlocks;
        sampleDump = false;
    }
    // APS5_PIPELINE_SAMPLE_DUMPS=1: an occlusion counter dump behind pipelined draws is an ordered commit item (recorded
    // after the draws whose samples it sums, as a drain would order it) instead of a drain: the hub's ~12 dumps per
    // present each emptied the pipeline and idled the committer (play1018s: 1833 drains of 6550 per 10 s).
    static const bool pipelineSampleDumps = std::getenv("APS5_PIPELINE_SAMPLE_DUMPS") != nullptr;
    if (sampleDump && !drainAll && pipelineSampleDumps && DrawPipeline::Active() && DrawPipeline::Queue0().Busy()) {
        if (deferredLabels().labels.empty()) {
            const auto address = Pm4::SampleDumpAddress(packet);
            const auto queueId = submission.queue;
            DrawPipeline::Queue0().Enqueue([this, address, queueId] {
                GuestMemory::SetCurrentPacket(0x46, queueId);
                bool written = false;
                {
                    GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Label);
                    std::lock_guard gpuLock(GuestMemory::GpuMutex());
                    if (const auto localDevice = device.Load()) {
                        recordDeferredLabels(localDevice.get(), queueId);
                        written = localDevice->DumpSamplesOnGpu(address);
                    }
                }
                if (written) Graphics::Recorder::CountRecordedWork();
                else dumpSampleCounters(address);
            }, {{address, address + Pm4::SampleDumpBytes}});
            // Handled: nothing below records, stores or drains for it.
            sampleDump = false;
            wroteOnGpu = true;
        } else {
            // keepsDrawPipeline let the packet in undrained: the inline dump below must still follow the queued draws.
            DrawPipeline::Queue0().Drain(DrawPipeline::DrainReason::Packet, 0x46);
        }
    }
    if (sampleDump && !drainAll) {
        GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Label);
        std::lock_guard gpuLock(GuestMemory::GpuMutex());
        if (const auto localDevice = device.Load()) {
            recordDeferredLabels(localDevice.get(), submission.queue);
            wroteOnGpu = localDevice->DumpSamplesOnGpu(Pm4::SampleDumpAddress(packet));
        }
    }

    if (header == FlipPacketHeader) {
        GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Flush);
        std::lock_guard gpuLock(GuestMemory::GpuMutex());
        static_cast<void>(Graphics::StorageTexture::StoreSmallPending("flip"));
    }

    static const bool syncFlip = std::getenv("APS5_SYNC_FLIP") != nullptr;
    const bool drains = drainAll ? ((Pm4::AccessesMemory(header) && opcode != 0x16) || opcode == 0x42 || opcode == 0x46 || opcode == 0x58 || header == FlipPacketHeader)
                                 : (!wroteOnGpu && !orderedAlready && (opcode == 0x49 || opcode == 0x37 || opcode == 0x40 || opcode == 0x45 || opcode == 0x50 || opcode == 0x83 || (opcode == 0x1e && !WorkloadSlotTableCovers(Pm4::AtomicMemAddress(packet), Pm4::AtomicMemBytes(packet))) || sampleDump || (drawPacket && drawDrain) || (header == FlipPacketHeader && syncFlip)));
    if (drains) {

        static const bool unlockedDrain = std::getenv("APS5_NO_UNLOCKED_DRAIN") == nullptr && !drainAll;

        bumpEpoch(&EpochBumps::drains);
        std::shared_ptr<VulkanDevice> draining;
        std::uint64_t epoch = 0;
        {
            GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Flush);
            std::lock_guard gpuLock(GuestMemory::GpuMutex());
            if (profile) {
                ++drainCounts[header == FlipPacketHeader ? 0xffffu : opcode];
                ++drainTotal;
                if (std::chrono::steady_clock::now() - lastSyncReport > std::chrono::seconds(10)) {
                    lastSyncReport = std::chrono::steady_clock::now();
                    reportSync();
                }
            }
            draining = device.Load();

            recordDeferredLabels(draining.get(), submission.queue);
            if (draining != nullptr) {
                if (unlockedDrain && draining->CanWaitUnlocked()) epoch = draining->SubmitAndEpoch();
                else draining->WaitIdle();
            }
            if (epoch == 0) draining.reset();
        }
        if (epoch != 0) {
            ++unlockedDrains;
            try {
                draining->WaitRecorded(epoch);
            } catch (...) {

                GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Flush);
                std::lock_guard gpuLock(GuestMemory::GpuMutex());
                draining.reset();
                throw;
            }
            GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Flush);
            std::lock_guard gpuLock(GuestMemory::GpuMutex());
            draining->ReapRecorded(epoch);
            draining.reset();
        }
    }
    return drains;
}

void Driver::dumpSampleCounters(std::uint64_t address) {
    std::uint64_t samples = 0;
    {
        GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Flush);
        std::lock_guard gpuLock(GuestMemory::GpuMutex());
        auto* recorder = Graphics::Recorder::Active();
        require(recorder != nullptr, "occlusion counters without the command recorder are not implemented");
        recorder->CountSamples();
        samples = recorder->SamplesTotal();
    }
    constexpr std::uint64_t ready = 1ull << 63u;
    for (std::uint64_t db = 0; db < Pm4::SampleDumpBlocks; ++db) {
        const std::uint64_t value = ready | (db == 0 ? samples : 0u);
        GuestMemory::Write(address + db * Pm4::SampleDumpStride, std::as_bytes(std::span(&value, 1)).first(Pm4::SampleDumpBlockBytes), 8);
    }
}

bool Driver::enqueueDmaPacket(std::span<const std::uint32_t> packet, std::uint32_t opcode, std::uint32_t queue, const QueueState& state) {
    static const bool enabled = std::getenv("APS5_PIPELINE_DMA") != nullptr;
    if (!enabled || opcode != 0x50 || queue != 0 || packet.size() < 7 || !DrawPipeline::Active() || !deferredLabels().labels.empty()) return false;
    const auto source = ((packet[1] >> 29u) & 3u) | ((packet[6] >> 24u) & 4u) | ((packet[6] >> 25u) & 8u);
    constexpr std::size_t immediateLimit = std::size_t{16} << 20u;
    constexpr std::size_t gpuStoreLimit = 65536;
    if (source == 2 && Pm4::DmaBytes(packet) > gpuStoreLimit && Pm4::DmaDestination(packet) != Pm4::DmaSelectGds) {
        const auto fillAddress = packet[4] | (static_cast<std::uint64_t>(packet[5]) << 32u);
        const auto fillBytes = Pm4::DmaBytes(packet);
        const auto pattern = packet[2];
        if (fillAddress % 16 != 0 || fillBytes % 16 != 0 || !GuestMemory::Accessible(reinterpret_cast<const void*>(fillAddress), fillBytes, true) || !device.Load()) return false;
        DrawPipeline::Queue0().Enqueue([this, queue, fillAddress, fillBytes, pattern] {
            GuestMemory::SetCurrentPacket(0x50, queue);
            if (const auto localDevice = device.Load()) {
                storeBufferFill(queue, fillAddress, fillBytes, {pattern, pattern, pattern, pattern}, localDevice, false, false);
                ++dmaFills;
            }
        }, {{fillAddress, fillAddress + fillBytes}});
        return true;
    }
    if (source == 2) {
        const auto store = Pm4::ResolveStore(packet, state, immediateLimit);
        if (!store.has_value() || store->Bytes().empty() || !GuestMemory::Accessible(reinterpret_cast<const void*>(store->address), store->Bytes().size(), true)) return false;
        const auto address = store->address;
        std::vector<std::byte> bytes(store->Bytes().begin(), store->Bytes().end());
        const auto size = bytes.size();
        const bool metadataFill = Pm4::DmaDestination(packet) != 1;
        const auto fillAddress = packet[4] | (static_cast<std::uint64_t>(packet[5]) << 32u);
        const auto fillBytes = packet[6] & 0x3ffffffu;
        const auto pattern = packet[2];
        DrawPipeline::Queue0().Enqueue([this, queue, address, bytes = std::move(bytes), metadataFill, fillAddress, fillBytes, pattern] {
            GuestMemory::SetCurrentPacket(0x50, queue);
            if (metadataFill) {
                Graphics::NoteDepthMetadataFill(fillAddress, fillBytes, pattern);
                Graphics::NoteColorMetadataFill(fillAddress, fillBytes, pattern);
            }
            commitDmaStore(queue, address, bytes);
        }, {{address, address + size}});
        return true;
    }
    if (Pm4::DmaSource(packet) == 0 || Pm4::DmaSource(packet) == 3) Graphics::RequireNoMultisampleTarget(packet[2] | (static_cast<std::uint64_t>(packet[3]) << 32u), packet[6] & 0x3ffffffu, "a DMA copy");
    const auto copy = Pm4::DecodeMemoryCopy(packet);
    if (!copy.has_value() || copy->destination % 4 != 0 || copy->bytes % 4 != 0 || !GuestMemory::Accessible(reinterpret_cast<const void*>(copy->source), copy->bytes) || !GuestMemory::Accessible(reinterpret_cast<const void*>(copy->destination), copy->bytes, true)) return false;
    DrawPipeline::Queue0().Enqueue([this, queue, copy = *copy] {
        DrawPipeline::Queue0().ReleaseRunningReads();
        GuestMemory::SetCurrentPacket(0x50, queue);
        constexpr std::size_t gpuStoreLimit = 65536;
        if (copy.bytes > gpuStoreLimit) {
            GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Copy);
            std::lock_guard gpuLock(GuestMemory::GpuMutex());
            if (const auto localDevice = device.Load()) {
                recordDeferredLabels(localDevice.get(), queue);
                const auto outcome = localDevice->CopyBuffer(copy.destination, copy.source, copy.bytes, 0, std::numeric_limits<std::size_t>::max(), 0, 0, queue, [](std::span<const std::byte>, std::uint64_t) {});
                if (outcome.path == 1 || outcome.path == 3) return;
                if (outcome.path == 2 && !Graphics::RegisteredReadableCovers(copy.destination, copy.bytes) && !localDevice->StoresPendingOver(copy.destination, copy.bytes)) ++copiesToHostMemory;
            }
        }
        std::vector<std::byte> bytes(copy.bytes);
        GuestMemory::Read(copy.source, bytes, 1);
        commitDmaStore(queue, copy.destination, bytes);
    }, {{copy->destination, copy->destination + copy->bytes}}, 0, {}, {{copy->source, copy->source + copy->bytes}});
    return true;
}

void Driver::commitDmaStore(std::uint32_t queue, std::uint64_t address, std::span<const std::byte> bytes) {
    constexpr std::size_t gpuStoreLimit = 65536;
    GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Label);
    std::lock_guard gpuLock(GuestMemory::GpuMutex());
    const auto localDevice = device.Load();
    Graphics::StorageTexture::FlushPending(address, bytes.size(), nullptr, "packet store", Graphics::PublishScope::PartialUnits);
    recordDeferredLabels(localDevice.get(), queue);
    int reason = 4;
    std::uint64_t stamp = 0;
    if (bytes.size() <= gpuStoreLimit && localDevice != nullptr) {
        stamp = ++eventSerial;
        reason = localDevice->WriteLabelOnGpu(address, bytes, stamp, queue);
    }
    if (reason == 0 || reason == 5 || reason == 6) {
        noteLabelStore(address, bytes, stamp, queue);
        if (reason == 0) ++storesOnGpu;
        else ++storesBehindCompletions;
    } else {
        if (reason != 1 && localDevice != nullptr) localDevice->WaitIdle();
        GuestMemory::Write(address, bytes, 1);
        noteLabelStore(address, bytes, stamp != 0 ? stamp : ++eventSerial, queue);
        ++storesOnCpu;
    }
    Graphics::Recorder::CloseLabelGroup(GuestMemory::TrackerGeneration());
}

bool Driver::enqueueLabelPacket(std::span<const std::uint32_t> packet, std::uint32_t opcode, std::uint32_t queue) {
    static const bool ordered = std::getenv("APS5_PIPELINE_DRAIN_LABELS") == nullptr;
    if (!ordered || !DeferLabels() || (opcode != 0x49 && opcode != 0x37) || !deferredLabels().labels.empty()) return false;
    const auto interruptSelect = opcode == 0x49 ? (packet[2] >> 24u) & 7u : 0u;
    const bool endOfPipeInterrupt = interruptSelect != 0 && interruptSelect != 3;
    const auto label = Pm4::DecodeLabelWrite(packet);
    if (!label.has_value()) {
        if (opcode != 0x49) return false;
        const bool storesNothing = (packet[2] >> 29u) == 0 || (packet[3] | (static_cast<std::uint64_t>(packet[4]) << 32u)) == 0;
        if (!storesNothing) return false;
        if (!endOfPipeInterrupt) {
            ++noOpLabels;
            return true;
        }
    }
    std::uint64_t address = 0;
    std::vector<std::byte> bytes;
    std::vector<DrawPipeline::Range> writes;
    if (label.has_value()) {
        const auto stored = label->Bytes();
        address = label->address;
        bytes.assign(stored.begin(), stored.end());
        writes.emplace_back(address, address + bytes.size());
    }
    static const bool labelEpoch = std::getenv("APS5_LABEL_EPOCH") != nullptr;
    if (endOfPipeInterrupt && labelEpoch) bumpEpoch(&EpochBumps::drains);
    auto stored = bytes;
    DrawPipeline::Queue0().Enqueue([this, queue, opcode, address, bytes = std::move(bytes), endOfPipeInterrupt] {
        GuestMemory::SetCurrentPacket(opcode, queue);
        commitLabel(queue, address, bytes, endOfPipeInterrupt);
    }, std::move(writes), address, std::move(stored));
    return true;
}

void Driver::commitLabel(std::uint32_t queue, std::uint64_t address, std::span<const std::byte> bytes, bool endOfPipeInterrupt) {
    GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Label);
    std::lock_guard gpuLock(GuestMemory::GpuMutex());
    const auto localDevice = device.Load();
    recordDeferredLabels(localDevice.get(), queue);
    const bool workOpen = Graphics::Recorder::RecordedWorkSinceSubmit() != 0;
    int reason = localDevice != nullptr ? 0 : 1;
    if (!bytes.empty()) {
        const auto stamp = ++eventSerial;
        reason = localDevice != nullptr ? localDevice->WriteLabelOnGpu(address, bytes, stamp, queue) : 4;
        if (reason != 0 && reason != 5 && reason != 6) {
            if (reason != 1 && localDevice != nullptr) localDevice->WaitIdle();
            GuestMemory::Write(address, bytes, 4);
        }
        noteLabelStore(address, bytes, stamp, queue);
        Graphics::Recorder::CloseLabelGroup(GuestMemory::TrackerGeneration());
        countLabelOutcome(reason);
        ++immediateLabels;
    } else {
        ++noOpLabels;
    }
    if (endOfPipeInterrupt) {
        bool deferred = false;
        if (localDevice != nullptr && (reason == 0 || reason == 5 || reason == 6)) {
            deferred = localDevice->AfterRecordedWork([this, queue] { deliverGuestEopInterrupt(queue); }, queue == 0);
            if (deferred && workOpen) localDevice->SubmitRecorded(queue == 0);
        }
        if (!deferred) deliverGuestEopInterrupt(queue);
    }
    submitDueAfterCommit(localDevice.get());
}

void Driver::submitDueAfterCommit(VulkanDevice* localDevice) {
    if (localDevice == nullptr) return;
    const auto pending = Graphics::Recorder::PendingLabelSince();
    const bool due = pending.has_value() && std::chrono::steady_clock::now() - *pending >= labelFlushDeadline();
    const bool capped = batchCap() != 0 && Graphics::Recorder::RecordedWorkSinceSubmit() >= batchCap();
    if (due || capped) localDevice->SubmitRecorded(true);
}

}
