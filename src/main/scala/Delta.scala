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
import difftest.util.Delayer

import scala.collection.mutable.ListBuffer

object Delta {
  private val instances = ListBuffer.empty[DifftestBundle]
  def apply(bundles: MixedVec[Valid[DifftestBundle]], config: GatewayConfig): MixedVec[Valid[DifftestBundle]] = {
    instances ++= bundles.map(_.bits)
    val module = Module(new DeltaEndpoint(chiselTypeOf(bundles).toSeq, config))
    module.in := bundles
    module.out
  }
  def collect(): Unit = {
    val deltaCpp = ListBuffer.empty[String]
    val deltaInsts = instances.filter(_.supportsDelta).distinct
    val deltaDecl = deltaInsts.map { inst =>
      val len = inst.dataElements.flatMap(_._3).length
      val elemType = s"uint${inst.deltaElemBytes * 8}_t"
      s"$elemType ${inst.desiredCppName}_elem[$len];"
    }

    deltaCpp += "#ifndef __DIFFTEST_DELTA_H__"
    deltaCpp += "#define __DIFFTEST_DELTA_H__"
    deltaCpp += "#include \"diffstate.h\""
    deltaCpp +=
      s"""
         |typedef struct {
         |  ${deltaDecl.mkString("\n  ")}
         |} DeltaState;
         |""".stripMargin

    def deltaSync(dst: String, src: String): Seq[String] = {
      deltaInsts.map { inst =>
        val name = inst.desiredCppName
        s"memcpy(&($dst->$name), $src->${name}_elem, sizeof(${inst.desiredModuleName}));"
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

class DeltaEndpoint(bundles: Seq[Valid[DifftestBundle]], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val deltas = in.filter(_.bits.supportsDelta).flatMap { v_gen =>
//    val hasLimit = v_gen.bits.deltaValidLimit.isDefined
//    val gen_elem = VecInit(v_gen.bits.dataElements.flatMap(_._3))
//    val income_elem = if (hasLimit) {
//      val isInitDelta = RegInit(true.B)
//      when(v_gen.valid) {
//        isInitDelta := false.B
//      }
//      val initBuf = RegEnable(gen_elem, isInitDelta && v_gen.valid)
//
//    } else {
//      gen_elem
//    }
//    val elemSize = v_gen.bits.dataElements.flatMap(_._3).size
//    val limitSize = v_gen.bits.deltaValidLimit.getOrElse(elemSize)
//    val inInit = if (limitSize != elemSize) {
//      val initNum = (elemSize + limitSize - 1) / limitSize
//      println(elemSize +" "+ limitSize + " " + initNum)
//      val isFirstDelta = RegInit(true.B)
//      when(v_gen.valid) {
//        isFirstDelta := false.B
//      }
//      val initCnt = RegEnable(VecInit.fill(initNum)(1.U).asUInt, 0.U, v_gen.valid && isFirstDelta)
//      when(initCnt =/= 0.U) {
//        initCnt := initCnt >> 1
//      }
//      assert(!(initCnt.orR && v_gen.valid))
//      initCnt.orR || (v_gen.valid && isFirstDelta)
//    } else {
//      false.B
//    }
    // For preg state, filter with RenameTable
    def getPregFilter(suffix: String, idx: Int): Bool = {
      def getRatValue(gen: DifftestBundle, isVec: Boolean): Vec[UInt] = {
        if (isVec) {
          gen.bits.asInstanceOf[ArchVecRenameTable].value
        } else {
          gen.bits.asInstanceOf[ArchRenameTable].value
        }
      }
      if (v_gen.bits.desiredCppName == "pregs_" + suffix) {
        val rat = in.filter(_.bits.desiredCppName == "rat_" + suffix)
        VecInit(rat.map{r =>
          r.valid && r.bits.coreid === v_gen.bits.coreid &&
            VecInit(getRatValue(r.bits, suffix == "vec").map{v => v === idx.U}).asUInt.orR
        }).asUInt.orR
      } else {
        false.B
      }
    }
    v_gen.bits.dataElements.flatMap(_._3).zipWithIndex.map { case (data, idx) =>
      val state = RegInit(0.U.asTypeOf(data))
      val cppName = v_gen.bits.desiredCppName
      val filter = if (cppName.contains("pregs")) {
        VecInit(Seq("int", "fp", "vec").map{s => getPregFilter(s, idx)}).asUInt.orR
      } else {
        true.B
      }
      val update = v_gen.valid && data =/= state && filter
      when(update) {
        state := data
      }
//      val initDelay = idx / limitSize + 1
      val elem = Wire(Valid(new DiffDeltaElem(v_gen.bits)))
//      elem.valid := Mux(inInit, Delayer(v_gen.valid, initDelay), update)
      elem.bits.coreid := v_gen.bits.coreid
      elem.bits.index := idx.U
      elem.valid := update
      elem.bits.data := data
//      elem.bits.data := Mux(inInit, Delayer(data, initDelay), data)
      elem
    }
  }
  val withDeltas = MixedVecInit((in.filterNot(_.bits.supportsDelta) ++ deltas).toSeq)
  val out = IO(Output(chiselTypeOf(withDeltas)))
  out := withDeltas
}
