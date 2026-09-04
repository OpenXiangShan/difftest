# UVHS GBus runtime

This directory contains the UVHS GBus host runtime used by the optional
`DIFFTEST_HOSTIF=GBUS` build.  The files are copied unchanged from the Hejian
U2.2 runtime release installed at:

`/nfs/tools/UVHS/runtime_sw_service/export/gbus_runtime`

The runtime is an external vendor component; the upstream release terms apply
and the binary must not be modified.  The copy is kept in the source tree so a
checked-out MinJie workspace can build without relying on a machine-specific
absolute path.  `install/check_gbus_runtime.sh` verifies the ABI and SHA-256
identifiers before use.

Recorded release artifacts:

| file | SHA-256 | size |
| --- | --- | ---: |
| `include/uvaps_gbus_runtime.h` | `6c321d54a5db25828d24ccf735c598d7e9a5c60c175b2083e30f40a4560db955` | 8,478 |
| `lib/libuvgbus.so` | `c34b59ed9760c8a27c3509dba0174d519c9df6a620d542a53ebd83dfd59e5c49` | 6,402,384 |

Set `GBUS_RUNTIME_ROOT` to this directory (or to an approved site runtime)
when compiling `fpga-host`.
