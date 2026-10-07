# FPGA FAST reference and segmented checking

FAST advances the reference without comparing each DUT register snapshot.
It batches ordinary commits within one squash window, flushes at skips and
architectural events, and requires exactly the requested number of executed
instructions. FAST-only therefore measures speculative execution throughput;
it does not establish DiffTest correctness.

With segmented fork checking, each child inherits the reference and checker
state at a window boundary, switches NEMU to SLOW, and runs the original
checkers. The parent and children independently parse the same shared raw
XDMA packets. No extra packet copy or DiffTestState compression is needed.
The producer retains packets until every active reader has released them.
The slowest reader can still apply backpressure when the pool is full.

## Build and run

Generate the normal FPGA DiffTest headers first, then rebuild the host after
changing feature flags (the host target does not track header/flag changes):

```bash
make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 USE_THREAD_MEMPOOL=1
DIFFTEST_FAST_ONLY=1 ./build/fpga-host <normal workload arguments>

make fpga-build FPGA=1 DIFFTEST_FORK=1 USE_THREAD_MEMPOOL=1
DIFFTEST_RAW_FORK=1 DIFFTEST_RAW_FORK_INTERVAL_MS=5000 \
  DIFFTEST_RAW_POOL_PACKETS=1048576 ./build/fpga-host <normal workload arguments>
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
| `DIFFTEST_RAW_FORK` | off | `1` selects FAST plus forked SLOW authorities |
| `DIFFTEST_SHARED_RAW` | off | `1` uses the shared raw pool even without fork |
| `DIFFTEST_RAW_POOL_PACKETS` | `NUM_BLOCKS` (4096 normally) | Power of two, at least two packet slots |
| `DIFFTEST_RAW_FORK_INTERVAL_MS` | 0 | One segment; nonzero intervals must be at least 3000 ms |
| `DIFFTEST_RAW_FORK_DRAIN_TIMEOUT_MS` | 300000 | Positive final drain timeout; failure/timeout kills and reaps outstanding children |

Pool bytes are packet slots multiplied by `sizeof(FpgaPackgeHead)`, derived
from the generated packet width. The 768-byte experimental packet size is
not fixed in the driver interface. Both original and FIFO XDMA drivers use
ordinary `read(fd, buffer, count)`. No registration ioctl or read-time pool
configuration is added. The receiver accumulates short reads into a complete
packet before publication. Driver descriptor pool settings remain independent.

## Validation and limits

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
segments per run. Completed child readers are released by the parent after
`waitpid`. Segment capacity exhaustion fails the run explicitly.

The tests in [tests/fast-ref](../tests/fast-ref/README.md) exercise real REF
execution, fork checking, failure propagation and the receive loop through
pipes without opening FPGA devices. They do not replace an FPGA/Linux
integration run or establish new performance measurements. Cross-machine
compression/transport is a separate change.
