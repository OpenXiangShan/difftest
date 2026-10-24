# TraceRTL Kunminghu V3 difftest migration progress

| Phase | Change summary | Build | GemsFDTD smoke | gcc_expr smoke | SPEC06 0.3 |
|---|---|---|---|---|---|
| 1 | Source GSIM wrapper ported as `ee497ea5a` | Not run; Phase 2 pending | Not applicable | Not applicable | Not applicable |
| 2 | Initial independent GSIM build dispatched | RUNNING `01M5NQ560Q3SWC9BHCCTYMD1KD` | Not applicable | Not applicable | Not applicable |
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

### Test results

- RUNNING: runset `tracertl/migrate-kmhv3-kimi/20261024-1`, build task `01M5NQ560Q3SWC9BHCCTYMD1KD`.
- Full `featx doctor` before dispatch: 57 OK, 2 unrelated SPEC26 warnings, 0 failures.
- No elaboration, compile, or link result yet.

### Issues and debugging

- No Phase 2 failures recorded.

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
