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
import difftest.replay.Replay
import difftest.trace.Trace
import difftest.util.VerificationExtractor
import difftest.validate.Validate

import scala.collection.mutable.ListBuffer

case class GatewayConfig(
  style: String = "dpic",
  hasGlobalEnable: Boolean = false,
  isSquash: Boolean = false,
  hasReplay: Boolean = false,
  replaySize: Int = 1024,
  hasDutZone: Boolean = false,
  isBatch: Boolean = false,
  batchSize: Int = 64,
  hasInternalStep: Boolean = false,
  isNonBlock: Boolean = false,
  hasBuiltInPerf: Boolean = false,
  traceDump: Boolean = false,
  traceLoad: Boolean = false,
  hierarchicalWiring: Boolean = false,
  exitOnAssertions: Boolean = false,
  isFPGA: Boolean = false,
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
  def hasDeferredResult: Boolean = isNonBlock || hasInternalStep
  def needTraceInfo: Boolean = hasReplay
  def needEndpoint: Boolean =
    hasGlobalEnable || hasDutZone || isBatch || isSquash || hierarchicalWiring || traceDump || traceLoad
  def needPreprocess: Boolean = hasDutZone || isBatch || isSquash || needTraceInfo
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
    if (hasReplay) macros ++= Seq("CONFIG_DIFFTEST_REPLAY", s"CONFIG_DIFFTEST_REPLAY_SIZE ${replaySize}")
    if (hasDeferredResult) macros += "CONFIG_DIFFTEST_DEFERRED_RESULT"
    if (hasInternalStep) macros += "CONFIG_DIFFTEST_INTERNAL_STEP"
    if (traceDump || traceLoad) macros += "CONFIG_DIFFTEST_IOTRACE"
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
    macros.toSeq
  }
  def check(): Unit = {
    if (hasReplay) require(isSquash)
    if (hasInternalStep) require(isBatch)
    if (isBatch) require(!hasDutZone)
    // TODO: support dump and load together
    require(!(traceDump && traceLoad))
  }
}

case class GatewayResult(
  cppMacros: Seq[String] = Seq(),
  vMacros: Seq[String] = Seq(),
  instances: Seq[DifftestBundle] = Seq(),
  structPacked: Option[Boolean] = None,
  exit: Option[UInt] = None,
  step: Option[UInt] = None,
) {
  def +(that: GatewayResult): GatewayResult = {
    GatewayResult(
      cppMacros = cppMacros ++ that.cppMacros,
      vMacros = vMacros ++ that.vMacros,
      instances = instances ++ that.instances,
      structPacked = if (structPacked.isDefined) structPacked else that.structPacked,
      exit = if (exit.isDefined) exit else that.exit,
      step = if (step.isDefined) step else that.step,
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
      case 'B' => config = config.copy(isBatch = true)
      case 'I' => config = config.copy(hasInternalStep = true)
      case 'N' => config = config.copy(isNonBlock = true)
      case 'P' => config = config.copy(hasBuiltInPerf = true)
      case 'T' => config = config.copy(traceDump = true)
      case 'L' => config = config.copy(traceLoad = true)
      case 'H' => config = config.copy(hierarchicalWiring = true)
      case 'X' => config = config.copy(exitOnAssertions = true)
      case 'F' => config = config.copy(isFPGA = true)
      case x   => println(s"Unknown Gateway Config $x")
    }
    config.check()
  }

  def apply[T <: DifftestBundle](gen: T, delay: Int): T = {
    val bundle = WireInit(0.U.asTypeOf(gen))
    if (!config.traceLoad) {
      if (config.needEndpoint) {
        val packed = WireInit(bundle.asUInt)
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

  def collect(): GatewayResult = {
    val exit = Option.when(config.exitOnAssertions) {
      val asserted = RegInit(false.B)
      VerificationExtractor.sink(asserted)
      // Holds 1 after any assertion is asserted.
      RegEnable(1.U(64.W), 0.U(64.W), asserted)
    }
    val instances = instanceWithDelay.map(_._1).toSeq
    val sink = if (config.needEndpoint) {
      val gatewayIn = if (config.traceLoad) {
        MixedVecInit(Trace.load(instances).toSeq.map(_.asUInt))
      } else {
        val packed = WireInit(0.U.asTypeOf(MixedVec(instances.map(gen => UInt(gen.getWidth.W)))))
        for ((data, idx) <- packed.zipWithIndex) {
          DifftestWiring.addSink(data, s"gateway_$idx", config.hierarchicalWiring)
        }
        packed
      }
      val endpoint = Module(new GatewayEndpoint(instanceWithDelay.toSeq, config))
      endpoint.in := gatewayIn
      GatewayResult(
        instances = endpoint.instances,
        structPacked = Some(config.isBatch),
        step = Some(endpoint.step),
      )
    } else {
      GatewayResult(instances = instances) + GatewaySink.collect(config)
    }
    sink + GatewayResult(
      cppMacros = config.cppMacros,
      vMacros = config.vMacros,
      exit = exit,
    )
  }
}

class GatewayEndpoint(instanceWithDelay: Seq[(DifftestBundle, Int)], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(instanceWithDelay.map { case (gen, _) => UInt(gen.getWidth.W) })))

  val bundle = if (config.traceLoad) {
    MixedVecInit(in.zip(instanceWithDelay).map { case (i, (gen, _)) => i.asTypeOf(gen) }.toSeq)
  } else {
    val delayed = MixedVecInit(
      in.zip(instanceWithDelay).map { case (i, (gen, d)) => Delayer(i.asTypeOf(gen), d) }.toSeq
    )
    if (config.traceDump) Trace(delayed)
    delayed
  }

  val preprocessed = if (config.needPreprocess) {
    WireInit(Preprocess(bundle))
  } else {
    WireInit(bundle)
  }

  val replayed = if (config.hasReplay) {
    WireInit(Replay(preprocessed, config))
  } else {
    WireInit(preprocessed)
  }

  val validated = Validate(replayed, config)

  val squashed = if (config.isSquash) {
    WireInit(Squash(validated, config))
  } else {
    WireInit(validated)
  }
  val instances = chiselTypeOf(squashed).map(_.bits).toSeq

  val zoneControl = Option.when(config.hasDutZone)(Module(new ZoneControl(config)))
  val step = IO(Output(UInt(config.stepWidth.W)))
  val control = Wire(new GatewaySinkControl(config))

  if (config.isBatch) {
    val batch = Batch(squashed, config)
    step := RegNext(batch.step, 0.U) // expose Batch step to check timeout
    control.enable := batch.enable
    GatewaySink.batch(Batch.getTemplate, control, batch.io, config)
  } else {
    val squashed_enable = VecInit(squashed.map(_.valid).toSeq).asUInt.orR
    step := RegNext(squashed_enable, 0.U)
    control.enable := squashed_enable
    if (config.hasDutZone) {
      zoneControl.get.enable := squashed_enable
      control.dut_zone.get := zoneControl.get.dut_zone
    }

    for (id <- 0 until squashed.length) {
      GatewaySink(control, squashed(id), config)
    }
  }

  GatewaySink.collect(config)

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
    val out = Option.when(config.isFPGA) {
      IO(new Bundle {
        val data = Output(UInt(config.batchBitWidth.W))
        val enable = Output(Bool())
      })
    }
    if (config.isFPGA) {
      out.get.data := Cat(io.data, io.info)
      out.get.enable := control.enable
      dontTouch(out.get)
    }
  }

  def collect(config: GatewayConfig): GatewayResult = {
    config.style match {
      case "dpic" => DPIC.collect()
      case _      => DPIC.collect() // Default: DPI-C
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
