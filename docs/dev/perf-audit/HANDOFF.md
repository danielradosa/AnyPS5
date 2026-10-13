# Performance audit handoff

Audit of why Astro Bot (PPSA21564 / PPSA22520) runs at 8-9 FPS in game with host CPU threads and the GPU both under-used. Started 2026-10-13 on branch `claude/funny-bohr-bcq7nc`.

The first round was stopped after six analyses to save cloud credits. A second, smaller round then answered the two main questions. This file records what finished, what the results say, and exactly what is left.

## Files in this folder

| File | What it is |
|---|---|
| `HANDOFF.md` | This file: answers so far, status, settings to try, remaining work, how to continue. |
| `FINDINGS.md` | Every finding from the eight finished analyses, in full: mechanism, code evidence, proposal, risk, how to measure. 98 findings. The two newest analyses come first and also include a one-frame timeline, structural changes and build settings. |
| `results/*.json` | The same eight analyses as raw structured output, one file per subsystem. |
| `workflow.js` | Round 1 workflow, with the prompt, file list and focus questions for all 12 planned subsystems. |
| `workflow-part2.js` | Round 2 workflow: the frame critical path and guest libraries analyses, with a tighter tool-call budget. |

## Answers so far

None of this has been through adversarial verification. It is the analysts' reading of the code, with file and line evidence in `FINDINGS.md`. Where several analysts reached the same conclusion independently, that is noted.

### Why are the CPU threads and the GPU both under-used?

From `frame-critical-path`, which builds on the six round-1 analyses.

1. **One thread does nearly all the work.** The queue-0 worker interprets every graphics packet in order and holds the process-wide `GpuMutex` through each whole draw (`exec-draw-2`, `graphics-draw-1-locked-draw`). Compute workers add little:
   - A compute submission without memory waits is held back until queue 0 has executed every older submission (`Submission.cpp:253-280`, `QueueWorker.cpp:92-98`, `critical-path-1`).
   - The work those workers do have competes for the same lock, as do the presenter and every completion reaper.

   In practice about one core does useful driver work.
2. **CPU and GPU take turns instead of overlapping.** Everything goes to one `VkQueue` in batches of at most 64 items. Whenever a packet, draw or dispatch needs memory that a queued batch writes, the worker submits and then blocks until the GPU has drained everything queued (`PacketMemory.cpp:166-211`, `VulkanDevice.cpp:3005-3031`, `Recorder.cpp:2902-2915`, `critical-path-2`, rated critical). The GPU runs out of work during the wait, then sits idle while the worker records again.
3. **Completion is polled, not signalled.**
   - Interrupts the game waits on (end-of-pipe events) reach it only when some thread wins a `try_lock` and reaps finished batches, which is polled every 0.25-1 ms (`critical-path-4`).
   - `WAIT_REG_MEM` spins for 1.5 ms, then sleeps in 200 µs steps (`WaitMemory.cpp:17-30`, `critical-path-5`).
4. **A flip waits for a full GPU drain.** The presenter waits for everything queued so far, including the next frame's early batches, before the flip completes (`Presentation.cpp:103-114`, `critical-path-3`).

What does **not** bound the frame at 8-9 FPS:
- vblank pacing: at about 115 ms per frame, the emulated vblank target has already passed;
- the flip queue: it has 16 slots, so the game's submit rarely blocks on it.

The game's own threads mostly wait for events, flips and labels from the lagging worker. That is why the host CPU looks idle.

The analyst estimates 55-90 `vkQueueSubmit` calls per frame for about 3000 draws and 300 dispatches. On top of those come tens to a few hundred blocking CPU waits for the GPU, at roughly 0.3-1 ms each. That would be a large share of a ~115 ms frame. These counts depend on content and were not measured.

### Is it the unfinished functions?

**No** (`guest-libs-build`, verdict `not-a-cause`).

- **Throwing stubs crash; they cannot slow anything down.** Every `NotImplemented_nid_no_patch` stub throws (`core/libs/prx/libc/src/General.cpp:314-316`). Nothing on the guest thread path catches it, so the process ends (`libkernel/Pthread/src/Thread.cpp:172-184`).
- **The silent stubs in `docs/dev/TechnicalDebt.md` are off the frame path**: dialogs, network, HMD, USB, audio. The Astro Bot specific entries add no waits.
- **The game's own command building is cheap.** The `sceAgcDcb*` builders do a few checks and write words, with no locks or logging. The `sceAgcCreateShader` log is commented out (`libSceAgc/Shader/src/CreateShader.cpp:66`), so the `APS5_AGC_CREATE_LOG` CMake option does nothing.

What does cost time outside the driver's draw path:
- **An unoptimized build** if `CMAKE_BUILD_TYPE` was never set (`guest-build-type-unset`, rated critical). Checked: no CMake file in the project sets a default build type or any `-O` flag. GCC then builds at `-O0`.
- **Windows on an Intel CPU without `--to-intel`:** every SSE4a instruction traps into an exception handler and is emulated (`guest-sse4a-trap-emulation`).
- **Windows fine-grained timed waits** busy-spin their last 0.5 ms (`libkernel/Time/TimedWait.cpp`).
- **No host thread priorities**, so nothing favours the one worker everything depends on. The game is also always told it has 13 CPUs (mask 0x1FFF).
- **Unconditional texture logging** to stdout with `fflush`, under `GpuMutex`.

## Where it stopped

| Phase | Plan | Status |
|---|---|---|
| 1. Map | Read-only deep dives, one per subsystem | **8 of 12 done.** `threading-sync`'s questions were folded into `frame-critical-path`. `textures`, `device-present` and `shader-compile` are not done. |
| 2. Verify | A skeptic re-checks each critical/high/medium claim | **Not started.** |
| 3. Gaps | A completeness critic, then targeted follow-ups | Not started. |
| 4. Synthesize | Final ranked report | Not started. This file's "Answers so far" is an interim version. |

| Key | Subsystem | Status |
|---|---|---|
| `frame-critical-path` | One frame end to end, threading and GPU/CPU synchronization | Round 2: 7 findings (1 critical, 1 high) |
| `guest-libs-build` | Game-side libraries, kernel waits, stubs, build configuration | Round 2: 9 findings (1 critical, 1 high) |
| `exec-draw` | Execution-side draw path (`Execution/src/Driver/Draw/*`) | Round 1: 13 findings (1 critical, 3 high) |
| `graphics-draw` | Graphics-side draw recording (`Graphics/src/Draw.cpp`) | Round 1: 13 findings (2 critical, 6 high) |
| `recorder` | Command recording, submission, pipelines | Round 1: 15 findings (4 high) |
| `memory-tracking` | Guest memory tracking, `GpuMutex`, captures | Round 1: 13 findings (5 high) |
| `buffers` | Guest buffers, uploads, host imports, write-backs | Round 1: 14 findings (4 high) |
| `shader-resources` | Descriptors, samplers, resource cache, validation | Round 1: 14 findings (1 critical, 3 high) |
| `threading-sync` | Queue workers, labels, `WAIT_REG_MEM`, queue ordering | Covered by `frame-critical-path` |
| `textures` | Textures, detiling, DCC metadata, render-target aliasing | **Not done** (round-1 agent was lost when stopped) |
| `device-present` | Vulkan device and queues, presentation, VideoOut | **Not done** (partly covered by `frame-critical-path`) |
| `shader-compile` | Recompiler placement, caching, generated SPIR-V quality on the GPU | **Not done** |

### Cost of what ran

- **Round 1.** Each map agent took 70-120 model turns and read 21-37 M cached input tokens. With two running at once, each took 15-20 minutes.
- **Round 2.** Both agents together took 11 minutes and 136 tool calls. They were told to grep first and read only the lines they needed. Use that approach for anything that runs next.

## The six themes from round 1

The round-1 analysts found the same problems from different directions.

### 1. Almost all draw work runs under one global lock (raised by 4 analysts)

`exec-draw-2`, `graphics-draw-1-locked-draw`, `shader-resources-draw-build-under-gpumutex`, `memory-tracking-gpumutex-unfair-wide-hold`.

Draw decoding, shader lookup and recompilation run before the lock. From `lockForDraw` (`Execution/src/Driver/Draw/Draw.cpp:260`) until the draw returns, everything else holds `GuestMemory::GpuMutex()`:
- resource key building, cache lookup and revalidation;
- full resource builds and pipeline creation;
- guest-memory validation;
- Vulkan recording.

Dispatches already have an unlocked `PrepareDispatch` stage; draws do not.

### 2. Caches look too small or too volatile, so steady-state frames redo work

`exec-draw-1` (critical), `exec-draw-3`, `graphics-draw-2-cache-capacity`, `recorder-pipeline-lru-1`, `shader-resources-rescache-lru-capacity`.

- **Draw-cache key:** it hashes user-data words that change every frame, so draws may miss every frame and take the most expensive path.
- **Resource cache:** it defaults to 1024 entries (`ShaderResources.cpp:2039`, `APS5_RESOURCE_CACHE_ENTRIES`).
- **Pipeline store:** it is hard-capped at 256 entries (`Pipeline.cpp:552`, not configurable). Evicted pipelines are destroyed and recreated.

### 3. The CPU waits for the GPU, often while holding the lock

`recorder-locked-waits-1`, `memory-tracking-compare-syncs-under-lock`, `graphics-draw-6-input-sync`, `exec-draw-7`, `exec-draw-9`, `buffers-copied-writers-overlisted`, `buffers-lease-pins-guest-frees`, and `critical-path-2` from round 2.

These default paths wait for the GPU while holding the lock:
- validation compares that go through the flush hook;
- index and vertex inputs produced by an earlier GPU pass;
- indirect draw arguments read on the CPU;
- guest `free`/`mprotect` of ranges the GPU still uses.

### 4. Write tracking depends on the platform, and its fallback is expensive

`graphics-draw-7-write-watch` (critical), `memory-tracking-linux-writewatch-compiled-out`, `shader-resources-linux-write-watch`, `memory-tracking-linux-no-page-cache`, `buffers-linux-page-query-uncached`, `memory-tracking-windows-imports-unwatched`.

Without the host write watch, draws re-copy and compare guest memory byte by byte.

- **Linux:**
  - The write watch is compiled in only when the build machine's kernel headers define `PAGEMAP_SCAN` and `UFFD_FEATURE_WP_ASYNC` (`core/libs/prx/libc/src/GuestWriteWatch.cpp:28`, roughly kernel 6.7+).
  - It can also fail at run time, in which case it prints `[memory] write watch unavailable: ...` (`GuestWriteWatch.cpp:157`).
  - Every guest-memory range check is a `PROCMAP_QUERY` ioctl (kernel 6.11+) or a full `/proc/self/maps` parse.
- **Windows:** host-imported memory is excluded from write tracking.

### 5. GPU-side serialization

`recorder-barriers-1`, `graphics-draw-5-pass-churn`, `recorder-renderpass-1`, `recorder-single-queue-1`.

- Full pipeline barriers between almost every pair of commands (`Execution/src/VulkanDevice.cpp:3287-3311`).
- Render-pass breaks, with full barriers at each boundary.
- One `VkQueue` for all guest queues.

### 6. Fixed per-draw overhead

`graphics-draw-3-dcc-scan`, `graphics-draw-4-snapshot-bindings`, `shader-resources-per-draw-descriptor-set`, `exec-draw-5`, `exec-draw-6`, `shader-resources-texture-log-fflush`.

- A full DCC key and CMASK scan of every color target, on every draw.
- A second descriptor set and buffer snapshots for every recorded draw.
- About 25-35 heap allocations per draw.
- Unconditional texture logging.

## What to change first

### Structural changes (from `frame-critical-path`)

1. **Shrink `GpuMutex` to recording and submission only (XL).** Give draws the unlocked prepare stage dispatches already have: key, lookup, resource build, capture and pipeline creation outside the lock. Files: `Execution/src/Driver/Draw/Draw.cpp`, `Graphics/src/Draw.cpp`, `Graphics/src/ShaderResources.cpp`, `Graphics/src/Pipeline.cpp`.
2. **Take GPU-produced data off the worker's blocking path (XL).**
   - Consume indirect arguments, counts and index data directly on the GPU from host imports.
   - Add an unlocked draw presync.
   - Give `COND_WRITE` and other store packets GPU paths.
   - When a wait is unavoidable, submit queued work before starting expensive packets.

   This ends the CPU/GPU ping-pong. Files: `Packets/PacketMemory.cpp`, `Draw/IndirectDraw.cpp`, the draw path.
3. **Add a completion thread (L).** It blocks on the recorder's timeline semaphore and then:
   - retires batches;
   - delivers end-of-pipe events;
   - wakes `WAIT_REG_MEM` waiters directly;
   - completes flips.

   This replaces `try_lock` polling. Files: `Queues/QueueWorker.cpp`, `Synchronization/WaitMemory.cpp`, `Synchronization/DeferredLabels.cpp`, `Graphics/src/Recorder.cpp`.
4. **Replace the receive-order gating of compute queues with explicit memory dependencies (XL).** Give async compute its own `VkQueue` and recorder, with timeline-semaphore waits between queues. Files: `Queues/Submission.cpp`, `Queues/QueueWorker.cpp`, `Recorder.cpp`, `VulkanDevice.cpp`.
5. **Make flip completion asynchronous and bounded to the frame's own work (M).**
   - Drop the unconditional trailing `FinishPresent`.
   - Complete the flip when that frame's batches and the blit have finished.
   - Take `GpuMutex` once per present.

   Files: `Driver/Presentation.cpp`, `VulkanDevice.cpp`.

### In Draw.cpp

For `Graphics/src/Draw.cpp` (`graphics-draw-*`):
1. Move resource building and pipeline creation out of the lock.
2. Stop re-scanning DCC keys that `Refresh` just checked.
3. Reuse descriptor sets and bindings across recorded draws.
4. Relax the render-pass continuation rules and the barriers at pass boundaries.
5. Avoid CPU copies and GPU waits for index and vertex data the GPU produced.

For `Execution/src/Driver/Draw/Draw.cpp` (`exec-draw-*`):
1. Take volatile words out of the draw-cache key.
2. Add a "data hit" path like dispatches have.
3. Re-capture only the stage that differs.
4. Stop deep-copying `RecompileResult`.
5. Cut per-draw allocations and `drawCacheMutex` round trips.

## Try today, no code changes

These come from the analysts and are **experiments, not recommendations**, except item 1. Change one thing at a time and compare against a baseline run.

1. **Build in Release.** Look at `CMAKE_BUILD_TYPE` in `build/CMakeCache.txt`. If it is empty or `Debug`, reconfigure with `-DCMAKE_BUILD_TYPE=Release` (or `RelWithDebInfo` for profiling), then rebuild the `libs` target (see `docs/dev/BUILD.md`). An empty build type means no optimization at all.
2. **Intel CPU:** relink the title with `--to-intel`. On Windows, `APS5_TRACE_SSE4A=1` shows whether emulated instructions run in game.
3. **Linux: confirm the write watch works.** Search stderr for `[memory] write watch unavailable`. Build and run on kernel 6.7 or later, or 6.11 or later for `PROCMAP_QUERY`.
4. **Profile:** run with `APS5_PROFILE_DRAW=1` (add `APS5_NO_HOLD_PROFILE=1` for lower overhead) and `APS5_PROFILE_GPU=1`. Read these lines:
   - `[lock]`: lock waits by site, and locked GPU waits;
   - `[gpu] worker at ...`: where each worker's time goes;
   - `[present]`: batches per flip, GPU busy and idle gaps;
   - `[presync]`.
5. **Logging:** redirect stdout to a file or `/dev/null` (`NUL` on Windows).
6. **Windows:** `APS5_COARSE_TIMED_WAITS=1` stops guest timed waits from busy-spinning, which frees cores.
7. **Feed the GPU sooner:**
   - `APS5_BATCH_CAP=16` (default 64), also try 256;
   - `APS5_LABEL_FLUSH_US=250` (default 1000);
   - `APS5_REAP_EACH_PACKET=1` and `APS5_POLL_REAP=1`.
8. **Free a core on 4-8 core hosts:** `APS5_NO_PAUSE_SPIN=1`.
9. **Larger caches:** `APS5_RESOURCE_CACHE_ENTRIES=16384` and `APS5_DRAW_CACHE_ENTRIES=32768`.
10. **Host imports:**
    - `APS5_HOST_IMPORT_MIB=12288`;
    - on Windows also `APS5_WORKING_SET_MIB=16384`;
    - on Linux `APS5_WRITE_WATCH_IMPORTS=watch`.
11. **Diagnostic only:** `APS5_FLIP_INFLIGHT=2` should change nothing, because the trailing full drain still waits. If it does change nothing, that confirms `critical-path-3`.

More settings are listed per analysis in `FINDINGS.md`. Some are A/B diagnostics only (for example `APS5_NO_WRITE_WATCH=1`); they make things slower on purpose to show what a feature is worth.

## What is left

1. **Verification** of the critical and high claims: 33 across the eight analyses. This is the most useful next step. One verification agent per two or three analyses, told to read only the cited lines, would keep the cost down.
2. **Three map analyses**, with prompts in the `SUBSYSTEMS` array of `workflow.js`:
   - `textures`: CPU detiling, texture hashing, render-to-texture write-back;
   - `shader-compile`: compile placement and the GPU cost of the generated SPIR-V;
   - `device-present`: the remainder not covered by `frame-critical-path`.
3. **Final synthesis.** A single report following the outline at the end of `workflow.js`, built from `FINDINGS.md` and the verification results.

### How to continue in a new session

1. Check out a branch from `main`.
2. Ask Claude Code to continue the audit from `docs/dev/perf-audit/HANDOFF.md`, naming which of the steps above to run.
3. `workflow-part2.js` is the cheaper template: it points agents at a digest of earlier results and sets a tool-call budget. Its `DIGEST` path pointed into the old session's scratch folder, so change it to `docs/dev/perf-audit/FINDINGS.md` or `results/*.json`. A workflow script cannot read files, but its agents can.
4. Save new outputs next to the old ones in `results/`. The repository's `.gitignore` excludes `*.json`, so add them with `git add -f`.

The analysis is read-only, so it needs no build and no GPU.
