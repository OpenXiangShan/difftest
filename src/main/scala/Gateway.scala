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
import difftest.dpic.DPIC
import difftest.squash.Squash
import difftest.batch.Batch

import scala.collection.mutable.ListBuffer

case class GatewayConfig(
  style: String = "dpic",
  hasGlobalEnable: Boolean = false,
  isSquash: Boolean = false,
  squashReplay: Boolean = false,
  replaySize: Int = 256,
  diffStateSelect: Boolean = false,
  isBatch: Boolean = false,
  batchSize: Int = 32,
  isNonBlock: Boolean = false
) {
  if (squashReplay) require(isSquash)
  def hasDutZone: Boolean = diffStateSelect
  def dutZoneSize: Int = if (hasDutZone) 2 else 1
  def dutZoneWidth: Int = log2Ceil(dutZoneSize)
  def dutBufLen: Int = if (isBatch) batchSize else 1
  def maxStep: Int = if (isBatch) batchSize else 1
  def stepWidth: Int = log2Ceil(maxStep + 1)
  def hasDeferredResult: Boolean = isNonBlock
  def needTraceInfo: Boolean = squashReplay
  def needEndpoint: Boolean = hasGlobalEnable || diffStateSelect || isBatch || isSquash
  def needPreprocess: Boolean = hasDutZone || isBatch || isSquash || needTraceInfo
  // Macros Generation for Cpp and Verilog
  def cppMacros: Seq[String] = {
    val macros = ListBuffer.empty[String]
    macros += s"CONFIG_DIFFTEST_${style.toUpperCase}"
    macros += s"CONFIG_DIFFTEST_ZONESIZE $dutZoneSize"
    macros += s"CONFIG_DIFFTEST_BUFLEN $dutBufLen"
    if (isBatch) macros ++= Seq("CONFIG_DIFFTEST_BATCH", s"DIFFTEST_BATCH_SIZE ${batchSize}")
    if (isSquash) macros += "CONFIG_DIFFTEST_SQUASH"
    if (squashReplay) macros += "CONFIG_DIFFTEST_SQUASH_REPLAY"
    if (hasDeferredResult) macros += "CONFIG_DIFFTEST_DEFERRED_RESULT"
    macros.toSeq
  }
  def vMacros: Seq[String] = {
    val macros = ListBuffer.empty[String]
    macros += s"CONFIG_DIFFTEST_STEPWIDTH ${stepWidth}"
    if (isNonBlock) macros += "CONFIG_DIFFTEST_NONBLOCK"
    if (hasDeferredResult) macros += "CONFIG_DIFFTEST_DEFERRED_RESULT"
    macros.toSeq
  }
}

case class GatewayResult(
  cppMacros: Seq[String] = Seq(),
  vMacros: Seq[String] = Seq(),
  instances: Seq[(DifftestBundle, String)] = Seq(),
  step: Option[UInt] = None
)

object Gateway {
  private val instances = ListBuffer.empty[DifftestBundle]
  private var config = GatewayConfig()

  def apply[T <: DifftestBundle](gen: T, style: String): T = {
    config = GatewayConfig(style = style)
    if (config.needEndpoint) {
      register(WireInit(0.U.asTypeOf(gen)))
    } else {
      val signal = WireInit(0.U.asTypeOf(gen))
      val bundle = Wire(new GatewayBundle(gen, config))
      bundle.enable := true.B
      bundle.data := signal
      GatewaySink(bundle, config)
      signal
    }
  }

  def register[T <: DifftestBundle](gen: T): T = {
    val gen_pack = WireInit(gen.asUInt)
    DifftestWiring.addSource(gen_pack, s"gateway_${instances.length}")
    instances += gen
    gen
  }

  def collect(): GatewayResult = {
    if (config.needEndpoint) {
      val endpoint = Module(new GatewayEndpoint(instances.toSeq, config))
      GatewayResult(
        cppMacros = config.cppMacros,
        vMacros = config.vMacros,
        instances = endpoint.instances,
        step = Some(endpoint.step)
      )
    } else {
      GatewaySink.collect(config)
      GatewayResult(
        cppMacros = config.cppMacros,
        vMacros = config.vMacros,
        step = Some(1.U)
      )
    }
  }
}

class GatewayEndpoint(signals: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val instances = if (config.needTraceInfo) Seq((new DiffTraceInfo(config), config.style)) else Seq()
  val in = WireInit(0.U.asTypeOf(MixedVec(signals.map(_.cloneType))))
  val in_pack = WireInit(0.U.asTypeOf(MixedVec(signals.map(gen => UInt(gen.getWidth.W)))))
  for ((data, id) <- in_pack.zipWithIndex) {
    DifftestWiring.addSink(data, s"gateway_$id")
    in(id) := data.asTypeOf(in(id).cloneType)
  }

  val preprocessed = if (config.needPreprocess) {
    WireInit(Preprocess(in, config))
  } else {
    WireInit(in)
  }

  val squashed = if (config.isSquash) {
    WireInit(Squash(preprocessed, config))
  } else {
    WireInit(preprocessed)
  }

  val zoneControl = Option.when(config.hasDutZone)(Module(new ZoneControl(config)))
  val step = IO(Output(UInt(config.stepWidth.W)))
  if (config.isBatch) {
    val batch = Batch(squashed, config)
    step := batch.step
    if (config.hasDutZone) zoneControl.get.enable := batch.enable

    val bundle = Wire(new GatewayBatchBundle(squashed.toSeq.map(_.cloneType), config))
    bundle.enable := batch.enable
    if (config.hasDutZone) bundle.dut_zone.get := zoneControl.get.dut_zone
    bundle.data := batch.data
    bundle.info := batch.info

    GatewaySink.batch(squashed, bundle, config)
  } else {
    val squashed_enable = WireInit(true.B)
    if (config.hasGlobalEnable) {
      squashed_enable := VecInit(squashed.flatMap(_.bits.needUpdate).toSeq).asUInt.orR
    }
    step := Mux(squashed_enable, 1.U, 0.U)
    if (config.hasDutZone) zoneControl.get.enable := squashed_enable

    for (id <- 0 until squashed.length) {
      val bundle = Wire(new GatewayBundle(squashed(id).cloneType, config))
      bundle.enable := squashed_enable
      if (config.hasDutZone) bundle.dut_zone.get := zoneControl.get.dut_zone
      bundle.data := squashed(id)

      GatewaySink(bundle, config)
    }
  }

  GatewaySink.collect(config)

}

object GatewaySink {
  def apply(bundle: GatewayBundle, config: GatewayConfig): Unit = {
    config.style match {
      case "dpic" => DPIC(bundle, config)
      case _      => DPIC(bundle, config) // Default: DPI-C
    }
  }

  def batch(template: MixedVec[DifftestBundle], bundle: GatewayBatchBundle, config: GatewayConfig): Unit = {
    config.style match {
      case "dpic" => DPIC.batch(template, bundle, config)
      case _      => DPIC.batch(template, bundle, config) // Default: DPI-C
    }
  }

  def collect(config: GatewayConfig): Unit = {
    config.style match {
      case "dpic" => DPIC.collect()
      case _      => DPIC.collect() // Default: DPI-C
    }
  }
}

class GatewayBaseBundle(config: GatewayConfig) extends Bundle {
  val enable = Bool()
  val dut_zone = Option.when(config.hasDutZone)(UInt(config.dutZoneWidth.W))
}

class GatewayBundle(gen: DifftestBundle, config: GatewayConfig) extends GatewayBaseBundle(config) {
  val data = gen
}

class GatewayBatchBundle(bundles: Seq[DifftestBundle], config: GatewayConfig) extends GatewayBaseBundle(config) {
  val data = Vec(config.batchSize, MixedVec(bundles))
  val info = Vec(config.batchSize, UInt(log2Ceil(config.batchSize).W))
}

object Preprocess {
  def apply(bundles: MixedVec[DifftestBundle], config: GatewayConfig): MixedVec[DifftestBundle] = {
    val module = Module(new Preprocess(bundles.toSeq.map(_.cloneType), config))
    module.in := bundles
    module.out
  }
}

class Preprocess(signals: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(signals)))
  val out = if (config.needTraceInfo) {
    val traceInfo = new DiffTraceInfo(config)
    val signalsWithInfo = signals ++ Seq(traceInfo)
    IO(Output(MixedVec(signalsWithInfo)))
  } else {
    IO(Output(MixedVec(signals)))
  }

  if (config.needTraceInfo) {
    for ((data, id) <- in.zipWithIndex) {
      out(id) := data
    }
    val traceinfo = out.filter(_.desiredCppName == "trace_info").head.asInstanceOf[DiffTraceInfo]
    traceinfo.coreid := out.filter(_.isUniqueIdentifier).head.coreid
    traceinfo.squash_idx.get := 0.U // default value, set in Squash
  } else {
    out := in
  }

  if (config.hasDutZone || config.isSquash || config.isBatch) {
    // Special fix for int writeback. Work for single-core only
    if (in.exists(_.desiredCppName == "wb_int")) {
      require(in.count(_.isUniqueIdentifier) == 1, "only single-core is supported yet")
      val writebacks = in.filter(_.desiredCppName == "wb_int").map(_.asInstanceOf[DiffIntWriteback])
      val numPhyRegs = writebacks.head.numElements
      val wb_int = Reg(Vec(numPhyRegs, UInt(64.W)))
      for (wb <- writebacks) {
        when(wb.valid) {
          wb_int(wb.address) := wb.data
        }
      }

      val commits = in.filter(_.desiredCppName == "commit").map(_.asInstanceOf[DiffInstrCommit])
      val num_skip = PopCount(commits.map(c => c.valid && c.skip))
      assert(num_skip <= 1.U, p"num_skip $num_skip is larger than one. Squash not supported yet")
      val wb_for_skip = out.filter(_.desiredCppName == "wb_int").head.asInstanceOf[DiffIntWriteback]
      for (c <- commits) {
        when(c.valid && c.skip) {
          wb_for_skip.valid := true.B
          wb_for_skip.address := c.wpdest
          wb_for_skip.data := wb_int(c.wpdest)
          for (wb <- writebacks) {
            when(wb.valid && wb.address === c.wpdest) {
              wb_for_skip.data := wb.data
            }
          }
        }
      }
    }
  }
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
