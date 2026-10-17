#include "prx/libSceAgcDriver/Execution/include/ProfileOutput.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Diagnostics.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Queues/WorkerAffinity.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Shaders/ShaderRegistry.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Synchronization/RunLabels.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Execution/include/Pm4.hpp"
#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Recorder.hpp"
#include "prx/libSceAgcDriver/Graphics/include/UnitShadow.hpp"
#include "Optimization/ResourceProgram.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace AgcDriver::DriverDetail {

namespace {

double MillisecondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

bool ProfileDraw() {
    static const bool profile = std::getenv("APS5_PROFILE_DRAW") != nullptr;
    return profile;
}

// APS5_DRAW_KEY_PROBE=1 (diagnostic, draw-prep thread only): how prepared draws relate to the last draws with the same
// shape key (Driver::drawShapeKey: the register key without user-data words), and where prepareDraw's time goes.
// Feeds the address-free draw key decision (AnyPS5-tools/draw-key-map-1015.md, stage 1); changes no draw.
bool drawKeyProbe() {
    static const bool probe = std::getenv("APS5_DRAW_KEY_PROBE") != nullptr;
    return probe;
}

enum ProbePhase : std::size_t { ProbeKey, ProbeLookup, ProbeDecode, ProbeVertexInfo, ProbeInvocation, ProbeWalk, ProbeMaterialize, ProbeVariant, ProbeInsert, ProbeClassify, ProbePhaseCount };
constexpr std::array<const char*, ProbePhaseCount> ProbePhaseNames{"key", "lookup", "decode", "vertex-info", "invocation", "walk", "materialize", "variant", "insert", "probe"};

struct ProbeStage {
    bool valid = false;
    std::uint64_t variantId = 0;
    std::vector<std::uint32_t> userData;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> runs;
    std::vector<std::uint32_t> words;
};

struct ProbeSets {
    std::array<std::vector<ProbeStage>, 4> sets;
    std::size_t count = 0;
    std::size_t next = 0;
};

struct DrawKeyProbe {
    std::unordered_map<std::uint64_t, ProbeSets> table;
    std::array<double, ProbePhaseCount> ns{};
    double prepareNs = 0;
    std::uint64_t prepareCalls = 0, decodes = 0, probed = 0, shapeNew = 0, shapeKnown = 0, drawsSameCode = 0, drawsSameRuns = 0, drawsExplained = 0, drawsIdentical = 0, drawsUserEqual = 0;
    std::array<std::uint64_t, 5> bestRank{};
    std::uint64_t stages = 0, stagesSameCode = 0, stagesSameRuns = 0, stagesWordsEqual = 0, stagesExplained = 0, runsTotal = 0, runsExplained = 0, userWordsDiffering = 0, stagesUserDiffering = 0, wordsCompared = 0, wordsDiffering = 0;
};

DrawKeyProbe& keyProbe() {
    static auto* probe = new DrawKeyProbe;
    return *probe;
}

struct ProbeTimer {
    bool on = drawKeyProbe();
    std::chrono::steady_clock::time_point last = on ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
    void Mark(ProbePhase phase) {
        if (!on) return;
        const auto now = std::chrono::steady_clock::now();
        keyProbe().ns[phase] += static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(now - last).count());
        last = now;
    }
};

// A pointer in a user-data word pair (low word, then a high word whose low 16 bits complete the address).
std::uint64_t userPointer(std::span<const std::uint32_t> words, std::size_t at) {
    return static_cast<std::uint64_t>(words[at]) | (static_cast<std::uint64_t>(words[at + 1] & 0xffffu) << 32u);
}

struct StageVerdict {
    bool sameCode = false, sameRuns = false, wordsEqual = false, explained = false;
    std::size_t runsExplained = 0, userDiffering = 0, wordsDiffering = 0;
};

StageVerdict compareStage(const ProbeStage& before, const ProbeStage& now) {
    StageVerdict verdict;
    verdict.sameCode = before.variantId == now.variantId;
    for (std::size_t i = 0; i < std::min(before.userData.size(), now.userData.size()); ++i) verdict.userDiffering += before.userData[i] != now.userData[i] ? 1 : 0;
    verdict.sameRuns = before.runs.size() == now.runs.size() && std::equal(before.runs.begin(), before.runs.end(), now.runs.begin(), [](const auto& a, const auto& b) { return a.second - a.first == b.second - b.first; });
    verdict.wordsEqual = before.words == now.words;
    if (before.words.size() == now.words.size()) {
        for (std::size_t i = 0; i < now.words.size(); ++i) verdict.wordsDiffering += before.words[i] != now.words[i] ? 1 : 0;
    }
    std::vector<std::uint64_t> deltas{0};
    if (before.userData.size() == now.userData.size()) {
        for (std::size_t k = 0; k + 1 < now.userData.size(); ++k) {
            const auto was = userPointer(before.userData, k);
            const auto is = userPointer(now.userData, k);
            if (was != is && was != 0 && is != 0) deltas.push_back(is - was);
        }
    }
    for (const auto& [begin, end] : now.runs) {
        const bool found = std::any_of(before.runs.begin(), before.runs.end(), [&](const auto& old) {
            return old.second - old.first == end - begin && std::find(deltas.begin(), deltas.end(), begin - old.first) != deltas.end();
        });
        verdict.runsExplained += found ? 1 : 0;
    }
    verdict.explained = verdict.runsExplained == now.runs.size();
    return verdict;
}

void probeDraw(std::uint64_t shapeKey, const std::vector<DrawProgram>& programs, const std::vector<std::shared_ptr<DispatchVariant>>& fresh) {
    auto& probe = keyProbe();
    ++probe.probed;
    std::vector<ProbeStage> current(programs.size());
    for (std::size_t i = 0; i < programs.size(); ++i) {
        if (fresh[i] == nullptr || fresh[i]->compiled == nullptr) continue;
        current[i] = {true, fresh[i]->compiled->PipelineVariantId(), programs[i].userData, fresh[i]->runs, fresh[i]->words};
    }
    if (probe.table.size() > 32768) probe.table.clear();
    auto& ring = probe.table[shapeKey];
    if (ring.count == 0) {
        ++probe.shapeNew;
    } else {
        ++probe.shapeKnown;
        int bestScore = -1;
        std::size_t bestRank = 0;
        std::vector<StageVerdict> best;
        for (std::size_t rank = 0; rank < ring.count; ++rank) {
            const auto& set = ring.sets[(ring.next + ring.sets.size() - 1 - rank) % ring.sets.size()];
            if (set.size() != current.size()) continue;
            std::vector<StageVerdict> verdicts(current.size());
            bool code = true, runs = true, explained = true;
            for (std::size_t i = 0; i < current.size(); ++i) {
                if (!current[i].valid || !set[i].valid) continue;
                verdicts[i] = compareStage(set[i], current[i]);
                code = code && verdicts[i].sameCode;
                runs = runs && verdicts[i].sameRuns;
                explained = explained && verdicts[i].explained;
            }
            const int score = (code ? 4 : 0) + (code && runs ? 2 : 0) + (code && runs && explained ? 1 : 0);
            if (score > bestScore) {
                bestScore = score;
                bestRank = rank;
                best = std::move(verdicts);
            }
        }
        if (bestScore < 0) {
            ++probe.bestRank[4];
        } else {
            ++probe.bestRank[bestRank];
            bool code = true, runs = true, explained = true, identical = true, userEqual = true;
            for (std::size_t i = 0; i < current.size(); ++i) {
                if (!current[i].valid) continue;
                const auto& verdict = best[i];
                ++probe.stages;
                probe.stagesSameCode += verdict.sameCode ? 1 : 0;
                probe.stagesSameRuns += verdict.sameRuns ? 1 : 0;
                probe.stagesWordsEqual += verdict.wordsEqual ? 1 : 0;
                probe.stagesExplained += verdict.explained ? 1 : 0;
                probe.runsTotal += current[i].runs.size();
                probe.runsExplained += verdict.runsExplained;
                probe.userWordsDiffering += verdict.userDiffering;
                probe.stagesUserDiffering += verdict.userDiffering != 0 ? 1 : 0;
                probe.wordsCompared += verdict.sameRuns ? current[i].words.size() : 0;
                probe.wordsDiffering += verdict.sameRuns ? verdict.wordsDiffering : 0;
                code = code && verdict.sameCode;
                runs = runs && verdict.sameRuns;
                explained = explained && verdict.explained;
                identical = identical && verdict.wordsEqual;
                userEqual = userEqual && verdict.userDiffering == 0;
            }
            probe.drawsSameCode += code ? 1 : 0;
            probe.drawsSameRuns += code && runs ? 1 : 0;
            probe.drawsExplained += code && runs && explained ? 1 : 0;
            probe.drawsIdentical += code && runs && identical ? 1 : 0;
            probe.drawsUserEqual += userEqual ? 1 : 0;
        }
    }
    ring.sets[ring.next] = std::move(current);
    ring.next = (ring.next + 1) % ring.sets.size();
    ring.count = std::min(ring.count + 1, ring.sets.size());
}

void reportKeyProbe() {
    auto& probe = keyProbe();
    const auto count = [](std::uint64_t value) { return static_cast<unsigned long long>(value); };
    const auto prepared = std::max<std::uint64_t>(probe.probed, 1);
    std::string phases;
    char part[96];
    double inside = 0;
    for (std::size_t i = 0; i < ProbePhaseCount; ++i) {
        inside += probe.ns[i];
        std::snprintf(part, sizeof(part), " %s %.1f", ProbePhaseNames[i], probe.ns[i] / 1e3 / static_cast<double>(prepared));
        phases += part;
    }
    AgcDriver::ProfilePrint_nid_no_patch("[draw-key-probe] (10 s): %llu prepared draws: shape new %llu, known %llu (best of the last value sets by rank 1-4 %llu/%llu/%llu/%llu, none comparable %llu); draws with every stage same code %llu, + same run layout %llu, + every run moved with a user pointer %llu, + identical words %llu; user data all equal %llu; stages %llu: same code %llu, same runs %llu, words equal %llu, explained %llu (runs %llu of %llu), user words differing %llu in %llu stages, words differing %llu of %llu compared; us per prepared draw:%s; prepareDraw %llu calls %.1f ms (phases %.1f ms), %llu decodes, shape table %zu\n",
        count(probe.probed), count(probe.shapeNew), count(probe.shapeKnown), count(probe.bestRank[0]), count(probe.bestRank[1]), count(probe.bestRank[2]), count(probe.bestRank[3]), count(probe.bestRank[4]),
        count(probe.drawsSameCode), count(probe.drawsSameRuns), count(probe.drawsExplained), count(probe.drawsIdentical), count(probe.drawsUserEqual),
        count(probe.stages), count(probe.stagesSameCode), count(probe.stagesSameRuns), count(probe.stagesWordsEqual), count(probe.stagesExplained), count(probe.runsExplained), count(probe.runsTotal), count(probe.userWordsDiffering), count(probe.stagesUserDiffering), count(probe.wordsDiffering), count(probe.wordsCompared),
        phases.c_str(), count(probe.prepareCalls), probe.prepareNs / 1e6, inside / 1e6, count(probe.decodes), probe.table.size());
    auto table = std::move(probe.table);
    probe = DrawKeyProbe{};
    probe.table = std::move(table);
}

}

bool Driver::drawPrepEnabled() {
    static const bool enabled = [] {
        return std::getenv("APS5_NO_DRAW_PREP") == nullptr && std::getenv("APS5_LOCKED_DRAW_PREPARE") == nullptr && std::getenv("APS5_DUMP_DRAW_SHADERS") == nullptr && std::getenv("APS5_DUMP_DRAW_SLOT1") == nullptr && drawEntries() && registerKeyEnabled();
    }();
    return enabled;
}

bool Driver::drawPrepSync() {
    static const bool sync = std::getenv("APS5_DRAW_PREP_SYNC") != nullptr;
    return sync;
}

std::size_t Driver::drawPrepAhead() {
    static const std::size_t ahead = [] {
        const char* text = std::getenv("APS5_DRAW_PREP_AHEAD");
        const auto parsed = text != nullptr ? std::strtoull(text, nullptr, 10) : 64ull;
        return static_cast<std::size_t>(std::max<unsigned long long>(parsed, 2));
    }();
    return ahead;
}

void Driver::startDrawPrep() {
    try {
        drawPrep.thread = std::thread([this] { runDrawPrep(); });
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[draw-prep] not started: %s\n", error.what());
    }
}

void Driver::stopDrawPrep() noexcept {
    if (!drawPrep.thread.joinable()) return;
    try {
        reportDrawPrep(true);
    } catch (...) {
    }
    {
        std::lock_guard lock(drawPrep.mutex);
        drawPrep.stop = true;
        drawPrep.cancel.store(true, std::memory_order_seq_cst);
    }
    drawPrep.changed.notify_all();
    drawPrep.thread.join();
}

bool Driver::beginDrawPrep(const Submission& submission, const QueueState& queue) {
    if (submission.queue != 0 || !drawPrep.thread.joinable() || submission.shaders == nullptr || submission.shaders->empty()) return false;
    const auto localDevice = device.Load();
    if (localDevice == nullptr) return false;
    {
        std::lock_guard lock(drawPrep.mutex);
        drawPrep.submission = &submission;
        drawPrep.scratch = queue;
        drawPrep.target = localDevice->Target();
        drawPrep.deviceSerial = localDevice->Serial();
        drawPrep.pipelineLibraries = localDevice->GraphicsPipelineLibraries();
        drawPrep.listRestart = localDevice->PrimitiveListRestart();
        ++drawPrep.generation;
        drawPrep.cancel.store(false, std::memory_order_seq_cst);
        drawPrep.cursor.store(0, std::memory_order_seq_cst);
        drawPrep.wakeAt.store(DrawPrepState::Never, std::memory_order_seq_cst);
        drawPrep.helperCursor.store(0, std::memory_order_seq_cst);
    }
    drawPrep.changed.notify_all();
    return true;
}

void Driver::endDrawPrep() noexcept {
    drawPrep.cancel.store(true, std::memory_order_seq_cst);
    std::unique_lock lock(drawPrep.mutex);
    drawPrep.changed.notify_all();
    drawPrep.released.wait(lock, [&] { return !drawPrep.busy; });
    drawPrep.submission = nullptr;
}

void Driver::drawPrepProgress(std::size_t cursor) {
    drawPrep.cursor.store(cursor, std::memory_order_seq_cst);
    auto wake = drawPrep.wakeAt.load(std::memory_order_seq_cst);
    if (cursor <= wake || !drawPrep.wakeAt.compare_exchange_strong(wake, DrawPrepState::Never, std::memory_order_seq_cst)) return;
    std::lock_guard lock(drawPrep.mutex);
    drawPrep.changed.notify_all();
}

void Driver::awaitDrawPrep(std::size_t cursor) {
    while (drawPrep.helperCursor.load(std::memory_order_acquire) <= cursor) std::this_thread::yield();
}

bool Driver::waitWorkerPast(std::size_t offset, double DrawPrepState::*waited) {
    const auto started = std::chrono::steady_clock::now();
    std::unique_lock lock(drawPrep.mutex);
    drawPrep.wakeAt.store(offset, std::memory_order_seq_cst);
    drawPrep.changed.wait(lock, [&] { return drawPrep.stop || drawPrep.cancel.load(std::memory_order_seq_cst) || drawPrep.cursor.load(std::memory_order_seq_cst) > offset; });
    drawPrep.wakeAt.store(DrawPrepState::Never, std::memory_order_seq_cst);
    drawPrep.*waited += MillisecondsSince(started);
    return !drawPrep.stop && !drawPrep.cancel.load(std::memory_order_seq_cst);
}

void Driver::runDrawPrep() noexcept {
    PinWorkerThread("draw prep");
    GuestMemory::TagGpuLockThread(0x100);
    std::uint64_t seen = 0;
    std::unique_lock lock(drawPrep.mutex);
    for (;;) {
        drawPrep.changed.wait(lock, [&] { return drawPrep.stop || (drawPrep.submission != nullptr && drawPrep.generation != seen); });
        if (drawPrep.stop) return;
        seen = drawPrep.generation;
        drawPrep.busy = true;
        const auto* submission = drawPrep.submission;
        lock.unlock();
        const auto started = std::chrono::steady_clock::now();
        try {
            walkDrawPrep(*submission);
        } catch (...) {
            ++drawPrep.counts[PrepFailed];
        }
        drawPrep.helperCursor.store(DrawPrepState::Never, std::memory_order_release);
        drawPrep.busyMs += MillisecondsSince(started);
        try {
            reportDrawPrep(false);
        } catch (...) {
        }
        lock.lock();
        drawPrep.busy = false;
        drawPrep.released.notify_all();
    }
}

void Driver::walkDrawPrep(const Submission& submission) {
    auto& scratch = drawPrep.scratch;
    auto& counts = drawPrep.counts;
    const auto& commands = submission.commands;
    const auto ahead = drawPrepAhead();
    std::vector<std::size_t> draws;
    std::vector<RunLabel> labels;
    ++counts[PrepSubmissions];
    const auto stop = [&](DrawPrepCount reason) { ++counts[reason]; };
    for (std::size_t cursor = 0; cursor < commands.size();) {
        if (drawPrep.cancel.load(std::memory_order_relaxed)) return;
        drawPrep.helperCursor.store(cursor, std::memory_order_release);
        const auto header = commands[cursor];
        if (Pm4::FillerPacket(header)) {
            ++cursor;
            continue;
        }
        const auto count = Pm4::PacketWords(header);
        if (count == 0 || cursor + count > commands.size()) return stop(PrepStopOther);
        const auto packet = std::span(commands).subspan(cursor, count);
        const auto opcode = (header >> 8u) & 0xffu;
        const auto next = cursor + count;
        if (Pm4::Predicated(header) && scratch.predication.operation != 0) return stop(PrepStopPredication);
        if (opcode == 0x3f || header == RenderingWaitPacketHeader || header == FlipPacketHeader) {
            cursor = next;
            continue;
        }
        switch (opcode) {
            case 0x10: {
                const auto operation = (header >> 2u) & 0x3fu;
                if (operation != 0 && operation != 0x09 && operation != 0x0a && operation != 0x0b && operation != 0x0c && operation != 0x1a) return stop(PrepStopOther);
                [[fallthrough]];
            }
            case 0x11: case 0x12: case 0x13: case 0x20: case 0x26: case 0x28: case 0x2a: case 0x2f: case 0x69: case 0x76: case 0x79: case 0x7a: case 0x81:
                try {
                    Pm4::Execute(packet, scratch);
                } catch (const std::exception&) {
                    return stop(PrepStopReplay);
                }
                break;
            case 0x63: case 0x64: case 0x9f: {
                const auto list = submission.registerLists.find(cursor);
                if (list == submission.registerLists.end()) return stop(PrepStopReplay);
                try {
                    Pm4::ExecuteIndirectRegisters(packet, list->second, scratch);
                } catch (const std::exception&) {
                    return stop(PrepStopReplay);
                }
                break;
            }
            case 0x37: case 0x49:
                try {
                    if (const auto label = Pm4::DecodeLabelWrite(packet)) NoteLabel(labels, label->address, label->Bytes());
                } catch (const std::exception&) {
                    return stop(PrepStopReplay);
                }
                break;
            case 0x15: case 0x16: case 0x1e: case 0x42: case 0x45: case 0x46: case 0x50: case 0x58: case 0x83:
                break;
            case 0x40:
                if (Pm4::CopyDataToRegister(packet)) return stop(PrepStopRegisterCopy);
                break;
            case 0x3c: case 0x93: {
                if (submission.heldAtSubmit.contains(cursor) || WrittenByRun(packet, labels)) break;
                const auto awaited = packet[2] | (static_cast<std::uint64_t>(packet[3]) << 32u);
                if (GuestMemory::Accessible(reinterpret_cast<const void*>(awaited), Pm4::WaitAwaitedBytes(packet)) && Pm4::WaitSatisfiedUnchecked(packet)) break;
                ++counts[PrepWaitsGated];
                if (!waitWorkerPast(cursor, &DrawPrepState::gatedMs)) return;
                break;
            }
            case 0x24: case 0x25: case 0x27: case 0x2c: case 0x2d: case 0x35: case 0x38: {
                ++counts[PrepDraws];
                const auto index = draws.size();
                draws.push_back(cursor);
                if (index >= ahead && drawPrep.cursor.load(std::memory_order_seq_cst) <= draws[index - ahead]) {
                    ++counts[PrepThrottled];
                    if (!waitWorkerPast(draws[index - ahead / 2], &DrawPrepState::throttledMs)) return;
                }
                const auto worker = drawPrep.cursor.load(std::memory_order_seq_cst);
                if (drawPrepSync() ? worker > cursor : worker >= cursor) {
                    ++counts[PrepTooLate];
                    break;
                }
                const auto reached = static_cast<std::size_t>(std::upper_bound(draws.begin(), draws.end(), worker) - draws.begin());
                drawPrep.drawsAhead = index >= reached ? index - reached : 0;
                if (drawKeyProbe()) {
                    const auto started = std::chrono::steady_clock::now();
                    prepareDraw(submission, packet);
                    keyProbe().prepareNs += static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started).count());
                    ++keyProbe().prepareCalls;
                } else {
                    prepareDraw(submission, packet);
                }
                break;
            }
            case 0x22:
                return stop(PrepStopConditional);
            default:
                return stop(PrepStopOther);
        }
        cursor = next;
    }
}

void Driver::prepRead(std::uint64_t address, std::span<std::byte> out) {
    const std::pair<std::uint64_t, std::uint64_t> range{address, address + out.size()};
    const auto pending = Graphics::Recorder::PendingWriteSnapshot();
    if (Graphics::Recorder::SnapshotOverlaps(pending.get(), address, out.size()) || Graphics::StorageTexture::AnyPendingUnitInside(std::span(&range, 1)) || Graphics::AnyShadowedOverlaps(address, out.size())) {
        auto& prep = Get().drawPrep;
        if (ProfileDraw()) {
            const auto mask = (Graphics::Recorder::SnapshotOverlaps(pending.get(), address, out.size()) ? 1u : 0u) | (Graphics::StorageTexture::AnyPendingUnitInside(std::span(&range, 1)) ? 2u : 0u) | (Graphics::AnyShadowedOverlaps(address, out.size()) ? 4u : 0u);
            ++prep.readAborts[prep.capturing ? 1 : 0][mask];
            ++prep.abortsAhead[std::min<std::size_t>(prep.drawsAhead / 8, prep.abortsAhead.size() - 1)];
        }
        prep.abortReason.store(PrepAbortPending, std::memory_order_relaxed);
        throw DrawPrepAbort{};
    }
    if (GuestMemory::CopyMapped(address, out) != GuestMemory::Compare::Equal) {
        Get().drawPrep.abortReason.store(PrepAbortUnmapped, std::memory_order_relaxed);
        throw DrawPrepAbort{};
    }
}

ShaderMemory::PendingWrite Driver::prepPendingWrite(std::uint64_t address, std::size_t bytes, std::span<std::byte> known) {
    using Policy = ShaderMemory::PendingWrite;
    const auto policy = queryPendingWrite(address, bytes, known);
    if (known.empty() && bytes > sizeof(std::uint32_t)) {
        const std::pair<std::uint64_t, std::uint64_t> range{address, address + bytes};
        if (policy == Policy::None && (Graphics::StorageTexture::AnyPendingUnitInside(std::span(&range, 1)) || Graphics::AnyShadowedOverlaps(address, bytes))) return Policy::Sync;
        return policy;
    }
    if (policy == Policy::None || policy == Policy::Raw || policy == Policy::KnownValue) return policy;
    auto& prep = Get().drawPrep;
    if (ProfileDraw()) {
        ++prep.policyAborts;
        ++prep.abortsAhead[std::min<std::size_t>(prep.drawsAhead / 8, prep.abortsAhead.size() - 1)];
    }
    prep.abortReason.store(PrepAbortPending, std::memory_order_relaxed);
    throw DrawPrepAbort{};
}

bool Driver::entryCurrent(const DrawEntry& entry, const std::vector<DrawProgram>& programs) const {
    if (entry.stages.size() != programs.size()) return false;
    for (std::size_t i = 0; i < programs.size(); ++i) {
        const auto& variants = entry.stages[i];
        if (variants.empty()) {
            if (entry.decode != nullptr && entry.decode->roles.size() > i && entry.decode->roles[i] == ShaderRecompiler::ProgramRole::GeometryBack) continue;
            return false;
        }
        const auto& variant = *variants.front();
        if (variant.userData != programs[i].userData) return false;
        std::vector<ShaderRecompiler::MemoryRegion> regions;
        appendEntryRegions(variant, regions);
        for (const auto& region : regions) {
            if (GuestMemory::CompareMapped(region.guestAddress, region.bytes) != GuestMemory::Compare::Equal) return false;
        }
    }
    return true;
}

void Driver::prepareDraw(const Submission& submission, std::span<const std::uint32_t> packet) {
    using Role = ShaderRecompiler::ProgramRole;
    using Stage = ShaderRecompiler::ShaderStage;
    auto& counts = drawPrep.counts;
    const auto& scratch = drawPrep.scratch;
    if (ShaderRecompiler::DebugProbeActive()) return;
    drawPrep.abortReason.store(0, std::memory_order_relaxed);
    drawPrep.capturing = false;
    ProbeTimer timer;
    try {
        auto drawParameters = Pm4::ResolveDraw(packet, scratch);
        if (drawParameters.indirect) {
            ++counts[PrepIndirect];
            return;
        }
        std::string rejected;
        const bool empty = !drawParameters.indexed && (drawParameters.indexCount == 0 || drawParameters.instanceCount == 0);
        if (empty || Graphics::DecodeColorMetadataPass(scratch) || precheckRegisters(scratch, drawParameters, [&] { return drawPrep.listRestart; }, rejected)) {
            ++counts[PrepPrechecked];
            return;
        }
        std::uint64_t dataShape = 0;
        const auto drawKey = drawRegisterKey(scratch, *submission.shaders, drawPrep.deviceSerial, &dataShape);
        const auto shapeKey = timer.on ? drawShapeKey(scratch, *submission.shaders, drawPrep.deviceSerial) : 0;
        timer.Mark(ProbeKey);
        std::shared_ptr<DrawEntry> entry;
        {
            std::lock_guard cacheLock(drawCacheMutex);
            if (const auto found = drawCache.find(drawKey); found != drawCache.end()) entry = found->second;
        }
        std::shared_ptr<const DrawDecode> decode = entry != nullptr ? entry->decode : nullptr;
        timer.Mark(ProbeLookup);
        if (decode == nullptr) {
            decode = decodeDraw(scratch, submission);
            if (timer.on) ++keyProbe().decodes;
        }
        timer.Mark(ProbeDecode);
        const auto& graphics = decode->state;
        const auto& pixel = decode->pixel;
        const auto& roles = decode->roles;
        std::vector<DrawProgram> programs = decode->programs;
        if (graphics.stages.mesh) {
            auto& words = programs.front().userData;
            require(programs.front().firstUserSgpr == 0 && words.size() >= ShaderRecompiler::MeshIndexBufferUserWord + 4, "mesh program lacks the hidden user words");
            const auto descriptor = Graphics::MeshIndexBufferDescriptor(drawParameters);
            std::copy(descriptor.begin(), descriptor.end(), words.begin() + ShaderRecompiler::MeshIndexBufferUserWord);
        }
        if (entry != nullptr && entryCurrent(*entry, programs)) {
            ++counts[PrepCurrent];
            timer.Mark(ProbeLookup);
            return;
        }
        timer.Mark(ProbeLookup);
        std::vector<ShaderRecompiler::MemoryRegion> memory;
        std::vector<ShaderRecompiler::LinkedProgram> linked;
        for (std::size_t i = 0; i < programs.size(); ++i) {
            for (const auto& region : programs[i].memory) {
                if (!region.bytes.empty()) memory.push_back(region);
            }
            linked.push_back({roles[i], programs[i].binary, programs[i].userDataBase, programs[i].firstUserSgpr, programs[i].userData});
        }
        std::vector<std::optional<ShaderRecompiler::ShaderVertexStageInfo>> vertexInfos(programs.size());
        std::vector<std::vector<Graphics::DecodeRead>> decodeReads(programs.size());
        for (std::size_t i = 0; i < programs.size(); ++i) {
            const auto& program = programs[i];
            if (program.binary.stage == Stage::Fragment || roles[i] == Role::GeometryBack) continue;
            vertexInfos[i] = Graphics::DecodeVertexStageInfo(program.binary.header, program.binary.headerAddress, program.userData, &decodeReads[i], false, 8u - program.firstUserSgpr, &prepRead);
        }
        timer.Mark(ProbeVertexInfo);
        drawPrep.capturing = true;
        ShaderMemory shaderMemory(memory, &prepPendingWrite, nullptr, nullptr, &prepRead);
        std::vector<StageCapture> stageCaptures(programs.size());
        std::uint32_t pushCursor = 0;
        for (std::size_t i = 0; i < programs.size(); ++i) {
            if (roles[i] == Role::GeometryBack) continue;
            const auto& program = programs[i];
            pushCursor = Graphics::StagePushOffset(pushCursor, program.binary.stage, drawPrep.pipelineLibraries);
            const auto request = drawStageRequest(i, pushCursor, programs, graphics, pixel, vertexInfos, memory, linked, drawParameters, *drawPrep.target);
            const auto invocation = TryInvocationFor(*program.snapshot, program.codeOffset, request);
            timer.Mark(ProbeInvocation);
            if (!invocation) {
                ++counts[PrepNotPrepared];
                return;
            }
            auto& stageCapture = stageCaptures[i];
            stageCapture.forgetSerial = GuestMemory::ForgetSerial();
            stageCapture.pushOffset = pushCursor;
            const auto capture = shaderMemory.Capture(*invocation);
            stageCapture.regions = shaderMemory.TakeRecentRegions();
            stageCapture.userDataUse = capture->userDataUse;
            memory = shaderMemory.Regions();
            timer.Mark(ProbeWalk);
            stageCapture.compiled = invocation->Materialize(*capture);
            stageCapture.capture = capture;
            timer.Mark(ProbeMaterialize);
            const auto slotEnd = (pushCursor / Graphics::PipelinePushSlotBytes + 1u) * Graphics::PipelinePushSlotBytes;
            if (stageCapture.compiled->pushConstants.size() > slotEnd - pushCursor) {
                ++counts[PrepFailed];
                return;
            }
            pushCursor += static_cast<std::uint32_t>(stageCapture.compiled->pushConstants.size());
        }
        std::vector<std::shared_ptr<DispatchVariant>> fresh(programs.size());
        bool any = false;
        for (std::size_t i = 0; i < programs.size(); ++i) {
            const auto& stageCapture = stageCaptures[i];
            if (stageCapture.compiled == nullptr || !CacheableResult(*stageCapture.compiled)) continue;
            fresh[i] = drawVariant(stageCapture, programs[i], vertexInfos[i], decodeReads[i]);
            any = true;
        }
        timer.Mark(ProbeVariant);
        if (!any) {
            ++counts[PrepUncacheable];
            return;
        }
        if (timer.on) {
            probeDraw(shapeKey, programs, fresh);
            timer.Mark(ProbeClassify);
        }
        std::vector<std::shared_ptr<DispatchVariant>> parked;
        if (backgroundSpecializeEnabled()) parked = parkSpecializations(drawKey, fresh, stageCaptures, programs, decode, pixel, vertexInfos, drawParameters, *drawPrep.target, drawPrep.deviceSerial, drawPrep.pipelineLibraries);
        if (!insertDrawEntry(drawKey, fresh, decode, true, entry == nullptr, drawDataHits() ? dataShape : 0)) {
            ++counts[PrepRaced];
            return;
        }
        noteParked(parked, fresh);
        timer.Mark(ProbeInsert);
        ++counts[entry == nullptr ? PrepInserted : PrepRefreshed];
    } catch (...) {
        const auto reason = drawPrep.abortReason.exchange(0, std::memory_order_relaxed);
        ++counts[reason != 0 ? reason : static_cast<std::size_t>(PrepFailed)];
    }
}

void Driver::reportDrawPrep(bool final) {
    static const bool profile = std::getenv("APS5_PROFILE_DRAW") != nullptr;
    if (!profile) return;
    const auto now = std::chrono::steady_clock::now();
    if (!final && now - drawPrep.reported < std::chrono::seconds(10)) return;
    const auto& counts = drawPrep.counts;
    const auto count = [&](DrawPrepCount which) { return static_cast<unsigned long long>(counts[which]); };
    AgcDriver::ProfilePrint_nid_no_patch("[draw-prep] (10 s): %llu submissions, %llu draws walked: %llu prepared, %llu refreshed, %llu entries current, %llu too late, %llu indirect, %llu prechecked, %llu without a prepared shader; aborts: %llu pending GPU writes, %llu unmapped, %llu failed, %llu raced, %llu uncacheable; stops: predication %llu, COND_EXEC %llu, register copy %llu, replay %llu, other %llu; waits: %llu gated %.1f ms, %llu throttled %.1f ms; busy %.1f ms\n",
                                         count(PrepSubmissions), count(PrepDraws), count(PrepInserted), count(PrepRefreshed), count(PrepCurrent), count(PrepTooLate), count(PrepIndirect), count(PrepPrechecked), count(PrepNotPrepared),
                                         count(PrepAbortPending), count(PrepAbortUnmapped), count(PrepFailed), count(PrepRaced), count(PrepUncacheable),
                                         count(PrepStopPredication), count(PrepStopConditional), count(PrepStopRegisterCopy), count(PrepStopReplay), count(PrepStopOther),
                                         count(PrepWaitsGated), drawPrep.gatedMs, count(PrepThrottled), drawPrep.throttledMs, drawPrep.busyMs);
    const auto sites = [&](std::size_t phase, unsigned mask) { return static_cast<unsigned long long>(drawPrep.readAborts[phase][mask]); };
    const auto several = [&](std::size_t phase) { return sites(phase, 3) + sites(phase, 5) + sites(phase, 6) + sites(phase, 7); };
    const auto& ahead = drawPrep.abortsAhead;
    AgcDriver::ProfilePrint_nid_no_patch("[draw-prep] pending aborts by site (10 s): vertex reads over a recorded write %llu, a storage image %llu, a shadow %llu, several %llu; capture reads %llu, %llu, %llu, %llu; capture policy %llu; draws ahead of the worker 0-7 %llu, 8-15 %llu, 16-23 %llu, 24-31 %llu, 32-39 %llu, 40+ %llu" "\n",
                                         sites(0, 1), sites(0, 2), sites(0, 4), several(0), sites(1, 1), sites(1, 2), sites(1, 4), several(1), static_cast<unsigned long long>(drawPrep.policyAborts),
                                         static_cast<unsigned long long>(ahead[0]), static_cast<unsigned long long>(ahead[1]), static_cast<unsigned long long>(ahead[2]), static_cast<unsigned long long>(ahead[3]), static_cast<unsigned long long>(ahead[4]), static_cast<unsigned long long>(ahead[5]));
    if (drawKeyProbe()) reportKeyProbe();
    drawPrep.readAborts = {};
    drawPrep.policyAborts = 0;
    drawPrep.abortsAhead = {};
    drawPrep.counts = {};
    drawPrep.gatedMs = drawPrep.throttledMs = drawPrep.busyMs = 0;
    drawPrep.reported = now;
}

}
