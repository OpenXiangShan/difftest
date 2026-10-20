# Run DiffTest EMU and simv

Use this guide when a CPU project has already integrated DiffTest and produced an executable such as `build/emu` or `build/simv`.

## Prerequisites

- A DiffTest-enabled executable. The default EMU path produces `build/emu`.
- A workload image, such as `microbench.bin`, `linux.bin`, or an ELF file.
- A reference-model shared library for differential checking, such as `riscv64-nemu-interpreter-so`.
- Permission to execute the files and read the image and reference model.

The examples below use these environment variables:

```bash
export NOOP_HOME=/path/to/integrated/project
export WORKLOAD=$NOOP_HOME/ready-to-run/microbench.bin
export REF_SO=$NOOP_HOME/ready-to-run/riscv64-nemu-interpreter-so
```

`NOOP_HOME` normally points at the host project root that contains both its `Makefile` and `difftest/` directory.

## Run EMU

Run MicroBench with a reference model:

```bash
$NOOP_HOME/build/emu -i "$WORKLOAD" --diff "$REF_SO"
```

Run without differential checking:

```bash
$NOOP_HOME/build/emu -i "$WORKLOAD" --no-diff
```

Bound a run by cycle count:

```bash
$NOOP_HOME/build/emu -i "$WORKLOAD" --no-diff -C 10000
```

Show the complete runtime help for the current executable:

```bash
$NOOP_HOME/build/emu --help
```

### Expected Result

A successful workload normally prints a test result and then `HIT GOOD TRAP`. For the bundled MicroBench image, the final lines also include instruction count, cycle count, and performance counters.

A differential mismatch prints the failing checker and the DUT/reference values before the simulation stops. Continue with [Debug a mismatch](debugging.md).

## Run simv

`simv` uses the VCS-style testbench argument syntax with a Verilator, VCS, or other supported simulator backend.

```bash
$NOOP_HOME/build/simv +workload="$WORKLOAD" +diff="$REF_SO"
$NOOP_HOME/build/simv +workload="$WORKLOAD" +no-diff +max-cycles=100000
```

The most common argument differences are:

| Purpose | EMU | simv |
|---------|-----|------|
| Workload | `-i FILE` | `+workload=FILE` |
| Reference model | `--diff=PATH` | `+diff=PATH` |
| Disable checking | `--no-diff` | `+no-diff` |
| Cycle limit | `-C NUM` | `+max-cycles=NUM` |
| Instruction limit | `-I NUM` | `+max-instrs=NUM` |
| Waveform | `--dump-wave` | `+dump-wave` |

The exact options depend on build-time features. See [Runtime options](runtime-options.md) for the shared reference.

## Workload Images

A plain binary is loaded into linear memory starting at `0x8000_0000` by default. DiffTest also accepts:

- gzip-compressed images, detected by their magic bytes.
- zstd-compressed images, detected by their magic bytes.
- ELF images, loaded according to their ELF segments.
- Footprint images, loaded on demand through `--as-footprints`.

Some builds disable optional formats for size or platform reasons. The related build options are `IMAGE_GZ_COMPRESS=0`, `NO_ZSTD_COMPRESSION=1`, and `IMAGE_ELF=0`.

## Output and Logs

The console is the first place to inspect a run. Important output includes:

- DUT UART output.
- DiffTest initialization and reference-model messages.
- The first differential mismatch, including the checker name and state values.
- Trap status and final performance counters.

Interactive splitview builds also write `build/logs/uart.log`, `build/logs/host.log`, and `build/logs/all.log`. Use `--splitview-log=PATH` to choose another log directory.

## Related Documentation

- [Runtime options](runtime-options.md)
- [Debug a mismatch](debugging.md)
- [Plugins](plugins.md)
- [Software checking flow](../developers/software-check.md)
