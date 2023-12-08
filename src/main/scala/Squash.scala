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

import scala.collection.mutable.ListBuffer

object Squash {
  private val instances = ListBuffer.empty[DifftestBundle]

  def apply[T <: Seq[DifftestBundle]](bundles: T): SquashEndpoint = {
    val module = Module(new SquashEndpoint(bundles))
    module
  }

  def collect(): Seq[String] = {
    Seq("CONFIG_DIFFTEST_SQUASH")
  }
}

class SquashEndpoint(bundles: Seq[DifftestBundle]) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val out = IO(Output(MixedVec(bundles)))

  val state = RegInit(0.U.asTypeOf(MixedVec(bundles)))

  // Mark the initial commit events as non-squashable for initial state synchronization.
  val hasValidCommitEvent = VecInit(state.filter(_.desiredCppName == "commit").map(_.bits.getValid).toSeq).asUInt.orR
  val isInitialEvent = RegInit(true.B)
  when (isInitialEvent && hasValidCommitEvent) {
    isInitialEvent := false.B
  }
  val tick_first_commit = isInitialEvent && hasValidCommitEvent

  // If one of the bundles cannot be squashed, the others are not squashed as well.
  val supportsSquashVec = VecInit(in.zip(state).map{ case (i, s) => i.supportsSquash(s) }.toSeq)
  val supportsSquash = supportsSquashVec.asUInt.andR

  // If one of the bundles cannot be the new base, the others are not as well.
  val supportsSquashBaseVec = VecInit(state.map(_.supportsSquashBase).toSeq)
  val supportsSquashBase = supportsSquashBaseVec.asUInt.andR

  val control = Module(new SquashControl)
  control.clock := clock
  control.reset := reset

  // Submit the pending non-squashable events immediately.
  val should_tick = !control.enable || !supportsSquash || !supportsSquashBase || tick_first_commit
  out := Mux(should_tick, state, 0.U.asTypeOf(MixedVec(bundles)))

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
}

class SquashControl extends ExtModule with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val reset = IO(Input(Reset()))
  val enable = IO(Output(Bool()))

  setInline("SquashControl.v",
    """
      |module SquashControl(
      |  input clock,
      |  input reset,
      |  output reg enable
      |);
      |
      |import "DPI-C" context function void set_squash_scope();
      |
      |initial begin
      |  set_squash_scope();
      |  enable = 1'b1;
      |end
      |
      |// For the C/C++ interface
      |export "DPI-C" task set_squash_enable;
      |task set_squash_enable(int en);
      |  enable = en;
      |endtask
      |
      |// For the simulation argument +squash_cycles=N
      |reg [63:0] squash_cycles;
      |initial begin
      |  squash_cycles = 0;
      |  if ($test$plusargs("squash-cycles")) begin
      |    $value$plusargs("squash-cycles=%d", squash_cycles);
      |    $display("set squash cycles: %d", squash_cycles);
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
