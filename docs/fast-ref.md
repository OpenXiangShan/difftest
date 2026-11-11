# FAST reference and fork checking

Fork checking accelerates differential testing by running a FAST REF ahead of multiple SLOW checkers that verify separate execution segments in parallel. Linux copy-on-write (COW) snapshots preserve each segment's starting REF state without eagerly copying its memory; pages are copied only when written. XDMA packets remain shared between processes. Segment results become trusted in execution order, so a segment is accepted only after all preceding segments pass.

- **Receiver / supervisor:** Receives XDMA packets into the shared pool, manages REF processes and accepts segment results in order. Final Good Trap requires the trusted segments to cover the entire run.
- **FAST leader:** Advances NEMU in batches using DUT commits and synchronization events, without full register comparison. It periodically forks a SLOW checker and records endpoint state/store hashes and execution position for comparison. FAST-only can report DUT Good Trap, but does not establish checked equivalence.
- **SLOW checkers:** Each replays one segment from its snapshot and runs the existing DUT checks. Passing DUT checks and matching endpoint hashes and execution position allow the segment to become trusted. If DUT checks pass but the endpoint differs from FAST, the supervisor waits until the starting prefix is trusted, stops the old FAST leader, discards later segments and promotes this SLOW process to FAST at its verified endpoint. A DUT mismatch from a trusted starting state terminates execution with diagnostics.

Fork currently supports one core over XDMA, including multiple DMA channels. Profiles with checker queues that require history migration are rejected. Shared packets remain available until their readers finish; a full pool applies backpressure.

## Build options

Generate normal FPGA headers first and rebuild when changing build flags. NEMU requires `CONFIG_FAST_REF`; fork also requires enabled store hashing.

| Option | Purpose |
|---|---|
| `FPGA=1` | Build for FPGA execution. |
| `DIFFTEST_FAST_REF=1` | Enable runtime selection of SLOW or FAST. |
| `DIFFTEST_FORK_REF=1` | Enable fork checking; requires `DIFFTEST_FAST_REF=1` and `USE_THREAD_MEMPOOL=1`. |
| `USE_THREAD_MEMPOOL=1` | Enable the XDMA receive pool. |
| `DIFFTEST_PERFCNT=1` | Print performance statistics. |

## Runtime options

| Argument | Default | Purpose |
|---|---|---|
| `--ref-mode slow\|fast\|fork` | `fork` when compiled with FORK_REF, otherwise `slow` | Select REF execution mode. |
| `--ref-fork-interval SECONDS` | `1` | Time-based segment interval. `0` disables time-based cuts. Independent of LightSSS `--fork-interval`. |
| `--packet-pool-log2 N` | `22` | Allocate `2^N` shared packet slots for fork mode; 3 GiB with 768-byte packets. |

```bash
make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 DIFFTEST_FORK_REF=1 USE_THREAD_MEMPOOL=1
./build/fpga-host --ref-mode fork --ref-fork-interval 1 --packet-pool-log2 22 <normal arguments>
```

`FAST PerfCnt` reports speculative FAST completion before the checkers finish. On success, `SLOW PerfCnt` includes checker completion and uses the last trusted segment's instruction and cycle counts. Time through host completion to include writer drain. On a DUT mismatch, the failing SLOW waits for its predecessors to pass before printing its report and commit history.

## Periodic checkpoints

`--take-cpt-interval=5M` requests a checkpoint every five million DUT instructions. The default `-1` disables saving.
Output is stored in a per-run directory under `$NOOP_HOME/build/<timestamp>-ref-cpt-<id>/`.
Use a single-core FPGA XDMA host built with `DIFFTEST_FORK_REF=1 USE_THREAD_MEMPOOL=1`.
The REF must export `difftest_trigger_checkpoint`, save restore state in PMEM, and write zstd
([NEMU PMEM change](https://github.com/OpenXiangShan/NEMU/pull/1281) and
[NEMU zstd change](https://github.com/OpenXiangShan/NEMU/pull/1282)).
Serialization writes restore metadata into that REF's PMEM, so a REF that will continue executing needs
a separate COW writer.

Completed files are named `periodic-instr<actual-instr>.zstd`. During serialization, the REF writes
`periodic-instr<actual-instr>.partial-<pid>_memory_.zstd`; after the writer exits successfully,
the host renames it to the completed filename.

In SLOW/FAST mode, the host forks a single-threaded parser before starting XDMA receiver threads. The parser
executes REF and creates COW writer children. SLOW saves after checking; FAST flushes deferred execution
and synchronizes its register cache before saving. FAST output is labeled `FAST_ONLY` and does not
establish SLOW-checked equivalence. The host stops receiving at the parser endpoint and waits for writers
before reporting the result. RefDriveTrace with periodic saving requires FORK mode.
The asynchronous AMU verifier is unsupported with periodic saving.

In FORK mode, checkpoint interval crossings also close execution segments, independently of
`--ref-fork-interval`. A completed SLOW checker keeps its endpoint alive until all predecessors are trusted.
The supervisor then selects one of two save paths, without creating another parser:

| Accepted result | Serializer process | REF continuation |
|---|---|---|
| MATCH | The existing checker becomes the writer | Exits after serialization |
| DIVERGED | The checker creates a separate COW sibling | The original checker can be promoted to FAST |

For MATCH, the supervisor registers the checker PID with the writer manager before sending `SAVE`.
After any final-endpoint statistics, the checker copies the checkpoint path and releases its packet reader.
It publishes `WRITING` as its last segment access, then serializes and exits. `WRITING` permits segment
reuse while serialization is still running; it does not mean the file is complete. The writer manager
keeps the PID and path independently of the segment and exclusively reaps that process.

For DIVERGED, the checker reports its COW sibling PID and waits for the supervisor to register it before
accepting `PROMOTE`. This keeps serializer metadata writes out of the checker that will continue as FAST.
The supervisor's own REF is never used for saving; discarded speculative segments do not save.

`FPGA_CPT_WRITERS` limits concurrent registered writers (default 16), across both FORK save paths.
`FPGA_CPT_KEEP` retains the newest completed checkpoints by instruction endpoint (default 20).
Both settings accept positive integers.
Publication and retention happen after successful writer exit; shutdown drains all registered writers
and propagates serialization or publication errors. Packet-reader lifetime, segment reuse and writer
capacity are managed separately, so compression does not keep a MATCH segment or packet reader occupied.

FORK checkpoints are labeled `SLOW_ACCEPTED`. The instruction count comes from the last executed SLOW
window, even if the parser has loaded the next window. Boundaries can overshoot the requested interval
by a commit group; filenames report the actual endpoint. M-mode boundaries are skipped because the REF
checkpoint export returns without producing a file. Saving resumes at a later interval boundary.
Checkpoints retain the selected profile's existing checker coverage limits.

Instruction-budget endpoints drain the verified prefix and writers while remaining `EXCEED` (host exit 1);
they are not GoodTrap or workload completion.

`--ila-size N` (default `0`, disabled) limits DUT cycles ahead of the SLOW-checked base, which Host advances after successful checking.
