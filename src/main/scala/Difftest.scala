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
import difftest.dpic.DPIC

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths}
import scala.collection.mutable.ListBuffer

trait DifftestWithClock {
  val clock  = Clock()
}

trait DifftestWithCoreid {
  val coreid = UInt(8.W)
}

trait DifftestWithIndex {
  val index = UInt(8.W)
}

trait DifftestWithValid {
  val valid = Bool()
}

trait DifftestWithAddress { this: DifftestWithValid =>
  val numElements: Int
  val needFlatten: Boolean = false

  val address = UInt(log2Ceil(numElements).W)
}

abstract class DifftestBundle extends Bundle
  with DifftestWithClock
  with DifftestWithCoreid {
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

  def withValid: Boolean = this.isInstanceOf[DifftestWithValid]
  def getValid: Bool = {
    this match {
      case b: DifftestWithValid => b.valid
      case _ => true.B
    }
  }
  def isFlatten: Boolean = this.isInstanceOf[DifftestWithAddress] &&
    this.asInstanceOf[DifftestWithAddress].needFlatten
  def numFlattenElements: Int = this.asInstanceOf[DifftestWithAddress].numElements

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

  def diffBytes(aligned: Int = 8): Seq[UInt] = {
    val sizes = diffSizes(aligned)
    diffElements.map(_._2).zip(sizes).flatMap{ case (elem, size) =>
      elem.zip(size).flatMap { case (e, s) =>
        (0 until s).map(i => {
          val msb = Seq(i * aligned + aligned - 1, e.getWidth - 1).min
          e(msb, i * aligned)
        })
      }
    }
  }

  // Size of this DiffTest Bundle.
  def size(round: Int): Int = diffSizes(round).map(_.sum).sum
  // Byte size of this DiffTest Bundle.
  def byteSize: Int = size(8)

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
}

class DiffArchEvent extends DifftestBundle
  with DifftestWithValid
{
  val interrupt     = UInt(32.W)
  val exception     = UInt(32.W)
  val exceptionPC   = UInt(64.W)
  val exceptionInst = UInt(32.W)

  // DiffArchEvent must be instantiated once for each core.
  override def isUniqueIdentifier: Boolean = true
  override val desiredCppName: String = "event"
}

class DiffInstrCommit(numPhyRegs: Int = 32) extends DifftestBundle
  with DifftestWithIndex
  with DifftestWithValid
{
  val skip     = Bool()
  val isRVC    = Bool()
  val rfwen    = Bool()
  val fpwen    = Bool()
  val vecwen   = Bool()
  val wpdest   = UInt(log2Ceil(numPhyRegs).W)
  val wdest    = UInt(8.W)

  val pc       = UInt(64.W)
  val instr    = UInt(32.W)
  val robIdx   = UInt(10.W)
  val lqIdx    = UInt(7.W)
  val sqIdx    = UInt(7.W)
  val isLoad   = Bool()
  val isStore  = Bool()
  val nFused   = UInt(8.W)
  val special  = UInt(8.W)

  def setSpecial(
    isDelayedWb: Bool = false.B,
    isExit: Bool = false.B,
  ): Unit = {
    special := Cat(isExit, isDelayedWb)
  }
  override val desiredCppName: String = "commit"
}

class DiffTrapEvent extends DifftestBundle {
  val hasTrap  = Bool()
  val cycleCnt = UInt(64.W)
  val instrCnt = UInt(64.W)
  val hasWFI   = Bool()

  val code     = UInt(3.W)
  val pc       = UInt(64.W)

  override val desiredCppName: String = "trap"
}

class DiffCSRState extends DifftestBundle {
  val priviledgeMode = UInt(64.W)
  val mstatus = UInt(64.W)
  val sstatus = UInt(64.W)
  val mepc = UInt(64.W)
  val sepc = UInt(64.W)
  val mtval = UInt(64.W)
  val stval = UInt(64.W)
  val mtvec = UInt(64.W)
  val stvec = UInt(64.W)
  val mcause = UInt(64.W)
  val scause = UInt(64.W)
  val satp = UInt(64.W)
  val mip = UInt(64.W)
  val mie = UInt(64.W)
  val mscratch = UInt(64.W)
  val sscratch = UInt(64.W)
  val mideleg = UInt(64.W)
  val medeleg = UInt(64.W)
  override val desiredCppName: String = "csr"
  override val desiredOffset: Int = 1
}

class DiffDebugMode extends DifftestBundle {
  val debugMode = Bool()
  val dcsr = UInt(64.W)
  val dpc = UInt(64.W)
  val dscratch0 = UInt(64.W)
  val dscratch1 = UInt(64.W)
  override val desiredCppName: String = "dmregs"
}

class DiffIntWriteback(val numElements: Int = 32) extends DifftestBundle
  with DifftestWithValid
  with DifftestWithAddress
{
  val data  = UInt(64.W)
  override val desiredCppName: String = "wb_int"
  override val needFlatten: Boolean = true
}

class DiffFpWriteback(numElements: Int = 32) extends DiffIntWriteback(numElements) {
  override val desiredCppName: String = "wb_fp"
}

class DiffArchIntRegState extends DifftestBundle {
  val value = Vec(32, UInt(64.W))
  override val desiredCppName: String = "regs_int"
  override val desiredOffset: Int = 0
}

abstract class DiffArchDelayedUpdate(val numElements: Int = 32) extends DifftestBundle
  with DifftestWithIndex
  with DifftestWithValid
  with DifftestWithAddress
{
  val data = UInt(64.W)
  val nack = Bool()
}

class DiffArchIntDelayedUpdate extends DiffArchDelayedUpdate {
  override val desiredCppName: String = "regs_int_delayed"
}

class DiffArchFpDelayedUpdate extends DiffArchDelayedUpdate {
  override val desiredCppName: String = "regs_fp_delayed"
}

class DiffArchFpRegState extends DifftestBundle {
  val value = Vec(32, UInt(64.W))
  override val desiredCppName: String = "regs_fp"
  override val desiredOffset: Int = 2
}

class DiffArchVecRegState extends DifftestBundle {
  val value = Vec(64, UInt(64.W))
  override val desiredCppName: String = "regs_vec"
  override val desiredOffset: Int = 4
}

class DiffVecCSRState extends DifftestBundle {
  val vstart = UInt(64.W)
  val vxsat  = UInt(64.W)
  val vxrm   = UInt(64.W)
  val vcsr   = UInt(64.W)
  val vl     = UInt(64.W)
  val vtype  = UInt(64.W)
  val vlenb  = UInt(64.W)
  override val desiredCppName: String = "vcsr"
  override val desiredOffset: Int = 5
}

class DiffSbufferEvent extends DifftestBundle
  with DifftestWithIndex
  with DifftestWithValid
{
  val addr = UInt(64.W)
  val data = Vec(64, UInt(8.W))
  val mask = UInt(64.W)
  override val desiredCppName: String = "sbuffer"
}

class DiffStoreEvent extends DifftestBundle
  with DifftestWithIndex
  with DifftestWithValid
{
  val addr   = UInt(64.W)
  val data   = UInt(64.W)
  val mask   = UInt(8.W)
  override val desiredCppName: String = "store"
}

class DiffLoadEvent extends DifftestBundle
  with DifftestWithIndex
  with DifftestWithValid
{
  val paddr  = UInt(64.W)
  val opType = UInt(8.W)
  val fuType = UInt(8.W)
  override val desiredCppName: String = "load"
}

class DiffAtomicEvent extends DifftestBundle
  with DifftestWithValid
{
  val addr = UInt(64.W)
  val data = UInt(64.W)
  val mask = UInt(8.W)
  val fuop = UInt(8.W)
  val out  = UInt(64.W)
  override val desiredCppName: String = "atomic"
}

class DiffL1TLBEvent extends DifftestBundle
  with DifftestWithValid
  with DifftestWithIndex
{
  val satp = UInt(64.W)
  val vpn = UInt(64.W)
  val ppn = UInt(64.W)
  override val desiredCppName: String = "l1tlb"
}

class DiffL2TLBEvent extends DifftestBundle
  with DifftestWithValid
  with DifftestWithIndex
{
  val valididx = Vec(8, Bool())
  val satp = UInt(64.W)
  val vpn = UInt(64.W)
  val ppn = Vec(8, UInt(64.W))
  val perm = UInt(8.W)
  val level = UInt(8.W)
  val pf = Bool()
  override val desiredCppName: String = "l2tlb"
}

class DiffRefillEvent extends DifftestBundle
  with DifftestWithValid
  with DifftestWithIndex
{
  val addr  = UInt(64.W)
  val data  = Vec(8, UInt(64.W))
  val idtfr = UInt(8.W) // identifier for flexible usage
  override val desiredCppName: String = "refill"
}

class DiffLrScEvent extends DifftestBundle
  with DifftestWithValid
{
  val success = Bool()
  override val desiredCppName: String = "lrsc"
}

class DiffRunaheadEvent extends DifftestBundle
  with DifftestWithIndex
  with DifftestWithValid
{
  val branch        = Bool()
  val may_replay    = Bool()
  val pc            = UInt(64.W)
  val checkpoint_id = UInt(64.W)
  override val desiredCppName: String = "runahead"
}

class DiffRunaheadCommitEvent extends DifftestBundle
  with DifftestWithIndex
  with DifftestWithValid
{
  val pc            = UInt(64.W)
  override val desiredCppName: String = "runahead_commit"
}

class DiffRunaheadRedirectEvent extends DifftestBundle
  with DifftestWithValid
{
  val pc            = UInt(64.W) // for debug only
  val target_pc     = UInt(64.W) // for debug only
  val checkpoint_id = UInt(64.W)
  override val desiredCppName: String = "runahead_redirect"
}

class DiffCoverage(
  val desiredCppName: String,
  val numElements: Int
) extends DifftestBundle
  with DifftestWithValid
  with DifftestWithAddress
{
  val covered = Bool()
  override val needFlatten: Boolean = true
}

trait DifftestModule[T <: DifftestBundle] {
  val io: T
}

object DifftestModule {
  private val instances = ListBuffer.empty[(DifftestBundle, String)]

  def apply[T <: DifftestBundle](
    gen:      T,
    style:    String  = "dpic",
    dontCare: Boolean = false,
    delay:    Int     = 0,
  ): T = {
    val id = register(gen, style)
    val mod = style match {
      // By default, use the DPI-C style.
      case _ => DPIC(gen, delay)
    }
    if (dontCare) {
      mod := DontCare
    }
    mod
  }

  def register[T <: DifftestBundle](gen: T, style: String): Int = {
    val id = instances.length
    val element = (gen, style)
    instances += element
    id
  }

  def hasDPIC: Boolean = instances.exists(_._2 == "dpic")
  def finish(cpu: String, cppHeader: Boolean = true): Unit = {
    DPIC.collect()
    if (cppHeader) {
      generateCppHeader(cpu)
    }
  }

  def generateCppHeader(cpu: String): Unit = {
    val difftestCpp = ListBuffer.empty[String]
    difftestCpp += "#ifndef __DIFFSTATE_H__"
    difftestCpp += "#define __DIFFSTATE_H__"
    difftestCpp += ""
    difftestCpp += "#include <cstdint>"
    difftestCpp += ""

    val cpu_s = cpu.replace("-", "_").replace(" ", "").toUpperCase
    difftestCpp += s"#define CPU_$cpu_s"
    difftestCpp += ""

    val numCores = instances.count(_._1.isUniqueIdentifier)
    difftestCpp += s"#define NUM_CORES $numCores"
    difftestCpp += ""

    val uniqBundles = instances.groupBy(_._1.desiredModuleName)
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
        difftestCpp += s"#define $configWidthName ${bundleType.numFlattenElements}"
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
      val instanceCount = if (bundleType.isFlatten) bundleType.numFlattenElements else nInstances / numCores
      require(nInstances % numCores == 0, s"Cores seem to have different # of ${instanceName}")
      require(cppIsArray || nInstances == numCores, s"# of ${instanceName} should not be ${nInstances}")
      val arrayWidth = if (cppIsArray) s"[$instanceCount]" else ""
      difftestCpp += f"  $className%-30s $instanceName$arrayWidth;"
    }
    difftestCpp += "} DiffTestState;"
    difftestCpp += ""

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
