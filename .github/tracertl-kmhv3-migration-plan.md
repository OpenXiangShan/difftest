# TraceRTL Kunminghu V3 difftest migration

## Header

Scope is limited to this isolated `difftest` checkout, its `.github/tracertl-kmhv3-migration-*` records, and the TraceRTL/GSIM compatibility changes required by the superproject migration. Do not modify another worktree, unrelated simulator functionality, NEMU reference behavior, or workload data.

Target configuration: upstream difftest `99f77ff455766d18cd05927b144641bd9744b19a`, GSIM two-step build, `TRACERTL_MODE=1`, DRAMsim3, one emulator thread, and the Featx default GSIM step-2 ChiselDB bridge. The superproject is OpenXiangshan `f85a1f71d` plus `perf-mem-stall` `726a8ac4a` in `tracertl/migrate-kmhv3-kimi`.

Execution requirements: preserve upstream difftest APIs, add source trace/GSIM support from `7f4f14ea49c58add5e6c3fc43dad6b8cad269689`, produce a fresh emulator through Featx, and pass real SPEC06 trace execution. A link produced from stale generated RTL is not a pass.

## Design rationale

The source and target difftest histories diverged. Taking source `7f4f14ea4` wholesale would lose upstream changes through `99f77ff45`; taking only the target would omit TraceRTL initialization, trace arguments, GSIM external-module wrappers, and runtime integration. Cherry-pick the source wrapper onto the target, resolve it against current APIs, and make only build-evidenced compatibility fixes after that.

The superproject supplies `TRACERTL_MODE=1` and the TraceRTL source tree. Difftest must parse `--tracertl-file`, initialize the reader, drive GSIM runtime callbacks, and link generated model support. Recorded NEMU physical addresses are used; `--gen-paddr` and wrong-path emulation are outside scope.

## Prerequisites

```bash
export FEATX_ROOT=/nfs/share/gkc/featx
export PATH=/nfs/home/gongkaichen/tracertl/toolsets/featx/.venv/bin:$PATH
export NODE=tracertl/migrate-kmhv3-kimi
export WT=/nfs/home/gongkaichen/tracertl/featx-wt/$NODE
```

Reference inputs:

- Source wrapper: `/nfs/home/gongkaichen/tracertl/perf-mem-stall/difftest` at `7f4f14ea49c58add5e6c3fc43dad6b8cad269689`.
- Trace manifest: `/nfs/share/gkc/traces/nemu-trace-spec06-v1/_manifest.tsv`.
- Existing attempt logs are diagnostic evidence only; its binary and branch must not be reused.

Set `BUILD_TASK` and `BUILD_RUNSET` from the new Featx build submission record. Do not substitute DeepSeek's task or runset IDs.

Common inspection commands:

```bash
cd "$WT/difftest"
git status --short --branch
git diff --check
featx ps -a --node "$NODE" --limit 30
featx logs "$BUILD_TASK" -n 120
```

For a failure, read the last approximately 100 log lines first and identify the first elaboration, compile, link, assertion, or trace-checker error. Escalate to generated-source inspection only after the console evidence identifies the failing interface. Record every failed attempt in the progress document.

## Phase 1: port the source GSIM wrapper

Files in scope: `Makefile`, `src/test/csrc/common/args.cpp`, `src/test/csrc/common/args.h`, `src/test/csrc/emu/emu.cpp`, `src/test/csrc/gsim/gsim.cpp`, and directly related headers if the merge requires them.

Create the local branch, fetch the source commit from the local source checkout, and cherry-pick it onto upstream `99f77ff45`:

```bash
cd "$WT/difftest"
git switch -c tracertl/migrate-kmhv3-kimi
git fetch /nfs/home/gongkaichen/tracertl/perf-mem-stall/difftest 7f4f14ea49c58add5e6c3fc43dad6b8cad269689
git cherry-pick 7f4f14ea49c58add5e6c3fc43dad6b8cad269689
git diff --check 99f77ff455766d18cd05927b144641bd9744b19a..HEAD
git diff --stat 99f77ff455766d18cd05927b144641bd9744b19a..HEAD
```

Expected behavior: upstream difftest history remains the base and the TraceRTL/GSIM wrapper is present without unrelated reversions.

PASS: cherry-pick conflicts are resolved, `git diff --check` succeeds, trace argument/runtime changes are present, and no conflict markers remain. FAIL: any unresolved conflict, wholesale reversion of upstream files, or missing trace/GSIM path; resolve before building.

## Phase 2: pass a fresh superproject GSIM build

Additional files in scope only when console/link evidence requires them: `gsim.mk`, `Makefile`, `src/test/csrc/gsim/gsim.cpp`, and generated-runtime bridge sources already used by the current build system.

After the superproject merge is reviewed:

```bash
cd "$WT"
featx recipe set "$NODE" build tool=gsim mode=trace cores=64 gsim.jobs=64 options.threads=1 options.dramsim3=true options.chiseldb=false cache=false
featx recipe set "$NODE" run workload=spec06 mode=trace coverage=0.3 W=20000000 I=40000000 cores_per_point=1
featx recipe set "$NODE" post steps.collect.plugin=collect
featx doctor
featx submit "$NODE" build -m "Independent TraceRTL migration onto f85a1f71d"
featx ps -a --node "$NODE" --limit 20
featx wait "$BUILD_TASK" --timeout 36000
featx logs "$BUILD_TASK" -n 120
```

If the build fails, change only the identified compatibility interface and repeat this phase in a new Featx runset. Do not accept an emulator left by an earlier attempt.

PASS: Featx build state is succeeded; Scala elaboration and GSIM generation completed in this build, the difftest link produced a fresh `build/emu`, and provenance records this submodule HEAD. FAIL: Scala errors, stale generated artifacts, unresolved symbols, missing trace runtime objects, or a failed task state.

## Phase 3: pass real trace smoke runs

Use reduced instruction limits for diagnosis and real trace files. `run.coverage=null` is required with an explicit point.

```bash
cd "$WT"
featx submit "$NODE" run --bin "$NODE/$BUILD_RUNSET" --point GemsFDTD_38797_0.210512 --set run.coverage=null --set run.W=0 --set run.I=100000 -m "TraceRTL startup and ROB-progress smoke"
featx ps -a --node "$NODE" --limit 20
featx submit "$NODE" run --bin "$NODE/$BUILD_RUNSET" --point gcc_expr_4892_0.159615 --set run.coverage=null --set run.W=0 --set run.I=100000 -m "TraceRTL compressed-alignment smoke"
featx ps -a --node "$NODE" --limit 20
```

Inspect each point's `simulator_out.txt` and `simulator_err.txt` through `featx logs` with its run task ID and `-j POINT`.

PASS: both jobs complete normally, initialize the real trace, advance beyond instruction 1 and the 15000-cycle watchdog, and have no assertion, stuck watchdog, or TraceRTL mismatch. FAIL: any abort, no-commit watchdog, checker mismatch, missing trace input, or infrastructure error. A failure blocks the full campaign and enters Phase 3A.

## Phase 3A: reproduce runtime failures with Verilator waveforms

For any runtime error after implementation and building, keep the failing GSIM runset and dispatch a fresh Verilator build with FST waveform support. Use the dedicated `build-verilator-fst` directory, not the GSIM `build` directory, so RTL and generated sources are recreated. The single-element artifact list prevents fallback to an old GSIM emulator:

```bash
cd "$WT"
featx submit "$NODE" build \
  --set build.tool=verilator \
  --set build.cache=false \
  --set build.options.flags.EMU_TRACE=fst \
  --set build.options.flags.BUILD_DIR="$WT/build-verilator-fst" \
  --set "build.artifact=[build-verilator-fst/emu]" \
  --set build.verilator.args=CXX=clang++ \
  -m "Waveform reproduction of TraceRTL runtime failure"
featx ps -a --node "$NODE" --limit 30
```

Both Makefiles export the absolute `BUILD_DIR` as `DIFFTEST_BUILD_DIR`; `common/FileControl.scala` uses it for generated support files and retains `NOOP_HOME/build/generated-src` as the fallback when unset. Do not copy or reuse the GSIM `build/generated-src` directory.

Set `WAVE_TASK` and `WAVE_RUNSET` from this new submission, then wait with `featx wait "$WAVE_TASK" --timeout 36000`. Reproduce the same point with `run.coverage=null` and run arguments that enable `--dump-wave`, focused `-b`/`-e` cycle bounds, and `--wave-path` under the new run directory. Keep the waveform and exact runset as evidence.

PASS: the Verilator run reproduces the failure inside a bounded FST capture and the relevant IFU/ROB/trace handshake signals can be correlated with the owning Scala RTL. Use the waveform to form or reject a concrete fix hypothesis, then return to Phase 2 and repeat Phase 3. FAIL: no waveform, a different failure without an explanation, or a waveform that cannot be bounded to the failing interval; adjust capture bounds and repeat before any full campaign.

## Final verification: SPEC06 0.3 coverage

Run the full campaign only after Phases 1-3 pass with the same recorded binary:

```bash
cd "$WT"
featx submit "$NODE" run post --bin "$NODE/$BUILD_RUNSET" --set run.coverage=0.3 -m "SPEC06 0.3 coverage TraceRTL validation"
featx ps -a --node "$NODE" --limit 30
featx result "$NODE/$BUILD_RUNSET" --json
```

Use the actual new run runset for `featx result` if Featx assigns a different name. Inspect `run/points.json`, per-point completion logs, and `post/collect/summary.json`.

Exit criteria: exactly 143 configured points across 55 benchmarks, 143 successful Featx jobs, `completed_points: 143`, and no hidden assertion or TraceRTL mismatch in the logs. Any active or failed count is reported explicitly and is not called complete.
