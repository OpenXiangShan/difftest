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
    val squashIn = Stamp(bundles)
    val module = Module(new SquashEndpoint(chiselTypeOf(squashIn).toSeq, config))
    module.in := squashIn
    module.out
  }
}

object Stamp {
  def apply(bundles: MixedVec[DifftestBundle]): MixedVec[DifftestBundle] = {
    val module = Module(new Stamper(chiselTypeOf(bundles).toSeq))
    module.in := bundles
    module.out
  }
}

class Stamper(bundles: Seq[DifftestBundle]) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val numCores = in.count(_.isUniqueIdentifier)
  val stamp = RegInit(0.U.asTypeOf(Vec(numCores, UInt(12.W)))) // StampSize corresponds to Cpp Macros
  val commits = in.filter(_.desiredCppName == "commit").map(_.asInstanceOf[DiffInstrCommit])
  val commitLen = commits.length / numCores
  val commitSum = VecInit.tabulate(numCores) { id =>
    val commitCnt =
      commits.slice(id * commitLen, (id + 1) * commitLen).map { c => Mux(c.valid && !c.skip, 1.U + c.nFused, 0.U) }
    VecInit.tabulate(commitLen) { idx =>
      commitCnt.take(idx + 1).reduce(_ + _)
    }
  }
  stamp.zip(commitSum).foreach { case (s, sum) =>
    s := s + sum.last
  }

  // Instantiation of LoadEvent corresponds to InstrCommit
  val commitData = in.filter(_.desiredCppName == "commit_data").map(_.asInstanceOf[DiffCommitData])
  val loads = in.filter(_.desiredCppName == "load").map(_.asInstanceOf[DiffLoadEvent])
  val loadQueues = loads.zip(commits).zip(commitData).zip(commitSum.flatten).map { case (((ld, c), cd), sum) =>
    val lq = WireInit(0.U.asTypeOf(new DiffLoadEventQueue))
    lq.inheritFrom(ld)
    lq.stamp := stamp(ld.coreid) + sum
    lq.commitData := cd.data
    lq.regWen := ((c.rfwen && c.wdest =/= 0.U) || c.fpwen) && !c.vecwen
    lq.wdest := c.wdest
    lq.fpwen := c.fpwen
    lq
  }

  val stores = in.filter(_.desiredCppName == "store").map(_.asInstanceOf[DiffStoreEvent])
  val storeQueues = stores.map { st =>
    val sq = WireInit(0.U.asTypeOf(new DiffStoreEventQueue))
    sq.inheritFrom(st)
    val base = stamp(sq.coreid)
    val inc = commitSum(sq.coreid).last
    // If no instr committed in the same cycle, store event will be checked in next commit
    sq.stamp := Mux(inc === 0.U, base + 1.U, base + inc)
    sq
  }

  val withStamp = MixedVecInit(
    (in.filterNot(b => Seq("load", "store").contains(b.desiredCppName)) ++ loadQueues ++ storeQueues).toSeq
  )
  val out = IO(Output(chiselTypeOf(withStamp)))
  out := withStamp
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
        if (u.squashQueue) {
          false.B
        } else {
          if (config.hasBuiltInPerf) DifftestPerf(s"SquashTick_${g}_${u.desiredCppName}", wt)
          wt
        }
      }
      .reduce(_ || _)
  })

  val s_out_vec = uniqBundles.zip(want_tick_vec).map { case (u, wt) =>
    val (s_in, s_do) = in.zip(do_squash).filter(_._1.desiredCppName == u.desiredCppName).unzip
    val squasher = Module(new Squasher(chiselTypeOf(s_in.head), s_in.length, numCores, config))
    squasher.in.zip(s_in).foreach { case (i, s_i) => i := s_i }
    squasher.do_squash.zip(s_do).foreach { case (d, s_d) => d := s_d }
    wt := squasher.want_tick
    val group_tick =
      group_name_vec
        .zip(group_tick_vec)
        .collect { case (n, gt) if u.squashGroup.contains(n) => gt }
        .foldLeft(false.B)(_ || _)
    squasher.should_tick := wt || group_tick || global_tick
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
  val want_tick = IO(Output(Bool()))
  val should_tick = IO(Input(Bool()))

  val state = RegInit(0.U.asTypeOf(Vec(length, bundleType)))
  val out = IO(Output(Vec(length, bundleType)))

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

  // If one of the bundles cannot be squashed, the others are not squashed as well.
  val supportsSquashVec = VecInit(in.zip(state).map { case (i, s) => i.supportsSquash(s) }.toSeq)
  val supportsSquash = supportsSquashVec.asUInt.andR

  // If one of the bundles cannot be the new base, the others are not as well.
  val supportsSquashBaseVec = VecInit(state.map(_.supportsSquashBase).toSeq)
  val supportsSquashBase = supportsSquashBaseVec.asUInt.andR

  want_tick := !supportsSquash || !supportsSquashBase || tick_first_commit.getOrElse(false.B)

  for (((i, ds), s) <- in.zip(do_squash).zip(state)) {
    when(should_tick) {
      s := i
    }.elsewhen(ds) {
      s := i.squash(s)
    }
  }
  out := Mux(should_tick, state, 0.U.asTypeOf(out))
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
