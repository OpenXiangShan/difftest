# FAST/fork software checks

Run from the DiffTest root after generating the FPGA C++ headers. The runtime
fixture expects one core, squash and integer/FP/vector architectural states,
with fused commits and physical integer register probes (the KMHV2 ESBIFDU
profile used by the experiment). History-dependent checker queues must be off.
The test program is a RISC-V nop followed by an add/store/jump loop, with 100
nonzero store effects; it calls the real FAST and SLOW checkers and REF APIs.

```bash
make -f tests/fast-ref/Makefile fast-ref-pool-test FPGA=1
make -f tests/fast-ref/Makefile fast-ref-xdma-test FPGA=1 \
  USE_THREAD_MEMPOOL=1 USE_SERIAL_PORT=0 WITH_DRAMSIM3=0 \
  WITH_CHISELDB=0 WITH_CONSTANTIN=0
make -f tests/fast-ref/Makefile fast-ref-runtime-test FPGA=1 \
  DIFFTEST_FORK=1 USE_THREAD_MEMPOOL=1 USE_SERIAL_PORT=0 \
  WITH_DRAMSIM3=0 WITH_CHISELDB=0 WITH_CONSTANTIN=0
python3 tests/fast-ref/run.py --perf /path/to/fast-perf.so \
  --plain /path/to/fast-plain.so --old /path/to/old-slow.so \
  --hash-disabled /path/to/fast-without-store-hash.so
```

The four NEMU libraries respectively enable FAST_REF + STORE_HASH +
PERF_OPT_SHARE; FAST_REF + STORE_HASH; no FAST_REF; and FAST_REF without
STORE_HASH. Keep the same ISA/register configuration across libraries.
Additional STORE_LOG rollback support is unnecessary.

The runner records logs and JSON results under `build/fast-ref-tests`.
It verifies ordinary SLOW with old exports, FAST exact execution and skip,
short REF execution rejection, missing API/hash rejection, nonzero-store
fork matching, hash/stamp mismatch, unexpected exit, timeout, multiple
segments, out-of-order completion and distrust after an earlier failure.
Pool tests use a real fork to verify retained packet bytes, publication,
wraparound, reader saturation and parent-owned reader retirement.
XDMA tests replace device `open` with a pipe, exercising the real receiver
and parser loop with partial packets, truncated EOF and blocked-read shutdown.
No FPGA device or serial port is touched.
