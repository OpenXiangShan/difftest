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

package difftest.preprocess

import chisel3._
import chisel3.util._
import difftest._
import difftest.gateway.GatewayConfig
import difftest.util.PipelineConnect

object Preprocess {
  def apply(
    bundles: DecoupledIO[MixedVec[DifftestBundle]],
    config: GatewayConfig,
  ): DecoupledIO[MixedVec[DifftestBundle]] = {
    val module = Module(new PreprocessEndpoint(chiselTypeOf(bundles.bits).toSeq, config))
    module.in <> bundles
    module.out
  }

  def getArchRegs(bundles: Seq[DifftestBundle], isHardware: Boolean): Seq[ArchRegState with DifftestBundle] = {
    bundles.collect { case p: DiffPhyRegState => p }
      .groupBy(_.desiredCppName)
      .flatMap { case (name, pregs) =>
        val archTarget = pregs.head.archTarget
        val ratTarget = pregs.head.ratTarget
        require(!bundles.exists(_.isInstanceOf[archTarget.type]))
        if (isHardware) {
          val needRat = pregs.head.needRat
          val rats = bundles.collect {
            case rat: DiffArchRenameTable if rat.desiredCppName == ratTarget.desiredCppName => rat
          }
          require((needRat && rats.length == pregs.length) || (!needRat && rats.isEmpty))
          pregs.zipWithIndex.map { case (preg, idx) =>
            val archReg = Wire(archTarget)
            archReg.coreid := preg.coreid
            if (needRat) {
              val rat = rats(idx)
              require(rat.numPhyRegs == preg.numPhyRegs)
              archReg.value.zipWithIndex.foreach { case (data, vid) =>
                data := preg.value(rat.value(vid))
              }
            } else {
              archReg.value := preg.value
            }
            archReg
          }
        } else {
          Seq.fill(pregs.length)(archTarget)
        }
      }
      .toSeq
  }
  // Replace PhyReg + Rename with ArchReg + CommitData/VecCommitData
  def replaceRegs(bundles: Seq[DifftestBundle]): Seq[DifftestBundle] = {
    def getBundle[T <: DifftestBundle](name: String): Seq[T] =
      bundles.filter(_.desiredCppName == name).asInstanceOf[Seq[T]]

    val numCores = bundles.count(_.isUniqueIdentifier)
    val archRegs = getArchRegs(bundles, true)

    val commits = getBundle[DiffInstrCommit]("commit")
    val phyInts = getBundle[DiffPhyIntRegState]("pregs_xrf")
    val phyFps = getBundle[DiffPhyFpRegState]("pregs_frf")
    val phyVecs = getBundle[DiffPhyVecRegState]("pregs_vrf")
    val commitDatas = commits.zipWithIndex.flatMap { case (c, idx) =>
      val coreID = idx / (commits.length / numCores)
      val intData = phyInts(coreID).value(c.wpdest)
      val fpData = if (phyFps.nonEmpty) phyFps(coreID).value(c.wpdest) else 0.U
      val cd = Wire(new DiffCommitData)
      cd.coreid := c.coreid
      cd.index := c.index
      cd.valid := c.valid && (c.rfwen || c.fpwen)
      cd.data := Mux(c.fpwen, fpData, intData)
      // Also skip vec_commit_data (used in vec_load check) for single core
      val vcd = Option.when(phyVecs.nonEmpty && numCores > 1) {
        val gen = Wire(new DiffVecCommitData)
        gen.coreid := c.coreid
        gen.index := c.index
        gen.valid := c.valid && (c.v0wen || c.vecwen)
        gen.data := c.otherwpdest.map { wpdest =>
          phyVecs(coreID).value(wpdest)
        }
        gen
      }
      Seq(cd) ++ vcd.toSeq
    }

    bundles.filterNot(b => Seq("pregs_", "rat_").exists(s => b.desiredCppName.contains(s))) ++ archRegs ++ commitDatas
  }
}

class PreprocessEndpoint(bundles: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Flipped(Decoupled(MixedVec(bundles))))
  val pipelined = Wire(Decoupled(MixedVec(bundles)))
  PipelineConnect(in, pipelined, pipelined.fire)

  val replaceReg = if (!config.softArchUpdate && pipelined.bits.exists(_.desiredCppName == "pregs_xrf")) {
    // extract ArchReg in Hardware
    Preprocess.replaceRegs(pipelined.bits)
  } else {
    pipelined.bits
  }

  // LoadEvent will not be checked when single-core
  val skipLoad = if (replaceReg.count(_.isUniqueIdentifier) == 1) {
    replaceReg.filterNot(_.desiredCppName == "load")
  } else {
    replaceReg
  }

  val preprocessed = MixedVecInit(skipLoad.toSeq)
  val out = IO(Decoupled(chiselTypeOf(preprocessed)))
  pipelined.ready := out.ready
  out.valid := pipelined.valid
  out.bits := preprocessed
}
