# Performance audit handoff

Audit of why Astro Bot (PPSA21564 / PPSA22520) runs at 8-9 FPS in game with host CPU threads and the GPU both under-used. Started 2026-10-13 on branch `claude/funny-bohr-bcq7nc`. It was stopped partway to save cloud credits. This file records what finished, what the results say so far, and exactly what is left.

## Files in this folder

| File | What it is |
|---|---|
| `HANDOFF.md` | This file: status, early conclusions, remaining work, how to continue. |
| `FINDINGS.md` | Every finding from the six finished subsystem analyses, in full: mechanism, code evidence, proposal, risk, how to measure. 82 findings. |
| `results/*.json` | The same six analyses as raw structured output, one file per subsystem. |
| `workflow.js` | The workflow script that ran the audit, with the prompt, file list and focus questions for all 12 subsystems. |
| `frame-journey.html` | Animated model of how a frame moves through the driver (game, submit, workers, GPU lock, recording, GPU, presenter, screen). Open it in a browser. Published copy: https://claude.ai/artifact/XnVFX8KC5yTQcNDP9hyFuC |

## Where it stopped

The audit was planned in four phases. It was stopped during the first one.

| Phase | Plan | Status |
|---|---|---|
| 1. Map | 12 read-only deep dives, one per subsystem | **6 of 12 finished.** 2 were running and lost (see below). 4 never started. |
| 2. Verify | One skeptic per subsystem tries to refute its critical/high/medium findings | **Not started.** Verification was queued behind the map agents. |
| 3. Gaps | A completeness critic, then up to 4 targeted follow-ups | Not started. |
| 4. Synthesize | Final ranked report: root causes, Draw.cpp optimizations, backlog, best-case architecture, measurement plan | Not started. |

Map phase, by subsystem:

| Key | Subsystem | Status |
|---|---|---|
| `exec-draw` | Execution-side draw path (`Execution/src/Driver/Draw/*`) | Done: 13 findings (1 critical, 3 high) |
| `graphics-draw` | Graphics-side draw recording (`Graphics/src/Draw.cpp`) | Done: 13 findings (2 critical, 6 high) |
| `recorder` | Command recording, submission, pipelines (`Recorder.cpp`, `Pipeline.cpp`) | Done: 15 findings (4 high) |
| `memory-tracking` | Guest memory tracking, `GpuMutex`, captures, write evidence | Done: 13 findings (5 high) |
| `buffers` | Guest buffers, uploads, host imports, write-backs, copy/fill | Done: 14 findings (4 high) |
| `shader-resources` | Descriptors, samplers, resource cache, validation | Done: 14 findings (1 critical, 3 high) |
| `textures` | Textures, detiling, DCC metadata, unit shadows, render-target aliasing | **Lost**: was about half done when stopped |
| `threading-sync` | Queue workers, submission, labels, `WAIT_REG_MEM`, ordering between queues | **Lost**: had just started |
| `device-present` | Vulkan device and queues, presentation, VideoOut flip/vblank | Not started |
| `shader-compile` | Shader recompiler placement, caching, quality of generated SPIR-V | Not started |
| `guest-libs-build` | libSceAgc on the game thread, kernel waits, stubs and unfinished functions, build flags | Not started |
| `frame-critical-path` | One frame traced end to end: every blocking point and the critical path | Not started |

The run ID was `wf_124135c1-232`. Its journal lived in this session's container, so a new session cannot resume it from cache. Everything it produced is in `results/`.

### Cost of what ran

Each finished map agent took 70-120 model turns, read 21-37 M cached input tokens and wrote about 0.4 M cache tokens. With two agents running at once (the container has 4 CPUs), each took 15-20 minutes. The 12 verification agents were planned to cost less each, since they only re-check cited lines. Use these numbers to size whatever runs next.

## What the six finished analyses say so far

**Nothing below has been verified.** These are claims from independent analysts, each with code evidence in `FINDINGS.md`. Where four analysts reached the same conclusion separately, that is noted. It raises confidence but does not replace verification.

### 1. Almost all draw work runs under one global lock (raised by 4 analysts)

`exec-draw-2`, `graphics-draw-1-locked-draw`, `shader-resources-draw-build-under-gpumutex`, `memory-tracking-gpumutex-unfair-wide-hold`.

Draw decoding, shader lookup and recompilation run before the lock. From `lockForDraw` (`Execution/src/Driver/Draw/Draw.cpp:260`) until the draw returns, everything else holds `GuestMemory::GpuMutex()`: resource key building, cache lookup, revalidation, full resource builds, pipeline creation, guest-memory validation and Vulkan recording. Dispatches already have an unlocked `PrepareDispatch` stage; draws do not. `GpuMutex` is an unfair recursive mutex shared with every queue worker and the presenter.

This fits the symptom: one worker thread doing almost all the draw work, one at a time, with other cores idle.

### 2. Caches look too small or too volatile, so steady-state frames redo work

`exec-draw-1` (critical), `exec-draw-3`, `graphics-draw-2-cache-capacity`, `recorder-pipeline-lru-1`, `shader-resources-rescache-lru-capacity`.

- The draw-cache key hashes user-data words that change every frame (SRT and constant-ring pointers), so draws may miss every frame and take the most expensive path.
- The shared resource cache defaults to 1024 entries (`ShaderResources.cpp:2039`, `APS5_RESOURCE_CACHE_ENTRIES`), possibly below one AAA frame's distinct draws.
- The pipeline store is hard-capped at 256 entries (`Pipeline.cpp:552`, not configurable). Evicted pipelines are destroyed and recreated, and their fast-path draw recipes fall back to the slow path.

### 3. The CPU waits for the GPU, often while holding the lock

`recorder-locked-waits-1`, `memory-tracking-compare-syncs-under-lock`, `graphics-draw-6-input-sync`, `exec-draw-7`, `exec-draw-9`, `buffers-copied-writers-overlisted`, `buffers-lease-pins-guest-frees`.

Several default paths wait for GPU completion inside the `GpuMutex` hold:
- validation compares that go through the flush hook;
- index and vertex inputs produced by an earlier GPU pass;
- CPU-read indirect draw arguments;
- guest `free`/`mprotect` of ranges the GPU still uses.

While one thread waits, every other worker and the presenter wait behind it. Meanwhile the GPU has little queued.

### 4. Write tracking depends on the platform, and its fallback is expensive

`graphics-draw-7-write-watch` (critical), `memory-tracking-linux-writewatch-compiled-out`, `shader-resources-linux-write-watch`, `memory-tracking-linux-no-page-cache`, `buffers-linux-page-query-uncached`, `memory-tracking-windows-imports-unwatched`.

Every per-draw "unchanged since last time" proof relies on the host write watch. Without it, draws re-copy, re-scan and compare guest memory byte by byte.

- **Linux:** the write watch is compiled in only when the build machine's kernel headers define `PAGEMAP_SCAN` and `UFFD_FEATURE_WP_ASYNC` (`core/libs/prx/libc/src/GuestWriteWatch.cpp:28`, roughly kernel 6.7+). It can also fail at run time; it then prints `[memory] write watch unavailable: ...` (`GuestWriteWatch.cpp:157`). Separately, Linux has no page-state cache: every guest-memory range check is a `PROCMAP_QUERY` ioctl (kernel 6.11+) or a full `/proc/self/maps` parse.
- **Windows:** host-imported memory is excluded from write tracking (see `docs/dev/TechnicalDebt.md`), so most GPU-visible memory falls back to compares.

### 5. GPU-side serialization

`recorder-barriers-1`, `graphics-draw-5-pass-churn`, `recorder-renderpass-1`, `recorder-single-queue-1`.

- Full pipeline barriers between almost every pair of commands (`Execution/src/VulkanDevice.cpp:3287-3311`).
- Render-pass breaks with full barriers at each pass boundary.
- A single `VkQueue` and recorder for all guest queues, so async compute never overlaps graphics.

Even when work reaches the GPU, it runs one step at a time.

### 6. Fixed per-draw overhead

`graphics-draw-3-dcc-scan`, `graphics-draw-4-snapshot-bindings`, `shader-resources-per-draw-descriptor-set`, `exec-draw-6`, `exec-draw-5`, `shader-resources-texture-log-fflush`.

- A full DCC key and CMASK scan of every color target on every draw.
- A second descriptor set allocated and copied per recorded draw, plus memcpy snapshots of read-only buffers.
- About 25-35 heap allocations per draw on the hit path, and a deep copy of each compiled stage's result.
- Unconditional stdout logging with `fflush` for every texture created, under the lock.

### Draw.cpp in particular

For `Graphics/src/Draw.cpp` see `graphics-draw-*`. Ranked by the analyst:
1. Move resource building and pipeline creation out of the lock.
2. Stop re-scanning DCC keys that `Refresh` just checked.
3. Reuse descriptor sets and bindings across recorded draws.
4. Relax render-pass continuation rules and the barriers at pass boundaries.
5. Avoid CPU copies and GPU waits for GPU-produced index and vertex data.

For `Execution/src/Driver/Draw/Draw.cpp` see `exec-draw-*`:
1. Take volatile words out of the draw-cache key.
2. Add a "data hit" path like dispatches have.
3. Re-capture only the stage that differs.
4. Stop deep-copying `RecompileResult`.
5. Cut per-draw allocations and `drawCacheMutex` round trips.

### "Is it the unfinished functions?"

Not answered yet; that was the `guest-libs-build` agent's job. What is known:
- Functions that are not implemented throw and end the process. They crash, they do not slow things down.
- The six finished analyses list the incomplete or approximated code they ran into (the "Incomplete or stubbed code noticed" section per subsystem in `FINDINGS.md`). Most of the cost they found is in finished code paths: locking, synchronization, cache sizing, memory tracking.

## Try today, no code changes

These come from the analysts and are **experiments, not recommendations**. Change one thing at a time and compare against a baseline run.

1. **Build in Release.** The root `CMakeLists.txt` sets no default `CMAKE_BUILD_TYPE`. Configure with `-DCMAKE_BUILD_TYPE=Release`, then rebuild the `libs` target (see `docs/dev/BUILD.md`).
2. **Linux: confirm the write watch works.** Search stderr for `[memory] write watch unavailable`. Build and run on kernel 6.7 or later, and 6.11 or later for `PROCMAP_QUERY`. If the message names userfaultfd, allow it for unprivileged users (`sysctl -w vm.unprivileged_userfaultfd=1`), as the shader-resources analyst suggests.
3. **Profile.** Run with `APS5_PROFILE_DRAW=1` (add `APS5_NO_HOLD_PROFILE=1` for lower overhead) and `APS5_PROFILE_GPU=1`. The `[lock] queue 0x0 waited ... ms for the GPU mutex ...` lines show lock waits by site. The `[gpu] worker at ... s: dispatch ... draw ... wait ...` lines show where each worker's time goes.
4. **Larger caches:** `APS5_RESOURCE_CACHE_ENTRIES=16384` and `APS5_DRAW_CACHE_ENTRIES=32768`.
5. **Batch size A/B:** `APS5_BATCH_CAP=256` and `APS5_BATCH_CAP=32` against the default 64.
6. **Host-import budget:** `APS5_HOST_IMPORT_MIB=12288`. On Windows also `APS5_WORKING_SET_MIB=16384`.
7. **Linux:** `APS5_WRITE_WATCH_IMPORTS=watch` (default `probe`).
8. **Parallel mirror refresh:** `APS5_MIRROR_REFRESH_THREADS=7`.
9. **Logging:** redirect stdout to a file or `/dev/null`. Texture creation logs and flushes for every texture and mip.

Each analysis has more settings in its "Settings to try" table in `FINDINGS.md`. Some are A/B diagnostics only (for example `APS5_NO_WRITE_WATCH=1`, `APS5_NO_COLLECT_MEMO=1`); they make things slower on purpose to show what a feature is worth.

## What is left

1. **Six map analyses:** `textures`, `threading-sync`, `device-present`, `shader-compile`, `guest-libs-build`, `frame-critical-path`. Their prompts, file lists and focus questions are in the `SUBSYSTEMS` array in `workflow.js`.
2. **Verification** of the critical and high findings already in `results/` (29 claims across the six files) and of the new ones.
3. **Completeness check** for anything nobody looked at.
4. **Final synthesis** following the report outline at the end of `workflow.js`:
   - TL;DR of root causes;
   - how a frame flows;
   - why threads and the GPU sit idle;
   - the unfinished-functions question;
   - Draw.cpp optimizations;
   - a backlog split into try-today, quick wins and structural changes;
   - the best-case architecture;
   - a measurement plan.

### Cheaper ways to finish

Pick by budget. The full original plan is about 25 more agents.

- **Smallest (3 agents).** One agent combining `frame-critical-path` and `threading-sync`; this answers "why are the threads idle". One `guest-libs-build` agent; this answers the unfinished-functions question and checks build flags. One synthesis agent that reads `FINDINGS.md` and the two new results.
- **Medium (about 7 agents).** The above, plus `textures`, `shader-compile` and `device-present`. Then one verification agent for only the 29 existing critical/high claims plus new critical ones, then synthesis.
- **Lower cost per agent.** Run map agents with a cheaper model or lower reasoning effort (`opts.model` / `opts.effort` in the workflow script). Keep the stronger model for verification and synthesis, where mistakes cost more.

### How to continue in a new session

1. Check out branch `claude/funny-bohr-bcq7nc`.
2. Ask Claude Code to continue the audit from `docs/dev/perf-audit/HANDOFF.md`, naming the option above you want.
3. To reuse `workflow.js`:
   - keep only the remaining entries in its `SUBSYSTEMS` array;
   - pass the existing `results/*.json` in through the workflow's `args` so verification and synthesis can see them (a workflow script cannot read files itself);
   - save new outputs next to the old ones in `results/`.

The analysis is read-only, so it needs no build and no GPU.
