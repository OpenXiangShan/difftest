# TraceRTL Kunminghu V3 difftest migration progress

| Phase | Change summary | Build | GemsFDTD smoke | gcc_expr smoke | SPEC06 0.3 |
|---|---|---|---|---|---|
| 1 | Source GSIM wrapper ported as `ee497ea5a` | Not run; Phase 2 pending | Not applicable | Not applicable | Not applicable |
| 2 | Fresh GSIM build after pipeline trace metadata fixes | PASS `20261024-6`, `01M5NS6Y3627GZFDBT7KZ7MBB7` | Not applicable | Not applicable | Not applicable |
| 3 | Validate real trace startup and alignment | PASS `20261026-6`, `01M5T5DMKZNCHFR5V58FG82JSQ` after RAW slot-rank fix | FAIL `20261026-7`: ROB no-commit watchdog | Not run; blocked by new runtime failure | Blocked until both smoke tests pass |
| 3A | Reproduce and diagnose assertion with isolated Verilator FST | PASS `20261024-12` isolated build; PASS `20261024-13` waveform reproduction and shift/bank diagnosis | IBuffer shift root cause fixed; new LoadQueueRAW failure found | Not run | Blocked |
| 3B | Diagnose LoadQueueRAW duplicate mask write | PASS isolated build `20261026-3` and FST `20261026-5`; slot-rank fix committed | Waveform confirmed equal-rank compressed-slot collision; RAW assertion cleared in retest | Not run | Blocked |
| 3C | Diagnose ROB watchdog and FP writeback | PASS GSIM `20261026-35` and isolated Verilator `20261026-38`; PASS bounded FST diagnostic `20261026-39` | FAIL GSIM `20261026-37`: persistent ROB watchdog; fresh Verilator completed 100007 instructions and captured cycles 14000-30000 for comparison | Not run | Blocked |
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
- FAIL: runset `20261026-7`, task `01M5T78AREN29PDDPN60F3S7Q8`, hit `[CROB-STUCK]` with `deq=73`, `commit_v=1`, and `realDestSize=3`. The watchdog at `Rob.scala:1194-1201` is a print, not the hardware assertion itself; GSIM's formatted-print assertion truncates the remaining details. `realDestSize` is not an instruction count, and review confirms `commitDebugUop(2 * i)` selects the correct head slot. The FST must inspect entry `0x73` and bank 3 for incomplete writeback versus a commit-wide block or metadata mismatch. Isolated Verilator build `20261026-8`, task `01M5T7BCGTYSR3P5Z3Q68K6VTB`, is queued for the shared 64-core build allocation; no RTL change is justified yet.
- PASS: isolated Verilator build `20261026-8` completed with `build-verilator-fst-rob/` and an uncached binary. Diagnostic runset `20261026-9`, task `01M5TABCFSE0P2288YX8STC6MW`, reached 1827 instructions by cycle 20000 without reproducing the watchdog; its FST covers cycles 14000-16000 and is being used to locate the last commit for a later capture. This cycle-limited diagnostic is not a smoke pass.
- Runset `20261026-10` reached 41621 instructions at cycle 40000 without a watchdog. Runset `20261026-11`, task `01M5TB2B5P5M2BR0MHKSGF1VJM`, then completed the full short Verilator diagnostic with 100007 instructions at cycle 58875 and no CROB-STUCK or TraceRTL mismatch. This shows that the migrated hardware can run this trace under Verilator, but it does not explain or clear the GSIM watchdog and is not counted as the required GSIM smoke pass. The remaining investigation is simulator/build/initialization-specific.
- INVALID: runsets `20261026-25` and `20261026-28` failed Scala elaboration, then relinked the old model. Their empty FALU-collision logs are not evidence. Runset `20261026-29` passed `make comp` and elaboration and contains the corrected marker, but `20261026-30` segfaulted before a collision record, so no FALU behavior was changed.
- CONFIRMED BUILD DEFECT: `build/gsim-compile/other/gsim.o` still used the old `0x2ce3980` `sizeof(SSimTop)` from October 24, while the regenerated `SimTop.h` requires `0x2ce4b28`. The crash is an indexed write in `subStep411` just beyond the old allocation. `difftest/gsim.mk` had no generated-header dependency, so wrapper objects were not rebuilt when the model layout changed. The `4148d7ed1` / `8350d1ea6` fix makes every GSIM object depend on `SimTop.h`. `make comp` passes and a dry run confirms `gsim.cpp` is rebuilt. The FALU probe is removed; a fresh build must verify matching allocation and model sizes.
- Direct GDB inspection also confirmed `commit_w=0` and packed uop state `0x10` at the watchdog. This supports a genuine writeback/readiness failure, but does not prove that the layout defect is the only cause. Reset and initialization A/Bs were rejected and are not retained.
- FAIL: runset `20261026-37`, task `01M5TP15A8SQ0RMD2W1TMAADBG`, used the freshly linked GSIM binary from runset `20261026-35` with SHA-256 `a78f2a9c71c1ae336a44ea14cddea6a2939853b000f1a6532dcacd5e986b75e1`. It initialized the real GemsFDTD trace, preread 40000000 instructions, and reached instruction 1 before reproducing `[CROB-STUCK] deq=73 valid=1 realDest=3`. The following `gprintf` abort at `SimTop0.cpp:26` is the GSIM formatted-print assertion, not a distinct root-cause diagnosis. This failure persists with matching wrapper/model allocation sizes; no campaign has been dispatched.
- CONFIRMED FORMATTER DEFECT: the generated `gprintf` at `build/gsim-compile/model/SimTop0.cpp:22-26` supports only `%d`, `%c`, and `%x`, whereas `Rob.scala:1197-1198` uses `%b`. The C assertion therefore follows the watchdog print; it does not replace the no-commit diagnosis. A prior diagnostic format-only A/B showed `formerUopNum=0` and `latterUopNum=1`, identifying one outstanding latter-slot writeback rather than proving FALU arbitration loss. The temporary format change remains reverted.
- PASS: runset `20261026-38`, task `01M5TPANFA7837B5SKAN458Z23`, freshly elaborated and built the isolated Verilator emulator in `build-verilator-fst-crob-37/` with `EMU_TRACE=fst` and `CXX=clang++`. `make emu` returned 0; the uncached binary SHA-256 is `cd03104fe3c58202c73fd54579119924b26b9f4375dcbd027d4a77eb21a6653b`.
- LIMITED REFERENCE: the successful Verilator diagnostic's earlier 14000-16000-cycle FST contains PC `0x3b96e` only in the trace-reader frontend, not in any of the 853 examined ROB PC sources. It cannot identify the pending ROB entry or unfinished latter FSUB and supplies no evidence for or against FALU interference. The fresh RTL ownership index is preserved separately from the waveform cache.
- PASS AS VERILATOR DIAGNOSTIC ONLY: runset `20261026-39`, task `01M5TQVNG0Q4C9RG6MZ2MB79BQ`, completed 100007 instructions at cycle 58875 with the fresh runset-38 binary and no watchdog, assertion, or TraceRTL mismatch. It preserved a 113 MB FST covering cycles 14000-30000. This does not clear the GSIM failure or count as the required GSIM smoke pass.
- DIAGNOSTIC STOP: runset `20261026-40`, task `01M5TRCJSGFHDNHXKFVT7N4P5W`, stopped the unchanged runset-35 GSIM binary at `noCommitCycleCnt=14999`, before the formatter abort. Current-model offsets gave `cycle=34082`, `deq=0x73`, `uopState=0x10`, `commit_w=0`, `pc0=0x3b96e`, and `pc1=0x3b972`; the last commit is therefore near cycle 19083, within the new FST window. Featx's success state reflects the intentional debugger stop plus its synthetic cycle-limit marker, not a smoke pass. The first transition watchpoint also required `deq=0x73` and did not fire.
- DIAGNOSTIC STOP: runset `20261026-41`, task `01M5TRJ7EG3BG0ZR9Z18C77FSQ`, recorded the unchanged GSIM model's entry `0x73` transitioning `uopState=0x20 -> 0x10` at cycle 18732 (`deq=0x64`), then becoming head at cycle 19083 (`deq=0x70 -> 0x73`). It remained `0x10` at the cycle-34082 watchdog. Thus one latter writeback arrived and the second did not; the first state watchpoint missed this earlier transition because it incorrectly required the entry already to be head.
- CONFIRMED GOOD REFERENCE: the new FST contains the matching entry with `uopState=0x21` at cycle 18617. It transitions to `0x20` at 18627, `0x10` at 18628, and `0x00` at 18631; the full ROB commit group advances `0x70 -> 0x78` at 18979. Enqueue lanes 3 and 4 identify two latter `FSUB.D` instructions: `0x3b972` / pdest `0x78`, and `0x3b976` / pdest `0x73`. The first writes back through FEX2 and ROB port 21 at 18627; the second through FEX0 and ROB port 19 at 18630. The good trace has `fmulToFaddValid=0`, no flush, correct FEX0 metadata, and no simultaneous-output collision. It proves the expected path but does not by itself prove where GSIM diverges; absolute simulator cycle numbers differ.
- DIAGNOSTIC STOP: runset `20261026-42`, task `01M5TT967VH6180HNBF7NJDCB3`, probed the unchanged GSIM model over cycles 18715-18755 with offsets recomputed from the current header. The first latter FSUB appears in FEX2 with `robIdx=0x73`, `isFormer=0`, pdest `0x78`, and its port-21 writeback precedes the `0x20 -> 0x10` transition. No FEX0 issue/FADD-stage candidate with `robIdx=0x73`, `isFormer=0` appears through cycle 18755, so the second FSUB is not being lost by the already-observed FEX0 result pipeline. The source-1 cancellation at 18734 belongs to a different instruction (`robIdx=0x7c`, former slot, pdest `0x79`) and is not evidence about the missing FSUB. A broader probe must distinguish non-issue from a later wrong-metadata result.
- CONFIRMED NON-ISSUE: runset `20261026-43`, task `01M5TTHXMAKF6RJH6JH7FB1ASH`, extended the same unchanged-binary probe through the watchdog. Only the first latter FSUB (`0x3b972`, pdest `0x78`) appears: FEX2 issue at cycle 18729, FALU result at 18731, and port-21 writeback at 18732. Neither FALU pipe reports a second `robIdx=0x73`, `isFormer=0` candidate through cycle 34082. This places the missing second FSUB before FEX issue rather than in FALU result arbitration. The next investigation is FP issue-queue allocation and source wakeup.
- CONFIRMED QUEUE STALL: runset `20261026-44`, task `01M5V2G8JNWYJB1R09NP3504KZ`, located the second FSUB in the GSIM FP issue queue. It enters `enqEntries(0)` at 18721 and `fastEntries(0)` at 18722 with pdest `0x73`, sources `0x74`/`0x56`, and opcode `0x46`. Source 0 is woken by port 6 at 18725 and by replay port 5 at 18730, becoming ready at 18731. Source 1 instead changes from ready to not-ready at 18724 and remains not-ready through the watchdog at 34082; `issued=0` and `blocked=0` throughout. Thus the replay wakeup for source 0 arrives, but source 1 is unexpectedly cancelled. The good FST keeps source 1 at `bypassDelay=3`, `bypassSource=0`, and ready. The next probe compares source-1 delay/source and the load/FEX cancellation inputs. No RTL was changed.

### Phase 3D: preserve narrow dependency shifts

- DIAGNOSTIC STOP: runset `20261026-45`, task `01M5V37JHQX7T46XA2JTJ2NNAH`, confirmed the exact source-1 cancellation condition in the unchanged GSIM model. At 18722 the target FP queue entry has source 1 with `bypassDelay=1` and `bypassSource=6`; at 18723 it advances to delay 2 while load-cancel 6 is asserted, and source 1 becomes not-ready at 18724. Source 0 receives the replay wakeup at 18730 and is ready at 18731, but source 1 never recovers. The entry remains valid, unissued, and unblocked at the cycle-34082 watchdog. The good FST keeps source 1 at delay 3, source 0, and ready.
- CONFIRMED GSIM WIDTH DEFECT: runset `20261026-46`, task `01M5V4HXNP686XJ00E5EZ3C7K3`, recorded `fpBusyTable.loadDependency[86][2]` values 4 at 18717, 16 at 18722, 32 at 18723, 64 at 18724, and 128 at 18725, despite `LoadDependencyWidth = 2`. Allocation of physical register `0x56` at 18709 starts with zero dependency, so this is not simply a stale value retained across allocation. The good Verilator trace instead has `loadDependency_86_2 = 1` at 18613, 2 at 18614, and 0 at 18615, exactly matching the required two-bit wrap. `BusyTable_1.sv` implements `{old[0], 1'h0}`, whereas the generated GSIM `SimTop177.cpp` assigns an unmasked `uint8_t` left shift to the two-bit next-state. This proves incorrect GSIM lowering in the dependency path and explains the unexpected pending dependency at consumer enqueue.
- PLANNED WORKAROUND: preserve the intended width explicitly in `BusyTable.scala` at both existing shift assignments using `(source << 1)(LoadDependencyWidth - 1, 0)` and `(x << 1)(LoadDependencyWidth - 1, 0)`. Do not patch generated C++ as a deliverable or change cancellation, replay, FALU arbitration, or ROB compression. A fresh GSIM build, generated-mask inspection, and both real trace smoke tests are required before any campaign. If runtime fails again, use a fresh isolated Verilator FST build and waveform analysis.

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
