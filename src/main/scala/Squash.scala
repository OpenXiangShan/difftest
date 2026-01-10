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
import difftest.validate.Validate._
import difftest.util.PipelineConnect

object Squash {
  def apply(
    bundles: DecoupledIO[MixedVec[Valid[DifftestBundle]]],
    config: GatewayConfig,
  ): DecoupledIO[MixedVec[Valid[DifftestBundle]]] = {
    val squashInBits = Stamp(bundles.bits)
    val squashIn = Wire(Decoupled(chiselTypeOf(squashInBits)))
    squashIn.bits := squashInBits
    squashIn.valid := bundles.valid
    bundles.ready := squashIn.ready
    val module = Module(new SquashEndpoint(chiselTypeOf(squashInBits).toSeq, config))
    module.in <> squashIn
    module.out
  }
}

object Stamp {
  def apply(bundles: MixedVec[Valid[DifftestBundle]]): MixedVec[Valid[DifftestBundle]] = {
    val module = Module(new Stamper(chiselTypeOf(bundles).toSeq))
    module.in := bundles
    module.out
  }
}

class Stamper(bundles: Seq[Valid[DifftestBundle]]) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val numCores = in.count(_.bits.isUniqueIdentifier)
  val stamp = RegInit(0.U.asTypeOf(Vec(numCores, UInt(12.W)))) // StampSize corresponds to Cpp Macros
  val commits = in.filter(_.bits.desiredCppName == "commit").map(_.asInstanceOf[Valid[DiffInstrCommit]])
  val commitLen = commits.length / numCores
  val commitSum = VecInit.tabulate(numCores) { id =>
    val commitCnt =
      commits.slice(id * commitLen, (id + 1) * commitLen).map { c =>
        Mux(c.valid && !c.bits.skip, 1.U + c.bits.nFused, 0.U)
      }
    VecInit.tabulate(commitLen) { idx =>
      commitCnt.take(idx + 1).reduce(_ + _)
    }
  }
  stamp.zip(commitSum).foreach { case (s, sum) =>
    s := s + sum.last
  }

  // Instantiation of LoadEvent corresponds to InstrCommit
  val commitData = in.filter(_.bits.desiredCppName == "commit_data").map(_.asInstanceOf[Valid[DiffCommitData]])
  val vecCommitData =
    in.filter(_.bits.desiredCppName == "vec_commit_data").map(_.asInstanceOf[Valid[DiffVecCommitData]])
  val hasVCD = vecCommitData.nonEmpty
  val loads = in.filter(_.bits.desiredCppName == "load").map(_.asInstanceOf[Valid[DiffLoadEvent]])
  val loadQueues =
    loads.zip(commits).zip(commitData).zip(commitSum.flatten).zipWithIndex.map { case ((((ld, c), cd), sum), idx) =>
      val lq = WireInit(0.U.asTypeOf(Valid(new DiffLoadEventQueue)))
      lq.inheritFrom(ld)
      lq.bits.stamp := stamp(ld.bits.coreid) + sum
      lq.bits.commitData := cd.bits.data
      if (hasVCD) {
        lq.bits.vecCommitData := vecCommitData(idx).bits.data
      }
      lq.bits.regWen := ((c.bits.rfwen && c.bits.wdest =/= 0.U) || c.bits.fpwen) && !c.bits.vecwen
      lq.bits.wdest := c.bits.wdest
      lq.bits.fpwen := c.bits.fpwen
      lq.bits.vecwen := c.bits.vecwen
      lq.bits.v0wen := c.bits.v0wen
      lq
    }

  val stores = in.filter(_.bits.desiredCppName == "store").map(_.asInstanceOf[Valid[DiffStoreEvent]])
  val storeQueues = stores.map { st =>
    val sq = WireInit(0.U.asTypeOf(Valid(new DiffStoreEventQueue)))
    sq.inheritFrom(st)
    val base = stamp(sq.bits.coreid)
    val inc = commitSum(sq.bits.coreid).last
    // If no instr committed in the same cycle, store event will be checked in next commit
    sq.bits.stamp := Mux(inc === 0.U, base + 1.U, base + inc)
    sq
  }

  val withStamp = MixedVecInit(
    (in.filterNot(b => Seq("load", "store").contains(b.bits.desiredCppName)) ++ loadQueues ++ storeQueues).toSeq
  )
  val out = IO(Output(chiselTypeOf(withStamp)))
  out := withStamp
}

class SquashEndpoint(bundles: Seq[Valid[DifftestBundle]], config: GatewayConfig) extends Module {
  val in = IO(Flipped(Decoupled(MixedVec(bundles))))
  val numCores = in.bits.count(_.bits.isUniqueIdentifier)

  val pipelined = Wire(Decoupled(MixedVec(bundles)))
  PipelineConnect(in, pipelined, pipelined.ready)
  val control = Module(new SquashControl(config))
  control.clock := clock
  control.reset := reset
  val in_replay =
    pipelined.bits
      .map(_.bits)
      .filter(_.desiredCppName == "trace_info")
      .map(_.asInstanceOf[DiffTraceInfo].in_replay)
      .foldLeft(false.B)(_ || _)
  val timeout_count = RegInit(0.U(32.W))
  val timeout = timeout_count === 200000.U
  val global_tick = !control.enable || in_replay || timeout

  val uniqBundles = bundles.map(_.bits).distinctBy(_.desiredCppName)
  // Tick and Submit the pending non-squashable events immediately.
  val want_tick_vec = WireInit(VecInit.fill(uniqBundles.length)(false.B))
  when(!want_tick_vec.asUInt.orR) {
    timeout_count := timeout_count + 1.U
  }.otherwise {
    timeout_count := 0.U
  }
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
    val s_in = pipelined.bits.filter(_.bits.desiredCppName == u.desiredCppName)
    val squasher = Module(new Squasher(chiselTypeOf(s_in.head), s_in.length, numCores, config))
    squasher.in.zip(s_in).foreach { case (i, s_i) => i := s_i }
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
  val out = IO(Decoupled(MixedVec(s_out_vec.flatMap(chiselTypeOf(_)))))
  s_out_vec.zipWithIndex.foreach { case (vec, i) =>
    val base = if (i != 0) {
      s_out_vec.take(i).map(_.length).sum
    } else 0
    vec.zipWithIndex.foreach { case (gen, idx) =>
      out.bits(base + idx) := gen
    }
  }
  pipelined.ready := out.ready
  out.valid := VecInit(out.bits.map(_.valid)).asUInt.orR
}

// It will help do squash for bundles with same Class, return tick and state
class Squasher(bundleType: Valid[DifftestBundle], length: Int, numCores: Int, config: GatewayConfig) extends Module {
  val in = IO(Input(Vec(length, bundleType)))
  val want_tick = IO(Output(Bool()))
  val should_tick = IO(Input(Bool()))

  val state = RegInit(0.U.asTypeOf(Vec(length, bundleType)))
  val out = IO(Output(Vec(length, bundleType)))

  // Mark the initial commit events as non-squashable for initial state synchronization.
  val tick_first_commit = Option.when(bundleType.bits.desiredCppName == "commit") {
    VecInit
      .tabulate(numCores) { id =>
        val hasValidCommitEvent = VecInit(state.map(c => c.bits.coreid === id.U && c.valid).toSeq).asUInt.orR
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

  for ((i, s) <- in.zip(state)) {
    when(should_tick) {
      s := i
    }.otherwise {
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
       |`include "DifftestMacros.svh"
       |module SquashControl(
       |  input clock,
       |  input reset,
       |  output enable
       |);
       |
       |`ifndef SYNTHESIS
       |`ifndef CONFIG_DIFFTEST_FPGA
       |`define SQUASH_CTRL
       |`endif // CONFIG_DIFFTEST_FPGA
       |`endif // SYNTHESIS
       |
       |`ifndef SQUASH_CTRL
       |  assign enable = 1;
       |`else
       |`ifdef DIFFTEST
       |import "DPI-C" context function void set_squash_scope();
       |  reg _enable;
       |  assign enable = _enable;
       |initial begin
       |  _enable = 1;
       |  set_squash_scope();
       |end
       |
       |// For the C/C++ interface
       |export "DPI-C" function set_squash_enable;
       |function void set_squash_enable(int en);
       |  _enable = en;
       |endfunction
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
       |      _enable = 0;
       |    end
       |  end
       |end
       |`endif // DIFFTEST
       |`endif // SQUASH_CTRL
       |
       |endmodule
       |""".stripMargin,
  )
}
