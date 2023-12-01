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

package difftest.squash

import chisel3._
import chisel3.experimental.ExtModule
import chisel3.util._
import chisel3.util.experimental.BoringUtils
import difftest._
import difftest.gateway.GatewayConfig

import scala.collection.mutable.ListBuffer

object Squash {
  private val instances = ListBuffer.empty[DifftestBundle]

  def apply[T <: Seq[DifftestBundle]](bundles: T, config: GatewayConfig): SquashEndpoint = {
    val module = Module(new SquashEndpoint(bundles, config))
    module
  }

  def collect(config: GatewayConfig): Seq[String] = {
    var macros = Seq("CONFIG_DIFFTEST_SQUASH")
    if (config.squashReplay) {
      macros ++= Seq("CONFIG_DIFFTEST_SQUASH_REPLAY")
    }
    macros
  }
}

class SquashEndpoint(bundles: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val out = IO(Output(MixedVec(bundles)))
  val idx = Option.when(config.diffStateSelect)(IO(Output(UInt(log2Ceil(config.replaySize).W))))

  val state = RegInit(0.U.asTypeOf(MixedVec(bundles)))

  // Mark the initial commit events as non-squashable for initial state synchronization.
  val hasValidCommitEvent = VecInit(state.filter(_.desiredCppName == "commit").map(_.bits.getValid).toSeq).asUInt.orR
  val isInitialEvent = RegInit(true.B)
  when (isInitialEvent && hasValidCommitEvent) {
    isInitialEvent := false.B
  }
  val tick_first_commit = isInitialEvent && hasValidCommitEvent

  // If one of the bundles cannot be squashed, the others are not squashed as well.
  val supportsSquashVec = VecInit(in.zip(state).map{ case (i, s) => i.supportsSquash(s, config.squashSize - 1) }.toSeq)
  val supportsSquash = supportsSquashVec.asUInt.andR

  // If one of the bundles cannot be the new base, the others are not as well.
  val supportsSquashBaseVec = VecInit(state.map(_.supportsSquashBase).toSeq)
  val supportsSquashBase = supportsSquashBaseVec.asUInt.andR

  val control = Module(new SquashControl(config))
  control.clock := clock
  control.reset := reset

  // Submit the pending non-squashable events immediately.
  val should_tick = !control.enable || !supportsSquash || !supportsSquashBase || tick_first_commit

  val squashed = Mux(should_tick, state, 0.U.asTypeOf(MixedVec(bundles)))

  // Sometimes, the bundle may have squash dependencies.
  val do_squash = WireInit(VecInit.fill(in.length)(true.B))
  in.zip(do_squash).foreach{ case (i, do_m) =>
    if (i.squashDependency.nonEmpty) {
      do_m := VecInit(in.filter(b => i.squashDependency.contains(b.desiredCppName)).map(bundle => {
        // Only if the corresponding bundle is valid, we update this bundle
        bundle.coreid === i.coreid && bundle.asInstanceOf[DifftestBaseBundle].getValid
      }).toSeq).asUInt.orR
    }
  }

  for (((i, d), s) <- in.zip(do_squash).zip(state)) {
      when (should_tick) {
        s := i
      }.elsewhen (d) {
        s := i.squash(s)
      }
  }

  if (config.squashReplay) {
    val replay_data = Mem(config.replaySize, in.cloneType)
    val replay_wptr = RegInit(0.U(log2Ceil(config.replaySize).W))
    val replay_rptr = RegInit(0.U(log2Ceil(config.replaySize).W))
    val replay_table = Mem(config.replaySize, replay_rptr.cloneType)

    // Maybe every state is non-squashable, preventing two replayable instructions have the same idx
    val squash_idx = RegInit(0.U(log2Ceil(config.replaySize).W))

    when (should_tick & !control.replay.get) {
      val next_squash_idx = Mux(squash_idx === (config.replaySize - 1).U, 0.U, squash_idx + 1.U)
      replay_table(next_squash_idx) := replay_wptr
      squash_idx := next_squash_idx
    }
    when ((should_tick || do_squash.asUInt.orR) && !control.replay.get) {
      replay_data(replay_wptr) := in
      replay_wptr := replay_wptr + 1.U
      when (replay_wptr === (config.replaySize - 1).U) {
        replay_wptr := 0.U
      }
    }
    val in_replay = RegInit(false.B)
    when (control.replay.get) {
      when (!in_replay) {
        in_replay := true.B
        replay_rptr := replay_table(control.replay_idx.get)
      }.otherwise {
        replay_rptr := replay_rptr + 1.U
        when (replay_rptr === (config.replaySize - 1).U) {
          replay_rptr := 0.U
        }
      }
    }
    idx.get := Mux(in_replay, control.replay_idx.get, squash_idx)
    out := Mux(in_replay, replay_data(replay_rptr), squashed)
  }
  else {
    out := squashed
  }

}

class SquashControl(config: GatewayConfig) extends ExtModule with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val reset = IO(Input(Reset()))
  val enable = IO(Output(Bool()))
  val replay = Option.when(config.squashReplay)(IO(Output(Bool())))
  val replay_idx = Option.when(config.squashReplay)(IO(Output(UInt(log2Ceil(config.replaySize).W))))

  val replay_port = if (config.squashReplay)
    s"""
      |  output reg replay,
      |  output reg [${log2Ceil(config.replaySize)-1}:0] replay_idx,
      |""".stripMargin
  val replay_init = if (config.squashReplay)
    s"""
      |  replay = 1'b0;
      |  replay_idx = ${log2Ceil(config.replaySize)}'b0;
      |""".stripMargin
  val replay_task = if (config.squashReplay)
    """
      |export "DPI-C" task set_squash_replay;
      |task set_squash_replay(int idx);
      |  replay = 1'b1;
      |  replay_idx = idx;
      |endtask
      |""".stripMargin

  setInline("SquashControl.v",
    s"""
      |module SquashControl(
      |  input clock,
      |  input reset,
      |$replay_port
      |  output reg enable
      |);
      |
      |import "DPI-C" context function void set_squash_scope();
      |
      |initial begin
      |  set_squash_scope();
      |  enable = 1'b1;
      |$replay_init
      |end
      |
      |// For the C/C++ interface
      |export "DPI-C" task set_squash_enable;
      |task set_squash_enable(int en);
      |  enable = en;
      |endtask
      |$replay_task
      |
      |// For the simulation argument +squash_cycles=N
      |reg [63:0] squash_cycles;
      |initial begin
      |  squash_cycles = 0;
      |  if ($$test$$plusargs("squash-cycles")) begin
      |    $$value$$plusargs("squash-cycles=%d", squash_cycles);
      |    $$display("set squash cycles: %d", squash_cycles);
      |  end
      |end
      |
      |reg [63:0] n_cycles;
      |always @(posedge clock) begin
      |  if (reset) begin
      |    n_cycles <= 64'h0;
      |  end
      |  else begin
      |    n_cycles <= n_cycles + 64'h1;
      |    if (squash_cycles > 0 && n_cycles >= squash_cycles) begin
      |      enable = 0;
      |    end
      |  end
      |end
      |
      |
      |endmodule;
      |""".stripMargin
  )
}
