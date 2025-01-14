# DiffTest

DiffTest (差分测试): a modern co-simulation framework for RISC-V processors.

## Usage

DiffTest supports the following run-time command-line arguments.
This list is not complete as we are still working on improving the documentation.

- `-i, --image=FILE` for the workload to be executed by the design-under-test (DUT)

  - DiffTest supports linear (binary, gz, zstd, ELF) and footprint input formats, controlled by build-time and run-time arguments.

  - By default, the image is loaded as a binary file with a linear (continuous) address space starting at 0x8000_0000.
    This behavior is overrided if an advanced image format (gz, zstd, ELF) is detected.

  - Compressed binary files in gz or zstd formats are supported, determined by leading magic numbers of the image file.
    Once detected, they will be first decompressed and then loaded into the linear memory.
    Use `IMAGE_GZ_COMPRESS=0` or `NO_ZSTD_COMPRESSION=1` to disable their support at build-time.

  - ELF files are supported, determined by leading magic numbers of the image file. Use `IMAGE_ELF=0` to disable it at build-time.

  - [Footprint inputs](https://doi.org/10.1145/3649329.3655911) are supported by the run-time argument `--as-footprints`.
    Basically, every time when a new address is accessed by the CPU, a new data block is read from the image file and put at the accessed address.

For more details on compile-time arguments, see the Makefiles.
For more details and a full list of supported run-time command-line arguments, run `emu --help`.

## Example: Generate Verilog for DiffTest Interfaces

DiffTest interfaces are provided in [Chisel bundles](src/main/scala/Bundles.scala) and expected to be integrated
into Chisel designs with auto-generated C++ interfaces.

**We strongly recommend using Chisel as the design description language when using DiffTest.**
It will greatly benefit the verification setup since we are providing some advanced features
only in Chisel, such as datapath optimizations for higher simulation speed on emulation platforms.

If you are using DiffTest in a non-Chisel environment, we still provide examples of the generated Verilog modules.
You may configure the test interfaces in [src/test/scala/DifftestMain.scala](src/test/scala/DifftestMain.scala) based on your design details.
The generated Verilog and C++ files will match (in type and count) what you have described about your use case.
After running the following command, files will be generated at `build`.

```bash
make
```

We support the DiffTest Profile as a configuration file for DiffTest to record and reconstruct DiffTest interfaces
through a `json` file.

## Example Chisel Usage: Connecting Your Own Design with DiffTest

We are supporting Chisel 3.6.1 (the last version supporting Scala FIRRTL Compiler)
as well as 6.6.0 (the latest stable version supporting MLIR FIRRTL Compiler).

Here are the detail instructions on integrating DiffTest to your own project.

1. Add this submodule to your design.

In your Git project:
```bash
git submodule add https://github.com/OpenXiangShan/difftest.git
```

In Mill `build.sc`:
```scala
import $file.difftest.build

// We recommend using a fixed Chisel version.
object difftest extends millbuild.difftest.build.CommonDiffTest {
  def crossValue: String = "3.6.1"

  override def millSourcePath = os.pwd / "difftest"
}

// This is for advanced users only.
// All supported Chisel versions are listed in `build.sc`.
// To pass a cross value to difftest:
object difftest extends Cross[millbuild.difftest.build.CommonDiffTest](chiselVersions) {
  override def millSourcePath = os.pwd / "difftest"
}
```

In `Makefile`:
```Makefile
emu: sim-verilog
	@$(MAKE) -C difftest emu WITH_CHISELDB=0 WITH_CONSTANTIN=0
```

2. Add difftest modules (in Chisel or Verilog) to your design.
All modules have been listed in the [APIs](#apis) chapter. Some of them are optional.

```scala
import difftest._

val difftest = DifftestModule(new DiffInstrCommit, delay = 1, dontCare = true)
difftest.valid  := io.in.valid
difftest.pc     := SignExt(io.in.bits.decode.cf.pc, AddrBits)
difftest.instr  := io.in.bits.decode.cf.instr
difftest.skip   := io.in.bits.isMMIO
difftest.isRVC  := io.in.bits.decode.cf.instr(1, 0)=/="b11".U
difftest.rfwen  := io.wb.rfWen && io.wb.rfDest =/= 0.U
difftest.wdest  := io.wb.rfDest
difftest.wpdest := io.wb.rfDest
```

3. Call `val difftesst = DifftestModule.finish(cpu: String)` at the top module whose module name should be `SimTop`. The variable name `difftest` must be used to ensure DiffTest could capture the input signals.

An optional UART input/output can be connected to DiffTest. DiffTest will automatically DontCare it internally.

```scala
val difftest = DifftestModule.finish("Demo")

// Optional UART connections. Remove this line if UART is not needed.
difftest.uart <> mmio.io.uart
```

Alternatively, you can skip the optional UART connections by using an overloaded version
of `DifftestModule.finish(cpu: String, createTopIO: Boolean)` with the 2nd parameter
`createTopIO` set to `false`. This overloaded version can be used in non-module context
(e.g. in App class) as following.

```scala
object Main extends App {
  // ...
  DifftestModule.finish("Demo", false)
}
```

4. Generate verilog files for simulation.

5. `make emu` and start simulating & debugging!

We provide example designs, including:
- [XiangShan](https://github.com/OpenXiangShan/XiangShan)
- [NutShell](https://github.com/OSCPU/NutShell/tree/dev-difftest)
- [Rocket](https://github.com/OpenXiangShan/rocket-chip/tree/dev-difftest)

If you encountered any issues when integrating DiffTest to your own design, feel free to let us know with necessary information on how you have modified your design. We will try our best to assist you.

## APIs (DiffTest Interfaces)

Currently we are supporting the RISC-V base ISA as well as some extensions,
including Float/Double, Debug, and Vector. We also support checking the cache
coherence via RefillTest.

| Probe Name | Descriptions | Mandatory |
| ---------- | ------------ | --------- |
| `DiffArchEvent` | Exceptions and interrupts | Yes |
| `DiffInstrCommit` | Executed instructions | Yes |
| `DiffTrapEvent` | Simulation environment call | Yes |
| `DiffArchIntRegState` | General-purpose registers | Yes |
| `DiffArchFpRegState` | Floating-point registers | No |
| `DiffArchVecRegState` | Vector registers | No |
| `DiffCSRState` | Control and status registers (CSRs) | Yes |
| `DiffVecCSRState` | CSRs for the Vector extension | No |
| `DiffHCSRState` | CSRs for the Hypervisor extension | No |
| `DiffDebugMode` | Debug mode registers | No |
| `DiffIntWriteback` | General-purpose writeback operations | No |
| `DiffFpWriteback` | Floating-point writeback operations | No |
| `DiffVecWriteback` | Vector writeback operations | No |
| `DiffArchIntDelayedUpdate` | Delayed general-purpose writeback | No |
| `DiffArchFpDelayedUpdate` | Delayed floating-point writeback | No |
| `DiffStoreEvent` | Store operations | No |
| `DiffSbufferEvent` | Store buffer operations | No |
| `DiffLoadEvent` | Load operations | No |
| `DiffAtomicEvent` | Atomic operations | No |
| `DiffL1TLBEvent` | L1 TLB operations | No |
| `DiffL2TLBEvent` | L2 TLB operations | No |
| `DiffRefillEvent` | Cache refill operations | No |
| `DiffLrScEvent` | Executed LR/SC instructions | No |
| `DiffNonRegInterruptPengingEvent` | Non-register interrupts pending | No |
| `DiffMhpmeventOverflowEvent` | Mhpmevent-register overflow | No |
| `DiffCriticalErrorEvent` | Raise critical-error | No |
| `DiffSyncAIAEvent` | Synchronization of AIA | No |
| `DiffSyncCustomMflushpwrEvent` | custom CSR mflushpwr | No |

The DiffTest framework comes with a simulation framework with some top-level IOs.
They will be automatically created when calling `DifftestModule.finish(cpu: String)`.

* `LogCtrlIO`
* `PerfCtrlIO`
* `UARTIO`

These IOs can be used along with the controller wrapper at `src/main/scala/common/LogPerfControl.scala`.

For compatibility on different platforms, the CPU should access a C++ memory via
DPI-C interfaces. This memory will be initialized in C++.

You may also use macro `DISABLE_DIFFTEST_RAM_DPIC` to remove memory DPI-Cs and use Verilog arrays instead.

```scala
val mem = DifftestMem(memByte, 8)
when (wen) {
    mem.write(
    addr = wIdx,
    data = in.w.bits.data.asTypeOf(Vec(DataBytes, UInt(8.W))),
    mask = in.w.bits.strb.asBools
    )
}
val rdata = mem.readAndHold(rIdx, ren).asUInt
```

To use DiffTest, please include all necessary modules and top-level IOs in your design.
It's worth noting the Chisel Bundles may have arguments with default values.
Please set the correct parameters for the interfaces.

## Plugins

There are several plugins to improve the RTL-simulation and debugging process.

### LightSSS: a lightweight simulation snapshot mechanism

After the simulation aborts, we require some debugging information to assist locating the root cause, such as waveform and DUT/REF logs.
Traditionally, this requires a second run for the simulation with debugging enabled for the last period of simulation (region of interest, ROI).
To avoid such tedious stage, we propose a snapshot mechanism to periodically take snapshots for the simulation process with minor performance overhead.
A recent snapshot will be restored after the simulation aborts to reproduce the abortion with debugging information enabled.
To understand the technical details of this mechanism, please refer to [our MICRO'22 paper](https://xiangshan-doc.readthedocs.io/zh-cn/latest/tutorials/publications/#towards-developing-high-performance-risc-v-processors-using-agile-methodology).

The plugin LightSSS is by default included at compilation time and should be manually enabled during simulation time using `--enable-fork`.
You may configure the snapshot period using `--fork-interval`. A typical period is 1 (for small designs) to 30 (for super large designs) seconds.
After the simulation aborts, DiffTest automatically re-runs the simulation from a recent snapshot and enables debugging information, including waveform and DUT/REF logs.
You may want to redirect the stderr to a file to capture the REF logs output by NEMU and Spike.
Please avoid using `--enable-fork` together with other debugging options, such as `-b`, `-e`, `--dump-wave`, `--dump-ref-trace`, etc.
The behavior when they are enabled simultaneously is undefined.

### spike-dasm: a disassembly engine for RISC-V instructions

When the simulation aborts, DiffTest gives a report on the current architectural states and a list of recently commited instructions.
To simplify the debugging process, we may want the disassembly of the executed instructions.
DiffTest is currently using the `spike-dasm` command provided by the [riscv-isa-sim](https://github.com/riscv-software-src/riscv-isa-sim) project for RISC-V instruction disassembly.

To use this plugin, you are required to build it from source and install the tool to somewhere in your `PATH`.
DiffTest will automatically detect its existence by searching the `PATH`.
Please refer to [the original README](https://github.com/riscv-software-src/riscv-isa-sim?tab=readme-ov-file#build-steps) for detailed installation instructions.
Feel free to change the `--prefix=` argument to where you have access to, such as `~/.cache`, so that you won't need the `sudo` privilege for the installation.

## References

* Theories of DiffTest / DiffTest 的基本原理
  * [一生一芯计划讲义](https://ysyx.oscc.cc/slides/2205/12.html)
* Advanced theories of DiffTest / DiffTest 原理的进阶问题讨论
  * [Paper 1](https://ieeexplore.ieee.org/document/9923860), [Paper 2](https://jcst.ict.ac.cn/en/article/doi/10.1007/s11390-023-3285-8)
  * SMP-DiffTest 支持多处理器的差分测试方法: [PPT](https://github.com/OpenXiangShan/XiangShan-doc/blob/main/slides/20210624-RVWC-SMP-Difftest%20%E6%94%AF%E6%8C%81%E5%A4%9A%E5%A4%84%E7%90%86%E5%99%A8%E7%9A%84%E5%B7%AE%E5%88%86%E6%B5%8B%E8%AF%95%E6%96%B9%E6%B3%95.pdf), [视频](https://www.bilibili.com/video/BV1NM4y1T7Hz/)
* Next-generation DiffTest / DiffTest 的下一步演进
  * DiffTest on Cadence Palladium: [Slides (in Chinese, 中文版)](https://github.com/OpenXiangShan/XiangShan-doc/raw/main/slides/20240827-CadenceLIVEChina-%E9%9D%A2%E5%90%91Palladium%E7%9A%84%E9%AB%98%E6%80%A7%E8%83%BD%E5%A4%84%E7%90%86%E5%99%A8%E8%BD%AF%E7%A1%AC%E4%BB%B6%E5%8D%8F%E5%90%8C%E9%AA%8C%E8%AF%81%E6%A1%86%E6%9E%B6%E9%83%A8%E7%BD%B2%E4%B8%8E%E5%8A%A0%E9%80%9F%E6%96%B9%E6%B3%95.pdf)
  * Integrating DiffTest with Coverage-Guided Fuzzing: [Slides](https://github.com/OpenXiangShan/XiangShan-doc/raw/main/slides/20241130-Finding-More-Bugs-with-DiffTest-and-XFUZZ.pdf)
