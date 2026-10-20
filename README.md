# DiffTest

DiffTest is a co-simulation and differential-testing framework for RISC-V processors. It connects probe bundles emitted by a design under test (DUT) to a reference model, checks architectural state, and transports events efficiently across RTL simulation, FPGA, and emulation workflows.

**Current compatibility:** Chisel `7.14.0` with the MLIR FIRRTL compiler. Support for Chisel `3.6.1` and older versions has been removed.

## Choose Your Path

| You want to... | Start here |
|----------------|------------|
| Run an existing DiffTest-built emulator | [For emulator users](#for-emulator-users) |
| Connect DiffTest to your own CPU | [For CPU integrators](#for-cpu-integrators) |
| Change DiffTest internals | [For DiffTest developers](#for-difftest-developers) |

The documentation is organized by audience:

```text
docs/
├── users/          # Run, configure, and debug an existing emulator
├── integration/    # Connect DiffTest to a CPU or generate standalone interfaces
└── developers/     # Understand and modify DiffTest internals
```

## For Emulator Users

Use this path when your CPU project already integrates DiffTest and produces an executable such as `build/emu` or `build/simv`. You need the executable, a workload image, and optionally a reference-model shared library.

### Quick Start

```bash
export NOOP_HOME=/path/to/integrated/project
export WORKLOAD=$NOOP_HOME/ready-to-run/microbench.bin
export REF_SO=$NOOP_HOME/ready-to-run/riscv64-nemu-interpreter-so

$NOOP_HOME/build/emu -i "$WORKLOAD" --diff "$REF_SO"
```

A successful MicroBench run exits normally and reports `HIT GOOD TRAP`. A mismatch identifies the failing checker and prints the DUT and reference state.

### Common Runtime Options

| Option | Purpose |
|--------|---------|
| `-i, --image=FILE` | Select the workload image |
| `--diff=PATH` | Enable checking with a reference-model `.so` |
| `--no-diff` | Run the DUT without differential checking |
| `-C, --max-cycles=NUM` | Stop after a cycle limit |
| `-I, --max-instr=NUM` | Stop after an instruction limit |
| `-b, --log-begin=NUM` | Start detailed logging at cycle `NUM` |
| `-e, --log-end=NUM` | Stop detailed logging at cycle `NUM` |
| `-s, --seed=NUM` | Set the random seed |
| `--dump-wave` | Dump the configured logging window as a waveform |

Run the executable with `--help` for the complete option list available in that build.

### User Documentation

- [Run EMU and simv](docs/users/running-emu.md): image handling, reference models, and expected results.
- [Runtime options](docs/users/runtime-options.md): common options grouped by task, plus EMU/simv syntax differences.
- [Debug a mismatch](docs/users/debugging.md): console logs, Query DB, and waveforms.
- [Plugins](docs/users/plugins.md): LightSSS snapshots and `spike-dasm` disassembly.

## For CPU Integrators

Use this path when adapting DiffTest to a new RISC-V CPU. Chisel is the supported integration path. The generated C++ runtime and DPI-C bridge are derived from the Chisel probe definitions, so the Scala and C++ sides stay aligned automatically.

### Integration at a Glance

1. Add DiffTest as a Git submodule.
2. Add DiffTest as a Mill module and expose an `emu` Make target.
3. Instantiate the required and optional `Diff*` probe bundles.
4. Wrap the CPU with `DifftestModule.top` and implement `HasDiffTestInterfaces` when using the current Chisel API.
5. Connect the simulation memory path, either through `DifftestMem` DPI-C or the FPGA AXI interface.
6. Generate RTL, build the emulator, and verify with MicroBench before booting Linux.

### Minimal Chisel Setup

Add the submodule and Mill module:

```bash
git submodule add https://github.com/OpenXiangShan/difftest.git
```

```scala
import $file.difftest.build

object difftest extends millbuild.difftest.build.CommonDiffTest {
  override def millSourcePath = os.pwd / "difftest"
}
```

Expose a build target in the host project's `Makefile`:

```make
emu: sim-verilog
	@$(MAKE) -C difftest emu WITH_CHISELDB=0 WITH_CONSTANTIN=0
```

Register a probe on the DUT:

```scala
import difftest._

val instrCommit = DifftestModule(new DiffInstrCommit, delay = 1, dontCare = true)
instrCommit.valid  := io.in.valid
instrCommit.pc     := SignExt(io.in.bits.decode.cf.pc, AddrBits)
instrCommit.instr  := io.in.bits.decode.cf.instr
instrCommit.skip   := io.in.bits.isMMIO
instrCommit.isRVC  := io.in.bits.decode.cf.instr(1, 0) =/= "b11".U
instrCommit.rfwen  := io.wb.rfWen && io.wb.rfDest =/= 0.U
instrCommit.wdest  := io.wb.rfDest
instrCommit.wpdest := io.wb.rfDest
```

The CPU top should expose the simulation clock, reset, and DiffTest interfaces. The current wrapper pattern is:

```scala
import chisel3.stage.ChiselGeneratorAnnotation
import circt.stage._

class MyCpuTop extends Module with HasDiffTestInterfaces {
  override def cpuName: Option[String] = Some("MyCpu")
  // Instantiate CPU logic and DiffTest probes here.
  // Override connectTopIOs and difftestMemIO when needed.
}

object Main extends App {
  val (newArgs, firtoolOptions) = DifftestModule.parseArgs(args)
  val generator = ChiselGeneratorAnnotation(() => DifftestModule.top(new MyCpuTop))
  (new ChiselStage).execute(
    newArgs,
    Seq(generator, CIRCTTargetAnnotation(CIRCTTarget.SystemVerilog)) ++ firtoolOptions
  )
}
```

### Integration Checklist

- Implement the mandatory bundles: `DiffArchEvent`, `DiffInstrCommit`, `DiffTrapEvent`, `DiffCSRState`, and register-state bundles.
- Instantiate optional bundles only when the corresponding CPU behavior exists.
- Keep PCs, instruction packets, register indices, and privilege state in the width and encoding expected by the bundles.
- Route architectural state updates through the rename-table and delayed-update interfaces when the CPU has physical registers.
- Connect all top-level IOs that the generated simulation expects.
- Use `DifftestMem` or `difftestMemIO` for the simulation memory path.
- Start with no optional transport stages, verify MicroBench, then enable squash, delta, batch, replay, or FPGA mode individually.

### Integration Documentation

- [Integrate DiffTest with a Chisel CPU](docs/integration/chisel.md): complete setup and wrapper API.
- [Probe bundle reference](docs/integration/interfaces.md): current bundle list, mandatory status, top-level IOs, and memory interfaces.
- [Non-Chisel designs](docs/integration/non-chisel.md): generate Verilog and C++ interfaces from a DiffTest profile.

## For DiffTest Developers

Use this path when modifying the probe schema, transport pipeline, DPI-C bridge, software checkers, reference proxy, or backend build systems.

### Architecture at a Glance

```text
DUT probes (Bundles.scala, Difftest.scala)
  -> optional trace / preprocess / replay
  -> validate
  -> optional squash / delta / batch
  -> generated DPI-C sink
  -> C++ DiffState and checkers
  -> reference model
```

The hardware side decides which events cross the simulation boundary and when. The software side reconstructs architectural state, advances the reference model, and reports the first mismatch. Changes to either side must preserve event ordering and bundle ABI compatibility.

### Repository Map

| Area | Main locations |
|------|----------------|
| Probe definitions | `src/main/scala/Bundles.scala`, `src/main/scala/Difftest.scala` |
| Transport pipeline | `Gateway.scala`, `Preprocess.scala`, `Squash.scala`, `Delta.scala`, `Batch.scala`, `Replay.scala` |
| DPI-C generation | `DPIC.scala`, generated `build/generated-src/difftest-dpic.*` |
| C++ checking runtime | `src/test/csrc/difftest/` |
| Reference proxy | `src/test/csrc/difftest/refproxy.cpp` |
| Runtime arguments | `src/test/csrc/common/args.cpp` |
| EMU, VCS, FPGA, and PLDM builds | `Makefile`, `emu.mk`, `vcs.mk`, `fpga.mk`, `palladium.mk` |
| Generated outputs | `build/rtl/`, `build/generated-src/`, `build/*-compile/` |

### Change Checklist

- **Probe or ABI change:** update `Bundles.scala`, the corresponding `Diff*` class, C++ generated-struct consumers, and checker behavior together.
- **Transport change:** validate ordering and flush behavior under squash, delta, batch, and replay configurations.
- **Checker change:** reproduce the mismatch first and preserve both DUT and reference diagnostics.
- **Build change:** verify the affected backend and update the matching build/test documentation.
- **Generated files:** never edit `build/` or generated `difftest-dpic.*` files manually.

### Verification Ladder

Run from the host project root unless the linked document states otherwise:

```bash
make clean
make emu -j2
./build/emu -i "$WORKLOAD" --diff "$REF_SO"
```

The minimum acceptance gate is a clean build followed by `HIT GOOD TRAP` on MicroBench. Transport, replay, and FPGA changes should also run the relevant backend or a short Linux boot before merging.

### Developer Documentation

- [Repository layout](docs/developers/repository-layout.md): source tree, key files, and where changes belong.
- [Hardware transport flow](docs/developers/hardware-flow.md): pipeline stages and configuration letters.
- [Software checking flow](docs/developers/software-check.md): DPI-C landing, checkers, and reference-model calls.
- [Build, test, and debug](docs/developers/build-and-test.md): EMU, simv, FPGA Sim, Palladium, Query DB, and waveforms.
- [Development workflow](docs/developers/workflow.md): planning, progress records, verification, and debugging escalation.

## Input Formats

DiffTest accepts linear binaries, gzip, zstd, ELF images, and footprint inputs. A plain binary is loaded as a continuous address space starting at `0x8000_0000` by default. gzip, zstd, and ELF formats are detected from their headers. Use `--as-footprints` to load memory accesses from a footprint image.

Related build options include `IMAGE_GZ_COMPRESS=0`, `NO_ZSTD_COMPRESSION=1`, and `IMAGE_ELF=0` when a backend needs to remove an optional input format.

## References

- DiffTest theory and advanced discussion: [YSYX lecture](https://ysyx.oscc.cc/slides/2205/12.html), [Paper 1](https://ieeexplore.ieee.org/document/9923860), [Paper 2](https://jcst.ict.ac.cn/en/article/doi/10.1007/s11390-023-3285-8)
- SMP-DiffTest: [slides](https://github.com/OpenXiangShan/XiangShan-doc/blob/main/slides/20210624-RVWC-SMP-Difftest%20%E6%94%AF%E6%8C%81%E5%A4%9A%E5%A4%84%E7%90%86%E5%99%A8%E7%9A%84%E5%B7%AE%E5%88%86%E6%B5%8B%E8%AF%95%E6%96%B9%E6%B3%95.pdf), [video](https://www.bilibili.com/video/BV1NM4y1T7Hz/)
- DiffTest on Palladium: [slides](https://github.com/OpenXiangShan/XiangShan-doc/raw/main/slides/20240827-CadenceLIVEChina-%E9%9D%A2%E5%90%91Palladium%E7%9A%84%E9%AB%98%E6%80%A7%E8%83%BD%E5%A4%84%E7%90%86%E5%99%A8%E8%BD%AF%E7%A1%AC%E4%BB%B6%E5%8D%8F%E5%90%8C%E9%AA%8C%E8%AF%81%E6%A1%86%E6%9E%B6%E9%83%A8%E7%BD%B2%E4%B8%8E%E5%8A%A0%E9%80%9F%E6%96%B9%E6%B3%95.pdf)
- Coverage-guided fuzzing with DiffTest: [slides](https://github.com/OpenXiangShan/XiangShan-doc/raw/main/slides/20241130-Finding-More-Bugs-with-DiffTest-and-XFUZZ.pdf)

## License

DiffTest is released under the Mulan PSL v2. See [LICENSE](LICENSE).
