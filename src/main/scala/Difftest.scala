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
import difftest.batch.Batch
import difftest.dpic.DPIC
import difftest.squash.Squash

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths}
import scala.collection.mutable.ListBuffer

trait DifftestWithCoreid {
  val coreid = UInt(8.W)
}

trait DifftestWithIndex {
  val index = UInt(8.W)
}

sealed trait DifftestBundle extends Bundle with DifftestWithCoreid { this: DifftestBaseBundle =>
  def bits: DifftestBaseBundle = this

  // Used to detect the number of cores. Must be used only by one Bundle.
  def isUniqueIdentifier: Boolean = false
  // A desired offset in the C++ struct can be specified.
  val desiredOffset: Int = 999

  val desiredCppName: String
  def desiredModuleName: String = {
    val className = this.getClass.getName.replace("$", ".").replace("Diff", "Difftest")
    className.split("\\.").filterNot(_.forall(java.lang.Character.isDigit)).last
  }

  def order: (Int, String) = (desiredOffset, desiredModuleName)

  def isIndexed: Boolean = this.isInstanceOf[DifftestWithIndex]
  def getIndex: Option[UInt] = {
    this match {
      case b: DifftestWithIndex => Some(b.index)
      case _ => None
    }
  }

  def needUpdate: Option[Bool] = if (hasValid) Some(getValid) else None

  protected val needFlatten: Boolean = false
  def isFlatten: Boolean = hasAddress && this.needFlatten

  // Elements without clock, coreid, and index.
  def diffElements: Seq[(String, Seq[UInt])] = {
    val filteredElements = Seq("clock", "coreid", "index")
    val raw = elements.toSeq.reverse.filterNot(e => filteredElements.contains(e._1))
    raw.map{ case (s, data) =>
      data match {
        case v: Vec[_] => (s, Some(v.asInstanceOf[Vec[UInt]]))
        case u: UInt => (s, Some(Seq(u)))
        case _ => println(s"Unknown type: ($s, $data)")
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
  def toCppDeclaration: String = {
    val cpp = ListBuffer.empty[String]
    cpp += "typedef struct {"
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
  // returns a squashed, right-value Bundle. Default: overriding `base` with `this`
  def squash(base: DifftestBundle): DifftestBundle = this
}

class DiffArchEvent extends ArchEvent with DifftestBundle {
  // DiffArchEvent must be instantiated once for each core.
  override def isUniqueIdentifier: Boolean = true
  override val desiredCppName: String = "event"
}

class DiffInstrCommit(nPhyRegs: Int = 32) extends InstrCommit(nPhyRegs)
  with DifftestBundle
  with DifftestWithIndex
{
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
    when (valid && that.valid) {
      squashed.nFused := nFused + that.nFused + 1.U
    }
    squashed
  }
}

class DiffTrapEvent extends TrapEvent with DifftestBundle {
  override val desiredCppName: String = "trap"
  override def needUpdate: Option[Bool] = Some(hasTrap || hasWFI)
  override def supportsSquashBase: Bool = !hasTrap && !hasWFI
}

class DiffCSRState extends CSRState with DifftestBundle {
  override val desiredCppName: String = "csr"
  override val desiredOffset: Int = 1
  override val squashDependency: Seq[String] = Seq("commit", "event")
}

class DiffDebugMode extends DebugModeCSRState with DifftestBundle {
  override val desiredCppName: String = "dmregs"
}

class DiffIntWriteback(numRegs: Int = 32) extends DataWriteback(numRegs) with DifftestBundle {
  override val desiredCppName: String = "wb_int"
  override protected val needFlatten: Boolean = true
  // TODO: We have a special and temporary fix for int writeback in Squash.scala
  // It is only required for MMIO data synchronization for single-core co-sim
  override def supportsSquashBase: Bool = true.B
}

class DiffFpWriteback(numRegs: Int = 32) extends DiffIntWriteback(numRegs) {
  override val desiredCppName: String = "wb_fp"
}

class DiffArchIntRegState extends ArchIntRegState with DifftestBundle {
  override val desiredCppName: String = "regs_int"
  override val desiredOffset: Int = 0
}

abstract class DiffArchDelayedUpdate(numRegs: Int) extends ArchDelayedUpdate(numRegs)
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
}

class DiffSbufferEvent extends SbufferEvent with DifftestBundle
  with DifftestWithIndex
{
  override val desiredCppName: String = "sbuffer"
}

class DiffStoreEvent extends StoreEvent with DifftestBundle
  with DifftestWithIndex
{
  override val desiredCppName: String = "store"
}

class DiffLoadEvent extends LoadEvent with DifftestBundle
  with DifftestWithIndex
{
  override val desiredCppName: String = "load"
  // TODO: currently we assume it can be dropped
  override def supportsSquashBase: Bool = true.B
}

class DiffAtomicEvent extends AtomicEvent with DifftestBundle {
  override val desiredCppName: String = "atomic"
}

class DiffL1TLBEvent extends L1TLBEvent with DifftestBundle
  with DifftestWithIndex
{
  override val desiredCppName: String = "l1tlb"
  // TODO: currently we assume it can be dropped
  override def supportsSquashBase: Bool = true.B
}

class DiffL2TLBEvent extends L2TLBEvent with DifftestBundle
  with DifftestWithIndex
{
  override val desiredCppName: String = "l2tlb"
  // TODO: currently we assume it can be dropped
  override def supportsSquashBase: Bool = true.B
}

class DiffRefillEvent extends RefillEvent with DifftestBundle
  with DifftestWithIndex
{
  override val desiredCppName: String = "refill"
  // TODO: currently we assume it can be dropped
  override def supportsSquashBase: Bool = true.B
}

class DiffLrScEvent extends ScEvent with DifftestBundle {
  override val desiredCppName: String = "lrsc"
}

class DiffRunaheadEvent extends RunaheadEvent with DifftestBundle
  with DifftestWithIndex
{
  override val desiredCppName: String = "runahead"
}

class DiffRunaheadCommitEvent extends RunaheadEvent with DifftestBundle
  with DifftestWithIndex
{
  override val desiredCppName: String = "runahead_commit"
}

class DiffRunaheadRedirectEvent extends RunaheadRedirectEvent with DifftestBundle {
  override val desiredCppName: String = "runahead_redirect"
}

trait DifftestModule[T <: DifftestBundle] {
  val io: T
}

object DifftestModule {
  private val enabled = true
  private val instances = ListBuffer.empty[(DifftestBundle, String)]
  private val macros = ListBuffer.empty[String]

  def apply[T <: DifftestBundle](
    gen:      T,
    style:    String  = "dpic",
    dontCare: Boolean = false,
    delay:    Int     = 0,
  ): T = {
    val difftest: T = Wire(gen)
    if (enabled) {
      val id = register(gen, style)
      val sink = style match {
        case "batch" => Batch(gen)
        // By default, use the DPI-C style.
        case _ => DPIC(gen)
      }
      sink := Squash(Delayer(difftest, delay))
      sink.coreid := difftest.coreid
    }
    if (dontCare) {
      difftest := DontCare
    }
    difftest
  }

  def register[T <: DifftestBundle](gen: T, style: String): Int = {
    val id = instances.length
    val element = (gen, style)
    instances += element
    id
  }

  def hasDPIC: Boolean = instances.exists(_._2 == "dpic")
  def hasBatch: Boolean = instances.exists(_._2 == "batch")
  def finish(cpu: String, cppHeader: Option[String] = Some("dpic")): Unit = {
    val difftest_step = IO(Output(Bool()))
    difftest_step := true.B

    if (hasDPIC) {
      val dpic_tuple = DPIC.collect()
      macros ++= dpic_tuple._1
      difftest_step := dpic_tuple._2
    }
    if (hasBatch) {
      macros ++= Batch.collect()
    }
    macros ++= Squash.collect()
    if (cppHeader.isDefined) {
      generateCppHeader(cpu, cppHeader.get)
    }
  }

  def generateCppHeader(cpu: String, style: String): Unit = {
    val difftestCpp = ListBuffer.empty[String]
    difftestCpp += "#ifndef __DIFFSTATE_H__"
    difftestCpp += "#define __DIFFSTATE_H__"
    difftestCpp += ""
    difftestCpp += "#include <cstdint>"
    difftestCpp += ""

    macros.foreach(m => difftestCpp += s"#define $m")
    difftestCpp += ""

    val cpu_s = cpu.replace("-", "_").replace(" ", "").toUpperCase
    difftestCpp += s"#define CPU_$cpu_s"
    difftestCpp += ""

    val headerInstances = instances.filter(_._2 == style)

    val numCores = headerInstances.count(_._1.isUniqueIdentifier)
    if (headerInstances.nonEmpty) {
      difftestCpp += s"#define NUM_CORES $numCores"
      difftestCpp += ""
    }

    val uniqBundles = headerInstances.groupBy(_._1.desiredModuleName)
    // Create cpp declaration for each bundle type
    uniqBundles.values.map(_.map(_._1)).foreach(bundles => {
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
      difftestCpp += bundleType.toCppDeclaration
      difftestCpp += ""
    })

    // create top-level difftest struct
    difftestCpp += "typedef struct {"
    for ((className, cppInstances) <- uniqBundles.toSeq.sortBy(_._2.head._1.order)) {
      val bundleType = cppInstances.head._1
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

    val class_def =
      s"""
         |class DiffStateBuffer {
         |public:
         |  virtual ~DiffStateBuffer() {}
         |  virtual DiffTestState* get() = 0;
         |  virtual DiffTestState* next() = 0;
         |};
         |
         |extern DiffStateBuffer* diffstate_buffer;
         |
         |extern void diffstate_buffer_init();
         |extern void diffstate_buffer_free();
         |""".stripMargin

    difftestCpp += class_def
    difftestCpp += "#endif // __DIFFSTATE_H__"
    difftestCpp += ""

    val outputDir = sys.env("NOOP_HOME") + "/build/generated-src"
    Files.createDirectories(Paths.get(outputDir))
    val outputFile = outputDir + "/diffstate.h"
    Files.write(Paths.get(outputFile), difftestCpp.mkString("\n").getBytes(StandardCharsets.UTF_8))
  }
}

private class Delayer[T <: Data](gen: T, n_cycles: Int) extends Module {
  val i = IO(Input(gen.cloneType))
  val o = IO(Output(gen.cloneType))

  var r = WireInit(i)
  for (_ <- 0 until n_cycles) {
    r = RegNext(r)
  }
  o := r
}

object Delayer {
  def apply[T <: Data](gen: T, n_cycles: Int): T = {
    if (n_cycles > 0) {
      val delayer = Module(new Delayer(gen, n_cycles))
      delayer.i := gen
      delayer.o
    }
    else {
      gen
    }
  }
}

// Difftest emulator top

// XiangShan log / perf ctrl, should be inited in SimTop IO
// If not needed, just ingore these signals
class PerfInfoIO extends Bundle {
  val clean = Input(Bool())
  val dump = Input(Bool())
}

class LogCtrlIO extends Bundle {
  val log_begin, log_end = Input(UInt(64.W))
  val log_level = Input(UInt(64.W)) // a cpp uint
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
