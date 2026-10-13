# Performance audit: raw findings (unverified)

Generated from the six subsystem deep-dives that finished before the audit was stopped. Each finding is one analyst's reading of the code. **None of them has been through the adversarial verification step yet**, so treat impact levels as claims to check. The JSON for each subsystem is in [results/](results/).

## Contents

- [Execution-side draw path (Execution/src/Driver/Draw/*)](#exec-draw) (13 findings)
- [Graphics-side draw recording (core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp and its helpers: Pipeline, Recorder render-pass handling, DCC/CMASK, storage-target residency, draw snapshots, DepthSurface, ColorTarget transfer)](#graphics-draw) (13 findings)
- [Command recording, submission and pipelines (Recorder, Pipeline, PipelineCache)](#recorder) (15 findings)
- [Guest memory tracking, GpuMutex, captures and write evidence (GuestMemory.cpp/.hpp, WriteWatchCoverage, MemoryAccessScope, ShaderMemory, Driver/Memory/{WriteEvidence,WrittenBuffers,CaptureValidation,CaptureDiagnostics}, DeviceAccess) plus the libc write-watch backends they call (GuestWriteWatch.cpp on Linux, WindowsMappings.hpp on Windows)](#memory-tracking) (13 findings)
- [Guest buffers, uploads, host imports, write-backs, copy/fill HLE (GuestBufferMemory, BufferPool, BdaResources, Resources, BufferCopy/BufferFill)](#buffers) (14 findings)
- [Shader resource binding: descriptors, samplers, validation (ShaderResources, DescriptorCache, ResourceCache, texture/sampler caches, ShaderValidation memo, bindless tables)](#shader-resources) (14 findings)

<a id="exec-draw"></a>
## Execution-side draw path (Execution/src/Driver/Draw/*)

### How it works

The queue worker calls Driver::draw once per DRAW_* packet (PacketExecution.cpp:235-287). The draw runs these steps in order:
1. Pm4::ResolveDraw (Pm4.cpp:706-733).
2. precheckDraw (DrawPrecheck.cpp:14-67). It checks for metadata passes, depth maintenance, draws with no color writes, primitive restart, and the DrawRejection register rules.
3. Loads the device without a lock.
4. drawRegisterKey (DrawRegisterKey.cpp:9-57). This is an FNV hash over every present register in the 45 DrawKeyRegisters ranges, including each stage's RSRC2-counted user-data words and the program snapshot identities. The key is looked up in drawCache under drawCacheMutex.
5. resolveDrawDecode. The full decodeDraw runs only when no entry exists.
6. Copies decode->programs, builds memory/linked, and constructs a ShaderMemory.
7. lookupDraw validates each stage's 4 or fewer variants against live guest memory with validateVariant. That function covers pending-write syncs, the StorageTexture and shadow checks, and CompareMapped on every captured run.
8. Outcome:
   - Full hit: the cached compiled results and vertex infos are used.
   - Otherwise: compileDrawStage runs for every stage (SourceHandleFor memo, then ShaderMemory::Capture SRT walk, then the Recompile memo, then a deep copy). cacheDrawStages then builds variants and calls insertDrawEntry (LRU, 4096 entries).
9. Checks for deferred-label overlap, which can restart the whole draw. Then rect-list and snapshots.
10. Either the CPU indirect path (patch, recompile and Graphics::Draw per record), or findDrawRecipe followed by KnownDrawRejection.
11. lockForDraw (takes GpuMutex and records labels), then noteDrawWriters.
12. DrawFromRecipe or Graphics::Draw, then attachDrawRecipe.

Defaults:
- Draw entries, the register key and recipes are ON.
- Verify, trace and dump switches are all opt-in: APS5_VERIFY_DRAW_ENTRIES, APS5_VERIFY_DRAW_RECIPE, APS5_DUMP_DRAW_SHADERS, APS5_DUMP_DRAW_SLOT1, APS5_TRACE_INDIRECT_DRAWS, APS5_TRACE_CAPSYNC, APS5_TRACE_DRAWS, APS5_CAPTURE_TRACE, APS5_PROFILE_DRAW and APS5_LOCKED_DRAW_PREPARE.
- APS5_LOG_OUT_DEBUG compiles to (void)0.

### Threads and locks

Draws run on one thread: the graphics queue's worker. All per-draw work is serial in PM4 order.

Outside GpuMutex by default (APS5_LOCKED_DRAW_PREPARE unset): precheck, key, decode, validation, capture and recompile.

drawCacheMutex (one std::mutex shared by all workers): taken 3-4 times per hit (Draw.cpp:72, DrawLookup.cpp:118, DrawCache.cpp:113 and 128) and on every insert (DrawCache.cpp:59). The holds are short.

GuestMemory::GpuMutex (recursive, process-wide):
- lockForDraw (Draw.cpp:260-276) takes it and holds it until the function returns. That covers recording deferred labels, noteDrawWriters, all of Graphics::Draw or DrawFromRecipe, and attachDrawRecipe.
- Graphics::Draw includes the target refresh, the resource cache, the ShaderResources build, CachedPipeline (vkCreateGraphicsPipelines on a miss), command recording, and recorder->Sync for waited draws.
- In the CPU-indirect loop it stays held while later records are recompiled.
- It is also taken briefly by recordQueuedLabelsAfterCapture's try_lock (every draw while labels are queued).
- The flush hook takes it whenever validation, capture or argument reads overlap pending GPU writes (Recorder.cpp:1010-1020). SyncThrough there blocks the worker until the GPU finishes.

Other locks per draw:
- StorageTexture's pending mutex and the unit-shadow registry mutex (once per run) in validateVariant.
- writtenBuffersMutex.
- validationMutex.
- On misses: the snapshot handle-memo mutex and the recompiler's source mutex.

### Findings

#### exec-draw-1: Draw-cache key hashes volatile user-data words (SRT and constant-ring pointers), so draws likely miss with 'no entry' every frame and take the most expensive path

- **Impact (claimed):** critical · **Effort:** L · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawRegisterKey.cpp:9-57; Draw.cpp:69-83,197-222; DrawCache.cpp:39-46,99-107`

**Mechanism.** The register key mixes the value of every present register in the DrawKeyRegisters ranges. That includes the user-data words each stage reads (PS 0x00c.., GS/VS 0x08c.., HS 0x10c..). Those words hold the per-draw SRT, constant and vertex-table pointers. A title that takes them from a per-frame ring gets a new key for the same draw every frame. When the key is absent, the draw runs the full path:
- DecodeState + DecodePixelStageInfo + program prepare.
- DecodeVertexStageInfo: 2 GuestMemory::Read calls and 2 vector allocations per vertex semantic.
- Per stage: SourceHandleFor, then ShaderMemory::Capture (SRT walk, a 4 KiB node allocated and copied per page), then Recompile (result memo), then a deep copy.
- A new DispatchVariant per stage (words copy and stable_sort).
- insertDrawEntry under drawCacheMutex, with an O(words) compare against existing variants and LRU eviction.
- No draw recipe, because a recipe is only looked up after drawHit.
With 4096 entries (the default) and more than about 1,500 draws per frame across 2-3 buffered frames, entries keep being evicted.

**Evidence.** - DrawRegisterKey.cpp:16-22: userEnd = base + RSRC2 user count. Lines 28-37 mix every present register in each range and skip only shader registers past that count, so the user-word values are hashed.
- State.hpp:168: shader ranges {0x008,0x24}, {0x08a,0x22}, {0x10b,0x21} cover the 32 user words of each stage.
- Draw.cpp:69-83: drawCache.find(drawKey) and ++absent on a miss.
- DrawDecode.cpp:106-112: decodeDraw when decode==nullptr.
- Draw.cpp:164-180: decodeVertexInfo for every non-hit stage. ShaderInputState.cpp:198-215 does 2 Reads and 2 DecodeRead vector allocations per semantic.
- Draw.cpp:201-210: compileDrawStage for every stage when !drawHit.
- DrawCapture.cpp:25-40,79-80: capture, Regions() rebuild, Recompile and copy.
- DrawCapture.cpp:92-123: variant build and insertDrawEntry.
- DrawCache.cpp:39-46: APS5_DRAW_CACHE_ENTRIES unset gives 4096. Lines 99-107 evict.
- Draw.cpp:360: recipe only if drawHit.
- Defaults: drawEntries ON (DrawCache.cpp:19-22: APS5_NO_DRAW_SRT_ENTRIES and APS5_DISPATCH_STAMP_VALIDATE unset). registerKeyEnabled ON (DrawCache.cpp:29-32: APS5_NO_DRAW_KEY unset). APS5_DRAW_KEY_ALL_USER_WORDS is opt-in and only makes the key stricter.
- Commit d9632d55 states that PPSA21564 uses a per-draw constant ring that later frames rewrite, so these pointers rotate.

**Why this impact.** A miss costs the decode, the capture walk, materialization, roughly 100 heap allocations for the copy, and the variant build and insert, on every draw. Estimate: tens to hundreds of µs per draw, against a few µs for a hit. If most of about 1,000-3,000 draws per frame miss, that alone can explain a queue-0 frame time above 100 ms (8-9 FPS), with only one thread busy.

**Proposal.** Use a two-level key in Draw.cpp/DrawRegisterKey.cpp:
(a) A structural key: drawRegisterKey without the user-word values, keeping the RSRC2 counts, program snapshot and offset. It maps to a cached DrawDecode template. When the exact key is absent but the structural key hits, copy the template and reload only program.userData from queue.shader (the loop at DrawDecode.cpp:42-45 and initializeMerged's pointer words). This skips DecodeState, DecodePixelStageInfo and ExportMappings.
(b) Store each stage's userData words in its DispatchVariant and compare them before memory validation. A draw whose ring addresses recur then hits under the structural entry, without depending on LRU size.
(c) Keep a per-structural-key volatility counter. After K consecutive absent or Differing outcomes, skip validation and variant insertion for the next M draws and go straight to capture. This stops LRU churn and wasted validation.
(d) Interim: raise the default drawCacheEntries (for example to 16384) if the counters show evictions.

**Risk.** Medium. The decode must be a pure function of the registers excluding user-word values (decodeDraw only copies them into program.userData). Check this with the existing facade log under APS5_VERIFY_DRAW_RECIPE=1 (DrawDecode.cpp:113-131) and sameDecode.

**How to measure.** Set APS5_PROFILE_DRAW=1 and read these lines:
- '[draw-cache] N lookups (10 s): N no entry, N validated ... N hits ... misses by reason ...; N entries ..., N evictions in total' and '[draw-cache] register key (10 s): N lookups, N hits, key build X us each; decode skipped N, partial N' (DrawCacheStatistics.cpp:16-17).
- '[draw] driver phases ... us per packet: decode, program prepare, capture, recompile, key/lookup/validate' (DrawTiming.cpp:30).
The hit rate is registerKeyHits divided by registerKeyLookups.

#### exec-draw-2: GpuMutex is held across all of Graphics::Draw (resource build, pipeline creation, recording); draws have no unlocked prepare stage like dispatch's PrepareDispatch

- **Impact (claimed):** high · **Effort:** L · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:42,260-276,343-346,370-387`

**Mechanism.** lockForDraw takes the process-wide recursive GpuMutex through a unique_lock declared at function scope. Nothing releases it before return. So noteDrawWriters, DrawFromRecipe or Graphics::Draw, and attachDrawRecipe all run while every other queue worker, the presenter and the flush hook are locked out. Graphics::Draw does the following under the lock:
- Resident-target refresh: FlushPending, CollectWrites and the DCC key scan.
- resolveDrawResources: DrawResourceKey, Find/Revalidate, or a full ShaderResources build.
- CachedPipeline, which calls vkCreateGraphicsPipelines on a miss.
- Recording.
- recorder->Sync() for waited draws.
The dispatch path runs PrepareDispatch (stage A) before it takes the lock. The draw path has no equivalent.

**Evidence.** - Draw.cpp:42: std::unique_lock gpuLock(GpuMutex(), defer_lock). Lines 260-276: lockForDraw does gpuLock.lock() and recordLabelsForPacket. Lines 370-385: noteDrawWriters, Draw/DrawFromRecipe and attachDrawRecipe, all still locked.
- Graphics/src/Draw.cpp:1562 refreshResidentTarget, 1615 resolveDrawResources (992 make_shared<ShaderResources>), 1740 CachedPipeline. Pipeline.cpp:248 calls vkCreateGraphicsPipelines. Draw.cpp:1441-1447 calls recorder->Sync() for waited draws.
- Dispatch.cpp:288-313: PrepareDispatch runs before std::lock_guard gpuLock. VulkanDevice.hpp:219 (PrepareDispatch) and :249 (PrepareRecipe) are dispatch-only. A grep for PrepareDraw finds none.
- GuestMemory.hpp:118-123: 'the graphics worker's frame is mostly spent waiting for this lock behind the compute workers'.
- Default: APS5_LOCKED_DRAW_PREPARE unset (Draw.cpp:41), so the lock is first taken at lockForDraw, but it still covers all of Graphics::Draw.

**Why this impact.** Every draw's Graphics::Draw time (resource builds, a pipeline compile on a miss, the recorder sync) blocks every compute-queue worker and the flush hook. This is why extra CPU threads do not help: queues take turns on one mutex, and with queue 0 issuing thousands of draws per frame the lock is held for most of the frame.

**Proposal.** (1) Effort S, in Draw.cpp:
- Call noteDrawWriters (which takes only writtenBuffersMutex) and KnownDrawRejection before lockForDraw.
- Call gpuLock.unlock() right after localDevice->Draw or DrawFromRecipe returns, before attachDrawRecipe (which needs only drawCacheMutex) and before drawn().
- In the indirect loop, unlock after each record's Draw.
(2) Effort L: add VulkanDevice::PrepareDraw that mirrors PrepareDispatch. Run these without the lock: DrawResourceKey, SharedResourceCache Find/Revalidate, ShaderResources stage A, ValidationKey, and the pipeline lookup. Create missing pipelines outside GpuMutex, synchronized only on the VkPipelineCache. Keep only target refresh, the label record and command recording under the lock. Add a draw PrepareRecipe for DrawFromRecipe's prechecks.

**Risk.** Medium-high for (2). ShaderResources, StorageTexture refresh and the recorder assume GpuMutex, so follow the split already proven for dispatch. (1) is low risk.

**How to measure.** Set APS5_PROFILE_DRAW=1 (GPU-lock profiling is on unless APS5_NO_LOCK_PROFILE is set) and read:
- '[lock] queue 0x.. waited X ms ... held X ms in total, by site (holds, held, max): draw ...' and 'waited behind (holder/site ms)' for the compute queues (GuestMemory.cpp:1617-1645).
- '[draw] driver phases ... lock wait, labels, Graphics::Draw'.

#### exec-draw-3: Draws have no 'data hits': any changed flattened-SRT constant makes a stage Differing and forces capture + materialize + a new variant

- **Impact (claimed):** high · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawLookup.cpp:93-110; DrawCapture.cpp:85-123`

**Mechanism.** lookupDraw calls validateVariant without the `live` output, so masked=false and any differing captured dword is a miss. Draw variants are built without dataPositions or flatBinding. Per-object constants that the shader reads through the SRT end up in the captured runs; when they change each frame, every stage that reads them misses. The miss then:
- recaptures and rematerializes (a new snapshot means a result-memo miss);
- deep-copies the result;
- inserts a variant (maximum 4 per stage), evicting older ones.
Dispatch handles the same case with EqualData: it patches the FlattenedSrt binding words into a copy of the cached result.

**Evidence.** - DrawLookup.cpp:100: validateVariant(programs[i].binary.codeAddress, submission.queue, *variant, regions, imagesFlushed, runsSynced, sampling), with no live pointer.
- VariantValidation.cpp:72: masked = live != nullptr && ...
- DrawCapture.cpp:92-121: the variant gets compiled, shader, forgetSerial, pushOffset, runs and words; dataPositions stays empty.
- Dispatch counterpart: DispatchLookup.cpp:43 passes &liveData, :120-135 patches guestDescriptor. DispatchInsert.cpp:29-36 runs DataWordPositions on capture->readTrace.
- dataHits() is ON by default (DispatchCache.cpp:12-15; APS5_NO_DATA_HITS unset; TemplateDataRefresh ON because APS5_NO_TEMPLATE_DATA_REFRESH is unset, VulkanDevice.cpp:2555).
- Variant cap: dispatchVariants() defaults to 4 (DispatchCache.cpp:22-29) and is used by DrawCache.cpp:83.

**Why this impact.** Even when register keys hit, every draw whose SRT-read constants change each frame (transforms, animation and material parameters) pays the full miss cost from exec-draw-1. A level frame of a 3D action game has many such draws.

**Proposal.** Port data hits to draws:
- Keep the shared_ptr<ResourceCapture> from compileDrawStage in StageCapture.
- In cacheDrawStages, set variant->flatBinding and dataPositions/dataSlots with DataWordPositions over the stage's runs and the FlattenedSrt binding (as in insertDispatch).
- In lookupDraw, pass a per-stage liveData vector and accept EqualData.
- On EqualData, build a patched RecompileResult (copy-on-write of that one binding's guestDescriptor) and set programResults[i] to it. Mark the draw as non-recipe.
- In Graphics, extend template data refresh to draw templates so the resource cache also hits.

**Risk.** Medium. DataWordPositions already excludes aliased and other reads. Verify with an APS5_VERIFY_DATA_HITS-style mode for draws.

**How to measure.** With APS5_PROFILE_DRAW=1:
- '[draw-cache] ... misses by reason: front stage differing N, fragment differing N, other stage differing N'. Compare them against hits.
- '[capture] N captures ...' rate (ShaderMemory.cpp:244).
- '[draw] driver phases' capture and recompile µs per packet.

#### exec-draw-7: CPU indirect draws: a GPU wait to read the arguments, then per record a recompile and a full Graphics::Draw, with records 2..N recompiled under GpuMutex

- **Impact (claimed):** high · **Effort:** M · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:277-351; IndirectDraw.cpp:15-58`

**Mechanism.** classifyIndirectDraw sends these draws to the CPU path:
- every multi or count-indirect draw on a non-vertex path (NGG mesh, tessellation), because the mesh GPU-side path requires count==1, no countIndirect and no base-vertex or start-instance location;
- draws whose drawIndex is in an SGPR;
- draws that fail on feature gaps.
The CPU path then:
- reads the count and records through GuestMemory::Read. That goes through the flush hook, which submits and waits for the GPU when compute culling has just written them.
- for each record: allocates a std::set, patches the user words, and runs compileDrawStage (capture, Recompile and copy) for each patched program. It then runs fold, a rect-list rebuild, snapshot(), KnownDrawRejection and one full Graphics::Draw.
- takes lockForDraw at the first record and never releases it, so later records' captures and recompiles run under GpuMutex.

**Evidence.** - IndirectDraw.cpp:23-25: meshGpuSide = meshPath && indexed && count==1 && !countIndirect && no base or start location; otherwise fail(NonVertexPath). Line 26: DrawIndex.
- Draw.cpp:292-296: recordQueuedLabelsBeforeRead and Pm4::ReadDrawCount/ReadDrawArguments. Pm4.cpp:688-704 calls GuestMemory::Read, which calls FlushGpuWrites (GuestMemory.cpp:1717-1719). Recorder.cpp:1010-1020 then takes GpuMutex and calls SyncThrough.
- Draw.cpp:305: std::set per record. Lines 328-335: compileDrawStage per patched program. Line 343: lockForDraw inside the loop; the unique_lock persists. Line 346: localDevice->Draw per record.
- Commit fb55cb2c records that PPSA21564's indirect mesh draws had caused about 170 GPU waits per 10 s, and only the count==1 case moved to the GPU.

**Why this impact.** For GPU-driven multi-draw-indirect on the NGG path, one packet becomes a GPU drain (the CPU and GPU take turns, so both sit partly idle) plus N recompiles plus N Graphics::Draw calls, all serialized under GpuMutex.

**Proposal.** (a) Fold-only fast path: when every patched word is the front program's vertexOffsetSgpr or instanceOffsetSgpr and the result reports neither shared nor conflict (RecompileResult fields), skip compileDrawStage and apply fold() to `direct`. This is already how direct draws treat those SGPRs (Draw.cpp:184-192).
(b) Run all records' compiles and snapshots before the first lockForDraw, or unlock between records.
(c) Pass all records to one Graphics::Draw that records several vkCmdDraw* calls.
(d) In Graphics, extend the MeshArguments compute expansion to count>1 and countIndirect, so mesh MDI stays on the GPU.

**Risk.** Medium. (a) needs the analyzer's vertexOffsetShared/Conflict flags to be exact. (d) is a shader-side change.

**How to measure.** With APS5_PROFILE_DRAW=1:
- '[draws] ... indirect: gpu-side N (N rewritten), cpu-side N (non-vertex path N, draw index N ...), argument reads X ms' (Graphics/src/Draw.cpp:395-401).
- '[hooksync]' attribution for the IndirectArguments read site.
For per-packet decisions, run briefly with APS5_TRACE_INDIRECT_DRAWS=1 (IndirectDraw.cpp:51-56).

#### exec-draw-4: A draw with one differing stage re-captures and re-materializes every stage, including stages whose variant validated Equal

- **Impact (claimed):** medium · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawLookup.cpp:87; Draw.cpp:197-210`

**Mechanism.** Validation stops at the first stage that misses, so later stages are never validated. The compile loop then calls compileDrawStage for every stage whenever drawHit is false. For example, when the front (vertex/mesh) stage's constants differ, the pixel stage is still recaptured, rematerialized and deep-copied. cacheDrawStages then builds a fresh variant for it, and insertDrawEntry finds it 'present' only after comparing runs and words element by element.

**Evidence.** - DrawLookup.cpp:87: for (...; !miss && i < programs.size(); ++i).
- Draw.cpp:201-210: if (drawHit) {...} else { results.push_back(compileDrawStage(...)) } for every stage.
- DrawCapture.cpp:42-46: matched regions are only merged when drawHit.
- DrawCache.cpp:74-79: find_if comparing pushOffset, runs and words, then ++present.

**Why this impact.** Most draws have 2 stages (vertex/NGG front plus pixel), or 3 with the geometry back stage. Reusing the matched stages cuts the miss cost by about half when one stage differs, which is the common 'front stage differing' case.

**Proposal.** In lookupDraw, keep validating the remaining stages after a miss (still with a running push cursor). In Draw.cpp's compile loop, reuse a stage when matched[i] != nullptr && matched[i]->pushOffset == pushCursorBytes: set programResults[i] = matched[i]->compiled.get() and append matchedRegions[i] to memory. Compile only the stages that differ. Leave fresh[i] null for reused stages. For recipeStages, use matched[i] where fresh[i] is null.

**Risk.** Low-medium. LinkedProgram carries only binaries and user-data spans (Recompiler.hpp:222-228), so a reused stage's compile inputs are unchanged. The push-offset check guards the layout.

**How to measure.** With APS5_PROFILE_DRAW=1:
- '[draw-cache] ... N stage validations (N equal ...)' and the miss reasons.
- '[draw] driver phases' capture and recompile µs per packet before and after.
- '[capture] N captures'.

#### exec-draw-5: Every compiled draw stage deep-copies the shared, immutable RecompileResult

- **Impact (claimed):** medium · **Effort:** S · **Category:** allocation-copy · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawCapture.cpp:79-82; Draw.cpp:207-209,282-290`

**Mechanism.** compileDrawStage gets a shared_ptr<const RecompileResult> from the result memo and returns `RecompileResult result = *stageCapture.compiled;` by value. That copies spirv (shared), the bindings vector, and for each DescriptorBinding up to 12 vectors (guestDescriptor and several vector<bool>), plus pushConstants, vertexAttributes, parameterExports and fragmentParameters. On CPU-indirect hits, Draw.cpp copies every stage's result again.

**Evidence.** - DrawCapture.cpp:79-80: stageCapture.compiled = Recompile(request, *capture); then ShaderRecompiler::RecompileResult result = *stageCapture.compiled.
- Recompiler.hpp:402-424: RecompileResult fields. Recompiler.hpp:312-340: DescriptorBinding, with 12 vector members.
- Recompiler.cpp:569-575: a memo hit returns the shared result, so the copy is pure overhead.
- Draw.cpp:285: results.push_back(RecompileResult(*programResults[i])) for indirect CPU draws that hit.

**Why this impact.** A material pixel shader with 10-20 bindings means about 50-200 heap allocations plus descriptor word copies per stage per miss. That multiplies the miss cost from exec-draw-1 and exec-draw-3.

**Proposal.** Make compileDrawStage return the shared_ptr (stageCapture.compiled) and set programResults[i] = stageCapture.compiled.get(), as the hit path does with matched[i]->compiled.get(). Keep the `results` vector only for rect-list stages and for the indirect-CPU patch path, and copy only on a real patch. Rect-list already uses its own results slots.

**Risk.** Low. Lifetimes are held by stageCaptures until the draw returns.

**How to measure.** '[draw] driver phases ... recompile' µs per packet with APS5_PROFILE_DRAW=1, or a heap profiler / perf on the queue-0 worker.

#### exec-draw-6: Fixed overhead on the hit path: about 25-35 heap allocations, an unconditional ShaderMemory construction and 3-4 drawCacheMutex acquisitions per draw

- **Impact (claimed):** medium · **Effort:** M · **Category:** allocation-copy · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:72,89,122-159,195-196,253-258,353-361; DrawLookup.cpp:78,118-156`

**Mechanism.** A full cache hit still does the following:
- Copies decode->programs: 1 + P allocations, P userData copies, P shared_ptr refcount operations.
- Grows the memory and linked vectors.
- Allocates about 12 per-draw vectors: vertexInfos (about 900 B per program), decodeReads, results.reserve, stages, programResults, stageCaptures, matched, matchedRegions plus P inner vectors, fresh, recompiled, ranks, pushOffsets, resultIndex, snapshots, recipeStages.
- Constructs a ShaderMemory that is never used on a hit. Its constructor copies and sorts the regions and creates 2P std::map nodes.
- Takes drawCacheMutex 3-4 times: key lookup, the counters/rotate step, findDrawRecipe and attachDrawRecipe.
- On a rank>0 hit, rebuilds the DrawEntry (make_shared plus a copy of every stage's variant vector) under the mutex.
- KnownDrawRejection builds a vector key and takes a global mutex.

**Evidence.** - Draw.cpp:89: std::vector<DrawProgram> programs = decode->programs.
- Draw.cpp:122-128: memory and linked. Lines 132-133, 145-159, 195-196: per-draw vectors.
- Draw.cpp:144: ShaderMemory shaderMemory(memory, ...). ShaderMemory.cpp:90-95 builds RequestMemoryView (RequestMemoryView.cpp:19-37: copy and sort) and does initial.emplace into a std::map.
- Draw.cpp:253-258: snapshots. Lines 353-356: recipeStages.
- Draw.cpp:72, DrawLookup.cpp:118, DrawCache.cpp:113 and 128: drawCacheMutex.
- DrawLookup.cpp:136-150: rotated entry copy.
- Graphics/src/Draw.cpp:1485-1491: KnownValidationFailure (vector key plus validationMutex). It runs per draw without a recipe (Draw.cpp:365).

**Why this impact.** Estimated 2-5 µs per draw. At 1,000-3,000 draws per frame that is 2-15 ms on the single queue-0 thread. It matters once misses are fixed.

**Proposal.** - Use thread_local scratch vectors through the existing ShaderRecompiler::ThreadOwned pattern (as CaptureValidation.cpp:18-19,63-64 does), or fixed std::array storage, since programs are at most 5 (DrawDecode.cpp:53).
- Construct ShaderMemory lazily (std::optional, built on the first compileDrawStage).
- Use a const reference to decode->programs unless the draw is mesh or CPU-indirect, which are the only paths that mutate userData.
- Take drawCacheMutex once: return the entry and its recipes snapshot together from the key lookup, and fold the counters into thread-local tallies flushed with the profile report.
- Replace the rotate-on-hit copy with a per-stage atomic preferred-rank hint.

**Risk.** Low.

**How to measure.** With APS5_PROFILE_DRAW=1:
- '[draw] driver phases ... vectors, program prepare, decode' µs per packet on a hit-heavy scene.
- '[draw-cache] ... validated in X us each'.

#### exec-draw-9: Validation can wait on the GPU before the cheap compare, once per variant, and draw-written buffers always force a sync

- **Impact (claimed):** medium · **Effort:** M · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Dispatch/VariantValidation.cpp:20-111; Driver/Memory/WrittenBuffers.cpp:39-43; Draw.cpp:344,371`

**Mechanism.** validateVariant first runs syncPendingRuns over the regions that overlap pending GPU writes. When the raw bytes match, it calls FlushGpuWrites (the flush hook submits and waits on the GPU) before it compares the variant's other runs. A variant whose non-pending runs differ still pays that sync, and lookupDraw repeats this for up to 4 variants per stage. noteDrawWriters registers every buffer a draw writes as a foreign writer (program 0), so classifyPendingWrite returns Sync (syncedForeign) for any later read of draw-written memory. The evidence and known-value skips never apply to it. With write evidence on, each synced region also allocates and memcpys a `before` copy.

**Evidence.** - VariantValidation.cpp:76 calls syncPendingRuns before validateCaptured at :106. Lines 24-45: pending overlap, classify, CompareMapped, then GuestMemory::FlushGpuWrites. Lines 37-41: `before` allocation.
- DrawLookup.cpp:93-110: loops over the variants, calling validateVariant each time.
- WrittenBuffers.cpp:39-43: noteForeignWriter(begin, end, queue), with program=0. WriteEvidence.cpp:158-161: writer->program == 0 gives syncedForeign and Sync.
- Recorder.cpp:1010-1020: the hook takes GpuMutex and calls SyncThrough.
- Defaults (WriteEvidence.cpp:34-67): APS5_NO_WRITE_EVIDENCE, APS5_NO_FOREIGN_WRITERS and APS5_NO_VALIDATE_SKIP are all unset, so all three features are ON.

**Why this impact.** Each sync is a submit plus a wait for GPU completion on the single draw thread. The CPU then stalls and the GPU drains, which matches the reported under-use of both. The frequency depends on how many SRT reads overlap pending GPU writes.

**Proposal.** (a) In validateVariant, call CompareMapped on all non-pending regions first and return Differing before any syncPendingRuns.
(b) In lookupDraw, choose the candidate variant by comparing non-pending runs across all variants, then sync once for that one.
(c) Record draw-written buffers with a program identity: call noteWrittenBuffers with the pixel or front program's codeAddress instead of noteForeignWriter, so the writer-keyed evidence and known-value policies can skip syncs.

**Risk.** (a) and (b): low, the order changes but the outcome does not. (c): medium, because the evidence model must hold for raster-order writes. Keep the streak threshold.

**How to measure.** With APS5_PROFILE_DRAW=1:
- '[draw] driver phases ... validate GPU wait, capture hook waits'.
- '[validate] ... synced N waiting X ms (... foreign writer N, label N ...)' (CaptureDiagnostics.cpp:34).
- '[hooksync] N pending-write syncs waited X ms' attribution for the DrawCache site.

#### exec-draw-8: A queued label that overlaps a capture restarts the whole draw after a stale variant was already inserted into the cache

- **Impact (claimed):** low · **Effort:** S · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:222-229; Driver/Synchronization/DeferredLabels.cpp:162-183`

**Mechanism.** cacheDrawStages inserts the freshly captured variants before recordQueuedLabelsAfterCapture checks whether a deferred label (RELEASE_MEM or WRITE_DATA) overlaps the captured memory. On overlap, the labels are recorded and Driver::draw recurses from the start, redoing key, decode, validation and capture. The stale variant takes one of the stage's 4 slots. On the retry, the label-written runs classify as Sync (syncedLabel), which costs a GPU wait. Separately, every draw while labels are queued try_locks GpuMutex just to record them.

**Evidence.** - Draw.cpp:222: cacheDrawStages(...). Line 229: if (recordQueuedLabelsAfterCapture(submission.queue, memory)) return draw(queue, packet, submission, rejected).
- DeferredLabels.cpp:165-171: overlap check, then recordQueuedLabelsBeforeRead and ++captureRetries. Lines 174-182: try_lock on GpuMutex.
- VariantValidation.cpp:93-97: validation already detects QueuedLabel (counted as a Gate miss).
- WriteEvidence.cpp:168-177: a label in range gives Policy::Sync.
- Default: DeferLabels() ON (DeferredLabels.cpp:15-19; APS5_LABEL_LOCK_EACH and APS5_DRAIN_COMPLETION_LABELS unset).

**Why this impact.** It wastes a full miss plus one GPU sync per affected draw, and pollutes the variant slots. It matters only for draws that read label-written memory; the counters show how often that happens.

**Proposal.** - When lookupDraw reports EntryOutcome::QueuedLabel for any stage, or before the first compileDrawStage, call recordQueuedLabelsBeforeRead and then capture.
- Move the overlap check ahead of cacheDrawStages so a restart never inserts.
- On restart, reuse the already-resolved decode instead of recursing into Driver::draw.

**Risk.** Low.

**How to measure.** With APS5_PROFILE_DRAW=1: '[labels] ... after a capture: N groups recorded by a try, N packets redone for a queued label over the capture' (PacketFlush.cpp:102), and the '[draw-cache]' gate misses.

#### exec-draw-10: Draws that throw are never negatively cached, so they redo decode, capture and recompile every frame and build diagnostic strings

- **Impact (claimed):** low · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Packets/PacketExecution.cpp:246-285; Draw.cpp:30-388`

**Mechanism.** An exception from decode, compile or Graphics::Draw unwinds Driver::draw without inserting anything. The same key misses again next frame and repeats all the work before the throw. Each failure also builds what+suffix strings, inserts into a static std::set<pair<uint64,string>> (a string compare), and runs reportSkip (a global mutex, a string hash and a set insert). Only source-resolve failures are memoized (FailureMemo, ShaderRegistry.cpp:81-84,131-138); validation failures are memoized inside Graphics::Draw (KnownValidationFailure).

**Evidence.** - PacketExecution.cpp:281-284: catch, skipped(error.what()), CountDrawSkip(Thrown). Lines 246-266: string building, dumpedTargets.insert, reportSkip.
- ShaderDiagnostics.cpp:17-27: lock, then std::hash<std::string> and an insert.
- No drawCache insert on a throw (Draw.cpp:222 is never reached).

**Why this impact.** The cost scales with the number of thrown draws per frame, which is unknown and visible in the [draws] line. Each one costs a full miss (tens of µs or more) plus about 1-3 µs of string work.

**Proposal.** When the throw happens before any guest-memory-dependent step (that is, in decodeDraw or Graphics::DecodeState, which are pure functions of key-covered registers), store the message in a small map keyed by drawKey under drawCacheMutex. Check it right after the key lookup (Draw.cpp:76) and return DrawVerdict::Rejected. Cache only those failures, not capture or Graphics failures that depend on memory.

**Risk.** Low when limited to register-only failures.

**How to measure.** '[draws] ... skipped packets: nothing N in X ms, prechecked N in X ms, thrown N in X ms' (Graphics/src/Draw.cpp:403-406), and the '[draw] driver phases ... skipped' row, with APS5_PROFILE_DRAW=1.

#### exec-draw-11: Per-draw precheck and register-key hashing redo pure register work that the cache entry could carry

- **Impact (claimed):** low · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawPrecheck.cpp:14-67; DrawRegisterKey.cpp:9-57`

**Mechanism.** Before the key lookup, each draw runs DecodeColorMetadataPass, DepthMaintenanceRejection, ColorWriteMask, the restart checks and Graphics::DrawRejection (about 30 register finds with std::string returns). drawRegisterKey then mixes up to about 390 registers, two serial 64-bit multiplies each (offset, then value). For every shader-bank register it runs an any_of over 3 user ranges, then does 5 ShaderRegistry upper_bound walks. All of this is a pure function of registers the key already covers.

**Evidence.** - DrawPrecheck.cpp:16, 26, 32-41, 42-61, 62-65.
- State.cpp:824-882 (DrawRejection).
- DrawRegisterKey.cpp:24-26 (unread any_of), 33-36 (mix(it->first); mix(it->second)), 39-55 (registry walks).
- State.hpp:154-171: 45 ranges, about 390 registers.
- Default: drawPrecheck ON (APS5_NO_DRAW_PRECHECK unset, DrawPrecheck.cpp:9-12).

**Why this impact.** Estimated 1-3 µs per draw, which is 1-9 ms per frame at 1,000-3,000 draws on the critical thread.

**Proposal.** - Mix (offset << 32 | value) as one word, which halves the serial multiply chain.
- Precompute per-bank masks of the hashed user words once per draw instead of calling any_of per register.
- Store the precheck verdict, or the fact that it passed, in DrawEntry and skip DrawRejection on a register-key hit. Keep the cheap DecodeColorMetadataPass and indexCount checks.

**Risk.** Low. The precheck reads only registers that DrawKeyRegisters covers (the facade test exists in tests/Graphics.cpp:198-211).

**How to measure.** With APS5_PROFILE_DRAW=1: '[draw-cache] register key ... key build X us each', and the 'precheck' row of '[draw] driver phases'.

#### exec-draw-12: Global locks per run in variant validation: the StorageTexture pending registry and the unit-shadow registry

- **Impact (claimed):** low · **Effort:** S · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Dispatch/VariantValidation.cpp:81-90; Graphics/src/Texture.cpp:2433-2450; Graphics/src/UnitShadow.cpp:483-505`

**Mechanism.** For each validated variant of each stage, validateVariant calls StorageTexture::AnyPendingOverlaps(variant.runs) and AnyShadowedOverlaps(variant.runs):
- The first takes the global pending mutex and scans every pending or flushing texture against every run, which is O(T x R).
- The second takes the shadow registry mutex once per run.
A stage with 50 captured runs means about 50 mutex acquisitions per validation, contended with compute workers and the flush hook that use the same registries.

**Evidence.** - VariantValidation.cpp:81.
- Texture.cpp:2436: std::lock_guard lock(pending.mutex), then loops over pending.textures and pending.flushing.
- UnitShadow.cpp:499-503: AnyShadowedOverlaps(span) calls AnyShadowedOverlaps(begin, bytes) per run. Line 487 locks registry.mutex each time.
- Default: UnitShadowEnabled ON when write-watch is available (UnitShadow.cpp:418-420; APS5_NO_UNIT_SHADOW unset).

**Why this impact.** A few µs per draw, plus lock contention with other queue workers. It grows with the number of captured runs and pending textures.

**Proposal.** - Add a batch AnyShadowedOverlaps(ranges) that takes the registry lock once.
- Short-circuit both checks on a lock-free generation or count: skip them when no texture is pending and no shadow unit is live (PendingSerial and a live-unit counter).

**Risk.** Low.

**How to measure.** '[draw] driver phases ... key/lookup/validate' µs per packet, and '[draw-cache] ... validated in X us each', with APS5_PROFILE_DRAW=1. Run perf on the worker to see the mutex time.

#### exec-draw-13: The capture word reader always increments shared atomic profile counters, even with profiling off

- **Impact (claimed):** low · **Effort:** S · **Category:** logging-debug-overhead · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/ShaderMemory.cpp:101,123,137`

**Mechanism.** ShaderMemory::read runs `++CaptureTotals().reads` (a sequentially consistent locked RMW) for every SRT word the capture walk reads, and page() runs `++CaptureTotals().pages`. Neither is gated on CaptureProfiled(). All counters share one static struct and its cache lines across every queue worker that captures (draw misses and dispatch misses), so the lines bounce between cores.

**Evidence.** - ShaderMemory.cpp:123: ++CaptureTotals().reads; executes unconditionally.
- ShaderMemory.cpp:101: ++CaptureTotals().pages;
- CaptureProfile, ShaderMemory.cpp:20-53: about 20 adjacent std::atomic counters.
- CaptureProfiled() is APS5_PROFILE_DRAW (ShaderMemory.cpp:63-66). The other timers are gated on it; these two counters are not.

**Why this impact.** Hundreds of locked increments per captured stage, with false sharing when queues capture at the same time. Small, but on the miss path that dominates in exec-draw-1.

**Proposal.** Gate both increments on CaptureProfiled(), or keep thread-local tallies and flush them in the 500-capture report (the wordsKnown counters that the [copy] line needs can stay unconditional).

**Risk.** None.

**How to measure.** perf annotate on ShaderMemory::read, and the '[draw] driver phases ... capture' row with APS5_PROFILE_DRAW=1 before and after.

### Incomplete or stubbed code noticed

- `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/IndirectDraw.cpp:23-28`: The GPU-side indirect path covers only vertex-path draws and the single-record mesh case: indexed, count==1, no countIndirect, no base-vertex, start-instance or drawIndex location. All other cases fall back to the CPU path: NGG/mesh or tessellation multi and count-indirect draws, drawIndex in an SGPR, and feature gaps. *Performance relevance:* The CPU path waits on the GPU to read the arguments, then recompiles and runs a full Graphics::Draw per record under GpuMutex (see exec-draw-7). This is a major cost for GPU-driven rendering.
- `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1770; Execution/src/Driver/Draw/Draw.cpp:354-363`: Draw recipes (the fast record path) are built only for direct, cacheable, non-waited draws without a depth target (`!state.depth`), and are only looked up after a full draw-cache hit. *Performance relevance:* Depth-tested 3D geometry, the bulk of a frame, never uses the recipe path. Each such draw also pays a findDrawRecipe that misses plus KnownDrawRejection, then a full Graphics::Draw under GpuMutex.
- `core/libs/prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp:219,249 (dispatch only)`: There is no draw counterpart to PrepareDispatch (the unlocked stage A of the resource build) or PrepareRecipe (the unlocked recipe prechecks). *Performance relevance:* The whole draw resource build, pipeline lookup and creation, and recipe proof run under the process-wide GpuMutex (exec-draw-2).
- `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawLookup.cpp:100; DrawCapture.cpp:92-121`: Draw variants lack the dispatch path's data-hit support (dataPositions, flatBinding, EqualData). *Performance relevance:* Any change in a flattened-SRT constant forces a full capture, materialize and variant insert per draw (exec-draw-3).
- `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawPrecheck.cpp:16-25,35-39,42-60`: Rejected or limited cases:
- CB metadata passes (eliminate fast clear, DCC decompress) run synchronously under GpuMutex at precheck, and indirect ones are unsupported.
- Depth/stencil-only draws on a queue that never set a pixel program address are rejected as 'not implemented'.
- Primitive restart works only for strips, or for lists with VK_EXT_primitive_topology_list_restart, and only with an all-ones index. *Performance relevance:* Rejected draws skip their GPU work but pay the skip and report path. Metadata passes read DCC keys on the CPU and can wait for pending key writes while holding GpuMutex.
- `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Packets/PacketExecution.cpp:281-285`: Draws that throw (unsupported state, recompiler failures) have no negative cache keyed by the draw key. *Performance relevance:* They repeat decode, capture and recompile every frame before failing again (exec-draw-10).
- `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:41-54`: The legacy APS5_LOCKED_DRAW_PREPARE mode (opt-in) takes GpuMutex at the start of the draw and holds it through decode, validation, capture and recompile. *Performance relevance:* Off by default. Setting it would serialize all prepare work against the other queues, so it should not be used for performance.

### Settings to try without code changes

| Setting | Expected effect | Risk |
|---|---|---|
| `APS5_PROFILE_DRAW=1 (optionally with APS5_NO_HOLD_PROFILE=1 to lower its overhead)` | No speedup. It prints every 10 s: '[draw-cache]' hit, no-entry and miss reasons; '[draw] driver phases' µs per packet; '[lock]' waits and holds per site; '[hooksync]'; '[capture]'; '[draws]' with indirect CPU/GPU counts and thrown draws; '[labels]'; '[validate]'. These show whether misses (exec-draw-1/3), GPU syncs (exec-draw-7/9) or GpuMutex holds (exec-draw-2) dominate. | Adds clock reads, lock-profiling overhead and stderr output, so FPS drops somewhat while it is on. |
| `APS5_DRAW_CACHE_ENTRIES=32768` | If '[draw-cache]' shows many 'no entry' lookups and evictions, a larger LRU can keep entries across a 2-3 frame SRT/constant ring, so a draw's key repeats before it is evicted. This raises register-key hits and cuts decode, capture and recompile time. | More memory, because each entry holds a decode and up to 4 variants of captured words per stage. Hits stay unchanged if the pointers never repeat. |
| `APS5_DISPATCH_VARIANTS=8` | Draw entries keep up to 8 variants per stage (DrawCache.cpp:83 uses dispatchVariants()), so draws that alternate among more than 4 constant sets under one key can hit. | Each miss validates up to 8 variants, each of which may sync on pending writes. It also changes the dispatch cache (8 variants per entry, 8192 entries). |
| `APS5_BATCH_CAP=256 (A/B against the default 64, and against 32)` | Fewer SubmitRecorded/vkQueueSubmit calls under GpuMutex per frame (PacketFlush.cpp:46 caps at 64 recorded works per submit by default), so less lock-held submit time on queue 0. | Larger batches can start GPU work later and reduce CPU/GPU overlap. Measure FPS and the '[lock]' Flush site. |
| `APS5_WORKER_AFFINITY=1 (Windows on hybrid Intel CPUs only)` | Pins the queue workers and the presenter to P-cores (WorkerAffinity.cpp:13-17). Linux already does this by default on hybrid CPUs. This avoids the single-threaded queue-0 draw path running on an E-core. | Minimal. No effect on non-hybrid CPUs. |
| `APS5_WRITE_EVIDENCE_AFTER=2` | The write-evidence streak needed to skip a sync drops from 4 to 2 (WriteEvidence.cpp:87), so fewer validation and capture syncs on pending GPU writes ('validate GPU wait' and 'capture hook waits'). | Higher chance of reading stale data from a GPU writer that changes its output, which shows up as rendering glitches. |
| `APS5_TRACE_INDIRECT_DRAWS=1 (short run only)` | No speedup. It logs each indirect draw's CPU-or-GPU decision and the reason (IndirectDraw.cpp:51-56, DrawPrecheck.cpp:28-30), which shows whether Astro Bot's NGG multi-draws take the CPU path. | One stderr line per indirect draw packet, which is slow. |

<a id="graphics-draw"></a>
## Graphics-side draw recording (core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp and its helpers: Pipeline, Recorder render-pass handling, DCC/CMASK, storage-target residency, draw snapshots, DepthSurface, ColorTarget transfer)

### How it works

Graphics::Draw (Draw.cpp:1494) runs once per DRAW packet, called by VulkanDevice::Draw (VulkanDevice.cpp:2530-2537) after Driver::draw has decoded the state and found the shaders. prepareDrawInputs (Draw.cpp:783-907) validates the draw and memoizes the fragment-output validation. It then copies the index buffer on the CPU and scans it for the highest index, and copies each vertex range into host-visible Buffers. Those copies are reused only when the write watch proves the memory unchanged (Recorder draw snapshots).

For each color target, the draw looks up the cached StorageTexture of the surface (CachedStorageSurface, then StorageTexture::Refresh: FlushPending, CollectWrites, key proof, upload if changed). It then scans the DCC and CMASK keys (materializeRegisterClear and materializeCmaskClear). Depth uses a host-only DepthSurface image.

When every target is resident (the default), the draw is recorded:
- resolveDrawResources reuses a ResourceCache template (Revalidate) or builds a ShaderResources.
- CachedPipeline finds or creates the VkPipeline, VkRenderPass, layout and shader modules; AcquireFramebuffer follows.
- recordDraw continues the open render pass only if the target views and extent are the same, the previous draw wrote no memory, there is no feedback and the draw is not GPU-indirect. Otherwise the old pass ends with an ALL to ALL barrier, and a new ALL_COMMANDS barrier plus vkCmdBeginRenderPass is recorded.
- PrepareDrawBindings copies read-only in-place buffers into snapshot Buffers and a second, copied descriptor set.
- Bind, push constants, draw. keepRecordedDraw keeps the objects until the fence and marks the targets dirty.

The driver submits the batch every 64 recorded works (APS5_BATCH_CAP), or at labels and syncs. Targets that are not resident go through a synchronous path: CPU read of the whole target, GPU detile, SubmitAndWait, CPU write-back.

### Threads and locks

The graphics queue's PM4 runs on its worker thread. By default Driver::draw decodes the state and looks up shaders without the lock (APS5_LOCKED_DRAW_PREPARE is unset). Then lockForDraw (Execution/src/Driver/Draw/Draw.cpp:260-271) takes GuestMemory::GpuMutex, which is recursive and process-wide, and holds it for the whole Graphics::Draw:
- index and vertex copies
- target Refresh and uploads
- DCC and CMASK scans
- the full ShaderResources build (draws have no unlocked stage A, unlike dispatches)
- vkCreateGraphicsPipelines on a pipeline miss
- descriptor allocation and update
- recording

Flush-hook waits nested in a draw (FlushForAccess, then SyncThrough or Sync, meaning submit plus a fence wait) also run with the lock held.

The inner mutexes taken per draw are:
- validationMutex
- the StorageTextureCache mutex (Refresh runs under it)
- the DCC memo mutex
- the ResourceCache mutex
- Pipelines().mutex
- the DescriptorCache mutex
- the DepthSurface mutex
- the write-tracker lock
- the pending-image registry mutex

While the GpuMutex is held, other queue workers (async compute dispatch recording), the presenter, reapCompletionLabels (try_lock fails) and game threads that hit the flush hook on pending ranges all block. A frame's draws therefore run one after another on a single core, and they also shut out work on the other queues. That matches the user's observation that threads sit idle and the GPU is starved.

### Findings

#### graphics-draw-1-locked-draw: The whole draw runs under the global GpuMutex on one worker; draws have no unlocked resource-build stage and compile pipelines under the lock

- **Impact (claimed):** critical · **Effort:** L · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:260-271,383; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:992,1740; core/libs/prx/libSceAgcDriver/Graphics/include/ShaderResources.hpp:89-97`

**Mechanism.** Driver::draw takes GuestMemory::GpuMutex in lockForDraw, then calls localDevice->Draw, which reaches Graphics::Draw. Everything in Graphics::Draw executes inside that hold:
- index and vertex CPU copies and the index scan
- per-target CachedStorageSurface/Refresh plus the DCC and CMASK scans
- the ShaderResources build: descriptor set allocation and writes, host-import lookups, texture cache lookups each with their own Refresh
- CachedPipeline, which on a miss runs vkCreateShaderModule per stage, vkCreatePipelineLayout, vkCreateRenderPass and vkCreateGraphicsPipelines
- AcquireFramebuffer (vkCreateFramebuffer on a miss)
- PrepareDrawBindings memcpy snapshots and descriptor copies
- recording
The dispatch path already splits its build: stage A runs without the lock (ShaderResources deferred ctor; VulkanDevice::PrepareDispatch at VulkanDevice.cpp:3040) and only Complete() runs under the lock (VulkanDevice.cpp:3416-3420). Draws always construct ShaderResources fully under the lock (Draw.cpp:992; the draw ctor at ShaderResources.hpp:90 has no deferred flag). Nested flush-hook waits then happen with the mutex held, so every other queue and the presenter wait too.

**Evidence.** Execution Draw.cpp:260-264: `const auto lockForDraw = [&] { ... GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Draw); gpuLock.lock();`; line 383 `localDevice->Draw(graphics, drawParameters, stages, snapshots, ...)` runs after it.
Graphics Draw.cpp:992 `resolved.resources = std::make_shared<ShaderResources>(context, shaders, state.color, draw.indexAddress, ..., snapshots);`.
ShaderResources.hpp:91-97: 'Two-stage build for dispatches (see build): with `deferred` the constructor runs stage A only, which needs no device lock, and Complete() runs stage B under GuestMemory::GpuMutex' (compute only).
Pipeline.cpp:126/142/181/248 create objects inside CachedPipeline (Draw.cpp:1740).
APS5_LOCKED_DRAW_PREPARE is opt-in (Execution Draw.cpp:41), so by default only the decode and shader stage are unlocked.
Draw.cpp:254-255 comment: 'the [lock] line's 'draw' hold max (tens of ms against an average well under a millisecond)'.

**Why this impact.** All draw recording for a frame is serial on one core while holding the lock every queue needs. A per-draw locked cost of a few tens of microseconds times 2,000-3,000 draws gives 60-120 ms per frame, the same order as the observed ~110 ms (8-9 FPS) frame. The other cores and the async-compute queues sit blocked on the mutex, and the GPU is fed in bursts.

**Proposal.** 1) Give draws a deferred build, mirroring dispatches. Add a `deferred` overload of the draw ShaderResources ctor that runs buildPrepare only (ShaderResources.cpp:889). Call it from Driver::draw before lockForDraw (Execution Draw.cpp around lines 361-371), pass it through VulkanDevice::Draw, and in resolveDrawResources call Complete() (buildComplete, ShaderResources.cpp:1031) instead of constructing. Probe SharedResourceCache().Find unlocked, as PrepareDispatch does.
2) Precompute the lock-free parts into a PreparedDraw struct outside the lock: CachedFragmentOutputs (own mutex), BuildVertexInputLayout, DrawResourceKey, pipelineKey and CachedPipeline (Pipelines().mutex; the layout key comes only from the stages' bindings), and DepthSurfaceView (own mutex). Graphics::Draw then only records.
3) Create missing pipelines on a small job pool keyed by pipelineKey, so a first-use vkCreateGraphicsPipelines never runs under the GpuMutex. Have the draw wait on a future outside the lock, or skip the draw for one frame.

**Risk.** Stage A reads memory that can change before the lock. The dispatch path already handles this with the stage B re-check, presync and PrecollectSurfaces, and draws must reuse those proofs. Creating pipelines outside the lock must respect device replacement; the store entries already carry the device and pool identity.

**How to measure.** APS5_PROFILE_DRAW=1, then read:
- the [lock] line per queue: acquisitions, waits and hold time by site 'draw'
- the [draws] phase totals (resources, pipeline, readTarget, record) and 'longest draw ... longest phase'
- the [resources] stage A/B split for dispatches, as the reference for what a draw split saves

#### graphics-draw-7-write-watch: Every per-draw reuse proof depends on the host write watch: on Linux without PAGEMAP_SCAN or async userfaultfd write-protect, draws re-copy, re-scan and round-trip their targets

- **Impact (claimed):** critical · **Effort:** M · **Category:** memory-tracking · **Default path:** yes
- **Location:** `core/libs/prx/libc/src/GuestWriteWatch.cpp:131-158,257-307; core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:1078-1123,1342-1348; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:742-748; core/libs/prx/libSceAgcDriver/Graphics/src/Texture.cpp:1229-1330`

**Mechanism.** GuestMemory::CollectWrites returns 0 when the tracker is not watched. On Linux the watch exists only if the build saw PAGEMAP_SCAN and UFFD_FEATURE_WP_ASYNC (kernel 6.7 headers) and the running kernel supports them. Otherwise:
- CopyDrawInput never reuses a snapshot (`if (copy.generation != 0)`) and KeepDrawInput returns early, so every draw re-copies index and vertex data and re-scans indices.
- PrepareDrawBindings re-snapshots every read-only buffer.
- ProvedClearKeys cannot prove anything, so it scans.
- StorageTexture::Refresh sees every unit as changed: ChangedBlocks fills BlockWritten and returns false. droppable() is false without tracking, so a target with pending results is written back and re-uploaded, per draw per target.
If the headers were missing at build time, no message is printed.

On Windows, CollectWrites walks GetWriteWatch, which costs in proportion to the pages covered (GuestMemory.cpp:1025-1067). The walk is memoized only within a collect epoch, and the epoch is bumped at submissions, reaps, waits and drains (PacketExecution.cpp:106, QueueWorker.cpp:34), with a 64-entry per-thread ring (GuestMemory.cpp:997-1000). So large targets are walked again after every bump.

**Evidence.** GuestWriteWatch.cpp:297: `#if defined(__linux__) && defined(PAGEMAP_SCAN) && defined(UFFD_FEATURE_WP_ASYNC)`, else `return false;`.
Line 157: '[memory] write watch unavailable: %s failed (%s); guest memory is compared instead'.
GuestMemory.cpp:1343-1348: ChangedBlocks pre-fills BlockWritten and returns false when unwatched.
Texture.cpp comment: 'Without a generation (no tracking) a unit's stamps say nothing, so it is stored.'
Draw.cpp:744: `if (copy.generation != 0) copy.buffer = recorder->ReusableDrawSnapshot(...)`.
APS5_NO_WRITE_WATCH (opt-out) is unset by default.

**Why this impact.** This depends on the host, but it is the default behaviour on any Linux host or build without the kernel feature. There, each draw copies O(index + vertex + constant) bytes and moves O(target size) through GPU write-back and re-upload, which is far beyond any other cost. On Windows and on watched Linux hosts, the per-epoch page walks of the targets cost tens of microseconds per target per draw.

**Proposal.** - At startup, log clearly (and fail loudly in Release) when GuestWriteWatchAvailable_nid_postfix() is false, including when the feature was compiled out.
- Document that Linux needs a 6.7+ kernel and 6.7+ headers.
- Add a fallback tracker for older kernels: /proc/self/clear_refs soft-dirty bits, or mprotect with SIGSEGV.
- Enlarge ThreadCollectMemo (64 entries) to cover a draw's working set of targets, buffers and keys.
- Consider bumping the collect epoch only at real ordering points that affect CPU writes.

**Risk.** A soft-dirty or mprotect fallback has its own overhead and signal-safety problems. Over-long epochs risk missing CPU writes, so the epoch semantics must stay conservative.

**How to measure.** - Grep stderr for '[memory] write watch unavailable'.
- APS5_PROFILE_DRAW=1: [draws] 'target lookups: full-scissor/partial/pages walked/us' and 'resident lookups ... >= 1 ms'; [texture] reused versus uploads.
- APS5_TRACE_UPLOAD=1 shows '[upload] ... untracked' reasons.

#### graphics-draw-2-cache-capacity: Resource-cache, pipeline-store and framebuffer bounds are far below an AAA frame's working set, so draws fully rebuild and pipelines are recreated every frame

- **Impact (claimed):** high · **Effort:** M · **Category:** shader-pipeline-compile · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2038-2050; core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:514-562,314; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1719`

**Mechanism.** ResourceCache::Insert holds at most APS5_RESOURCE_CACHE_ENTRIES entries (default 1024), shared by dispatch and draw templates. Draw keys contain every texture, sampler and written-buffer descriptor word (ContentKey, ShaderResources.cpp:1192-1236), so each material/texture combination is its own entry. With several thousand distinct draws plus hundreds of dispatches per frame, last frame's entries are evicted before they can be reused.

The pipeline store is hard-bounded at 256 entries (Pipeline.cpp:552) and evicts least-recently-used entries no in-flight batch holds. Re-creating an evicted pipeline runs vkCreateShaderModule per stage (126), vkCreatePipelineLayout (142), vkCreateRenderPass (181) and vkCreateGraphicsPipelines (248), all under the GpuMutex. The persistent VkPipelineCache only saves the driver back-end compile.

Stages without a variantId get a private pipeline on every draw (Pipeline.cpp:520-523, `++store.uncached; return std::make_shared<Pipeline>(...)`), and so does a 64-bit hash collision (537-540). Framebuffers are cached per Pipeline with a bound of 8 (Pipeline.cpp:314).

The draw path's Insert (Draw.cpp:1719) passes no `evicted` vector, so displaced templates are destroyed inline under the lock. The dispatch path instead hands them to recorder->Keep (VulkanDevice.cpp:3400-3406, APS5_NO_DEFERRED_EVICTION).

**Evidence.** ShaderResources.cpp:2041-2045: `const char* value = std::getenv("APS5_RESOURCE_CACHE_ENTRIES"); const auto parsed = value ? std::strtoull(value, nullptr, 10) : 1024ull;`.
Pipeline.cpp:552: `constexpr std::size_t bound = 256;`.
Pipeline.cpp:520-523: `if (key.empty()) { ++store.uncached; return std::make_shared<Pipeline>(context, state, vertexInput, resources, shaders, attachmentLayout); }`.
Draw.cpp:1719: `SharedResourceCache().Insert(contentKey, resources);`.
Defaults: APS5_NO_DRAW_RESOURCE_CACHE, APS5_NO_TEXTURE_CACHE and APS5_NO_PIPELINE_CACHE are all unset, so the caches are on but bounded.

**Why this impact.** If the per-frame working set is larger than the bounds, the hit rate across frames is close to zero. Every draw then pays a full build (set allocation and writes, import and texture lookups with Refresh). Every frame also re-creates pipelines past 256; each costs roughly 0.1-1 ms even with a driver cache hit, so 100 evictions per frame alone is 10-100 ms. This could become critical if [pipecache] shows evictions or private pipelines.

**Proposal.** - Raise the resource-cache default to about 16384, or give draws and dispatches separate capacities.
- Make the pipeline bound configurable (for example APS5_PIPELINE_CACHE_ENTRIES, default 4096).
- Deduplicate VkShaderModule by variantId and VkRenderPass by attachment format, layout and export key; compatible passes may be shared. A pipeline miss then costs only vkCreateGraphicsPipelines.
- Key framebuffers globally by (render-pass-compatibility key, views, extent) instead of per Pipeline with a bound of 8.
- At Draw.cpp:1719, pass a `std::vector<std::shared_ptr<ShaderResources>> evicted` and recorder->Keep each entry, as VulkanDevice::dispatch does.
- Log once per stage whenever a draw falls to the private-pipeline path.

**Risk.** Cached ShaderResources pin descriptor sets, textures and storage images past the texture-cache budgets (comment at ShaderResources.cpp:2038-2040), so memory and VRAM use grows. More pipelines use more driver memory. Shared render passes must remain compatible: same formats, samples, layouts and attachment count.

**How to measure.** APS5_PROFILE_DRAW=1, then read:
- [rescache] 'draws: N hits, N misses, N invalidated, N uncacheable', plus the miss-churn line
- [pipecache] 'lookups: hits, misses, private (no variant id), evicted, cached'
- [descriptors] sets, pools and dedicated pools
- [draws] 'per kind (avg us, count): recipe hit / template hit / build / BDA'

#### graphics-draw-3-dcc-scan: Every draw scans the whole DCC key range (and the CMASK) of every color target, repeating work Refresh just proved

- **Impact (claimed):** high · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:114-153,913-927; core/libs/prx/libSceAgcDriver/Graphics/src/DccMetadata.cpp:414-471`

**Mechanism.** refreshResidentTarget runs for every resident color target of every draw (Draw.cpp:1562; also in DrawWithRecipe at 2074). Its lookup() runs StorageTexture::Refresh, which already establishes the DCC keys from write stamps (Texture.cpp:1259 `keys = ProvedKeys();`, which goes through ProvedClearKeys with KeyFastPath on by default, DccMetadata.cpp:583-617).

materializeRegisterClear (Draw.cpp:116) then calls CurrentDccKeys(color.dccAddress, color.bytes) without any proof. currentDccKeys leads to readDccKeys, which takes the memo mutex, checks AnyShadowedOverlaps, and runs AllKeysEqual over color.bytes/256 bytes (DccMetadata.cpp:436-438). In steady state the keys are all 0xff after the first write, so the 8-byte compare loop runs to the end every time.

materializeCmaskClear (Draw.cpp:139-141) does the same over cmaskBytes (4 KiB per 1024x512 metablock, State.cpp:686-691) for every target with CMASK enabled.

When the title's clear kernel wrote the keys and is still pending in the batch, currentDccKeys instead calls recorder->SyncThrough (DccMetadata.cpp:462-468). That is a full GPU wait under the lock, once per compute-cleared target per frame. Fills recorded by the DMA HLE are memoized and do not wait.

**Evidence.** Draw.cpp:116: `if (CurrentDccKeys(color.dccAddress, color.bytes) != DccKeys::ClearRegister) return;`.
Draw.cpp:140: `const auto state = CurrentDccKeys(color.cmaskAddress, metadataBytes);`.
DccMetadata.cpp:437: `const bool uniform = AllKeysEqual(keys + 1, count - 1, first);`.
DccMetadata.cpp:28-30 comment: 'an all-0xff (uncompressed) surface's keys are read to the end each time (132 KiB for a 4K RGBA8 target)'.
Sizes in SW_64KB_R_X: 2560x1440 RGBA16F is about 30 MB, so 118 KiB of keys; 1440p RGBA8 is about 15.7 MB, so 61 KiB.

**Why this impact.** A 3-4 render-target (MRT) G-buffer or forward draw scans about 250-450 KiB. Over 2,000-3,000 draws that is about 0.5-1.3 GB of byte compares per frame on the locked thread, roughly 15-40 ms per frame. A single-threaded, early-exit loop runs at about 20-40 GB/s.

**Proposal.** Replace the unproven scan with a proved one.
- Add a raw-key variant to DccMetadata, for example `DccKeys ProvedRawKeys(std::uint64_t meta, std::size_t count, DccKeyProof& proof)`. It mirrors ProvedClearKeys:
  1. Collect first: `collected = GuestMemory::CollectWrites(meta, count)`.
  2. If `proof.generation != 0 && GuestMemory::UnchangedSince(meta, count, proof.generation)` and `!Recorder::SnapshotWriteOverlaps(meta, count)`, return the cached raw ByteKeys.
  3. Otherwise scan and store a proof only when the result is stable.
- Keep one proof per target. StorageTexture already has a foreignKeyProofs ring (Texture.cpp:1452-1458) to hold it; alternatively use a small map keyed by dccAddress and cmaskAddress.
- Use it in materializeRegisterClear and materializeCmaskClear.

The per-draw cost falls from O(bytes/256) to O(number of 64 KiB stamp blocks), about 1-4 checks.

**Risk.** Keep the 'collect before scan' order and treat memoized or pending results as unproven (as ProvedClearKeys does). Otherwise a title clear kernel writing the keys between draws would be missed. The pending-write case must still take the SyncThrough or PendingStoreKeys path.

**How to measure.** APS5_PROFILE_DRAW=1, then read:
- [dcc] 'N scans, X MiB scanned, N memo hits, ... Y ms; key proofs: proved/scanned/unstable'
- [draws] readTarget phase total and 'target lookups ... us'
- [recorder] top sync sites for SyncThrough calls from currentDccKeys

#### graphics-draw-4-snapshot-bindings: PrepareDrawBindings copies every read-only in-place buffer and allocates and copies a second descriptor set for every recorded draw

- **Impact (claimed):** high · **Effort:** L · **Category:** allocation-copy · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2775-2862 (called from core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1362); core/libs/prx/libSceAgcDriver/Graphics/include/Recorder.hpp:94-95; core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:2222-2257`

**Mechanism.** recordDraw calls resources.PrepareDrawBindings for every recorded draw, whether the template was just built or hit. The function walks every allocation, and each read-only guest buffer read in place through a host import, plus every moved or data-word override, becomes a CPU snapshot:
- CollectWrites, then a std::map lookup in ReusableDrawSnapshot
- on a miss, a new pooled host-visible Buffer and a memcpy of the whole descriptor view (ShaderResources.cpp:2809-2815)

If anything was selected, it then:
- allocates a new set through DescriptorCache::Allocate (mutex plus vkAllocateDescriptorSets; the newest pool is tried first and fuller pools are retried)
- runs vkUpdateDescriptorSets copying every binding of the template set (2830-2841), then the snapshot writes (2842-2858)
- keeps the result alive with recorder.Keep

So a fresh build writes two descriptor sets per draw. A set larger than the chain pool limits (for example a bindless table over 1024 sampled images) gets a dedicated VkDescriptorPool created and destroyed per set (ShaderResources.cpp:2156-2176).

The snapshot pool is capped at 1024 entries and 256 MiB (Recorder.hpp:94-95), which a frame of about 3,000 draws with about 3 read-only buffers each overflows. Every snapshot is also dropped whenever the global guest-allocation generation moves (Recorder.cpp:2225). That generation is bumped by any DirectMemory map or unmap and, on Windows, by a heap-cache release (GuestAllocations.cpp:110-129; GuestHeap.cpp:171-172).

**Evidence.** ShaderResources.cpp:2809-2815: `auto buffer = recorder.ReusableDrawSnapshot(begin, bytes); if (buffer == nullptr) { buffer = std::make_shared<Buffer>(...); std::memcpy(buffer->Bytes().data(), reinterpret_cast<const void*>(begin), bytes); recorder.KeepDrawSnapshot(...); }`.
2826: `result->allocation = result->cache->Allocate(_layout, sizes);`.
2840: `update(context.device, 0, nullptr, static_cast<std::uint32_t>(copies.size()), copies.data());`.
Recorder.hpp:94-95: `DrawSnapshotBudget = 256 MiB; DrawSnapshotEntries = 1024;`.
Recorder.cpp:2225: `if (found->second.registryGeneration != GuestAllocations::GuestAllocationsGeneration_nid_postfix() || ...) { eraseDrawSnapshot(found); return {}; }`.

**Why this impact.** Each draw pays N memcpys (as large as each read-only view; vertex pools read through buffer loads can be megabytes), one or two set allocations and a full descriptor copy. The copies repeat across frames whenever the 1024-entry pool thrashes or any allocation mapping changes. For about 3,000 draws this is likely several to tens of milliseconds per frame on the locked thread, plus extra GPU reads from system memory.

**Proposal.** (a) When the template is freshly built (resolved.built != nullptr), have the build bind the snapshot buffers directly for read-only direct elements. PrepareDrawBindings then selects nothing and skips the second set and copy.
(b) Replace per-snapshot Buffers with a per-batch linear host-visible arena: one VkBuffer with a bump allocator, released at batch completion. Bind the snapshots with VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC offsets, or with VK_KHR_push_descriptor. This removes the per-draw set allocation and the copy of every binding.
(c) Raise DrawSnapshotEntries (for example 16384) and the budget.
(d) Make registry invalidation range-scoped. The invalidator already receives the changed ranges (GuestAllocationsSetInvalidator), so drop only the snapshots that overlap them instead of comparing the global generation.

**Risk.** The snapshots exist because PM4 WRITE_DATA or DMA (and guest CPU stores) run before the deferred batch executes. Any in-place path must keep that guarantee. The arena must outlive the batch (recorder.Keep), and dynamic offsets must respect minStorageBufferOffsetAlignment.

**How to measure.** - APS5_PROFILE_DRAW=1: [draws] 'record' phase, where PrepareDrawBindings runs, and 'resources' (bindings/upload/descriptors/other).
- [descriptors] 'sets N from M pools and K dedicated pools'.
- APS5_CAPTURE_TRACE=1: count 'draw-snapshot' lines per batch.

#### graphics-draw-5-pass-churn: Render-pass churn: strict continuation rules plus full ALL_COMMANDS barriers at every pass boundary drain the GPU between passes

- **Impact (claimed):** high · **Effort:** M · **Category:** gpu-side-efficiency · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1337-1365,1388-1409,1437; core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:1347-1398; core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:144-188`

**Mechanism.** A draw continues the open pass only if all of the following hold:
- passKey is equal: same attachment views and extent
- the previous draw left the pass continuable, meaning `!resources.WritesMemory()`. WritesMemory is true for any storage-image or buffer write, a copied write, BDA completion work or a lease (ShaderResources.cpp:2918-2920).
- the draw does not sample its own attachment
- the draw is not a GPU-indirect or mesh-indirect draw
- nothing was recorded in between. recorder->Commands() ends the pass (Recorder.cpp:1352); it is called by dispatches, by uploads during the resource build or Refresh, by FillClear, and by label or key-store flushes (Draw.cpp:1359-1361). Address-based (BDA) draws flush unconditionally because HoldsLease() is true. The 64-work submit (APS5_BATCH_CAP, DeferredLabels.cpp:83-89) also ends it.

Each new pass records:
- endOpenRenderPass: vkCmdEndRenderPass plus an ALL_COMMANDS to ALL_COMMANDS barrier (Recorder.cpp:1386-1389)
- the draw's own ALL_COMMANDS to color/depth/vertex/shader barrier (Draw.cpp:1395-1397), even though the end-of-pass barrier just covered those accesses (coveredAccess is computed in Commands() but recordDraw ignores it)
- vkCmdBeginRenderPass with LOAD/STORE on every attachment in GENERAL layout (Pipeline.cpp:149-170)

**Evidence.** Draw.cpp:1365: `const bool continued = !capture && !readsTarget && !gpuIndirect && !meshIndirect && recorder->ContinuesRenderPass(passKey);`.
Draw.cpp:1437: `recorder->LeaveRenderPassOpen(passKey, drawTiming, !resources.WritesMemory(), ...)`.
Recorder.cpp:1388: `recordBarrier(open->commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, ...)`.
Draw.cpp:1396: `cmdPipelineBarrier(commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, ..., 0, 1, &before, ...)`.
APS5_DRAW_TRANSITIONS is unset by default (lean path).

**Why this impact.** Every pass boundary is a full pipeline drain, and two drains in a row. Draws that write storage images or buffers (common in PS5 renderers), indirect draws and BDA draws each get their own pass, so the GPU runs them one at a time with idle bubbles. This is one reason the GPU looks underused even when the CPU feeds it.

**Proposal.** 1) Add a subpass self-dependency (src = dst = subpass 0; FRAGMENT_SHADER|COLOR_ATTACHMENT_OUTPUT|LATE_FRAGMENT_TESTS to the same stages plus VERTEX_SHADER; BY_REGION) to the render pass at Pipeline.cpp:176-181. When the previous draw only wrote memory that this draw does not read (check its WritesOverlap against this draw's InPlaceReads and storage images), record an in-pass vkCmdPipelineBarrier and continue the pass instead of ending it.
2) Have recordDraw call recorder->Commands(&covered) and skip the pre-pass barrier when `covered` already includes those accesses. Narrow srcStage from ALL_COMMANDS to the classes recorded since the last barrier.
3) For GPU-indirect draws, record the argument barrier and constant-rule rewrites before the first pass of a run of indirect draws to the same targets, so they can continue one pass.
4) Test APS5_BATCH_CAP values.

**Risk.** Narrower barriers can introduce read-after-write races. Validate with APS5_BARRIER_VALIDATE=1 and image comparisons. An in-pass barrier needs the self-dependency declared in every pipeline's render pass, which also changes render-pass compatibility.

**How to measure.** - APS5_PROFILE_DRAW=1: [draws] 'render passes N begun, M draws continued one'; [barriers] by class ('draw', 'label-run').
- APS5_PROFILE_GPU=1: [gputime], comparing 'batch ms first to last command' with class time ('untimed ... outside every range') to see GPU idle time.

#### graphics-draw-6-input-sync: Index and vertex inputs are always CPU-copied; if a GPU pass produced them, each draw first waits for the GPU under the lock

- **Impact (claimed):** high · **Effort:** M · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:737-753,847-904,1024-1027; core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:993-1018`

**Mechanism.** CopyDrawInput first calls GuestMemory::FlushGpuWrites, which goes to Recorder's FlushForAccess. When the range overlaps a pending GPU write (index or vertex data written earlier by a dispatch in the open or in-flight batch: compute skinning, GPU culling or compaction, particles), FlushForAccess takes the GpuMutex and calls recorder->SyncThrough(address, bytes, true). Inside a draw the mutex is already held, so this is a nested, locked submit plus fence wait.

The bytes are then memcpy'd into a new HOST_VISIBLE Buffer, a GPU to CPU to GPU round trip. For index buffers, the CPU then scans every index to find the maximum (Draw.cpp:854-864), which is only used to size the vertex copies and to check maxDrawIndexedIndexValue. Reuse happens only if Recorder draw snapshots prove the range unchanged, which never applies to GPU-written data.

On the mesh (GS) path, recordDrawCommands never binds the copied index buffer (Draw.cpp:1024-1027): the mesh shader reads indices through MeshIndexBufferDescriptor. The copy and scan there are wasted work, and can still sync.

**Evidence.** Draw.cpp:741: `GuestMemory::FlushGpuWrites(address, bytes);`.
Draw.cpp:750-751: `copy.buffer = std::make_shared<Buffer>(...); GuestMemory::Read(address, copy.buffer->Bytes(), alignment);`.
Recorder.cpp:1013-1018: `std::lock_guard gpu(GuestMemory::GpuMutex()); if (... recorder->PendingWriteOverlaps(address, bytes)) { ... Recorder::CountSync(1); recorder->SyncThrough(address, bytes, true); }`.
Draw.cpp:854: `for (std::size_t offset = 0; offset < indexBytes; offset += draw.indexSize)`.

**Why this impact.** Each draw whose inputs a GPU pass produced costs a full GPU drain under the global lock (submit latency plus all queued GPU work, often 0.2-2 ms) and leaves the GPU idle while the CPU finishes recording. With tens of such draws per frame this alone is a large part of the frame. When it does not apply, the remaining cost is the O(indexCount) scan on snapshot misses.

**Proposal.** In CopyDrawInput (and the vertex loop at Draw.cpp:894-900): when `recorder->PendingWriteOverlaps(address, bytes)` and `HostImportFor(context, address, bytes)` covers the range, do not flush or copy. Bind import->buffer at offset (address - import->base) directly; index offsets must be a multiple of indexSize, and the imports need VERTEX/INDEX usage bits. Call recorder->NotePendingRead(address, bytes, ...) and keep the import. The pass-begin barrier already covers this case: it lists VK_ACCESS_INDEX_READ_BIT and VERTEX_ATTRIBUTE_READ_BIT from ALL_COMMANDS (Draw.cpp:1395).

Skip the max-index scan when the stage has no vertex attributes, or when limits.maxDrawIndexedIndexValue == UINT32_MAX; vertex copies can then use VertexBufferExtent or the in-place import. Skip the index copy entirely on the mesh path, except for the triangle-fan restart check.

**Risk.** Binding in place requires that the CPU does not write the range before the batch runs; NotePendingRead makes driver-side CPU stores wait. Guest CPU writes to GPU-produced buffers are unusual, but the snapshot rationale applies. Import buffers may lack index/vertex usage flags and need them added at import creation.

**How to measure.** APS5_PROFILE_DRAW=1, then read:
- [draws] 'hook waits inside draws X ms (max Y)' and the 'vertex' phase total
- [recorder] 'pending write N/T s' and 'top sync sites (source@caller ...)'
- [hooksync]

#### graphics-draw-8-sync-fallback: Targets that are not resident (linear, or a failed lookup) make the draw synchronous: CPU copies and retiles the whole target, plus a full GPU drain

- **Impact (claimed):** high · **Effort:** M · **Category:** upload-readback · **Default path:** no (opt-in)
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1555-1608,1799-2001; core/libs/prx/libSceAgcDriver/Graphics/src/ColorTargetTransfer.cpp:8-41; core/libs/prx/libSceAgcDriver/Graphics/src/ColorTargetLayout.cpp:155-169`

**Mechanism.** A target is resident only if it is tiled, the detiler exists and the lookup did not throw. Exceptions in refreshResidentTarget (Attachable(), layout size mismatch, Draw.cpp:1564-1565) are logged once per address (924) and leave the target non-resident for good. The draw then becomes synchronous (recordable is false, Draw.cpp:1614) and per draw it:
- allocates a host Buffer, two DeviceBuffers and a std::vector, each the size of the target (1580-1583)
- reads the whole target with GuestMemory::Read plus a memcpy (1584-1585), or, for non-RenderTarget tilings, runs ReadColorTarget with a CPU per-texel detile (one memcpy per pixel, ColorTargetLayout.cpp:155-160)
- fills DCC clears on the CPU (1594-1597)
- runs the GPU detile, render and retile
- runs batch->SubmitAndWait (1966), which syncs the recorder and drains the GPU
- writes the target back with GuestMemory::WriteChanged or WriteColorTarget (1996-1997; Write re-reads, CPU-tiles and writes the whole target) and runs MarkDccUncompressed
Linear color targets always take this path.

**Evidence.** Draw.cpp:1555: `if ((binding.gpuTiling || (color.tileMode != ColorTileMode::Linear && context.detiler != nullptr)) && residentTargets)`.
Draw.cpp:924: `std::fprintf(stderr, "[gpu] color target 0x%llx stays non-resident: %s\n", ...)`.
Draw.cpp:1966: `batch->SubmitAndWait();`.
ColorTargetLayout.cpp:158: `for (std::uint32_t x = 0; x < width; ++x) std::memcpy(row + ..., source.data() + offset(x, y), elementBytes);`.
APS5_NO_RESIDENT_TARGETS (opt-out) is unset, so residency is on.

**Why this impact.** Only taken for linear targets or targets whose lookup failed, but each such draw moves about 4x the target's bytes through the CPU (a 1080p RGBA8 target is about 8 MB) and drains the GPU. A few per frame cost milliseconds; one recurring post-process or UI target can dominate the frame.

**Proposal.** - Add a resident path for Linear color targets: a storage image whose upload and write-back are plain buffer-image copies through the host import.
- For Standard4KB/64KB targets that failed the resident lookup, use the GPU TextureDetiler instead of the CPU per-texel loop, and reuse staging buffers per target address.
- Count failed resident lookups by reason in [draws] instead of logging once, and retry the lookup after the cause (for example an eviction) clears.

**Risk.** The linear resident path needs the write-back rules of StorageTexture (pending units, DCC) to apply to the linear layout. Retrying lookups must not throw on every draw.

**How to measure.** - APS5_PROFILE_DRAW=1: [draws] 'N synchronous avg X us; ... waited or synchronous because: non-resident target N' and the readTarget/writeBack phases.
- stderr: '[gpu] color target 0x... stays non-resident: ...'.

#### graphics-draw-9-recipe-depth: Draw recipes (the fast path for draw-cache hits) exclude every draw with a depth target

- **Impact (claimed):** medium · **Effort:** S · **Category:** incomplete-implementation · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1732,1770,2056`

**Mechanism.** Draw only builds a recipe when `!state.depth` (1770) and there is no proxied target (1732). It must also be direct, not waited, cacheable, Reusable(), and have no moved buffers. DrawWithRecipe additionally requires `recipe.targetViews.size() == state.colors.size()` (2056), so a depth view cannot be part of a recipe.

Nearly every 3D scene draw has a depth target. Those draws always take the full path: validation memo, DrawResourceKey with Find and Revalidate, pipelineKey with CachedPipeline, and AcquireFramebuffer. They never get the recipe path that skips lookups and key building. Depth surfaces are host-only and stable until device teardown (DepthSurface.cpp:152-165), so a recipe could simply store the view. targetViews already includes the depth view (pushed at Draw.cpp:1610 and copied into the recipe at 1778).

**Evidence.** Draw.cpp:1770: `if (recipeOut != nullptr && DrawRecipes() && cacheable && !outcome.waited && args == nullptr && resources->Reusable() && !state.depth && resolved.moved.empty())`.
Draw.cpp:2056: `Require(recipe.targets.size() == state.colors.size() && recipe.targetViews.size() == state.colors.size(), ...)`.
APS5_NO_DRAW_RECIPE is unset, so recipes are on, but they do not apply here.

**Why this impact.** It removes the cheapest per-draw path for most of the frame. The gain is limited because draw-cache hits also need identical register and user data across frames (Execution side).

**Proposal.** - Allow `recipe.targetViews.size() == state.colors.size() + (state.depth ? 1 : 0)` and drop `!state.depth` at 1770.
- In DrawWithRecipe, pass the stored targetViews, depth included, to AcquireFramebuffer. Validity is already covered by the recipe->device check (VulkanDevice.cpp:2976-2979), because ClearDepthSurfaces runs only per device.
- Allow moved read-only buffers by recomputing MovedReadOnlyBuffers at the hit and passing them to PrepareDrawBindings.

**Risk.** If depth surfaces are ever evicted in future, a stored VkImageView would dangle. A weak handle or a serial is then needed.

**How to measure.** - APS5_PROFILE_DRAW=1: [draws] 'recipe hits N, misses by reason' and 'per kind ... recipe hit (count)'.
- The Execution [recipe] precheck misses 'no recipe'.

#### graphics-draw-10-bookkeeping: Per-draw key building, allocations and lookups under the lock (validation key, vertex layout, resource and pipeline keys, framebuffer list, InPlaceReads copies, loader lookups)

- **Impact (claimed):** medium · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:456-565,583-603,873-904,1353-1361,1456-1464,1021-1037; core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:291-327,364-463,514-547; core/libs/prx/libSceAgcDriver/Graphics/include/VertexInput.hpp:97-120,166-194`

**Mechanism.** Each recorded draw rebuilds or looks up the following, all under the GpuMutex:
- ValidationKey: a std::vector<uint64_t> of about 40 + 12 words per stage, then a std::map<std::vector,...> lookup under validationMutex, then a returned copy of std::set<uint32_t> fragmentOutputs (Draw.cpp:532-565).
- BuildVertexInputLayout: a std::set of locations and one vkGetPhysicalDeviceFormatProperties call per attribute (VertexInput.hpp:100-114).
- PlanVertexCopies: 3 vectors and a stable_sort.
- DrawResourceKey: each stage's ContentKey vector concatenated, FNV-hashed and fully compared in ResourceCache::Find.
- pipelineKey: a std::vector<std::byte> grown by about 100 small inserts with no reserve, a byte-wise FNV (Pipeline.cpp:456-463) and a full key compare, all under the process-wide Pipelines().mutex.
- maskedState: copies the whole State (two vectors) when a write mask changes (1731-1738).
- AcquireFramebuffer: erase_if plus weak_ptr::lock per owner.
- The `touches` lambda: calls ReadsOverlap, which calls InPlaceReads() and builds a new vector per queued label store checked (1353-1361; ShaderResources.cpp:2923-2926; GuestBufferMemory.cpp:2722-2734). PrepareDrawBindings and MarkGpuWrites build it again.
- Context::Function: a loader vkGetDeviceProcAddr name lookup under the loader's global mutex (Context.hpp:40-42,147-154) for every mesh draw (vkCmdDrawMeshTasksEXT, 1021/1026) and every count-indirect draw (1036-1037).
- VulkanDevice::Draw copies the roughly 1.3 KB Context (including a shared_ptr refcount) per draw (VulkanDevice.cpp:2535).

**Evidence.** Draw.cpp:535: `std::vector<std::uint64_t> key;` and 537: `static std::map<std::vector<std::uint64_t>, std::set<std::uint32_t>> memo;`.
VertexInput.hpp:113: `context.formatProperties(context.physical, format.format, &properties);`.
Pipeline.cpp:357-362: `append` uses `key.insert(...)` per field.
Pipeline.cpp:456-463: byte-wise FNV loop.
Draw.cpp:1021: `context.Function<PFN_vkCmdDrawMeshTasksIndirectEXT>("vkCmdDrawMeshTasksIndirectEXT")` (no DeviceFunctions entry).

**Why this impact.** Each item costs about 0.5-5 microseconds, so 10-20 microseconds per draw under the global lock, or about 30-60 ms per 3,000-draw frame. That is much of the gap between a lean recorded draw and the observed per-draw cost.

**Proposal.** - Compute one 64-bit structural hash per compiled stage at compile time, and one per decoded State in the Execution draw-cache decode (DrawDecode is already cached by draw key). Key validation, pipeline and resource caches with fixed-size std::array keys or the precomputed hashes in std::unordered_map, keeping a full compare on hit.
- Return fragment outputs as a uint32 bitmask.
- Memoize vertex-format support per VkFormat in a static table.
- Compute InPlaceReads() once per draw and pass it down.
- Add vkCmdDrawMeshTasksEXT, vkCmdDrawMeshTasksIndirectEXT, vkCmdDrawIndexedIndirectCountKHR and vkCmdDrawIndirectCountKHR to DeviceFunctions.
- reserve() the key vectors.
- Return the cached Context by const reference from graphicsContext().

**Risk.** Hash collisions must still fall back to a full compare. Precomputed state hashes must include every field the pipeline key uses.

**How to measure.** - APS5_PROFILE_DRAW=1: [draws] phase totals validate/vertex/lookup/pipeline/prepare; [vk] line (DeviceProcLookups); [pipecache] lookups.
- A sampling profiler (perf, VTune) on the queue-0 worker thread.

#### graphics-draw-11-indirect: Indirect draws: each GPU-side indirect draw begins its own render pass, rewrites constant-rule records one vkCmdUpdateBuffer at a time, and some cases read records on the CPU after a full sync

- **Impact (claimed):** medium · **Effort:** M · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1074-1122,1330-1365,1632-1676`

**Mechanism.** GPU path (default; APS5_NO_GPU_INDIRECT_DRAW is unset):
- gpuIndirect forces a new pass (1365).
- It records an ALL_COMMANDS|HOST to DRAW_INDIRECT|TRANSFER barrier (1077).
- If either dimension's rule is Constant, it allocates a new DeviceBuffer per draw (1089), records vkCmdCopyBuffer and a barrier, then one vkCmdUpdateBuffer per record per dimension (1094-1098), then another barrier. That is O(count) commands plus a device allocation per indirect draw.
- Mesh-indirect draws allocate a Buffer and record RecordMeshArguments per draw (1109-1116).

The decision step (1634-1648) falls back to the CPU in three cases:
- a pending storage image over the arguments: FlushPending, then recorder->Sync(), a full drain (1635-1639)
- a pending label or copied writer
- arguments that are not imported
The CPU path then reads every record through the flush hook (1665-1667), which costs a SyncThrough per draw when the arguments are still being produced.

**Evidence.** Draw.cpp:1089: `scratch = std::make_unique<DeviceBuffer>(context, bytes, ...)`.
Draw.cpp:1094-1097: `for (std::uint32_t record = 0; record < args->count; ++record) { ... update(commands, scratch->Handle(), base + args->VertexDwordOffset(), 4, &args->vertexConstant); ...}`.
Draw.cpp:1637-1638: `Recorder::CountSync(2); recorder->Sync();`.

**Why this impact.** GPU-driven renderers issue many indirect draws per frame. Each gets its own pass with full barriers and per-draw allocations, and the CPU fallbacks add full GPU drains. Most visible if Astro Bot uses multi-draw indirect.

**Proposal.** - Use a per-batch scratch arena instead of a DeviceBuffer per draw.
- Replace the per-record vkCmdUpdateBuffer loop with a tiny compute dispatch, or batch all pending constant-rule rewrites of the batch into one pre-pass dispatch.
- Record the argument barrier once before a run of indirect draws into the same targets, so they can continue a pass.
- For PendingImage, flush and use SyncThrough on the argument range instead of a full Sync().

**Risk.** Rewrites must stay ordered after the argument producers and before the indirect reads. Arena lifetime must cover the batch.

**How to measure.** APS5_PROFILE_DRAW=1, then read:
- [draws] 'indirect: gpu-side N (M rewritten), cpu-side K (reasons), argument reads X ms'
- 'render passes begun'
- [recorder] top sync sites

#### graphics-draw-12-gpu-placement: GPU-side efficiency: vertex, index and snapshot copies live in host-cached system memory, and render targets are storage images in GENERAL layout

- **Impact (claimed):** low · **Effort:** M · **Category:** gpu-side-efficiency · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Resources.cpp:10-48; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:750; core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2812; core/libs/prx/libSceAgcDriver/Graphics/src/Texture.cpp:719; core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:144-170`

**Mechanism.** Buffer defaults to HOST_VISIBLE|HOST_COHERENT and prefers HOST_CACHED (Resources.cpp:33-41). Index and vertex copies (Draw.cpp:750) and draw snapshots (ShaderResources.cpp:2812) therefore sit in system memory. On a discrete GPU every vertex fetch and constant read crosses PCIe each time the draw executes, every frame, even when a snapshot is reused across frames.

Resident render targets are StorageTextures with TRANSFER|STORAGE|SAMPLED usage (Texture.cpp:719). Lean draws render into them in VK_IMAGE_LAYOUT_GENERAL with LOAD/STORE ops, and on AMD drivers storage usage plus GENERAL disables DCC color compression.

**Evidence.** Resources.cpp:33-38: `constexpr VkMemoryPropertyFlags hostDefault = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; if (properties == hostDefault) { ... hostDefault | VK_MEMORY_PROPERTY_HOST_CACHED_BIT ...}`.
Texture.cpp:719: `imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;`.
Pipeline.cpp:149-157: loadOp LOAD, storeOp STORE, initial/final layout attachmentLayout (GENERAL when lean).

**Why this impact.** This lowers GPU efficiency (memory-bound vertex fetch over PCIe, higher bandwidth), but the user sees the GPU underused, so it is secondary until the CPU-side limits are removed.

**Proposal.** - For buffers kept in the draw snapshot pool (KeepDrawSnapshot), allocate DEVICE_LOCAL|HOST_VISIBLE memory (ReBAR) when the device offers it. Otherwise upload once to a DeviceBuffer and reuse that device copy while the snapshot stays valid.
- Keep HOST_CACHED only for buffers the CPU reads back.

**Risk.** ReBAR memory is limited on some systems, and CPU writes to it must be sequential (write-combined).

**How to measure.** - APS5_PROFILE_GPU=1: [gputime] by class ('draw' ms compared with batch time).
- A GPU profiler (Nsight Graphics or RGP) to check vertex-fetch or memory-bound draws.

#### graphics-draw-13-zero-count: Indexed draws with a zero index or instance count throw instead of being no-ops, after the full prepare and the lock

- **Impact (claimed):** low · **Effort:** S · **Category:** incomplete-implementation · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:807; core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawPrecheck.cpp:15`

**Mechanism.** precheckDraw returns Nothing only for non-indexed zero counts. An indexed draw with indexCount == 0 or instanceCount == 0 is decoded, looks up or compiles its shaders, takes the GpuMutex in lockForDraw, and then throws in prepareDrawInputs ('zero-count indexed draws are unsupported'). It is counted as a DrawSkip::Thrown packet. Hardware treats such a draw as a no-op. Engines that cull on the CPU by zeroing counts would pay an exception (allocation plus unwind) and a lock acquisition per culled draw.

**Evidence.** DrawPrecheck.cpp:15: `if (!drawParameters.indirect && !drawParameters.indexed && (drawParameters.indexCount == 0 || drawParameters.instanceCount == 0)) return DrawVerdict::Nothing;`.
Draw.cpp:807: `if (args == nullptr) Require(draw.indexCount != 0 && draw.instanceCount != 0, "zero-count indexed draws are unsupported");`.

**Why this impact.** Costly only if the title emits many zero-count indexed draws. Cheap to fix.

**Proposal.** - Remove the `!drawParameters.indexed` condition at DrawPrecheck.cpp:15.
- In prepareDrawInputs, set inputs.nothing for indexed zero counts instead of throwing.

**Risk.** None for direct draws. Indirect draws are unaffected because their counts live in records.

**How to measure.** APS5_PROFILE_DRAW=1: [draws] 'skipped packets: ... thrown N in X ms'.

### Incomplete or stubbed code noticed

- `core/libs/prx/libSceAgcDriver/Graphics/include/ShaderResources.hpp:89-97; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:992`: Draws have no two-stage, unlocked ShaderResources build; only compute has the deferred constructor and Complete(). Draw resources are always built entirely under GpuMutex. *Performance relevance:* This is the main serialization point: it keeps the most expensive per-draw phase ('resources') inside the global lock.
- `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1770,2056`: Draw recipes are not built for draws with a depth target or a proxied target (and are rejected for indirect draws, moved buffers or non-reusable templates). DrawWithRecipe requires targetViews.size() == colors.size(). *Performance relevance:* Depth-tested 3D draws, most of a frame, can never use the recipe fast path.
- `core/libs/prx/libSceAgcDriver/Graphics/src/DepthSurface.cpp:22-83,152-165; core/libs/prx/libSceAgcDriver/Graphics/src/State.cpp:195-197`: Depth/stencil targets are host-only VkImages:
- cleared once at creation with DB_DEPTH_CLEAR
- never written back to guest memory, never refreshed from it, never evicted
- HTILE is not modeled
- decodeDepth throws for DB_RENDER_CONTROL depth/stencil clear draws (zero mask 0x1f9f), so per-frame depth clears are dropped *Performance relevance:* Depth is not cleared each frame, so early-Z rejects or keeps fragments wrongly. GPU load is not representative and may look low. Any guest compute that reads depth memory (HiZ, SSAO) sees stale data.
- `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/DrawPrecheck.cpp:31-37`: Depth/stencil-only draws without a pixel shader are rejected with 'depth/stencil-only draws without a pixel shader are not implemented'. *Performance relevance:* Shadow-map and depth-prepass draws are skipped. The measured 8-9 FPS excludes this GPU work, so real cost is higher once it is implemented, and frames render incompletely.
- `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1555-1608`: Linear color targets have no resident path, and a target whose resident lookup throws stays non-resident after one log line. *Performance relevance:* Each such draw is synchronous: O(target) CPU copy, tiling and write-back plus SubmitAndWait.
- `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:137-153; core/libs/prx/libSceAgcDriver/Graphics/src/State.cpp:773-780`: Partial support for color metadata:
- CMASK only handles all-fast-cleared or all-expanded states; a mixed CMASK, or a DCC target whose CMASK is not expanded, throws.
- DCC keys of mipmapped color targets are ignored.
- DCC handles only uniform clear codes. *Performance relevance:* Throws drop draws. The clear materialization does whole-target fills and, when the keys came from a compute kernel, a GPU sync per fast clear.
- `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:793,807`: Draw modifier flags and zero-count indexed draws throw 'unsupported' instead of being decoded or treated as no-ops. *Performance relevance:* Each such packet runs the full prepare, the lock and an exception.
- `core/libs/prx/libSceAgcDriver/Graphics/src/State.cpp:824-881`: DrawRejection or DecodeState reject the following features, so the draw is skipped:
- MSAA
- alpha-to-coverage
- sample masks
- depth export and ordered fragment execution
- clip distance and layer exports
- window offsets
- Z_24 depth
- stencil add/sub by values other than 1
- non-normal CB modes (except metadata passes) *Performance relevance:* Missing GPU work makes current FPS numbers optimistic. The decode and exception cost shows in the [draws] skipped-packets counts.
- `core/libs/prx/libSceAgcDriver/Graphics/include/Draw.hpp:94-107; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:658-662,1695-1711`: A stale comment says copied-write draws are listed only under APS5_RECORD_COPIED_DRAWS=1. The code lists them by default (APS5_NO_RECORD_COPIED_DRAWS is the opt-out), and VulkanDevice consults the list. DrawCopiedWriters is a std::vector scanned linearly for every indirect, fill and draw-indirect check and erased in O(n) at each completion. *Performance relevance:* O(n) to O(n^2) per batch when many draws have copied writes; such draws also never continue a render pass (WritesMemory).
- `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1779-1783; core/libs/prx/libSceAgcDriver/Execution/include/Recipe.hpp:80`: recipe->passKey is computed but never read, and it omits the blends/exportIndex mix that recordDraw uses. *Performance relevance:* Dead per-recipe work; a latent bug if it is ever used to continue passes.
- `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:3`: GpuColorTransfer.hpp is included but not used by the draw path; GpuColorTransfer serves only the presenter. *Performance relevance:* None (cleanup).

### Settings to try without code changes

| Setting | Expected effect | Risk |
|---|---|---|
| `APS5_PROFILE_DRAW=1` | Prints these lines every 10 s: - [draws]: phases, recorded/synchronous counts and reasons, passes begun/continued, hook waits, indirect paths, recipe hits - [rescache], [pipecache] (evicted/private), [dcc] (MiB scanned, ms), [descriptors], [texture] - [lock] (GpuMutex hold and wait by site) and [recorder] (syncs by source and top sync sites) Together these rank the findings above with real numbers. | Adds per-draw timers and per-batch timestamp queries (BatchStampsEnabled), so FPS drops slightly. Use it for diagnosis only. |
| `APS5_RESOURCE_CACHE_ENTRIES=16384` | If the [rescache] draw misses are capacity misses (the default holds 1024 entries for draws and dispatches together), template hits rise and the 'resources' phase shrinks. | Cached templates pin descriptor sets, textures and storage images, so RAM and VRAM use grows. Revalidate time per hit is added. |
| `APS5_BATCH_CAP=256 (also compare with 32)` | 256 means fewer vkQueueSubmit calls and fewer forced render-pass ends at the 64-work submit, so more 'draws continued one'. 32 gives the GPU work earlier, which raises CPU/GPU overlap if [gputime] shows idle gaps. | A larger cap delays GPU start and makes each Sync wait for more work; a smaller cap means more submits and pass breaks. |
| `APS5_PROFILE_GPU=1` | [gputime] shows GPU time per batch and per class against wall time, which tells whether the GPU is truly idle (starved) or busy (GPU-bound). | Adds timestamp queries and a Commands() call per timed range, which slightly perturbs recording. |
| `No env var: grep the stderr log for '[memory] write watch unavailable', '[gpu] color target 0x... stays non-resident' and '[pipeline-cache] loaded'` | The first line means every draw re-copies inputs and round-trips its targets (Linux needs kernel and headers 6.7+ for PAGEMAP_SCAN and async userfaultfd write-protect; a build that lacked the headers prints nothing). The second identifies synchronous draws. The third confirms the persistent pipeline cache is used. | None. |
| `Build with -DCMAKE_BUILD_TYPE=Release` | The root CMakeLists.txt sets no default build type, so a configure without one compiles without optimization. Every per-draw path above becomes several times slower. | None. Release is what docs/dev/BUILD.md shows. |
| `APS5_PROFILE_DRAW=1 APS5_TRACE_DRAWS=1 (only for a few seconds)` | Prints one [draw] line per draw with its phase times, plus inputs for synchronous draws. It identifies which draws have the long readTarget, resources or pipeline phases. | Very heavy stderr output that distorts timing. Use it only for short captures. |
| `APS5_BARRIER_VALIDATE=1` | Checks barrier coverage of recorded work. Use it as a safety net when trying the pass-continuation or barrier-narrowing changes. | Adds CPU overhead per recorded command. |

<a id="recorder"></a>
## Command recording, submission and pipelines (Recorder, Pipeline, PipelineCache)

### How it works

Every guest queue records into one Recorder: one open batch, one in-flight deque, and one VkQueue that present also uses. A worker holding GuestMemory::GpuMutex calls Commands(). ensureOpen reuses a spare command buffer and fence (vkBeginCommandBuffer ONE_TIME) and ends any open render pass.
Dispatch: a leading ALL->COMPUTE barrier unless the previous trailing barrier covered SHADER_R/W, then bind, push and dispatch. An unconditional COMPUTE->ALL|HOST trailing barrier follows, then NotePendingReads/Writes, which re-sorts and republishes the lock-free pending-write snapshot when a range is new.
Draw: CachedPipeline builds a byte-vector key, FNV-hashes it and looks it up in a global LRU of 256. A miss synchronously creates shader modules, a layout, a render pass and the pipeline via vkCreateGraphicsPipelines with a disk-persisted VkPipelineCache. AcquireFramebuffer uses a per-pipeline LRU of 8. The draw either continues the open pass (same views and extent, previous draw wrote no memory, nothing recorded since) or ends it (vkCmdEndRenderPass + ALL->ALL barrier) and begins a new LOAD/STORE pass in GENERAL layout after an ALL->graphics barrier. Then descriptor bind, 128 B push constants and the draw. The pass is left open, and objects are kept until the fence.
Labels are queued vkCmdUpdateBuffer stores, recorded as one run with a barrier pair at Submit. A label table lets WAIT_REG_MEM read recorded values without the GPU.
Submit (vkEndCommandBuffer + vkQueueSubmit signalling a fence and a timeline serial) happens after 64 draws/dispatches (APS5_BATCH_CAP), 1 ms after the first label, at CPU access to pending ranges, drains, flips and submission ends.
Batches retire via Reap (fence poll on queue 0), Sync/SyncThrough (fence waits) or WaitSerial+FinishUpTo (unlocked timeline wait). Completions (CPU write-backs) run under the mutex; kept objects are freed on a release thread. The CPU runs ahead of the GPU except at those waits.

### Threads and locks

Each guest queue has a worker thread. Decode, recompile lookup and capture run unlocked. Resource build, CachedPipeline (any vkCreate*Pipelines also holds PipelineStore::mutex), command recording, Submit/vkQueueSubmit, Reap/finish completions and snapshot rebuilds all run under the one recursive GuestMemory::GpuMutex (depth 1 in the draw/dispatch hold).
Other users of the mutex: label record/flush between packets (try_lock, but blocking on queue 0 for cap/deadline), drains, the presenter (PresentDisplayBuffer and QueuePresent on the same VkQueue), and game/HLE threads through the flush hook when the lock-free snapshot overlaps.
Unlocked GPU waits exist only for WaitSerial (drains, dispatch presync) and the hook's syncThroughUnlocked, at depth 1 with a timeline. These wait for fences WHILE HOLDING the mutex, stalling every other queue worker, hooked game thread and the presenter's locked steps ([lock] 'locked GPU waits'):
- nested hooks (depth >= 2 inside a draw/dispatch hold)
- Sync() and locked SyncThrough
- FinishUpTo without a prior wait (lease waiter)
- BoundKeptBytes
- SamplesTotal (WAIT_BIT)
- CommandBatch::Wait
Kept objects are destroyed on a release thread after the outermost unlock by default (APS5_RELEASE_UNDER_LOCK=1 or APS5_RELEASE_ON_UNLOCK=1 change this). labelTableMutex and completedMutex are small leaf locks. Net: GPU command recording is effectively single-threaded.

### Findings

#### recorder-barriers-1: Full-pipeline barriers between almost every pair of commands serialize GPU execution

- **Impact (claimed):** high · **Effort:** L · **Category:** gpu-side-efficiency · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp:3287-3311; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1368,1395-1396; core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:1384-1395,1765-1768,1780-1821`

**Mechanism.** Barriers are emitted per command, not per hazard:
- Every dispatch ends with a COMPUTE->ALL_COMMANDS|HOST barrier (SHADER_WRITE -> SHADER_R/W|TRANSFER_R/W|HOST_READ).
- Its leading ALL_COMMANDS|HOST->COMPUTE barrier is skipped only when the previous command's trailing barrier covered SHADER_R/W.
- Every draw that cannot continue the open render pass records a leading ALL_COMMANDS->(COLOR_ATTACHMENT_OUTPUT|EARLY/LATE_FRAGMENT|VERTEX_INPUT|shader stages) barrier.
- Every pass end records vkCmdEndRenderPass plus an ALL_COMMANDS->ALL_COMMANDS barrier.
Draws never use the covered mask. So dispatch->dispatch costs 1 barrier, draw->dispatch 1, and dispatch->draw or draw->new-pass draw 2, whether or not the two commands touch the same memory.
On AMD and NVIDIA drivers, a barrier with an ALL_COMMANDS/COMPUTE source stage and SHADER/ATTACHMENT write access is a wait-for-idle plus cache flush/invalidate. Independent dispatches and draws therefore never overlap, and the GPU drains and refills between them.

**Evidence.** VulkanDevice.cpp:3307-3311:
  constexpr VkAccessFlags dispatchedAccess = ...|VK_ACCESS_HOST_READ_BIT;
  RecordMemoryBarrier(context, commands, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_SHADER_WRITE_BIT, dispatchedAccess); ... recorder.MarkCovered(dispatchedAccess);
  No condition guards the trailing barrier.
- Leading barrier elision only at 3287: if ((covered & shaderAccess) == shaderAccess && barrierElision).
- Draw.cpp:1368: recorder->Commands() is called with no coveredAccess pointer, so a draw never elides. Draw.cpp:1395-1396 records vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, ...COLOR_ATTACHMENT_OUTPUT|EARLY/LATE_FRAGMENT|VERTEX_INPUT|inputs.shaderStages...) for each non-continued draw.
- Recorder.cpp:1386-1389 (endOpenRenderPass): cmdEndRenderPass, then recordBarrier(ALL_COMMANDS, ALL_COMMANDS, SHADER_WRITE|COLOR_ATTACHMENT_WRITE|DEPTH_STENCIL_ATTACHMENT_WRITE|TRANSFER_WRITE, ...).

Defaults:
- Recorder::MergeBarriers() (1765-1768) is ON unless APS5_FULL_BARRIERS or APS5_NO_BARRIER_ELISION is set.
- The hazard tracker NoteAccess (1780-1821) runs only with APS5_BARRIER_VALIDATE (opt-in) and only counts. Recorder.hpp:439-446 says 'the real barriers are recorded as before'.

**Why this impact.** A frame has hundreds of dispatches and thousands of draws, so this means roughly one GPU drain per dispatch plus two per render-pass break. Small dispatches and short passes become dominated by drain and fill bubbles, which matches the observed under-utilized GPU. Async compute overlap is impossible. The gain is largest once the CPU side stops being the only limiter. The [present] line's GPU busy/idle split shows whether the GPU is currently the bound.

**Proposal.** Make barriers hazard-driven, reusing the existing tracker:
1. Replace the per-command trailing barrier with a pending source scope on the open batch (Batch::coveredAccess already exists): accumulate the src stage/access of commands recorded since the last barrier.
2. Before each command, run the NoteAccess checks (RAW/WAW/WAR on guest ranges via imports, image written, or conservative for address-based, BDA and lease builds). Record one merged barrier (accumulated src -> this command's dst stages/access) only on a hazard, then clear the tracker as NoteAccess already does.
3. Keep host visibility as one ALL_COMMANDS->HOST barrier at Submit (the hostReadOwed path at Recorder.cpp:2778-2783).
4. Let draws pass &covered to Commands() at Draw.cpp:1368, and widen endOpenRenderPass's dst access to attachment, index, vertex and uniform reads, so the leading draw barrier can be elided after a pass end or a dispatch.
Roll out behind a switch (e.g. APS5_HAZARD_BARRIERS=1), first for dispatch->dispatch only, then draws.

**Risk.** High correctness risk if a hazard is missed:
- one guest range aliased through different VkBuffers or imports (compare guest addresses, not handles)
- storage images in GENERAL layout
- BDA or address-based builds, which must stay conservative
- render-pass attachment feedback
Counting mode can validate first: APS5_BARRIER_VALIDATE=1 already reports what would be skipped.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [barriers] 'N recorded (10 s) by class' (dispatch-lead, dispatch-trail, draw) and the 'merged' count.
- Adding APS5_BARRIER_VALIDATE=1 appends 'validate: would emit X ... would skip Y' (the upper bound of the saving).
- [present] 'GPU busy / idle gaps' per present.
- APS5_PROFILE_GPU=1 gives [gputime] per-class times. Note its BOTTOM_OF_PIPE timestamps add serialization of their own.

#### recorder-pipeline-compile-1: Graphics and compute pipeline creation runs synchronously under GpuMutex (and the pipeline store mutex) on every cache miss

- **Impact (claimed):** high · **Effort:** M · **Category:** shader-pipeline-compile · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:514-562,94-254; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1740; core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:260-276,370,383; core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp:3504-3536`

**Mechanism.** CachedPipeline takes the global PipelineStore::mutex. On a miss it constructs a Pipeline, which calls:
- vkCreateShaderModule per stage
- vkCreatePipelineLayout
- vkCreateRenderPass
- vkCreateGraphicsPipelines(context.pipelineCache)
This runs inside Graphics::Draw, which Driver::draw reaches only after lockForDraw() took GuestMemory::GpuMutex. Dispatch pipelines are likewise created in VulkanDevice::dispatch under the hold.
The whole driver compile (milliseconds to hundreds of ms on a driver-cache miss, typically sub-ms on a VkPipelineCache hit) blocks every other queue worker, label flush, presenter step and flush-hooked game thread. Recording effectively stops for all queues.

**Evidence.** - Pipeline.cpp:517-518: auto& store = Pipelines(); std::lock_guard lock(store.mutex);
- Pipeline.cpp:549: auto pipeline = std::make_shared<Pipeline>(...)
- Pipeline constructor: vkCreateShaderModule (126), vkCreatePipelineLayout (142), vkCreateRenderPass (181), vkCreateGraphicsPipelines (248).
- Execution/src/Driver/Draw/Draw.cpp:264 gpuLock.lock() inside lockForDraw; 370 lockForDraw(); 383 localDevice->Draw(...) -> VulkanDevice::Draw -> Graphics::Draw -> Draw.cpp:1740 CachedPipeline(...).
- Compute: VulkanDevice.cpp:3517 vkCreateShaderModule, 3526 vkCreatePipelineLayout, 3530 vkCreateComputePipelines, all inside dispatch() (the 'held here, by every Dispatch caller' comment near 3169).

Defaults: APS5_NO_PIPELINE_CACHE (Pipeline.cpp:515) is opt-in, so the store is on. The VkPipelineCache is persisted to <exe dir>/shader_cache/vk-pipelines-<vendor>-<device>-<driver>-<uuid>.bin (PipelineCache.cpp:33-61; ShaderDiskCache.cpp:34-58, also on _WIN32). It is saved every 10 s by a background thread (PipelineCache.cpp:125-132) and disabled only by ANYPS5_NO_SHADER_CACHE. So only first-time (driver-cache-miss) compiles are long, but every miss still runs under the lock.

**Why this impact.** Each miss freezes the whole emulated GPU front end, not just the drawing queue. During first play, or after LRU evictions (recorder-pipeline-lru-1), this shows up as stutter and as threads blocked in [lock]. Even cache hits cost object creation (modules, layout, render pass, pipeline) under the lock.

**Proposal.** S: share sub-objects across pipelines.
- Cache the VkRenderPass per (attachment formats, export map, attachmentLayout).
- Cache the VkPipelineLayout per (set layout, push stages).
- Cache the VkShaderModule per variantId, or skip modules via VK_KHR_maintenance5 (VkShaderModuleCreateInfo in the stage pNext).
A miss then only pays vkCreateGraphicsPipelines.

M: create or look up the pipeline before lockForDraw in Execution/src/Driver/Draw/Draw.cpp. The decoded State, stage variants and vertex input are known there, and the descriptor layout key comes from the binding plan. Hold only the store mutex, use a per-key in-progress future so concurrent misses don't double-compile, and pass the shared_ptr into Draw. Fall back to the in-lock path when the masked state or proxied formats differ.

L: async compilation on a thread pool. Either skip the draw until the pipeline is ready (behind a switch), or use VK_EXT_graphics_pipeline_library with a fast-linked pipeline while the optimized one compiles.

**Risk.** The pre-lock key must equal what Draw computes (masked blend state, proxied formats, fragment outputs), so keep the in-lock path as a fallback. Async skipping causes visible pop-in.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [pipecache] 'N lookups: hits, misses, private, evicted, cached'.
- [draws] 'pipeline=X ms' phase total and 'longest phase of any draw: pipeline Y'.
- [lock] 'held ... by site (holds, held, max)' for Draw and Dispatch.
On first versus second run, look for the '[pipeline-cache] loaded N KiB' message at startup.

#### recorder-pipeline-lru-1: Pipeline store capped at 256 entries: evicted pipelines are destroyed and recreated, and their draw recipes fall back to the slow path

- **Impact (claimed):** high · **Effort:** S · **Category:** shader-pipeline-compile · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:549-561; core/libs/prx/libSceAgcDriver/Execution/include/Recipe.hpp:76-77; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:2109-2112`

**Mechanism.** After each insertion over 256 entries, the least recently used entry that no in-flight batch references is erased. Its destructor destroys the VkPipeline, VkRenderPass, VkPipelineLayout and VkShaderModules synchronously under GpuMutex.
When the configuration comes back, the full creation repeats: object creation plus a driver VkPipelineCache lookup or compile.
Draw recipes keep only a weak_ptr to the pipeline, so an eviction also turns every recipe hit for that configuration into a miss (ObjectsGone). Those draws fall back to the full Graphics::Draw path: resource resolution, key building and CachedPipeline.
A frame with more than 256 distinct pipeline configurations (shader pairs × blend/depth/stencil/raster permutations; see recorder-dynamic-state-1) thrashes every frame.

**Evidence.** - Pipeline.cpp:552-560: constexpr std::size_t bound = 256; while (store.entries.size() > bound) { const auto victim = std::find_if(..., use_count() == 1); ... store.entries.erase(victim); ++store.evicted; }
- Pipeline::~Pipeline -> release() (260-272): vkDestroyPipeline, vkDestroyRenderPass, vkDestroyPipelineLayout, vkDestroyShaderModule.
- Recipe.hpp:76: std::weak_ptr<Graphics::Pipeline> pipeline;
- Draw.cpp:2109-2110: auto pipeline = recipe.pipeline.lock(); if (pipeline == nullptr) return miss(DrawRecipeMiss::ObjectsGone);

Default: bound is a compile-time constant with no env override.

**Why this impact.** If the working set exceeds 256, each frame pays N pipeline creations plus N slow-path draws under the global lock. That is plausible for an AAA title, but it needs confirming with the [pipecache] evicted count.

**Proposal.** - Raise the bound (e.g. 4096) or make it configurable (APS5_PIPELINE_ENTRIES, defaulting high). Pipelines are small; memory is dominated by driver code objects of tens of KB each.
- Optionally bound by bytes or age (evict only entries unused for K presents, using Recorder::Presents()) instead of count.
- Reduce key diversity (recorder-dynamic-state-1) so fewer entries are needed.

**Risk.** Higher memory use. No correctness risk: eviction semantics are unchanged.

**How to measure.** With APS5_PROFILE_DRAW=1, in steady gameplay:
- [pipecache] 'evicted' and 'misses' per 10 s should be about 0.
- [draws] 'recipe hits N, misses by reason: ... objects gone M'.

#### recorder-locked-waits-1: Several default-path GPU waits block while holding GpuMutex, stalling every other queue and hooked thread

- **Impact (claimed):** high · **Effort:** M · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:2902-2915,2955-2961,3044-3048,3216-3259,1057,1895-1907; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:122-124,1635-1638; core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:735-738,999-1007; core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp:1431,1711,1715,3171-3176`

**Mechanism.** Only two waits release the mutex: WaitSerial (drains, dispatch presync) and the flush hook's syncThroughUnlocked. The latter refuses unless the mutex depth is exactly 1 and a timeline exists.
Everything else ends in finish(wait=true), i.e. vkWaitForFences while the caller holds GpuMutex. That covers Sync(), SyncThrough without waitUnlocked, FinishUpTo without a prior WaitSerial, nested hooks, and hooks inside completions. When the target is the open batch, Sync() submits and waits for every in-flight batch: a full GPU drain.
The common nested case: a draw or dispatch already holds the mutex at depth 1, and its resource build or CPU-read indirect arguments read guest memory. The flush hook then runs at depth 2 and waits locked.
Meanwhile all other queue workers, label flushes, the presenter's locked steps and game threads hitting the hook queue behind a thread that is only waiting for the GPU. CPU threads and the GPU are then idle at the same time.

**Evidence.** Recorder.cpp:
- 3048: if (HookLockedWait() || timeline == VK_NULL_HANDLE || GuestMemory::GpuMutex().DepthOnThisThread() != 1) return false;
- 2955-2961: if (!SyncThroughEnabled() || (open != nullptr && overlaps(*open, address, end))) { ... Sync(); return; }
- 3253-3254: waitFences(context.device, 1, &batch->fence, VK_TRUE, ...) inside finish, which runs under the hold (3241 NoteLockedGpuWait).
- 1057: if (HookFullSync() || completionDepth != 0) recorder->Sync();
- 1895-1907: SamplesTotal calls vkGetQueryPoolResults(..., VK_QUERY_RESULT_WAIT_BIT) on segments that may still be in flight. It is called under GpuMutex from PacketMemory.cpp:219-223 (dumpSampleCounters fallback) and is not counted as a sync.

Default-path callers:
- Draw.cpp:1637-1638: CountSync(2); recorder->Sync() for indirect args with pending storage writes.
- Draw.cpp:124: SyncThrough for DCC keys.
- GuestBufferMemory.cpp:738: Sync() on mirror refresh.
- GuestBufferMemory.cpp:1001-1002: Submit(); FinishUpTo(target) with no prior unlocked wait (lease waiter).
- VulkanDevice.cpp:1431 (fill), 1711/1715 (copy) and 3175 (indirect-dispatch args): locked waits.

Dispatches already have an unlocked presync (VulkanDevice.cpp:3005-3034); draws have none.
Defaults: APS5_HOOK_LOCKED_WAIT is opt-in, so unlocked hook waits are ON at depth 1. APS5_NO_TIMELINE is opt-in, so timeline semaphores are used when available.

**Why this impact.** A locked wait of N ms costs N ms of every other queue's recording and of any game thread reading guest memory through the hook. This convoy is the most direct explanation for 'threads and GPU both under-used' within this subsystem.

**Proposal.** 1. Add a draw presync that mirrors VulkanDevice::presync. Before lockForDraw in Execution/src/Driver/Draw/Draw.cpp, test the ranges the draw will read (ShaderMemory regions, the index range, indirect args and count) against Recorder::SnapshotWriteOverlaps. On a hit, take the lock briefly, SubmitAndEpoch for the newest overlapping batch, release, and WaitRecorded(serial) unlocked. Nested hook waits inside the draw then find their target already signaled.
2. Replace Sync() with a targeted SyncThrough(range) where only one range matters: Draw.cpp:1638 (args->arguments, RangeBytes), VulkanDevice.cpp:3175 (arguments, 12), GuestBufferMemory.cpp:738 (address, bytes).
3. For the lease waiter (GuestBufferMemory.cpp:999-1007), SubmitAndEpoch under the lock, release, WaitSerial, then relock and FinishUpTo, as the drain does.
4. In SamplesTotal, only read segments whose batch finished (store the serial per segment, Reap first). Otherwise submit and WaitSerial unlocked before taking the lock.

**Risk.** The presync result can be stale by the time the lock is taken, but the in-lock check remains as a correct fallback. Releasing the lock mid-operation must not happen inside a hold (keep depth-1 only).

**How to measure.** With APS5_PROFILE_DRAW=1:
- [lock] 'locked GPU waits N / X ms' per queue, plus 'waited behind (holder/site ms)'.
- [recorder] 'fence waits by thread', 'top sync sites (source@caller ...)', 'hook waits unlocked ... locked N'.
- [draws] 'hook waits inside draws X ms'.
- [presync] line for the dispatch equivalent.

#### recorder-dynamic-state-1: Only viewport/scissor/depth bounds/depth bias are dynamic; stencil reference/masks, blend constants and depth/raster state multiply pipelines

- **Impact (claimed):** medium · **Effort:** S · **Category:** shader-pipeline-compile · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:199-204,338-345,396-424; core/libs/prx/libSceAgcDriver/Graphics/src/State.cpp:146-191,236-241,668`

**Mechanism.** pipelineKey includes:
- each VkStencilOpState, which carries reference, compareMask and writeMask decoded per draw from DB_STENCILREFMASK
- the four blend constants, read from CB_BLEND_RGBA for any slot with blending enabled, even when no CONSTANT factor is used
- depthTest/Write/Compare, cullMode, frontFace, topology and primitiveRestart
Each distinct value (e.g. a per-material stencil reference) creates a new pipeline: a new driver compile or cache lookup and a new store entry, which feeds the 256-entry LRU thrash.
Vulkan 1.0 core already offers VK_DYNAMIC_STATE_BLEND_CONSTANTS, STENCIL_COMPARE_MASK, STENCIL_WRITE_MASK and STENCIL_REFERENCE, with no extension needed.

**Evidence.** - Pipeline.cpp:199-201: std::vector<VkDynamicState> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}; if (depthBounds) ...DEPTH_BOUNDS; if (depthBias) ...DEPTH_BIAS;
- Key: Pipeline.cpp:407 for (const auto value : state.blendConstants) append(key, value); and 422-423 append(key, state.stencilFront); append(key, state.stencilBack);
- State.cpp:149-150 face.compareMask/writeMask from refMask; 191 face.reference = reference; 239-240 stencilFace(..., read(cx, 0x10c) / 0x10d ...).
- State.cpp:668: for (i < 4) result.blendConstants[i] = readFloat(cx, 0x105 + i); inside the blending-enabled branch.
- Pipeline::Continue (338-345) only sets viewport, scissor, depth bias and depth bounds.

**Why this impact.** It multiplies the number of pipelines, and so the synchronous compiles under the lock and the LRU evictions. The size of the effect depends on how much the title varies stencil reference and blend constants per draw.

**Proposal.** S:
- Add VK_DYNAMIC_STATE_BLEND_CONSTANTS, STENCIL_COMPARE_MASK, STENCIL_WRITE_MASK and STENCIL_REFERENCE to every pipeline.
- Hash only compareOp/failOp/passOp/depthFailOp of the stencil faces, and drop blendConstants from pipelineKey.
- Set the values in Pipeline::Begin and Continue (add cmdSetBlendConstants and cmdSetStencilCompareMask/WriteMask/Reference to DeviceFunctions and FillDeviceFunctions).
M: where VK_EXT_extended_dynamic_state/2 is available, make cull mode, front face, depth test/write/compare, stencil ops and primitive restart dynamic too.

**Risk.** Low for the core dynamic states. They must be set on every draw, including continued passes and recipe draws (DrawWithRecipe uses the same Begin/Continue).

**How to measure.** With APS5_PROFILE_DRAW=1, compare [pipecache] 'misses' and 'cached' before and after over the same scene.

#### recorder-snapshot-rebuild-1: Pending-write snapshot fully re-sorted under the mutex for every finished batch and every new written range

- **Impact (claimed):** medium · **Effort:** S · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:2336-2356,3197-3214,2265-2283,3174-3192,2902-2915,2886-2900,2979-2983`

**Mechanism.** publishPendingWrites:
- allocates a new vector
- copies every noted range of the open batch, every in-flight batch and every finishing batch
- runs std::sort (O(N log N)) and merges
- stores the result into the atomic shared_ptr
The Finishing scope destructor in finish() calls it unconditionally for each retired batch. Reap, Sync, SyncThrough and FinishUpTo retire batches one at a time, so k batches cost k full rebuilds. Every draw or dispatch whose written ranges are not already covered triggers another (noteWrite returns true).
With 64 work items per batch, each noting several ranges, and a few batches in flight, N reaches thousands of ranges. That is tens of µs per rebuild, all under GpuMutex.

**Evidence.** - Recorder.cpp:2341-2352: auto merged = std::make_shared<WriteRanges>(); ... insert(open->writes); for (inFlight) insert; for (finishing) insert; std::sort(merged->begin(), merged->end()); ... pendingWrites.store(std::move(merged)).
- Recorder.cpp:3209-3212: try { recorder.publishPendingWrites(); } catch (...) {} in ~Finishing, for every batch.
- Recorder.cpp:3178-3184: Reap loops finish(..., false, 4) per signaled batch.
- Recorder.cpp:2278-2282: noteWrite skips the rebuild only if SnapshotCovers(address, end).
- Recorder.cpp:3206-3208: the code itself notes that a stale snapshot is 'a superset (a stale entry only causes a false hit, never a miss)'.

**Why this impact.** Per frame this is about 50-100 batch retirements plus every uncovered note, each paying a full sort under the global lock. The [recorder] line reports the exact total.

**Proposal.** S: in the retire loops (Reap, Sync, SyncThrough, FinishUpTo, syncThroughUnlocked, BoundKeptBytes), suppress the per-batch publish in ~Finishing (e.g. a deferPublish flag on Recorder) and publish once after the loop. This is safe: in between, the snapshot is a superset.
M: publish incrementally.
- Add: copy the current merged snapshot, binary-search the insertion point and merge neighbours, O(N_merged) with no sort.
- Remove: keep each batch's ranges pre-sorted and pre-merged at Submit, and rebuild by k-way merging the remaining batches' vectors.

**Risk.** Low: superset semantics only cause extra hook syncs. Make sure the deferred publish also runs on exception paths (keep the try/catch).

**How to measure.** With APS5_PROFILE_DRAW=1, the [recorder] line field 'snapshot N rebuilds X ms, M notes covered'.

#### recorder-linear-scan-1: PendingWriteOverlaps and related queries scan every pending range linearly, several times per draw

- **Impact (claimed):** medium · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:2358-2391,2393-2425; core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2767,2801; core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:735,2355; core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp:1418,1640,1648`

**Mechanism.** These queries iterate batch.writes of the open batch and of every in-flight batch: O(total pending ranges) per call.
- PendingWriteOverlaps
- PendingWriteSettled (which also makes a vkGetFenceStatus call per overlapping batch)
- NewestWriteNote
- PendingWritesOver
PrepareDrawBindings calls PendingWriteOverlaps once per bound buffer item of every recorded draw, and resource builds, fills and copies call it again. Yet a sorted, merged union of exactly these ranges is maintained for the flush hook and searchable in O(log n).

**Evidence.** - Recorder.cpp:2358-2363: for (const auto& [begin, finish] : batch.writes) if (address < finish && begin < end) return true;
- Recorder.cpp:2365-2372: open first, then for (const auto& batch : inFlight).
- ShaderResources.cpp:2801: if (!direct || recorder.PendingWriteOverlaps(item.address, item.size)) continue; (per item, in PrepareDrawBindings).
- Snapshot search: Recorder.cpp:635-640 (std::partition_point over the merged ranges).
- 20 call sites outside Recorder.cpp (grep).

**Why this impact.** The cost scales with draws × bindings × pending ranges, and all of it runs inside the draw hold under GpuMutex.

**Proposal.** Fast-path each query (when activeRecorder == this) with: auto snapshot = pendingWrites.load(); if (!SnapshotOverlaps(snapshot.get(), address, bytes)) return false;
Run the per-batch scan only on a hit, since the hit still needs the target serial. The snapshot is a superset (open + inFlight + finishing), so a miss is exact.
Optionally, also keep per-batch [min,max] bounds so the scan can skip whole batches.

**Risk.** None if the snapshot superset invariant holds; it does, since it is rebuilt under the same mutex at every note and finish.

**How to measure.** There is no dedicated counter.
- Linux: perf record on the queue-0 worker, looking at the overlaps/PendingWriteOverlaps symbols.
- Windows: APS5_SAMPLE_WORKER=<file> (the built-in sampler).
- With APS5_PROFILE_DRAW=1: the [draws] 'resources' and 'record' phase totals.

#### recorder-renderpass-1: Render-pass continuation is fragile; every break costs end/begin plus two full barriers, with attachments in GENERAL layout

- **Impact (claimed):** medium · **Effort:** M · **Category:** gpu-side-efficiency · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1337-1368,1388-1409,1437,1740; core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:1347-1357,1363-1396; core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:144-175`

**Mechanism.** A draw continues the open pass only if all of these hold:
- same attachment views and extent
- the previous draw wrote no memory (continuable = !WritesMemory())
- it does not sample its own targets
- it is not a GPU-indirect or mesh-indirect draw
- nothing was recorded through Commands() since; every Commands() call ends the pass. That includes resource-build uploads and copies, FlushStores/FlushKeyStores, texture refreshes and dispatches.
Every break costs vkCmdEndRenderPass, an ALL->ALL barrier, the next draw's ALL->graphics barrier and vkCmdBeginRenderPass with LOAD/STORE on every attachment. On AMD, color and depth attachment writes in the source scope imply CB/DB cache flushes per break.
Recorded ('lean') draws render color in VK_IMAGE_LAYOUT_GENERAL, and depth is always GENERAL. Some drivers keep less compression (DCC/HTILE) in GENERAL; this is driver-dependent.

**Evidence.** - Draw.cpp:1365: const bool continued = !capture && !readsTarget && !gpuIndirect && !meshIndirect && recorder->ContinuesRenderPass(passKey);
- Draw.cpp:1437: recorder->LeaveRenderPassOpen(passKey, drawTiming, !resources.WritesMemory(), ...).
- Recorder.cpp:1352: if (open->renderPass.open) endOpenRenderPass(); in every Commands().
- Recorder.cpp:1364: open->renderPass.continuable && key == key.
- Pipeline.cpp:149-150 / 167-170: loadOp LOAD, storeOp STORE; 162: depthReference VK_IMAGE_LAYOUT_GENERAL.
- Draw.cpp:1740: lean ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.

Default: APS5_DRAW_TRANSITIONS is opt-in, so the lean path is on.

**Why this impact.** Each broken pass adds GPU drains and attachment cache flushes. If many draws begin new passes (the [draws] line shows begun versus continued), GPU time per draw grows sharply.

**Proposal.** - Record a draw's resource-build uploads into the batch before the pass is (re)opened, using a per-batch 'prologue' list. Do this only when their destinations are freshly allocated staging or snapshot buffers that no earlier command in the batch reads (no WAR possible). Uploads then stop ending the pass.
- Let a GPU-indirect draw continue when its argument range has no pending write in the open batch (OpenWriteOverlaps false), so the argument barrier can be skipped.
- Medium term: VK_KHR_dynamic_rendering (core in 1.3) to drop VkRenderPass/VkFramebuffer objects. Consider ATTACHMENT_OPTIMAL layouts for targets not bound as storage or sampled concurrently.

**Risk.** The upload ordering must be proven safe (fresh destinations only). Layout changes need correct transitions.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [draws] 'render passes N begun, M draws continued one'.
- [barriers] 'draw' class count.
- APS5_PROFILE_GPU=1: [gputime] 'draw' class time.

#### recorder-submit-1: vkQueueSubmit, reaps and completion write-backs run inside the GpuMutex hold; queue 0 blocks on the lock for capped flushes

- **Impact (claimed):** medium · **Effort:** M · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:2764-2827,3174-3192,3290-3302; core/libs/prx/libSceAgcDriver/Execution/src/Driver/Packets/PacketFlush.cpp:42-87; core/libs/prx/libSceAgcDriver/Execution/src/Driver/Synchronization/DeferredLabels.cpp:59-64,82-88; core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp:1232-1241`

**Mechanism.** A batch is submitted after 64 recorded draws/dispatches (APS5_BATCH_CAP default 64), or 1000 µs after its first label (APS5_LABEL_FLUSH_US default with label batch submit on), or at waits, drains, flips and submission ends. That is roughly (draws+dispatches)/64 submits per frame, plus label-driven ones.
Each submit runs vkEndCommandBuffer and vkQueueSubmit (fence + timeline signal) while holding GpuMutex. On queue 0, a capped or deadline flush takes the lock outright (blocking), and SubmitRecorded first Reaps. That Reap runs every finished batch's completions (CPU write-backs of copied buffers) and snapshot rebuild inside the same hold.
The GPU also cannot start a batch before it is submitted, so with slow recording it idles until the cap or deadline.

**Evidence.** - Recorder.cpp:2771: AssertGpuLockHeld("Recorder::Submit").
- Recorder.cpp:2792: endCommandBuffer.
- Recorder.cpp:2808: queueSubmit(context.queue, 1, &submission, batch->fence).
- PacketFlush.cpp:46: const bool capped = ... batchCap() != 0 && work >= batchCap();
- PacketFlush.cpp:64-68: else if (deferredDue || (queue == 0 && (capped || ...))) outright = true; ... gpuLock.lock();
- PacketFlush.cpp:85: localDevice->SubmitRecorded(queue == 0);
- VulkanDevice.cpp:1239-1240: if (reapFirst && OpportunisticReap()) state->recorder->Reap(); state->recorder->Submit();
- DeferredLabels.cpp:86: : 64ull; DeferredLabels.cpp:62: (LabelBatchSubmit() ? 1000 : 250).
- Recorder.cpp:3295-3301: for (auto& action : batch->completions) action(); inside finish (under the hold).

Defaults: APS5_NO_OPPORTUNISTIC_REAP, APS5_NO_LABEL_BATCH_SUBMIT and APS5_BATCH_CAP are all unset.

**Why this impact.** About 50 or more submits per frame at 20-200 µs each (higher on some Windows drivers), plus write-back memcpys, all extend the global hold that every other queue waits behind.

**Proposal.** S:
- Tune APS5_BATCH_CAP from [recorder] submit timings.
- Move the Reap in SubmitRecorded after vkQueueSubmit, so the GPU gets the batch before the CPU spends time on completions.
M: add a dedicated submit thread owning a queue mutex. Submit() under GpuMutex ends the command buffer and enqueues {cmd, fence, serial}. The thread calls vkQueueSubmit in FIFO order. The presenter's vkQueueSubmit and vkQueuePresentKHR take the same queue mutex and preserve order by enqueueing too.
- Timeline WaitSerial is wait-before-signal safe.
- finish(wait) must first wait until the batch was handed to the driver (a per-serial 'submitted' atomic).

**Risk.** Submission order between the recorder and the presenter must remain FIFO. A fence waited on before its submission would block until the thread submits it, which is fine as long as the thread never needs GpuMutex.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [recorder] 'submits N, vkQueueSubmit mean X us, max Y us'.
- [recorder] 'reaps (with work) retired N batches in X ms' and 'completions ran X ms'.
- [labels] 'N at the next packet, N by deadline, N by size cap'.
- [present] 'per flip: N batches recorded'.

#### recorder-single-queue-1: One VkQueue and one recorder for all guest rings: no async-compute overlap, and every wait waits for other queues' batches too

- **Impact (claimed):** medium · **Effort:** XL · **Category:** gpu-side-efficiency · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp:919-926,1110,1130,2293,2414; core/libs/prx/libSceAgcDriver/Graphics/include/Recorder.hpp:735-736; core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:1490,3087-3094`

**Mechanism.** The device is created with queueCount = 1. The recorder, the CommandBatch path and the presenter all submit to that single queue.
There is one open batch for the whole process, tagged by whichever worker opened it, so graphics and async-compute rings are interleaved in one in-order stream separated by full barriers. The PS5's graphics/compute overlap is lost.
A CPU wait for one queue's producer (hook SyncThrough, drains via WaitSerial(newest)) is a wait for every batch submitted before it, from any queue. A wait on a range in the open batch submits it and waits for the whole timeline.

**Evidence.** - VulkanDevice.cpp:920-926: queueInfo.queueCount = 1; ... deviceInfo.queueCreateInfoCount = 1.
- VulkanDevice.cpp:1110: vkGetDeviceQueue(state->device, family, 0, &state->queue).
- VulkanDevice.cpp:1130: one std::make_unique<Graphics::Recorder>.
- Recorder.hpp:735-736: std::unique_ptr<Batch> open; std::deque<std::unique_ptr<Batch>> inFlight;
- Recorder.cpp:1490: batch->queue = GuestMemory::GpuLockThreadTag();
- Recorder.cpp:3053-3055: open target -> Submit(); targetSerial = submissions; (newest).
- Recorder.cpp:3087-3094 and 3144: the [syncwait] 'batches ahead by queue' counter exists precisely because of this.
- Presenter vkQueueSubmit and vkQueuePresentKHR on state->queue: VulkanDevice.cpp:2293, 2414.

**Why this impact.** It caps GPU parallelism and lengthens every CPU-GPU sync. Combined with the global recording lock, it serializes the whole graphics pipeline.

**Proposal.** Use per-guest-queue (or per-class: graphics vs compute) Recorder instances, each with its own open batch, command pool and timeline. Submit them to separate VkQueues: a second queue of the same family, or a compute-only family.
- Express cross-queue ordering only where the guest does (labels and WAIT_REG_MEM), via timeline semaphore waits in VkTimelineSemaphoreSubmitInfo.
- Split GpuMutex per recorder for recording, keeping shared caches (imports, texture cache, pipeline store) behind finer locks.
This is the 'best possible' architecture step and the largest effort.

**Risk.** High: cross-queue hazards on shared images and buffers (VK_SHARING_MODE_CONCURRENT or ownership transfers), label table semantics, completion ordering (see the 'Known gap' at Recorder.cpp:2931).

**How to measure.** With APS5_PROFILE_DRAW=1:
- [syncwait] 'batches ahead by queue' and 'avg batches up to the target'.
- [lock] per-queue 'waited ... ms'.
- [present] GPU busy versus idle.

#### recorder-pipelinekey-1: Per-draw pipeline key built as a growing heap vector and FNV-hashed byte by byte under the store mutex

- **Impact (claimed):** low · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:354-359,365-447,449-456,514-534; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1740,1770`

**Mechanism.** Every draw on the full path rebuilds the pipeline key:
- dozens of std::vector::insert calls with no reserve, so several reallocations for roughly 300-800 bytes covering shaders, bindings, attributes, LayoutKey words, blend states, formats, depth/stencil and mesh/tess fields
- a one-byte-at-a-time FNV-1a hash
- a full byte compare against the stored key
all under PipelineStore::mutex inside the GpuMutex hold.
The full path covers every depth-tested draw (recipes require !state.depth) and every indirect draw. AcquireFramebuffer then scans the pipeline's framebuffer list. Estimated 1-4 µs per draw.

**Evidence.** - Pipeline.cpp:357-358: const auto bytes = std::as_bytes(std::span(&value, 1)); key.insert(key.end(), bytes.begin(), bytes.end());
- Pipeline.cpp:451-454: for (const auto byte : key) { hash ^= ...; hash *= 1099511628211ull; }
- Pipeline.cpp:520: key built after std::lock_guard lock(store.mutex) at 518.
- Pipeline.cpp:528: if (it->key == key).
- Draw.cpp:1770: recipe creation requires ... && !state.depth && ... (depth draws never get recipes).
- Draw.cpp:1740: CachedPipeline(...) per full-path draw.

**Why this impact.** A few µs × thousands of non-recipe draws comes to several ms per frame inside the global hold. That is modest next to other costs but cheap to remove.

**Proposal.** - Build the key into a reserved, thread_local buffer (reserve 1024).
- Hash word-at-a-time (e.g. reuse ShaderRecompiler::HashBytes or an XXH3/wyhash-style mixer), and compute the key before taking store.mutex.
- Better: cache a weak_ptr<Pipeline> in the driver's DrawEntry (Execution draw cache), keyed by decode + variant ids + layout key, so hits skip key building entirely.
- Consider allowing recipes for depth draws.

**Risk.** Low (a hash change only). The cached weak_ptr must be revalidated on device replacement (alive()).

**How to measure.** With APS5_PROFILE_DRAW=1: [draws] 'pipeline=X ms' phase total per 10 s, and the [draws] per-kind averages.

#### recorder-framebuffer-1: Framebuffers are cached per Pipeline with an LRU of 8, so pipelines used on many targets recreate them every frame

- **Impact (claimed):** low · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:291-327,63-75; core/libs/prx/libSceAgcDriver/Graphics/include/Pipeline.hpp:56-72; core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:2111-2112`

**Mechanism.** Each Pipeline object owns its own framebuffer list, capped at 8. N pipelines drawing to the same targets each create a separate VkFramebuffer.
A pipeline used against more than 8 target sets per frame (full-screen blits, post-process chains, mip or shadow cascades) evicts and recreates framebuffers every frame. Each is a vkCreateFramebuffer, with the destroy later, under GpuMutex.
A recipe whose weak framebuffer was evicted falls into AcquireFramebuffer again. Non-resident targets get a fresh framebuffer per draw.

**Evidence.** - Pipeline.cpp:314-319: constexpr std::size_t bound = 8; while (framebuffers.size() >= bound) { victim = find_if(use_count() == 1); framebuffers.erase(victim); }
- Pipeline.cpp:311: std::make_shared<Framebuffer>(context, renderPass, targets, extent) -> vkCreateFramebuffer at 74.
- Pipeline.cpp:312: if (!resident) return framebuffer; (no caching).
- Pipeline.hpp:72: std::vector<CachedFramebuffer> framebuffers; (a per-pipeline member).
- Draw.cpp:2111-2112: recipe framebuffer weak lock, then AcquireFramebuffer.

**Why this impact.** Each vkCreateFramebuffer is small (µs-scale), but there are potentially hundreds per frame for post-processing-heavy titles, all under the global lock.

**Proposal.** Move the framebuffer cache to the device. Key it by (render-pass compatibility key: formats + attachmentLayout + attachment count, views, extent), and share it across pipelines, which works once render passes are shared (recorder-pipeline-compile-1). Use an LRU sized in the hundreds, evicting only unreferenced entries. VK_KHR_dynamic_rendering removes framebuffers altogether.

**Risk.** Low. The views-recycling guard (owners weak_ptr identity) must be kept.

**How to measure.** No counter exists. Add framebuffer creations per 10 s to the [pipecache] line (APS5_PROFILE_DRAW), or use perf to sample vkCreateFramebuffer.

#### recorder-atomic-snapshot-1: Lock-free snapshot read is an atomic<shared_ptr> load (internal spinlock plus refcount RMW) on every flush-hook call

- **Impact (claimed):** low · **Effort:** S · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:627,642-654,1010,2278,2301; core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:1713-1714`

**Mechanism.** FlushForAccess runs on every driver or HLE guest-memory access made through GuestMemory Read/Write, from workers and game threads alike. It calls SnapshotOverlaps, which does pendingWrites.load(). noteWrite calls SnapshotCovers, which loads again.
std::atomic<std::shared_ptr> is not lock-free in libstdc++ or MSVC. A load takes an internal lock bit or spinlock and increments and decrements the control-block refcount: two or more contended RMWs on cache lines that all threads share.
The code already keeps a publishGeneration counter, bumped after every store, that would allow per-thread caching.

**Evidence.** - Recorder.cpp:627: std::atomic<std::shared_ptr<const WriteRanges>> pendingWrites;
- Recorder.cpp:644: const auto snapshot = pendingWrites.load(std::memory_order_acquire);
- Recorder.cpp:1010: if (!HookSnapshotEnabled() || SnapshotOverlaps(address, bytes)) in FlushForAccess.
- GuestMemory.cpp:1714: FlushGpuWrites calls the hook, from every Read/Write at 1719, 1735, 1748, 1806, 1841.
- Recorder.cpp:2353: publishGeneration.fetch_add(1, release) after each store.
- Default: APS5_NO_HOOK_SNAPSHOT is opt-in, so the snapshot path is on.

**Why this impact.** It scales with the hook call rate across all threads. It is cheap per call but a shared-cacheline hotspot under multi-threaded access.

**Proposal.** Use a thread_local cached std::shared_ptr<const WriteRanges> plus the cached publishGeneration value. On each check, do a relaxed/acquire load of publishGeneration; reload the shared_ptr only when it changed.
The race window is the same as today: the generation moves after the store, and a reader that sees the old generation could equally have loaded the old pointer.

**Risk.** Low: identical visibility semantics. The thread_local copy keeps one stale snapshot alive per thread (memory only).

**How to measure.** With APS5_PROFILE_DRAW=1: [recorder] 'hook N calls, M locked' gives the call rate. Linux perf: look for contention in _Sp_atomic::load / __atomic_wait.

#### recorder-host-barrier-1: Every dispatch's trailing barrier includes HOST stage/HOST_READ, although Submit already provides batch-end host visibility

- **Impact (claimed):** low · **Effort:** S · **Category:** gpu-side-efficiency · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp:3307-3308; core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:1394,2776-2783,1610,1627-1630`

**Mechanism.** Host visibility only matters once the batch's fence is observed. Draws already defer it: endOpenRenderPass sets hostReadOwed, and Submit records one ALL_COMMANDS->HOST barrier. Label runs at Submit also include HOST.
Dispatches still put VK_PIPELINE_STAGE_HOST_BIT and VK_ACCESS_HOST_READ_BIT in every trailing barrier. That widens each barrier's destination scope, and on some drivers it can add per-barrier cache write-back work. This is driver-dependent.

**Evidence.** - VulkanDevice.cpp:3307: dispatchedAccess = ...|VK_ACCESS_HOST_READ_BIT;
- VulkanDevice.cpp:3308: ..., VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_HOST_BIT, ...
- Recorder.cpp:1394: open->hostReadOwed = true; (draws).
- Recorder.cpp:2778-2781: if (open->hostReadOwed && !hostReadCovered) recordBarrier(..., ALL_COMMANDS, HOST, ..., HOST_READ).

**Why this impact.** The benefit is driver-dependent. It is cheap to remove and is a natural first step of recorder-barriers-1.

**Proposal.** Drop HOST from the dispatch trailing barrier and its covered mask. Set hostReadOwed on the open batch (add Recorder::OweHostRead(), or let MarkCovered set it) so Submit records the single host barrier.
Audit the covered-mask consumers (fill, copy, unit shadow, storage write-back) that test HOST_READ coverage; the fill/copy/shadow checks test only TRANSFER/SHADER bits.

**Risk.** Low: one host barrier before the fence is the Vulkan-correct pattern. Check that no CPU read happens between a dispatch and its batch's Submit without going through Submit+fence (all syncs submit first).

**How to measure.** APS5_PROFILE_GPU=1: [gputime] 'dispatch-trail' class time. Also the [present] GPU busy per present.

#### recorder-proc-lookup-1: vkGetDeviceProcAddr name lookups on default wait paths and per-batch query reads

- **Impact (claimed):** low · **Effort:** S · **Category:** other · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:2878,3076,2087,1876,1883,3225; core/libs/prx/libSceAgcDriver/Graphics/include/Context.hpp:147-154`

**Mechanism.** Context::Function resolves the entry point by name on every call; the header notes this goes through the loader's lookup, which takes a mutex.
The constructor's table (Recorder.cpp:1072-1088) omits several hot entries:
- vkWaitSemaphoresKHR: resolved on every WaitSerial (drains, dispatch presync) and every unlocked hook wait.
- vkGetQueryPoolResults: per finished batch once occlusion counting starts on the no-BDA path.
- vkCmdWriteTimestamp: per timed range, profiling only.
- vkGetFenceStatus: per waited batch, profiling only.

**Evidence.** - Recorder.cpp:2878: context.Function<PFN_vkWaitSemaphoresKHR>("vkWaitSemaphoresKHR") in WaitSerial.
- Recorder.cpp:3076: same in syncThroughUnlocked.
- Recorder.cpp:2087: context.Function<PFN_vkGetQueryPoolResults>(...) in readSamples.
- Context.hpp:150-151: ++DeviceProcLookups(); deviceProc(device, name).

**Why this impact.** Roughly 0.1-1 µs plus loader mutex contention per wait. It is small but on the default path, and trivially fixed.

**Proposal.** Resolve vkWaitSemaphoresKHR (when timeline), vkGetQueryPoolResults and vkCmdWriteTimestamp once in the Recorder constructor alongside the existing PFN members, and use them via function(...).

**Risk.** None.

**How to measure.** With APS5_PROFILE_DRAW=1: the [vk] 'deviceProc lookups inside the device call' line, and DeviceProcLookups().

### Incomplete or stubbed code noticed

- `core/libs/prx/libSceAgcDriver/Graphics/include/Context.hpp:20-22,110-112`: RenderCache, DrawQueue and GraphicsPipelineCache are forward-declared, and Context has renderCache/drawQueue/graphicsPipelines members. No definition or assignment exists anywhere in core (grep finds none). *Performance relevance:* These are planned async-draw-queue and dedicated graphics-pipeline-cache hooks that were never built. Pipelines are still created synchronously in CachedPipeline under GpuMutex, and recording is single-threaded.
- `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:1775-1821; Graphics/include/Recorder.hpp:439-446`: The hazard tracker (NoteAccess) only counts: 'the real barriers are recorded as before'. It runs only with APS5_BARRIER_VALIDATE=1. *Performance relevance:* The infrastructure for hazard-driven barrier elision exists but does not drive barriers. Every dispatch and pass boundary still gets a full barrier (recorder-barriers-1).
- `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:2929-2933`: Known gap: a later batch writing the same range GPU-direct in place is not ordered against an earlier batch's completion write-back ('went from deterministically stale to racy'). *Performance relevance:* This limits moving completions out of the GpuMutex or onto other threads or queues. Any async-completion or multi-queue work must close it first.
- `core/libs/prx/libSceAgcDriver/Graphics/src/Pipeline.cpp:199-204`: Pipelines use only VIEWPORT, SCISSOR, DEPTH_BOUNDS and DEPTH_BIAS as dynamic state. There is no use of the core stencil/blend-constant dynamic states, extended dynamic state, graphics pipeline libraries or dynamic rendering. *Performance relevance:* This multiplies pipeline count and synchronous compiles (recorder-dynamic-state-1, recorder-pipeline-lru-1).
- `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:1895-1907,1960-1992`: SamplesTotal memcpy-reads the GPU sample counter buffer that a recorded fold dispatch may still be writing, and waits with VK_QUERY_RESULT_WAIT_BIT on in-flight segments. *Performance relevance:* It is a hidden locked GPU wait (not counted in the [recorder] sync table) on the ZPASS_DONE fallback path, plus a possible stale value (correctness).
- `core/libs/prx/libSceAgcDriver/Graphics/include/Draw.hpp:89-103 vs Graphics/src/Draw.cpp:1697,1711`: The header says draws with copied writes are recorded and waited for 'until' APS5_RECORD_COPIED_DRAWS. The code lists them without waiting by default (APS5_NO_RECORD_COPIED_DRAWS unset). *Performance relevance:* The documentation is stale. The default path does not wait, which is the faster behaviour, so the comment misleads anyone tuning sync behaviour.

### Settings to try without code changes

| Setting | Expected effect | Risk |
|---|---|---|
| `APS5_PROFILE_DRAW=1` | No speedup. Every 10 s it prints [recorder] (syncs by source/site/thread, snapshot rebuilds, submits and vkQueueSubmit mean/max, reaps, completions), [lock] (per-queue waits, holds by site, locked GPU waits), [draws] (render passes begun/continued, pipeline phase, recipe misses), [pipecache] (hits/misses/evicted), [barriers], [present] (GPU busy vs idle gaps per present) and [syncwait]. This is the baseline needed to rank the findings. | Profiling overhead: clock reads, per-batch timestamp queries, fence-status queries in finish. |
| `APS5_PROFILE_DRAW=1 APS5_BARRIER_VALIDATE=1` | Adds 'validate: would emit X (...) + Y batch ends, would skip Z' to [barriers]. This quantifies how many barriers hazard-driven recording could remove; behaviour is unchanged. | Extra CPU per command (range bookkeeping). Diagnostics only. |
| `APS5_BATCH_CAP=256 (default 64; 0 disables the cap)` | Roughly 4x fewer vkQueueSubmit calls and queue-0 reaps per frame, which shortens GpuMutex holds when [recorder] shows a high submit mean. | The GPU starts each batch later. Cross-queue waits on labels in the open batch rely on the 1 ms deadline or a wait-triggered submit. It could lower FPS if the GPU is starved. |
| `APS5_BATCH_CAP=16` | The GPU starts work sooner and CPU/GPU overlap improves when [present] shows large GPU idle gaps while the CPU records. | More submits and reaps under the lock. |
| `APS5_LABEL_FLUSH_US=250 (default 1000 with label batch submit)` | Label-carrying batches are submitted after 250 µs, reducing the latency of cross-queue WAIT_REG_MEM polls and drains. | More submits ([labels] 'by deadline' grows). |
| `ANYPS5_SHADER_CACHE_DIR=<persistent fast path>, and make sure ANYPS5_NO_SHADER_CACHE is unset` | The SPIR-V disk cache and the vk-pipelines-*.bin VkPipelineCache persist across runs. A second run of the same area avoids driver compiles during the synchronous in-lock pipeline creation, so stutter drops. | None: caches are validated by header, UUID and checksum and are rejected when stale. |
| `APS5_RELEASE_QUEUE_MAX=1024 (default 256)` | Avoids inline destruction of kept objects on worker threads when the release thread falls behind (see [recorder] 'batches over the queue bound'). | Higher peak memory from deferred objects. |
| `APS5_PROFILE_GPU=1` | [gputime] per-class GPU time (dispatch-lead, dispatch-trail, draw, label-run, copy and so on), the untimed share and the batch span. This shows how much GPU time barriers and passes consume. | BOTTOM_OF_PIPE timestamps around each range serialize the GPU further, and only 512 ranges per batch are timed. Treat absolute numbers as inflated. |
| `APS5_FULL_BARRIERS=1` | A negative control: records every leading barrier. The FPS delta against default shows how sensitive this title is to barrier count, and so the upside of recorder-barriers-1. | Slower. Diagnostic only. |
| `APS5_HOOK_LOCKED_WAIT=1` | A negative control: flush-hook waits stay under GpuMutex. An FPS drop confirms lock-held GPU waits are a major limiter, and so the upside of recorder-locked-waits-1. | Slower. Diagnostic only. |

<a id="memory-tracking"></a>
## Guest memory tracking, GpuMutex, captures and write evidence (GuestMemory.cpp/.hpp, WriteWatchCoverage, MemoryAccessScope, ShaderMemory, Driver/Memory/{WriteEvidence,WrittenBuffers,CaptureValidation,CaptureDiagnostics}, DeviceAccess) plus the libc write-watch backends they call (GuestWriteWatch.cpp on Linux, WindowsMappings.hpp on Windows)

### How it works

The driver reads and writes guest memory through GuestMemory. Every Read, ReadCommitted, EqualsCommitted, Write and WriteChanged first calls FlushGpuWrites. That calls Recorder's FlushForAccess hook, which syncs the GPU when recorded work writes the range and publishes pending storage-image or unit-shadow results. The call then checks the mapping with describePages and does the memcpy or memcmp. On Windows, describePages uses a 1-byte-per-4KiB page-state cache. On Linux there is no cache: each call is a PROCMAP_QUERY ioctl, or a parse of /proc/self/maps.

CPU writes are detected differently per OS. On Linux, direct-memory mappings are registered for asynchronous userfaultfd write protection. PAGEMAP_SCAN with PM_SCAN_WP_MATCHING reports the written pages and write-protects them again. On Windows, private commits use GetWriteWatch. Section-backed direct-memory views use VirtualProtect arming plus an access-violation handler (HandleWrite).

CollectWrites walks a range under one tracker mutex and stamps each touched 64 KiB block with a new 32-bit generation. Walks are memoized per thread per epoch in a 64-entry ring. The epoch is bumped at every submission start, wait and drain. Consumers (texture and storage-image caches, draw-input snapshots, heap mirrors, unit shadows, dispatch/draw caches) store a generation and call UnchangedSince to skip byte compares. If it fails, or the range is unwatched (CollectWrites returns 0), they fall back to full-surface compares, copies and re-uploads.

Driver GPU stores call MarkWritten on their blocks. Driver CPU stores (storeOwn) walk the range before and after the store. ShaderMemory captures the descriptor and SRT dwords a shader reads, page by page or word by word. Word-wise reads go through a per-dword write-evidence classifier, which avoids GPU syncs. Draw and dispatch cache hits re-validate those dwords with CompareMapped.

### Threads and locks

Threads that run this code: the queue workers (graphics queue 0 plus each compute queue), the presenter, game threads (Submit's CheckRange, the flush hook, write faults) and the release thread.

GpuMutex wraps std::recursive_mutex. That is a pthread recursive mutex on Linux and SRW-based on MSVC. It is unfair and has no priority, and timing is added only under APS5_PROFILE_DRAW. It is held for these operations:
- Each draw from lockForDraw through the whole Graphics::Draw. This includes draw-input copies, CheckRange, precollect walks, texture lookups and compares, and recording.
- Each dispatch's stage B.
- Label, store and DMA recording.
- Drains, flips and presentation.
- The flush hook's sync. The GPU wait is released only at depth 1, so a nested hook waits for the fence while holding the mutex.

The tracker std::mutex is held across every write-watch kernel walk and every UnchangedSince, MarkWritten, TrackerGeneration, ChangedBlocks and StoredOver call.

Other locks:
- Linux: GuestWriteWatch::_lock (a shared_mutex) is taken exclusive in every Collect.
- Windows: WindowsMappings::mutex is taken by every Collect and by the game-thread access-violation handler (HandleWrite).
- The flush hook takes StorageTexture Pending().mutex and UnitShadow Registry().mutex on every access. The write-evidence path takes writtenBuffersMutex (with an O(4096) scan) and labelTableMutex.
- DeviceUseGate takes a global std::mutex twice per packet.

The memo ring and the collect epoch are per thread and take no lock.

### Findings

#### memory-tracking-linux-no-page-cache: Linux has no page-state cache: every guest-memory range check is a PROCMAP_QUERY syscall, or a /proc/self/maps parse on older kernels

- **Impact (claimed):** high · **Effort:** M · **Category:** memory-tracking · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:483-647`

**Mechanism.** describePages uses the cached PageSpan tables only for the 'arena' and 'image' spans. On Linux GuestArenaRange returns size 0, so arena.allocate() fails. The image span is built only under _WIN32, and the ForgetPages invalidator is never installed. So every verify(), DescribeCommitted, CompareMapped and CopyMapped call does at least one ioctl(PROCMAP_QUERY) per VMA run (kernel 6.11 or later). On older kernels, or with APS5_NO_PROCMAP_QUERY set, it instead opens std::ifstream("/proc/self/maps") and parses lines with istringstream until it reaches the address. Callers per draw: draw-cache hit validation runs CompareMapped once per captured run of every stage; the index buffer gets a CheckRange; each vertex copy gets a CheckRange plus another one inside CopyDrawInput's Read; DrawDecode does a CheckRange; and every capture page costs Accessible plus Read, i.e. 2 queries. Every label/WAIT_REG_MEM Read and Write adds one more. Windows hits the cache, which costs a few byte loads.

**Evidence.** core/libs/prx/libc/src/GuestArena.cpp:109-133: the Arena() body that sets _base/_end exists only under #ifdef _WIN32, so on Linux Size() is 0 (:183-186). GuestMemory.cpp:485-486: arena.allocate() is fed GuestArenaRange. :387-388: allocate() returns false when size==0. :488-504: the image span is _WIN32-only. :505: the invalidator is set only if something is cached. :542: spanOf() is therefore always null on Linux. :590-608: one ioctl(fd, PROCMAP_QUERY) per run. :615-643: the ifstream /proc/self/maps fallback. :517-529: APS5_NO_PROCMAP_QUERY (default unset), which also falls back when the kernel lacks the ioctl. Callers: VariantValidation.cpp:28 and CaptureValidation.cpp:128 (CompareMapped per region); Graphics/src/Draw.cpp:813 (index CheckRange), :896-897 (vertex CheckRange plus CopyDrawInput->Read at :751); ShaderMemory.cpp:104 and :111 (Accessible plus Read per page); Execution/src/Driver/Draw/DrawDecode.cpp:70.

**Why this impact.** An estimated ~10-40 describePages per draw. At thousands of draws per frame that is 30k-120k syscalls per frame on the graphics worker, at roughly 0.5-2 µs each, so ~15-240 ms per frame against a ~110-125 ms frame at 8-9 FPS. On kernels older than 6.11 each call becomes a file open plus text parse (tens of µs to ms), which would make the title unplayable. Part of this (CheckRange and CopyDrawInput inside Graphics::Draw) runs inside the GpuMutex hold, so it also stalls compute queues. This is Linux-only; Windows already caches.

**Proposal.** Port the page-state cache to Linux:
1. In PageSpan::allocate, under !_WIN32, reserve the table with mmap(PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE) over the guest window: GuestArena's ArenaStart 0x2'0000'0000 to ApplicationAreaEnd 0xFC'0000'0000, about 250 M entries, demand-zero.
2. In the Linux branch of describePages, store the flags for the whole [vma_start, vma_end) run that PROCMAP_QUERY returns, like the Windows branch does at :578-585, with the same GuestAllocationsGeneration before/after re-check.
3. Call GuestAllocationsSetInvalidator(&ForgetPages) on Linux too.
4. Make every Linux guest mmap/munmap/mprotect (libkernel DirectMemory Map at DirectMemory.cpp:273, unmap/protect, flexible memory) report through GuestAllocations Begin/End/recordChange or GuestAllocationsInvalidate.

Cheaper interim step: a per-thread cache of the last ~8 PROCMAP_QUERY results (start, end, flags), valid only while GuestAllocationsGeneration_nid_postfix() is unchanged.

**Risk.** If a mapping change is not reported, a page stays cached as readable after it is unmapped. The memcpy/memcmp then crashes with SIGSEGV instead of the clean 'not readable' exception. Every Linux path that changes guest mappings must be audited. Host-only ranges must stay uncached, as on Windows.

**How to measure.** With APS5_PROFILE_DRAW=1, the [guestmem] line shows 'verify N calls ... s'. That covers CheckRange, Accessible and DescribeCommitted only: CompareMapped and CopyMapped have no TimedAccess, so add one around the Linux ioctl, e.g. a TimedAccess(CounterQuery) as the Windows branch has. Outside the program: `strace -f -c -e trace=ioctl -p <pid>` or `perf trace -s -p <pid>`. In the driver: the [draw] driver-phase rows DrawRowKeyLookupValidate and Graphics::Draw phases. Check `uname -r`: 6.11 or later is needed for PROCMAP_QUERY.

#### memory-tracking-windows-imports-unwatched: Windows: every host import is excluded from write tracking, so snapshot reuse is disabled for most GPU-visible memory and each check becomes a full compare or copy

- **Impact (claimed):** high · **Effort:** M · **Category:** memory-tracking · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:284-290,408-413`

**Mechanism.** On Windows, decideImportWatch unconditionally sets unwatchImports=true. After each import, GuestMemory::Unwatch calls WriteWatchCoverage::Exclude on every overlapping 64 KiB block. tracker.covers() is then false, so CollectWrites returns 0 and UnchangedSince is always false for those resources. The fallbacks that follow:
- fastRevalidate fails with FastFail::Collect, which forces a full resource build per draw.
- A cachedTexture hit is proven only by EqualsCommitted, a whole-surface memcmp through the flush hook under the texture-cache mutex.
- StorageTexture::Refresh compares the whole surface against 'original'.
- CopyDrawInput gets generation 0, so it skips ReusableDrawSnapshot. Every indexed draw allocates a new Buffer, copies the index buffer and scans all indices for the maximum.
- Heap mirrors refuse the range.
- Unit shadows require Watched().
- knownValueCurrent can never succeed.
Windows direct memory is a section (CreateFileMappingW) mapped as views. Write detection for those views is VirtualProtect arming plus the access-violation handler, not GetWriteWatch dirty bits. The import itself goes through a separate alias view of the section. So the debt note's concern (a driver marking pages dirty after the import window) applies to private MEM_WRITE_WATCH memory, not to section views.

**Evidence.** GuestBufferMemory.cpp:286-290 (#ifdef _WIN32 ... state.unwatchImports = true;), :408-413 (ImportWatched then `if (entry.unwatched) GuestMemory::Unwatch(base, bytes)`), :397 (entry.alias = GuestArenaMapAlias for MEM_MAPPED). GuestMemory.cpp:1144-1147 (coverage.Exclude); WriteWatchCoverage.hpp:22-27 (Covers false for excluded blocks); GuestMemory.cpp:1102 (collectWrites returns 0 when !covers). Consumers: ShaderResources.cpp:1458-1459 (FastFail::Collect), :403-412 (EqualsCommitted fallback), Texture.cpp:1247-1251, 1302 (equalsOriginal), Graphics/src/Draw.cpp:740-751 and 849-866 (copy plus index scan), GuestBufferMemory.cpp:843-845 (heapUnwatched), UnitShadow.cpp:520, WriteEvidence.cpp:137-141. Section-backed direct memory: libkernel/DirectMemory/DirectMemory.cpp:229, 271. Protection-based tracking: WindowsMappings.hpp:134-145, 233-260; CrashReport.cpp:244. docs/dev/TechnicalDebt.md:243 notes that this 'disables snapshot reuse and unit shadows' and that restoring tracking on import retirement and measuring the comparison cost remain unimplemented.

**Why this impact.** Per draw, every texture, storage image and index or vertex buffer in imported memory takes the slow path: a full rebuild, plus a memcmp or copy proportional to resource size (MiB per texture, all index bytes per indexed draw). Every compare also runs the flush hook, which can sync the GPU. With thousands of draws per frame this means GB of memory traffic per frame plus serialized lock time. This alone can explain low FPS on Windows.

**Proposal.** 1. Keep tracking for imports made through a section alias (entry.alias != nullptr, i.e. MEM_MAPPED direct-memory views). Their CPU-write detection is protection-based on the guest view and independent of the GPU driver's activity on the alias; recorded GPU writes are already stamped with MarkWritten. Unwatch only imports of private (MEM_WRITE_WATCH) memory.
2. Port the Linux ProbeImportWriteProtection approach (GuestBufferMemory.cpp:307-313) to Windows, so the decision is measured on the user's driver.
3. Implement the debt item: restore coverage when an import is retired (coverage.Restore exists in WriteWatchCoverage.hpp:51-73).

**Risk.** If the GPU driver's import somehow changed the guest view's protection, or skipped the AV handler, CPU writes could be missed and textures or buffers would be stale. Verify with the probe and a visual A/B. Private-memory imports must stay compared.

**How to measure.** stderr shows '[write-watch] host imports are compared on Windows because driver writes can arrive after the import window' when this path is active. With APS5_PROFILE_DRAW=1: [guestmem] 'compare N calls X MiB Y s' and the read-site 'texture-compare' samples; the resource-build fast-fail counters (FastFail::Collect / Changed) and LookupOutcomes rows; [hooksync] 'texture-compare' and 'mirror-refresh' entries; the [draws] pagesWalked and lookup µs.

#### memory-tracking-gpumutex-unfair-wide-hold: GpuMutex is an unfair recursive mutex held across the whole draw, including its guest-memory validation

- **Impact (claimed):** high · **Effort:** M · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/include/GuestMemory.hpp:118-145`

**Mechanism.** GpuMutexType::lock() is std::recursive_mutex::lock() plus owner and depth bookkeeping when not profiling. It gives no FIFO order and no priority, so a releasing compute worker can immediately re-acquire it ahead of a waiting graphics worker. Every queue worker takes it per draw, dispatch stage B, label/store recording, drain and flip; the presenter and game threads take it through the flush hook. For draws, lockForDraw takes it before localDevice->Draw, so everything in Graphics::Draw runs under it: CheckRange, CopyDrawInput copies, the precollect walks, texture lookups and compares, uploads and Vulkan recording. Dispatches have an unlocked stage A (PrepareDispatch) but draws do not. APS5_LOCKED_DRAW_PREPARE (opt-in) only moves the lock earlier. The header itself says the graphics worker's frame is mostly spent waiting for this lock behind the compute workers.

**Evidence.** GuestMemory.hpp:118-122 ('the graphics worker's frame is mostly spent waiting for this lock behind the compute workers'), :139 (std::recursive_mutex mutex). GuestMemory.cpp:1575-1580 (non-profiled lock() = mutex.lock(); acquired()), :1544-1557 (unlock + unlock hook). Execution/src/Driver/Draw/Draw.cpp:41-60 (APS5_LOCKED_DRAW_PREPARE opt-in), :260-276 (lockForDraw), :370-383 (localDevice->Draw / DrawFromRecipe under the lock). Execution/src/VulkanDevice.cpp:2531-2537 (Draw = Graphics::Draw, no unlocked stage); :2590-2602 (dispatches have stage A / stage B). Graphics/src/Draw.cpp:813, 849, 896-897 (guest-memory work inside the draw).

**Why this impact.** Recording for all queues is effectively single-threaded. Because the lock is unfair, the graphics queue can wait behind long compute holds; the code's own comment says this wait dominates queue 0's frame. Everything per-draw that is described in the other memory-tracking findings (syscalls, walks, compares, copies) is multiplied by this serialization. That matches the user's report of idle CPU threads and an idle GPU.

**Proposal.** 1. (S/M) Graphics-priority handoff in GpuMutexType: queue-0 threads increment an atomic waitingGraphics before lock(). For an outermost acquisition by a non-queue-0 thread, lock() spins briefly and then yields while waitingGraphics>0. Alternatively use a ticket lock for outermost acquisitions so the order is FIFO. Recursion is kept by the existing owner/depth fields.
2. (M) Give draws a stage A like dispatches: run prepareDrawInputs' CheckRange, CopyDrawInput, CollectWrites and ImageRecord lookups before lockForDraw, re-checking the generations under the lock. Leave only recording and descriptor binding under the lock.
3. (XL) Split GpuMutex per recorder/queue and protect the shared caches with their own locks.

**Risk.** A priority gate can starve compute queues the graphics queue waits on through WAIT_REG_MEM, so bound the yield. Moving draw preparation out of the lock needs the same generation and serial re-checks that dispatch stage A/B uses (PendingSerial, PublishGeneration, ForgetSerial).

**How to measure.** APS5_PROFILE_DRAW=1. The [lock] line per queue shows 'waited X ms for the GPU mutex in N of M acquisitions', 'held ... by site (holds, held, max)', 'waited behind (holder/site ms)', and 'lock() / try / try failed; locked GPU waits'. Compare q0's waited ms against the frame time. APS5_NO_HOLD_PROFILE=1 reduces the profiling cost.

#### memory-tracking-compare-syncs-under-lock: Snapshot-validation compares go through the flush hook and sync the GPU, often while GpuMutex is held

- **Impact (claimed):** high · **Effort:** M · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:1746-1750`

**Mechanism.** Whenever UnchangedSince fails (block stamped, range untracked or unwatched), cache validation falls back to EqualsCommitted, which calls FlushGpuWrites. FlushForAccess then calls recorder->SyncThrough if recorded GPU work writes into the range: the open batch is submitted and its fence is waited on. When the compare runs inside a draw's or dispatch's hold (stage B, depth 2), syncThroughUnlocked refuses to release the mutex (depth != 1). The wait therefore happens while holding GpuMutex and every other queue stops. The compare is only a cache check: if recorded work will overwrite the range, the snapshot is stale anyway, so waiting to compare bytes buys nothing.

**Evidence.** GuestMemory.cpp:1746-1749 (EqualsCommitted -> FlushGpuWrites). Recorder.cpp:1010-1020 (hook: SnapshotOverlaps -> lock GpuMutex -> SyncThrough(…, true)), :3048 (`GpuMutex().DepthOnThisThread() != 1` returns false, so the wait is locked). Compare sites: ShaderResources.cpp:407-412 (equalsCommitted lambda in cachedTexture), Texture.cpp:1247-1251 / 1302 (Refresh equalsOriginal), GuestBufferMemory.cpp:725-739 (prepareRange: FlushGpuWrites + full recorder->Sync() before each writable mirror compare). GuestMemory.hpp:169-171 (NoteLockedGpuWait counts these).

**Why this impact.** Each locked GPU wait drains the GPU pipeline and blocks all queue workers for the length of a batch, which can be milliseconds. A handful per frame turns the GPU and CPU into lock-step and leaves both under-utilized, which matches the user's symptom. The frequency rises sharply wherever write tracking is ineffective (Windows imports, Linux unwatched imports, 64 KiB false sharing).

**Proposal.** At cache-validation sites, check Recorder::SnapshotOverlaps(address, bytes) first. If it overlaps, treat the snapshot as changed: re-upload, ordered after the pending writes in the same recorder stream, or take the storage-image view. Do not compare through the hook. Otherwise use EqualsCommittedUnsynced. For writable mirrors, replace the full recorder->Sync() in prepareRange with a SyncThrough of the range, done in an unlocked pre-phase like dispatch stage A's pre-sync (VulkanDevice.cpp:2600-2602).

**Risk.** The re-upload must be recorded after the pending GPU writes it supersedes; for in-place imported memory, the GPU copy must read after the writes. Wrong ordering gives stale texels. Keep the synced compare when it is a real semantic read (Read, labels).

**How to measure.** APS5_PROFILE_DRAW=1: the [lock] line 'locked GPU waits N / X ms' per queue; [hooksync] entries by queue/packet/site (texture-compare, mirror-refresh, draw-cache) with waited ms; the DrawRowValidateWait and DrawRowCaptureHookWaits driver phases; [presync] for dispatches.

#### memory-tracking-linux-writewatch-compiled-out: Linux write watch is silently compiled out when the build host's kernel headers predate 6.7, so all tracking falls back to compares

- **Impact (claimed):** high · **Effort:** S · **Category:** build-config · **Default path:** yes
- **Location:** `core/libs/prx/libc/src/GuestWriteWatch.cpp:13-28,252-263`

**Mechanism.** The whole Watch class (userfaultfd async write protection plus PAGEMAP_SCAN) exists only if <linux/fs.h> defines PAGEMAP_SCAN and <linux/userfaultfd.h> defines UFFD_FEATURE_WP_ASYNC, i.e. linux-libc-dev 6.7 or later. Otherwise GuestWriteWatchAvailable() returns false, and no message is printed: the '[memory] write watch unavailable' line lives inside the compiled-out class. WriteTracker.watched is then false, so CollectWrites returns 0 and UnchangedSince is false everywhere, and every consumer takes the compare/copy path described in memory-tracking-windows-imports-unwatched. At runtime the kernel must also be 6.7 or later (that case is reported).

**Evidence.** GuestWriteWatch.cpp:28 (`#if defined(__linux__) && defined(PAGEMAP_SCAN) && defined(UFFD_FEATURE_WP_ASYNC)`), :153-158 (message only inside the class), :257-262 (Available false otherwise). GuestMemory.cpp:817-819 (watched = GuestWriteWatchAvailable), :1102 (return 0). Compare GuestMemory.cpp:41-66, which defines PROCMAP_QUERY locally when headers lack it.

**Why this impact.** A build on a distro with older headers (e.g. Ubuntu 22.04's linux-libc-dev 5.15) loses all snapshot reuse on Linux with no diagnostic. That multiplies per-draw compares, copies and flush-hook syncs.

**Proposal.** Provide local fallback definitions, as GuestMemory.cpp does for PROCMAP_QUERY:
- struct page_region and struct pm_scan_arg;
- PAGEMAP_SCAN _IOWR('f', 16, struct pm_scan_arg);
- PAGE_IS_WRITTEN, PM_SCAN_WP_MATCHING, PM_SCAN_CHECK_WPASYNC;
- UFFD_FEATURE_WP_ASYNC and UFFD_FEATURE_WP_UNPOPULATED.
Then the feature is decided at runtime by the existing probe. Always print one startup line naming the write-watch mode (watched / unavailable: reason).

**Risk.** Low. The runtime probe (probe()) already validates behaviour before use.

**How to measure.** On Linux, look for either '[memory] write watch unavailable: ...' or a '[write-watch] host imports ...' line on stderr. Neither appearing suggests the feature is compiled out. With APS5_PROFILE_DRAW=1 the [guestmem] line will show 'collect 0 calls'.

#### memory-tracking-64k-false-sharing: UnchangedSince checks at 64 KiB granularity, so one CPU page write or label store invalidates every snapshot sharing that block

- **Impact (claimed):** medium · **Effort:** M · **Category:** memory-tracking · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:1196-1223`

**Mechanism.** Collects stamp the whole 64 KiB block for any written 4 KiB page. MarkWritten stamps whole blocks for each GPU label or store the driver records (WriteLabelOnGpu calls it per label). UnchangedSince and UnchangedSinceAll compare only these block stamps. Page-precise information already exists: cpuPages keeps per-4 KiB CPU stamps and driverPieces keeps up to 4 byte ranges per block. But only StoredOver uses it. Any resource that shares a 64 KiB block with per-frame CPU data or labels therefore fails reuse every frame: textures, index/vertex snapshots, dispatch spans and SRT tables suballocated next to dynamic constants or labels. The failure leads to a full rebuild (FastFail::Changed), ReusableDrawSnapshot eviction plus re-copy, or a Refresh compare or upload. driverPieces' 4-slot ring also degrades: with labels 0x20 apart, 'dropped' makes StoredOver conservative.

**Evidence.** GuestMemory.cpp:746-748 (WriteBlockBytes 65536). :1012-1019 (Linux stampWrittenRun stamps blocks) and :1041-1045 (Windows). :1259-1272 (MarkWritten stamps blocks). :1196-1207 / 1209-1223 (UnchangedSince(All): `stampOf(block) > generation`). :860-893 (cpuPages / driverPieces with 4 pieces). :1274-1306 (StoredOver uses page/piece precision). Comment at :765-767 ('the title's per-job slots are 0x20 apart'). VulkanDevice.cpp:1328, 1337 (MarkWritten per label). Consumers: ShaderResources.cpp:1558 (FastFail::Changed), Recorder.cpp:2225-2227 (snapshot erase), DispatchLookup.cpp:63-69.

**Why this impact.** Reuse misses turn per-draw validation into copies, compares and re-uploads, done inside the GpuMutex hold for draws. PS5 engines suballocate many resources from big heaps, so 64 KiB neighbourhoods often contain per-frame data. The cost is title-dependent but recurs every frame.

**Proposal.** When stampOf(block) > generation, make UnchangedSince(All) refine as StoredOver does. Fail only if a CPU page stamp (cpuPages) newer than the generation intersects the query's bytes within the block, or a driverPieces entry newer than it overlaps (or entry.whole/dropped is newer). Raise driverPieces capacity (e.g. 16) or keep a 4 KiB-granular driver page stamp array per block. Treat blocks with no page info (coverage restore, import-window stamps) as changed, as StoredOver does.

**Risk.** Must stay conservative wherever page info is missing or evicted. Adds a hash lookup per stamped block (cheap next to the compare it avoids).

**How to measure.** Add a counter to the [guestmem] line for 'block-stale but page-clean' answers. Existing signals with APS5_PROFILE_DRAW=1: resource fast-fail 'Changed' counts, LookupOutcomes snapshot/compare rows, [guestmem] compare MiB, and Recorder draw-snapshot reuse (the [draws] lines).

#### memory-tracking-collect-walk-serialization: Write-watch walks run under one global tracker mutex, and the per-thread memo (64 entries, reset at every submission, wait and drain) thrashes

- **Impact (claimed):** medium · **Effort:** M · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:1076-1121`

**Mechanism.** collectWrites takes the tracker std::mutex, then runs walkWrites while holding it:
- Linux: a PAGEMAP_SCAN ioctl that walks every PTE of the range and write-protects the written ones again; GuestWriteWatch also takes its _lock exclusively.
- Windows: GetWriteWatch, or a per-16 KiB std::map walk plus VirtualProtect re-arming under WindowsMappings::mutex.
Every UnchangedSince, MarkWritten, TrackerGeneration (each label group close), ChangedBlocks and StoredOver from every worker contends on the same mutex. The memo is a per-thread 64-entry FIFO ring with a linear scan. It matches only within the thread's current epoch, which is bumped at every submission start, WAIT_REG_MEM and drain. A graphics epoch touching more than 64 distinct texture and buffer ranges therefore re-walks them. UnchangedSinceCollected and CollectWritesUncached always walk; they are used for late labels, ClassifyAccess and AccessKeptByCpu on hook calls, and evictStale.

**Evidence.** GuestMemory.cpp:1100 (lockTracker), :1113 (walkWrites under the lock), :1022-1074 (walk). GuestWriteWatch.cpp:84 (unique_lock), :107-127 (PAGEMAP_SCAN, PM_SCAN_WP_MATCHING). WindowsMappings.hpp:219-275 (mutex, per-page map find, VirtualProtect, GetWriteWatch). GuestMemory.cpp:995-999 (64-entry ring), :1087-1099 (linear scan, epoch match), :1133-1136 (BumpCollectEpoch). PacketExecution.cpp:106 (bump per submission). WaitMemory.cpp:74-79 (per wait). PacketMemory.cpp:28 and 172 (drains). GuestMemory.cpp:1316-1327 and Texture.cpp:2508, 2520 (uncached walks in the hook). Header GuestMemory.hpp:79-96 documents the epoch design and APS5_PACKET_EPOCH (opt-in, worse).

**Why this impact.** Each walk costs about µs to tens of µs (proportional to range pages; a 16 MiB surface is 4096 PTEs) and is serialized across all queue workers and the presenter. Memo misses multiply walks per draw (stage A precollect, CopyDrawInput, the hook's uncached collects). Contention on the tracker mutex adds to the GpuMutex serialization.

**Proposal.** (a) Replace the 64-entry ring with a per-thread open-addressing hash keyed by page-aligned begin, storing end, epoch and unwatch serial, with capacity around 1-4K and entries invalidated lazily by epoch. That makes the memo hold a whole epoch's working set.
(b) Move the kernel walk out of the tracker mutex. Under the lock, bump the generation and register an in-flight walk. Unlocked, run the ioctl into a thread-local run list. Relock, stamp with the pre-bumped generation, and unregister. TrackerGeneration must wait for in-flight walks that started before it, to keep the guarantee documented at :1310-1312.
(c) Shard stamp access by block range, or make UnchangedSince read stamps with atomics, so readers do not take the mutex.

**Risk.** The TrackerGeneration / late-label ordering argument relies on walks being complete under the mutex. Breaking it can trust a label over a CPU store. Hash memo capacity must keep the 'entry only for a completed walk' rule.

**How to measure.** APS5_PROFILE_DRAW=1 [guestmem]: 'collect N calls X MiB Y s', 'collect-memo hits', 'collect epochs', 'tracker waits N / M' (waits divided by acquisitions shows contention). Graphics [draws] 'pagesWalked'. A/B with APS5_SHARED_COLLECT_MEMO=1 (256-entry shared ring) to estimate the capacity effect, and APS5_NO_COLLECT_MEMO=1 to see the upper bound of walk cost.

#### memory-tracking-flush-hook-fixed-cost: Every guest-memory access pays a flush-hook toll: two global mutexes, an atomic shared_ptr load and unconditional caller sampling

- **Impact (claimed):** medium · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Recorder.cpp:993-1044`

**Mechanism.** FlushGpuWrites, called by Read, ReadCommitted, EqualsCommitted, Write and WriteChanged, always runs FlushForAccess. That does: hookCalls.fetch_add on one global cache line; a QueuedLabelRanges scan; pendingWrites.load(), an std::atomic<std::shared_ptr> load (lock bit plus refcount RMW on a shared control block); StorageTexture::AccessKeptByCpu, which calls overlappingPending and locks the global Pending().mutex; then PublishShadowsOnly, whose AnyShadowedOverlaps locks the UnitShadow Registry().mutex. These run whether or not anything is pending. In addition, Read and Write call CountReadCaller and CountWriteCaller unconditionally, without the APS5_PROFILE_DRAW gate. Every 256th read and 16th write takes the global callersMutex, and on Windows calls GetModuleHandleExA(FROM_ADDRESS) through ModuleOffset, which takes the loader's lock (the code itself avoids this elsewhere for that reason). Captures run this per 4 KiB page and per word-wise dword.

**Evidence.** GuestMemory.cpp:1713-1715 (FlushGpuWrites -> hook), :1719-1721 (Read: hook, TimedAccess, CountReadCaller), :1841-1843 (Write: hook, CountWriteCaller), :302-312 and :224-236 (sampling; callersMutex), :76-80 (Windows ModuleOffset = GetModuleHandleExA), :275-277 (comment: per-call GetModuleHandleEx would take the loader lock under the GpuMutex). Recorder.cpp:994 (hookCalls), :627, 642-646 (atomic<shared_ptr> load), :1038-1044 (AccessKeptByCpu / PublishShadowsOnly). Texture.cpp:2460-2465 (Pending().mutex), :2399-2400. UnitShadow.cpp:483-488 (registry.mutex).

**Why this impact.** Hundreds of ns to µs per access uncontended, and more with eight workers hammering the same lines and mutexes. That is multiplied by every capture page and word, every draw-input copy, every label or WAIT read and every compare, so several ms per frame, partly under GpuMutex.

**Proposal.** 1. Fast reject without locks: keep std::atomic<std::size_t> counts of pending storage textures and live shadow units, and publish an immutable sorted range snapshot (like pendingWrites) for each registry. FlushForAccess then returns after a few loads when nothing overlaps.
2. Cache the pendingWrites snapshot per thread, keyed by Recorder::PublishGeneration(), as PendingView already does, instead of an atomic shared_ptr load per access.
3. Make hookCalls per-thread or profile-only.
4. Gate CountReadCaller and CountWriteCaller behind MemoryProfiled().

**Risk.** Low. The snapshot must be republished on every registry change under the existing registry mutexes; a reader seeing a slightly old snapshot must still fall through to the locked path when it overlaps.

**How to measure.** perf record -g on the queue workers, looking at pthread_mutex_lock/futex or SRW waits under FlushForAccess. With APS5_PROFILE_DRAW=1: [guestmem] read/write/compare call counts (calls × per-call toll), [hooksync] 'hook calls / locks', and the [capture] page and word read µs per capture.

#### memory-tracking-store-double-walk: Each driver CPU store costs two write-watch walks, a write fault and a re-protect; WriteChanged also scans differing blocks byte by byte

- **Impact (claimed):** medium · **Effort:** M · **Category:** memory-tracking · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:1225-1257,1800-1848`

**Mechanism.** Write runs FlushGpuWrites, CheckRange, then StoreOwnBytes, which calls storeOwn: tracker lock, walkWrites(Cpu) (one kernel walk), the memcpy, walkWrites(Driver) (a second kernel walk), then stamp. The memcpy write-faults the protected page: on Linux an in-kernel uffd-wp fault; on Windows an access violation, HandleWrite and VirtualProtect for armed section views. The second walk re-protects the page, which on Linux means a TLB flush to every CPU running the process. So a 4-byte label store costs about 3 kernel entries, a fault and a shootdown, under the tracker mutex, usually while the caller also holds GpuMutex. This path is used for:
- labels when the recorder is idle or the range is not imported (reason 1/4);
- deferred-label fallbacks;
- 16 Writes per ZPASS sample dump;
- COPY_DATA / DMA_DATA / WRITE_DATA fallbacks;
- write-backs (WriteChanged, WriteChangedCommitted).
WriteChanged also walks each differing 256-byte block one byte at a time and memcpy's each byte run. A fully rewritten target (e.g. 8 MiB) means ~8 M scalar iterations, and millions of tiny memcpys when channels alternate equal and changed.

**Evidence.** GuestMemory.cpp:1243-1249 (walk, store(), walk, stamp), :1837-1848 (Write), :1817-1834 (WriteChanged byte loop: `if (current[run] == original[run]) { ++run; continue; }` ... memcpy per run). GuestWriteWatch.cpp:111-119 (PM_SCAN_WP_MATCHING re-protects). WindowsMappings.hpp:134-145, 248-258. Callers: PacketMemory.cpp:39, 83, 127 (label/store fallbacks under GpuMutex at :30, 74, 112), :226-229 (dumpSampleCounters 16 Writes), DeferredLabels.cpp:120-123, VulkanDevice.cpp:1289 (`if (recorder.Idle()) return 1;`), Pm4.cpp:833-877, Texture.cpp:3700 and GuestBufferMemory.cpp:2564-2640 (WriteChanged write-backs), Graphics/src/Draw.cpp:1996.

**Why this impact.** There can be hundreds of label and store fallbacks per frame, each a few to tens of µs inside GpuMutex. Write-backs of large images add milliseconds of scalar byte scanning per image on the worker that holds the lock.

**Proposal.** 1. Store through an unwatched alias of the same backing, then MarkWritten, with no pre/post walk. On Linux, a MAP_SHARED mapping of the direct-memory memfd (GuestArenaSharedBacking_nid_postfix gives file and offset). On Windows, GuestArenaMapAlias_nid_postfix already exists. The guest view's protection state is untouched, so earlier CPU stores stay pending for the next collect and are stamped as CPU there.
2. Vectorize WriteChanged: compare 32 or 64 bytes with SIMD and turn movemask bits into runs. Use AVX-512BW masked byte stores, or AVX2 32-bit maskstore when the differing bytes align to dwords. This keeps the 'never touch unchanged bytes' rule (GuestMemory.hpp:49-51).

**Risk.** The alias mapping lifecycle must follow remaps and unmaps. WriteChanged must never write an unchanged byte (it races with CPU writes), so no 8-byte read-modify-write merges.

**How to measure.** APS5_PROFILE_DRAW=1 [guestmem]: 'write N calls ... s', 'write-changed N calls X MiB Y s', and the write callers. The [sync] report's label outcome counters (reason 1/2/4 = CPU stores). Linux: `perf stat -e page-faults,tlb:tlb_flush -p <pid>`. Windows: the exception count in ETW/WPA.

#### memory-tracking-capture-per-word-cost: Shader-resource capture does several syscalls per page and up to ~7 global-mutex acquisitions plus a linear 4096-entry scan per word on pages with pending GPU writes

- **Impact (claimed):** medium · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/ShaderMemory.cpp:97-201`

**Mechanism.** Page fetch: Accessible (describePages) then GuestMemory::Read (flush hook, CheckRange's second describePages, 4 KiB memcpy). A page overlapped by recorded GPU writes is read word by word (APS5_NO_WORDWISE_CAPTURE unset, the default). For each dword, queryPendingWrite builds a fresh PendingView whose Load() does an atomic shared_ptr load. classifyPendingWrite then takes:
- the UnitShadow registry mutex (AnyShadowedOverlaps);
- writtenBuffersMutex, with newestWriter scanning the writtenBuffers deque newest-first; the deque holds up to 4096 64-byte entries, every draw's written buffers are pushed in, and the scan runs to the end when no writer overlaps;
- labelTableMutex twice or more (WideLabelIn, plus LookupLabelValue per dword);
- the tracker mutex twice (knownValueCurrent: CollectWrites + UnchangedSince);
- writtenBuffersMutex again (dwordEvidence).
Then Accessible again and GuestMemory::Read (hook plus CheckRange). CaptureTotals counters (++reads, ++pages and others) are seq_cst global atomics bumped on every word whether profiling or not, and word-wise reads take two steady_clock::now() calls each.

**Evidence.** ShaderMemory.cpp:104 and 111 (Accessible + Read per page), :105 (pendingWrite on the page), :123 (`++CaptureTotals().reads;` unconditional), :143-144 (per-word pendingWrite + Accessible), :159, 171, 184 (GuestMemory::Read per word), :183-186 (clock reads). WriteEvidence.cpp:206-211 (new PendingView per query), :15-22 (Load), :145-204 (classification steps), :137-141 (knownValueCurrent). WrittenBuffers.cpp:45-55 (reverse linear scan), :32-43 (noteForeignWriter per draw writer). WriteEvidence.hpp:14 (WrittenBufferRing = 4096). Execution Draw.cpp:144 (ShaderMemory with queryPendingWrite per draw).

**Why this impact.** This runs on every draw-cache and dispatch-cache miss (each miss captures all stages) and on GPU-driven data (SRTs and arguments written by compute). With hundreds of captured words per stage, tens to hundreds of µs per capture go to locks and syscalls instead of analysis, on the queue worker's critical path.

**Proposal.** 1. Hold one PendingView in ShaderMemory and reload it only when Recorder::PublishGeneration() changes.
2. Classify a whole page (or run) once and reuse the answer for its dwords.
3. Replace the writtenBuffers deque scan with an interval index: std::map keyed by begin plus a max-end skip, or per-64 KiB buckets.
4. Replace Accessible + Read per page with one explicit FlushGpuWrites + CopyMapped (one describePages).
5. Gate the CaptureTotals counters, except wordsKnown, and the word-wise clock reads under CaptureProfiled().

**Risk.** Per-page classification must give the same answer as per-dword classification wherever writers, labels or evidence differ within a page. Fall back to per-dword when the page has mixed states.

**How to measure.** APS5_PROFILE_DRAW=1 [capture] line: captures, word reads, pages fetched / word-wise, words waited, 'page reads' and 'word reads' µs per capture, hook GPU waits. [draw-cache] hits and misses. The [validate] line.

#### memory-tracking-evidence-sampling-syncs: Write-evidence sampling forces a GPU sync on 1 in 16 validations, even when the evidence already proves a raw read is safe

- **Impact (claimed):** medium · **Effort:** S · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Memory/WriteEvidence.cpp:9-11,86-99,199-203`

**Mechanism.** A region overlapping recorded GPU writes may be read without waiting only if its writer is known and stable: streak ≥ APS5_WRITE_EVIDENCE_AFTER (default 4), writer range ≤ APS5_WRITE_EVIDENCE_MAX_KIB (default 16 KiB), no labels. Even then, every 16th SampledReadScope (APS5_WRITE_EVIDENCE_SAMPLE default 16; per-site counters by default) sets sampledRead, and classifyPendingWrite returns Sync. The region then goes through FlushGpuWrites, i.e. Recorder SyncThrough: the open batch is submitted and its fence waited on, on the queue worker's critical path. The synced paths also heap-allocate a 'before' copy vector per pending region.

**Evidence.** WriteEvidence.cpp:9-11 (`reads.fetch_add(1) % writeEvidenceSampleEvery() == 0`), :91-94 (default 16), :86-89 (default 4), :96-99 (16 KiB), :162-165 / 189-196 (evidence gates), :199-202 (`if (sampledRead()) { reason = syncedSample; return Policy::Sync; }`). CaptureValidation.cpp:151 (scope per validation), :177-181 and 189-206 (sync path, `before` copies at :191-203). VariantValidation.cpp:20-56 (syncPendingRuns: before vector + FlushGpuWrites per pending region). DrawCapture.cpp:33-36 (sampling scope per capture).

**Why this impact.** In GPU-driven frames (compute writing indirect args, SRTs or constants that later draws and dispatches read), each forced sample costs a full CPU-to-GPU round trip of ~0.1-several ms. A few to dozens per frame directly lengthen the frame and leave the GPU idle while the CPU waits.

**Proposal.** 1. Adaptive sampling: store a per-dword or per-writer sample interval in DwordEvidence and double it after each agreeing sample (cap e.g. 1/1024). Reset to 1/16 on a mismatch or writer change.
2. Or verify asynchronously: read raw, enqueue (address, raw bytes) as a completion action of the writing batch, and compare at reap. On mismatch, reset the evidence and invalidate the dependent draw/dispatch cache entries.
3. Reuse thread-owned buffers for the 'before' copies.

**Risk.** Fewer samples, or deferred verification, means a stale read is detected later. The exposure is the same kind as today's 15 in 16 unsampled raw reads.

**How to measure.** APS5_PROFILE_DRAW=1 [validate] line: 'synced N waiting X ms (... sampled N ...)', 'skipped the sync N', 'missed without the sync N', and the dword streak stats. APS5_TRACE_CAPSYNC=1 gives per-region detail. A/B with APS5_WRITE_EVIDENCE_SAMPLE=1000000.

#### memory-tracking-write-fault-cost: Write detection costs game threads a fault per armed page: Windows section views use an access-violation exception, Linux uses uffd-wp faults plus TLB flushes

- **Impact (claimed):** medium · **Effort:** M · **Category:** memory-tracking · **Default path:** yes
- **Location:** `core/libs/prx/libc/include/WindowsMappings.hpp:134-145,219-275`

**Mechanism.** Windows: direct memory is a section mapped as 16 KiB views. A clearing Collect re-arms each written view page and its aliases with VirtualProtect(PAGE_READONLY), one syscall per page and alias, under the global WindowsMappings::mutex while the tracker mutex is also held. The game's next store to that page raises EXCEPTION_ACCESS_VIOLATION, which the process's crash handler (ReportCrash) turns into HandleWrite: the same global mutex plus VirtualProtect back. That is user-mode exception dispatch plus 2 syscalls per first store per page per collect cycle, on game CPU threads, contending with driver collects. Linux: PAGEMAP_SCAN with PM_SCAN_WP_MATCHING re-protects written PTEs, flushing the TLB across all CPUs of the process. The next store takes an in-kernel async uffd-wp fault, which is cheaper and has no user round trip. Re-arming happens for every collected range, whether or not any snapshot will ever compare against it again.

**Evidence.** WindowsMappings.hpp:248-258 (clear: VirtualProtect per alias, `other.armed = true`), :134-145 (HandleWrite: lock mutex, VirtualProtect), :220 (Collect locks mutex), :278 (pageBytes 0x4000). libc/src/specifics/windows/CrashReport.cpp:244 (AV -> GuestArenaHandleWrite). libkernel/DirectMemory/DirectMemory.cpp:229, 271 (section views). GuestWriteWatch.cpp:111-119 (WP matching re-protect), :160-166 (UFFDIO_WRITEPROTECT for fresh ranges). GuestMemory.cpp:1100-1113 (walk under tracker lock).

**Why this impact.** A game rewriting per-frame data (constants, dynamic vertices, command buffers in tracked direct memory) pays µs to tens of µs per 16 KiB page per frame on Windows, and serializes on WindowsMappings::mutex against the driver's collects. That slows the CPU side that feeds the GPU. On Linux the cost is lower but TLB-shootdown IPIs grow with core count.

**Proposal.** 1. Re-arm only ranges that have a live snapshot consumer. Pass a 'keep watching' hint from the caches, and leave ranges whose entries were evicted unarmed until the next lookup.
2. On Windows, arm at the 64 KiB stamp granularity (4 views per fault) to cut faults up to 4×. The stamps are 64 KiB anyway.
3. Adaptively switch ranges the CPU rewrites every frame (high fault rate) to compare mode.

**Risk.** Leaving a range unarmed means its next use must treat it as changed (a full compare or upload). Coarser arming loses 4 KiB cpuPages precision, which only StoredOver uses.

**How to measure.** Windows: add an atomic HandleWrite counter to the [guestmem] line; ETW exception counts in WPA. Linux: `perf stat -e page-faults,tlb:tlb_flush -p <pid>`. [guestmem] 'collect-dirty' (walks that found written pages).

#### memory-tracking-device-gate-per-packet: DeviceUseGate takes a global std::mutex twice per PM4 packet on every queue worker

- **Impact (claimed):** low · **Effort:** S · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/DeviceAccess.cpp:23-32`

**Mechanism.** For every packet except waits and flips, PacketExecution builds a std::shared_lock on deviceReplacement. That covers SET_*_REG, draws, dispatches and labels. lock_shared locks the gate's std::mutex and checks 'replacing'. unlock_shared locks it again, decrements 'users' and calls condition_variable::notify_all whenever users reaches 0, which happens on almost every packet when one worker is active. Tens of thousands of packets per frame across 8+ workers bounce one cache line and mutex.

**Evidence.** PacketExecution.cpp:116-117 (`std::shared_lock deviceUse(deviceReplacement, std::defer_lock); if (opcode != 0x3c && ...) deviceUse.lock();` inside the per-packet loop). DeviceAccess.cpp:23-32 (lock_shared / unlock_shared with mutex + notify_all), :34-39 (writer). Driver.hpp:284 (DeviceUseGate deviceReplacement).

**Why this impact.** About 40-200 ns per packet uncontended, more when contended. At tens of thousands of packets per frame that is roughly 1-10 ms of aggregate worker time, a small but free saving.

**Proposal.** Replace it with an atomic users counter plus an atomic<bool> replacing:
- Reader: users.fetch_add(1); if replacing, undo, then wait with atomic::wait on replacing and retry.
- Writer: set replacing, then wait until users==0 with atomic::wait/notify.
std::shared_mutex would also do.

**Risk.** Low. Device replacement is rare; keep the seq_cst ordering between the flag and the counter.

**How to measure.** perf record on the queue workers (pthread_mutex_lock / futex samples inside Driver's packet loop). APS5_PROFILE_DRAW packet timing (the [packets] rows' per-packet overhead).

### Incomplete or stubbed code noticed

- `core/libs/prx/libc/src/GuestArena.cpp:109-133 (used by GuestMemory.cpp:483-506)`: The Linux guest arena is empty: the Arena() constructor body runs only under _WIN32, so GuestArenaRange reports 0 bytes on Linux. GuestMemory's page-state cache and its ForgetPages invalidator are therefore never enabled on Linux. *Performance relevance:* Every CheckRange, Accessible, CompareMapped, CopyMapped and DescribeCommitted on Linux becomes a PROCMAP_QUERY ioctl, or a /proc/self/maps parse on kernels older than 6.11 (finding memory-tracking-linux-no-page-cache).
- `docs/dev/TechnicalDebt.md:243; Graphics/src/GuestBufferMemory.cpp:286-290`: Windows host imports are always unwatched (compared). Restoring tracking on import retirement, and measuring the comparison cost, are listed as unimplemented. *Performance relevance:* Disables snapshot reuse, unit shadows, heap mirrors and known-value evidence for imported memory on Windows. Every validation becomes a full compare or copy (finding memory-tracking-windows-imports-unwatched).
- `core/libs/prx/libc/src/GuestWriteWatch.cpp:28`: Linux write watch is compiled only when the build headers provide PAGEMAP_SCAN and UFFD_FEATURE_WP_ASYNC, and nothing is printed when it is compiled out. *Performance relevance:* Silent total loss of write tracking on Linux builds against older headers (finding memory-tracking-linux-writewatch-compiled-out).
- `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:852-857,878-893`: driverPieces keeps only 4 sub-block byte ranges per 64 KiB block. Older ones fold into 'dropped', which makes StoredOver answer 'stored over' conservatively. *Performance relevance:* With labels 0x20 apart in one block, precision is lost after 4 stores, so more write-backs and compares happen.
- `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:785,895-936`: The tracker generation and block stamps are 32-bit (std::atomic<std::uint32_t>), while the API returns uint64. Every walk, MarkWritten and store increments the generation, about 1e5 per second at this load, so it wraps after hours. *Performance relevance:* Not a cost today, but after a wrap 'stamp > generation' comparisons invert and UnchangedSince could report stale data as unchanged. It is a latent correctness risk for long sessions, and any reuse fix must keep the comparisons monotonic.
- `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:1762-1785,589-614`: CompareMapped and CopyMapped (draw and dispatch-cache validation) and the Linux PROCMAP_QUERY ioctl are not timed by TimedAccess. Only the Windows VirtualQuery is counted under 'query'. *Performance relevance:* The [guestmem] profile under-reports the main per-draw validation cost and the Linux syscall cost. Instrumentation should be added before tuning.
- `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:260-276,370-383`: Draws have no unlocked 'stage A'. Dispatches split preparation (PrepareDispatch, unlocked) from stage B, but Graphics::Draw runs entirely under GpuMutex, including guest-memory checks, copies and collects. *Performance relevance:* Draw-side memory validation serializes all queues (finding memory-tracking-gpumutex-unfair-wide-hold).

### Settings to try without code changes

| Setting | Expected effect | Risk |
|---|---|---|
| `APS5_PROFILE_DRAW=1 (optionally with APS5_NO_HOLD_PROFILE=1 to reduce overhead)` | Diagnostic, not a fix. Every 10 s it prints [guestmem] (collect calls, MiB and s, memo hits, epochs, tracker waits; compare, verify, read and write totals), [lock] (per-queue GpuMutex waits, holds by site, locked GPU waits), [validate] (sampled and evidence syncs with waited ms), [capture], [hooksync] and [draw-cache]. Together these show which mechanism dominates on the user's machine. | Adds timing overhead; FPS drops somewhat while enabled. |
| `No env var: on Linux check `uname -r` (PROCMAP_QUERY needs 6.11 or later, write watch needs 6.7 or later) and stderr for '[memory] write watch unavailable' or '[write-watch] host imports ...'; on Windows look for '[write-watch] host imports are compared on Windows ...'` | Tells which tracking regime is active. On Linux, no write-watch lines at all means the feature was compiled out; a kernel older than 6.11 means a /proc/self/maps parse per range check. Upgrading the kernel or building against 6.7+ headers can give a large speedup on Linux. | None. |
| `APS5_WRITE_WATCH_IMPORTS=watch (Linux only; the default is 'probe')` | If the startup probe decided 'imported ranges are compared', this keeps host imports write-watched, restoring snapshot reuse (fast revalidation, draw-input snapshot reuse, unit shadows) for GPU-visible memory. Fewer full compares, copies and flush-hook syncs per draw. | If the driver really breaks write protection on imported pages, CPU edits can be missed (stale textures or buffers) or show up as spurious writes. Check visually for corruption. |
| `APS5_SHARED_COLLECT_MEMO=1 (default: per-thread 64-entry ring)` | Uses the tracker's shared 256-entry memo ring, keyed by thread epoch, so the graphics worker can memoize up to 4× more ranges per epoch. Fewer PAGEMAP_SCAN/GetWriteWatch walks if its working set exceeds 64 ranges. Compare [guestmem] 'collect calls' and 'collect-memo hits'. | Memo lookups move under the tracker mutex, adding contention. No correctness change. |
| `APS5_WRITE_EVIDENCE_SAMPLE=1000000 (default 16)` | Practically removes the 1-in-16 forced GPU syncs on evidence-proven raw reads of GPU-written regions. The [validate] 'sampled' count and 'synced ... waiting ms' should drop. | Stale reads of GPU-written SRTs or arguments would no longer be caught by sampling, so rendering glitches are possible if the evidence is wrong. |
| `APS5_WRITE_EVIDENCE_AFTER=1 and/or APS5_WRITE_EVIDENCE_MAX_KIB=64 (defaults 4 and 16)` | Evidence is trusted after one unchanged observation, and for writer ranges up to 64 KiB, so fewer 'evidence short' and 'larger range' syncs in [validate]. | Higher chance of reading data the pending GPU work changes, so visual glitches are possible. |
| `APS5_NO_WRITE_WATCH=1 (A/B only)` | Disables tracking entirely; everything is compared. If FPS barely changes, tracking is already ineffective (imports unwatched, compiled out, or 64 KiB false sharing), which confirms the reuse findings. If FPS drops a lot, tracking works and the focus should be on lock and syscall costs. | Slower; diagnostic only. |
| `APS5_NO_COLLECT_MEMO=1 (A/B only)` | Every CollectWrites walks. The FPS delta and the [guestmem] 'collect ... s' growth show how much walk cost the memo saves, and how much a bigger memo could still save. | Slower; diagnostic only. |

<a id="buffers"></a>
## Guest buffers, uploads, host imports, write-backs, copy/fill HLE (GuestBufferMemory, BufferPool, BdaResources, Resources, BufferCopy/BufferFill)

### How it works

Every draw or dispatch build (ShaderResources) feeds one GuestBufferMemory. Each buffer V# element becomes a region through AddWritable/AddReadable, which calls DescribeCommitted once per element. Captured constants arrive through AddSnapshot. Address-based (BDA) shaders first call AcquireRegistered, which leases every readable registered allocation (about 1200). That lease is served from a per-device cached AddressSpace, valid while the registry generation and the import epoch stay unchanged.

UploadPrepare sorts and merges the regions. It runs unlocked in a non-BDA dispatch's stage A and under GpuMutex for draws. A region binds in place through a host import when its start is aligned to minStorageBufferOffsetAlignment. Imports use VK_EXT_external_memory_host on the whole registered allocation, are made on demand, and stop at APS5_HOST_IMPORT_MIB (default 6 GiB).
- **Misaligned regions inside an import** are copied by the GPU into a pooled host-visible buffer. Read-only ones also get an extra CPU snapshot buffer.
- **Written or atomic dispatch elements of 16 KiB–2 MiB** get device-local staging shadows.
- **Everything else** is CPU-copied on every build, with a full reference copy kept for written ranges. This covers ranges that are not imported: read-only on Linux, partially uncommitted, over budget, or with a misaligned allocation base.

For BDA builds, registered ranges that cannot be imported become persistent mirrors:
- Exe image (Windows MEM_IMAGE): read-only sections are filled once; writable sections are memcmp'd in 64 KiB blocks on every build.
- Heaps: refreshed from write-watch stamps.

UploadFinish runs under GpuMutex. It reconciles and creates imports, refreshes writable mirrors (which may sync the recorder) and records the GPU copies. BdaResources builds or reuses the page table and allocates a 16 KiB fault buffer per build. MarkGpuWrites records the GPU copy-backs. CPU write-backs (WriteChanged diffs) run as batch completion actions. All buffers come from BufferPool, with one VkDeviceMemory per buffer. The copy and fill HLE replace their kernels with vkCmdCopyBuffer or vkCmdFillBuffer into imports, or with a CPU memcpy or fill. The fill's CPU path drains the whole device.

### Threads and locks

This code runs on the per-queue PM4 worker threads.
- **Draws** hold GuestMemory::GpuMutex for the whole draw (Draw.cpp:232), so all their uploads are serialized.
- **Non-BDA dispatches**: stage A (UploadPrepare, CPU copies of read-only regions) runs unlocked. Stage B (UploadFinish, import create/retire, mirror refresh, GPU copy record) runs under GpuMutex.
- **BDA builds** run entirely under GpuMutex (ShaderResources.cpp:774-779).

Lock nesting: GpuMutex → Imports().mutex. HostImportFor holds Imports().mutex while it copies a full registry lease under the GuestAllocations registry mutex. The write-tracker mutex nests below these. Separate locks: Mirrors().mutex, the BufferPool mutex, the BDA TableCache mutex and the Leases mutex. The RefreshPool helpers (3 by default) run memcmp while the caller holds GpuMutex.

GPU waits made while holding GpuMutex:
- prepareRange calls recorder->Sync().
- WaitForLeases calls FinishUpTo, whose fence waits happen under the lock.
- FillBuffer and CopyBuffer call SyncThrough.
- The fill HLE's CPU fallback calls WaitIdle (vkDeviceWaitIdle).

Completion actions (CPU WriteChanged write-backs, BDA CheckFault) run in Reap/finish under GpuMutex, on whichever thread reaps. Guest game threads enter through malloc/free/mprotect, which take the registry mutex. While any address-based work is in flight, all readable ranges are pinned, so these threads go into WaitForLeases: they take GpuMutex and wait for the GPU.

### Findings

#### buffers-copied-writers-overlisted: Copied-writer registries list every BDA dispatch and every GPU-copied-back draw, causing needless CPU syncs for fills, copies, indirect arguments and labels

- **Impact (claimed):** high · **Effort:** S · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/VulkanDevice.cpp:3317-3328`

**Mechanism.** **Dispatches:** dispatch() adds the build to state->copiedWriters whenever NeedsCompletion() is true. NeedsCompletion is `bda != nullptr || HasCopiedWrites()`, so every address-based (BDA) dispatch and every fault-buffer build is listed. That includes builds whose descriptor writes all land in host imports in place, where WriteBack stores nothing.

**Draws:** Draw.cpp computes copiedWrites = HasCopiedWrites() before keepRecordedDraw calls MarkGpuWrites, which runs RecordCopyBacks. HasCopiedWrites counts gpuCopy regions until copiedBack is set, so a draw with misaligned written V#s goes into DrawCopiedWriters even though the GPU copies its results back.

The consumers treat every listed writer as a CPU store still to come:
- FillBuffer: SyncThrough under GpuMutex.
- CopyBuffer: the CPU copy is refused, or the copy syncs first.
- DispatchIndirect: falls back to a CPU read of the arguments, which waits for the GPU through the flush hook.
- Labels: deferred behind completions as CPU stores.

WritesOverlap also matches the build's in-place (direct) ranges.

**Evidence.** VulkanDevice.cpp:3317 `if (resources.NeedsCompletion()) { ... writers->push_back(kept); }`. ShaderResources.hpp:133 `bool NeedsCompletion() const { return bda != nullptr || guestMemory.HasCopiedWrites(); }`. GuestBufferMemory.cpp:2620-2623 WriteBack only calls MarkWritten for direct regions (no store). Draw.cpp:1699 `const bool copiedWrites = completion && resources->HasCopiedWrites();` runs before Draw.cpp:1182 `resources->MarkGpuWrites(recorder);`. GuestBufferMemory.cpp:2542 `if (region.gpuCopy && (region.copiedBack || region.deviceLocal)) continue;`.

Consumers:
- VulkanDevice.cpp:1416-1432 FillBuffer: `any_of(state->copiedWriters ... WritesOverlap)` leads to `recorder.SyncThrough(address, bytes)`.
- VulkanDevice.cpp:1622-1626 / 1704-1716 CopyBuffer: completionStoreOverlaps leads to SyncThrough.
- VulkanDevice.cpp:3176-3196 indirect: cpuReason=2, then `GuestMemory::Read(arguments...)`.
- VulkanDevice.cpp:379-381 CopiedWriterOverlaps, used at 1315 to send the label to AfterCompletions (return 5).

**Why this impact.** GPU-driven work hits this every frame: compute that writes indirect arguments or buffers that are then cleared by fill kernels. Each hit is a GPU-to-CPU round trip on the queue worker, usually with GpuMutex held. That keeps the GPU queue draining and every worker waiting, which matches the symptom of an under-used CPU and GPU.

**Proposal.** 1. In VulkanDevice::dispatch, push to copiedWriters only if `resources.HasCopiedWrites()`. Keep the OnComplete for the BDA CheckFault.
2. In Draw.cpp, compute `listed` after MarkGpuWrites. Alternatively, have HasCopiedWrites treat gpuCopy regions as copied back when the use is a recorded draw, since keepRecordedDraw always calls RecordCopyBacks.
3. Add GuestBufferMemory::CopiedWritesOverlap(address, bytes), which tests only write ranges whose owner region is written back by the CPU (not direct, not a copiedBack or deviceLocal gpuCopy, not a read-only mirror). Use it in State::CopiedWriterOverlaps, FillBuffer, CopyBuffer and DispatchIndirect.
4. Optionally, merge State::copiedWriters and DrawCopiedWriters into one interval index (the tech-debt note in Draw.hpp:94-106).

**Risk.** Low. The lists exist only to order work against CPU write-backs. Direct and GPU-copied-back writes are already ordered by the recorded barriers (fill/copy ALL_COMMANDS->TRANSFER, indirect ALL_COMMANDS->DRAW_INDIRECT). The completion action that runs BDA CheckFault must stay.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [fill-sync] `N synced ... waited X ms`
- [indirect] `pending label or copied write N ... CPU argument reads took X s`
- [copy] `refused cpu: destination unsettled / source unsettled`, `waited for the source`
- [sync] `deferred behind completions N`
- [recorder] sync site table, source 4

Compare these counts before and after the change.

#### buffers-misaligned-gpu-copy: V# ranges inside host imports are copied by the GPU instead of bound in place, though per-element byte adjustments already exist

- **Impact (claimed):** high · **Effort:** M · **Category:** upload-readback · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:2030-2060`

**Mechanism.** When a region's start is not a multiple of minStorageBufferOffsetAlignment relative to its import base, UploadPrepare and UploadFinish move it to the GPU-copy path. Yet Descriptor() already handles any offset inside a direct region: it binds at `offset - adjustment` and the shader adds a per-element byte offset. That is how misaligned V#s inside an AddressSpace base region are bound today.

What the copy path costs per affected build:
- A pooled host-visible buffer for each region.
- For read-only regions, a second inputSnapshot buffer plus a CPU memcpy under GpuMutex.
- Per region: FlushKeyStoresOverlapping, FlushStoresOverlapping and StorageTexture::FlushPending.
- Two global barriers around the copies: ALL_COMMANDS|HOST->TRANSFER and TRANSFER->ALL_COMMANDS.
- For written ranges, a copy-back with two more global barriers.

The build also becomes non-reusable: DirectRegions() returns nullopt for a host-visible gpuCopy region, so noteReusable fails and every draw or dispatch rebuilds its descriptor set and upload. The changing device addresses of pooled buffers also break the BDA table cache (69% misses noted in BdaResources.cpp:26-31). This never happens on RADV (alignment 4); it does on NVIDIA (16 or more) and Intel (64).

**Evidence.** GuestBufferMemory.cpp:2038 `const bool misaligned = (region.begin - entry->base) % context.limits.minStorageBufferOffsetAlignment != 0;` and 2049 `if (misaligned || staged) {` lead to the copy path. GuestBufferMemory.cpp:2172 bind in place only `if (... % minStorageBufferOffsetAlignment == 0)`.

The adjustment mechanism already exists:
- Descriptor 2487-2497: `base = region.direct ? region.direct->base : ...; adjustment = offset % max(minStorageBufferOffsetAlignment,4); return {handle, offset - adjustment, range}`.
- ShaderResources.cpp:1096-1107 patches `allocation.adjustment` into push constants or the data byte.
- SpirvBufferAccess.cpp:63-71,98 loads memoryByteOffsets and applies `access.byteOffset`.
- GuestBufferMemory.cpp:1634-1636 returns on BaseOverlap::Inside, so V#s in BDA builds already bind in place with an adjustment.

Costs in code: 2053 Buffer per region; 2360-2367 inputSnapshot Buffer plus memcpy; 2333 and 2415 barriers; 2448 and 2474 copy-back barriers; 2705-2707 DirectRegions nullopt. The comments at 1742-1746 and Draw.cpp:1680-1682 report about 31000 such regions per run (~34 bytes each) and 0x48-byte constant buffers at the menu.

**Why this impact.** On NVIDIA and Intel this hits common small constant-buffer V#s on many draws per frame. Each one adds GPU pipeline-draining barriers, allocations and CPU copies inside the lock, and blocks reuse of the resource cache and recipes, so thousands of builds per frame stay on the slow path.

**Proposal.** 1. ShaderResources::buildPrepare knows, before UploadPrepare (ShaderResources.cpp:976), whether every GuestBuffers allocation has pushByte>=0 or dataAllocation>=0. Pass this as GuestBufferMemory::AllowAdjustedViews().
2. When it is set, UploadPrepare drops `misaligned` from the copy condition at 2049, and UploadFinish drops the alignment test at 2172. Bind as direct whenever an import serves the region and it is not staged.
3. Keep the copy only when (a) adjustments are unavailable, (b) the ViewBytes-rounded view would cross the import's end (the Require at 2494), or (c) device-local staging is chosen.

**Risk.** Medium.
- Written misaligned elements will store over PCIe in place, as aligned ones do now; dispatches can still use device-local staging.
- The adjustment must stay below 256 (the spec caps the alignment at 256).
- The view-end edge case near the end of an import needs the fallback.

**How to measure.** With APS5_PROFILE_DRAW=1, the [buffers] uploads line: `copies on the GPU: N regions (KiB) copied out of imports, M written sub-ranges copied back`. Also the [rescache] hit/miss rates and the [address] `[bda-table] hits/misses` field. With APS5_PROFILE_GPU, the [gputime] StagingIn/StagingOut classes.

#### buffers-linux-page-query-uncached: On Linux every accessibility check costs a PROCMAP_QUERY ioctl or a full /proc/self/maps parse (the page-state cache is filled only on Windows), and the buffer path makes these checks for each V# element

- **Impact (claimed):** high · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/GuestMemory.cpp:536-647`

**Mechanism.** describePages checks a per-page cache first, but only the `#ifdef _WIN32` branch ever stores into it. On Linux the cache always reads 0, so every Accessible, CheckRange and DescribeCommitted call costs either:
- one PROCMAP_QUERY ioctl per VMA run, or
- when PROCMAP_QUERY is missing (kernels older than 6.11, or APS5_NO_PROCMAP_QUERY set), opening /proc/self/maps and parsing it with ifstream and istringstream on every call.

The Linux image span is never set either. GuestBufferMemory makes these calls:
- per V# element: addDescriptorRegion calls DescribeCommitted;
- per copied range: addCopiedRange;
- per CPU copy: GuestMemory::Read calls CheckRange;
- per writable or heap mirror on every BDA build: prepareRange calls Accessible;
- per import attempt;
- per written range at write-back: storeChanged calls DescribeCommitted, and WriteChanged calls CheckRange.

The fill HLE (CheckRange) and copy HLE (Accessible ×3) add more. Each ioctl takes mmap_lock, competing with guest mmap and mprotect.

**Evidence.** GuestMemory.cpp:551-588 `#ifdef _WIN32 ... for (auto at = first; at < last; at += PageBytes) span->store(at, value);` is the only store. Linux branch 589-614: `ioctl(fd, PROCMAP_QUERY, &query)` with no store; 615-643: `std::ifstream maps("/proc/self/maps"); while (std::getline(maps, line))`. GuestMemory.cpp:488-504: the image span is set only under _WIN32.

Callers: GuestBufferMemory.cpp:1648 `GuestMemory::DescribeCommitted(begin, ...)` in addDescriptorRegion; 1570 addCopiedRange; 2299 `GuestMemory::Read` (whose CheckRange is at GuestMemory.cpp:1724); 728 prepareRange `Require(GuestMemory::Accessible(...))`; 400 importAllocation; 2562 storeChanged. BufferFill.cpp:133 `GuestMemory::CheckRange`; BufferCopy.cpp:135,156 `GuestMemory::Accessible`.

**Why this impact.** With thousands of draws, each having several V#s, this is roughly 10^4 or more queries per frame, all made under GpuMutex. With PROCMAP_QUERY each one costs a syscall plus mmap_lock. On 6.8-era kernels (Ubuntu 24.04 GA) each one regenerates and parses the whole maps file (hundreds of µs to ms), which alone can cap the frame rate. Not relevant on Windows, which caches.

**Proposal.** 1. On Linux, after a successful PROCMAP_QUERY of a readable VMA inside the arena or image span, store PageReadable|PageWritable for its pages, as GuestMemory.cpp:578-585 does on Windows. Use the same GuestAllocationsGeneration re-check and forget the pages if the generation changed. Registry mutations already invalidate through ForgetPages (GuestMemory.cpp:505), and the Linux write watch uses userfaultfd WP (GuestWriteWatch.cpp:50,164), which leaves VMA protections alone.
2. Initialize the image span on Linux from the dl_iterate_phdr extent used in GuestAllocationsRegisterMainImage.
3. In addDescriptorRegion, skip DescribeCommitted when HostImportCovers(context, begin, end - begin) is true: imported ranges are pinned and fully committed.

**Risk.** Low to medium. A protection change made outside the registry would be missed; Windows already relies on the same assumption. Keep unreadable or reserved pages uncached, as on Windows.

**How to measure.** With APS5_PROFILE_DRAW=1, the [guestmem] line: `verify N calls X MiB Y s`. Compare calls per second with draws per second. Check `uname -r`: 6.11 or newer means PROCMAP_QUERY; older means the /proc/self/maps parse path.

#### buffers-lease-pins-guest-frees: Address-based builds pin every readable range: guest free/mprotect wait for the GPU under GpuMutex

- **Impact (claimed):** high · **Effort:** M · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:971-1026`

**Mechanism.** An address-based build leases every readable registered range. The cached AddressSpace keeps that lease, and each build and recorded batch using the space holds it until its WriteBack at batch completion. The registry counts a range as pinned whenever use_count != 1. So any guest free, realloc, mprotect or munmap of any readable range, small malloc blocks included, calls WaitForLeases while address-based work is in flight.

WaitForLeases first drops the cache, which is cheap. Then it:
1. locks GpuMutex, queuing behind whole draws;
2. calls recorder->Submit() and FinishUpTo(newestLeaseSerial), whose fence waits run under the mutex, or a full Sync() if no lease serial was noted.

The next build republishes a space and pins everything again, so a guest thread can loop.

**Evidence.** GuestBufferMemory.cpp:
- 1430 `lease = GuestAllocations::GuestAllocationsAcquire_nid_postfix();`
- 1522 `built->lease = std::move(lease);`
- 2687-2688 `lease.clear(); space.reset();` happen only in WriteBack.
- 994-1009 `std::lock_guard lock(GuestMemory::GpuMutex()); ... recorder->Submit(); recorder->FinishUpTo(target);` or `recorder->Sync();`.

GuestAllocations.cpp:261 `range.use_count() != 1` means pinned; 270-274 calls the waiter; 282 prints `still pinned after %d recorder syncs`; 428-435 the lease copies every readable range. GuestHeap.cpp:232-237 GuestHeapFree calls `mutation.RequireUnpinned`. Recorder.cpp:2887-2889: FinishUpTo comment `a caller that did not wait first, WaitForLeases`.

**Why this impact.** Game threads stall on GPU completion and on the global GpuMutex each time they free memory during a frame with BDA work in flight. Meanwhile the waiter's locked fence wait blocks every PM4 worker. That directly matches the symptom of under-used CPU threads and GPU.

**Proposal.** 1. (S) Rework WaitForLeases to follow the presync pattern (VulkanDevice.cpp:3010-3031): under GpuMutex, only Submit (SubmitAndEpoch) and read the target serial; release the lock; WaitSerial(target) unlocked when HasTimeline(); then relock for FinishUpTo.
2. (M) Defer guest heap frees of pinned blocks. GuestHeapFree on a pinned range moves the block to a quarantine list, keeping the registry entry but excluding it from new leases, and returns immediately. A lease-drop or completion callback does the real Remove and rawFree. munmap and mprotect keep the synchronous wait.

**Risk.** Step 1 is low risk. Step 2 is medium risk: freed blocks are held longer (more memory), and realloc and RequireUnpinned semantics must be preserved for mprotect and munmap.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [address-sync] `N pin-contention waits by guest threads (X finished the lease batch, Y drained the recorder, Z cache-only drops) S s`
- [lock] holder matrix, untagged column (game threads)
- stderr `leased allocation ... still pinned after 8 recorder syncs`

#### buffers-generation-churn: Every guest malloc/free bumps the registry generation, invalidating the address-space cache and forcing full-lease import reconciles on labels, fills and copies

- **Impact (claimed):** medium · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:1309-1321`

**Mechanism.** Each Mutation bumps the generation unconditionally in GuestAllocationsEnd. That covers every guest malloc, free and realloc through ApplicationHeap and GuestHeap, plus mmap and mprotect. A changed generation then:
- Makes the cached AddressSpace miss (rebuiltGeneration). The next BDA build re-leases about 1200 shared_ptrs under the registry mutex (no reserve, an atomic increment each), locks Imports once per range, runs importAllocation or acquireMirror for each, and rebuilds and hashes the BDA table.
- Makes HostImportFor leave its fast path. HostImportFor is called for every GPU label, fill, copy-HLE and indirect-argument check. On a miss it acquires a full lease and runs refreshImports while holding Imports().mutex.
- Makes refreshImports walk every import and re-call GuestMemory::Unwatch on each unwatched one. On Windows all imports are unwatched, and Exclude loops over every 64 KiB block: about 98K iterations for 6 GiB imported.
- Makes HostImportSerial(reconcile=true), used by Revalidate, repeat the same work.

**Evidence.** GuestAllocations.cpp:110-112 `GuestAllocationsEnd_nid_postfix ... generation.fetch_add(1)`; 428-435 Acquire copies every range under `registry().mutex`. GuestHeap.cpp:227-273: every API opens a `GuestAllocations::Mutation`.

GuestBufferMemory.cpp:
- 1404 `else if (current->generation != generation) spaces.rebuiltGeneration...`
- 1314-1318 `if (!importsStale(...)) {...} const auto lease = GuestAllocations::GuestAllocationsAcquire_nid_postfix(); refreshImports(...)`
- 491-495 per import `if (it->second.unwatched) GuestMemory::Unwatch(...)`
- 2769 HostImportSerial reconcile.

VulkanDevice.cpp:1292 (label), 1380 (fill), 1739-1740 (copy), 3179 (indirect) call HostImportFor. WriteWatchCoverage.hpp:42-47 loops per block.

**Why this impact.** How much this costs depends on how often the game allocates. Engines that malloc and free per frame (GuestHeap.cpp:121-123 notes hundreds of MB re-allocated each frame) turn cache hits into full rebuilds on each BDA build, and put lease copies onto hot label and fill paths, all inside GpuMutex.

**Proposal.** 1. In HostImportFor, when the table is stale, validate only the import that contains the address: do one O(log N) registry lookup, via a new call returning the shared_ptr<const Range> at that base, and compare it with entry.range. Leave the full reconcile to the build paths.
2. In refreshImports, re-apply Unwatch only when the import's watch state could have changed: track a per-import flag, and on Windows check GuestArenaCommitGeneration.
3. Publish an immutable lease snapshot per generation (shared_ptr<const Lease>) so acquiring a lease costs one atomic increment. The pin check must then also count holders of the snapshot.
4. Make the AddressSpace incremental: GuestAllocationsEnd already reports the changed ranges, so patch only the regions overlapping them.

**Risk.** Steps 1 and 2 are low risk. Steps 3 and 4 are medium risk: pin semantics, and keeping the space consistent with the registry.

**How to measure.** With APS5_PROFILE_DRAW=1, the [address] line: `space hits N / rebuilds: generation M, epoch .., waiter drop ..` and `us per build: lease X, imports pass Y`. Also the [lock] 'label'/'fill' site hold times. APS5_TRACE_HOST_IMPORT=1 shows retire churn.

#### buffers-sticky-import-refusal: Import refusals are permanent per base address, and Linux never imports read-only ranges, so affected memory is CPU-copied on every build

- **Impact (claimed):** medium · **Effort:** M · **Category:** upload-readback · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:343-404`

**Mechanism.** importAllocation refuses a range and records its base in state.failed when:
- on Linux, the range is not fully accessible and writable (`Accessible(..., true)`), so read-only guest mappings (GPU-read-only direct memory, read-only image sections) are never imported;
- on Windows, any page is not MEM_COMMIT, or the range is a fully read-only private or image range.

`failed` is keyed by base address and cleared only when no range starts at that base anymore (504). So a heap registered whole and committed on demand, or a range later mprotect'ed to writable at the same base, stays un-imported for its whole life. Nothing is printed for these refusals.

Fallbacks on every use:
- Descriptor regions are CPU-copied through GuestMemory::Read, whose flush hook may SyncThrough, into a pool buffer. Written ranges also keep a full `uploaded` vector and get a CPU WriteChanged byte-diff at batch completion.
- The build becomes non-reusable and is listed as a copied writer, which feeds the syncs in buffers-copied-writers-overlisted.
- BDA builds get a heap mirror or, when pages are uncommitted, a sparse copy of every committed page on each build. Over APS5_ADDRESS_COPY_MAX_MIB=64 the process aborts.
- The fill HLE falls back to WaitIdle (see buffers-fill-cpu-fallback-waitidle).
- BDA stores into these ranges throw a fault.

**Evidence.** GuestBufferMemory.cpp:
- 344 `if (... || state.failed.contains(base)) return nullptr;`
- 399-403 Linux `if (!GuestMemory::Accessible(..., true)) { state.failed.insert(base); return nullptr; }`
- 375-378 Windows `info.State != MEM_COMMIT ... failed.insert`
- 387-390 read-only refused
- 504 `it = leasedRangeAt(lease, *it) != nullptr ? std::next(it) : state.failed.erase(it);`
- 2294-2300 copyRegion `GuestMemory::Read(...); if (written) region.uploaded.assign(...)`
- 2666-2684 CPU write-back
- 1476-1495 per-build copy and `FATAL ... APS5_ADDRESS_COPY_MAX_MIB`

BdaResources.cpp:265 `read-only in the BDA table ... not ones served by a mirror or a copy`. The arena's shared memfd backing is already available: GuestBufferMemory.cpp:264 `GuestArenaSharedBacking_nid_postfix(..., &file, &offset)`.

**Why this impact.** Wherever Astro Bot binds read-only-mapped or lazily committed GPU memory, every draw or dispatch that touches it pays a CPU copy and loses reuse. Written ranges also add completion write-backs, which gate labels and fills. The per-build sparse copies can reach 64 MiB per BDA dispatch. This becomes high impact if the profile histogram shows large 'outside imports' totals.

**Proposal.** 1. Key `failed` by Range identity (a weak_ptr, or the registry generation of that base). Retry when the Range object changes; on Windows, also retry when the commit generation changes.
2. On Linux, import read-only ranges through a read-write alias of the shared arena backing: mmap(MAP_SHARED, PROT_READ|PROT_WRITE) of the memfd offset that GuestArenaSharedBacking returns. createHostPointerImport (214) already uses entry.alias, and destroyImport must unmap it as on Windows. For non-NVIDIA drivers, try createDmaBufImport before the writable check, since udmabuf imports pages regardless of VMA protection.
3. On Windows, retry partially committed heaps after commits, or import their committed sub-ranges.

**Risk.** Medium. Alias views make read-only memory GPU-writable, but BDA permissions still grant Write only to writable ranges (2508). More pinned memory counts toward APS5_HOST_IMPORT_MIB.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [buffers] `N copied regions on the CPU: ... outside imports (MiB): 0x..:count/MiB` histogram (2279-2285)
- `[resources] guest upload of ... copied X MiB took Y ms`
- [buffers] image mirrors `heap N (X MiB)`

APS5_TRACE_BIGBUF=1 lists copies of 1 MiB or more.

#### buffers-fill-cpu-fallback-waitidle: Fill HLE CPU fallback drains the whole device (recorder Sync plus vkDeviceWaitIdle) under GpuMutex

- **Impact (claimed):** medium · **Effort:** S · **Category:** cpu-gpu-sync-stall · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Memory/BufferFill.cpp:192-202`

**Mechanism.** VulkanDevice::FillBuffer returns false when:
- no import serves the range (not one readable Range, allocation not import-aligned, a sticky failure, or over budget);
- the address is not 16-aligned;
- or the import retired during its own SyncThrough.

The fill HLE then calls localDevice->WaitIdle(), which syncs every recorded batch and runs vkDeviceWaitIdle while holding GpuMutex. It then writes the pattern in 1 MiB chunks through GuestMemory::Write, paying the flush hook, CheckRange and StoreOwnBytes on each chunk.

**Evidence.** BufferFill.cpp:
- 121 `std::lock_guard gpuLock(GuestMemory::GpuMutex());`
- 171 `const bool stored = cleared || localDevice->FillBuffer(base, bytes, pattern);`
- 192-201 `if (!stored) { ++cpuFills; localDevice->WaitIdle(); ... GuestMemory::Write(base + done, ...)`

VulkanDevice.cpp:
- 1210-1219 WaitIdle `state->recorder->Sync(); ... vkDeviceWaitIdle`
- 1380-1381 `import = HostImportFor(...); if (import == nullptr) return false;`
- 1435-1436 returns false after the import retired

**Why this impact.** Each occurrence is a full GPU drain with every worker blocked. Combined with import refusals or a budget overflow it can happen many times per frame. It is rare when all GPU memory is imported.

**Proposal.** Replace WaitIdle with targeted ordering:
- GuestMemory::Write's flush hook already makes pending writers over the range finish (SyncThrough).
- For write-after-read, call recorder->SyncThrough over the batches whose PendingReadOverlaps(base, bytes) is true when Recorder::ReadTracking() is on; otherwise call recorder->Sync() without vkDeviceWaitIdle.

Also fix the root cause by getting the range imported (see buffers-sticky-import-refusal and buffers-import-budget).

**Risk.** Low to medium. The CPU store must still follow every recorded GPU reader and writer of the range; recorder Sync() covers this, and vkDeviceWaitIdle adds only waits on work the recorder does not own.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [fill] `N buffer fills (MiB), M stored by the CPU; hold phases ms: ... cpu X`
- [recorder] sync site table: WaitIdle passes its caller to CountSync(0, return address)
- [sync] device drains by packet

#### buffers-writable-image-mirror-compare: Writable exe-image mirrors are fully memcmp'd on every address-based build, and any pending write inside them triggers a full recorder Sync (Windows)

- **Impact (claimed):** medium · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:1415-1425`

**Mechanism.** On Windows the exe image is MEM_IMAGE and cannot be imported; the comment at 42-49 puts its writable sections at about 15 MiB. On every address-based build, cache hits included, each writable image mirror goes through prepareRange over its whole range, and all of its 64 KiB blocks are compared. compareBlock runs a memcmp of guest bytes against the shadow and, if they differ, two memcpys. RefreshPool spreads this over 3 helper threads plus the caller, but the caller holds GpuMutex the whole time.

prepareRange also:
- runs FlushGpuWrites over the whole range, which costs flush-hook registry scans;
- calls recorder->Sync() (a full locked drain) if any recorded work has a pending write anywhere in the range. On Windows, labels in .data/.bss are never imported, so they become AfterCompletions CPU stores that note pending writes.

**Evidence.** GuestBufferMemory.cpp:
- 1419-1424 cache-hit path: `else if (region.mirror->writable && prepareRefresh(...)) appendBlocks(...)`
- 614-626 `std::memcmp(block.mirror->shadow.data() + offset, guest, block.length) ... memcpy x2`
- 669 `requested = text != nullptr ? std::atoi(text) : 3;`
- 733 `GuestMemory::FlushGpuWrites(address, bytes);`
- 735-738 `if (recorder->PendingWriteOverlaps(address, bytes)) { ... recorder->Sync(); }`

VulkanDevice.cpp:1292-1298: a label without an import goes to `recorder.AfterCompletions`. Recorder.cpp:2707 `noteWriteOn(batch, address, bytes.size(), true)`. On Linux the writable image is remapped anonymously and imported (GuestAllocations.cpp:73-101).

**Why this impact.** About 240 blocks (15 MiB) are compared per BDA build, roughly 0.5-1.5 ms with 4 threads, under the global lock and multiplied by the number of BDA builds per frame. Any pending label in the exe's data turns each such build into a full GPU drain. This applies to Windows hosts only.

**Proposal.** 1. (S) In prepareRange, call recorder->SyncThrough(address, bytes) instead of Sync(), so the wait covers only batches up to the newest writer of the range.
2. (M) Memoize the refresh of each writable image mirror per collect epoch (GuestMemory::BumpCollectEpoch, per packet) or per recorder submission. Skip the compare when no driver store (StoreOwnBytes/MarkWritten) hit the range since then. This approximates GPU-time reads the same way today's record-time snapshot does.
3. (M/L) On Windows, track writes to the writable image (guard pages/VEH) so the heap-mirror ChangedBlocks path can replace the memcmp.

**Risk.** Step 1 is low risk. Step 2 relaxes freshness within one packet or submission: CPU writes made concurrently during recording may be missed. That is already a race on hardware, but it should be gated behind a switch first.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [address] `us per build: ... mirror prepare X, compare Y (N blocks, M copied)`
- [buffers] image mirrors `refreshes: blocks compared, copied, refresh syncs N`
- [recorder] sync site table, source 4

#### buffers-heap-mirror-per-block: Each non-importable registered heap block becomes its own heap mirror, refreshed one by one on every BDA build

- **Impact (claimed):** medium · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:776-806`

**Mechanism.** For BDA builds, every readable registered range that is not imported becomes a mirror, and releasable ranges become heap mirrors. Guest malloc blocks are registered one by one at raw+16. Their base is therefore never aligned to minImportedHostPointerAlignment, so each block gets its own heap mirror: a Buffer with its own VkDeviceMemory, plus a generations vector.

Every BDA build, cache hit or not, then runs refreshHeapMirrors. It sorts the mirrors, groups them into contiguous runs (malloc blocks are never contiguous because of their headers) and, for each run, calls:
- prepareRange: Accessible (a syscall on Linux), the flush hook, and PendingWriteOverlaps, which can trigger a full Sync;
- CollectWrites;
- ChangedBlocks for each mirror, under the write-tracker mutex.

sweepMirrors then scans every mirror under the mirror mutex. When a Range is replaced, the mirror is rebuilt by re-reading the whole range. The heap mirror budget is 24 GiB of host-visible memory, and the process aborts past it.

**Evidence.** GuestBufferMemory.cpp:
- 1466-1475 `region.mirror = acquireMirror(context, range, blocks, range->releasable);`
- 344 alignment refusal
- 783 `while (last < mirrors.size() && mirrors[last]->base == mirrors[last - 1]->base + mirrors[last - 1]->bytes) ++last;`
- 786-792 prepareRange, CollectWrites, ChangedBlocks per mirror
- 852 `std::make_shared<Buffer>(context, range->bytes, ...)`
- 864 `GuestMemory::Read(range->address, mirror->buffer->Bytes())`
- 889-901 sweepMirrors
- 587-593 `APS5_HEAP_MIRROR_MIB ... 24576`
- 849 heapMirrorFatal

GuestHeap.cpp:207-212 `address = alignUp(raw + HeaderBytes, alignment); mutation.Add(pointer, bytes, true, true)`.

**Why this impact.** The fixed per-mirror CPU work (syscall, locks, write-watch query) multiplies with the number of registered heap blocks and with the number of BDA builds per frame, all under GpuMutex. It also uses up VkDeviceMemory allocation slots, which are limited to 4096 on NVIDIA Windows.

**Proposal.** 1. Mirror, or better import, at the granularity of the containing page-aligned arena block (GuestHeap spans are 1 MiB; large blocks are 64 KiB-granular) instead of per Range. The BDA table entries for each registered block then point into the shared span import or mirror with an offset, and permissions still come from the per-Range entries.
2. Short term: in refreshHeapMirrors, merge mirrors that share a 64 KiB block run into one hull, with one prepareRange, one CollectWrites and one ChangedBlocks per hull.

**Risk.** Medium. Spans contain unregistered bytes (headers, free blocks); the BDA permissions per Range still prevent access to them. Import identity checks (sameRange) would need span keys.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [buffers] image mirrors `N ranges (X MiB, W writable, H heap Y MiB) ... heap refills, unwatched heaps`
- [address] `mirror prepare X us` per build
- [bufferpool] large-tier misses

#### buffers-import-budget: The 6 GiB host-import budget is below a PS5 title's mapped memory, and overflow falls back to copies, mirrors, device drains and BDA faults

- **Impact (claimed):** medium · **Effort:** S · **Category:** upload-readback · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:345-365`

**Mechanism.** Imports stop at APS5_HOST_IMPORT_MIB, 6144 MiB by default. PS5 titles can map about 12.5 GiB of direct and flexible memory. Past the budget:
- descriptor regions are CPU-copied on every build;
- BDA builds get heap mirrors (host-visible, up to APS5_HEAP_MIRROR_MIB=24576, abort past it);
- the fill HLE drains the device;
- the copy HLE gives up and runs its kernel as a dispatch;
- BDA stores into these ranges throw.

The refusal message prints at most every 10 s. The live total is recomputed by iterating all imports on every new import.

**Evidence.** GuestBufferMemory.cpp:
- 348-351 `std::getenv("APS5_HOST_IMPORT_MIB") ... : 6144ull`
- 352-353 `for (const auto& [address, existing] : state.imports) live += existing.bytes;`
- 362 `[gpu] host import budget: ... refused so far`
- 447-448 a second live loop

BufferCopy.cpp:164 `if (outcome.path == CopyNotImported || ...) return false;`. BdaResources.cpp:265 `past APS5_HOST_IMPORT_MIB`.

**Why this impact.** If Astro Bot's registered GPU memory exceeds 6 GiB, every later-imported heap falls onto the slowest paths, which feeds most of the other findings. On a host with enough RAM, raising the budget is a free fix.

**Proposal.** 1. Default the budget from installed RAM, for example min(physical RAM − 8 GiB, 16 GiB).
2. Keep a running liveBytes in HostImports, updated by importAllocation, retireImport, refreshImports and ClearHostImports.
3. Print the first refusal immediately together with the variable to raise.

**Risk.** Low. Pinned memory reduces host RAM headroom. On Windows, imports may then need APS5_WORKING_SET_MIB (VulkanDevice.cpp:864-870).

**How to measure.** Look for the stderr line `[gpu] host import budget: X MiB live of Y MiB (APS5_HOST_IMPORT_MIB); N imports (M MiB) refused`. With APS5_TRACE_HOST_IMPORT=1, each import prints `ok (N live, X MiB live, Y MiB ever)`.

#### buffers-pool-dedicated-allocations: Each transient per-build buffer has its own VkDeviceMemory, and pool slots are capped at maxMemoryAllocationCount/6

- **Impact (claimed):** medium · **Effort:** M · **Category:** allocation-copy · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Resources.cpp:10-58`

**Mechanism.** Every Buffer is its own VkBuffer plus VkDeviceMemory, with vkMapMemory and vkGetBufferDeviceAddress on top. BufferPool only recycles whole allocations, keyed by (capacity, usage, properties). Each build takes:
- a data buffer per ShaderData/FlattenedSrt binding;
- a buffer per CPU-copied region;
- a buffer per GPU-copied region, plus an inputSnapshot for read-only ones;
- a 16 KiB+ fault buffer per BDA or rect-list build, and a BDA table on a cache miss.

Pool slots per tier are min(4096, maxMemoryAllocationCount/6). On NVIDIA Windows (maxMemoryAllocationCount 4096) that is 682 slots per tier, so thousands of transient buffers per frame cause misses: a vkAllocateMemory/vkFreeMemory per buffer (tens of µs). Every eviction runs a linear min_element over all keys under the pool mutex. Varying usage bits (DEVICE_ADDRESS, TRANSFER_DST) split the keys further.

**Evidence.** Resources.cpp:
- 29 `Check(... vkCreateBuffer ...)`
- 49 `vkAllocateMemory buffer`
- 52 `vkMapMemory`

BufferPool.cpp:
- 21 `maxSlots(std::max<std::size_t>(1, std::min<std::size_t>(MaxSlots(), context.limits.maxMemoryAllocationCount / 6u)))`
- 91 `std::min_element(tier.free.begin(), tier.free.end(), ...)`
- 117-122 eviction loop under the mutex

Per-build allocations: ShaderResources.cpp:2316 `std::make_unique<Buffer>(context, size, ...)` (data); GuestBufferMemory.cpp:2289, 2053, 2348, 2361; BdaResources.cpp:126-127 fault buffer plus memset, 195 table.

**Why this impact.** Allocation churn adds tens of µs per miss to every build, all under GpuMutex for draws. It is worst on NVIDIA Windows, where the allocation-count cap shrinks the pool.

**Proposal.** Add a per-batch linear sub-allocator:
- A few large, persistently mapped host-visible buffers (HOST_CACHED where the CPU reads back) with all usage bits set (STORAGE|TRANSFER_SRC|TRANSFER_DST|SHADER_DEVICE_ADDRESS).
- Hand out (VkBuffer, offset) slices aligned to minStorageBufferOffsetAlignment.
- Recycle a batch's slices all at once when it completes, via recorder.Keep, or after a CommandBatch wait.
- Use it for data buffers of non-reusable builds, copy regions, input snapshots and fault buffers. Keep BufferPool for long-lived objects (mirrors, tables, cached templates).

Buffer/Descriptor would need an offset member; Descriptor already returns offsets.

**Risk.** Medium. Slice lifetimes must follow batch completion exactly. Reusable cached templates must not keep per-batch slices.

**How to measure.** With APS5_PROFILE_DRAW=1:
- [bufferpool] `small/large/device: hits, misses, evictions, retained`
- [buffers] uploads line `buffer allocation X ms`
- [resources] phase totals

#### buffers-windows-imports-unwatched: On Windows every host import is excluded from the write watch, so surface and snapshot validation in GPU memory falls back to byte compares

- **Impact (claimed):** medium · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:284-291`

**Mechanism.** On Windows, decideImportWatch always sets unwatchImports=true; APS5_WRITE_WATCH_IMPORTS and the write-protection probe exist only on Linux. Every import is therefore unwatched, so CollectWrites/UnchangedSince cannot vouch for ranges inside imports, which is nearly all GPU memory.

As a result, storage-image refresh, texture validation and snapshot reuse fall back to EqualsCommitted, a memcmp of the whole surface against its saved copy. refreshImports also re-runs Unwatch on every import whenever the generation changes, which walks every 64 KiB block.

**Evidence.** GuestBufferMemory.cpp:
- 286-290 `#ifdef _WIN32 ... state.unwatchImports = true; ... "host imports are compared on Windows ..."`
- 1215-1218 `ProbeImportWriteProtection ... probe.failure = "the Linux write watch";`
- 412-413 `if (entry.unwatched) GuestMemory::Unwatch(base, bytes);`
- 494

Texture.cpp:1229 `GuestMemory::CollectWrites(...)` and 1250 `return GuestMemory::EqualsCommitted(descriptor.baseAddress, original);`. ShaderResources.cpp:365-412 the same pattern for draw snapshots. GuestMemory.cpp:1146-1147 the Windows Unwatch uses `coverage.Exclude`.

**Why this impact.** On Windows, every texture or storage-image check in imported memory becomes a memcmp of the surface (MiB per render target) instead of a stamp check. This adds CPU time inside the lock on texture-heavy frames. Not relevant on Linux, where the probe usually keeps imports watched.

**Proposal.** 1. Port ProbeImportWriteProtection to Windows: create a MEM_WRITE_WATCH scratch region, import it, check GetWriteWatch after a GPU read, and keep imports watched when no driver write lands after the import window. The ImportWindow stamp kind already exists for this.
2. Let APS5_WRITE_WATCH_IMPORTS=watch work on Windows for A/B testing.
3. Skip re-Unwatch of imports that are already unwatched.

**Risk.** Medium to high. A wrong probe result means stale textures. Keep the compare as the fallback.

**How to measure.** Look for the startup line `[write-watch] host imports are compared on Windows ...`. With APS5_PROFILE_DRAW=1, check the [guestmem] `compare N calls X MiB Y s` and the [texture] lookup outcomes.

#### buffers-fault-buffer-blocks-reuse: BDA and rect-list builds get a per-build 16 KiB fault buffer, which makes them never reusable

- **Impact (claimed):** medium · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/BdaResources.cpp:123-128`

**Mechanism.** Every build with a FaultBuffer binding gets a BdaResources object. That is every BDA build and every rect-list draw (the TessellationControl stage has a fault buffer and no table). Each one:
- allocates a buffer of FaultBufferBytes = (9+4096)*4 = 16420 bytes, which lands in the 32 KiB pool class;
- memsets it on every build;
- scans 4096 words at completion in markWrittenPages.

Because bda is non-null, NeedsCompletion() is true, so noteReusable bails out and the build is never cached. Every rect-list draw (fullscreen and post passes) therefore rebuilds its descriptor set and upload and registers an OnComplete.

**Evidence.** BdaResources.cpp:
- 126-127 `fault = std::make_unique<Buffer>(context, FaultBufferBytes, ...); std::memset(fault->Bytes().data(), 0, ...)`
- 281-285 `for (std::uint32_t slot = 0; slot < Abi::WrittenPageSlots; ++slot)`

BdaAbi.hpp:49-53 `WrittenPageSlots = 4096; FaultBufferBytes = (WrittenSlotsWord + WrittenPageSlots) * 4`. ShaderAddressResources.cpp:18 `rectListFault = shader.stage == TessellationControl && tables == 0 && faults == 1`. ShaderResources.cpp:1050 `else if (usesFaultBuffer) bda = std::make_unique<BdaResources>(context);` and 1161 `if (NeedsCompletion() || HoldsLease()) return;`. Draw.cpp:1677-1679 comment: every rect-list draw needs one.

**Why this impact.** Post-processing and blit passes cannot use the resource cache or draw recipes. Each one pays a full build (descriptor alloc and update, uploads) plus completion work, under GpuMutex.

**Proposal.** 1. Separate the per-use fault buffer from the cached template: a build whose only completion work is the fault check stays reusable, and each use gets a fresh fault slice (from the batch sub-allocator) bound in a small second descriptor set.
2. Alternatively, make the rect-list validity check a debug option, since it only diagnoses InvalidRectangle.
3. Separately, memset only the 9 header words, and have the shader keep a slot count so markWrittenPages scans only the used slots.

**Risk.** Low to medium. Making the check optional loses the InvalidRectangle and loop-guard diagnostics; the second-set approach needs pipeline layout changes.

**How to measure.** With APS5_PROFILE_DRAW=1: the [draw] per-kind averages (build vs template hit vs BDA), the [rescache] hits and misses, and the number of draws reported as `recorded with completion`.

#### buffers-input-snapshot-double-copy: Read-only GPU-copied regions get a CPU memcpy into an extra buffer plus a GPU copy into another host-visible buffer

- **Impact (claimed):** low · **Effort:** S · **Category:** allocation-copy · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:2352-2375`

**Mechanism.** For a gpuCopy region with no pending writer that is not written, recordGpuCopies:
1. allocates an inputSnapshot Buffer;
2. memcpy's the guest bytes into it on the CPU;
3. keeps it in the batch;
4. records vkCmdCopyBuffer from inputSnapshot to region->buffer.

But region->buffer is itself a mapped host-visible buffer (copyBufferProperties(false)), so the CPU could fill it directly. The two global barriers around the copies are recorded either way.

**Evidence.** GuestBufferMemory.cpp:
- 2360-2365 `if (!pendingWriter && !writable) { inputSnapshot = std::make_shared<Buffer>(...); std::memcpy(inputSnapshot->Bytes().data(), reinterpret_cast<const void*>(address), bytes); copySource = inputSnapshot->Handle(); ... recorder->Keep(inputSnapshot);`
- 2375 `CopyBuffer(context, commands, copySource, copyOffset, region->buffer->Handle(), 0, bytes);`
- 2053 the region buffer is created with `copyBufferProperties(false)`
- 1820-1822 `HOST_VISIBLE | HOST_COHERENT`
- 2333 and 2415 barriers

**Why this impact.** Avoids one pool allocation, one GPU transfer and possibly both barriers for each read-only misaligned constant buffer. This is a cheap stop-gap if buffers-misaligned-gpu-copy is not adopted.

**Proposal.** When `!pendingWriter && !writable && region->buffer->Mapped()`, memcpy straight into region->buffer->Bytes() and skip CopyBuffer. Record the leading and trailing barriers only if at least one region in `copies` still needs a GPU copy; a host write before submission is visible without a barrier. Keep the CaptureTrace log.

**Risk.** Low. The CPU read already happens today, only into a different buffer.

**How to measure.** With APS5_PROFILE_DRAW=1, the [buffers] uploads line `copies on the GPU: N regions`. With APS5_PROFILE_GPU, [gputime] StagingIn. With APS5_CAPTURE_TRACE, `copy-input-snapshot` entries.

### Incomplete or stubbed code noticed

- `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:1213-1218,1246-1251`: ProbeImportWriteProtection and ProbeDmaBufImportWriteProtection are stubs on Windows (they return failure 'the Linux write watch'), and decideImportWatch hard-codes unwatchImports=true there (286-290). *Performance relevance:* All Windows host imports are compared rather than watched, so texture and snapshot validation in GPU memory falls back to memcmp (see buffers-windows-imports-unwatched).
- `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:1102`: 'a guest buffer off the storage buffer offset alignment in a shader without shader data is not implemented': per-element adjustments need push constants or a shader-data buffer. *Performance relevance:* This is the only real limit on binding misaligned import sub-ranges in place. Today all of them are copied (buffers-misaligned-gpu-copy).
- `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:2490`: Require: 'a guest buffer view in a GPU owner that does not start at a DWORD boundary is not implemented'. *Performance relevance:* Forces regions to be rounded to dwords (addDescriptorRegion 1632-1633) and constrains in-place binding.
- `core/libs/prx/libSceAgcDriver/Graphics/src/BdaResources.cpp:279`: 'more than 4096 pages stored to through the BDA table in one use are not implemented': the written-page log is fixed at 4096 slots. *Performance relevance:* Fixes the fault buffer at 16 KiB per build, memset and scanned every time (buffers-fault-buffer-blocks-reuse).
- `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:2567-2572`: storeChanged throws 'aliased writes are not implemented' when the GPU changed bytes in a non-writable direct-memory alias. *Performance relevance:* Not a performance stub, but it shows write-backs must re-query commitment and writability (DescribeCommitted) for every range.
- `core/libs/prx/libSceAgcDriver/Graphics/include/Draw.hpp:94-106`: Tech debt: two copied-writer registries (VulkanDevice State::copiedWriters and DrawCopiedWriters) that fill, copy, indirect and label code scan linearly. The comment is stale: it still describes opt-in APS5_RECORD_COPIED_DRAWS, but Draw.cpp:1697 now lists by default (APS5_NO_RECORD_COPIED_DRAWS). *Performance relevance:* Linear scans plus over-listing cause unnecessary GPU syncs (buffers-copied-writers-overlisted).
- `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:1487-1495,849`: Hard aborts instead of degradation: an address-based build copying more than APS5_ADDRESS_COPY_MAX_MIB (64) aborts with FATAL, as does going past the heap mirror budget (APS5_HEAP_MIRROR_MIB=24576). *Performance relevance:* The sparse per-build copy path, which can copy up to 64 MiB per BDA dispatch, is reachable on the default path when imports are refused.
- `core/libs/prx/libSceAgcDriver/Graphics/src/GuestBufferMemory.cpp:1010-1014`: WaitForLeases falls back to std::this_thread::yield() when nothing recorded holds the lease, and the guest thread spins until APS5_PIN_WAIT_MS (60000 ms). *Performance relevance:* Guest threads can busy-spin in free() while a build in progress holds a lease.

### Settings to try without code changes

| Setting | Expected effect | Risk |
|---|---|---|
| `APS5_PROFILE_DRAW=1` | Measurement only. Prints every 10 s: [address], [address-sync], [buffers] (uploads, CPU-copied regions by granule, image mirrors, staging), [bufferpool], [guestmem] (verify/compare/collect calls and seconds), [fill], [fill-sync], [copy], [indirect], [sync] and [lock]. Together these show which findings dominate. | Some timing overhead, and stderr volume. |
| `APS5_TRACE_HOST_IMPORT=1` | Prints each import, failure (with /proc/self/maps or VirtualQuery detail) and retire, with live and 'ever' MiB. Fast growth of 'MiB ever' shows import churn; failures show which ranges fall back to copies. | Log volume only. |
| `APS5_HOST_IMPORT_MIB=12288` | If stderr shows '[gpu] host import budget: ... refused', more guest memory is imported in place. That removes per-build CPU copies, heap mirrors, fill-HLE WaitIdle drains and BDA write faults for those ranges. | Pins up to 12 GiB of host RAM; needs at least 32 GB of RAM. On Windows this may also need APS5_WORKING_SET_MIB. |
| `APS5_WORKING_SET_MIB=16384 (Windows only)` | If imports fail with '[gpu] host import of ... failed at vkAllocateMemory', raising the working-set minimum lets the driver lock the pages, so ranges are imported instead of copied. | A large locked working set reduces memory available to other processes. |
| `APS5_MIRROR_REFRESH_THREADS=7` | On Windows with 8 or more cores, the per-BDA-build writable image mirror compare (about 15 MiB) finishes sooner, which shortens GpuMutex holds. The value is clamped to 16; the default is 3. | The helper threads compete with game threads for cores, and they help only when at least 8 blocks are compared. |
| `APS5_BDA_TABLE_CACHE_ENTRIES=32` | Useful if the [address] line shows many [bda-table] misses with 'hash differs low/heap'. More tables are kept, so fewer ~38 KiB tables are rebuilt and hashed per BDA build. | About 38 KiB of host-visible memory per retained table; tables are held weakly. |
| `APS5_NO_SNAPSHOT_CHECK=1` | Skips the memcmp of each captured snapshot against live import- or mirror-backed memory in AddSnapshot. Saves CPU per BDA build; the snapshots time appears in the [address] line. | Loses only the consistency diagnostic. The region serves the GPU from live memory either way. |
| `APS5_STAGING_POOL_MIB=1024` | If [bufferpool] shows device-tier evictions or misses, more device-local staging shadows and detiler scratch buffers are kept, so fewer vkAllocateMemory calls happen per dispatch. | Up to 1 GiB of VRAM retained. |
| `APS5_WRITE_WATCH_IMPORTS=watch (Linux only)` | If the startup line says imports 'are compared', this keeps them write-watched, so texture and snapshot validation uses stamps instead of full memcmp. | The probe found driver writes after the import window, so texture data may be stale. Use for A/B testing only. |
| `Linux kernel >= 6.11 with APS5_NO_PROCMAP_QUERY unset` | Avoids the /proc/self/maps open-and-parse path that every guest-memory accessibility check takes on older kernels (there are thousands per frame from the buffer path). Do not set APS5_NO_PROCMAP_QUERY. | None. The kernel upgrade is the only cost. |

<a id="shader-resources"></a>
## Shader resource binding: descriptors, samplers, validation (ShaderResources, DescriptorCache, ResourceCache, texture/sampler caches, ShaderValidation memo, bindless tables)

### How it works

Per draw, Graphics::Draw (under GuestMemory::GpuMutex, taken by Driver::draw lockForDraw) gets fragment outputs from CachedFragmentOutputs (ValidateShaders memoized by variant/state key; recipe hits skip it). resolveDrawResources then, for recordable draws whose stages have variant ids, builds DrawResourceKey (marker + device + each stage's ShaderResources::ContentKey: binding headers and every T#/S#/written-V# word; read-only V# bases and ShaderData/FlattenedSrt words are masked by moved-buffer templates) and probes the process-wide ResourceCache (LRU of 1024 entries shared with dispatches). A hit is proven by Revalidate (fastRevalidate: per-element CollectWrites and DCC key proofs, one ScanPending, UnchangedSinceAll, StorageImagesCached; else a full re-lookup walk) plus MovedReadOnlyBuffers. A miss constructs ShaderResources: buildPrepare (binding plan, SamplerCache::Get per S#, one pooled host Buffer per data binding, precollectImages = per image element decode + DescribeSurface + CollectWrites + DCC key scan + texture-cache probe, GuestBufferMemory::UploadPrepare, DescriptorCache::Layout and Allocate) and buildComplete (fastTexture or cachedTexture/cachedStorageTexture per element, UploadFinish, one vkUpdateDescriptorSets for the whole set, noteReusable). DrawWithRecipe skips key/Find but runs ProveCurrent. Every recorded draw then runs PrepareDrawBindings: each read-only guest buffer read in place is memcpy'd into a host Buffer (unless the recorder's snapshot is reusable), moved data words get new Buffers, a second descriptor set is allocated from the pool chain, every binding is VkCopyDescriptorSet'd from the template and the snapshot elements rewritten, and that set is bound. Dispatches reuse the same objects but run key/Find/stage A/presync off the lock and refresh template data via vkCmdUpdateBuffer.

### Threads and locks

One worker thread per AGC queue interprets PM4. A draw takes GpuMutex (process-wide, recursive) in Execution/src/Driver/Draw/Draw.cpp lockForDraw before VulkanDevice::Draw and holds it through key build, Find, Revalidate, the complete two-stage build, PrepareDrawBindings and command recording, so draws of all queues serialize. Dispatches run key/Find/stage A/precollect/presync unlocked in VulkanDevice::PrepareDispatch and lock only for stage B. Nested under GpuMutex this subsystem takes: ResourceCache::mutex (Find/Insert/Touch; the draw-path Insert destroys LRU victims inside it), DescriptorCache::mutex (layout map, every Allocate incl. failing vkAllocateDescriptorSets on full pools, every Free from the release thread), SamplerCache::mutex, TextureCache::mutex (held across snapshot reads, flush-hook waits and Texture creation), StorageTextureCache::mutex (Refresh under it), the GuestMemory write-tracker mutex (collect-memo misses, UnchangedSince), the pending-image registry, BufferPool::mutex (each data/snapshot Buffer take and put) and validationMutex. Unlocked dispatch stage A on other threads contends on the same self-locking caches; flush-hook waits for recorded GPU work inside lookups stall every queue while holding GpuMutex.

### Findings

#### shader-resources-draw-build-under-gpumutex: Draw resource resolution (key, Find, Revalidate, full two-stage build, no presync) runs entirely under GpuMutex, unlike dispatches

- **Impact (claimed):** critical · **Effort:** L · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Execution/src/Driver/Draw/Draw.cpp:260-383`

**Mechanism.** Driver::draw locks GpuMutex (lockForDraw) before VulkanDevice::Draw; Graphics::Draw -> resolveDrawResources then builds the content key, probes the shared cache, revalidates, or constructs ShaderResources whose draw constructor runs build() = buildPrepare + buildComplete. So stage A (binding plan, sampler cache, data Buffer allocation, precollectImages: decode + DescribeSurface + write-watch CollectWrites walk + DCC key scan + texture-cache probe per image element, UploadPrepare, layout/set allocation) runs under the process-wide lock, and there is no unlocked presync, so any flush-hook wait inside a draw's texture/buffer lookups (EqualsCommitted, ReadTextureSurface, FlushPending) blocks every queue while the GPU drains. The dispatch path already has the machinery to avoid all of this.

**Evidence.** Execution/src/Driver/Draw/Draw.cpp:42 'std::unique_lock gpuLock(GuestMemory::GpuMutex(), std::defer_lock)'; :260-275 lockForDraw locks it; :370 'lockForDraw();' then :383 'localDevice->Draw(graphics, drawParameters, stages, snapshots, ...)' (also :343/:346 for CPU-read indirect records). Graphics/src/Draw.cpp:1615 resolveDrawResources -> :968-970 DrawResourceKey + SharedResourceCache().Find + Revalidate, :992 'std::make_shared<ShaderResources>(context, shaders, state.color, ...)'. ShaderResources.cpp:745-748 draw ctor = prepareAddressBindings + build(); :804-807 build = buildPrepare + buildComplete. Contrast: ShaderResources.cpp:762-783 deferred compute ctor runs only stage A ('unlockedPrepare = true'); Execution/src/VulkanDevice.cpp:3040-3092 PrepareDispatch does key (:3061), Find (:3062), stage A (:3068), PrecollectSurfaces (:3072) and the unlocked presync (:3085-3089) before the mutex. ShaderResources.cpp:2477-2481: the CollectWrites walk 'is most of the lookups' time under the lock'. PresyncSurfaces/PrecollectSurfaces are only called from VulkanDevice.cpp (3072, 3087, 3117, 3128, 3609), never from the draw path. The only draw lock switch, APS5_LOCKED_DRAW_PREPARE (Execution Draw.cpp:41), is opt-in and locks earlier.

**Why this impact.** Draws are the bulk of a frame (thousands vs hundreds of dispatches). Holding the single GpuMutex across per-draw resource work serializes all queue workers onto one core and turns every producer wait into a whole-process stall, which is exactly the observed under-use of both host threads and the GPU.

**Proposal.** Mirror PreparedDispatch for draws. Add a deferred draw constructor ShaderResources(context, shaders, target, indexAddress, indexBytes, snapshots, deferred) following the compute one (lockedBuild when any stage has a BdaPagetable binding). In Execution/src/Driver/Draw/Draw.cpp before lockForDraw (line 370): compute DrawResourceKey and SharedResourceCache().Find (self-locking); if found call PrecollectSurfaces on it, else run stage A unlocked; then call PresyncSurfaces() and VulkanDevice::presync() so producer batches are waited for without the lock (as PrepareDispatch :3085-3089). Carry a PreparedDraw {key, cached, resources, presyncSerial} into VulkanDevice::Draw -> Graphics::Draw -> resolveDrawResources, which under the lock only does ReapRecorded(presyncSerial), Revalidate (hit) or Complete() (miss). For DrawWithRecipe do the template's PrecollectSurfaces + presync before locking. Discard the prepared object if lockForDraw detects a replaced device (Draw.cpp:268-272); a non-recordable draw can still Complete() it as a normal build.

**Risk.** Stage A already runs unlocked for dispatches, so GuestBufferMemory::UploadPrepare and the sampler/descriptor/texture caches are exercised thread-safely today; draw-specific inputs (alias checks against target/index) are pure. Hazards: device replacement between prepare and lock; a found cache object invalidated before the lock (Revalidate under the lock stays authoritative; use pointer-conditional Remove as dispatch does); address-based (BDA) draws must stay fully locked.

**How to measure.** APS5_PROFILE_DRAW=1: [lock] lines (GuestMemory.cpp:1644-1645) per site acquisitions/waits and waited ms for the Draw site vs Dispatch; [draws] phase totals lookup= and resources= (bindings/upload/descriptors/other) and 'hook waits inside draws'; [resources] phase totals (stage A, precollect, images). After the change the draw stage-A time should move out of the lock like the dispatch 'resources: A unlocked' row.

#### shader-resources-per-draw-descriptor-set: Every recorded draw allocates a second descriptor set, copies the whole set and memcpy-snapshots read-only buffers (PrepareDrawBindings), even on cache and recipe hits

- **Impact (claimed):** high · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2775-2859`

**Mechanism.** Because draws are batched and the guest may rewrite constant memory before the batch runs, each read-only guest buffer read in place (and each moved buffer / moved data-word block) is copied into a fresh host Buffer, then a new descriptor set is allocated from the global pool chain, the template set is copied binding by binding (including every image and sampler array) and the snapshot elements are rewritten. The template's own set (fully written by buildComplete) is not bound when this happens. The set is freed on the release thread when the batch completes.

**Evidence.** Draw.cpp:1362 'const auto drawBindings = resources.PrepareDrawBindings(*recorder, record.moved);' in recordDraw, used by the normal path (Draw.cpp:1765) and DrawWithRecipe (Draw.cpp:2124). ShaderResources.cpp:2780-2816: per selected allocation 'recorder.ReusableDrawSnapshot(begin, bytes)' else 'std::make_shared<Buffer>(context, bytes, ...)' + 'std::memcpy(..., begin, bytes)' with no size cap (Recorder only retains <=256 MiB, Recorder.hpp:94); moved data words: make_shared<Buffer> + memcpy (:2784-2788). :2817 returns {} only when nothing was selected. :2818-2856: std::map counts, sizes vector, DescriptorCache::Allocate (mutexed pool chain), vkUpdateDescriptorSets with one VkCopyDescriptorSet per binding, a second vkUpdateDescriptorSets with per-element writes, recorder.Keep(result). Draw.cpp:1411-1416 binds this set instead of the template's _set, so a miss writes the whole set (ShaderResources.cpp:1052-1094) and then copies it. ~DrawBindings (:2723-2725) -> DescriptorCache::Free (mutex + vkFreeDescriptorSets). The dispatch path avoids per-use sets with vkCmdUpdateBuffer into the template's data buffers (RefreshData :2357-2398), which cannot be recorded inside the render pass a draw continues.

**Why this impact.** Nearly every PS5 draw reads constants or structured data through read-only V#s, so this runs for almost every draw: at ~3000 draws/frame that is ~3000 set allocate/free pairs, ~6000 vkUpdateDescriptorSets calls, tens of thousands of descriptor copies, ~8 heap allocations and one or more BufferPool take/put (global mutex) per draw, all under GpuMutex. It turns template and recipe hits, designed to be nearly free, back into a per-draw descriptor build.

**Proposal.** Stage 1: persist the VkDescriptorBufferInfo/VkDescriptorImageInfo arrays built in buildComplete; create one VkDescriptorUpdateTemplate per cached set layout (store it in DescriptorCache next to the layout); PrepareDrawBindings patches a thread_local copy of the infos for the selected elements and issues a single vkUpdateDescriptorSetWithTemplate (no VkCopyDescriptorSet, no std::map/vectors). Stage 2: give the Recorder per-batch transient descriptor pools (no FREE bit, vkResetDescriptorPool when the batch fence signals) and a per-batch linear upload arena (persistently mapped, bump-allocated at minStorageBufferOffsetAlignment) for snapshots and moved data words, replacing make_shared<Buffer>/BufferPool and DescriptorCache::Allocate/Free per draw. Stage 3: declare ShaderData/FlattenedSrt (and snapshotted read-only elements while within maxDescriptorSetStorageBuffersDynamic) as VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC into the arena, so a template or recipe hit binds the cached set with new dynamic offsets and allocates/updates nothing (SPIR-V is identical for dynamic storage buffers); or VK_KHR_push_descriptor for sets within maxPushDescriptors.

**Risk.** Arena and transient-pool memory must outlive the batch (tie them to the Recorder batch as recorder.Keep does). Dynamic-offset layouts change the pipeline layout key (pipelines rebuilt once). Template infos stay valid for the object's lifetime because Revalidate never swaps objects (it fails instead).

**How to measure.** APS5_PROFILE_DRAW=1: [draws] record= phase (PrepareDrawBindings runs in recordDraw); [descriptors] line (ShaderResources.cpp:1189) sets/pools growth per 10 s; [bufferpool] small-tier hits/misses; [lock] Draw-site hold. Add counters for PrepareDrawBindings sets and snapshot bytes memcpy'd per 10 s (none exist).

#### shader-resources-rescache-lru-capacity: Shared resource-cache LRU of 1024 entries (dispatch + draw) is likely smaller than one gameplay frame's distinct draw contents, giving cyclic thrash and near-zero template hits

- **Impact (claimed):** high · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2029-2051`

**Mechanism.** Insert adds at the front and pops from the back while entries.size() > capacity. With capacity 1024 shared by dispatches and draws, a frame touching more distinct (variant, T#/S#/written-V#) contents than that in roughly the same order every frame evicts each entry before its next-frame reuse; every draw then pays the full build plus an eviction.

**Evidence.** ShaderResources.cpp:2041-2045 'const auto parsed = value ? std::strtoull(value, nullptr, 10) : 1024ull;'; :2046-2050 eviction loop; ShaderResources.hpp:462-463 'One LRU bound covers dispatch and draw entries together (APS5_RESOURCE_CACHE_ENTRIES, default 1024)'; the sizing comment at :2038-2040 ('covers several frames of distinct dispatch and draw content') came from the menu stage (Draw.cpp:575-577). Draw.cpp:1719 inserts every reusable recorded draw build; VulkanDevice.cpp:3396-3442 inserts dispatch builds into the same SharedResourceCache. noteMiss (ShaderResources.cpp:1987-1988) counts misses whose key equals the variant's previous key as 'same key (not inserted or evicted)'.

**Why this impact.** A template hit costs stamp proofs; a miss costs dozens of texture lookups, data-buffer allocations, descriptor writes and a set allocation under GpuMutex. With thousands of draws per frame in a dense gameplay scene, an LRU smaller than the per-frame working set collapses the hit rate exactly where it matters.

**Proposal.** Raise the default (e.g. 8192) and bound by pinned bytes (sum of GuestBytes of textures/storage images each entry holds) rather than entry count; partition draw and dispatch entries so one cannot flush the other; use a scan-resistant policy (2Q / segmented LRU: insert into probation, promote on second hit) so one-shot contents do not evict hot templates.

**Risk.** Entries pin textures/storage images past the 2 GiB texture-cache budgets and hold one descriptor set each, so a bigger cache raises RAM/VRAM and descriptor-pool memory; correctness is unaffected because every hit is still proved by Revalidate.

**How to measure.** APS5_PROFILE_DRAW=1: '[rescache] draws: H hits, M misses, I invalidated, U uncacheable' (Draw.cpp:413), '[rescache] miss churn (draws) ... same key S (not inserted or evicted)' (ShaderResources.cpp:2021; a large S means LRU eviction) and the VulkanDevice '[rescache] ... N entries' line pinned at 1024. A/B with APS5_RESOURCE_CACHE_ENTRIES=8192.

#### shader-resources-linux-write-watch: On Linux all stamp-based validation depends on userfaultfd WP_ASYNC + PAGEMAP_SCAN (kernel >= 6.7 and matching build headers); without it every texture and snapshot check degrades to byte compares and full lookups

- **Impact (claimed):** high · **Effort:** M · **Category:** build-config · **Default path:** yes
- **Location:** `core/libs/prx/libc/src/GuestWriteWatch.cpp:28-158`

**Mechanism.** The write watch is compiled only if the build host's kernel headers define PAGEMAP_SCAN and UFFD_FEATURE_WP_ASYNC, and enabled only if userfaultfd is permitted and a PAGEMAP_SCAN probe works. Otherwise CollectWrites returns 0 and UnchangedSince is always false, which disables every fast path in ShaderResources: snapshot texture hits do a full-surface memcmp, the stage-B fast proof and fastRevalidate always fail into full lookups, DCC key proofs never hold, and draw snapshots are never reused.

**Evidence.** GuestWriteWatch.cpp:28 '#if defined(__linux__) && defined(PAGEMAP_SCAN) && defined(UFFD_FEATURE_WP_ASYNC)' (else GuestWriteWatchAvailable returns false, :257-263); :137-149 userfaultfd (EPERM unless vm.unprivileged_userfaultfd or UFFD_USER_MODE_ONLY), UFFD_FEATURE_WP_ASYNC|WP_UNPOPULATED, PAGEMAP_SCAN probe; :153-158 prints '[memory] write watch unavailable ... guest memory is compared instead'. GuestMemory.hpp:58-59 'Without write watching CollectWrites returns 0 and UnchangedSince is always false'. Effects: ShaderResources.cpp:407-412 snapshot hit -> GuestMemory::EqualsCommitted(address, it->bytes) over the whole surface; :2551 fastTexture returns null; :1459 fastRevalidate fails with FastFail::Collect so every hit runs fullWalk (:1715-1751); DccMetadata.cpp:597-600 ProvedClearKeys never proves; Recorder.cpp:2240 KeepDrawSnapshot ignores generation 0 so PrepareDrawBindings memcpy's every read-only buffer every draw. Windows uses GetWriteWatch (GuestMemory.cpp _WIN32 branches) and is unaffected.

**Why this impact.** Applies to the default build on Linux hosts with older kernels, older build headers or restricted userfaultfd. Then each block-compressed texture revalidation is an O(surface bytes) memcmp under GpuMutex (MBs per texture, several textures per draw, thousands of draws per frame), enough on its own to hold a game at single-digit FPS. Not applicable on Windows.

**Proposal.** Declare the PAGEMAP_SCAN ioctl/struct and UFFD_FEATURE_WP_ASYNC/WP_UNPOPULATED constants locally when the headers lack them so a binary built on an older distro still uses the feature on a new kernel; surface the fallback prominently (repeat the warning every 10 s under APS5_PROFILE_DRAW); for older kernels add a cheaper tracker (mprotect-based dirty tracking of the guest arena, or soft-dirty bits via /proc/self/clear_refs + pagemap) instead of whole-surface compares.

**Risk.** Local ioctl definitions must match the kernel ABI (stable since 6.7); an mprotect fallback adds a fault per first write per page per epoch.

**How to measure.** Startup stderr '[memory] write watch unavailable: ...'; APS5_PROFILE_DRAW=1 '[rescache] revalidate ... fast-fail by reason: collect N', [textures] 'snapshot reads over recorded writes', and [hooksync] TextureCompare read-site waits.

#### shader-resources-draw-insert-inline-eviction: Draw-path cache Insert destroys evicted ShaderResources inline, inside ResourceCache::mutex and under GpuMutex (dispatch path defers this to the recorder)

- **Impact (claimed):** medium · **Effort:** S · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:1719`

**Mechanism.** The draw path calls Insert without an evicted vector, so the LRU victim (and a replaced same-key entry) is destroyed while ResourceCache::mutex is held, under GpuMutex: descriptor set free, data Buffers returned to the pool, GuestBufferMemory teardown, and texture/storage-image destruction when the texture caches already dropped them.

**Evidence.** Draw.cpp:1719 'SharedResourceCache().Insert(contentKey, resources);' (no evicted vector). ShaderResources.cpp:2030 lock_guard, :2031-2035 replaced entry erased, :2046-2050 'entries.pop_back()' run the destructor inside it; ~ShaderResources -> release() (:2708-2717) -> DescriptorCache::Free (mutex + vkFreeDescriptorSets), unique_ptr<Buffer> -> BufferPool::Put (mutex). VulkanDevice.cpp:3396-3404 dispatch Insert passes '&evicted' and 'state->recorder->Keep(std::move(object))' so destruction 'runs on the release thread once the batch completed, not here under the mutex' (APS5_NO_DEFERRED_EVICTION). Draw.cpp:983 Remove on invalidation likewise drops the node under the cache mutex and the object at scope end under GpuMutex, whereas VulkanDevice.cpp:3456-3462 hands it to the recorder.

**Why this impact.** Once the LRU is full, every draw miss evicts and destroys one object under two locks; ResourceCache::mutex is also taken by PrepareDispatch's unlocked Find (VulkanDevice.cpp:3062) on other queues, which then wait behind Vulkan destroy calls.

**Proposal.** At Draw.cpp:1719 pass a local std::vector<std::shared_ptr<ShaderResources>> evicted and hand each to Recorder::Active()->Keep(...) exactly as VulkanDevice.cpp:3402-3403; at Draw.cpp:983 use Remove(key, cached.get()) and recorder->Keep(std::move(cached)) as the dispatch invalidation path does.

**Risk.** Evicted objects live until the open batch completes (slightly higher peak memory); identical to the proven dispatch behavior.

**How to measure.** APS5_PROFILE_DRAW=1: the insert lands in the [draws] pipeline= phase (Draw.cpp:1719 precedes timer.phase(PhasePipeline) at :1747); [lock] Draw-site waits; the dispatch 'resources: insert' row for comparison.

#### shader-resources-bindless-padding-lookups: Bindless image tables are padded to 16 slots with copies of one T#, and every padded copy gets its own decode, collect, DCC scan, cache probe, lookup and revalidation

- **Impact (claimed):** medium · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2586-2617`

**Mechanism.** The materializer resizes every table to BindlessSlots() entries, padding unused slots with the first usable T#, and appends them to the stage's sampled-image array. ShaderResources treats each element independently, so a table with k keys costs 16 image lookups per build and 16 per-element proofs per Revalidate, and puts 16x8 words into the content key. Storage images have a dedupe for identical consecutive elements; sampled images have none.

**Evidence.** core/shader/recompiler/Optimization/src/ResourceMaterializer.cpp:412-422 'const auto pad = candidates[...]; ... resolution.slots.resize(slots, pad);' with slots = BindlessSlots() default 16 (:1027-1034, APS5_BINDLESS_SLOTS clamp 1..48); :649-652 appends slots 1..C-1 as extra images; DescriptorBindingBuilder.cpp Populate builds one binding per kind with count = resources.size(). ShaderResources.cpp precollectImages :2496-2526 (DecodeTextureResource, DescribeSurface, CollectWrites, TextureClearKeys, Textures().mutex probe per element), resolveImageBinding :2593-2617 (fastTexture or cachedTexture per element), fastRevalidate :1453-1502 per element, ContentKey :1224 copies all descriptor words. Storage dedupe only: StorageDedupeEnabled :726-739, :2638, fastRevalidate skip :1505. Bindless is on by default (ResourceTracker.cpp:742 APS5_NO_BINDLESS_IMAGES opt-out).

**Why this impact.** E.g. a 3-key table does 13 redundant lookups per build and 13 redundant proofs per hit, each several global-lock acquisitions (texture cache, write tracker, pending registry, storage cache) under GpuMutex; padding also inflates per-set sampled-image counts (faster descriptor-pool exhaustion) and the number of surfaces per build toward the 64-entry per-thread collect memo (GuestMemory.cpp:995-999).

**Proposal.** In resolveImageBinding and precollectImages keep a small per-binding table keyed by the element's 8 words + imageDepthCompare + imageUnnormalized + imageSamplers bits mapping to the first element's texture index; for a repeat push the same shared_ptr into textures/textureFirstLayer/describedRanges and skip the record and the lookup (mirror StorageDedupeEnabled with an APS5_NO_SAMPLED_DEDUPE kill switch). In fastRevalidate and fullWalk skip an element whose texture equals an earlier one (as :1505 does for storage). Descriptor writes still fill every slot with the same view, so the SPIR-V contract is unchanged.

**Risk.** Elements with equal words but different per-element flags must not share; the dedupe key must include the flags. captureValidation keys records by object, so duplicates correctly share one record.

**How to measure.** APS5_PROFILE_DRAW=1: [bindless] line (ResourceMaterializer.cpp:267: tables bound, slots, keys avg); [textures] 'stage-A records N: fast hits / full lookups' and [resources] images/precollect phase totals should fall by roughly (slots - keys)/slots of the table elements.

#### shader-resources-texture-log-fflush: Unconditional stdout logging with fflush for every created texture and each of its mips, on the lookup path under GpuMutex and the texture-cache mutex

- **Impact (claimed):** medium · **Effort:** S · **Category:** logging-debug-overhead · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Texture.cpp:246-248`

**Mechanism.** APS5_LOG_OUT is not compiled out (only the *_DEBUG macros are): it is fprintf(stdout) followed by fflush(stdout). The snapshot Texture constructor prints one line plus one per mip, the storage-view constructor one line, and StorageTexture one line, all reached from cachedTexture/cachedStorageTexture while Textures().mutex (and for draws GpuMutex) is held.

**Evidence.** Texture.cpp:246 'APS5_LOG_OUT("Texture address=0x%llx ...")' and :248 'for (const auto& mip : mips) APS5_LOG_OUT("  mip ...")'; :462 storage-view texture; :688 StorageTexture; :1696-1698 and :3679-3681 upload/writeback submit lines. LogMacros.hpp:31-40 '_APS5_LOG_IMPL' = fprintf + fflush(stream). Callers: ShaderResources.cpp:428 (view) and :440 (snapshot) inside the lock_guard at :388. VulkanDevice.cpp:2532-2535 records that 'Two stdout lines per draw cost ~1.3 ms per frame of the queue-0 worker (part of it under the GPU mutex)' and gated those behind APS5_TRACE_DRAWS.

**Why this impact.** Each new sampled texture emits 1 + mipCount flushed lines (about 13 for a 4K chain) under two global locks; on a Windows console a flushed line costs roughly 0.1-1 ms. In streaming or texture-cache churn this adds milliseconds of fully serialized time per frame.

**Proposal.** Gate these sites behind the existing APS5_TRACE_TEXTURES switch (ShaderResources.cpp:434) or switch them to APS5_LOG_OUT_DEBUG, as was done for the per-draw lines at VulkanDevice.cpp:2534.

**Risk.** None (diagnostic output only).

**How to measure.** Count 'Texture address=' lines in stdout per 10 s against the [textures] 'sampled created' counters (APS5_PROFILE_DRAW=1); A/B with stdout redirected to /dev/null (Linux) or NUL (Windows).

#### shader-resources-host-memory-bindings: Shader-read data buffers and per-draw buffer snapshots live in host system memory, so every draw's constant/structured reads cross PCIe on discrete GPUs

- **Impact (claimed):** medium · **Effort:** M · **Category:** gpu-side-efficiency · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2312-2323`

**Mechanism.** addDataBuffer and PrepareDrawBindings create Buffers with the default HOST_VISIBLE|HOST_COHERENT properties, which Buffer resolves to HOST_CACHED system memory. These buffers are written once by the CPU and read many times by shaders; on a discrete GPU each L2 miss on them is a PCIe round trip. In-place guest buffers are host imports, also system memory. No BAR/ReBAR (DEVICE_LOCAL|HOST_VISIBLE) memory type is used anywhere in the driver.

**Evidence.** ShaderResources.cpp:2316 'std::make_unique<Buffer>(context, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | ...)'; :2784 and :2809 'std::make_shared<Buffer>(context, ..., VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ...)'; Resources.hpp:11 default 'VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT'; Resources.cpp:39-45 prefers HOST_CACHED 'because the CPU reads most of these buffers back'. The only CPU read of a data buffer is the memcmp fallback at ShaderResources.cpp:2740 when dataWords is empty. GuestBufferMemory::Descriptor (GuestBufferMemory.cpp:2480-2498) binds region.direct->buffer (the host import). grep finds no DEVICE_LOCAL|HOST_VISIBLE use in libSceAgcDriver. GuestBufferMemory.cpp:1766-1769 documents 2.8 ms per dispatch for scattered PCIe stores and moved written elements to device-local staging, but read-only draw inputs were left in host memory.

**Why this impact.** Consistent with a GPU that is busy waiting rather than computing: pixel and vertex waves stall on PCIe latency for constants and structured data. The effect is large on discrete GPUs without ReBAR-aware placement and absent on APUs.

**Proposal.** Allocate data buffers and draw snapshots with DEVICE_LOCAL|HOST_VISIBLE|HOST_COHERENT when such a type exists (ReBAR, or the 256 MiB BAR heap under a budget via VK_EXT_memory_budget), falling back to the current type; always keep dataWords so no CPU read touches write-combined memory. For large read-only guest buffers unchanged across frames (UnchangedSince), extend GuestBufferMemory's device-local staging to read-only elements: one vkCmdCopyBuffer per change, then bind the VRAM copy.

**Risk.** BAR heap is small without ReBAR; CPU reads from write-combined memory are very slow (Describe(), the :2740 memcmp); staging read-only data adds a copy per change and needs stamp-based invalidation.

**How to measure.** APS5_PROFILE_GPU=1 [gputime] Draw class and per-program GPU ms (Recorder.cpp:2154); an external GPU profiler (Nsight Graphics / RGP) for PCIe read throughput and shader stall reasons.

#### shader-resources-descriptor-pool-chain: DescriptorCache pool chain: fixed 1024-set pools with only 1024 sampled images, a newest-first retry loop of failing vkAllocateDescriptorSets under one mutex, pools never freed

- **Impact (claimed):** medium · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2139-2213`

**Mechanism.** Chain pools are created with ratios of 4 storage buffers and 1 sampled image per set. Image-heavy draw sets exhaust a pool's image budget long before its set budget, so the chain grows; each Allocate holds the cache mutex and walks pools from the newest, paying one failing driver call per full pool, before creating another. Sets above a pool's per-type capacity get a dedicated pool created and destroyed per set. Every build and every recorded draw allocates; every release frees under the same mutex from the release thread.

**Evidence.** ShaderResources.cpp:2139-2141 ChainPoolSets 1024, sizes STORAGE_BUFFER 4096, SAMPLED_IMAGE 1024, STORAGE_IMAGE 1024, SAMPLER 512; :2151 one lock_guard for all of Allocate; :2178-2187 'for (auto it = pools.rbegin(); ...)' with OUT_OF_POOL_MEMORY/FRAGMENTED_POOL retried; :2188-2201 new pool; :2156-2175 and :2207-2210 dedicated pool per oversized set; pools only destroyed in ~DescriptorCache (:2104-2108). Allocate called per build (:1003) and per recorded draw (:2824); Free per release (:2710, :2724).

**Why this impact.** A pixel shader with ~30 sampled images (e.g. two 16-slot bindless tables) fills a pool's images after ~34 sets, so live sets (1024 cached templates plus in-flight per-draw sets) spread over dozens to hundreds of pools and allocations walk failing pools while other queues and the release thread wait on the mutex; at least two Allocate/Free pairs happen per draw miss.

**Proposal.** Size chain pools from observed ratios (e.g. per 1024 sets: 8192 storage buffers, 16384 sampled images, 2048 samplers, 1024 storage images) or keep pools per layout key sized to the layout's counts times N; remember the pool that last succeeded instead of always starting at the newest; move transient per-draw sets to per-batch pools reset in one vkResetDescriptorPool; trim empty pools periodically.

**Risk.** Larger pools reserve more driver memory each; per-layout pools need eviction for rare layouts.

**How to measure.** APS5_PROFILE_DRAW=1 '[descriptors] layouts ... sets N from P pools and D dedicated pools' (ShaderResources.cpp:1189) every 10 s; add a counter of failing vkAllocateDescriptorSets attempts per Allocate; [lock] waits.

#### shader-resources-texture-cache-mutex-scope: Textures().mutex is held across full-surface compares, snapshot reads that wait in the flush hook, and Texture creation

- **Impact (claimed):** medium · **Effort:** M · **Category:** global-lock-serialization · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:386-452`

**Mechanism.** cachedTexture takes the sampled-texture cache mutex for the whole hit/miss handling, including GuestMemory::EqualsCommitted over the surface and ReadTextureSurface (both go through the flush hook and may wait for recorded GPU work), Texture construction (image creation, allocation, staging memcpy, detile recording) and budget eviction. The same mutex is taken per element by stage-A precollect on dispatch threads that deliberately run without GpuMutex.

**Evidence.** :388 'std::lock_guard lock(cache.mutex);' held through :407-412 equalsCommitted (comment :404-406: 'it waits for recorded work over the surface'), :433 ReadTextureSurface, :440 'std::make_shared<Texture>(context, *context.detiler, resource, components, entry.bytes, depthCompare)' (Texture.cpp:258-275 vkCreateImage/vkAllocateMemory, :295-296 staging memcpy), :444 eviction destroying Textures. Same mutex at precollectImages :2512-2513 (unlocked dispatch stage A) and fastTexture :2575-2576.

**Why this impact.** One snapshot creation or compare (MBs, possibly waiting on the GPU) blocks every other thread's texture probes for its full duration, undoing the benefit of moving dispatch stage A off GpuMutex.

**Proposal.** Split cachedTexture: probe/touch under the mutex, release it for the compare/read/creation, then re-lock to insert (if another thread inserted meanwhile keep that entry and drop ours); collect eviction victims under the lock and destroy them after unlocking.

**Risk.** Two threads can occasionally build the same texture (wasted work, not incorrect); the insert must re-check, and stage-A records/fastTexture identity checks must still find the entry they read.

**How to measure.** APS5_PROFILE_DRAW=1 LookupOutcomes rows 'sampled made snapshot', 'sampled hit snapshot', 'pending flush' (Texture.cpp:1170) in the [dispatch]/[indirect] tables; add try_lock contention counting to Textures().mutex like lockTracker (GuestMemory.cpp:946-950).

#### shader-resources-snapshot-texture-copies-budget: Block-compressed sampled textures keep a full CPU byte copy, are copied twice per creation, and live under a hard-coded 2 GiB budget whose evictions cascade into resource-cache invalidations

- **Impact (claimed):** medium · **Effort:** M · **Category:** allocation-copy · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:423-452`

**Mechanism.** BC formats are not storage-eligible, so they take the snapshot path: the whole guest surface is copied into the cache entry (kept only for later compares), copied again into a staging Buffer, uploaded and detiled. The cache is bounded at 2 GiB with LRU eviction; an evicted texture still referenced by a cached ShaderResources makes the next lookup create a new object, which fails that template's Revalidate and forces a rebuild.

**Evidence.** SampledFromStorageEligible :224-228 excludes IsBlockCompressed; :423 'std::vector<std::byte>(source != nullptr ? 0u : bytes)' kept in the entry; :433 ReadTextureSurface fills it; Texture.cpp:295-296 'auto staging = std::make_shared<Buffer>(...); std::memcpy(staging->Bytes().data(), snapshot.data(), snapshot.size());'; :443 'constexpr std::uint64_t budget = 2048ull << 20u;' (and :611 for storage images) with no override; :444 eviction. Cascade: fastTexture :2578 'it->texture != record.texture' fails, fullWalk :1729 compares the new object against textures[textureIndex] and fails, Draw.cpp:982-984 removes the template.

**Why this impact.** If a gameplay scene's sampled working set exceeds 2 GiB of guest bytes (plausible for a 16 GB-unified-memory PS5 title with streaming), the LRU churns every frame: each re-creation does two full CPU copies, a re-upload and detile, ~13 flushed log lines, and invalidates every template that references it.

**Proposal.** Make both budgets configurable and default them from VK_EXT_memory_budget and system RAM; when write watching is available drop the per-entry CPU byte copy (stamps prove reuse; keep a per-64 KiB-block hash for the rare compare) and read guest bytes directly into the staging Buffer (one copy); add a budget-eviction counter to [textures].

**Risk.** Raising budgets blindly grows RAM/VRAM; without write watch the byte copy is the only proof of reuse and must stay.

**How to measure.** APS5_PROFILE_DRAW=1 [textures] 'sampled created: X from storage images, Y snapshots' growing steadily during gameplay indicates churn; [rescache] revalidate failed/full counts; add an eviction counter at ShaderResources.cpp:444.

#### shader-resources-unmemoized-resource-rejections: Unsupported T#/S# features throw inside the resource build on every draw, every frame; only ValidateShaders failures are memoized

- **Impact (claimed):** low · **Effort:** S · **Category:** incomplete-implementation · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2586-2617`

**Mechanism.** Texture and sampler descriptor decoders throw for unimplemented features; the build has already done key/Find, stage A (descriptor set, data buffers, earlier element lookups) when the throw unwinds under GpuMutex. Nothing records the failure, so the same draw repeats the work every frame and still renders nothing.

**Evidence.** GuestTextureResource.cpp:110-113,126 (corner sampling, PRT default color, array pitch, MSAA, BC swizzle) and GuestSamplerResource.cpp:102-116,143 (forced sRGB, degamma off, secondary LOD bias, point preclamp, anisotropy override, PRT blend-zero, border color table) throw via Require; ShaderResources.cpp:289-293 and :566-570 throw for depth-surface accesses. SamplerCache::Get decodes before inserting (Sampler.cpp:75-77) so a failing S# retries every build; precollectImages swallows the decode error (:2522-2524) and stage B re-decodes and throws (:2596). PacketExecution.cpp:281-285 catches and counts DrawSkip::Thrown. KnownValidationFailure (Draw.cpp:1485-1492) only consults validationFailures(), filled only by CachedFragmentOutputs (:549-555).

**Why this impact.** Each such draw wastes part of a build plus an exception unwind under the global lock every frame; this is one concrete way unimplemented functions cost performance. It becomes medium if the [draws] thrown counter shows hundreds per second for Astro Bot.

**Proposal.** Memoize build failures keyed by the draw's resource key (DrawResourceKey without ranges, or the offending descriptor words plus per-element flags) in the same table as validationFailures and check it in KnownDrawRejection before lockForDraw; cache negative sampler decodes in SamplerCache. Then implement the most frequent missing features the skip counters report (e.g. border color tables via VK_EXT_custom_border_color).

**Risk.** The memo key must contain every word and flag the decode consumed so a later valid descriptor is not rejected.

**How to measure.** APS5_PROFILE_DRAW=1 [draws] '; skipped packets: ... thrown N in X ms' (Draw.cpp:403-406); APS5_TRACE_DRAWS=1 prints each failure reason.

#### shader-resources-key-hash-validation-memo: Per-draw key building and ordered-map lookups: DrawResourceKey vectors, FNV over the whole key, a validation memo built twice per draw with vector keys and std::set copies, layout std::map

- **Impact (claimed):** low · **Effort:** S · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/Draw.cpp:456-603`

**Mechanism.** Non-recipe draws rebuild their resource key from scratch (a vector per stage plus a concatenated copy), hash and compare it word by word, build the validation key twice (KnownDrawRejection and CachedFragmentOutputs) and search std::maps with vector keys under a global mutex, copying a std::set result; each build also copies the layout key and searches a std::map. ValidateShaders itself is memoized, but the memo is cleared wholesale at 1024 entries.

**Evidence.** Draw.cpp:583-603 DrawResourceKey via ShaderResources::ContentKey (ShaderResources.cpp:1192-1239; 'key.reserve(8 + program.bindings.size() * 12)' underestimates the 8 words per image element); ResourceCache KeyHash hashes every word (ShaderResources.hpp:495-501) and Find compares vectors under the mutex (:1946-1956). ValidationKey Draw.cpp:456-520; KnownDrawRejection (Execution Draw.cpp:362 -> Draw.cpp:1485-1492) and CachedFragmentOutputs (:532-565) each build it and search std::map under validationMutex; the memo returns a std::set<uint32_t> copy and is cleared at 1024 entries (:561), forcing full SPIR-V re-inspection (ShaderValidation.cpp:131-508). DescriptorCache::Layout copies the key into a new vector and searches std::map<vector> (ShaderResources.cpp:2110-2116) per build.

**Why this impact.** A few microseconds per non-recipe draw under GpuMutex; small individually but multiplied by thousands of draws per frame, and the wholesale memo clear causes periodic full-SPIR-V validation spikes.

**Proposal.** Precompute a 64-bit content hash per compiled stage when its variant/draw entry is built and use it for ResourceCache (full compare only on a hash hit); compute the validation key once per draw and share it between KnownDrawRejection and CachedFragmentOutputs; key the memo by an unordered_map on a 64-bit hash with LRU instead of clear(); return fragment outputs as a uint32 bitmask; use an unordered_map with a stored hash for DescriptorCache layouts.

**Risk.** Minimal; collisions are resolved by the existing full compares.

**How to measure.** APS5_PROFILE_DRAW=1 [draws] validate= and lookup= phase totals; [rescache] 'validation memo H hits / M misses'.

#### shader-resources-fasttexture-per-element-locks: Stage-B fast texture proof takes 4-6 global locks per sampled element instead of batching them like fastRevalidate

- **Impact (claimed):** low · **Effort:** M · **Category:** per-draw-cpu-cost · **Default path:** yes
- **Location:** `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:2531-2584`

**Mechanism.** For each sampled element of a fresh build, fastTexture separately collects writes, scans the pending registry, proves DCC keys, checks the storage cache, checks stamps and re-locks the texture cache. fastRevalidate performs the same proof for a whole object with one registry scan, one stamp query batch and one storage-cache pass.

**Evidence.** fastTexture: GuestMemory::CollectWrites (:2551, tracker mutex on memo miss), PendingStorageOverlaps (:2552), ProvedClearKeys (:2558), StorageImageCached (:2565, storage cache mutex), UnchangedSince (:2565/:2570, tracker mutex), FindPending (:2569), Textures().mutex (:2576). fastRevalidate batches: StorageTexture::ScanPending (:1525), GuestMemory::UnchangedSinceAll (:1558), StorageImagesCached (:1559).

**Why this impact.** A build with 20 sampled elements makes roughly 100 lock acquisitions under GpuMutex, contending with unlocked dispatch stage A on other threads; this is the common path whenever contents churn (cache misses).

**Proposal.** Restructure stage B like fastRevalidate: gather all records' queries, one ScanPending, one UnchangedSinceAll, one StorageImagesCached and one Textures().mutex section that validates and touches all entries and logs the lookups; fall back to cachedTexture only for elements that failed.

**Risk.** Must keep the collect-before-check ordering rule documented at :2541-2548 and the per-element fallback semantics.

**How to measure.** APS5_PROFILE_DRAW=1 LookupOutcomes 'sampled fast hit' / 'sampled fast miss' ms; [resources] images phase; [lock] Draw-site hold time.

### Incomplete or stubbed code noticed

- `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:920-922 and Graphics/src/ShaderValidation.cpp:471`: Guest buffers are bound only as writable VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; 'read-only descriptors are unsupported because the recompiler emits no NonWritable decoration' and uniform buffers are rejected, although the recompiler knows bufferWritten per element. *Performance relevance:* GPU side: drivers cannot use read-only/constant cache paths or assume no aliasing for constant and structured data that is never written, adding shader latency on every draw.
- `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.hpp:40-48 and ShaderResources.cpp:2102-2213`: No push descriptors, descriptor update templates, descriptor buffers or dynamic offsets anywhere in the driver (grep finds only vkResetDescriptorPool in TextureDetilerDescriptors.cpp); every draw uses pool-allocated sets updated with vkUpdateDescriptorSets and VkCopyDescriptorSet. *Performance relevance:* Root cause of the per-draw descriptor-set allocation/copy/free cost (finding shader-resources-per-draw-descriptor-set).
- `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:289-293, 566-570, 1102, 2610, 2635`: Not implemented: 32-bit integer read of a depth-layout texture, storage-image access to a depth/stencil surface, a misaligned guest buffer in a shader without shader data, unnormalized sampling of multi-level/array views, storage minLod above the addressed level; each throws. *Performance relevance:* The draw is dropped after a partial build under GpuMutex and the failure is not memoized, so the cost repeats every frame (finding shader-resources-unmemoized-resource-rejections).
- `core/libs/prx/libSceAgcDriver/Graphics/src/GuestTextureResource.cpp:100-113,126`: Texture streaming feedback (LOD warning / mip statistics) is ignored; corner sampling, PRT default color, nonzero array pitch, MSAA textures and BC swizzle throw. *Performance relevance:* A streamer that relies on mip feedback never gets it (streaming behavior and memory traffic may differ from console); throwing features drop draws every frame.
- `core/libs/prx/libSceAgcDriver/Graphics/src/GuestSamplerResource.cpp:102-116,143`: Forced sRGB, degamma disable, secondary LOD bias, point preclamp, anisotropy override, PRT blend-zero and border color tables throw; anisotropy threshold/bias are ignored. *Performance relevance:* Throws are retried on every build because SamplerCache::Get decodes before inserting; affected draws are skipped every frame.
- `core/shader/recompiler/Optimization/src/ResourceMaterializer.cpp:303-431`: Bindless image tables: storage and packed tables unsupported, tables with more distinct keys than APS5_BINDLESS_SLOTS (default 16) rejected, and every table padded to the slot count with copies of the first usable T#. *Performance relevance:* Padding multiplies per-element lookups and proofs in ShaderResources (finding shader-resources-bindless-padding-lookups); rejections drop draws.
- `core/libs/prx/libSceAgcDriver/Graphics/src/ShaderResources.cpp:443 and :611`: Sampled-texture and storage-image cache budgets are hard-coded to 2 GiB each with no override or VRAM-budget awareness. *Performance relevance:* Working sets above the budget churn every frame and cascade into resource-cache invalidations (finding shader-resources-snapshot-texture-copies-budget).
- `core/libs/prx/libc/src/GuestWriteWatch.cpp:28`: Linux write watch is compiled out silently when build headers lack PAGEMAP_SCAN/UFFD_FEATURE_WP_ASYNC, with a byte-compare fallback. *Performance relevance:* Disables every stamp-based fast path in resource validation on such builds (finding shader-resources-linux-write-watch).

### Settings to try without code changes

| Setting | Expected effect | Risk |
|---|---|---|
| `APS5_PROFILE_DRAW=1` | Prints every 10 s the lines needed to rank these findings: [lock] (GpuMutex waits per site), [draws] (lookup/resources/record/pipeline phases, hook waits, thrown skips, recipe hits), [rescache] (draw hits/misses/invalidated, miss churn 'same key'), [resources], [descriptors], [textures], [bufferpool], [bindless]. | Small timing overhead; no behavior change. |
| `APS5_RESOURCE_CACHE_ENTRIES=8192` | If [rescache] shows the cache pinned at 1024 entries and many 'same key' draw misses, more draws become template hits (Revalidate instead of a full build), cutting GpuMutex hold time per draw. | Entries pin textures/storage images beyond the 2 GiB caches and hold one descriptor set each: higher RAM/VRAM and descriptor-pool memory; correctness unchanged. |
| `APS5_BINDLESS_SLOTS=8` | Only if the [bindless] line shows keys avg well below 8 and zero capacity rejections: halves padded sampled-image lookups and proofs per bindless table and shrinks sets. | Tables with more distinct keys than 8 are rejected (draws dropped); changes specializations so shaders recompile once (clear the shader disk cache if variants mismatch). |
| `Run with stdout redirected to a file or the null device (Linux: '> /dev/null', Windows: '> NUL')` | Removes console cost of the unconditional fflush'd texture-creation log lines (Texture.cpp:246-248, 462, 688) emitted under GpuMutex and the texture-cache mutex; most visible on a Windows console during streaming. | stdout diagnostics are lost (stderr profile lines are unaffected). |
| `Linux only: run on kernel >= 6.7, allow userfaultfd ('sysctl -w vm.unprivileged_userfaultfd=1') and build against >= 6.7 kernel headers; confirm stderr has no '[memory] write watch unavailable' line` | Enables stamp-based reuse proofs; without it every texture/snapshot check is a byte compare or full lookup, which alone can explain single-digit FPS. | System-wide security setting change for userfaultfd. |
| `APS5_PROFILE_GPU=1` | [gputime] GPU ms per class and program shows whether the GPU is idle (CPU/lock bound) or slow per draw (e.g. PCIe-bound host-memory bindings). | Timestamp queries add small GPU overhead. |
