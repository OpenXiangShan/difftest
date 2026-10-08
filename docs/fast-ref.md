# FPGA FAST reference and segmented checking

FAST advances the reference without comparing each DUT register snapshot.
It batches ordinary commits within one squash window, flushes at skips and
architectural events, and requires exactly the requested number of executed
instructions. FAST-only therefore measures speculative execution throughput;
it does not establish DiffTest correctness. It still receives and parses the
FPGA stream, advances NEMU according to the DUT commits, and reports the usual
`HIT GOOD TRAP` when the DUT trap packet indicates successful completion and
FAST execution has not failed. It does not wait for any SLOW checker.

With segmented fork checking, each child inherits the reference and checker
state at a window boundary, switches NEMU to SLOW, and runs the original
checkers. The parent and children independently parse the same shared
XDMA packets. No extra packet copy or DiffTestState compression is needed.
Each reader releases every fully consumed packet by advancing its cursor;
release does not wait for the entire segment. A completed child immediately
stops retaining its last partial packet, while its reader ID remains reserved
until the parent reaps it. The producer reuses only the contiguous prefix below
all reader cursors. The slowest reader can still apply backpressure when the
pool is full.

## Build and run

Generate the normal FPGA DiffTest headers first, then rebuild the host after
changing feature flags (the host target does not track header/flag changes):

```bash
make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 USE_THREAD_MEMPOOL=1
DIFFTEST_FAST_ONLY=1 ./build/fpga-host <normal workload arguments>

make fpga-build FPGA=1 DIFFTEST_FORK=1 USE_THREAD_MEMPOOL=1
DIFFTEST_REF_FORK=1 DIFFTEST_REF_FORK_INTERVAL_MS=5000 \
  DIFFTEST_PACKET_POOL_SLOTS=1048576 ./build/fpga-host <normal workload arguments>
```

`DIFFTEST_FORK=1` also compiles FAST support. Without either runtime selector,
the compiled host uses the existing SLOW path and can load an older REF.
FAST capability symbols are optional at load time and checked once on the
first FAST step. Missing capabilities produce an error.

The NEMU REF needs `CONFIG_FAST_REF`. Fork checking additionally needs enabled
store-hash protocol version 1 (`CONFIG_DIFFTEST_STORE_HASH`); optional
`CONFIG_PERF_OPT_SHARE` accelerates FAST while retaining SLOW checking.
The required APIs are mode selection, instruction count, PC and skip for
FAST; state flushing, state/store digest and hash capability queries for fork.
Store effects are accumulated incrementally by NEMU. Boundary comparisons
cover architectural state, accumulated store effects and squash commit stamp;
they do not compare every byte of memory or provide collision-free proofs.

| Setting | Default | Meaning |
| --- | --- | --- |
| `DIFFTEST_FAST_ONLY` | off | `1` selects FAST without slow authorities |
| `DIFFTEST_REF_FORK` | off | `1` selects FAST plus forked SLOW authorities |
| `DIFFTEST_SHARED_PACKET_POOL` | off | `1` uses the shared packet pool even without fork |
| `DIFFTEST_PACKET_POOL_SLOTS` | `NUM_BLOCKS` (4096 normally) | Power of two, at least two packet slots |
| `DIFFTEST_REF_FORK_INTERVAL_MS` | 0 | One segment; nonzero intervals must be at least 3000 ms |
| `DIFFTEST_REF_FORK_DRAIN_TIMEOUT_MS` | 300000 | Positive final drain timeout; failure/timeout kills and reaps outstanding children |

`SharedPacketPool` stores unparsed XDMA packets in a process-shared sequential
ring with one producer and multiple readers. The original `MemoryIdxPool`
(`indexed_packet_pool` in the receiver) also stores XDMA packets, but indexes
and reorders them for the existing single-process path. Neither stores parsed
DiffTestState. REF fork automatically selects the shared pool.

Slow segments check disjoint squash windows. Adjacent segments can nevertheless
share a boundary packet because fork occurs inside packet parsing. Moreover,
later segments can finish before earlier ones. Both properties require keeping
per-reader cursors rather than freeing packets by the latest completed segment.

Pool bytes are packet slots multiplied by `sizeof(FpgaPackgeHead)`, derived
from the generated packet width. The 768-byte experimental packet size is
not fixed in the driver interface. Both original and FIFO XDMA drivers use
ordinary `read(fd, buffer, count)`. No registration ioctl or read-time pool
configuration is added. The receiver accumulates short reads into a complete
packet before publication. Driver descriptor pool settings remain independent.

## Completion and limits

Each child checks its segment independently. Only consecutive successful
segments beginning at window zero establish a trusted prefix. A locally
matching descendant of a failed segment is reported as `UNTRUSTED`.
`FastEndpoint` identifies parent completion; final Good Trap is reported only
after all children have been reaped and every segment has matched. Mismatch,
unexpected child exit, receive failure and drain timeout produce failure.
These paths terminate the run; this implementation does not roll back or
replay failed speculative descendants.

This preview supports a single core and a single DMA channel. It rejects
checker configurations with history-dependent load/store, delayed update,
matrix, CMO/MSYNC, refill or TLB queues rather than silently inheriting
incomplete checkpoints. There are 127 child reader slots and 256 total
segments per run. Completed children stop retaining payload immediately; their reader IDs
are recycled by the parent after `waitpid`. Segment capacity exhaustion fails the run explicitly.

Cross-machine compression/transport is a separate change.
