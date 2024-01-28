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
      val bundle = WireInit(gen)
      val sink = GatewaySink(gen, config)
      sink.io := bundle
      sink.enable := true.B
      bundle
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
        instances = endpoint.extraInstances.toSeq,
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
  val extraInstances = ListBuffer.empty[(DifftestBundle, String)]
  val in = WireInit(0.U.asTypeOf(MixedVec(signals.map(_.cloneType))))
  val in_pack = WireInit(0.U.asTypeOf(MixedVec(signals.map(gen => UInt(gen.getWidth.W)))))
  for ((data, id) <- in_pack.zipWithIndex) {
    DifftestWiring.addSink(data, s"gateway_$id")
    in(id) := data.asTypeOf(in(id).cloneType)
  }

  val preprocessed = if (config.needPreprocess) {
    val preprocess = Module(new Preprocess(in.toSeq.map(_.cloneType), config))
    extraInstances ++= preprocess.extraInstances
    preprocess.in := in
    WireInit(preprocess.out)
  } else {
    WireInit(in)
  }

  val squashed = if (config.isSquash) {
    val squash = Squash(preprocessed.toSeq.map(_.cloneType), config)
    squash.in := preprocessed
    WireInit(squash.out)
  } else {
    WireInit(preprocessed)
  }

  val zoneControl = Option.when(config.hasDutZone)(Module(new ZoneControl(config)))
  val step = IO(Output(UInt(config.stepWidth.W)))
  if (config.isBatch) {
    val batch = Batch(squashed.toSeq.map(_.cloneType), config)
    batch.in := squashed
    step := batch.step
    if (config.hasDutZone) {
      zoneControl.get.enable := batch.enable
    }

    val sink = GatewaySink.batch(squashed.toSeq.map(_.cloneType), config)
    sink.io := batch.out
    sink.info := batch.info
    sink.enable := batch.enable
    if (config.hasDutZone) {
      sink.dut_zone.get := zoneControl.get.dut_zone.get
    }
  } else {
    val squashed_enable = WireInit(true.B)
    if (config.hasGlobalEnable) {
      squashed_enable := VecInit(squashed.flatMap(_.bits.needUpdate).toSeq).asUInt.orR
    }
    step := Mux(squashed_enable, 1.U, 0.U)
    if (config.hasDutZone) {
      zoneControl.get.enable := squashed_enable
    }

    for(id <- 0 until squashed.length){
      val sink = GatewaySink(squashed(id).cloneType, config)
      sink.io := squashed(id)
      sink.enable := squashed_enable
      if (config.hasDutZone) {
        sink.dut_zone.get := zoneControl.get.dut_zone.get
      }
    }
  }

  GatewaySink.collect(config)

}


object GatewaySink{
  def apply[T <: DifftestBundle](gen: T, config: GatewayConfig): GatewaySink[T] = {
    config.style match {
      case "dpic" => DPIC(gen, config)
      case _ => DPIC(gen, config) // Default: DPI-C
    }
  }

  def batch[T <: Seq[DifftestBundle]](bundles: T, config: GatewayConfig): GatewayBatchSink[T] = {
    config.style match {
      case "dpic" => DPIC.batch(bundles, config)
      case _ => DPIC.batch(bundles, config) // Default: DPI-C
    }
  }

  def collect(config: GatewayConfig): Unit = {
    config.style match {
      case "dpic" => DPIC.collect()
      case _ => DPIC.collect() // Default: DPI-C
    }
  }
}

class GatewayBaseSink(config: GatewayConfig) extends Module {
  val enable = IO(Input(Bool()))
  val dut_zone = Option.when(config.hasDutZone)(IO(Input(UInt(config.dutZoneWidth.W))))
}

class GatewaySink[T <: DifftestBundle](gen: T, config: GatewayConfig) extends GatewayBaseSink(config) {
  val io = IO(Input(gen))
}

class GatewayBatchSink[T <: Seq[DifftestBundle]](bundles: T, config: GatewayConfig) extends GatewayBaseSink(config) {
  val io = IO(Input(Vec(config.batchSize, MixedVec(bundles))))
  val info = IO(Input(Vec(config.batchSize, UInt())))
}

class Preprocess(signals: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val extraInstances = ListBuffer.empty[(DifftestBundle, String)]
  val in = IO(Input(MixedVec(signals)))
  val out = if (config.needTraceInfo) {
    val traceInfo = new DiffTraceInfo(config)
    val signalsWithInfo = signals ++ Seq(traceInfo)
    extraInstances += ((traceInfo, config.style))
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
    traceinfo.squash_idx.get := 0.U //default value, set in Squash
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
  val dut_zone = Option.when(config.hasDutZone)(IO(Output(UInt(config.dutZoneWidth.W))))

  if (config.hasDutZone) {
    val zone = RegInit(0.U(config.dutZoneWidth.W))
    when(enable) {
      zone := zone + 1.U
      when(zone === (config.dutZoneSize - 1).U) {
        zone := 0.U
      }
    }
    dut_zone.get := zone
  }
}
