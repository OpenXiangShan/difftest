# FAST reference and fork checking

Build flags enable capabilities; main selects `--ref-mode slow|fast|fork`
(default `slow`). FAST parses DUT commits and advances NEMU without full
register comparison. FAST-only can report DUT Good Trap, but does not establish
checked equivalence. Fork children switch to SLOW and run the existing checkers.
Final Good Trap requires a trusted prefix covering the endpoint. Checkpoints
compare state/store hashes, store count and squash stamp. A SLOW checker with
a valid starting prefix and successful DUT checks can replace a divergent FAST
leader using its existing REF/parser state; authoritative checker failure terminates.

Generate normal FPGA headers first; rebuild when changing build flags.
NEMU requires `CONFIG_FAST_REF`; fork also requires enabled store hashing.

```bash
make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 USE_THREAD_MEMPOOL=1
./build/fpga-host --ref-mode fast <normal arguments>

make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 DIFFTEST_FORK=1 USE_THREAD_MEMPOOL=1
./build/fpga-host --ref-mode fork --ref-fork-interval 5 \
  --packet-pool-log2 20 <normal arguments>
```

FORK requires explicit FAST_REF. FAST_REF alone supports SLOW and FAST-only.
`--ref-fork-interval` sets seconds (default 10; 0 means one segment; nonzero
values must be >=3). The existing LightSSS `--fork-interval` is independent.
`--packet-pool-log2` is fork-only: slots = 2^N, default N=20 (1048576 packets).
Final check timeout is 300 seconds.

A long-lived receiver owns XDMA and starts a single-thread FAST leader before
receive/UART threads. FAST constructs SLOW snapshots with Linux COW clone and
CLONE_PARENT, so the receiver reaps all workers directly. Workers independently
parse shared packets. Matching checkers release their final partial packet;
divergent candidates keep it pinned until promotion or discard. The receiver
recycles reader IDs after `waitpid`. Only the prefix below all reader
cursors is reusable; a full pool applies backpressure. Pool bytes = slots *
`sizeof(FpgaPackgeHead)`; driver configuration is independent.

Fork requires XDMA and supports one core/channel and 128 active readers including the leader.
Checkpoint records are recycled after trust and reap, without a lifetime segment
limit. Checker queues requiring history migration are rejected. `FastEndpoint` marks speculative leader
completion; recovery may produce another endpoint. State/store hashes do not guarantee collision-free memory comparison.
