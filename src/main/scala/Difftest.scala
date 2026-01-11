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
import chisel3.reflect.DataMirror
import circt.stage.FirtoolOption
import chisel3.util._
import difftest.common.FileControl
import difftest.gateway.{Gateway, GatewayConfig, GatewayResult}
import difftest.util.Profile

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
  // A desired offset for registers to be compared with the REF in the C++ struct can be specified.
  // Since currently we cannot check the semantic correctness, this should strictly match the REF.
  val desiredRegOffset: Option[Int] = None

  val desiredCppName: String
  def actualCppName: String = if (desiredRegOffset.isDefined) s"regs.$desiredCppName" else desiredCppName
  def desiredModuleName: String = {
    val className = {
      val name = this.getClass.getName.replace("$", ".").replace("Diff", "Difftest")
      if (squashQueue) name.replace("Queue", "") else name
    }
    className.split("\\.").filterNot(_.forall(java.lang.Character.isDigit)).last
  }

  def order: (Int, String) = (desiredRegOffset.getOrElse(999), desiredModuleName)

  def isIndexed: Boolean = this.isInstanceOf[DifftestWithIndex]
  def getIndex: Option[UInt] = {
    this match {
      case b: DifftestWithIndex => Some(b.index)
      case _                    => None
    }
  }

  protected val needFlatten: Boolean = false
  def isFlatten: Boolean = hasAddress && this.needFlatten

  // Convert elements into flatten UInt/Vec[UInt]
  private def seqUIntHelper(in: Seq[(String, Data)]): Seq[(String, Seq[UInt])] = {
    in.flatMap { case (s, data) =>
      data match {
        case v: Vec[_] =>
          v.foreach(e => require(e.isInstanceOf[UInt], s"Vec of $e is not supported yet"))
          Some((s, v.asInstanceOf[Vec[UInt]]))
        case u: UInt   => Some((s, Seq(u)))
        case b: Bundle => seqUIntHelper(b.elements.toSeq.reverse).map(x => (s"${s}_${x._1}", x._2))
        case _         => throw new Exception(s"Unsupported data type: ($s, $data)")
      }
    }
  }

  def elementsInSeqUInt: Seq[(String, Seq[UInt])] = seqUIntHelper(elements.toSeq.reverse)

  // return (name, data_width_aligned, data_seq) for all elements, where width can be 8,16,32,64
  def totalElements: Seq[(String, Int, Seq[UInt])] = {
    elementsInSeqUInt.map { case (name, dataSeq) =>
      val width = dataSeq.map(_.getWidth).distinct
      require(width.length == 1, "should not have different width")
      require(width.head <= 64, s"do not support DifftestBundle element with width (${width.head}) >= 64")
      (name, math.pow(2, math.max(3, log2Ceil(width.head))).toInt, dataSeq)
    }
  }

  // return (name, data_width_aligned, data_seq) for all elements except coreid and index
  def dataElements: Seq[(String, Int, Seq[UInt])] =
    totalElements.filterNot(e => Seq("coreid", "index").contains(e._1))

  def toCppDeclMacro: String = {
    val macroName = s"CONFIG_DIFFTEST_${desiredModuleName.toUpperCase.replace("DIFFTEST", "")}"
    s"#define $macroName"
  }
  def toCppDeclaration(packed: Boolean, aligned: Boolean): String = {
    val cpp = ListBuffer.empty[String]
    val attribute = if (packed) "__attribute__((packed))" else ""
    cpp += s"typedef struct $attribute {"
    for ((name, size, elem) <- dataElements) {
      val isRemoved = isFlatten && Seq("valid", "address").contains(name)
      if (!isRemoved) {
        // Align elem to 8 bytes for bundle enabled to split when Delta
        val elemWidth = if (this.supportsDelta && aligned) deltaElemWidth else size
        val arrayType = s"uint${elemWidth}_t"
        val arrayWidth = if (elem.length == 1) "" else s"[${elem.length}]"
        cpp += f"  $arrayType%-8s $name$arrayWidth;"
      }
    }
    cpp += s"} ${desiredModuleName};"
    cpp.mkString("\n")
  }

  def toTraceDeclaration: String = {
    val cpp = ListBuffer.empty[String]
    cpp += "typedef struct __attribute__((packed)) {"
    totalElements.foreach { case (name, width, data) =>
      val (typeWidth, arrSuffix) = data match {
        case v: Vec[_] => (width, s"[${v.length}]")
        case _         => (width, "")
      }
      cpp += f"  ${s"uint${typeWidth}_t"}%-8s $name$arrSuffix;"
    }
    cpp += s"} ${desiredModuleName.replace("Difftest", "DiffTrace")};"
    cpp.mkString("\n")
  }

  // TODO: this should be implemented using reflection.
  def classArgs: Map[String, Any] = Map()

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

  val supportsDelta: Boolean = false
  def isDeltaElem: Boolean = this.isInstanceOf[DiffDeltaElem]
  def deltaElemWidth: Int = dataElements.map(_._2).max

  // Byte align all elements
  def getByteAlignElems(isTrace: Boolean): Seq[(String, Data)] = {
    val gen = if (DataMirror.isWire(this) || DataMirror.isReg(this) || DataMirror.isIO(this)) {
      this
    } else {
      0.U.asTypeOf(this)
    }
    val elems = if (isTrace) {
      gen.totalElements
    } else {
      // Reorder to separate locating and transmitted data
      def locFilter: ((String, Int, Seq[UInt])) => Boolean = { case (name, _, _) =>
        Seq("coreid", "index", "address").contains(name)
      }
      val raw = gen.totalElements.filterNot(this.isFlatten && _._1 == "valid")
      raw.filterNot(locFilter) ++ raw.filter(locFilter)
    }
    elems.flatMap { case (name, width, seq) =>
      seq.map(d => (name, d.asTypeOf(UInt(width.W))))
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
    val elems = bundle.totalElements
      .filterNot(this.isFlatten && _._1 == "valid" && !isTrace)
      .flatMap { case (_, width, seq) =>
        seq.map(d => (d, width / 8))
      }
    elems.zipWithIndex.foreach { case ((data, size), idx) =>
      val offset = elems.map(_._2).take(idx).sum
      data := MixedVecInit(byteSeq.slice(offset, offset + size).toSeq).asUInt
    }
    bundle
  }
}

private[difftest] class DiffDeltaElem(gen: DifftestBundle)
  extends DeltaElem(gen.deltaElemWidth)
  with DifftestBundle
  with DifftestWithIndex {
  override val desiredCppName: String = gen.desiredCppName + "_elem"
  override def desiredModuleName: String = gen.desiredModuleName + "Elem"
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

  override def classArgs: Map[String, Any] = Map("nPhyRegs" -> nPhyRegs)
}

private[difftest] class DiffCommitData extends CommitData with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "commit_data"
  override def supportsSquashBase: Bool = true.B
}

private[difftest] class DiffVecCommitData extends VecCommitData with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "vec_commit_data"
  override def supportsSquashBase: Bool = true.B
}

class DiffTrapEvent extends TrapEvent with DifftestBundle {
  override val desiredCppName: String = "trap"
  override def supportsSquashBase: Bool = !hasTrap && !hasWFI
}

class DiffCSRState extends CSRState with DifftestBundle {
  override val desiredCppName: String = "csr"
  override val desiredRegOffset: Option[Int] = Some(2)
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

class DiffHCSRState extends HCSRState with DifftestBundle {
  override val desiredCppName: String = "hcsr"
  override val desiredRegOffset: Option[Int] = Some(3)
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

//class DiffHCSRStateValidate extends DiffHCSRState with DifftestIsValidated

class DiffDebugMode extends DebugModeCSRState with DifftestBundle {
  override val desiredCppName: String = "dmregs"
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

class DiffTriggerCSRState extends TriggerCSRState with DifftestBundle {
  override val desiredCppName: String = "triggercsr"
  override val desiredRegOffset: Option[Int] = Some(7)
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

private[difftest] class DiffArchIntRegState extends ArchIntRegState with DifftestBundle {
  override val desiredCppName: String = "xrf"
  override val desiredRegOffset: Option[Int] = Some(0)
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

abstract class DiffArchDelayedUpdate(numRegs: Int)
  extends ArchDelayedUpdate(numRegs)
  with DifftestBundle
  with DifftestWithIndex {
  override def classArgs: Map[String, Any] = Map("numRegs" -> numRegs)
}

class DiffArchIntDelayedUpdate extends DiffArchDelayedUpdate(32) {
  override val desiredCppName: String = "regs_int_delayed"
}

class DiffArchFpDelayedUpdate extends DiffArchDelayedUpdate(32) {
  override val desiredCppName: String = "regs_fp_delayed"
}

private[difftest] class DiffArchFpRegState extends DiffArchIntRegState {
  override val desiredCppName: String = "frf"
  override val desiredRegOffset: Option[Int] = Some(1)
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

private[difftest] class DiffArchVecRegState extends ArchVecRegState with DifftestBundle {
  override val desiredCppName: String = "vrf"
  override val desiredRegOffset: Option[Int] = Some(4)
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

abstract class DiffArchRenameTable(numRegs: Int, val numPhyRegs: Int)
  extends ArchRenameTable(numRegs, numPhyRegs)
  with DifftestBundle {
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
  override def classArgs: Map[String, Any] = Map("numPhyRegs" -> numPhyRegs)
}

class DiffArchIntRenameTable(numPhyRegs: Int) extends DiffArchRenameTable(32, numPhyRegs) {
  override val desiredCppName: String = "rat_xrf"
}

class DiffArchFpRenameTable(numPhyRegs: Int) extends DiffArchRenameTable(32, numPhyRegs) {
  override val desiredCppName: String = "rat_frf"
}

class DiffArchVecRenameTable(numPhyRegs: Int) extends DiffArchRenameTable(64, numPhyRegs) {
  override val desiredCppName: String = "rat_vrf"
}

abstract class DiffPhyRegState(val numPhyRegs: Int) extends PhyRegState(numPhyRegs) with DifftestBundle {
  override val supportsDelta: Boolean = true
  override def classArgs: Map[String, Any] = Map("numPhyRegs" -> numPhyRegs)
  override val updateDependency: Seq[String] = Seq("commit", "event")
}

class DiffPhyIntRegState(numPhyRegs: Int) extends DiffPhyRegState(numPhyRegs) {
  override val desiredCppName: String = "pregs_xrf"
}

class DiffPhyFpRegState(numPhyRegs: Int) extends DiffPhyRegState(numPhyRegs) {
  override val desiredCppName: String = "pregs_frf"
}

class DiffPhyVecRegState(numPhyRegs: Int) extends DiffPhyRegState(numPhyRegs) {
  override val desiredCppName: String = "pregs_vrf"
}

class DiffVecCSRState extends VecCSRState with DifftestBundle {
  override val desiredCppName: String = "vcsr"
  override val desiredRegOffset: Option[Int] = Some(5)
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

class DiffFpCSRState extends FpCSRState with DifftestBundle {
  override val desiredCppName: String = "fcsr"
  override val desiredRegOffset: Option[Int] = Some(6)
  override val updateDependency: Seq[String] = Seq("commit", "event")
  override val supportsDelta: Boolean = true
}

class DiffSbufferEvent extends SbufferEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "sbuffer"
  override val squashGroup: Seq[String] = Seq("GOLDENMEM")
}

class DiffUncacheMMStoreEvent extends UncacheMMStoreEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "uncache_mm_store"
  override val squashGroup: Seq[String] = Seq("GOLDENMEM")
}

class DiffStoreEvent extends StoreEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "store"
}

private[difftest] class DiffStoreEventQueue extends DiffStoreEvent with DifftestWithStamp with DiffTestIsInherited {
  override val squashQueue: Boolean = true
}

class DiffLoadEvent extends LoadEvent with DifftestBundle with DifftestWithIndex {
  override val desiredCppName: String = "load"
  override val squashGroup: Seq[String] = Seq("REF", "GOLDENMEM")
}

private[difftest] class DiffLoadEventQueue extends DiffLoadEvent with DifftestWithStamp with DiffTestIsInherited {
  val commitData = UInt(64.W)
  val vecCommitData = Vec(16, UInt(64.W))
  val regWen = Bool()
  val wdest = UInt(8.W)
  val fpwen = Bool()
  val vecwen = Bool()
  val v0wen = Bool()
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

class DiffRunaheadCommitEvent extends RunaheadCommitEvent with DifftestBundle with DifftestWithIndex {
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

class DiffCriticalErrorEvent extends CriticalErrorEvent with DifftestBundle {
  override val desiredCppName: String = "critical_error"
}

class DiffSyncAIAEvent extends AIAEvent with DifftestBundle {
  override val desiredCppName: String = "sync_aia"
}

class DiffSyncCustomMflushpwrEvent extends SyncCustomMflushpwrEvent with DifftestBundle {
  override val desiredCppName: String = "sync_custom_mflushpwr"
}

private[difftest] class DiffTraceInfo(config: GatewayConfig) extends TraceInfo with DifftestBundle {
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

private[difftest] class DiffDeltaInfo extends DeltaInfo with DifftestBundle {
  override val desiredCppName: String = "delta_info"
}

trait DifftestModule[T <: DifftestBundle] {
  val io: T
}

object DifftestModule {
  private val enabled = true
  private val interfaces = ListBuffer.empty[(DifftestBundle, Int)]
  private val cppExtModules = ListBuffer.empty[(String, String)]
  private val cppExtHeaders = ListBuffer.empty[String]
  private val nameExcludes = ListBuffer.empty[String]
  private val cmdConfigs = ListBuffer.empty[String]

  // Some FIRTOOL options are customized for DiffTest
  def parseArgs(args: Array[String]): (Array[String], Seq[FirtoolOption]) = {
    cmdConfigs ++= args
    @tailrec
    def nextOption(args: Array[String], list: List[String]): Array[String] = {
      list match {
        case Nil => args
        case "--difftest-config" :: config :: tail =>
          Gateway.setConfig(config)
          nextOption(args.patch(args.indexOf("--difftest-config"), Nil, 2), tail)
        case "--difftest-exclude" :: names :: tail =>
          nameExcludes ++= names.split(",").map(_.trim)
          nextOption(args.patch(args.indexOf("--difftest-exclude"), Nil, 2), tail)
        case _ :: tail => nextOption(args, tail)
      }
    }

    val (chiselArgs, options) = difftest.Coverage.parseArgs(args)
    (nextOption(chiselArgs, args.toList), options)
  }

  def apply[T <: DifftestBundle](
    gen: T,
    dontCare: Boolean = false,
    delay: Int = 0,
  ): T = {
    val difftest: T = Wire(gen)
    val isExcluded = nameExcludes.exists(ex => gen.desiredModuleName.contains(ex))
    if (enabled && !isExcluded) {
      Gateway(gen, delay) := difftest
      interfaces.append((gen, delay))
    }
    if (dontCare) {
      difftest := DontCare
      difftest.bits.getValidOption.foreach(_ := false.B)
    }
    difftest
  }

  def get_current_interfaces(): Seq[(DifftestBundle, Int)] = interfaces.toSeq

  def get_command_configs(): Seq[String] = cmdConfigs.toSeq

  def collect(cpu: String): GatewayResult = {
    val gateway = Gateway.collect()
    generateCppHeader(
      cpu,
      gateway.instances,
      gateway.cppMacros,
      gateway.structPacked.getOrElse(false),
      gateway.structAligned.getOrElse(false),
    )
    if (gateway.cppExtModule.getOrElse(false)) {
      generateCppExtModules()
    }
    if (gateway.refClock.isDefined) {
      generateClockGate()
    }
    generateVerilogHeader(cpu, gateway.vMacros)
    Profile.generateJson(cpu, cmdConfigs.toSeq, interfaces.toSeq)
    gateway
  }

  def top[T <: RawModule with HasDiffTestInterfaces](cpuGen: => T, modPrefix: Option[String]): SimTop[T] =
    new SimTop(cpuGen, modPrefix)

  def top[T <: RawModule with HasDiffTestInterfaces](cpuGen: => T): SimTop[T] = top[T](cpuGen, None)

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
    FileControl.write(difftestSvh, "gateway_interface.svh")
  }

  def generateCppHeader(
    cpu: String,
    instances: Seq[DifftestBundle],
    macros: Seq[String],
    structPacked: Boolean,
    structAligned: Boolean,
  ): Unit = {
    val difftestCpp = ListBuffer.empty[String]
    difftestCpp += "#ifndef __DIFFTEST_STATE_H__"
    difftestCpp += "#define __DIFFTEST_STATE_H__"
    difftestCpp += ""
    difftestCpp += "#include <cstdint>"
    difftestCpp += ""

    macros.foreach(m => difftestCpp += s"#define $m")
    difftestCpp += ""

    val cpu_s = cpu.replace("-", "_").replace(" ", "").toUpperCase
    difftestCpp += s"#define CPU_$cpu_s"
    difftestCpp += ""

    val numCores = instances.count(_.isUniqueIdentifier)
    if (instances.nonEmpty) {
      difftestCpp +=
        s"""
           |#ifndef NUM_CORES
           |#define NUM_CORES $numCores
           |#endif
           |""".stripMargin
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
          require(bundles.length % numCores == 0, s"Cores seem to have different # of $macroName")
          difftestCpp += s"#define $configWidthName ${bundles.length / numCores}"
        }
        if (bundleType.isFlatten) {
          val configWidthName = s"CONFIG_DIFF_${macroName}_WIDTH"
          difftestCpp += s"#define $configWidthName ${bundleType.bits.getNumElements}"
        }
        difftestCpp += bundleType.toCppDeclaration(structPacked, structAligned)
        difftestCpp += ""
      })

    // create top-level difftest struct
    def toCppField(className: String, cppInstances: Seq[DifftestBundle]): String = {
      val bundleType = cppInstances.head
      val instanceName = bundleType.desiredCppName
      val cppIsArray = bundleType.isInstanceOf[DifftestWithIndex] || bundleType.isFlatten
      val nInstances = cppInstances.length
      val instanceCount = if (bundleType.isFlatten) bundleType.bits.getNumElements else nInstances / numCores
      require(nInstances % numCores == 0, s"Cores seem to have different # of $instanceName")
      require(cppIsArray || nInstances == numCores, s"# of $instanceName should not be $nInstances")
      val arrayWidth = if (cppIsArray) s"[$instanceCount]" else ""
      f"$className%-30s $instanceName$arrayWidth"
    }
    val (regStateBundles, eventBundles) = uniqBundles.toSeq.partition(_._2.head.desiredRegOffset.isDefined)
    // create DiffTestRegState
    difftestCpp += "typedef struct {"
    for ((className, cppInstances) <- regStateBundles.sortBy(_._2.head.desiredRegOffset.get)) {
      difftestCpp += f"  ${toCppField(className, cppInstances)};"
    }
    difftestCpp += "} DiffTestRegState;"
    difftestCpp += ""
    // create DiffTestState (RegState + others)
    difftestCpp += "typedef struct {"
    difftestCpp += "  DiffTestRegState regs;"
    for ((className, cppInstances) <- eventBundles.sortBy(_._2.head.desiredModuleName)) {
      difftestCpp += f"  ${toCppField(className, cppInstances)};"
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
    difftestCpp +=
      s"""
         |#ifdef CONFIG_DIFFTEST_QUERY
         |void difftest_query_init();
         |void difftest_query_step();
         |void difftest_query_finish();
         |#endif // CONFIG_DIFFTEST_QUERY
         |""".stripMargin
    difftestCpp += "#endif // __DIFFTEST_STATE_H__"
    difftestCpp += ""
    FileControl.write(difftestCpp, "difftest-state.h")
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
    FileControl.write(difftestCppExts, "difftest-extmodule.cpp")
  }
  def generateVerilogHeader(cpu: String, macros: Seq[String]): Unit = {
    val difftestV = ListBuffer.empty[String]
    macros.foreach(m => difftestV += s"`define $m")
    val cpu_s = cpu.replace("-", "_").replace(" ", "").toUpperCase
    difftestV += s"`define CPU_$cpu_s"
    FileControl.write(difftestV, "DifftestMacros.svh")
  }

  def generateClockGate(): Unit = {
    val difftest = ListBuffer.empty[String]
    difftest +=
      s"""
         |module DifftestClockGate(
         |	input     CK,
         |	input	    E,
         |	output    Q
         |);
         |
         |`ifdef SYNTHESIS
         |	BUFGCE bufgce_1 (
         |		.O(Q),
         |		.I(CK),
         |		.CE(E)
         |	);
         |`else
         |  assign Q = CK & E;
         |`endif // SYNTHESIS
         |endmodule
         |""".stripMargin
    FileControl.write(difftest, "DifftestClockGate.sv")
  }
}
