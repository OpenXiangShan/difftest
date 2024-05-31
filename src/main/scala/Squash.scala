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
import difftest._
import difftest.gateway.GatewayConfig
import difftest.common.DifftestPerf

object Squash {
  def apply(bundles: MixedVec[DifftestBundle], config: GatewayConfig): MixedVec[DifftestBundle] = {
    val module = Module(new SquashEndpoint(chiselTypeOf(bundles).toSeq, config))
    module.in := bundles
    module.out
  }
}

class SquashEndpoint(bundles: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val numCores = in.count(_.isUniqueIdentifier)

  // Sometimes, the bundle may have squash dependencies.
  // Only when one of the dependencies is valid, this bundle is squashed.
  val do_squash = VecInit(in.map(_.bits.needUpdate.getOrElse(true.B)).toSeq)
  in.zip(do_squash).foreach { case (i, do_s) =>
    if (i.squashDependency.nonEmpty) {
      do_s := VecInit(
        in.filter(b => i.squashDependency.contains(b.desiredCppName))
          .map(bundle => {
            // Only if the corresponding bundle is valid, we update this bundle
            bundle.coreid === i.coreid && bundle.asInstanceOf[DifftestBaseBundle].getValid
          })
          .toSeq
      ).asUInt.orR && i.bits.needUpdate.getOrElse(true.B)
    }
  }

  // Sometimes, the bundle may be flushed by new-coming others that affect what it checks
  val do_flush = if (config.hasSquashFlush) {
    VecInit(in.map { i =>
      in.collect { case b if b.squashFlush.contains(i.desiredCppName) => b.bits.needUpdate.getOrElse(true.B) }
        .foldLeft(false.B)(_ || _)
    }.toSeq)
  } else {
    VecInit.fill(in.length)(false.B)
  }

  val control = Module(new SquashControl(config))
  control.clock := clock
  control.reset := reset
  val in_replay =
    in.filter(_.desiredCppName == "trace_info").map(_.asInstanceOf[DiffTraceInfo].in_replay).foldLeft(false.B)(_ || _)
  val global_tick = !control.enable || in_replay

  val uniqBundles = bundles.distinctBy(_.desiredCppName)
  // Tick and Submit the pending non-squashable events immediately.
  val want_tick_vec = WireInit(VecInit.fill(uniqBundles.length)(false.B))
  // Record Tick Cause for each SquashGroup
  val group_name_vec = uniqBundles.flatMap(_.squashGroup).distinct
  val group_tick_vec = VecInit(group_name_vec.map { g =>
    uniqBundles
      .zip(want_tick_vec)
      .filter(_._1.squashGroup.contains(g))
      .map { case (u, wt) =>
        if (config.hasBuiltInPerf) DifftestPerf(s"SquashTick_${g}_${u.desiredCppName}", wt)
        wt
      }
      .reduce(_ || _)
  })

  val s_out_vec = uniqBundles.zip(want_tick_vec).map { case (u, wt) =>
    val (s_in, s_do) = in.zip(do_squash.zip(do_flush)).filter(_._1.desiredCppName == u.desiredCppName).unzip
    val (s_do_s, s_do_f) = s_do.unzip
    val squasher = Module(new Squasher(chiselTypeOf(s_in.head), s_in.length, numCores, config))
    squasher.in.zip(s_in).foreach { case (i, s_i) => i := s_i }
    squasher.do_squash.zip(s_do_s).foreach { case (d, s_d) => d := s_d }
    squasher.do_flush.zip(s_do_f).foreach { case (d, s_d) => d := s_d }
    wt := squasher.want_tick
    val group_tick =
      group_name_vec
        .zip(group_tick_vec)
        .collect { case (n, gt) if u.squashGroup.contains(n) => gt }
        .foldLeft(false.B)(_ || _)
    squasher.should_tick := group_tick || global_tick
    squasher.out
  }
  // Flatten Seq[MixedVec[DifftestBundle]] to MixedVec[DifftestBundle]
  val out = IO(Output(MixedVec(s_out_vec.flatMap(chiselTypeOf(_)))))
  s_out_vec.zipWithIndex.foreach { case (vec, i) =>
    val base = if (i != 0) {
      s_out_vec.take(i).map(_.length).sum
    } else 0
    vec.zipWithIndex.foreach { case (gen, idx) =>
      out(base + idx) := gen
    }
  }
}

// It will help do squash for bundles with same Class, return tick and state
class Squasher(bundleType: DifftestBundle, length: Int, numCores: Int, config: GatewayConfig) extends Module {
  val in = IO(Input(Vec(length, bundleType)))
  val do_squash = IO(Input(Vec(length, Bool())))
  val do_flush = IO(Input(Vec(length, Bool())))
  val want_tick = IO(Output(Bool()))
  val should_tick = IO(Input(Bool()))

  // Bundles with nonEmpty squashFlush can only be queued with Flush enabled
  val hasQueue =
    config.hasSquashQueue && bundleType.squashQueueSize != 0 && (config.hasSquashFlush || bundleType.squashFlush.isEmpty)
  val vecLen = if (hasQueue) bundleType.squashQueueSize else length
  val state = RegInit(0.U.asTypeOf(Vec(vecLen, bundleType)))
  val out = IO(Output(Vec(vecLen, bundleType)))

  // Mark the initial commit events as non-squashable for initial state synchronization.
  val tick_first_commit = Option.when(bundleType.desiredCppName == "commit") {
    VecInit
      .tabulate(numCores) { id =>
        val hasValidCommitEvent = VecInit(state.map(c => c.coreid === id.U && c.bits.getValid).toSeq).asUInt.orR
        val isInitialEvent = RegInit(true.B)
        when(isInitialEvent && hasValidCommitEvent) {
          isInitialEvent := false.B
        }
        isInitialEvent && hasValidCommitEvent
      }
      .asUInt
      .orR
  }

  val tick_load_multicore = Option.when(bundleType.desiredCppName == "load" && numCores > 1) {
    VecInit(state.map(_.bits.getValid).toSeq).asUInt.orR
  }

  val force_tick = tick_first_commit.getOrElse(false.B) || tick_load_multicore.getOrElse(false.B)
  if (hasQueue) {
    // Bundle will not be squashed, but buffered and submit together.
    val ptr = RegInit(0.U(log2Ceil(vecLen + 1).W))
    val offset = PopCount(do_squash)
    val queue_exceed = ptr +& offset > vecLen.U
    want_tick := queue_exceed || force_tick
    ptr := Mux(should_tick, offset, ptr + offset)
    in.zip(do_squash).zipWithIndex.foreach { case ((i, d), idx) =>
      val postEnq = if (idx != 0) PopCount(do_squash.take(idx)) else 0.U
      when(should_tick && d) {
        state(postEnq) := i
      }.elsewhen(d) {
        state(ptr + postEnq) := i
      }
    }
    state.zip(out).zipWithIndex.foreach { case ((s, o), idx) =>
      o := Delayer(Mux(should_tick && ptr > idx.U, s, 0.U.asTypeOf(o)), bundleType.squashQueueDelay)
      o.asInstanceOf[DifftestWithIndex].index := idx.U
    }
  } else {
    // If one of the bundles cannot be squashed, the others are not squashed as well.
    val supportsSquashVec = VecInit(in.zip(state).map { case (i, s) => i.supportsSquash(s) }.toSeq)
    val supportsSquash = supportsSquashVec.asUInt.andR

    // If one of the bundles cannot be the new base, the others are not as well.
    val supportsSquashBaseVec = VecInit(state.map(_.supportsSquashBase).toSeq)
    val supportsSquashBase = supportsSquashBaseVec.asUInt.andR

    want_tick := !supportsSquash || !supportsSquashBase || force_tick

    for ((((i, ds), df), s) <- in.zip(do_squash).zip(do_flush).zip(state)) {
      when(should_tick) {
        s := i
      }.elsewhen(ds) {
        s := i.squash(s)
      }.elsewhen(df) {
        s := 0.U.asTypeOf(s)
      }
    }
    out := Mux(should_tick, state, 0.U.asTypeOf(out))
  }
}

class SquashControl(config: GatewayConfig) extends ExtModule with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val reset = IO(Input(Reset()))
  val enable = IO(Output(Bool()))

  setInline(
    "SquashControl.v",
    s"""
       |module SquashControl(
       |  input clock,
       |  input reset,
       |  output reg enable
       |);
       |
       |`ifndef SYNTHESIS
       |`ifdef DIFFTEST
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
       |`endif // DIFFTEST
       |`endif // SYNTHESIS
       |
       |// For the simulation argument +squash_cycles=N
       |reg [63:0] squash_cycles;
       |initial begin
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
       |""".stripMargin,
  )
}
