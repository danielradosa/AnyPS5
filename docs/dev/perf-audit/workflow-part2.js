export const meta = {
  name: 'anyps5-perf-audit-part2',
  description: 'Two remaining AnyPS5 perf-audit analyses: end-to-end frame critical path with threading/sync, and guest libraries/stubs/build config',
  phases: [{ title: 'Analyze', detail: 'two parallel read-only deep dives' }],
}

const DIGEST = '/tmp/claude-0/-home-user-AnyPS5/0608cc09-de1d-5c69-9397-0a374ff7995d/scratchpad/digest.txt'

const COMMON = `You are analyzing the AnyPS5 repository at /home/user/AnyPS5. This is READ-ONLY analysis: do NOT modify, build, or commit anything.

AnyPS5 relinks PS5 executables into native Linux/Windows programs and implements PS5 system libraries natively (no emulator process; guest addresses are host process addresses). Graphics go through core/libs/prx/libSceAgcDriver: the game builds PM4 command buffers (core/libs/prx/libSceAgc), Driver::Submit copies them (Execution/src/Driver/Queues/Submission.cpp), one host worker thread per guest queue interprets PM4 (QueueWorker.cpp, Packets/PacketExecution.cpp), each draw/dispatch is decoded, its RDNA shaders recompiled to SPIR-V and cached, the guest memory shaders read is captured/validated, and Vulkan work is recorded (Graphics/src/Recorder.cpp) under one process-wide recursive lock GuestMemory::GpuMutex().

USER SYMPTOM: Astro Bot (PPSA21564 / PPSA22520, SDK 9) runs in game at 8-9 FPS, and host CPU threads and the GPU are both under-used.

EARLIER RESULTS: six other analysts already covered the draw path, Graphics Draw.cpp, Recorder/pipelines, memory tracking, buffers and shader resources. Read their digest FIRST: ${DIGEST} (threads/locks summary and critical/high claims per subsystem; full detail in docs/dev/perf-audit/FINDINGS.md if you need one specific finding — grep it, do not read it whole). Do not re-derive those; build on them, cite their ids where relevant, and say so if code you read contradicts one.

IMPORTANT FACTS
- ~320 APS5_* env vars toggle behavior. "APS5_NO_X" usually means X is ON by default; plain "APS5_X" is usually opt-in — verify at the getenv site. Always describe the DEFAULT path.
- Host OS unknown (Linux or Windows): call out #ifdef _WIN32 differences that matter.
- Reason in per-frame multiples (thousands of draws, hundreds of dispatches, many label writes and WAIT_REG_MEM waits per AAA frame).

BUDGET: cloud credits are limited. Be economical: use Grep to locate code and Read with offset/limit around the relevant lines instead of reading whole large files; aim for roughly 40-60 tool calls in total; stop once your questions are answered with evidence. Every claim must cite file:line from code you actually read. 6-12 well-evidenced findings beat many shallow ones. Your structured output is consumed by a program.`

const FINDING = {
  type: 'object',
  properties: {
    id: { type: 'string' },
    title: { type: 'string' },
    location: { type: 'string' },
    category: { type: 'string', enum: ['global-lock-serialization', 'cpu-gpu-sync-stall', 'per-draw-cpu-cost', 'memory-tracking', 'upload-readback', 'allocation-copy', 'shader-pipeline-compile', 'gpu-side-efficiency', 'polling-sleep-latency', 'logging-debug-overhead', 'incomplete-implementation', 'build-config', 'other'] },
    mechanism: { type: 'string' },
    evidence: { type: 'string' },
    onDefaultPath: { type: 'boolean' },
    impact: { type: 'string', enum: ['critical', 'high', 'medium', 'low'] },
    whyImpact: { type: 'string' },
    proposal: { type: 'string' },
    effort: { type: 'string', enum: ['S', 'M', 'L', 'XL'] },
    risk: { type: 'string' },
    howToMeasure: { type: 'string' },
  },
  required: ['id', 'title', 'location', 'category', 'mechanism', 'evidence', 'onDefaultPath', 'impact', 'whyImpact', 'proposal', 'effort', 'risk', 'howToMeasure'],
}

const BASE_PROPS = {
  subsystem: { type: 'string' },
  answer: { type: 'string', description: 'Direct answer to the main question, 150-400 words, citing file:line and finding ids' },
  howItWorks: { type: 'string', description: '<= 250 words' },
  threadingAndLocks: { type: 'string', description: '<= 200 words' },
  findings: { type: 'array', items: FINDING },
  incompleteOrStubbed: { type: 'array', items: { type: 'object', properties: { location: { type: 'string' }, what: { type: 'string' }, perfRelevance: { type: 'string' } }, required: ['location', 'what', 'perfRelevance'] } },
  quickExperiments: { type: 'array', items: { type: 'object', properties: { setting: { type: 'string' }, expectedEffect: { type: 'string' }, risk: { type: 'string' } }, required: ['setting', 'expectedEffect', 'risk'] } },
}
const BASE_REQ = ['subsystem', 'answer', 'howItWorks', 'threadingAndLocks', 'findings', 'incompleteOrStubbed', 'quickExperiments']

const CRITICAL_SCHEMA = {
  type: 'object',
  properties: {
    ...BASE_PROPS,
    timeline: {
      type: 'array',
      description: 'One frame, in order: each step a thread takes and what it blocks on',
      items: { type: 'object', properties: { thread: { type: 'string' }, step: { type: 'string' }, blocksOn: { type: 'string', description: 'what it waits for, or "nothing"' }, location: { type: 'string' } }, required: ['thread', 'step', 'blocksOn', 'location'] },
    },
    roundTripsPerFrame: { type: 'string', description: 'Estimate of CPU<->GPU round trips per AAA frame and the reasoning' },
    structuralChanges: { type: 'array', description: 'Top 5 structural changes to break the critical path', items: { type: 'object', properties: { change: { type: 'string' }, files: { type: 'string' }, unlocks: { type: 'string' }, effort: { type: 'string', enum: ['S', 'M', 'L', 'XL'] } }, required: ['change', 'files', 'unlocks', 'effort'] } },
  },
  required: [...BASE_REQ, 'timeline', 'roundTripsPerFrame', 'structuralChanges'],
}

const GUEST_SCHEMA = {
  type: 'object',
  properties: {
    ...BASE_PROPS,
    stubsVerdict: { type: 'string', enum: ['not-a-cause', 'minor-cause', 'significant-cause', 'unknown'] },
    buildAdvice: { type: 'array', items: { type: 'object', properties: { setting: { type: 'string' }, why: { type: 'string' }, evidence: { type: 'string' } }, required: ['setting', 'why', 'evidence'] } },
  },
  required: [...BASE_REQ, 'stubsVerdict', 'buildAdvice'],
}

const criticalPrompt = `${COMMON}

YOUR TASK: end-to-end frame critical path, including the queue-worker threading and GPU<->CPU synchronization model.
MAIN QUESTION (answer it in "answer"): why would both host CPU cores and the GPU be under-used, and what bounds the frame time?

FILES: libSceAgcDriver/Submit/src/{Dcb,Acb,CommandBuffer}.cpp; Execution/src/Driver/Queues/{Submission,QueueWorker,WorkerAffinity}.cpp; Execution/src/Driver/Packets/{PacketExecution,PacketMemory,PacketFlush,CommandValidation}.cpp; Execution/src/Driver/Synchronization/{WaitMemory,DeferredLabels,LabelHistory}.cpp; Execution/src/Driver/{Lifecycle,EntryPoints,Presentation}.cpp; Execution/src/VulkanDevice.cpp (only the present / SubmitRecorded / FinishPresent / RetirePresents paths); Eq/src/Event.cpp; core/libs/prx/libSceVideoOut/src/{Output,VideoOutDriver,Event}.cpp (flip, vblank, WaitForFlipRoom); core/libs/prx/libkernel/Equeue (only how EOP/flip events reach the game).

FOCUS:
- Trace ONE frame in order: game submit -> enqueue -> worker -> packets (draw, dispatch, label writes RELEASE_MEM/WRITE_DATA/EOP, WAIT_REG_MEM, flip) -> SubmitRecorded/vkQueueSubmit -> GPU -> presenter (Driver::Present) -> VideoOut flip/vblank -> game observes flip/EOP. Record each step in "timeline" with what it blocks on.
- How many worker threads exist and how ordering rules between queues (waitFree, orderReleased, queue0Before, heldAtSubmit) serialize them; the 'waitFree' poll loop in QueueWorker.cpp.
- WAIT_REG_MEM: what it polls, sleep granularity (PollSleep 200 us? condvar?), whether it flushes/submits recorded work first, whether it holds GpuMutex while waiting, Linux vs Windows differences.
- Labels: when a label write needs GPU completion before the CPU sees it; deferred labels and batching defaults (APS5_NO_LABEL_BATCHING, APS5_BATCH_CAP, APS5_LABEL_FLUSH_US); reapCompletionLabels; how EOP interrupts reach the game and whether the game's own threads block on them.
- Flip pacing: waitForFlipRoom on the game thread, VulkanDevice::FlipInFlight default, FIFO present, how many frames can be in flight; whether vblank emulation can cap or delay the game.
- Estimate CPU<->GPU round trips per AAA frame ("roundTripsPerFrame").
- Finish with the top 5 structural changes ("structuralChanges") that would break the critical path, with files.
Use ids prefixed "critical-path-".`

const guestPrompt = `${COMMON}

YOUR TASK: guest-side CPU libraries, kernel waits, stubs/unfinished functions, and build configuration.
MAIN QUESTION (answer it in "answer"): the user asked "is it the not-completed functions which block performance?" Answer with evidence, and say what else on the game side costs time. Set stubsVerdict accordingly.

FILES: core/libs/prx/libSceAgc/** (DcbDraw, DcbState, Command, Cb, Patch, Shader: per-draw command building on the game thread; look for heavy validation, logging, exceptions, map lookups per call); core/libs/prx/libkernel/{Time,Pthread/src,Equeue,EventFlag/src,Semaphore/src,SyncOnAddress/src}; core/libs/prx/libSceFiber; core/libs/prx/libSceUlt; core/libs/prx/libSceAudioOut (blocking pacing only); core/libs/prx/libc/include/CpuTopology.hpp and its .cpp; core/libs/prx/libkernel/Pthread/src/Thread.cpp (sceKernelGetAvailableCpumask, priorities, affinity); CMakeLists.txt, core/libs/CMakeLists.txt, core/libs/prx/libSceAgcDriver/CMakeLists.txt, core/shader/recompiler/CMakeLists.txt; docs/dev/TechnicalDebt.md (grep PPSA21564, PPSA22520, PPSA01325, Astro, and silent stubs); core/libs/prx/**/Unimplemented.cpp and NotImplemented_nid_no_patch usage; tools/import_audit.py (how a user can list which of the game's imports are stubs).

FOCUS:
(A) Game-thread CPU cost of sceAgc* DCB builders and sceAgcCreateShader logging (APS5_AGC_CREATE_LOG defaults ON in CMake?) — per call cost, stderr writes, locks.
(B) Kernel waits: timed waits, usleep/nanosleep/sceKernelUsleep, condvar timeouts, APS5_COARSE_TIMED_WAITS, Windows timer resolution (APS5_NO_TIMER_RESOLUTION), clock precision; fibers/ULT cost; guest thread priorities/affinities ignored; the CPU count/mask reported to the game (0x1FFF = 13 CPUs) vs host cores, and whether a job system that spins could oversubscribe a smaller host.
(C) Stubs: NotImplemented throws and terminates (crash, not slowdown) — confirm in code. Which silent stubs or approximations in TechnicalDebt or code could put a title on a slower path or add waits per frame? Anything Astro-Bot-specific? Explain how to use tools/import_audit.py to check the title's imports.
(D) Build config: default CMAKE_BUILD_TYPE when unset (empty => no -O flags with GCC?), optimization flags, LTO, -march, static libstdc++, -fno-asynchronous-unwind-tables, APS5_ENABLE_TIMING_LOG, APS5_AGC_CREATE_LOG, ANYPS5_ENABLE_SPIRV_TOOLS (SPIR-V optimization off by default?). Put concrete recommended settings in buildAdvice.
Use ids prefixed "guest-".`

phase('Analyze')
const [critical, guest] = await parallel([
  () => agent(criticalPrompt, { label: 'frame-critical-path', phase: 'Analyze', schema: CRITICAL_SCHEMA }),
  () => agent(guestPrompt, { label: 'guest-libs-build', phase: 'Analyze', schema: GUEST_SCHEMA }),
])
if (!critical) log('frame-critical-path returned nothing')
if (!guest) log('guest-libs-build returned nothing')
return { critical, guest }
