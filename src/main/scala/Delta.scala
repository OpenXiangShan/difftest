/***************************************************************************************
 * Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.delta

import chisel3._
import chisel3.util._
import difftest._
import difftest.common.FileControl
import difftest.gateway.GatewayConfig
import difftest.util.{LookupTree, PipelineConnect}

import scala.collection.mutable.ListBuffer

object Delta {
  private val instances = ListBuffer.empty[DifftestBundle]
  def apply(
    bundles: DecoupledIO[MixedVec[Valid[DifftestBundle]]],
    config: GatewayConfig,
  ): DecoupledIO[MixedVec[Valid[DifftestBundle]]] = {
    instances ++= bundles.bits.map(_.bits)
    val module = Module(new DeltaEndpoint(chiselTypeOf(bundles.bits).toSeq, config))
    module.in <> bundles
    module.out
  }
  def collect(): Unit = {
    val deltaCpp = ListBuffer.empty[String]
    val deltaInsts = instances.filter(_.supportsDelta).distinct
    val deltaDecl = deltaInsts.map { inst =>
      val len = inst.dataElements.flatMap(_._3).length
      val elemType = s"uint${inst.deltaElemWidth}_t"
      s"$elemType ${inst.desiredCppName}_elem[$len];"
    }

    deltaCpp += "#ifndef __DIFFTEST_DELTA_H__"
    deltaCpp += "#define __DIFFTEST_DELTA_H__"
    deltaCpp += "#include \"difftest-state.h\""
    deltaCpp += (new DiffDeltaInfo).toCppDeclaration(true, true)
    deltaCpp +=
      s"""
         |typedef struct {
         |  DifftestDeltaInfo delta_info;
         |  ${deltaDecl.mkString("\n  ")}
         |} DeltaState;
         |""".stripMargin

    def deltaSync(dst: String, src: String): Seq[String] = {
      deltaInsts.map { inst =>
        val destName = inst.actualCppName
        val srcName = inst.desiredCppName
        s"memcpy(&($dst->$destName), $src->${srcName}_elem, sizeof(${inst.desiredModuleName}));"
      }.toSeq
    }
    deltaCpp +=
      s"""
         |class DeltaStats {
         |private:
         |  DeltaState buffer[NUM_CORES];
         |public:
         |  DeltaStats() {
         |    memset(buffer, 0, sizeof(buffer));
         |  }
         |  DeltaState* get(int coreid){
         |    return buffer + coreid;
         |  }
         |  bool need_pending() {
         |    for (int i = 0; i < NUM_CORES; i++) {
         |      auto info = get(i)->delta_info;
         |      if (info.inPending != 0) {
         |        return true;
         |      }
         |    }
         |    return false;
         |  }
         |  void sync(int zone, int index) {
         |    for (int i = 0; i < NUM_CORES; i++) {
         |      DiffTestState* dut = diffstate_buffer[i]->get(zone, index);
         |      DeltaState* delta = get(i);
         |      ${deltaSync("dut", "delta").mkString("\n      ")}
         |    }
         |  }
         |};
         |""".stripMargin
    deltaCpp += "#endif // __DIFFTEST_DELTA_H__"
    FileControl.write(deltaCpp, "difftest-delta.h")
  }
}

class DeltaSplitter(v_gen: Valid[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Input(v_gen))
  val out = IO(Output(Vec(config.deltaLimit, Valid(new DiffDeltaElem(v_gen.bits)))))
  val inPending = IO(Output(Bool()))
  val first_elems = VecInit(in.bits.dataElements.flatMap(_._3))
  val r_elems = RegInit(0.U.asTypeOf(first_elems))

  val first_updates = VecInit(first_elems.zip(r_elems).map { case (e, s) => e =/= s && in.valid })
  r_elems.zip(first_elems).zip(first_updates).map { case ((r, e), u) =>
    when(u) {
      r := e
    }
  }
  val needUpdate = first_updates.asUInt.orR
  val r_updates = RegEnable(first_updates, needUpdate)
  val updates = Mux(needUpdate, first_updates, r_updates)
  val elems = Mux(needUpdate, first_elems, r_elems)

  val first_group_updates = VecInit(first_updates.grouped(config.deltaLimit).map(_.reduce(_ || _)).toSeq)
  val group_size = first_group_updates.length
  val r_group_updates = RegInit(0.U(group_size.W))
  val group_updates = Mux(needUpdate, first_group_updates.asUInt, r_group_updates)
  val group_idx = PriorityEncoder(group_updates)
  val mask = (~0.U(group_size.W) << (group_idx +& 1.U)).asUInt(group_size - 1, 0)
  val next_group_updates = group_updates.asUInt & mask
  r_group_updates := next_group_updates

  inPending := next_group_updates =/= 0.U
  out.zipWithIndex.foreach { case (gen, idx) =>
    val sel_map = Seq.tabulate(group_size) { gid =>
      val seqID = gid * config.deltaLimit + idx
      val delta = WireInit(0.U.asTypeOf(Valid(new DiffDeltaElem(v_gen.bits))))
      if (seqID < elems.length) {
        delta.valid := updates(seqID) && group_updates =/= 0.U
        delta.bits.coreid := in.bits.coreid
        delta.bits.index := seqID.U
        delta.bits.data := elems(seqID)
      }
      (gid.U, delta)
    }
    gen := LookupTree(group_idx, sel_map)
  }
}

class DeltaEndpoint(bundles: Seq[Valid[DifftestBundle]], config: GatewayConfig) extends Module {
  val in = IO(Flipped(Decoupled(MixedVec(bundles))))
  val pipelined = Wire(Decoupled(MixedVec(bundles)))
  PipelineConnect(in, pipelined, pipelined.ready)
  val toDeltas = pipelined.bits.filter(_.bits.supportsDelta)
  val inPending = Wire(Vec(toDeltas.length, Bool()))
  pipelined.ready := !RegNext(inPending.asUInt.orR)

  val deltas = toDeltas.zipWithIndex.flatMap { case (v_gen, idx) =>
    val module = Module(new DeltaSplitter(chiselTypeOf(v_gen), config))
    module.in := v_gen
    inPending(idx) := module.inPending
    module.out
  }
  val deltaInfo = Wire(Valid(new DiffDeltaInfo))
  deltaInfo.valid := VecInit(deltas.map(_.valid)).asUInt.orR
  deltaInfo.bits.coreid := 0.U
  deltaInfo.bits.inPending := PopCount(inPending)

  val withDeltas = MixedVecInit((pipelined.bits.filterNot(_.bits.supportsDelta) ++ deltas ++ Seq(deltaInfo)).toSeq)
  val out = IO(Decoupled(chiselTypeOf(withDeltas)))
  out.valid := VecInit(withDeltas.map(_.valid)).asUInt.orR
  out.bits := withDeltas
}
