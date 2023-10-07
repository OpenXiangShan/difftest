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

package difftest.merge

import chisel3._
import chisel3.util._
import chisel3.util.experimental.BoringUtils
import difftest._

import scala.collection.mutable.ListBuffer

object Merge {
  private val isEffective: Boolean = false
  private val instances = ListBuffer.empty[DifftestBundle]

  def apply[T <: DifftestBundle](gen: T): T = {
    if (isEffective) register(gen, WireInit(0.U.asTypeOf(gen))) else gen
  }

  def register[T <: DifftestBundle](original: T, merged: T): T = {
    // There seems to be a bug in WiringUtils when original is some IO of Module.
    // We manually add a Wire for the source to avoid the WiringException.
    BoringUtils.addSource(WireInit(original), s"merge_in_${instances.length}")
    BoringUtils.addSink(merged, s"merge_out_${instances.length}")
    instances += original.cloneType
    merged
  }

  def collect(): Seq[String] = {
    if (isEffective) {
      Module(new MergeEndpoint(instances.toSeq))
      Seq("CONFIG_DIFFTEST_MERGE")
    }
    else {
      Seq()
    }
  }
}

class MergeEndpoint(bundles: Seq[DifftestBundle]) extends Module {
  val in = WireInit(0.U.asTypeOf(MixedVec(bundles)))
  for ((data, i) <- in.zipWithIndex) {
    BoringUtils.addSink(data, s"merge_in_$i")
  }

  val out = Wire(MixedVec(bundles))
  for ((data, i) <- out.zipWithIndex) {
    BoringUtils.addSource(data, s"merge_out_$i")
  }

  val state = RegInit(0.U.asTypeOf(MixedVec(bundles)))

  // Mark the initial commit events as non-mergeable for initial state synchronization.
  val hasValidCommitEvent = VecInit(state.filter(_.desiredCppName == "commit").map(_.bits.getValid).toSeq).asUInt.orR
  val isInitialEvent = RegInit(true.B)
  when (isInitialEvent && hasValidCommitEvent) {
    isInitialEvent := false.B
  }
  val tick_first_commit = isInitialEvent && hasValidCommitEvent

  // If one of the bundles cannot be merged, the others are not merged as well.
  val supportsMergeVec = VecInit(in.zip(state).map{ case (i, s) => i.supportsMerge(s) }.toSeq)
  val supportsMerge = supportsMergeVec.asUInt.andR

  // If one of the bundles cannot be the new base, the others are not as well.
  val supportsBaseVec = VecInit(state.map(_.supportsBase).toSeq)
  val supportsBase = supportsBaseVec.asUInt.andR

  // Submit the pending non-mergeable events immediately.
  val should_tick = !supportsMerge || !supportsBase || tick_first_commit
  out := Mux(should_tick, state, 0.U.asTypeOf(MixedVec(bundles)))

  for ((i, s) <- in.zip(state)) {
    s := Mux(should_tick, i, i.mergeWith(s))
  }

  // Special fix for int writeback. Work for single-core only.
  if (bundles.exists(_.desiredCppName == "wb_int")) {
    require(bundles.count(_.isUniqueIdentifier) == 1, "only single-core is supported yet")
    val writebacks = in.filter(_.desiredCppName == "wb_int").map(_.asInstanceOf[DiffIntWriteback])
    val numPhyRegs = writebacks.head.numElements
    val wb_int = Reg(Vec(numPhyRegs, UInt(64.W)))
    for (wb <- writebacks) {
      when (wb.valid) {
        wb_int(wb.address) := wb.data
      }
    }
    val commits = out.filter(_.desiredCppName == "commit").map(_.asInstanceOf[DiffInstrCommit])
    val num_skip = PopCount(commits.map(c => c.valid && c.skip))
    assert(num_skip <= 1.U, p"num_skip $num_skip is larger than one. Merge not supported yet")
    val wb_for_skip = out.filter(_.desiredCppName == "wb_int").head.asInstanceOf[DiffIntWriteback]
    for (c <- commits) {
      when (c.valid && c.skip) {
        wb_for_skip.valid := true.B
        wb_for_skip.address := c.wpdest
        wb_for_skip.data := wb_int(c.wpdest)
      }
    }
  }
}
