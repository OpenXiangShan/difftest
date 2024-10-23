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
import chisel3.reflect.DataMirror
import difftest.common.DifftestWiring
import difftest.gateway.{Gateway, GatewayConfig}

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths, StandardOpenOption}
import scala.annotation.tailrec
import scala.collection.mutable.ListBuffer
import org.json4s.DefaultFormats
import org.json4s.native.Serialization.writePretty

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

  def toTraceDeclaration: String = {
    def byteWidth(data: Data) = (data.getWidth + 7) / 8 * 8
    val cpp = ListBuffer.empty[String]
    cpp += "typedef struct __attribute__((packed)) {"
    elements.toSeq.reverse.foreach { case (name, data) =>
      val (typeWidth, arrSuffix) = data match {
        case v: Vec[_] => (byteWidth(v.head), s"[${v.length}]")
        case _         => (byteWidth(data), "")
      }
      cpp += f"  ${s"uint${typeWidth}_t"}%-8s $name$arrSuffix;"
    }
    cpp += s"} ${desiredModuleName.replace("Difftest", "DiffTrace")};"
    cpp.mkString("\n")
  }

  def toJsonProfile: Map[String, Any] = Map("className" -> this.getClass.getName)

  // returns a Seq indicating the udpate dependencies. Default: empty
  // Only when one of the dependencies is valid, this bundle is updated.
  val updateDependency: Seq[String] = Seq()

  // returns Bool indicating whether `this` bundle can be squashed with `base`
  def supportsSquash(base: DifftestBundle): Bool = supportsSquashBase
  def supportsSquashBase: Bool = if (hasValid) !getValid else true.B
  // returns a seq of Group name of this bundle, Default: REF
  // Only bundles with same GroupName will affect others' squash state.
  // Some bundle will have several GroupName, such as LoadEvent
  // Optional GroupName: REF / GOLDENMEM
  val squashGroup: Seq[String] = Seq("REF")
  // returns a squashed, right-value Bundle. Default: overriding `base` with `this`
  def squash(base: DifftestBundle): DifftestBundle = this
  def squashQueue: Boolean = false

  // When enable squash, we append valid signal for DifftestBundle
  def genValidBundle(valid: Bool): Valid[DifftestBundle] = {
    val gen = Wire(Valid(chiselTypeOf(this)))
    gen.valid := valid
    gen.bits := this
    gen
  }
  def genValidBundle: Valid[DifftestBundle] = genValidBundle(this.getValid)

  // Byte align all elements
  def getByteAlignElems(isTrace: Boolean): Seq[(String, Data)] = {
    def byteAlign(data: Data): UInt = {
      val width: Int = (data.getWidth + 7) / 8 * 8
      data.asTypeOf(UInt(width.W))
    }
    val gen = if (DataMirror.isWire(this) || DataMirror.isReg(this) || DataMirror.isIO(this)) {
      this
    } else {
      0.U.asTypeOf(this)
    }
    val elems = if (isTrace) {
      gen.elements.toSeq.reverse
    } else {
      // Reorder to separate locating and transmitted data
      def locFilter: ((String, Data)) => Boolean = { case (name, _) =>
        Seq("coreid", "index", "address").contains(name)
      }
      val raw = gen.elements.toSeq.reverse.filterNot(this.isFlatten && _._1 == "valid")
      raw.filterNot(locFilter) ++ raw.filter(locFilter)
    }
    elems.flatMap { case (name, data) =>
      data match {
        case vec: Vec[_] => vec.zipWithIndex.map { case (v, i) => (s"{${name}_$i}", byteAlign(v)) }
        case _           => Seq((s"$name", byteAlign(data)))
      }
    }
  }
  def getByteAlignElems: Seq[(String, Data)] = getByteAlignElems(false)
  def getByteAlign(isTrace: Boolean): UInt = MixedVecInit(getByteAlignElems(isTrace).map(_._2)).asUInt
  def getByteAlign: UInt = getByteAlign(false)
  def getByteAlignWidth(isTrace: Boolean): Int = this.getByteAlign(isTrace).getWidth
  def getByteAlignWidth: Int = getByteAlignWidth(false)
  def reverseByteAlign(aligned: UInt, isTrace: Boolean): DifftestBundle = {
    require(aligned.getWidth == this.getByteAlignWidth(isTrace))
    val bundle = WireInit(0.U.asTypeOf(this))
    val byteSeq = aligned.asTypeOf(Vec(aligned.getWidth / 8, UInt(8.W)))
    val elems = bundle.elements.toSeq.reverse
      .filterNot(this.isFlatten && _._1 == "valid" && !isTrace)
      .flatMap { case (_, data) =>
        data match {
          case vec: Vec[_] => vec.toSeq
          case _           => Seq(data)
        }
      }
      .map { d => (d, (d.getWidth + 7) / 8) }
    elems.zipWithIndex.foreach { case ((data, size), idx) =>
      val offset = elems.map(_._2).take(idx).sum
      data := MixedVecInit(byteSeq.slice(offset, offset + size).toSeq).asUInt
    }
    bundle
  }
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

  override def toJsonProfile: Map[String, Any] = super.toJsonProfile ++ Map("nPhyRegs" -> nPhyRegs)
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
  override val updateDependency: Seq[String] = Seq("commit", "event")
}

class DiffHCSRState extends HCSRState with DifftestBundle {
  override val desiredCppName: String = "hcsr"
  override val desiredOffset: Int = 6
  override val updateDependency: Seq[String] = Seq("commit", "event")
}

//class DiffHCSRStateValidate extends DiffHCSRState with DifftestIsValidated

class DiffDebugMode extends DebugModeCSRState with DifftestBundle {
  override val desiredCppName: String = "dmregs"
}

class DiffTriggerCSRState extends TriggerCSRState with DifftestBundle {
  override val desiredCppName: String = "triggercsr"
  override val updateDependency: Seq[String] = Seq("commit", "event")
}

class DiffIntWriteback(numRegs: Int = 32) extends DataWriteback(numRegs) with DifftestBundle {
  override val desiredCppName: String = "wb_int"
  override protected val needFlatten: Boolean = true
  // It is required for MMIO/Load(only for multi-core) data synchronization, and commit instr trace record
  override def supportsSquashBase: Bool = true.B
  override def toJsonProfile: Map[String, Any] = super.toJsonProfile ++ Map("numRegs" -> numRegs)
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
  override val updateDependency: Seq[String] = Seq("commit", "event")
}

abstract class DiffArchDelayedUpdate(numRegs: Int)
  extends ArchDelayedUpdate(numRegs)
  with DifftestBundle
  with DifftestWithIndex {
  override def toJsonProfile: Map[String, Any] = super.toJsonProfile ++ Map("numRegs" -> numRegs)
}

class DiffArchIntDelayedUpdate extends DiffArchDelayedUpdate(32) {
  override val desiredCppName: String = "regs_int_delayed"
}

class DiffArchFpDelayedUpdate extends DiffArchDelayedUpdate(32) {
  override val desiredCppName: String = "regs_fp_delayed"
}

class DiffArchFpRegState extends ArchIntRegState with DifftestBundle {
  override val desiredCppName: String = "regs_fp"
  override val desiredOffset: Int = 2
  override val updateDependency: Seq[String] = Seq("commit", "event")
}

class DiffArchVecRegState extends ArchVecRegState with DifftestBundle {
  override val desiredCppName: String = "regs_vec"
  override val desiredOffset: Int = 4
  override val updateDependency: Seq[String] = Seq("commit", "event")
}

class DiffVecCSRState extends VecCSRState with DifftestBundle {
  override val desiredCppName: String = "vcsr"
  override val desiredOffset: Int = 5
  override val updateDependency: Seq[String] = Seq("commit", "event")
}

class DiffFpCSRState extends FpCSRState with DifftestBundle {
  override val desiredCppName: String = "fcsr"
  override val desiredOffset: Int = 7
  override val updateDependency: Seq[String] = Seq("commit", "event")
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

class DiffCMOInvalEvent extends CMOInvalEvent with DifftestBundle {
  override val desiredCppName: String = "cmo_inval"
  override val squashGroup: Seq[String] = Seq("GOLDENMEM")
  // TODO: currently we assume it can be dropped
  override def supportsSquashBase: Bool = true.B
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

class DiffNonRegInterruptPendingEvent extends NonRegInterruptPendingEvent with DifftestBundle {
  override val desiredCppName: String = "non_reg_interrupt_pending"
}

class DiffMhpmeventOverflowEvent extends MhpmeventOverflowEvent with DifftestBundle {
  override val desiredCppName: String = "mhpmevent_overflow"
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
  private val cppExtModules = ListBuffer.empty[(String, String)]
  private val cppExtHeaders = ListBuffer.empty[String]
  private val vMacros = ListBuffer.empty[String]
  private val jsonProfiles = ListBuffer.empty[Map[String, Any]]

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
      Gateway(gen, delay) := difftest
    }
    if (dontCare) {
      difftest := DontCare
      difftest.bits.getValidOption.foreach(_ := false.B)
    }
    jsonProfiles += (gen.toJsonProfile ++ Map("delay" -> delay))
    difftest
  }

  def finish(cpu: String, createTopIO: Boolean): Option[DifftestTopIO] = {
    val gateway = Gateway.collect()
    cppMacros ++= gateway.cppMacros
    vMacros ++= gateway.vMacros
    instances ++= gateway.instances

    generateCppHeader(cpu, gateway.structPacked.getOrElse(false))
    if (gateway.cppExtModule.getOrElse(false)) {
      generateCppExtModules()
    }
    generateVeriogHeader()
    generateJsonProfile(cpu)

    Option.when(createTopIO) {
      if (enabled) {
        createTopIOs(gateway.exit, gateway.step)
      } else {
        WireInit(0.U.asTypeOf(new DifftestTopIO))
      }
    }
  }

  def finish(cpu: String): DifftestTopIO = {
    finish(cpu, true).get
  }

  def createTopIOs(exit: Option[UInt], step: Option[UInt]): DifftestTopIO = {
    val difftest = IO(new DifftestTopIO)

    difftest.exit := exit.getOrElse(0.U)
    difftest.step := step.getOrElse(0.U)

    val timer = RegInit(0.U(64.W)).suggestName("timer")
    timer := timer + 1.U
    dontTouch(timer)

    val log_enable = difftest.logCtrl.enable(timer).suggestName("log_enable")
    dontTouch(log_enable)

    difftest.uart := DontCare

    require(DifftestWiring.isEmpty, s"pending wires left: ${DifftestWiring.getPending}")

    difftest
  }

  def generateSvhInterface(instances: Seq[DifftestBundle], numCores: Int): Unit = {
    // generate interface by jsonProfile, single-core interface will be copied numCore times
    val difftestSvh = ListBuffer.empty[String]
    val core_if_len = instances.length / numCores
    val gateway_args = instances.zipWithIndex.map { case (b, idx) =>
      val typeString = s"logic [${b.getWidth - 1}: 0]"
      val argName = s"gateway_$idx"
      (typeString, argName)
    }
    val core_args = gateway_args.take(core_if_len)
    def getInterface(args: Seq[(String, String)]): String = {
      args.map { case (t, name) => s"$t $name;" }.mkString("\n")
    }
    def getModPort(args: Seq[(String, String)]): String = {
      args.map(_._2).mkString(", ")
    }
    val if_assigns = Seq
      .tabulate(numCores) { coreid =>
        val offset = coreid * core_if_len
        Seq.tabulate(core_if_len) { idx =>
          s"assign gateway_out.gateway_${offset + idx} = core_in[$coreid].gateway_$idx;"
        }
      }
      .flatten
      .mkString("\n")
    difftestSvh +=
      s"""|interface core_if;
          |${getInterface(core_args)}
          |modport in (input ${getModPort(core_args)});
          |modport out (output ${getModPort(core_args)});
          |endinterface
          |
          |interface gateway_if;
          |${getInterface(gateway_args)}
          |modport in (input ${getModPort(gateway_args)});
          |modport out (output ${getModPort(gateway_args)});
          |endinterface
          |
          |module CoreToGateway (
          |  gateway_if.out gateway_out,
          |  core_if.in core_in[$numCores]
          |);
          |$if_assigns
          |endmodule""".stripMargin
    streamToFile(difftestSvh, "gateway_interface.svh")
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

  def createCppExtModule(name: String, func: String, header: Option[String]): Unit = {
    // generate external modules only once
    if (!cppExtModules.exists(_._1 == name)) {
      cppExtModules += ((name, func))
      // Some cpp external module may have header dependency
      if (header.isDefined && !cppExtHeaders.contains(header.get)) {
        cppExtHeaders += header.get
      }
    }
  }
  def createCppExtModule(name: String, func: String): Unit = createCppExtModule(name, func, None)

  def generateCppExtModules(): Unit = {
    val difftestCppExts = ListBuffer.empty[String]
    difftestCppExts += "#ifdef GSIM"
    difftestCppExts += "#include <cstdint>"
    difftestCppExts += "#include \"SimTop.h\""
    cppExtHeaders.foreach(h => difftestCppExts += s"#include $h")
    cppExtModules.foreach(m => difftestCppExts += m._2)
    difftestCppExts += "#endif // GSIM"
    streamToFile(difftestCppExts, "difftest-extmodule.cpp")
  }
  def generateVeriogHeader(): Unit = {
    val difftestVeriog = ListBuffer.empty[String]
    vMacros.foreach(m => difftestVeriog += s"`define $m")
    streamToFile(difftestVeriog, "DifftestMacros.v")
  }

  def generateJsonProfile(cpu: String): Unit = {
    val difftestJson = ListBuffer.empty[String]
    val profile = jsonProfiles ++ Map("cpu" -> cpu)
    difftestJson += writePretty(profile)(DefaultFormats)
    streamToFile(difftestJson, "difftest_profile.json")
  }

  def streamToFile(fileStream: ListBuffer[String], fileName: String, append: Boolean = false) {
    val outputDir = sys.env("NOOP_HOME") + "/build/generated-src/"
    Files.createDirectories(Paths.get(outputDir))
    val outputFile = outputDir + fileName
    val options = if (append) {
      Seq(StandardOpenOption.CREATE, StandardOpenOption.APPEND)
    } else {
      Seq(StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING)
    }
    Files.write(
      Paths.get(outputFile),
      (fileStream.mkString("\n") + "\n").getBytes(StandardCharsets.UTF_8),
      options: _*
    )
  }
}

// Difftest emulator top. Will be created by DifftestModule.finish
class DifftestTopIO extends Bundle {
  val exit = Output(UInt(64.W))
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
