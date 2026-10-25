# TraceRTL Kunminghu V3 difftest migration progress

| Phase | Change summary | Build | GemsFDTD smoke | gcc_expr smoke | SPEC06 0.3 |
|---|---|---|---|---|---|
| 1 | Source GSIM wrapper ported as `ee497ea5a` | Not run; Phase 2 pending | Not applicable | Not applicable | Not applicable |
| 2 | Fresh GSIM build after pipeline trace metadata fixes | PASS `20261024-6`, `01M5NS6Y3627GZFDBT7KZ7MBB7` | Not applicable | Not applicable | Not applicable |
| 3 | Validate real trace startup and alignment | PASS `20261026-1`, `01M5T17GXBKJ8FFKPH877W8H5P` after S1 shift fix | FAIL `20261026-2`: LoadQueueRAW mask write collision | Not run; blocked by new runtime failure | Blocked until both smoke tests pass |
| 3A | Reproduce and diagnose assertion with isolated Verilator FST | PASS `20261024-12` isolated build; PASS `20261024-13` waveform reproduction and shift/bank diagnosis | IBuffer shift root cause fixed; new LoadQueueRAW failure found | Not run | Blocked |
| 3B | Diagnose LoadQueueRAW duplicate mask write | PASS isolated Verilator build `20261026-3`; RUNNING corrected FST capture `20261026-5`, `01M5T4MEY3WXQW3CGE76KDC3HX` | `20261026-4`: 569 instructions, collision at 16551; initial window missed it | Not run | Blocked |
| Final | Full 143-point validation | Not run | Not run | Not run | Not run |

## Phase 1

### Changes made

- Created the required plan/progress records and added the user-required Verilator waveform failure gate.
- Based the independent branch on upstream `99f77ff455766d18cd05927b144641bd9744b19a` and cherry-picked source `7f4f14ea49c58add5e6c3fc43dad6b8cad269689` as `ee497ea5a`.
- Ported TraceRTL arguments, runtime initialization, GSIM wrappers, and build integration across seven files.
- Resolved `args.cpp` by preserving all upstream options and adding TraceRTL options. Resolved `gsim.mk` by combining upstream timing instrumentation with source stale-model cleanup and pipe-failure propagation.

### Test results

- PASS: wrapper cherry-pick completed with no unresolved conflicts and `git diff --check` passed.
- PASS: the branch is clean and contains upstream `99f77ff45` plus the independent documentation and wrapper commits.
- NOT RUN: no emulator build or runtime test yet; Phase 2 must provide those results.

### Issues and debugging

- External superproject submodule clones were unreliable, so the isolated checkout was initialized from local committed repositories. No source working-tree content was copied.
- The existing DeepSeek runset is not evidence of success: its build succeeded, but all 143 runs aborted at instruction 1 after a no-commit watchdog. Its branch and binary will not be reused.

## Phase 2

### Changes made

- Completed and committed the independent superproject merge as `7d611ca94`; the successful build records difftest `16c79d94a` and TraceRTL `e6d6fc12`.
- Configured Featx with GSIM trace mode, 64 build jobs, one emulator thread, no binary cache reuse, and the collector.
- Added compressed-ROB per-slot trace metadata handling and the user-required Verilator waveform debug path.
- Fixed current API mismatches: ROB `debugMeta(uop).pc`, IFU `PcInit(...).signGuard` and `s2_alignShiftNum`, checker `blockSel`, and removal of an obsolete store `vstart` assignment.
- Migrated `TraceDecodedEntry` and `TracePreDecodeFlatResp` jump offsets to `GuardedPc` and removed the redundant IFU conversion.
- Added a guarded-address helper for fixed and predicted targets in `TracePredictChecker`; retained `Pc` for `mispredPc`.
- Propagated `traceInfo` through the DataPath issue register and the Region vector-store-data path; initialized the trace-aligned IFU instruction `index` from its instruction-start halfword, `startOffset`.

### Test results

- FAIL: runset `20261024-1`, build `01M5NQ560Q3SWC9BHCCTYMD1KD`, failed at `XSCache.compile` because ZhuJiang types were absent; no RTL elaboration or runtime test occurred.
- FAIL: runset `20261024-2`, build `01M5NQ943G825T95J9PN2F8G0V`, reached merged Scala and reported nine API errors in ROB, IFU, TracePredictChecker, and NewStoreUnit; no runtime test occurred.
- FAIL: runset `20261024-3`, build `01M5NQK55XKPM97TFDNHB6KTPN`, failed during hardware elaboration at `TracePreDecoder.scala:38` with a 50-bit versus 51-bit address-width mismatch; no runtime test occurred.
- FAIL: runset `20261024-4`, build `01M5NQXMWNGY7B3K1RSZQJPNQQ`, moved to `TracePredictChecker.scala:207`, where the fixed target still used the old 50-bit type; no runtime test occurred.
- FAIL: runset `20261024-5`, build `01M5NR569VE98XW79WVRBD72K9`, reported 486 uninitialized-field errors: 390 DataPath, 60 Region, and 36 IFU `index` fields. No emulator or runtime test was produced.
- PASS: runset `20261024-6`, build `01M5NS6Y3627GZFDBT7KZ7MBB7`, freshly elaborated and generated the model with the three metadata fixes, then linked `build/emu` in step 2. Featx recorded uncached binary SHA-256 `978a1a044d4455998178240cee520da6ce8ccf8958ad34e59edf7922844072da`; `src-patches/super.patch` exactly matches the committed Scala changes.
- PASS: `git diff --check` passed for the Scala and progress-document changes.

### Issues and debugging

- Root cause of the first failure: the initializer skipped `XSCache/ZhuJiang` as optional, but `XSCache/src/main/scala/zhujiang/perf/XiangShanPerfBackend.scala` requires its types. Initialized the exact gitlink from a local checkout and resubmitted rather than changing RTL.
- Root cause of the second failure: source-side code retained old names and types removed by Kunminghu V3. Migrated each use to the current API instead of restoring the removed interfaces.
- Root cause of the third failure: current branch-offset helpers return 51-bit `GuardedPc`, while the old trace bundle retained a 50-bit `PrunedAddr`. Migrated the bundle type instead of truncating the offset.
- Root cause of the fourth failure: `PredCheckRedirect.target` is guarded, while the source fixed-target path still returned `PrunedAddr`. Converted all fixed-target producers and the predicted-target comparison to `GuardedPc`.
- Root cause of the fifth failure: upstream's partial bundle assignments did not carry the added trace metadata. Propagated it from the actual upstream payload and initialized the IFU index without `DontCare`.
- The runset `20261024-6` step-1 link stopped on generated ChiselDB/performance symbols. Featx's configured step 2 compiled `chisel_db.cpp` and completed the link with exit status 0; no additional compatibility source change or stale-model reuse was needed.

## Phase 3

### Changes made

- Added `DIFFTEST_BUILD_DIR` to isolate generated Verilator support files from the GSIM build tree.
- After waveform diagnosis, changed only `traceRTL.io.fromIFU.ifuS1AlignShift` in `Ifu.scala` from the previous S2 shift to `s1_alignShiftInstrNum`. Normal IFU and IBuffer alignment logic is unchanged.
- After the second waveform reproduction, changed LoadQueueRAW's age-rank comparisons to slot-aware `isBeforeSlot` and `isSameSlot` so compressed ROB slots cannot collapse onto one allocation rank.

### Test results

- FAIL: GemsFDTD `38797`, runset `20261024-7`, task `01M5PYMKAPPFCCRHDE8Y754RQJ`. The run loaded the real trace, preread 40000000 instructions, reported one finished instruction, then asserted `Bubble in ibuffer.out` at logged time 8425. The recorded command used the runset `20261024-6` binary, `--tracertl-file`, `W=0`, and `I=100000`.
- NOT RUN: gcc_expr `4892` awaits a fresh build and a passing GemsFDTD retest after the shift fix.

### Issues and debugging

- The failing assertion is at `src/main/scala/xiangshan/frontend/ibuffer/IBuffer.scala:129-131`: the output head is invalid while at least one later output is valid. Waveform analysis traced it to trace-packet bank alignment, not the earlier DeepSeek ROB watchdog.
- Runset `20261024-8` generated the Verilator model but failed C++ compilation because GCC rejects the default `-fno-slp-vectorize`. Runset `20261024-9` emitted a multiword `OPT_FAST` override without shell quoting and stopped before compilation. Neither produced a waveform.
- Runset `20261024-10`, task `01M5PZM5PJZKXG36ADRQQM65AP`, succeeded with `EMU_TRACE=fst` and `CXX=clang++`, but reused the GSIM `build/` tree. It is preserved as a compiler check but was not used for waveform diagnosis.
- FAIL: runset `20261024-11`, task `01M5Q05FM39J6618NFRH4K4RAP`, freshly elaborated RTL into the dedicated directory but Verilator could not find `DifftestMacros.svh`. `FileControl.scala` hardcoded `NOOP_HOME/build/generated-src`, so the attempt also wrote support files into the old GSIM tree. The failed isolated tree is preserved as `build-verilator-fst-20261024-11/`.
- PASS: runset `20261024-12`, task `01M5Q1CQMRVFP1R77W435WN98C`, completed the fresh isolated Verilator build with clean provenance. Its binary SHA-256 is `de09b270ce1e7467fc08a30a2897f5726420001f241ad96aa366cbca600fc889`. `DIFFTEST_BUILD_DIR` produced the support files under `build-verilator-fst/generated-src`; the GSIM macro copy was not modified by this attempt.
- REPRODUCED: runset `20261024-13`, task `01M5Q31GHSYEYBH6NDM8MKFVSM`, used the isolated binary and real trace, and hit the same IBuffer output-bubble assertion at logged time 8379. The 6 MB FST at `run/GemsFDTD_38797_0.210512/gems_fdtd_ibuffer.fst` covers cycles 7500-9000 and has been indexed together with the fresh RTL.
- CONFIRMED ROOT CAUSE: at the failing transaction, the current S1 enqueue-pointer bank is 0, while the migrated TraceRTL path registers shift 2 from the previous S2 packet. `s3ShiftNum` is 2, the shifted checker mask is `0x4`, and only `io_s3Decoded_2_compact_valid` is set. IBuffer has `enqPtr` bank 0 and `useBypass=1`, so the bypass logic in `IBuffer.scala:247-270` reads an invalid lane 0 and a valid later lane. `TraceRTL.scala:89-98` captures on the S1-to-S2 transfer, confirming a pipeline-stage selection error. The `Ifu.scala` fix uses the current S1 instruction-pointer shift.
- PASS: runset `20261026-1`, task `01M5T17GXBKJ8FFKPH877W8H5P`, freshly elaborated and built the S1 shift fix. The uncached binary SHA-256 is `13978cc4e9e450ff60bcb514a9e4097cd67673465f28c2702f25dbc095584a7f`.
- FAIL: runset `20261026-2`, task `01M5T32PE3KQAN6E6V21XP9X3R`, no longer hits the IBuffer assertion but fails at `LoadQueueData.scala:130`: two LoadQueueRAW mask writes target the same entry. This is a distinct runtime failure; its allocation/write-enable cause is not yet established. Isolated Verilator build `20261026-3`, task `01M5T36MAMTJNH0HRD4A4WSDV9`, is running in `build-verilator-fst-lq/` for the required waveform reproduction before further smoke or campaign dispatch.
- CONFIRMED ROOT CAUSE: the FST shows accepted lanes 0 and 1 with `enqueueRank=0`, equal ROB `flag/value` (entry 12), and opposite `isFormer` bits. Generic `isBefore` ignores that slot bit while full-bundle `===` distinguishes it, so the source rank algorithm collapses both lanes to rank 0. Both then select allocator output 0, giving `maskModule.io_waddr_0 = maskModule.io_waddr_1 = 3` at the assertion edge; lane 2 uses rank 2 and a distinct address. The fix in `LoadQueueRAW.scala:139-141` uses `isBeforeSlot` and `isSameSlot`, preserving age ranking and the port-index tie-break without adding debug-only logic. Runtime validation is pending.
- Runset `20261026-4` reproduced the same LoadQueueData assertion after 569 instructions at cycle 16551, but its 7500-9000 capture missed the failure. Runset `20261026-5`, task `01M5T4MEY3WXQW3CGE76KDC3HX`, repeated the same binary and point with bounds 16000-16700 and captured the same assertion at cycle 16551 in a 6.8 MB FST. This demonstrates progress beyond instruction 1 and the 15000-cycle watchdog, but the smoke still fails and is not a pass.

## Final verification

### Changes made

- No campaign-specific changes yet.

### Test results

- Configured points: 0 submitted by this independent attempt.
- Successful points: 0 verified.
- Running points: 0.
- Failed points: 0.

### Issues and debugging

- The full campaign is blocked until the fresh build and both smoke tests pass.
