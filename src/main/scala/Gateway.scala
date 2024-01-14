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

import scala.collection.mutable.ListBuffer

case class GatewayConfig(
                        style          : String  = "dpic",
                        hasGlobalEnable: Boolean = false,
                        isSquash       : Boolean = false,
                        squashReplay   : Boolean = false,
                        replaySize     : Int     = 256,
                        delayedStep    : Boolean = false,
                        isBatch        : Boolean = false,
                        batchSize      : Int     = 32
                        )
{
  if (squashReplay) require(isSquash)
  def maxStep: Int = if (isBatch) batchSize else 1
  def stepWidth: Int = log2Ceil(maxStep + 1)
  def hasDutPos: Boolean = delayedStep || isBatch
  def dutBufLen: Int = if (delayedStep) maxStep + 1 else maxStep
  def dutPosWidth: Int = log2Ceil(dutBufLen)
  def needTraceInfo: Boolean = squashReplay
  def needEndpoint: Boolean = hasGlobalEnable || delayedStep || isBatch || isSquash
}

object Gateway {
  private val instances = ListBuffer.empty[DifftestBundle]
  private var config    = GatewayConfig()

  def apply[T <: DifftestBundle](gen: T, style: String): T = {
    config = GatewayConfig(style = style)
    if (config.needEndpoint)
      register(WireInit(0.U.asTypeOf(gen)))
    else {
      val port = Wire(new GatewayBundle(config))
      port.enable := true.B
      val bundle = WireInit(0.U.asTypeOf(gen))
      GatewaySink(gen, config, port) := bundle.asUInt
      bundle
    }
  }

  def register[T <: DifftestBundle](gen: T): T = {
    val gen_pack = WireInit(gen.asUInt)
    DifftestWiring.addSource(gen_pack, s"gateway_${instances.length}")
    instances += gen
    gen
  }

  def collect(): (Seq[String], Seq[(DifftestBundle, String)], UInt) = {
    val macros = ListBuffer.empty[String]
    val extraInstances = ListBuffer.empty[(DifftestBundle, String)]
    val step = WireInit(1.U)
    if (config.needEndpoint) {
      val endpoint = Module(new GatewayEndpoint(instances.toSeq, config))
      macros ++= endpoint.macros
      extraInstances ++= endpoint.extraInstances
      step := endpoint.step
    }
    else {
      macros ++= GatewaySink.collect(config)
    }
    (macros.toSeq, extraInstances.toSeq, step)
  }
}

class GatewayEndpoint(signals: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = WireInit(0.U.asTypeOf(MixedVec(signals.map(_.cloneType))))
  val in_pack = WireInit(0.U.asTypeOf(MixedVec(signals.map(gen => UInt(gen.getWidth.W)))))
  for ((data, id) <- in_pack.zipWithIndex) {
    DifftestWiring.addSink(data, s"gateway_$id")
    in(id) := data.asTypeOf(in(id).cloneType)
  }

  val share_wbint = WireInit(in)
  if (config.hasDutPos || config.isSquash) {
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
      val wb_for_skip = share_wbint.filter(_.desiredCppName == "wb_int").head.asInstanceOf[DiffIntWriteback]
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

  val squashed = WireInit(share_wbint)
  val squash_idx = Option.when(config.squashReplay)(WireInit(0.U(log2Ceil(config.replaySize).W)))
  if (config.isSquash) {
    val squash = Squash(share_wbint.toSeq.map(_.cloneType), config)
    squash.in := share_wbint
    squashed := squash.out
    if (config.squashReplay) {
      squash_idx.get := squash.idx.get
    }
  }

  val signalsWithInfo = ListBuffer.empty[DifftestBundle]
  signalsWithInfo ++= signals
  if (config.needTraceInfo) signalsWithInfo += new DiffTraceInfo(config)
  val out = WireInit(0.U.asTypeOf(MixedVec(signalsWithInfo.toSeq.map(_.cloneType))))
  if (config.needTraceInfo) {
    for ((data, id) <- squashed.zipWithIndex) {
      out(id) := data
    }
    val traceinfo = out.filter(_.desiredCppName == "trace_info").head.asInstanceOf[DiffTraceInfo]
    traceinfo.coreid := out.filter(_.isUniqueIdentifier).head.coreid
    traceinfo.squash_idx.get := squash_idx.get
  }
  else {
    out := squashed
  }

  val out_pack = WireInit(0.U.asTypeOf(MixedVec(signalsWithInfo.toSeq.map(gen => UInt(gen.getWidth.W)))))
  for ((data, id) <- out_pack.zipWithIndex) {
    data := out(id).asUInt
  }

  val port = Wire(new GatewayBundle(config))

  val global_enable = WireInit(true.B)
  if(config.hasGlobalEnable) {
    global_enable := VecInit(out.filter(_.needUpdate.isDefined).map(_.needUpdate.get).toSeq).asUInt.orR
  }
  port.enable := global_enable

  if(config.hasDutPos) {
    val pos = RegInit(0.U(config.dutPosWidth.W))
    when (global_enable) {
      pos := pos + 1.U
      when (pos === (config.dutBufLen - 1).U) {
        pos := 0.U
      }
    }
    port.dut_pos.get := pos
  }

  val step = IO(Output(UInt(config.stepWidth.W)))
  val step_org = WireInit(0.U(config.stepWidth.W))
  val step_cnter = RegInit(0.U(config.stepWidth.W))
  val step_cond = WireInit(true.B)
  // Currently we set step when cnter reach maxStep
  step_cond := step_cnter === (config.maxStep - 1).U && global_enable
  when (step_cond) {
    step_cnter := 0.U
  }.elsewhen (global_enable) {
    step_cnter := step_cnter + 1.U
  }
  step_org := Mux(step_cond, config.maxStep.U, 0.U)
  if (config.delayedStep) {
    step := RegNext(step_org)
  }
  else {
    step := step_org
  }

  for(id <- 0 until out.length){
    GatewaySink(out(id).cloneType, config, port) := out_pack(id)
  }

  var macros = GatewaySink.collect(config)
  if (config.isBatch) {
    macros ++= Seq("CONFIG_DIFFTEST_BATCH", s"DIFFTEST_BATCH_SIZE ${config.batchSize}")
  }
  if (config.isSquash) {
    macros ++= Squash.collect(config)
  }

  val extraInstances = ListBuffer.empty[(DifftestBundle, String)]
  if (config.needTraceInfo) {
    extraInstances += ((signalsWithInfo.filter(_.desiredCppName == "trace_info").head, config.style))
  }
}


object GatewaySink{
  def apply[T <: DifftestBundle](gen: T, config: GatewayConfig, port: GatewayBundle): UInt = {
    config.style match {
      case "dpic" => DPIC(gen, config, port)
    }
  }

  def collect(config: GatewayConfig): Seq[String] = {
    config.style match {
      case "dpic" => DPIC.collect(config)
    }
  }
}

class GatewayBundle(config: GatewayConfig) extends Bundle {
  val enable = Bool()
  val dut_pos = Option.when(config.hasDutPos)(UInt(config.dutPosWidth.W))
}
