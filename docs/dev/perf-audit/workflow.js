export const meta = {
  name: 'anyps5-perf-audit',
  description: 'Deep performance audit of the AnyPS5 AGC graphics driver: map subsystems, adversarially verify, fill gaps, synthesize ranked optimizations',
  phases: [
    { title: 'Map', detail: 'one deep-dive agent per subsystem' },
    { title: 'Verify', detail: 'skeptic re-checks each subsystem\'s critical/high/medium claims' },
    { title: 'Gaps', detail: 'completeness critic + targeted follow-ups' },
    { title: 'Synthesize', detail: 'ranked report with Draw.cpp focus and best-case architecture' },
  ],
}

const COMMON = `You are analyzing the AnyPS5 repository at /home/user/AnyPS5. This is READ-ONLY analysis: do NOT modify, build, or commit anything.

AnyPS5 relinks PS5 executables into native Linux/Windows programs and implements PS5 system libraries natively (no emulator process). Graphics go through core/libs/prx/libSceAgcDriver: the game builds PM4 command buffers (core/libs/prx/libSceAgc), submits them (libSceAgcDriver/Submit), per-queue worker threads interpret PM4 (libSceAgcDriver/Execution), and each draw/dispatch is decoded, its RDNA shaders recompiled to SPIR-V (core/shader/recompiler, cached in memory and on disk), the guest memory the shaders read is captured/validated, and Vulkan work is recorded (libSceAgcDriver/Graphics/src/Recorder.cpp) — much of it under one process-wide recursive lock GuestMemory::GpuMutex() (59 call sites).

USER SYMPTOM: Astro Bot (PPSA21564 / PPSA22520, SDK 9) runs in-game at only 8-9 FPS, and the user observes that host CPU threads and the GPU are both under-utilized. They want to know what limits performance (serialization, CPU<->GPU sync stalls, per-draw CPU cost, readbacks, compile stalls, incomplete/stub functions, ...) and concrete optimizations, with special attention to Draw.cpp (both core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp and core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp).

IMPORTANT FACTS
- There are ~320 APS5_* env vars toggling behavior. Determine each relevant toggle's DEFAULT precisely: "APS5_NO_X" usually means feature X is ON by default; plain "APS5_X" is usually opt-in (OFF by default) — but verify by reading the getenv site. Always describe what the DEFAULT path does. Findings about opt-in debug paths are low-value unless the default is the slow path.
- Profiling infrastructure exists (APS5_PROFILE_DRAW, APS5_PROFILE_GPU, APS5_TRACE_*, GPU-lock profiling enabled under APS5_PROFILE_DRAW). Point out which existing switch measures each issue.
- A typical AAA PS5 frame issues thousands of draws and hundreds of dispatches plus many label writes (RELEASE_MEM/WRITE_DATA) and WAIT_REG_MEM waits; reason in per-frame multiples.
- Host OS is unknown (Linux or Windows); call out platform-specific behavior (#ifdef _WIN32) when it matters.
- Code style: no comments except tech-debt markers; proposals should fit the existing codebase.

HOW TO WORK: Read the assigned files thoroughly (Read with offset/limit for big files; Grep for call sites). Follow calls into other files when needed to understand cost and locking. Every claim must cite file:line and be grounded in code you actually read. Quantify mechanisms where possible ("allocates and fills a width*height*bpp vector per draw", "one vkQueueSubmit + fence wait per label"). Prefer depth over breadth: 6-15 well-evidenced findings beats 40 shallow ones. Your final structured output is consumed by another program, not a human.`

const SUBSYSTEMS = [
  {
    key: 'exec-draw',
    name: 'Execution-side draw path (Execution/src/Driver/Draw/*)',
    files: `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/*.cpp (Draw.cpp, DrawCache.cpp, DrawDecode.cpp, DrawLookup.cpp, DrawPrecheck.cpp, DrawCapture.cpp, DrawRegisterKey.cpp, IndirectDraw.cpp, DrawTiming.cpp, DrawCacheStatistics.cpp), Execution/include/Driver/Draw/*.hpp, Execution/include/Driver/Driver.hpp, Execution/src/Pm4.cpp (ResolveDraw and draw packet parsing), Execution/include/Recipe.hpp, Execution/include/CaptureTrace.hpp`,
    focus: `Walk Driver::draw from the PM4 packet to the Graphics-layer call. For each phase (precheck, register-key lookup, decode, program prepare, vertex-info decode, lookup/compile, capture, validation, record) give the steady-state (cache-warm) per-draw CPU cost: allocations and copies (e.g. "std::vector<DrawProgram> programs = decode->programs;" copies userData every draw), hashing, map lookups, which locks are held (GpuMutex, drawCacheMutex) and for how long. Does the draw cache actually HIT in steady state — what goes into drawRegisterKey; do volatile user-data words make keys miss every frame? What happens on a miss? Is GpuMutex held across the whole Graphics::Draw recording, and could decode/lookup/capture run outside it or on other threads? Check the verify/trace/dump paths are truly off by default. Propose concrete Draw.cpp optimizations.`,
  },
  {
    key: 'graphics-draw',
    name: 'Graphics-side draw recording (Graphics/src/Draw.cpp)',
    files: `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp (ALL 2177 lines — this is the user's focus file), Graphics/include/Draw.hpp, Graphics/include/VertexInput.hpp, Graphics/src/ColorTargetLayout.cpp + include, Graphics/src/ColorTargetTransfer.cpp + include, Graphics/src/GpuColorTransfer.cpp + include, Graphics/src/DepthSurface.cpp + include, Graphics/include/Context.hpp, Graphics/include/State.hpp, Graphics/src/State.cpp`,
    focus: `Per draw, what does recording do: render pass / dynamic rendering begin+end per draw (render-pass churn)? pipeline lookup/creation; descriptor set allocation/update; barriers; color/depth target residency checks; DCC/CMASK fast-clear handling that writes texels on the CPU (e.g. writeTexels allocates color.bytes and calls GuestMemory::Write); writeback of render targets into guest memory (GPU->CPU readback) and when it is triggered; vertex input handling (CPU-side vertex fetch/conversion?), index buffer conversion; indirect draws resolved on the CPU; StorageTexture::FlushPending and GuestMemory::Read/Write calls. Identify anything that is O(target size) or O(buffer size) per draw, any synchronous GPU wait, any CPU readback, and anything that forces the recorder to split/submit batches. Propose concrete Draw.cpp optimizations, ranked.`,
  },
  {
    key: 'recorder',
    name: 'Command recording, submission and pipelines (Recorder, Pipeline)',
    files: `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp (3374 lines) + Graphics/include/Recorder.hpp (770 lines), Graphics/src/Pipeline.cpp + include/Pipeline.hpp, Graphics/src/PipelineCache.cpp + include/PipelineCache.hpp, Graphics/include/Shaders.hpp`,
    focus: `Command-buffer lifecycle: how many draws go into one command buffer/batch; when vkQueueSubmit happens (per packet? per label? per draw? per flip?); timeline semaphores and fences; which waits block a worker thread WHILE HOLDING GpuMutex (see NoteLockedGpuWait / "locked GPU waits"); ReleaseUnderLock / ReleaseOnUnlock / HookLockedWait semantics; barrier generation (full barriers? APS5_FULL_BARRIERS, barrier elision); render-pass merging; synchronous vkCreateGraphicsPipelines/vkCreateComputePipelines on the worker on cache miss (stutter) and whether a VkPipelineCache is persisted to disk; dynamic state usage; descriptor management. Is everything on a single VkQueue? Is GPU execution overlapped with CPU recording of the next batch, or does the CPU wait for the GPU each batch (lockstep)?`,
  },
  {
    key: 'memory-tracking',
    name: 'Guest memory tracking, GpuMutex, captures and write evidence',
    files: `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp (1854 lines) + Execution/include/GuestMemory.hpp, Execution/include/WriteWatchCoverage.hpp, Execution/include/MemoryAccessScope.hpp, Execution/src/ShaderMemory.cpp + include/ShaderMemory.hpp, Execution/src/Driver/Memory/{WriteEvidence,WrittenBuffers,CaptureValidation,CaptureDiagnostics}.cpp + include/Driver/Memory/*.hpp, Execution/src/Driver/DeviceAccess.cpp + include`,
    focus: `How guest CPU writes are detected (mprotect + SIGSEGV? Windows GetWriteWatch? userfaultfd/soft-dirty? byte-compare snapshots?) and the per-frame cost (page faults per frame, full-range memcmp of captured regions, hashing); GpuMutexType implementation (recursive? fair? spin?) and how long it is held; Read/Write paths that call FlushGpuWrites (forcing GPU->CPU sync when the CPU reads); 64 KiB tracker-block granularity and false sharing between CPU-written and GPU-read data; snapshot reuse; how "capture" of shader-read memory works and whether each draw re-reads/hashes large regions. Windows: host imports compared instead of write-watched (see docs/dev/TechnicalDebt.md) which disables snapshot reuse. Quantify per-draw and per-frame costs.`,
  },
  {
    key: 'buffers',
    name: 'Guest buffers, uploads, host imports, writebacks, copy/fill HLE',
    files: `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp (2796 lines) + include/GuestBufferMemory.hpp, Graphics/src/BufferPool.cpp + include, Graphics/src/BdaResources.cpp + include, Graphics/src/Resources.cpp + include/Resources.hpp, Graphics/src/BufferAddress.cpp, Execution/src/Driver/Memory/{BufferCopy,BufferFill}.cpp + headers`,
    focus: `How guest memory becomes Vulkan buffers: host-pointer import (VK_EXT_external_memory_host) vs mirror copies vs staging uploads; per-draw and per-frame upload volumes; when GPU-written buffers are written back to guest memory and whether that is synchronous (waits for GPU); BDA tables; pool sizes and defaults (APS5_STAGING_POOL_MIB, APS5_HOST_IMPORT_MIB, APS5_HEAP_MIRROR_MIB, APS5_HEAP_CACHE_MIB, APS5_WORKING_SET_MIB); mirror refresh threading (APS5_MIRROR_REFRESH_THREADS — parallel by default?); NVIDIA Linux import restrictions (read-only ranges copied instead); copy/fill HLE in BufferCopy/BufferFill done on CPU vs GPU and their locking. Identify redundant re-uploads, whole-heap copies, and synchronous readbacks.`,
  },
  {
    key: 'shader-resources',
    name: 'Shader resource binding: descriptors, samplers, validation',
    files: `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp (2946 lines) + include/ShaderResources.hpp, Graphics/src/ShaderInputState.cpp + include, Graphics/src/ShaderValidation.cpp, Graphics/src/Sampler.cpp + include, Graphics/src/GuestSamplerResource.cpp + include, Graphics/src/GuestTextureResource.cpp + include, Graphics/src/ShaderAddressResources.cpp, Graphics/src/TextureFormat.cpp + include`,
    focus: `Per-draw descriptor-set allocation (pool chain), vkUpdateDescriptorSets volume, resource cache behavior (APS5_NO_RESOURCE_CACHE, APS5_RESOURCE_CACHE_ENTRIES, APS5_NO_DRAW_RESOURCE_CACHE), sampler cache, bindless image tables (APS5_BINDLESS_SLOTS, APS5_NO_BINDLESS_IMAGES), per-draw validation cost (ShaderValidation — does it run every draw?), descriptor counts, and hashing costs. Opportunities: push descriptors, descriptor buffers, descriptor templates, persistent sets keyed by content, avoiding re-resolving T#/V#/S# per draw.`,
  },
  {
    key: 'textures',
    name: 'Textures, detiling, DCC metadata, unit shadows, render-target aliasing',
    files: `core/libs/prx/libSceAgcDriver/Graphics/src/Texture.cpp (3767 lines) + include/Texture.hpp, Graphics/src/TextureCache.cpp + include, Graphics/src/TextureDetiler.cpp + include, Graphics/src/TextureDetilerDescriptors.cpp, Graphics/src/TextureTiling.cpp + include, Graphics/include/TextureSwizzleEquations.hpp, Graphics/src/DccMetadata.cpp + include, Graphics/src/UnitShadow.cpp + include, core/libs/prx/libSceAgcDriver/Graphics/shaders/*`,
    focus: `Texture upload/detiling on CPU vs GPU compute (is GPU detiling the default?); hashing texture contents per frame to detect changes (APS5_NO_TEXTURE_HASH, APS5_NO_TEXTURE_CACHE); render-target -> texture aliasing (render-to-texture): does sampling a just-rendered target force writeback to guest memory and re-upload/detile, or is the resident VkImage reused?; DCC key reading/writing on the CPU; what UnitShadow is and its per-frame cost; StorageTexture::FlushPending (who calls it, does it wait for the GPU); image layout transitions; cache eviction; locks held. Identify any O(texture size) CPU work per frame for textures that did not change.`,
  },
  {
    key: 'threading-sync',
    name: 'Queue workers, submission, packets, labels, WAIT_REG_MEM, ordering',
    files: `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Queues/{QueueWorker,Submission,WorkerAffinity}.cpp + include/Driver/Queues/*.hpp, Execution/src/Driver/{Lifecycle,EntryPoints,Diagnostics}.cpp, Execution/src/Driver/Packets/*.cpp + headers, Execution/src/Driver/Synchronization/*.cpp + headers (WaitMemory, DeferredLabels, LabelHistory, SynchronizationStatistics, WaitStatistics), Execution/src/WorkerSampler.cpp, libSceAgcDriver/Submit/src/*.cpp, libSceAgcDriver/Eq/src/*.cpp, Execution/include/QueueState.hpp, Execution/src/Driver/Dispatch/Dispatch.cpp (locking only)`,
    focus: `How many worker threads exist (one per guest queue: graphics + ACB compute queues?) and what each does; ordering constraints between queues (orderReleased, waitFree, queue0Before) that serialize queues; WAIT_REG_MEM handling (busy polling with 1 ms sleeps / PollSleep on Windows? condvar?); label writes (RELEASE_MEM/WRITE_DATA/EOP): does each label force a submit and a wait for GPU completion before the value is visible to the CPU (lockstep)?; deferred labels and batching (APS5_NO_LABEL_BATCHING, APS5_BATCH_CAP, APS5_LABEL_FLUSH_US defaults); reapCompletionLabels polling; flip waits (waitForFlipRoom); SuspendPoint; Submit copying command buffers (copyCommands/readRegisterLists) on the game's thread; EOP interrupts -> equeue events. Determine whether workers effectively run one at a time because of GpuMutex, and whether the game's render/submit thread blocks on worker progress.`,
  },
  {
    key: 'device-present',
    name: 'Vulkan device, queues, presentation, VideoOut flip/vblank',
    files: `core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp (3737 lines) + Execution/include/VulkanDevice.hpp, Execution/src/Driver/Presentation.cpp, Execution/src/{PresentationScaler,DisplayBuffer,DisplayFormat,AspectFit,BdaFeatures,SubgroupClock}.cpp + include/{Presentation,PresentationScaler,DisplayBuffer,VideoOutput,DisplayFormat}.hpp, core/libs/prx/libSceVideoOut/src/{Output,VideoOutDriver,Buffer,Event,DisplayWindow}.cpp`,
    focus: `Which Vulkan queues/families are created and actually used (single graphics queue? async compute/transfer queue used?); features/extensions enabled (dynamic rendering, extended dynamic state, timeline semaphores, descriptor buffers, external_memory_host, graphics pipeline library, pipeline creation cache control); present mode (FIFO/MAILBOX/IMMEDIATE) and swapchain image count; how a guest flip becomes a present (CPU copy/detile of the display buffer? GPU blit? DCC display buffers?); presenter thread; vblank emulation timing and whether it can cap/stall the game; waits on present/acquire fences while holding GpuMutex; any vkDeviceWaitIdle/vkQueueWaitIdle/vkWaitForFences in hot paths; CopyBuffer/CopyOutcome paths; memory type choices (host-visible device-local? BAR?).`,
  },
  {
    key: 'shader-compile',
    name: 'Shader recompiler: compile placement, caching, and generated-SPIR-V GPU efficiency',
    files: `core/shader/recompiler/{Recompiler.cpp,Recompiler.hpp,ShaderDiskCache.cpp,ShaderDiskCache.hpp,CacheKey.hpp,CompiledVariant.hpp,ThreadOwned.hpp,Deserialize.cpp}, core/shader/recompiler/SpirvBackend/src/SpirvOptimizer.cpp, SpirvBackend/src/SpirvModuleEmitter.cpp (skim for perf-relevant codegen), SpirvBackend/src/SpirvMemory/*, Optimization/src/ResourceMaterializer.cpp and DescriptorBindingBuilder.cpp (skim), core/libs/prx/libSceAgcDriver/Execution/src/Driver/Shaders/*.cpp, Execution/src/Driver/Dispatch/{DispatchLookup,DispatchInsert,DispatchCache,VariantValidation}.cpp, core/shader/recompiler/CMakeLists.txt`,
    focus: `(A) CPU side: when/where recompilation runs (synchronously on a queue worker while holding GpuMutex?), memo and disk-cache behavior in steady state (do keys include data that changes every frame -> recompile or re-validate each frame? "data hits", "liveWords", "patched"), per-draw/dispatch cache-validation cost, any background/parallel compile thread pool (none?). (B) GPU side: quality of the generated SPIR-V: vertex fetch via storage buffers/BDA instead of vertex input; bounds checks on every buffer access; wave64 emulated on 32-lane subgroups; EXEC-mask emulation overhead (every instruction predicated?); scalar registers as private arrays (register pressure / spilling); LDS emulation; SPIRV-Tools optimization disabled by default (ANYPS5_ENABLE_SPIRV_TOOLS OFF; APS5_SPIRV_OPT) so drivers get unoptimized SPIR-V; structured control flow quality. Estimate qualitative GPU cost multipliers and propose improvements.`,
  },
  {
    key: 'guest-libs-build',
    name: 'Guest-side CPU libraries, kernel waits, stubs/incomplete functions, build configuration',
    files: `core/libs/prx/libSceAgc/** (DcbDraw, DcbState, Command, Cb, Patch, Shader — per-draw command building on the game thread), core/libs/prx/libkernel/{Time,Pthread/src,Equeue,EventFlag/src,Semaphore/src,SyncOnAddress/src}, core/libs/prx/libSceFiber, core/libs/prx/libSceUlt, core/libs/prx/libSceAudioOut (blocking pacing), core/libs/prx/libc/include/CpuTopology.hpp (+ its .cpp), CMakeLists.txt, core/libs/CMakeLists.txt, core/libs/prx/libSceAgcDriver/CMakeLists.txt, core/shader/recompiler/CMakeLists.txt, docs/dev/TechnicalDebt.md (grep PPSA21564, PPSA22520, PPSA01325, Astro, and perf-relevant silent stubs), core/libs/prx/**/Unimplemented.cpp and NotImplemented_nid_no_patch usage`,
    focus: `(A) Game-thread CPU cost of sceAgc* DCB builders (heavy validation, logging — e.g. APS5_AGC_CREATE_LOG defaults ON -> stderr print per sceAgcCreateShader, exceptions, map lookups). (B) Kernel waits: timed waits/usleep/nanosleep/sceKernelUsleep implementation (coarse sleeps? polling? APS5_COARSE_TIMED_WAITS), condvar timeouts on Windows (timer resolution; APS5_NO_TIMER_RESOLUTION), clock precision, fibers/ULT implementation cost, guest thread priority/affinity ignored, CPU count/affinity reported to the game (sceKernelGetAvailableCpumask 0x1FFF = 13 CPUs) vs host cores -> oversubscription with spin-waiting job systems. (C) Incomplete/stubbed functions: which NotImplemented stubs or silent stubs could Astro Bot hit per frame, and whether they affect performance (NotImplemented throws and terminates -> crash, not slowdown; silent stubs may force slow fallback paths) — answer the user's question "is it the not-completed functions that block performance?" with evidence. (D) Build config: default CMAKE_BUILD_TYPE when unset (no optimization?), optimization flags, LTO, -march, static libstdc++, -fno-asynchronous-unwind-tables, APS5_ENABLE_TIMING_LOG; recommend concrete build settings.`,
  },
  {
    key: 'frame-critical-path',
    name: 'Cross-cutting: end-to-end frame critical path and thread utilization',
    files: `Start at core/libs/prx/libSceAgcDriver/Submit/src/{Dcb,Acb,CommandBuffer}.cpp -> Execution/src/Driver/Queues/Submission.cpp (Driver::Submit/enqueue) -> Execution/src/Driver/Queues/QueueWorker.cpp (run) -> Execution/src/Driver/Packets/PacketExecution.cpp (execute) -> draw (Execution/src/Driver/Draw/Draw.cpp -> Graphics/src/Draw.cpp -> Graphics/src/Recorder.cpp), dispatch (Execution/src/Driver/Dispatch/Dispatch.cpp), labels (Packets/PacketMemory.cpp, Synchronization/DeferredLabels.cpp), WAIT_REG_MEM (Synchronization/WaitMemory.cpp), flip (Execution/src/Driver/Presentation.cpp, Execution/src/VulkanDevice.cpp present path, core/libs/prx/libSceVideoOut/src/*), back to the game observing flip/EOP events (Eq/src/Event.cpp, libkernel/Equeue)`,
    focus: `Trace ONE frame end to end and build a timeline: which thread does what, and EVERY point where a thread blocks (mutex, condvar, fence/semaphore wait, sleep/poll, vkQueueSubmit, vkAcquireNextImageKHR, vkQueuePresentKHR) and what it waits for. Identify the critical path that bounds frame time; explain mechanistically why both host CPU cores and the GPU would show low utilization (e.g. lockstep CPU->GPU->CPU round trips for labels/waits, one global lock serializing all queue workers and the presenter, 1 ms polling sleeps, CPU readbacks); estimate how many CPU<->GPU round trips per frame a typical AAA frame triggers. Finish with the top 5 structural changes that would break the critical path, each with the files to change.`,
  },
]

const FINDINGS_SCHEMA = {
  type: 'object',
  properties: {
    subsystem: { type: 'string' },
    howItWorks: { type: 'string', description: 'Concise description of the steady-state hot path in this subsystem (<= 250 words)' },
    threadingAndLocks: { type: 'string', description: 'Which threads run this code, which locks are held, for how long, and what blocks (<= 200 words)' },
    findings: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          id: { type: 'string', description: 'short unique id like exec-draw-3' },
          title: { type: 'string' },
          location: { type: 'string', description: 'file:line or file:line-line (repo-relative)' },
          category: { type: 'string', enum: ['global-lock-serialization', 'cpu-gpu-sync-stall', 'per-draw-cpu-cost', 'memory-tracking', 'upload-readback', 'allocation-copy', 'shader-pipeline-compile', 'gpu-side-efficiency', 'polling-sleep-latency', 'logging-debug-overhead', 'incomplete-implementation', 'build-config', 'other'] },
          mechanism: { type: 'string', description: 'What the code does and why it costs time' },
          evidence: { type: 'string', description: 'Quoted/paraphrased code with line numbers proving the claim, including env-var default analysis' },
          onDefaultPath: { type: 'boolean' },
          impact: { type: 'string', enum: ['critical', 'high', 'medium', 'low'] },
          whyImpact: { type: 'string' },
          proposal: { type: 'string', description: 'Concrete change: what to change, where, and how' },
          effort: { type: 'string', enum: ['S', 'M', 'L', 'XL'] },
          risk: { type: 'string', description: 'Correctness/coherence risk of the proposal' },
          howToMeasure: { type: 'string', description: 'Existing APS5_* switch / log line that measures it, or what instrumentation to add' },
        },
        required: ['id', 'title', 'location', 'category', 'mechanism', 'evidence', 'onDefaultPath', 'impact', 'whyImpact', 'proposal', 'effort', 'risk', 'howToMeasure'],
      },
    },
    incompleteOrStubbed: {
      type: 'array',
      items: {
        type: 'object',
        properties: { location: { type: 'string' }, what: { type: 'string' }, perfRelevance: { type: 'string' } },
        required: ['location', 'what', 'perfRelevance'],
      },
    },
    quickExperiments: {
      type: 'array',
      description: 'Existing env vars / build flags the user can try today without code changes',
      items: {
        type: 'object',
        properties: { setting: { type: 'string' }, expectedEffect: { type: 'string' }, risk: { type: 'string' } },
        required: ['setting', 'expectedEffect', 'risk'],
      },
    },
  },
  required: ['subsystem', 'howItWorks', 'threadingAndLocks', 'findings', 'incompleteOrStubbed', 'quickExperiments'],
}

const VERDICT_SCHEMA = {
  type: 'object',
  properties: {
    verdicts: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          id: { type: 'string' },
          verdict: { type: 'string', enum: ['confirmed', 'partially-confirmed', 'refuted'] },
          correctedImpact: { type: 'string', enum: ['critical', 'high', 'medium', 'low', 'none'] },
          onDefaultPath: { type: 'boolean' },
          reasoning: { type: 'string' },
          correctedProposal: { type: 'string', description: 'Better or corrected proposal; empty if the original is fine' },
          extraEvidence: { type: 'string' },
        },
        required: ['id', 'verdict', 'correctedImpact', 'onDefaultPath', 'reasoning', 'correctedProposal', 'extraEvidence'],
      },
    },
    missedIssues: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          title: { type: 'string' }, location: { type: 'string' }, mechanism: { type: 'string' },
          impact: { type: 'string', enum: ['critical', 'high', 'medium', 'low'] }, proposal: { type: 'string' },
        },
        required: ['title', 'location', 'mechanism', 'impact', 'proposal'],
      },
    },
  },
  required: ['verdicts', 'missedIssues'],
}

const GAPS_SCHEMA = {
  type: 'object',
  properties: {
    gaps: {
      type: 'array',
      maxItems: 4,
      items: {
        type: 'object',
        properties: {
          title: { type: 'string' },
          question: { type: 'string', description: 'Precise question to answer from code' },
          whyItMatters: { type: 'string' },
          filesToRead: { type: 'string' },
        },
        required: ['title', 'question', 'whyItMatters', 'filesToRead'],
      },
    },
  },
  required: ['gaps'],
}

const mapPrompt = (s) => `${COMMON}

YOUR SUBSYSTEM: ${s.name}
FILES (paths under core/libs/prx/libSceAgcDriver unless given in full): ${s.files}

FOCUS QUESTIONS:
${s.focus}

Return the structured output. Use ids prefixed with "${s.key}-". For each finding set onDefaultPath honestly. Include incomplete/stubbed code you noticed that matters for performance, and quickExperiments (existing env vars/build flags worth trying, with the exact value to set).`

const verifyPrompt = (s, m, toCheck) => `You are a skeptical senior GPU-driver / console-emulation engineer reviewing performance claims about AnyPS5 (/home/user/AnyPS5). READ-ONLY: do not modify, build, or commit anything.

Context: AnyPS5 implements the PS5 AGC graphics driver natively (core/libs/prx/libSceAgcDriver): PM4 command buffers are interpreted by per-queue worker threads, shaders are recompiled RDNA->SPIR-V, guest memory read by shaders is captured/validated, Vulkan work is recorded mostly under one global recursive lock GuestMemory::GpuMutex(). User symptom: Astro Bot runs at 8-9 FPS in-game with under-utilized CPU threads and GPU. ~320 APS5_* env vars change behavior; "APS5_NO_X" usually means X is ON by default, plain "APS5_X" usually opt-in — verify at the getenv site.

Another analyst produced these findings for subsystem "${s.name}" (their summary of how it works: ${m.howItWorks}
Threading/locks: ${m.threadingAndLocks}).

For EACH finding below, open the cited code and try to REFUTE it:
1. Does the code at the cited location actually do what is claimed? (wrong line, misread condition, dead code, misread default)
2. Is it on the DEFAULT path in steady-state gameplay (caches warm), or only behind an opt-in env var, first use, or a rare event?
3. Is it already mitigated by another default-on mechanism (caches, memo, batching, deferral)?
4. Is the impact level plausible per frame for a AAA title with thousands of draws? Correct it.
5. Is the proposal correct and safe for guest-memory coherence / GPU ordering semantics? Give a better one if not.
Default to "refuted" if you cannot confirm it from code. Then list up to 3 significant issues in this subsystem the analyst MISSED (with file:line evidence).

FINDINGS TO CHECK (JSON):
${JSON.stringify(toCheck, null, 1)}`

phase('Map')
log(`Mapping ${SUBSYSTEMS.length} subsystems; each is adversarially verified as soon as its map finishes`)

const results = await pipeline(
  SUBSYSTEMS,
  (s) => agent(mapPrompt(s), { label: `map:${s.key}`, phase: 'Map', schema: FINDINGS_SCHEMA }),
  async (m, s) => {
    if (!m) return null
    const toCheck = (m.findings || []).filter(f => f.impact !== 'low').map(f => ({
      id: f.id, title: f.title, location: f.location, category: f.category, mechanism: f.mechanism,
      evidence: f.evidence, onDefaultPath: f.onDefaultPath, impact: f.impact, proposal: f.proposal,
    }))
    if (!toCheck.length) return { key: s.key, name: s.name, map: m, verify: { verdicts: [], missedIssues: [] } }
    const v = await agent(verifyPrompt(s, m, toCheck), { label: `verify:${s.key}`, phase: 'Verify', schema: VERDICT_SCHEMA })
    return { key: s.key, name: s.name, map: m, verify: v || { verdicts: [], missedIssues: [] } }
  },
)

const done = results.filter(Boolean)
const failedKeys = SUBSYSTEMS.map(s => s.key).filter(k => !done.find(d => d.key === k))
if (failedKeys.length) log(`WARNING: no result for subsystems: ${failedKeys.join(', ')}`)

const merged = done.map(r => {
  const byId = Object.fromEntries((r.verify.verdicts || []).map(v => [v.id, v]))
  return {
    key: r.key,
    subsystem: r.name,
    howItWorks: r.map.howItWorks,
    threadingAndLocks: r.map.threadingAndLocks,
    findings: (r.map.findings || []).map(f => {
      const v = byId[f.id]
      return {
        id: f.id, title: f.title, location: f.location, category: f.category, mechanism: f.mechanism,
        evidence: f.evidence, impactClaimed: f.impact, proposal: f.proposal, effort: f.effort, risk: f.risk, howToMeasure: f.howToMeasure,
        verification: v ? { verdict: v.verdict, impact: v.correctedImpact, onDefaultPath: v.onDefaultPath, reasoning: v.reasoning, correctedProposal: v.correctedProposal, extraEvidence: v.extraEvidence } : { verdict: f.impact === 'low' ? 'unverified-low' : 'unverified', impact: f.impact, onDefaultPath: f.onDefaultPath },
      }
    }),
    missedIssuesFoundByVerifier: r.verify.missedIssues || [],
    incompleteOrStubbed: r.map.incompleteOrStubbed || [],
    quickExperiments: r.map.quickExperiments || [],
  }
})

const counts = merged.flatMap(m => m.findings).reduce((acc, f) => { acc[f.verification.verdict] = (acc[f.verification.verdict] || 0) + 1; return acc }, {})
log(`Verification tally: ${JSON.stringify(counts)}`)

phase('Gaps')
const overview = merged.map(m => ({
  subsystem: m.subsystem, howItWorks: m.howItWorks, threadingAndLocks: m.threadingAndLocks,
  findings: m.findings.map(f => `${f.id} [${f.verification.verdict}/${f.verification.impact}] ${f.title} @ ${f.location}`),
  missed: m.missedIssuesFoundByVerifier.map(x => `${x.title} @ ${x.location} [${x.impact}]`),
}))
const critic = await agent(`${COMMON}

You are the completeness critic for a performance audit. Below is an overview of what ${merged.length} subsystem analysts found (with verifier verdicts). Identify up to 4 GAPS whose answers are most important for explaining 8-9 FPS with under-utilized CPU threads and GPU and for choosing optimizations — e.g. a critical claim still unverified or contradicted between subsystems, a hot path nobody read (check the repo yourself with Glob/Grep for code paths missing from the overview), an unclear default, or an unquantified per-frame round-trip count. Each gap must be answerable by reading code in this repo. Do not list gaps that need running the game.

OVERVIEW (JSON):
${JSON.stringify(overview, null, 1)}`, { label: 'critic', phase: 'Gaps', schema: GAPS_SCHEMA })

const gaps = (critic && critic.gaps) ? critic.gaps.slice(0, 4) : []
log(`Critic raised ${gaps.length} gap(s): ${gaps.map(g => g.title).join(' | ')}`)

const gapResults = (await parallel(gaps.map((g, i) => () => agent(`${COMMON}

GAP INVESTIGATION: ${g.title}
QUESTION: ${g.question}
WHY IT MATTERS: ${g.whyItMatters}
START WITH: ${g.filesToRead}

Answer the question rigorously from code and return findings in the structured format (ids prefixed "gap${i + 1}-"). In howItWorks, give the direct answer to the question.`, { label: `gap:${i + 1}`, phase: 'Gaps', schema: FINDINGS_SCHEMA })))).filter(Boolean)

phase('Synthesize')
const report = await agent(`You are the lead engineer writing the final performance-analysis report for the AnyPS5 maintainer/user (repo at /home/user/AnyPS5; you may Read/Grep files to double-check specifics, READ-ONLY). Write in clear technical English.

USER QUESTION (verbatim): "analyze this repository i forked and see how we can optimize parts of code especially Draw.cpp, and propose optimizations and best possible scenario - form my testing a game Astro Bot, ingame its about 8-9FPS and seems we are not using all our threads and GPU properly - be thorough and find whats the issue/or whether its the not completed funcitons which block also performance"

INPUT: subsystem analyses with adversarial verification verdicts (confirmed / partially-confirmed / refuted / unverified), verifier-found missed issues, and gap investigations. Treat "refuted" findings as wrong (list them only in the appendix). Prefer verifier corrected impact and corrected proposals. Resolve contradictions between subsystems by checking code yourself.

Write a Markdown report with these sections:
1. **TL;DR** — the 3-6 most likely root causes of 8-9 FPS with under-utilized CPU+GPU, ranked, each with confidence (high/medium/low) and one-line evidence (file:line).
2. **How a frame flows today** — threads, queues, the global GpuMutex, where work serializes, where CPU waits on GPU and vice versa (a compact ASCII timeline or numbered list).
3. **Why threads and GPU are under-utilized** — mechanistic explanation tied to code.
4. **Is it the unfinished functions?** — direct answer with evidence (throwing stubs vs silent stubs vs approximations), and which incomplete parts DO cost performance.
5. **Draw.cpp optimizations** — separate subsections for Graphics/src/Draw.cpp and Execution/src/Driver/Draw/Draw.cpp; ranked, each with location, mechanism, concrete change, expected effect, risk, effort.
6. **Full optimization backlog** — Markdown table (ID, title, location, impact, effort, risk, verdict) grouped into: (a) Try today: env vars / build flags (exact settings), (b) Quick code wins (S/M effort), (c) Structural changes (L/XL).
7. **Best possible scenario** — target architecture (e.g. lock decomposition, decoupling CPU timeline from GPU completion, async compile, persistent pipeline cache, GPU-side detile/clears, multithreaded recording) with realistic, honestly-hedged expectations of what each stage could unlock. Do NOT invent FPS numbers; reason about bottleneck shifts instead.
8. **Measurement plan** — exact env vars to run Astro Bot with (e.g. APS5_PROFILE_DRAW=1 ...), which log lines to read, and a decision tree mapping observations to which backlog items to do first.
9. **Appendix** — refuted/low-value claims (one line each) so nobody chases them; list of relevant env toggles with defaults.

Cite file:line for every concrete claim. Be specific and actionable; no filler.

DATA (JSON):
${JSON.stringify({ subsystems: merged, gapInvestigations: gapResults }, null, 1)}`, { label: 'synthesize', phase: 'Synthesize' })

return { report, merged, gapResults, counts, failedKeys }
