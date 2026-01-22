/***************************************************************************************
 * Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.gateway

import chisel3._
import chisel3.util._
import difftest._
import difftest.common.DifftestWiring
import difftest.util.Delayer
import difftest.dpic.DPIC
import difftest.preprocess.Preprocess
import difftest.squash.Squash
import difftest.batch.{Batch, BatchIO}
import difftest.delta.Delta
import difftest.replay.Replay
import difftest.trace.Trace
import difftest.validate.Validate

import scala.collection.mutable.ListBuffer

case class GatewayConfig(
  style: String = "dpic",
  hasGlobalEnable: Boolean = false,
  isSquash: Boolean = false,
  hasReplay: Boolean = false,
  replaySize: Int = 1024,
  hasDutZone: Boolean = false,
  isDelta: Boolean = false,
  isBatch: Boolean = false,
  batchSize: Int = 64,
  hasInternalStep: Boolean = false,
  isNonBlock: Boolean = false,
  hasBuiltInPerf: Boolean = false,
  traceDump: Boolean = false,
  traceLoad: Boolean = false,
  hierarchicalWiring: Boolean = false,
  softArchUpdate: Boolean = false,
  isFPGA: Boolean = false,
  isGSIM: Boolean = false,
) {
  def dutZoneSize: Int = if (hasDutZone) 2 else 1
  def dutZoneWidth: Int = log2Ceil(dutZoneSize)
  def dutBufLen: Int = if (isBatch) batchSize else 1
  def maxStep: Int = if (isBatch) batchSize else 1
  def stepWidth: Int = log2Ceil(maxStep + 1)
  def replayWidth: Int = log2Ceil(replaySize + 1)
  def batchArgByteLen: (Int, Int) = if (isFPGA) (1900, 100) else if (isNonBlock) (3600, 400) else (7200, 800)
  def batchBitWidth: Int = batchArgByteLen match { case (len1, len2) => (len1 + len2) * 8 }
  def batchSplit: Boolean = !isFPGA // Disable split for FPGA to reduce gates
  def deltaLimit: Int = 8
  def deltaQueueDepth: Int = 3
  def hasClockGate = isFPGA || isDelta
  def hasDeferredResult: Boolean = isNonBlock || hasInternalStep
  def needTraceInfo: Boolean = hasReplay
  def needEndpoint: Boolean =
    hasGlobalEnable || hasDutZone || isBatch || isSquash || hierarchicalWiring || traceDump || traceLoad || needPreprocess
  def needPreprocess: Boolean = hasDutZone || isBatch || isSquash || needTraceInfo || !softArchUpdate
  def useDPICtype: Boolean = !isFPGA && !isGSIM
  // Macros Generation for Cpp and Verilog
  def cppMacros: Seq[String] = {
    val macros = ListBuffer.empty[String]
    macros += s"CONFIG_DIFFTEST_${style.toUpperCase}"
    macros += s"CONFIG_DIFFTEST_ZONESIZE $dutZoneSize"
    macros += s"CONFIG_DIFFTEST_BUFLEN $dutBufLen"
    if (isBatch)
      macros ++= Seq(
        "CONFIG_DIFFTEST_BATCH",
        s"CONFIG_DIFFTEST_BATCH_SIZE ${batchSize}",
        s"CONFIG_DIFFTEST_BATCH_BYTELEN ${batchArgByteLen._1 + batchArgByteLen._2}",
      )
    if (isSquash) macros ++= Seq("CONFIG_DIFFTEST_SQUASH", s"CONFIG_DIFFTEST_SQUASH_STAMPSIZE 4096") // Stamp Width 12
    if (isDelta) macros += "CONFIG_DIFFTEST_DELTA"
    if (hasReplay) macros ++= Seq("CONFIG_DIFFTEST_REPLAY", s"CONFIG_DIFFTEST_REPLAY_SIZE ${replaySize}")
    if (hasDeferredResult) macros += "CONFIG_DIFFTEST_DEFERRED_RESULT"
    if (hasInternalStep) macros += "CONFIG_DIFFTEST_INTERNAL_STEP"
    if (traceDump || traceLoad) macros += "CONFIG_DIFFTEST_IOTRACE"
    if (isFPGA) macros += "CONFIG_DIFFTEST_FPGA"
    macros.toSeq
  }
  def vMacros: Seq[String] = {
    val macros = ListBuffer.empty[String]
    macros += s"CONFIG_DIFFTEST_STEPWIDTH ${stepWidth}"
    macros += s"CONFIG_DIFFTEST_BATCH_IO_WITDH ${batchBitWidth}"
    if (isNonBlock) macros += "CONFIG_DIFFTEST_NONBLOCK"
    if (hasDeferredResult) macros += "CONFIG_DIFFTEST_DEFERRED_RESULT"
    if (hasInternalStep) macros += "CONFIG_DIFFTEST_INTERNAL_STEP"
    if (traceDump || traceLoad) macros += "CONFIG_DIFFTEST_IOTRACE"
    if (isFPGA) macros += "CONFIG_DIFFTEST_FPGA"
    if (hasClockGate) macros += "CONFIG_DIFFTEST_CLOCKGATE"
    macros.toSeq
  }
  def check(): Unit = {
    if (hasReplay) require(isSquash)
    if (hasInternalStep) require(isBatch)
    if (isBatch) require(!hasDutZone)
    // Currently Delta depends on Batch to ensure update and sync order
    if (isDelta) require(isBatch)
    // Batch provides unified IO interface for FPGA Diff
    if (isFPGA) require(isBatch)
    // TODO: support dump and load together
    require(!(traceDump && traceLoad))
  }
}

class FpgaDiffIO(dataWidth: Int) extends DecoupledIO(UInt(dataWidth.W))

case class GatewayResult(
  cppMacros: Seq[String] = Seq(),
  vMacros: Seq[String] = Seq(),
  instances: Seq[DifftestBundle] = Seq(),
  structPacked: Option[Boolean] = None,
  structAligned: Option[Boolean] = None, // Align struct Elem to 8 bytes for Delta Feature
  cppExtModule: Option[Boolean] = None,
  refClock: Option[Clock] = None,
  exit: Option[UInt] = None,
  step: Option[UInt] = None,
  fpgaIO: Option[FpgaDiffIO] = None,
  clockEnable: Option[Bool] = None,
) {
  def +(that: GatewayResult): GatewayResult = {
    GatewayResult(
      cppMacros = cppMacros ++ that.cppMacros,
      vMacros = vMacros ++ that.vMacros,
      instances = instances ++ that.instances,
      structPacked = if (structPacked.isDefined) structPacked else that.structPacked,
      structAligned = if (structAligned.isDefined) structAligned else that.structAligned,
      cppExtModule = if (cppExtModule.isDefined) cppExtModule else that.cppExtModule,
      refClock = if (refClock.isDefined) refClock else that.refClock,
      exit = if (exit.isDefined) exit else that.exit,
      step = if (step.isDefined) step else that.step,
      fpgaIO = if (fpgaIO.isDefined) fpgaIO else that.fpgaIO,
      clockEnable = if (clockEnable.isDefined) clockEnable else that.clockEnable,
    )
  }
}

object Gateway {
  private val instanceWithDelay = ListBuffer.empty[(DifftestBundle, Int)]
  private var config = GatewayConfig()

  def setConfig(cfg: String): Unit = {
    cfg.foreach {
      case 'E' => config = config.copy(hasGlobalEnable = true)
      case 'S' => config = config.copy(isSquash = true)
      case 'R' => config = config.copy(hasReplay = true)
      case 'Z' => config = config.copy(hasDutZone = true)
      case 'D' => config = config.copy(isDelta = true)
      case 'B' => config = config.copy(isBatch = true)
      case 'I' => config = config.copy(hasInternalStep = true)
      case 'N' => config = config.copy(isNonBlock = true)
      case 'P' => config = config.copy(hasBuiltInPerf = true)
      case 'T' => config = config.copy(traceDump = true)
      case 'L' => config = config.copy(traceLoad = true)
      case 'H' => config = config.copy(hierarchicalWiring = true)
      case 'F' => config = config.copy(isFPGA = true)
      case 'G' => config = config.copy(isGSIM = true)
      case 'U' => config = config.copy(softArchUpdate = true)
      case x   => println(s"Unknown Gateway Config $x")
    }
    config.check()
  }

  def apply[T <: DifftestBundle](gen: T, delay: Int): T = {
    val bundle = WireInit(0.U.asTypeOf(gen)).suggestName(gen.desiredCppName)
    dontTouch(bundle)
    if (!config.traceLoad) {
      if (config.needEndpoint) {
        val packed = WireInit(UInt(bundle.getWidth.W), bundle.asUInt)
        DifftestWiring.addSource(packed, s"gateway_${instanceWithDelay.length}", config.hierarchicalWiring)
      } else {
        val control = WireInit(0.U.asTypeOf(new GatewaySinkControl(config)))
        control.enable := true.B
        GatewaySink(control, Delayer(bundle, delay).genValidBundle, config)
      }
    }
    instanceWithDelay += ((gen, delay))
    bundle
  }

  def getInstance(bundles: Seq[DifftestBundle]): Seq[DifftestBundle] = {
    val archRegs = if (!bundles.exists(_.desiredCppName == "xrf")) {
      Preprocess.getArchRegs(bundles, false)
    } else {
      Seq.empty
    }
    bundles ++ archRegs
  }

  def collect(): GatewayResult = {
    val instances = instanceWithDelay.map(_._1).toSeq
    val sink = if (config.needEndpoint) {
      val gatewayIn = if (config.traceLoad) {
        MixedVecInit(Trace.load(instances).toSeq.map(_.asUInt)).asUInt
      } else {
        val packed = WireInit(0.U.asTypeOf(MixedVec(instances.map(gen => UInt(gen.getWidth.W)))))
        for ((data, idx) <- packed.zipWithIndex) {
          DifftestWiring.addSink(data, s"gateway_$idx", config.hierarchicalWiring)
        }
        packed.asUInt
      }
      val endpoint = Module(new GatewayEndpoint(instanceWithDelay.toSeq, config))
      endpoint.in := gatewayIn
      val numCores = instances.count(_.isUniqueIdentifier)
      GatewayResult(
        vMacros = Seq(s"CONFIG_DIFFTEST_INTERFACE_WIDTH ${gatewayIn.getWidth / numCores}"),
        instances = endpoint.instances,
        structPacked = Some(config.isBatch),
        structAligned = Some(config.isDelta),
        refClock = Option.when(config.hasClockGate)(endpoint.clock),
        step = Some(endpoint.step),
        fpgaIO = endpoint.fpgaIO,
        clockEnable = endpoint.clockEnable,
      )
    } else {
      GatewayResult(instances = getInstance(instances)) + GatewaySink.collect(config, getInstance(instances))
    }
    sink + GatewayResult(
      cppMacros = config.cppMacros,
      vMacros = config.vMacros,
      cppExtModule = Some(config.isGSIM),
      exit = None,
    )
  }
}

class GatewayEndpoint(instanceWithDelay: Seq[(DifftestBundle, Int)], config: GatewayConfig) extends Module {
  val in = IO(Input(UInt(instanceWithDelay.map(_._1.getWidth).sum.W)))
  val in_bundle = in.asTypeOf(MixedVec(instanceWithDelay.map(_._1)))
  val decoupledIn = Wire(Decoupled(chiselTypeOf(in_bundle)))
  val clockEnable = Option.when(config.hasClockGate)(IO(Output(Bool())))
  // clockEnable should hold one more cycle than ready to sample signals when fire
  clockEnable.foreach { ce =>
    val ready = decoupledIn.ready
    ce := (ready && RegNext(ready)) || reset.asBool
  }
  decoupledIn.valid := !reset.asBool

  if (config.traceLoad) {
    decoupledIn.bits := in_bundle
  } else {
    val delayed = MixedVecInit(
      in_bundle.zip(instanceWithDelay.map(_._2)).map { case (i, d) => Delayer(i, d, decoupledIn.ready) }.toSeq
    )
    if (config.traceDump) Trace(delayed)
    decoupledIn.bits := delayed
  }

  if (!config.hasClockGate) {
    assert(decoupledIn.ready)
  }

  val preprocessed = if (config.needPreprocess) {
    Preprocess(decoupledIn, config)
  } else {
    decoupledIn
  }

  val replayed = if (config.hasReplay) {
    Replay(preprocessed, config)
  } else {
    preprocessed
  }

  val validated = Validate(replayed, config)

  val squashed = if (config.isSquash) {
    Squash(validated, config)
  } else {
    validated
  }
  val instances = Gateway.getInstance(chiselTypeOf(squashed.bits).map(_.bits).toSeq)
  val deltas = if (config.isDelta) {
    Delta(squashed, config)
  } else {
    squashed
  }
  val toSink = deltas

  val zoneControl = Option.when(config.hasDutZone)(Module(new ZoneControl(config)))
  val step = IO(Output(UInt(config.stepWidth.W)))
  val control = Wire(new GatewaySinkControl(config))

  val fpgaIO = Option.when(config.isBatch && config.isFPGA)(IO(new FpgaDiffIO(config.batchBitWidth)))

  if (config.isBatch) {
    val batch = Batch(toSink, config)
    step := RegNext(batch.bits.step, 0.U) // expose Batch step to check timeout
    control.enable := batch.valid
    GatewaySink.batch(Batch.getTemplate, control, batch.bits.io, config)
    if (config.isFPGA) {
      fpgaIO.get.bits := batch.bits.io.asUInt
      fpgaIO.get.valid := batch.valid
      batch.ready := fpgaIO.get.ready
    } else {
      batch.ready := true.B
    }
  } else {
    toSink.ready := true.B
    val sink_enable = VecInit(toSink.bits.map(_.valid).toSeq).asUInt.orR
    step := RegNext(sink_enable, 0.U)
    control.enable := sink_enable
    if (config.hasDutZone) {
      zoneControl.get.enable := sink_enable
      control.dut_zone.get := zoneControl.get.dut_zone
    }

    for (id <- 0 until toSink.bits.length) {
      GatewaySink(control, toSink.bits(id), config)
    }
  }

  GatewaySink.collect(config, instances)

}

object GatewaySink {
  def apply(control: GatewaySinkControl, io: Valid[DifftestBundle], config: GatewayConfig): Unit = {
    config.style match {
      case "dpic" => DPIC(control, io, config)
      case _      => DPIC(control, io, config) // Default: DPI-C
    }
  }

  def batch(template: Seq[DifftestBundle], control: GatewaySinkControl, io: BatchIO, config: GatewayConfig): Unit = {
    config.style match {
      case "dpic" => DPIC.batch(template, control, io, config)
      case _      => DPIC.batch(template, control, io, config) // Default: DPI-C
    }
  }

  def collect(config: GatewayConfig, instances: Seq[DifftestBundle]): GatewayResult = {
    config.style match {
      case "dpic" => DPIC.collect(config, instances)
      case _      => DPIC.collect(config, instances) // Default: DPI-C
    }
  }
}

class GatewaySinkControl(config: GatewayConfig) extends Bundle {
  val enable = Bool()
  val dut_zone = Option.when(config.hasDutZone)(UInt(config.dutZoneWidth.W))
}

class ZoneControl(config: GatewayConfig) extends Module {
  val enable = IO(Input(Bool()))
  val dut_zone = IO(Output(UInt(config.dutZoneWidth.W)))

  if (config.hasDutZone) {
    val zone = RegInit(0.U(config.dutZoneWidth.W))
    when(enable) {
      zone := zone + 1.U
      when(zone === (config.dutZoneSize - 1).U) {
        zone := 0.U
      }
    }
    dut_zone := zone
  }
}
