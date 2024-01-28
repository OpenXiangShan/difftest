/***************************************************************************************
 * Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.batch

import chisel3._
import chisel3.util._
import difftest._
import difftest.gateway.GatewayConfig

object Batch {
  def apply[T <: Seq[DifftestBundle]](bundles: T, config: GatewayConfig): BatchEndpoint = {
    val module = Module(new BatchEndpoint(bundles, config))
    module
  }
}

class BatchEndpoint(bundles: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val buffer = Mem(config.batchSize, in.cloneType)
  val out = IO(Output(Vec(config.batchSize, in.cloneType)))
  val info = IO(Output(Vec(config.batchSize, UInt())))
  val enable = IO(Output(Bool()))
  val step = IO(Output(UInt(config.stepWidth.W)))

  val need_store = WireInit(true.B)
  if (config.hasGlobalEnable) {
    need_store := VecInit(in.flatMap(_.bits.needUpdate).toSeq).asUInt.orR
  }
  val ptr = RegInit(0.U(log2Ceil(config.batchSize).W))
  when(need_store) {
    ptr := ptr + 1.U
    when(ptr === (config.batchSize - 1).U) {
      ptr := 0.U
    }
    buffer(ptr) := in
  }
  val do_sync = ptr === (config.batchSize - 1).U && need_store
  for (((data, ifo), idx) <- out.zip(info).zipWithIndex) {
    data := buffer(idx)
    ifo := idx.U
  }
  enable := RegNext(do_sync)
  step := Mux(enable, config.batchSize.U, 0.U)
}