# TraceRTL Kunminghu V3 difftest migration progress

| Phase | Change summary | Build | GemsFDTD smoke | gcc_expr smoke | SPEC06 0.3 |
|---|---|---|---|---|---|
| 1 | Source GSIM wrapper ported as `ee497ea5a` | Not run; Phase 2 pending | Not applicable | Not applicable | Not applicable |
| 2 | GuardedPc elaboration fix committed for next build | PENDING | Not applicable | Not applicable | Not applicable |
| 3 | Validate real trace startup and alignment | Not run | Not run | Not run | Not run |
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

- Completed and committed the independent superproject merge as `7d611ca94`; pinned difftest `661f67887` and TraceRTL `e6d6fc12`.
- Configured Featx with GSIM trace mode, 64 build jobs, one emulator thread, no binary cache reuse, and the collector.
- Added compressed-ROB per-slot trace metadata handling and the user-required Verilator waveform debug path. No additional ChiselDB compatibility change has been needed yet.
- Fixed current API mismatches: ROB `debugMeta(uop).pc`, IFU `PcInit(...).signGuard` and `s2_alignShiftNum`, checker `blockSel`, and removal of an obsolete store `vstart` assignment.
- Migrated `TraceDecodedEntry` and `TracePreDecodeFlatResp` jump offsets to `GuardedPc` and removed the redundant IFU conversion.

### Test results

- FAIL: runset `20261024-1`, build `01M5NQ560Q3SWC9BHCCTYMD1KD`, failed at `XSCache.compile` because ZhuJiang types were absent; no RTL elaboration or runtime test occurred.
- FAIL: runset `20261024-2`, build `01M5NQ943G825T95J9PN2F8G0V`, reached merged Scala and reported nine API errors in ROB, IFU, TracePredictChecker, and NewStoreUnit; no runtime test occurred.
- FAIL: runset `20261024-3`, build `01M5NQK55XKPM97TFDNHB6KTPN`, failed during hardware elaboration at `TracePreDecoder.scala:38` with a 50-bit versus 51-bit address-width mismatch; no runtime test occurred.
- PASS: the jump-offset bundle now uses `GuardedPc` end to end and `git diff --check` passed. A fresh build is pending.

### Issues and debugging

- Root cause of the first failure: the initializer skipped `XSCache/ZhuJiang` as optional, but `XSCache/src/main/scala/zhujiang/perf/XiangShanPerfBackend.scala` requires its types. Initialized the exact gitlink from a local checkout and resubmitted rather than changing RTL.
- Root cause of the second failure: source-side code retained old names and types removed by Kunminghu V3. Migrated each use to the current API instead of restoring the removed interfaces.
- Root cause of the third failure: current branch-offset helpers return 51-bit `GuardedPc`, while the old trace bundle retained a 50-bit `PrunedAddr`. Migrated the bundle type instead of truncating the offset.

## Phase 3

### Changes made

- No changes yet.

### Test results

- GemsFDTD `38797`: not run.
- gcc_expr `4892`: not run.

### Issues and debugging

- No Phase 3 failures recorded.
- Required failure handling: any runtime error triggers a separate Verilator `EMU_TRACE=fst` build, a bounded `--dump-wave` reproduction with an explicit waveform path, and waveform-to-RTL analysis before the campaign can resume.

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
