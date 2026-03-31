# DiffTest Build, Test & Debug

All commands run from the **project root** (the directory containing `difftest/`).

> For `DifftestConfig` letter meanings (e.g. `E`, `S`, `B`, `D`, …), see [hw-flow.md](hw-flow.md).

## 1. Environment Setup

```bash
export NOOP_HOME=$(pwd)
export WORKLOAD=$NOOP_HOME/ready-to-run/microbench.bin
export REF_SO=$NOOP_HOME/ready-to-run/riscv64-nemu-interpreter-so
```

---

## 2. Build & Run

> **Important:** run `make clean` before each build to avoid stale artifacts.

### 2.1 EMU (Verilator)

The most common software simulation path, using Verilator as the backend.

#### Build

```bash
make clean
make emu -j2
```

Optional build flags:

| Flag | Effect |
|------|--------|
| `NO_DIFF=1` | Compile without difftest support |
| `EMU_TRACE=1` | Enable VCD waveform dump support |
| `EMU_TRACE=fst` | Enable FST waveform dump support (smaller files) |
| `EMU_THREADS=<n>` | Multi-threaded Verilator simulation |
| `DIFFTEST_QUERY=1` | Enable Query DB (SQLite) support |
| `DIFFTEST_PERFCNT=1` | Enable performance counters |
| `MILL_ARGS="--difftest-config <CONFIG>"` | Set DifftestConfig (see [hw-flow.md](hw-flow.md)) |

Examples:

```bash
make clean && make emu NO_DIFF=1 EMU_TRACE=1 -j2
make clean && make emu MILL_ARGS="--difftest-config ESB" DIFFTEST_QUERY=1 -j2
make clean && make emu EMU_THREADS=2 -j2
```

#### Run

```bash
./build/emu -i $WORKLOAD --diff $REF_SO            # difftest with reference model
./build/emu -i $WORKLOAD --no-diff                  # without reference model
./build/emu -i $WORKLOAD --no-diff -C 10000         # limit to 10000 cycles
```

Key runtime options:

| Option | Effect |
|--------|--------|
| `-i, --image=FILE` | Boot image path |
| `--diff=PATH` | Reference SO for differential testing |
| `--no-diff` | Run without difftest comparison |
| `-C, --max-cycles=NUM` | Maximum simulation cycles |
| `-I, --max-instr=NUM` | Maximum instructions |
| `-b, --log-begin=NUM` | Start logging at cycle N |
| `-e, --log-end=NUM` | Stop logging at cycle N |
| `-s, --seed=NUM` | Random seed |
| `--dump-wave` | Dump waveform (from `-b` cycle; requires `EMU_TRACE`) |
| `--wave-path=FILE` | Custom waveform output path |

### 2.2 simv (VCS Top with Verilator Backend)

Uses the VCS-style testbench interface with Verilator as the simulation engine.

#### Build

```bash
make clean
make simv VCS=verilator -j2
```

Optional build flags:

| Flag | Effect |
|------|--------|
| `NO_DIFF=1` | Compile without difftest support |
| `EMU_TRACE=fst` | Enable FST waveform dump support |
| `EMU_THREADS=<n>` | Multi-threaded simulation |
| `DIFFTEST_QUERY=1` | Enable Query DB (SQLite) support |
| `DIFFTEST_PERFCNT=1` | Enable performance counters |
| `SYNTHESIS=1` | Synthesis mode (no DPI-C difftest) |
| `DISABLE_DIFFTEST_RAM_DPIC=1` | Disable RAM DPI-C |
| `DISABLE_DIFFTEST_FLASH_DPIC=1` | Disable Flash DPI-C |
| `WORKLOAD_SWITCH=1` | Enable workload-list switching at runtime |
| `MILL_ARGS="--difftest-config <CONFIG>"` | Set DifftestConfig (see [hw-flow.md](hw-flow.md)) |

Examples:

```bash
make clean && make simv NO_DIFF=1 VCS=verilator -j2
make clean && make simv VCS=verilator EMU_TRACE=fst -j2
make clean && make simv VCS=verilator EMU_THREADS=2 -j2
make clean && make simv MILL_ARGS="--difftest-config <CONFIG>" DIFFTEST_PERFCNT=1 DIFFTEST_QUERY=1 VCS=verilator -j2
```

#### Run

```bash
./build/simv +workload=$WORKLOAD +diff=$REF_SO                    # difftest
./build/simv +workload=$WORKLOAD +no-diff +max-cycles=100000      # no diff, cycle limit
./build/simv +workload=$WORKLOAD +diff=$REF_SO +dump-wave         # dump waveform
```

Key runtime options:

| Option | Effect |
|--------|--------|
| `+workload=FILE` | Boot image path |
| `+diff=PATH` | Reference SO for differential testing |
| `+no-diff` | Run without difftest comparison |
| `+max-cycles=NUM` | Maximum simulation cycles |
| `+max-instrs=NUM` | Maximum instructions |
| `+dump-wave` | Dump waveform (requires `EMU_TRACE=fst` at build) |
| `+b=NUM` | Start logging at cycle N |
| `+e=NUM` | Stop logging at cycle N |
| `+warmup_instr=NUM` | Warmup instructions before stat collection |
| `+workload-list=FILE` | Load workload list (requires `WORKLOAD_SWITCH=1` at build) |

### 2.3 FPGA Sim (FPGA Co-Simulation)

Co-simulation with FPGA host and Verilator testbench running in parallel.

#### Build

Build requires **three steps**: RTL generation, FPGA host build, and simv build.

```bash
# Step 1: Generate RTL with FPGA-suitable config
make clean
make sim-verilog MILL_ARGS="--difftest-config <CONFIG>" -j2

# Step 2: Build FPGA host tool
make -C difftest fpga-build FPGA=1 FPGA_SIM=1 DIFFTEST_PERFCNT=1

# Step 3: Build simv for co-simulation
make simv VCS=verilator WITH_CHISELDB=0 WITH_CONSTANTIN=0 FPGA_SIM=1
```

Optional flags for each step:

| Step | Flag | Effect |
|------|------|--------|
| Step 2 (fpga-build) | `USE_THREAD_MEMPOOL=1` | Enable thread memory pool for FPGA host |
| Step 2 (fpga-build) | `DIFFTEST_PERFCNT=1` | Enable performance counters |
| Step 2 (fpga-build) | `DIFFTEST_QUERY=1` | Enable Query DB (SQLite) support |
| Step 3 (simv) | `ASYNC_CLK_2N=1` | Enable async clock mode |
| Step 3 (simv) | `EMU_TRACE=fst` | Enable FST waveform dump support |

Example with thread pool, async clock, and waveform:

```bash
make -C difftest fpga-build FPGA=1 FPGA_SIM=1 USE_THREAD_MEMPOOL=1 DIFFTEST_PERFCNT=1
make simv VCS=verilator WITH_CHISELDB=0 WITH_CONSTANTIN=0 FPGA_SIM=1 ASYNC_CLK_2N=1 EMU_TRACE=fst
```

#### Run

```bash
bash difftest/scripts/fpga_sim/cosim.sh WORKLOAD=$WORKLOAD DIFF=$REF_SO
bash difftest/scripts/fpga_sim/cosim.sh WORKLOAD=$WORKLOAD DIFF=$REF_SO WAVE=1   # with waveform
```

| Parameter | Effect |
|-----------|--------|
| `WORKLOAD=FILE` | Boot image path (required) |
| `DIFF=PATH` | Reference SO (required) |
| `WAVE=1` | Enable waveform dump |

#### Precautions

- **Residual process check**: before every FPGA Sim run, check for leftover shared memory and processes:
  ```bash
  lsof /dev/shm/xdma_sim* 2>/dev/null
  ```
  If there is output, kill the listed PIDs and remove stale files (`rm -f /dev/shm/xdma_sim*`) before proceeding. Stale processes cause the next run to hang or produce incorrect results silently.

- **Wait for full completion**: always use `bash cosim.sh ... 2>&1 | tee <logfile>` and wait for the script to exit completely before inspecting results. Do not interrupt, background, or `Ctrl-C` mid-run — doing so may leave orphan processes.

- **Log preservation**: use `tee` to save each run's output to a distinct log file. Name logs after the change being tested (e.g. `build/cosim-phaseN-microbench.log`) so they can be compared across runs.

#### Cleanup

```bash
make -C difftest fpga-clean vcs-clean
```

---

## 3. Debugging Artifacts

### 3.1 Console Output (Mismatch Log)

The console output is the **first place** to look when a mismatch occurs. When difftest detects a discrepancy, it prints:

- The **failing checker name** (e.g. `InstrCommit`, `CSRState`)
- The **DUT state** and **REF state** at the point of mismatch
- The **cycle number** of the failure

**How to use:**

1. Note which checker failed from the console output.
2. Locate the checker source in [`src/test/csrc/difftest/checkers/`](../src/test/csrc/difftest/checkers) and read its comparison logic.
3. Inspect the printed DUT vs. REF state to understand which fields diverged.

**Commands by environment:**

| Environment | Command |
|-------------|---------|
| EMU | `./build/emu -i $WORKLOAD --diff $REF_SO` |
| simv | `./build/simv +workload=$WORKLOAD +diff=$REF_SO` |
| FPGA Sim | `bash difftest/scripts/fpga_sim/cosim.sh WORKLOAD=$WORKLOAD DIFF=$REF_SO` |

No special build flags are needed — any build with difftest enabled will produce mismatch logs.

### 3.2 Query DB

Query DB records per-event DiffTest state into a **SQLite database** at `build/difftest_query.db`. It captures every bundle event across the transport pipeline, allowing post-mortem inspection.

**When to use:**

- When a mismatch might be caused by transport reordering, dropped events, or stage-specific transformations (squash/batch/delta).
- Compare DBs from runs with different `--difftest-config` settings to determine which transport stage correlates with the mismatch.
- Compare DBs from different code revisions to find regressions.

**How to use:**

1. Build with `DIFFTEST_QUERY=1`, then run normally. The DB file `build/difftest_query.db` is produced automatically.
2. Open with any SQLite client (e.g. `sqlite3`, DB Browser for SQLite).
3. Each DiffTest bundle type has its own table; query by cycle or event fields.
4. Use the hex conversion script for easier inspection:

```bash
python3 difftest/scripts/query/convert_hex.py build/difftest_query.db
# Produces: build/difftest_query_hex.db
```

Relevant code: [`src/test/csrc/common/query.cpp`](../src/test/csrc/common/query.cpp)

**Build & run by environment:**

| Environment | Build | Run |
|-------------|-------|-----|
| EMU | `make clean && make emu DIFFTEST_QUERY=1 -j2` | `./build/emu -i $WORKLOAD --diff $REF_SO` |
| simv | `make clean && make simv DIFFTEST_QUERY=1 VCS=verilator -j2` | `./build/simv +workload=$WORKLOAD +diff=$REF_SO` |
| FPGA Sim | Add `DIFFTEST_QUERY=1` to Step 2 (fpga-build) in §2.3 | `bash difftest/scripts/fpga_sim/cosim.sh WORKLOAD=$WORKLOAD DIFF=$REF_SO` |

#### Reference DB Comparison

When debugging transport-stage issues (squash/batch/delta), comparing a suspect Query DB against a known-good **reference DB** is the most effective approach.

**Creating a reference DB:**

Run the same workload with difftest on a known-good code revision (or with a simpler `--difftest-config` that bypasses the suspect stage). Save the resulting `build/difftest_query.db` as your reference:

```bash
cp build/difftest_query.db ref-microbench.db   # or ref-linux.db
```

**Hex conversion (required before comparison):**

Query DB stores values in raw format. Convert both the suspect DB and the reference DB to hex for readable comparison:

```bash
python3 difftest/scripts/query/convert_hex.py build/difftest_query.db
# Produces: build/difftest_query_hex.db

python3 difftest/scripts/query/convert_hex.py ref-microbench.db
# Produces: ref-microbench-hex.db
```

**Cross-DB comparison with ATTACH:**

Use SQLite's `ATTACH` to join tables across the suspect and reference DBs in a single query:

```bash
sqlite3 build/difftest_query_hex.db \
  "ATTACH 'ref-microbench-hex.db' AS ref;
   SELECT a.STEP, a.NFUSED AS dut, b.NFUSED AS ref
   FROM main.InstrCommit a
   JOIN ref.InstrCommit b ON a.STEP=b.STEP AND a.MY_INDEX=b.MY_INDEX
   WHERE a.NFUSED != b.NFUSED
   ORDER BY a.STEP LIMIT 20;"
```

Replace the table name (`InstrCommit`) and column (`NFUSED`) with the checker and field that diverged. The first divergent STEP typically points to the root cause.

### 3.3 Waveforms

Waveforms capture hardware signal transitions and are used to inspect timing and event ordering at the RTL level. Build with waveform support enabled, then dump at runtime.

**When to use:**

- After formulating a hypothesis from mismatch logs or Query DB, verify timing and signal behavior.
- Check whether the expected DiffTest probe fired.
- Check whether squash, batch, or delta changed event timing relative to probes.
- Check whether the DUT stopped producing valid events.

**How to use:**

Open the generated `.vcd` or `.fst` file with a waveform viewer (e.g. GTKWave). Navigate to the suspect cycle range and inspect DiffTest probe signals and DUT outputs.

**Build & run by environment:**

| Environment | Build | Run |
|-------------|-------|-----|
| EMU (VCD) | `make clean && make emu EMU_TRACE=1 -j2` | `./build/emu -i $WORKLOAD --dump-wave -b 10 -e 12 --diff $REF_SO` |
| EMU (full) | Same as above | `./build/emu -i $WORKLOAD --dump-wave-full -e 12 --diff $REF_SO` |
| simv (FST) | `make clean && make simv VCS=verilator EMU_TRACE=fst -j2` | `./build/simv +workload=$WORKLOAD +diff=$REF_SO +dump-wave` |
| FPGA Sim | Add `EMU_TRACE=fst` to Step 3 (simv) in §2.3 | `bash difftest/scripts/fpga_sim/cosim.sh WORKLOAD=$WORKLOAD DIFF=$REF_SO WAVE=1` |

EMU runtime waveform options:

| Option | Effect |
|--------|--------|
| `--dump-wave` | Dump from `-b` (log-begin) to `-e` (log-end) cycle |
| `--dump-wave-full` | Dump from cycle 0 to `-e` cycle |
| `--wave-path=FILE` | Custom output path (default: `build/emu.vcd` or `build/emu.fst`) |
| `-b NUM` | Start cycle for waveform dump |
| `-e NUM` | End cycle for waveform dump |

---

## 4. Debug Workflow

1. **Reproduce** the mismatch with `--diff` (or `+diff`) and capture the console output. Note the failing checker name and cycle.
2. **Locate** the checker source in [`src/test/csrc/difftest/checkers/`](../src/test/csrc/difftest/checkers). Read its comparison logic and understand which fields diverged from the printed DUT/REF state.
3. **Query DB** (if transport stages are suspected): rebuild with `DIFFTEST_QUERY=1`, collect `build/difftest_query.db` from at least two runs (e.g. different `--difftest-config` settings or code revisions). Compare the DBs to narrow which transport stage is implicated.
4. **Waveform** (for RTL-level verification): after forming a hypothesis, rebuild with `EMU_TRACE=1` (or `EMU_TRACE=fst` for simv). Dump focused waveforms for the suspect time range to validate timing and signal ordering.

---

## 5. Phased Verification Strategy

When making multi-step changes to the hardware transport pipeline (e.g. modifying Squash, Batch, and Delta together), use a phased approach to isolate regressions early.

### Principles

- **One logical change per phase.** Each phase should modify one module or one aspect of the pipeline. Do not combine unrelated changes in the same phase.
- **Gate on tests before proceeding.** A phase is complete only when all required tests pass. Never start the next phase on a failing baseline.
- **Use a progress log.** Record each phase's changes, test results, and any debugging notes in a dedicated progress file (e.g. `.github/difftest-progress.md`). This provides a clear audit trail and makes it easier to bisect regressions.

### Test Ladder

Run tests in order of increasing cost. Stop at the first failure.

| Step | Test | Pass Criteria | Approx. Time |
|------|------|---------------|---------------|
| 1 | microbench | `HIT GOOD TRAP`, no `ABORT`/`mismatch` | ~1–2 min |
| 2 | linux (short) | No `ABORT`/`mismatch` within a `timeout 300` window | 5 min |
| 3 | linux (long) | No `ABORT`/`mismatch` within a `timeout 600` window | 10 min |

- Step 1 catches most functional regressions quickly.
- Step 2 exercises more complex boot code paths and interrupt handling.
- Step 3 is a final confidence check for timing-sensitive or rare-event issues. Only run after all phases pass Steps 1–2.

### Typical Workflow

1. **Compile** (full three-step build for FPGA Sim, or `make emu` for EMU).
2. **Run microbench.** If it fails, debug and fix before running linux.
3. **Run linux (5 min).** If it fails, the issue is likely related to more complex instruction sequences or interrupt timing.
4. After **all phases pass** Steps 1–2, run the **final 10-min linux test** once as the acceptance gate.
5. Record results in the progress log after each step.
