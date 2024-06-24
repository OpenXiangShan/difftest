/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

package difftest

import chisel3._
import chisel3.util._
import difftest.common.DifftestWiring
import difftest.gateway.{Gateway, GatewayConfig}

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths}
import scala.annotation.tailrec
import scala.collection.mutable.ListBuffer

trait DifftestWithCoreid {
  val coreid = UInt(8.W)
}

trait DifftestWithIndex {
  val index = UInt(8.W)
}

trait DifftestWithStamp {
  val stamp = UInt(16.W)
}

trait DiffTestIsInherited { this: DifftestBundle =>
  def inheritFrom[T <: DifftestBundle](parent: T): Unit = {
    parent.elements.foreach { case (name, data) =>
      require(this.elements.contains(name))
      this.elements(name) := data
    }
  }
}

sealed trait DifftestBundle extends Bundle with DifftestWithCoreid { this: DifftestBaseBundle =>
  def bits: DifftestBaseBundle = this

  // Used to detect the number of cores. Must be used only by one Bundle.
  def isUniqueIdentifier: Boolean = false
  // A desired offset in the C++ struct can be specified.
  val desiredOffset: Int = 999

  val desiredCppName: String
  def desiredModuleName: String = {
    val className = {
      val name = this.getClass.getName.replace("$", ".").replace("Diff", "Difftest")
      if (squashQueue) name.replace("Queue", "") else name
    }
    className.split("\\.").filterNot(_.forall(java.lang.Character.isDigit)).last
  }

  def order: (Int, String) = (desiredOffset, desiredModuleName)

  def isIndexed: Boolean = this.isInstanceOf[DifftestWithIndex]
  def getIndex: Option[UInt] = {
    this match {
      case b: DifftestWithIndex => Some(b.index)
      case _                    => None
    }
  }

  protected val needFlatten: Boolean = false
  def isFlatten: Boolean = hasAddress && this.needFlatten

  // Elements without clock, coreid, and index.
  def diffElements: Seq[(String, Seq[UInt])] = {
    val filteredElements = Seq("clock", "coreid", "index")
    val raw = elements.toSeq.reverse.filterNot(e => filteredElements.contains(e._1))
    raw.map { case (s, data) =>
      data match {
        case v: Vec[_] => (s, Some(v.asInstanceOf[Vec[UInt]]))
        case u: UInt   => (s, Some(Seq(u)))
        case _ =>
          println(s"Unknown type: ($s, $data)")
          (s, None)
      }
    }.map(x => (x._1, x._2.get))
  }
  // Sizes of the DiffTest elements.
  private def diffSizes(round: Int): Seq[Seq[Int]] = {
    diffElements.map(_._2.map(u => (u.getWidth + round - 1) / round))
  }

  def toCppDeclMacro: String = {
    val macroName = s"CONFIG_DIFFTEST_${desiredModuleName.toUpperCase.replace("DIFFTEST", "")}"
    s"#define $macroName"
  }
  def toCppDeclaration(packed: Boolean): String = {
    val cpp = ListBuffer.empty[String]
    val attribute = if (packed) "__attribute__((packed))" else ""
    cpp += s"typedef struct $attribute {"
    for (((name, elem), size) <- diffElements.zip(diffSizes(8))) {
      val isRemoved = isFlatten && Seq("valid", "address").contains(name)
      if (!isRemoved) {
        val arrayType = s"uint${size.head * 8}_t"
        val arrayWidth = if (elem.length == 1) "" else s"[${elem.length}]"
        cpp += f"  $arrayType%-8s $name$arrayWidth;"
      }
    }
    cpp += s"} ${desiredModuleName};"
    cpp.mkString("\n")
  }

  // returns Bool indicating whether `this` bundle can be squashed with `base`
  def supportsSquash(base: DifftestBundle): Bool = supportsSquashBase
  def supportsSquashBase: Bool = if (hasValid) !getValid else true.B
  // returns a Seq indicating the squash dependencies. Default: empty
  // Only when one of the dependencies is valid, this bundle is squashed.
  val squashDependency: Seq[String] = Seq()
  // returns a seq of Group name of this bundle, Default: REF
  // Only bundles with same GroupName will affect others' squash state.
  // Some bundle will have several GroupName, such as LoadEvent
  // Optional GroupName: REF / GOLDENMEM
  val squashGroup: Seq[String] = Seq("REF")
  // returns a squashed, right-value Bundle. Default: overriding `base` with `this`
  def squash(base: DifftestBundle): DifftestBundle = this
  def squashQueue: Boolean = false
}

class DiffArchEvent extends ArchEvent with DifftestBundle {
  // DiffArchEvent must be instantiated once for each core.
  override def isUniqueIdentifier: Boolean = true
  override val desiredCppName: String = "event"
}

class DiffInstrCommit(nPhyRegs: Int = 32) extends InstrCommit(nPhyRegs) with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "commit"

  private val maxNumFused = 255
  override def supportsSquash(base: DifftestBundle): Bool = {
    val that = base.asInstanceOf[DiffInstrCommit]
    val nextNFused = (nFused +& that.nFused) + 1.U
    !valid || (!skip && (!that.valid || nextNFused <= maxNumFused.U) && !special.asUInt.orR)
  }
  override def supportsSquashBase: Bool = {
    !valid || (!skip && !special.asUInt.orR)
  }
  override def squash(base: DifftestBundle): DifftestBundle = {
    val that = base.asInstanceOf[DiffInstrCommit]
    val squashed = WireInit(Mux(valid, this, that))
    squashed.valid := valid || that.valid
    when(valid && that.valid) {
      squashed.nFused := nFused + that.nFused + 1.U
    }
    squashed
  }
}

class DiffCommitData extends CommitData with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "commit_data"
  override def supportsSquashBase: Bool = true.B
}

class DiffTrapEvent extends TrapEvent with DifftestBundle {
  override val desiredCppName: String = "trap"
  override def supportsSquashBase: Bool = !hasTrap && !hasWFI
}

class DiffCSRState extends CSRState with DifftestBundle {
  override val desiredCppName: String = "csr"
  override val desiredOffset: Int = 1
  override val squashDependency: Seq[String] = Seq("commit", "event")
}

class DiffHCSRState extends HCSRState with DifftestBundle {
  override val desiredCppName: String = "hcsr"
  override val desiredOffset: Int = 6
  override val squashDependency: Seq[String] = Seq("commit", "event")
}

class DiffDebugMode extends DebugModeCSRState with DifftestBundle {
  override val desiredCppName: String = "dmregs"
}

class DiffIntWriteback(numRegs: Int = 32) extends DataWriteback(numRegs) with DifftestBundle {
  override val desiredCppName: String = "wb_int"
  override protected val needFlatten: Boolean = true
  // It is required for MMIO/Load(only for multi-core) data synchronization, and commit instr trace record
  override def supportsSquashBase: Bool = true.B
}

class DiffFpWriteback(numRegs: Int = 32) extends DiffIntWriteback(numRegs) {
  override val desiredCppName: String = "wb_fp"
}

class DiffVecWriteback(numRegs: Int = 32) extends DiffIntWriteback(numRegs) {
  override val desiredCppName: String = "wb_vec"
}

class DiffArchIntRegState extends ArchIntRegState with DifftestBundle {
  override val desiredCppName: String = "regs_int"
  override val desiredOffset: Int = 0
}

abstract class DiffArchDelayedUpdate(numRegs: Int)
  extends ArchDelayedUpdate(numRegs)
  with DifftestBundle
  with DifftestWithIndex

class DiffArchIntDelayedUpdate extends DiffArchDelayedUpdate(32) {
  override val desiredCppName: String = "regs_int_delayed"
}

class DiffArchFpDelayedUpdate extends DiffArchDelayedUpdate(32) {
  override val desiredCppName: String = "regs_fp_delayed"
}

class DiffArchFpRegState extends ArchIntRegState with DifftestBundle {
  override val desiredCppName: String = "regs_fp"
  override val desiredOffset: Int = 2
}

class DiffArchVecRegState extends ArchVecRegState with DifftestBundle {
  override val desiredCppName: String = "regs_vec"
  override val desiredOffset: Int = 4
}

class DiffVecCSRState extends VecCSRState with DifftestBundle {
  override val desiredCppName: String = "vcsr"
  override val desiredOffset: Int = 5
  override val squashDependency: Seq[String] = Seq("commit", "event")
}

class DiffSbufferEvent extends SbufferEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "sbuffer"
  override val squashGroup: Seq[String] = Seq("GOLDENMEM")
}

class DiffStoreEvent extends StoreEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "store"
}

class DiffStoreEventQueue extends DiffStoreEvent with DifftestWithStamp with DiffTestIsInherited {
  override val squashQueue: Boolean = true
}

class DiffLoadEvent extends LoadEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "load"
  override val squashGroup: Seq[String] = Seq("REF", "GOLDENMEM")
}

class DiffLoadEventQueue extends DiffLoadEvent with DifftestWithStamp with DiffTestIsInherited {
  val commitData = UInt(64.W)
  val regWen = Bool()
  val wdest = UInt(8.W)
  val fpwen = Bool()
  override val squashQueue: Boolean = true
}

class DiffAtomicEvent extends AtomicEvent with DifftestBundle {
  override val desiredCppName: String = "atomic"
  override val squashGroup: Seq[String] = Seq("GOLDENMEM")
}

class DiffL1TLBEvent extends L1TLBEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "l1tlb"
  override val squashGroup: Seq[String] = Seq("GOLDENMEM")
  // TODO: currently we assume it can be dropped
  override def supportsSquashBase: Bool = true.B
}

class DiffL2TLBEvent extends L2TLBEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "l2tlb"
  override val squashGroup: Seq[String] = Seq("GOLDENMEM")
  // TODO: currently we assume it can be dropped
  override def supportsSquashBase: Bool = true.B
}

class DiffRefillEvent extends RefillEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "refill"
  override val squashGroup: Seq[String] = Seq("GOLDENMEM")
  // TODO: currently we assume it can be dropped
  override def supportsSquashBase: Bool = true.B
}

class DiffLrScEvent extends ScEvent with DifftestBundle {
  override val desiredCppName: String = "lrsc"
}

class DiffRunaheadEvent extends RunaheadEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "runahead"
}

class DiffRunaheadCommitEvent extends RunaheadEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "runahead_commit"
}

class DiffRunaheadRedirectEvent extends RunaheadRedirectEvent with DifftestBundle {
  override val desiredCppName: String = "runahead_redirect"
}

class DiffTraceInfo(config: GatewayConfig) extends TraceInfo with DifftestBundle {
  override val desiredCppName: String = "trace_info"

  override val squashGroup: Seq[String] = Seq("REF", "GOLDENMEM")
  override def supportsSquash(base: DifftestBundle): Bool = {
    val that = base.asInstanceOf[DiffTraceInfo]
    !valid || !that.valid || (trace_size +& that.trace_size <= config.replaySize.U)
  }
  override def supportsSquashBase: Bool = true.B

  override def squash(base: DifftestBundle): DifftestBundle = {
    val that = base.asInstanceOf[DiffTraceInfo]
    val squashed = WireInit(Mux(valid, this, that))
    squashed.valid := valid || that.valid
    when(valid && that.valid) {
      squashed.trace_head := that.trace_head
      squashed.trace_size := trace_size + that.trace_size
    }
    squashed
  }
}

trait DifftestModule[T <: DifftestBundle] {
  val io: T
}

object DifftestModule {
  private val enabled = true
  private val instances = ListBuffer.empty[DifftestBundle]
  private val cppMacros = ListBuffer.empty[String]
  private val vMacros = ListBuffer.empty[String]

  def parseArgs(args: Array[String]): Array[String] = {
    @tailrec
    def nextOption(args: Array[String], list: List[String]): Array[String] = {
      list match {
        case Nil => args
        case "--difftest-config" :: config :: tail =>
          Gateway.setConfig(config)
          nextOption(args.patch(args.indexOf("--difftest-config"), Nil, 2), tail)
        case option :: tail => nextOption(args, tail)
      }
    }
    nextOption(args, args.toList)
  }

  def apply[T <: DifftestBundle](
    gen: T,
    dontCare: Boolean = false,
    delay: Int = 0,
  ): T = {
    val difftest: T = Wire(gen)
    if (enabled) {
      val sink = Gateway(gen)
      sink := Delayer(difftest, delay)
      sink.coreid := difftest.coreid
    }
    if (dontCare) {
      difftest := DontCare
    }
    difftest
  }

  def finish(cpu: String, createTopIO: Boolean): Option[DifftestTopIO] = {
    val gateway = Gateway.collect()
    cppMacros ++= gateway.cppMacros
    vMacros ++= gateway.vMacros
    instances ++= gateway.instances

    generateCppHeader(cpu, gateway.structPacked.getOrElse(false))
    generateVeriogHeader()

    Option.when(createTopIO) {
      if (enabled) {
        createTopIOs(gateway.step.getOrElse(0.U))
      } else {
        WireInit(0.U.asTypeOf(new DifftestTopIO))
      }
    }
  }

  def finish(cpu: String): DifftestTopIO = {
    finish(cpu, true).get
  }

  def createTopIOs(step: UInt): DifftestTopIO = {
    val difftest = IO(new DifftestTopIO)

    difftest.step := step

    val timer = RegInit(0.U(64.W)).suggestName("timer")
    timer := timer + 1.U
    dontTouch(timer)

    val log_enable = difftest.logCtrl.enable(timer).suggestName("log_enable")
    dontTouch(log_enable)

    difftest.uart := DontCare

    require(DifftestWiring.isEmpty, s"pending wires left: ${DifftestWiring.getPending}")

    difftest
  }

  def generateCppHeader(cpu: String, structPacked: Boolean): Unit = {
    val difftestCpp = ListBuffer.empty[String]
    difftestCpp += "#ifndef __DIFFSTATE_H__"
    difftestCpp += "#define __DIFFSTATE_H__"
    difftestCpp += ""
    difftestCpp += "#include <cstdint>"
    difftestCpp += ""

    cppMacros.foreach(m => difftestCpp += s"#define $m")
    difftestCpp += ""

    val cpu_s = cpu.replace("-", "_").replace(" ", "").toUpperCase
    difftestCpp += s"#define CPU_$cpu_s"
    difftestCpp += ""

    val numCores = instances.count(_.isUniqueIdentifier)
    if (instances.nonEmpty) {
      difftestCpp += s"#define NUM_CORES $numCores"
      difftestCpp += ""
    }

    val uniqBundles = instances.groupBy(_.desiredModuleName)
    // Create cpp declaration for each bundle type
    uniqBundles.values
      .foreach(bundles => {
        val bundleType = bundles.head
        difftestCpp += bundleType.toCppDeclMacro
        val macroName = bundleType.desiredCppName.toUpperCase
        if (bundleType.isInstanceOf[DifftestWithIndex]) {
          val configWidthName = s"CONFIG_DIFF_${macroName}_WIDTH"
          require(bundles.length % numCores == 0, s"Cores seem to have different # of ${macroName}")
          difftestCpp += s"#define $configWidthName ${bundles.length / numCores}"
        }
        if (bundleType.isFlatten) {
          val configWidthName = s"CONFIG_DIFF_${macroName}_WIDTH"
          difftestCpp += s"#define $configWidthName ${bundleType.bits.getNumElements}"
        }
        difftestCpp += bundleType.toCppDeclaration(structPacked)
        difftestCpp += ""
      })

    // create top-level difftest struct
    difftestCpp += "typedef struct {"
    for ((className, cppInstances) <- uniqBundles.toSeq.sortBy(_._2.head.order)) {
      val bundleType = cppInstances.head
      val instanceName = bundleType.desiredCppName
      val cppIsArray = bundleType.isInstanceOf[DifftestWithIndex] || bundleType.isFlatten
      val nInstances = cppInstances.length
      val instanceCount = if (bundleType.isFlatten) bundleType.bits.getNumElements else nInstances / numCores
      require(nInstances % numCores == 0, s"Cores seem to have different # of ${instanceName}")
      require(cppIsArray || nInstances == numCores, s"# of ${instanceName} should not be ${nInstances}")
      val arrayWidth = if (cppIsArray) s"[$instanceCount]" else ""
      difftestCpp += f"  $className%-30s $instanceName$arrayWidth;"
    }
    difftestCpp += "} DiffTestState;"
    difftestCpp += ""

    difftestCpp +=
      s"""
         |class DiffStateBuffer {
         |public:
         |  virtual ~DiffStateBuffer() {}
         |  virtual DiffTestState* get(int zone, int index) = 0;
         |  virtual DiffTestState* next() = 0;
         |  virtual void switch_zone() = 0;
         |};
         |
         |extern DiffStateBuffer** diffstate_buffer;
         |
         |extern void diffstate_buffer_init();
         |extern void diffstate_buffer_free();
         |""".stripMargin
    difftestCpp +=
      s"""
         |#ifdef CONFIG_DIFFTEST_PERFCNT
         |void diffstate_perfcnt_init();
         |void diffstate_perfcnt_finish(long long msec);
         |#endif // CONFIG_DIFFTEST_PERFCNT
         |""".stripMargin
    difftestCpp += "#endif // __DIFFSTATE_H__"
    difftestCpp += ""
    streamToFile(difftestCpp, "diffstate.h")
  }

  def generateVeriogHeader(): Unit = {
    val difftestVeriog = ListBuffer.empty[String]
    vMacros.foreach(m => difftestVeriog += s"`define $m")
    streamToFile(difftestVeriog, "DifftestMacros.v")
  }

  def streamToFile(fileStream: ListBuffer[String], fileName: String) {
    val outputDir = sys.env("NOOP_HOME") + "/build/generated-src/"
    Files.createDirectories(Paths.get(outputDir))
    val outputFile = outputDir + fileName
    Files.write(Paths.get(outputFile), fileStream.mkString("\n").getBytes(StandardCharsets.UTF_8))
  }
}

private class Delayer[T <: Data](gen: T, n_cycles: Int) extends Module {
  val i = IO(Input(chiselTypeOf(gen)))
  val o = IO(Output(chiselTypeOf(gen)))
}

private class DelayReg[T <: Data](gen: T, n_cycles: Int) extends Delayer(gen, n_cycles) {
  var r = WireInit(i)
  for (_ <- 0 until n_cycles) {
    r = RegNext(r, 0.U.asTypeOf(gen))
  }
  o := r
}

private class DelayMem[T <: Data](gen: T, n_cycles: Int) extends Delayer(gen, n_cycles) {
  val mem = Mem(n_cycles, chiselTypeOf(gen))
  val ptr = RegInit(0.U(log2Ceil(n_cycles).W))
  val init_flag = RegInit(false.B)
  mem(ptr) := i
  ptr := ptr + 1.U
  when(ptr === (n_cycles - 1).U) {
    init_flag := true.B
    ptr := 0.U
  }
  o := Mux(init_flag, mem(ptr), 0.U.asTypeOf(gen))
}

object Delayer {
  def apply[T <: Data](gen: T, n_cycles: Int, useMem: Boolean = false): T = {
    if (n_cycles > 0) {
      val delayer = if (useMem) {
        Module(new DelayMem(gen, n_cycles))
      } else {
        Module(new DelayReg(gen, n_cycles))
      }
      delayer.i := gen
      delayer.o
    } else {
      gen
    }
  }
}

// Difftest emulator top. Will be created by DifftestModule.finish
class DifftestTopIO extends Bundle {
  val step = Output(UInt(64.W))
  val perfCtrl = new PerfCtrlIO
  val logCtrl = new LogCtrlIO
  val uart = new UARTIO
}

class PerfCtrlIO extends Bundle {
  val clean = Input(Bool())
  val dump = Input(Bool())
}

class LogCtrlIO extends Bundle {
  val begin = Input(UInt(64.W))
  val end = Input(UInt(64.W))
  val level = Input(UInt(64.W)) // a cpp uint

  def enable(timer: UInt): Bool = {
    val en = WireInit(false.B)
    en := timer >= begin && timer < end
    en
  }
}

// UART IO, if needed, should be inited in SimTop IO
// If not needed, just hardwire all output to 0
class UARTIO extends Bundle {
  val out = new Bundle {
    val valid = Output(Bool())
    val ch = Output(UInt(8.W))
  }
  val in = new Bundle {
    val valid = Output(Bool())
    val ch = Input(UInt(8.W))
  }
}
