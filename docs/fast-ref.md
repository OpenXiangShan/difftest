# FPGA FAST reference and fork checking

FAST receives and parses the FPGA stream, then advances NEMU from DUT commits
without comparing every register snapshot. It batches within each squash window,
handles skip/events and rejects incomplete execution. FAST-only reports DUT
Good Trap when execution succeeds; it does not establish checked equivalence.

Fork mode snapshots at window boundaries. Each child switches to SLOW and runs
the existing checkers over its disjoint segment. Parent and children independently
parse shared XDMA packets. Final Good Trap requires every segment to match:
architectural/store digests, store count, squash stamp and complete window coverage.
Only consecutive matches establish a trusted prefix; later matches after a failed
segment are untrusted. Failure terminates the run without rollback.

## Build and run

Generate the normal FPGA headers first. Rebuild after changing feature flags.
NEMU needs `CONFIG_FAST_REF`; fork also needs enabled store-hash protocol 1
(`CONFIG_DIFFTEST_STORE_HASH`). `CONFIG_PERF_OPT_SHARE` is optional.

```bash
make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 USE_THREAD_MEMPOOL=1
DIFFTEST_FAST_ONLY=1 ./build/fpga-host <normal arguments>

make fpga-build FPGA=1 DIFFTEST_FORK=1 USE_THREAD_MEMPOOL=1
DIFFTEST_REF_FORK=1 DIFFTEST_REF_FORK_INTERVAL_MS=5000 \
  DIFFTEST_PACKET_POOL_SLOTS=1048576 ./build/fpga-host <normal arguments>
```

`DIFFTEST_FORK=1` includes FAST support. With both runtime selectors off, the
host uses SLOW and can load an older REF. FAST interfaces are checked once.

| Environment variable | Default | Meaning |
| --- | --- | --- |
| `DIFFTEST_FAST_ONLY` | off | FAST without SLOW checking |
| `DIFFTEST_REF_FORK` | off | FAST with forked SLOW checking |
| `DIFFTEST_SHARED_PACKET_POOL` | off | Shared packet pool without fork |
| `DIFFTEST_PACKET_POOL_SLOTS` | 4096 normally | Power of two, at least 2 slots |
| `DIFFTEST_REF_FORK_INTERVAL_MS` | 0 | One segment; nonzero values must be >=3000 |
| `DIFFTEST_REF_FORK_DRAIN_TIMEOUT_MS` | 300000 | Positive final drain timeout |

## Packet ownership and limits

The shared ring stores XDMA packets, not DiffTestState. Ordinary `read` fills a
slot; short reads accumulate before publication. Each reader releases full
packets as it consumes them. Completed children immediately release their last
partial packet; reader IDs are recycled by the parent after `waitpid`.
Only the prefix below all reader cursors is reusable: adjacent segments can
share a boundary packet and finish out of order. A full pool applies backpressure.
Pool bytes = slots * `sizeof(FpgaPackgeHead)`; driver configuration is independent.

Fork supports one core/channel, 127 child readers and 256 total segments. It
rejects history-dependent checker queues. `FastEndpoint` marks parent completion;
mismatch, receive failure, unexpected child exit or drain timeout makes the final
result fail. State/store digests do not provide collision-free memory comparison.
