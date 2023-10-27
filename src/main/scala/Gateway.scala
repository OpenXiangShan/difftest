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

object Gateway {
  private val instances = ListBuffer.empty[DifftestBundle]
  private var config    = GatewayConfig()

  def apply[T <: DifftestBundle](gen: T, style: String): T = {
    config = GatewayConfig(style = style)
    register(WireInit(0.U.asTypeOf(gen)))
  }

  def register[T <: DifftestBundle](gen: T): T = {
    BoringUtils.addSource(gen, s"gateway_${instances.length}")
    instances += gen
    gen
  }

  def collect(): (Seq[String], UInt) = {
    val endpoint = Module(new GatewayEndpoint(instances.toSeq, config))
    (endpoint.macros, endpoint.step)
  }
}

class GatewayEndpoint(signals: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = WireInit(0.U.asTypeOf(MixedVec(signals.map(_.cloneType))))
  for ((data, id) <- in.zipWithIndex) {
    BoringUtils.addSink(data, s"gateway_$id")
  }
  val out = WireInit(in)

  val port = Wire(new GatewayBundle(config))

  val global_enable = WireInit(true.B)
  if(config.hasGlobalEnable) {
    global_enable := VecInit(in.filter(_.needUpdate.isDefined).map(_.needUpdate.get).toSeq).asUInt.orR
  }

  val enable = WireInit(false.B)
  enable := global_enable
  port.enable := enable

  if(config.diffStateSelect) {
    val select = RegInit(false.B)
    when(global_enable) {
      select := !select
    }
    port.select.get := select
  }

  val step_width = 1
  val step = IO(Output(UInt(step_width.W)))
  step := Mux(enable, 1.U, 0.U)

  for(id <- 0 until in.length){
    GatewaySink(in(id).cloneType, config, port) := out(id)
  }

  val macros = GatewaySink.collect(config)
}


object GatewaySink{
  def apply[T <: DifftestBundle](gen: T, config: GatewayConfig, port: GatewayBundle): T = {
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
  var enable = Bool()
  val select = Option.when(config.diffStateSelect)(Bool())
}
