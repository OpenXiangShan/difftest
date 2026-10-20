# DiffTest Plugins

DiffTest includes optional tools that improve simulation debugging.

## LightSSS Snapshots

LightSSS periodically saves a lightweight simulation snapshot. If the simulation fails, DiffTest restores the most recent snapshot and reruns the region of interest with debugging information enabled.

Enable it at runtime:

```bash
./build/emu -i "$WORKLOAD" --diff "$REF_SO" --enable-fork --fork-interval 10
```

`--fork-interval` is measured in seconds. Typical values range from 1 second for a small design to 30 seconds for a large design.

Do not combine `--enable-fork` with `-b`, `-e`, `--dump-wave`, `--dump-ref-trace`, or other explicit debug-window options. The combined behavior is undefined.

For the original design, see the [MICRO'22 paper](https://xiangshan-doc.readthedocs.io/zh-cn/latest/tutorials/publications/#towards-developing-high-performance-risc-v-processors-using-agile-methodology).

## spike-dasm

`spike-dasm` turns the recently committed instruction stream in a mismatch report into disassembly. DiffTest finds it through `PATH`.

Install it from the [riscv-isa-sim project](https://github.com/riscv-software-src/riscv-isa-sim?tab=readme-ov-file#build-steps). Install under a user-owned prefix to avoid requiring root:

```bash
./configure --prefix="$HOME/.local"
make -j"$(nproc)"
make install
export PATH="$HOME/.local/bin:$PATH"
```

Verify installation:

```bash
spike-dasm --help
```

If `spike-dasm` is not in `PATH`, DiffTest still reports raw instructions but cannot add disassembly.

## Related Documentation

- [Runtime options](runtime-options.md)
- [Debug a mismatch](debugging.md)
