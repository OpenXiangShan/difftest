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
import chisel3.util.experimental.BoringUtils
import difftest._
import difftest.dpic.DPIC

import scala.collection.mutable.ListBuffer

case class GatewayConfig(
                        style          : String  = "dpic",
                        hasGlobalEnable: Boolean = false,
                        diffStateSelect: Boolean = false,
                        isBatch        : Boolean = false,
                        batchSize      : Int     = 32
                        )
{
  def needEndpoint: Boolean = hasGlobalEnable || diffStateSelect || isBatch
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
    BoringUtils.addSource(gen_pack, s"gateway_${instances.length}")
    instances += gen
    gen
  }

  def collect(): (Seq[String], UInt) = {
    if (config.needEndpoint) {
      val endpoint = Module(new GatewayEndpoint(instances.toSeq, config))
      (endpoint.macros, endpoint.step)
    }
    else {
      (GatewaySink.collect(config), 1.U)
    }
  }
}

class GatewayEndpoint(signals: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = WireInit(0.U.asTypeOf(MixedVec(signals.map(_.cloneType))))
  val in_pack = WireInit(0.U.asTypeOf(MixedVec(signals.map(gen => UInt(gen.getWidth.W)))))
  for ((data, id) <- in_pack.zipWithIndex) {
    BoringUtils.addSink(data, s"gateway_$id")
    in(id) := data.asTypeOf(in(id).cloneType)
  }
  val out = WireInit(in)
  val out_pack = WireInit(in_pack)

  if (config.diffStateSelect || config.isBatch) {
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

      for ((data, id) <- out_pack.zipWithIndex) {
        data := out(id).asUInt
      }
    }
  }

  val port_num = if (config.isBatch) config.batchSize else 1
  val ports = Seq.fill(port_num)(Wire(new GatewayBundle(config)))

  val global_enable = WireInit(true.B)
  if(config.hasGlobalEnable) {
    global_enable := VecInit(in.filter(_.needUpdate.isDefined).map(_.needUpdate.get).toSeq).asUInt.orR
  }

  val batch_data = Option.when(config.isBatch)(Mem(config.batchSize, in_pack.cloneType))
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
    ports.foreach(port => port.select.get := select)
  }

  val step_width = if (config.isBatch) log2Ceil(config.batchSize+1) else 1
  val upper = if(config.isBatch) config.batchSize.U else 1.U
  val step = IO(Output(UInt(step_width.W)))
  step := Mux(enable, upper, 0.U)

  if (config.isBatch) {
    for (ptr <- 0 until config.batchSize) {
      ports(ptr).batch_idx.get := ptr.asUInt(log2Ceil(config.batchSize).W)
      for(id <- 0 until in.length) {
        GatewaySink(in(id).cloneType, config, ports(ptr)) := batch_data.get(ptr)(id)
      }
    }
  }
  else {
    for(id <- 0 until in.length){
      GatewaySink(in(id).cloneType, config, ports.head) := out_pack(id)
    }
  }

  var macros = GatewaySink.collect(config)
  if (config.isBatch) {
    macros ++= Seq("CONFIG_DIFFTEST_BATCH", s"DIFFTEST_BATCH_SIZE ${config.batchSize}")
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
  val select = Option.when(config.diffStateSelect)(Bool())
  val batch_idx = Option.when(config.isBatch)(UInt(log2Ceil(config.batchSize).W))
}
