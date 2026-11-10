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
| `--ref-fork-interval SECONDS` | `5` | Fork interval; `0` checks the entire run as one segment. Independent of LightSSS `--fork-interval`. |
| `--packet-pool-log2 N` | `20` | Allocate `2^N` shared packet slots for fork mode. |

```bash
make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 DIFFTEST_FORK_REF=1 USE_THREAD_MEMPOOL=1
./build/fpga-host --ref-mode fork --ref-fork-interval 5 <normal arguments>
```

`FAST PerfCnt` reports speculative FAST completion before the SLOW drain. On success, `SLOW PerfCnt` uses the last trusted segment's instruction/cycle counts and the global runtime, including the drain. These reports show execution counts, IPC, runtime and speed. On a DUT mismatch, the failing SLOW waits for its predecessors to pass, then prints its own report and commit history before execution stops; failures from discarded snapshots are not reported as authoritative.
