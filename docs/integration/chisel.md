# Integrate DiffTest with a Chisel CPU

Use this guide when connecting DiffTest to a Chisel-based RISC-V design. DiffTest generates its C++ interface from the Chisel probe schema, so the probe definitions are the source of truth for the hardware/software boundary.

## Compatibility

- Chisel `7.14.0` and its Chisel plugin.
- Scala `2.13`.
- A host project using Mill and a Makefile-based simulation flow.

Older Chisel `3.6.1` integration is no longer supported. The former `DifftestModule.finish` API is also no longer present in the current source. Use the `HasDiffTestInterfaces` plus `DifftestModule.top` wrapper described below.

## 1. Add DiffTest as a Submodule

From the host project root:

```bash
git submodule add https://github.com/OpenXiangShan/difftest.git
git submodule update --init --recursive
```

## 2. Add the Mill Module

Add DiffTest to the host `build.sc` or `build.mill`:

```scala
import $file.difftest.build

object difftest extends millbuild.difftest.build.CommonDiffTest {
  override def millSourcePath = os.pwd / "difftest"
}
```

Add the DiffTest module to the module that generates the CPU and simulation top:

```scala
object generator extends CommonModule {
  override def moduleDeps = super.moduleDeps ++ Seq(difftest)
}
```

The exact source layout differs between Mill build files. Keep the `difftest` object name and point `millSourcePath` at the submodule.

## 3. Expose the Emulator Target

Add a target to the host Makefile:

```make
emu: sim-verilog
	@$(MAKE) -C difftest emu WITH_CHISELDB=0 WITH_CONSTANTIN=0
```

The host project remains responsible for generating `SimTop.sv`. DiffTest then builds its C++ runtime and Verilator or VCS top around that RTL.

## 4. Instantiate Probe Bundles

Instantiate mandatory probes in the CPU commit path. This abbreviated example shows the instruction-commit probe:

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

Use the following rules when wiring probes:

- Drive every compared field from the same architectural event that the reference model receives.
- Use `delay` when the probe event must align with a later commit stage.
- Use `dontCare = true` only while bringing up an optional probe; it prevents unconnected fields from causing elaboration errors, but the probe still needs meaningful values before checking it.
- Instantiate probes in a stable order and avoid conditional structure changes between cores.
- Keep optional extension probes disabled until the base ISA and CSRs pass MicroBench.

See [Probe bundle reference](interfaces.md) for the complete bundle list.

## 5. Wrap the CPU as SimTop

The current integration API is `HasDiffTestInterfaces`. The wrapper creates the `SimTop` module, collects registered probes, connects the DUT clock and reset, and exposes the DiffTest control IOs.

```scala
import chisel3._
import difftest._

class MyCpuTop extends Module with HasDiffTestInterfaces {
  val io = IO(new Bundle {
    // CPU-visible IO
  })

  override def cpuName: Option[String] = Some("MyCpu")

  // Instantiate the CPU and DiffTest probes here.

  // Connect additional buses that the generated testbench must expose.
  override def connectTopIOs(difftest: DifftestTopIO): Unit = {
    // difftest.uart <> io.uart
  }

  // Required for FPGA or AXI-based memory integration.
  override def difftestMemIO: Option[DifftestMemIO] = None
}
```

Generate the wrapped design:

```scala
import chisel3.stage.ChiselGeneratorAnnotation
import circt.stage._

object MyGenerator extends App {
  val (newArgs, firtoolOptions) = DifftestModule.parseArgs(args)
  val generator = ChiselGeneratorAnnotation(() => DifftestModule.top(new MyCpuTop))
  (new ChiselStage).execute(
    newArgs,
    Seq(generator, CIRCTTargetAnnotation(CIRCTTarget.SystemVerilog)) ++ firtoolOptions
  )
}
```

`DifftestModule.parseArgs` removes DiffTest-specific options and returns the remaining Chisel arguments and FIRRTL options. Keep this call in the generator entry point so `--difftest-config` and `--difftest-exclude` are applied before elaboration.

## 6. Connect Simulation Memory

DiffTest supports two memory integration styles.

### DPI-C Memory

Use `DifftestMem` when the simulator owns memory and the DUT accesses it through DPI-C:

```scala
val mem = DifftestMem(memByte, 8)

when(wen) {
  mem.write(
    addr = wIdx,
    data = in.w.bits.data.asTypeOf(Vec(DataBytes, UInt(8.W))),
    mask = in.w.bits.strb.asBools
  )
}

val rdata = mem.readAndHold(rIdx, ren).asUInt
```

The C++ runtime initializes this memory from the workload image and performs reference-model memory synchronization.

### AXI Memory for FPGA

Use `difftestMemIO` to expose the CPU memory port when `DifftestModule.isFPGA` is true:

```scala
override def difftestMemIO: Option[DifftestMemIO] = {
  if (DifftestModule.isFPGA) Some(DifftestMemIO(io.mem)) else None
}
```

The optional second `DifftestMemIO` argument exposes a separate memory-side AXI record. Leave it as `None` when the wrapper should generate the memory port.

## 7. Top-Level IO

`HasDiffTestInterfaces` collects CPU IO that is not the clock, reset, UART, or an FPGA memory record. These ports are recreated on the generated `SimTop` so the host testbench can connect them.

`SimTop` also creates a `DifftestTopIO` containing:

- `exit`
- `step`
- `perfCtrl`
- `logCtrl`
- `uart`

Use `connectTopIOs` to connect CPU UART or other control ports to this wrapper IO. See [Probe bundle reference](interfaces.md) for field-level details.

## 8. Generate and Verify

Generate RTL:

```bash
make clean
make sim-verilog
```

Build and run MicroBench:

```bash
make emu -j2
./build/emu -i "$WORKLOAD" --diff "$REF_SO"
```

Start with a simple transport configuration. After MicroBench passes, enable optional features one at a time:

1. Squash.
2. Batch or Delta.
3. Replay.
4. FPGA mode.
5. Additional ISA extension probes.

If a new configuration fails while the base configuration passes, compare Query DBs or event ordering before changing the DUT probes.

## Integration Checklist

- The host uses Chisel `7.14.0`.
- DiffTest is registered as a Mill module.
- At least the mandatory probes are instantiated.
- The top module mixes in `HasDiffTestInterfaces` and sets `cpuName`.
- The clock and reset seen by DiffTest match the DUT commit domain.
- The memory path is connected through `DifftestMem` or `difftestMemIO`.
- The generator calls `DifftestModule.parseArgs` and wraps the CPU with `DifftestModule.top`.
- RTL generation succeeds before the C++ build starts.
- MicroBench prints `HIT GOOD TRAP` with differential checking enabled.

## Related Documentation

- [Probe bundle reference](interfaces.md)
- [Non-Chisel integration](non-chisel.md)
- [Hardware transport flow](../developers/hardware-flow.md)
- [Build, test, and debug](../developers/build-and-test.md)
