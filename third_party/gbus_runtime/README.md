# UVHS GBus runtime

This directory contains the UVHS GBus host runtime used by
`DIFFTEST_HOSTIF=GBUS` builds:

- `include/uvaps_gbus_runtime.h`
- `lib/libuvgbus.so`

The shared library is copied unchanged from the approved UVHS U2.2 runtime
release. The header differs only by removal of trailing whitespace. The vendor
license and redistribution terms apply.

`fpga.mk` uses this directory by default. Set `GBUS_RUNTIME_ROOT` to select a
different approved runtime with the same `include/` and `lib/` layout.

| File | SHA-256 | Size |
| --- | --- | ---: |
| `include/uvaps_gbus_runtime.h` | `f10832d93b0cc497748a5c218b3fd72f7592288c5f47bae85949198d0bde5e8c` | 8,470 |
| `lib/libuvgbus.so` | `c34b59ed9760c8a27c3509dba0174d519c9df6a620d542a53ebd83dfd59e5c49` | 6,402,384 |
