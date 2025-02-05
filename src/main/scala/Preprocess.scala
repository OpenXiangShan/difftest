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

object Preprocess {
  def apply(bundles: MixedVec[DifftestBundle]): MixedVec[DifftestBundle] = {
    val module = Module(new PreprocessEndpoint(chiselTypeOf(bundles).toSeq))
    module.in := bundles
    module.out
  }
  def getCommitData(
    bundles: MixedVec[DifftestBundle],
    commits: Seq[DiffInstrCommit],
    wbName: String,
    regName: String,
  ): Seq[UInt] = {
    if (bundles.exists(_.desiredCppName == regName)) {
      if (bundles.exists(_.desiredCppName == wbName)) {
        val numCores = bundles.count(_.isUniqueIdentifier)
        val writeBacks = bundles.filter(_.desiredCppName == wbName).map(_.asInstanceOf[DiffIntWriteback])
        val phyRf = Reg(Vec(numCores, Vec(writeBacks.head.numElements, UInt(64.W))))
        for (wb <- writeBacks) {
          when(wb.valid) {
            phyRf(wb.coreid)(wb.address) := wb.data
          }
        }
        commits.map { c =>
          val data = WireInit(phyRf(c.coreid)(c.wpdest))
          for (wb <- writeBacks) { // Consider WriteBack valid in same cycle
            when(wb.valid && wb.coreid === c.coreid && wb.address === c.wpdest) {
              data := wb.data
            }
          }
          data
        }
      } else {
        val archRf = VecInit(bundles.filter(_.desiredCppName == regName).map(_.asInstanceOf[ArchIntRegState]).toSeq)
        commits.map { c => archRf(c.coreid).value(c.wdest) }
      }
    } else {
      Seq.fill(commits.length)(0.U)
    }
  }
}

class PreprocessEndpoint(bundles: Seq[DifftestBundle]) extends Module {
  val in = IO(Input(MixedVec(bundles)))

  // Special fix of writeback for get_commit_data
  // We use physical WriteBack for compare when load and MMIO, and record commit instr trace
  // As there are multiple DUT buffer in software side, writeBacks transferred and used may not in the same buffer
  // So we buffer writeBacks until instrCommit, and submit corresponding data
  val commits = in.filter(_.desiredCppName == "commit").map(_.asInstanceOf[DiffInstrCommit]).toSeq
  val fpData = Preprocess.getCommitData(in, commits, "wb_fp", "regs_fp")
  val vecData = Preprocess.getCommitData(in, commits, "wb_vec", "regs_vec")
  val intData = Preprocess.getCommitData(in, commits, "wb_int", "regs_int")
  val commitData = commits.zip(fpData).zip(vecData).zip(intData).map { case (((c, f), v), i) =>
    val cd = WireInit(0.U.asTypeOf(new DiffCommitData))
    cd.coreid := c.coreid
    cd.index := c.index
    cd.valid := c.valid
    cd.data := Mux(c.fpwen, f, Mux(c.vecwen, v, i))
    cd
  }

  val withCommitData = in.filterNot(_.desiredCppName.contains("wb")) ++ commitData

  // LoadEvent will not be checked when single-core
  val skipLoad = if (in.count(_.isUniqueIdentifier) == 1) {
    withCommitData.filterNot(_.desiredCppName == "load")
  } else {
    withCommitData
  }

  val preprocessed = MixedVecInit(skipLoad.toSeq)
  val out = IO(Output(chiselTypeOf(preprocessed)))
  out := preprocessed
}
