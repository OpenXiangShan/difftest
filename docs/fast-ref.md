# FAST reference and fork checking

Build flags enable capabilities; main selects `--ref-mode slow|fast|fork`
(default `slow`). FAST parses DUT commits and advances NEMU without full
register comparison. FAST-only can report DUT Good Trap, but does not establish
checked equivalence. Fork children switch to SLOW and run the existing checkers.
Final Good Trap waits for all segments to match state/store hashes, store count
and squash stamp. Only consecutive matches are trusted; failure terminates.

Generate normal FPGA headers first; rebuild when changing build flags.
NEMU requires `CONFIG_FAST_REF`; fork also requires enabled store-hash protocol 1.

```bash
make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 USE_THREAD_MEMPOOL=1
./build/fpga-host --ref-mode fast <normal arguments>

make fpga-build FPGA=1 DIFFTEST_FAST_REF=1 DIFFTEST_FORK=1 USE_THREAD_MEMPOOL=1
./build/fpga-host --ref-mode fork --fork-interval 5 \
  --packet-pool-slots 1048576 <normal arguments>
```

FORK requires explicit FAST_REF. FAST_REF alone supports SLOW and FAST-only.
`--fork-interval` reuses the existing seconds option (default 10; 0 means one
segment; nonzero values must be >=3). `--packet-pool-slots` is fork-only: a power
of two >=2, default `NUM_BLOCKS` (normally 4096). Final check timeout is 300 seconds.

Parent and children independently parse shared XDMA packets. Each reader releases
consumed packets; completed children also release their final partial packet.
The parent recycles reader IDs after `waitpid`. Only the prefix below all reader
cursors is reusable; a full pool applies backpressure. Pool bytes = slots *
`sizeof(FpgaPackgeHead)`; driver configuration is independent.

Fork supports one core/channel, 127 child readers and 256 total segments; checker
queues requiring history migration are rejected. `FastEndpoint` marks parent
completion. State/store hashes do not guarantee collision-free memory comparison.
