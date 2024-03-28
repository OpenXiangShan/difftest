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

package difftest.replay

import chisel3._
import chisel3.experimental.ExtModule
import chisel3.util._
import difftest._
import difftest.gateway.GatewayConfig

object Replay {
  def apply(bundles: MixedVec[DifftestBundle], config: GatewayConfig): MixedVec[DifftestBundle] = {
    val module = Module(new ReplayEndpoint(chiselTypeOf(bundles).toSeq, config))
    module.in := bundles
    module.out
  }
}

class ReplayEndpoint(bundles: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val info = WireInit(0.U.asTypeOf(new DiffTraceInfo))
  val appendIn = WireInit(0.U.asTypeOf(MixedVec(bundles ++ Seq(chiselTypeOf(info)))))
  in.zipWithIndex.foreach { case (gen, idx) => appendIn(idx) := gen }
  appendIn.last := info
  val out = IO(Output(chiselTypeOf(appendIn)))

  val control = Module(new ReplayControl(config))
  control.clock := clock
  control.reset := reset

  val buffer = Mem(config.replaySize, chiselTypeOf(appendIn))
  val ptr = RegInit(0.U(log2Ceil(config.replaySize).W))

  // write
  // ignore useless data when hasGlobalEnable
  val needStore = WireInit(true.B)
  if (config.hasGlobalEnable) {
    needStore := VecInit(in.flatMap(_.bits.needUpdate).toSeq).asUInt.orR
  }
  info.valid := needStore
  info.trace_head := ptr
  info.trace_tail := ptr
  when(needStore && !control.replay) {
    buffer(ptr) := appendIn
    ptr := ptr + 1.U
    when(ptr === (config.replaySize - 1).U) {
      ptr := 0.U
    }
  }

  // read
  val in_replay = RegInit(false.B) // indicates ptr in correct replay pos
  when(control.replay) {
    when(!in_replay) {
      in_replay := true.B
      ptr := control.replay_head // position of first corresponding replay data
    }.otherwise {
      ptr := ptr + 1.U
      when(ptr === (config.replaySize - 1).U) {
        ptr := 0.U
      }
    }
  }
  out := Mux(in_replay, buffer(ptr), appendIn)
  out.filter(_.desiredCppName == "trace_info").foreach { gen =>
    val info = gen.asInstanceOf[DiffTraceInfo]
    info.in_replay := in_replay
  }
}

class ReplayControl(config: GatewayConfig) extends ExtModule with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val reset = IO(Input(Reset()))
  val replay = IO(Output(Bool()))
  val replay_head = IO(Output(UInt(log2Ceil(config.replaySize).W)))

  setInline(
    "ReplayControl.v",
    s"""
       |module ReplayControl(
       |  input clock,
       |  input reset,
       |  output reg replay,
       |  output reg [${log2Ceil(config.replaySize) - 1}:0] replay_head
       |);
       |
       |`ifndef SYNTHESIS
       |`ifdef DIFFTEST
       |import "DPI-C" context function void set_replay_scope();
       |
       |initial begin
       |  set_replay_scope();
       |  replay = 1'b0;
       |  replay_head = ${log2Ceil(config.replaySize)}'b0;
       |end
       |
       |// For the C/C++ interface
       |export "DPI-C" task set_replay_head;
       |task set_replay_head(int head);
       |  replay = 1'b1;
       |  replay_head = head;
       |endtask
       |`endif // DIFFTEST
       |`endif // SYNTHESIS
       |endmodule;
       |""".stripMargin,
  )
}
