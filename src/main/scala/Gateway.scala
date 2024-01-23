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
                        diffStateSelect: Boolean = false,
                        isBatch        : Boolean = false,
                        batchSize      : Int     = 32,
                        isNonBlock     : Boolean = false,
                        )
{
  require(!(diffStateSelect && isBatch))
  if (squashReplay) require(isSquash)
  def hasDutPos: Boolean = diffStateSelect || isBatch
  def dutBufLen: Int = if (isBatch) batchSize else if (diffStateSelect) 2 else 1
  def dutPosWidth: Int = log2Ceil(dutBufLen)
  def maxStep: Int = if (isBatch) batchSize else 1
  def stepWidth: Int = log2Ceil(maxStep + 1)
  def hasDeferredResult: Boolean = isNonBlock
  def needTraceInfo: Boolean = squashReplay
  def needEndpoint: Boolean = hasGlobalEnable || diffStateSelect || isBatch || isSquash
  // Macros Generation for Cpp and Verilog
  def cppMacros: Seq[String] = {
    val macros = ListBuffer.empty[String]
    macros += s"CONFIG_DIFFTEST_${style.toUpperCase}"
    macros += s"CONFIG_DIFFTEST_BUFLEN ${dutBufLen}"
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

  def collect(): (Seq[String], Seq[String], Seq[(DifftestBundle, String)], UInt) = {
    val extraInstances = ListBuffer.empty[(DifftestBundle, String)]
    val step = WireInit(1.U)
    if (config.needEndpoint) {
      val endpoint = Module(new GatewayEndpoint(instances.toSeq, config))
      extraInstances ++= endpoint.extraInstances
      step := endpoint.step
    }
    else {
      GatewaySink.collect(config)
    }
    (config.cppMacros, config.vMacros, extraInstances.toSeq, step)
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

  val port_num = if (config.isBatch) config.batchSize else 1
  val ports = Seq.fill(port_num)(Wire(new GatewayBundle(config)))

  val global_enable = WireInit(true.B)
  if(config.hasGlobalEnable) {
    global_enable := VecInit(out.flatMap(_.bits.needUpdate).toSeq).asUInt.orR
  }

  val batch_data = Option.when(config.isBatch)(Mem(config.batchSize, out_pack.cloneType))
  val enable = WireInit(false.B)
  if(config.isBatch) {
    val batch_ptr = RegInit(0.U(log2Ceil(config.batchSize).W))
    when(global_enable) {
      batch_ptr := batch_ptr + 1.U
      when(batch_ptr === (config.batchSize - 1).U) {
        batch_ptr := 0.U
      }
      batch_data.get(batch_ptr) := out_pack
    }
    val do_batch_sync = batch_ptr === (config.batchSize - 1).U && global_enable
    enable := RegNext(do_batch_sync)
  }
  else{
    enable := global_enable
  }
  ports.foreach(port => port.enable := enable)

  if(config.diffStateSelect) {
    val select = RegInit(false.B)
    when(global_enable) {
      select := !select
    }
    ports.foreach(port => port.dut_pos.get := select.asUInt)
  }

  val step = IO(Output(UInt(config.stepWidth.W)))
  step := Mux(enable, config.maxStep.U, 0.U)

  if (config.isBatch) {
    for (ptr <- 0 until config.batchSize) {
      ports(ptr).dut_pos.get := ptr.asUInt
      for(id <- 0 until out.length) {
        GatewaySink(out(id).cloneType, config, ports(ptr)) := batch_data.get(ptr)(id)
      }
    }
  }
  else {
    for(id <- 0 until out.length){
      GatewaySink(out(id).cloneType, config, ports.head) := out_pack(id)
    }
  }

  GatewaySink.collect(config)

  val extraInstances = ListBuffer.empty[(DifftestBundle, String)]
  if (config.needTraceInfo) {
    extraInstances += ((signalsWithInfo.filter(_.desiredCppName == "trace_info").head, config.style))
  }
}


object GatewaySink{
  def apply[T <: DifftestBundle](gen: T, config: GatewayConfig, port: GatewayBundle): UInt = {
    config.style match {
      case "dpic" => DPIC(gen, config, port)
      case _ => DPIC(gen, config, port) // Default: DPI-C
    }
  }

  def collect(config: GatewayConfig): Unit = {
    config.style match {
      case "dpic" => DPIC.collect()
      case _ => DPIC.collect() // Default: DPI-C
    }
  }
}

class GatewayBundle(config: GatewayConfig) extends Bundle {
  val enable = Bool()
  val dut_pos = Option.when(config.hasDutPos)(UInt(config.dutPosWidth.W))
}
