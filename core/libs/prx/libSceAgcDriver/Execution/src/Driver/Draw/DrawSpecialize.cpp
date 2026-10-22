#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Queues/WorkerAffinity.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Shaders/ShaderRegistry.hpp"
#include "prx/libSceAgcDriver/Execution/include/TracyZone.hpp"
#include "prx/libc/include/CpuTopology.hpp"
#include "Optimization/ResourceProgram.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <span>
#include <stdexcept>
#include <utility>

namespace AgcDriver::DriverDetail {

namespace {

void RefuseGuestRead(std::uint64_t, std::span<std::byte>) {
    throw std::runtime_error("AGC driver: a background specialization read memory its draw's capture did not");
}

}

bool Driver::backgroundSpecializeEnabled() {
    static const bool enabled = std::getenv("APS5_BACKGROUND_SPECIALIZE") != nullptr && drawEntries();
    return enabled;
}

bool Driver::specializeEagerly() {
    static const bool eager = std::getenv("APS5_SPECIALIZE_ON_HIT") == nullptr;
    return eager;
}

std::size_t Driver::specializeThreads() {
    static const std::size_t threads = [] {
        const char* text = std::getenv("APS5_SPECIALIZE_THREADS");
        const auto parsed = text != nullptr ? std::strtoull(text, nullptr, 10) : 1ull;
        return static_cast<std::size_t>(std::clamp<unsigned long long>(parsed, 1, 16));
    }();
    return threads;
}

void Driver::startDrawSpecialize() {
    try {
        drawSpecialize.threads.reserve(specializeThreads());
        pthread_attr_t attributes;
        if (pthread_attr_init(&attributes) != 0) throw std::runtime_error("the thread attributes could not be initialized");
        pthread_attr_setstacksize(&attributes, 64u << 20u);
        drawSpecialize.stop.store(false);
        for (std::size_t index = 0; index < specializeThreads(); ++index) {
            pthread_t thread;
            if (pthread_create(&thread, &attributes, &Driver::drawSpecializeMain, this) == 0) drawSpecialize.threads.push_back(thread);
        }
        pthread_attr_destroy(&attributes);
        if (drawSpecialize.threads.empty()) throw std::runtime_error("no worker thread could be created");
        std::lock_guard lock(drawSpecialize.mutex);
        drawSpecialize.open.store(true);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[draw-specialize] not started: %s\n", error.what());
    }
}

void Driver::stopDrawSpecialize() noexcept {
    if (drawSpecialize.threads.empty()) return;
    {
        std::lock_guard lock(drawSpecialize.mutex);
        drawSpecialize.open.store(false);
        drawSpecialize.stop.store(true);
        drawSpecialize.queue.clear();
        drawSpecialize.depth.store(0, std::memory_order_relaxed);
    }
    drawSpecialize.changed.notify_all();
    for (const auto thread : drawSpecialize.threads) pthread_join(thread, nullptr);
    drawSpecialize.threads.clear();
    drawSpecialize.idle.notify_all();
}

void Driver::WaitSpecializations() {
    std::unique_lock lock(drawSpecialize.mutex);
    drawSpecialize.idle.wait(lock, [&] { return !drawSpecialize.open.load() || (drawSpecialize.queue.empty() && drawSpecialize.running == 0); });
}

SpecializeCounters Driver::DrawSpecializeCounters() {
    SpecializeCounters result;
    for (std::size_t which = 0; which < SpecializeCountCount; ++which) result.counts[which] = drawSpecialize.counts[which].load(std::memory_order_relaxed);
    result.jobNanoseconds = drawSpecialize.jobNanoseconds.load(std::memory_order_relaxed);
    result.waiting = drawSpecialize.depth.load(std::memory_order_relaxed);
    std::lock_guard cacheLock(drawCacheMutex);
    for (const auto& item : drawCache) {
        for (const auto& variants : item.second->stages) {
            result.specializedVariants += static_cast<std::size_t>(std::count_if(variants.begin(), variants.end(), [](const std::shared_ptr<DispatchVariant>& variant) { return variant->registration != nullptr && !variant->compiled->registrationModule; }));
        }
    }
    return result;
}

void* Driver::drawSpecializeMain(void* driver) {
    static_cast<Driver*>(driver)->runDrawSpecialize();
    return nullptr;
}

void Driver::runDrawSpecialize() noexcept {
    TracyZone::NameThread("draw specialize");
    if (WorkerAffinityMask() != 0) CpuTopology::PinTraced("draw specialize", nullptr, CpuTopology::Get().process);
    std::unique_lock lock(drawSpecialize.mutex);
    for (;;) {
        drawSpecialize.changed.wait(lock, [&] { return drawSpecialize.stop.load() || !drawSpecialize.queue.empty(); });
        if (drawSpecialize.stop.load()) return;
        auto job = std::move(drawSpecialize.queue.back());
        drawSpecialize.queue.pop_back();
        drawSpecialize.depth.store(drawSpecialize.queue.size(), std::memory_order_relaxed);
        ++drawSpecialize.running;
        lock.unlock();
        runSpecializeJob(*job);
        job.reset();
        lock.lock();
        --drawSpecialize.running;
        if (drawSpecialize.queue.empty() && drawSpecialize.running == 0) drawSpecialize.idle.notify_all();
    }
}

void Driver::submitSpecializations(std::vector<std::shared_ptr<SpecializeJob>>& jobs) {
    std::uint64_t queued = 0, displaced = 0;
    {
        std::lock_guard lock(drawSpecialize.mutex);
        for (auto& job : jobs) {
            if (!drawSpecialize.open.load()) continue;
            if (drawSpecialize.queue.size() >= SpecializeState::QueueBound) {
                drawSpecialize.queue.pop_front();
                ++displaced;
            }
            drawSpecialize.queue.push_back(std::move(job));
            ++queued;
        }
        drawSpecialize.depth.store(drawSpecialize.queue.size(), std::memory_order_relaxed);
    }
    if (queued != 0) drawSpecialize.changed.notify_all();
    drawSpecialize.counts[SpecializeQueued].fetch_add(queued, std::memory_order_relaxed);
    drawSpecialize.counts[SpecializeDropped].fetch_add(jobs.size() - queued + displaced, std::memory_order_relaxed);
    jobs.clear();
}

std::shared_ptr<const SpecializeTarget> Driver::specializeTarget(const ShaderRecompiler::SpirvTarget& target, std::uint64_t deviceSerial) {
    std::lock_guard lock(drawSpecialize.targetMutex);
    if (drawSpecialize.target != nullptr && drawSpecialize.targetSerial == deviceSerial) return drawSpecialize.target;
    auto owned = std::make_shared<SpecializeTarget>();
    owned->target = target;
    owned->capabilities.assign(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    for (const auto extension : target.supportedExtensions) owned->extensions.emplace_back(extension);
    for (const auto& extension : owned->extensions) owned->extensionViews.emplace_back(extension);
    owned->target.supportedCapabilities = owned->capabilities;
    owned->target.supportedExtensions = owned->extensionViews;
    owned->target.registrationWordLimit = 1;
    drawSpecialize.target = owned;
    drawSpecialize.targetSerial = deviceSerial;
    return owned;
}

std::vector<std::shared_ptr<DispatchVariant>> Driver::parkSpecializations(std::uint64_t drawKey, const std::vector<std::shared_ptr<DispatchVariant>>& fresh, const std::vector<StageCapture>& stageCaptures, const std::vector<DrawProgram>& programs, const std::shared_ptr<const DrawDecode>& decode, const ShaderRecompiler::ShaderPixelStageInfo& pixel, const std::vector<std::optional<ShaderRecompiler::ShaderVertexStageInfo>>& vertexInfos, const Pm4::DrawParameters& drawParameters, const ShaderRecompiler::SpirvTarget& target, std::uint64_t deviceSerial, bool pipelineLibraries) {
    using Stage = ShaderRecompiler::ShaderStage;
    static const std::vector<ShaderRecompiler::MemoryRegion> noMemory;
    static const std::vector<ShaderRecompiler::LinkedProgram> noLinked;
    std::vector<std::shared_ptr<DispatchVariant>> parked;
    if (decode == nullptr || !pipelineLibraries || !target.registrationModules || !drawSpecialize.open.load(std::memory_order_relaxed) || drawParameters.indirect || ShaderRecompiler::DebugProbeActive()) return parked;
    const auto& graphics = decode->state;
    if (graphics.stages.path != Graphics::ShaderPath::Vertex || graphics.stages.mesh || graphics.stages.tessellation || graphics.rectList || graphics.depthBiasPerFace || Graphics::SplitsAlternateAlpha(graphics)) return parked;
    if (std::any_of(stageCaptures.begin(), stageCaptures.end(), [](const StageCapture& stageCapture) { return stageCapture.compiled != nullptr && stageCapture.compiled->barycentricEmulation.active; })) return parked;
    std::shared_ptr<const SpecializeTarget> owned;
    for (std::size_t i = 0; i < fresh.size(); ++i) {
        const auto& stageCapture = stageCaptures[i];
        const auto& program = programs[i];
        if (fresh[i] == nullptr || stageCapture.compiled == nullptr || !stageCapture.compiled->registrationModule || !CacheableResult(*stageCapture.compiled)) continue;
        if ((program.binary.stage != Stage::Vertex && program.binary.stage != Stage::Fragment) || program.binary.codeAddress == NullPixelProgramAddress()) continue;
        if (owned == nullptr) owned = specializeTarget(target, deviceSerial);
        auto job = std::make_shared<SpecializeJob>();
        job->drawKey = drawKey;
        job->stage = i;
        job->variant = fresh[i];
        job->decode = decode;
        job->target = owned;
        for (const auto& region : stageCapture.regions) {
            job->runs.emplace_back(region.guestAddress, region.guestAddress + region.bytes.size());
            const auto count = region.bytes.size() / sizeof(std::uint32_t);
            const auto offset = job->words.size();
            job->words.resize(offset + count);
            std::memcpy(job->words.data() + offset, region.bytes.data(), count * sizeof(std::uint32_t));
        }
        job->userData = program.userData;
        job->request = drawStageRequest(i, stageCapture.pushOffset, programs, graphics, pixel, vertexInfos, noMemory, noLinked, drawParameters, owned->target);
        job->request.context.userData = job->userData;
        fresh[i]->specialization = std::move(job);
        if (parked.empty()) parked.resize(fresh.size());
        parked[i] = fresh[i];
    }
    return parked;
}

void Driver::noteParked(const std::vector<std::shared_ptr<DispatchVariant>>& parked, const std::vector<std::shared_ptr<DispatchVariant>>& fresh) {
    std::uint64_t kept = 0;
    std::vector<std::shared_ptr<SpecializeJob>> eager;
    {
        std::lock_guard cacheLock(drawCacheMutex);
        for (std::size_t i = 0; i < parked.size() && i < fresh.size(); ++i) {
            if (parked[i] == nullptr || parked[i] != fresh[i]) continue;
            ++kept;
            if (specializeEagerly() && fresh[i]->specialization != nullptr) eager.push_back(std::move(fresh[i]->specialization));
        }
    }
    if (kept != 0) drawSpecialize.counts[SpecializeParked].fetch_add(kept, std::memory_order_relaxed);
    if (!eager.empty()) submitSpecializations(eager);
}

bool Driver::specializeRoom() {
    if (drawSpecialize.open.load(std::memory_order_relaxed) && drawSpecialize.depth.load(std::memory_order_relaxed) < SpecializeState::QueueBound) return true;
    drawSpecialize.counts[SpecializeDropped].fetch_add(1, std::memory_order_relaxed);
    return false;
}

void Driver::runSpecializeJob(SpecializeJob& job) noexcept {
    const auto started = std::chrono::steady_clock::now();
    auto outcome = SpecializeFailed;
    try {
        outcome = specializeStage(job);
    } catch (const std::exception& error) {
        static std::atomic<std::uint64_t> reports{0};
        if (reports.fetch_add(1, std::memory_order_relaxed) < 20) std::fprintf(stderr, "[draw-specialize] stage %zu (program 0x%llx) kept its registration result: %s\n", job.stage, static_cast<unsigned long long>(job.request.shader.codeAddress), error.what());
    } catch (...) {
    }
    drawSpecialize.counts[outcome].fetch_add(1, std::memory_order_relaxed);
    drawSpecialize.counts[SpecializeDone].fetch_add(1, std::memory_order_relaxed);
    drawSpecialize.jobNanoseconds.fetch_add(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started).count()), std::memory_order_relaxed);
}

SpecializeCount Driver::specializeStage(SpecializeJob& job) {
    if (drawSpecialize.stop.load(std::memory_order_relaxed) || !specializationCurrent(job)) return SpecializeStale;
    if (ShaderRecompiler::DebugProbeActive()) return SpecializeFailed;
    const auto handle = PrepareShaderWithDiagnostics(job.request);
    const auto invocation = ShaderRecompiler::PreparedShaderInvocation::TryCreate(job.request, handle);
    if (!invocation) return SpecializeFailed;
    std::vector<ShaderRecompiler::MemoryRegion> memory;
    for (const auto& program : job.decode->programs) {
        for (const auto& region : program.memory) {
            if (!region.bytes.empty()) memory.push_back(region);
        }
    }
    std::size_t offset = 0;
    for (const auto& [begin, end] : job.runs) {
        const auto count = static_cast<std::size_t>((end - begin) / sizeof(std::uint32_t));
        memory.push_back({begin, std::as_bytes(std::span<const std::uint32_t>(job.words).subspan(offset, count))});
        offset += count;
    }
    ShaderMemory shaderMemory(memory, nullptr, nullptr, nullptr, nullptr, &RefuseGuestRead);
    const auto capture = shaderMemory.Capture(*invocation);
    auto result = invocation->Materialize(*capture);
    if (ShaderRecompiler::DebugProbeActive()) return SpecializeFailed;
    if (drawSpecialize.stop.load(std::memory_order_relaxed)) return SpecializeStale;
    if (result->registrationModule) return SpecializeNotSpecialized;
    const auto pushOffset = job.request.layout.pushConstantOffsetBytes;
    const auto slotEnd = (pushOffset / Graphics::PipelinePushSlotBytes + 1u) * Graphics::PipelinePushSlotBytes;
    if (!CacheableResult(*result) || result->pushConstants.size() > slotEnd - pushOffset) return SpecializeFailed;
    return publishSpecialization(job, std::move(result)) ? SpecializePublished : SpecializeStale;
}

bool Driver::specializationCurrent(const SpecializeJob& job) {
    const auto variant = job.variant.lock();
    if (variant == nullptr) return false;
    std::lock_guard cacheLock(drawCacheMutex);
    const auto found = drawCache.find(job.drawKey);
    if (found == drawCache.end() || job.stage >= found->second->stages.size()) return false;
    const auto& variants = found->second->stages[job.stage];
    return std::find(variants.begin(), variants.end(), variant) != variants.end();
}

bool Driver::publishSpecialization(const SpecializeJob& job, std::shared_ptr<const ShaderRecompiler::RecompileResult> result) {
    const auto old = job.variant.lock();
    if (old == nullptr) return false;
    auto variant = std::make_shared<DispatchVariant>();
    variant->runs = old->runs;
    variant->words = old->words;
    variant->forgetSerial = old->forgetSerial;
    variant->compiled = std::move(result);
    variant->registration = old->compiled;
    variant->shader = old->shader;
    variant->pushOffset = old->pushOffset;
    variant->vertexInfo = old->vertexInfo;
    variant->userData = old->userData;
    std::shared_ptr<DrawEntry> previous;
    std::lock_guard cacheLock(drawCacheMutex);
    const auto found = drawCache.find(job.drawKey);
    if (found == drawCache.end() || job.stage >= found->second->stages.size()) return false;
    const auto& entry = *found->second;
    const auto& variants = entry.stages[job.stage];
    const auto position = std::find(variants.begin(), variants.end(), old);
    if (position == variants.end()) return false;
    auto swapped = std::make_shared<DrawEntry>();
    swapped->decode = entry.decode;
    swapped->stages = entry.stages;
    swapped->stages[job.stage][static_cast<std::size_t>(position - variants.begin())] = variant;
    swapped->recipes.store(entry.recipes.load());
    swapped->touched = entry.touched;
    swapped->order = entry.order;
    swapped->prepared = entry.prepared;
    swapped->shape = entry.shape;
    accountDrawVariant(*old, false);
    accountDrawVariant(*variant, true);
    previous = std::exchange(found->second, std::move(swapped));
    return true;
}

}
