/***************************************************************************************
 * Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
 * Copyright (c) 2020-2021 Peng Cheng Laboratory
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

package difftest.common

import chisel3._
import chisel3.experimental.ExtModule
import chisel3.util._

class TopdownRobInfo extends Bundle {
  val valid = Bool()
  val robIdx = UInt(16.W)
  val robFlag = Bool()
  val cancelSource = UInt(3.W)
  val issued = Bool()
  val idealIssueTime = Bool()
}

class TopdownRobInfoBundle(val IQEntriesNum: Int, val RobEntriesNum: Int) extends Bundle {
  val in = Input(Vec(IQEntriesNum, new TopdownRobInfo))
  val out = Output(Vec(RobEntriesNum, new TopdownRobInfo))
}

object TopdownRobInfoPacking {
  val fieldSpecs: Seq[(String, Int)] = Seq(
    "valid" -> 1,
    "robIdx" -> 16,
    "robFlag" -> 1,
    "cancelSource" -> 3,
    "issued" -> 1,
    "idealIssueTime" -> 1,
  )
}

class TopdownRobInfoPackedBundle(val IQEntriesNum: Int, val RobEntriesNum: Int) extends Bundle {
  import TopdownDPI.packedWidth

  val in_valid = Input(UInt(packedWidth(IQEntriesNum, 1).W))
  val in_robIdx = Input(UInt(packedWidth(IQEntriesNum, 16).W))
  val in_robFlag = Input(UInt(packedWidth(IQEntriesNum, 1).W))
  val in_cancelSource = Input(UInt(packedWidth(IQEntriesNum, 3).W))
  val in_issued = Input(UInt(packedWidth(IQEntriesNum, 1).W))
  val in_idealIssueTime = Input(UInt(packedWidth(IQEntriesNum, 1).W))

  val out_valid = Output(UInt(packedWidth(RobEntriesNum, 1).W))
  val out_robIdx = Output(UInt(packedWidth(RobEntriesNum, 16).W))
  val out_robFlag = Output(UInt(packedWidth(RobEntriesNum, 1).W))
  val out_cancelSource = Output(UInt(packedWidth(RobEntriesNum, 3).W))
  val out_issued = Output(UInt(packedWidth(RobEntriesNum, 1).W))
  val out_idealIssueTime = Output(UInt(packedWidth(RobEntriesNum, 1).W))
}

class TopdownRobInfoHelper(val IQEntriesNum: Int, val RobEntriesNum: Int)
  extends ExtModule
  with HasExtModuleInline
  with TopdownDPI {
  import TopdownDPI._
  import TopdownRobInfoPacking._

  override def desiredName: String = s"TopdownRobInfoHelper_${IQEntriesNum}_$RobEntriesNum"

  protected val dpiFuncName: String = s"topdown_rob_info_${IQEntriesNum}_$RobEntriesNum"
  protected val header: String = "\"topdown_rob_info.h\""
  protected val hasClockPort: Boolean = false

  val io = IO(new TopdownRobInfoPackedBundle(IQEntriesNum, RobEntriesNum))

  protected val inputArgs: Seq[PackedArg] = makePackedArgs("io_in", IQEntriesNum, fieldSpecs)
  protected val outputArgs: Seq[PackedArg] = makePackedArgs("io_out", RobEntriesNum, fieldSpecs)

  protected val wrapperBody: String =
    s"""
       |  TopdownRobInfoFrame in[$IQEntriesNum] = {};
       |  TopdownRobInfoFrame out[$RobEntriesNum] = {};
       |  ${packedBitHelpers(inputArgs ++ outputArgs).mkString("\n  ")}
       |  ${scalarOutputAccums(outputArgs).mkString("\n  ")}
       |  ${wideOutputZeros(outputArgs).mkString("\n  ")}
       |  ${inputAssigns(IQEntriesNum).mkString("\n  ")}
       |  topdown_rob_info_apply($IQEntriesNum, $RobEntriesNum, in, out);
       |  ${outputAssigns(RobEntriesNum).mkString("\n  ")}
       |  ${scalarOutputWrites(outputArgs).mkString("\n  ")}
       |""".stripMargin

  emitTopdownDPI()
}

class TopdownRobInfoCollect(val IQEntriesNum: Int, val RobEntriesNum: Int) extends Module {
  import TopdownDPI._

  val io = IO(new TopdownRobInfoBundle(IQEntriesNum, RobEntriesNum))

  val helper = Module(new TopdownRobInfoHelper(IQEntriesNum, RobEntriesNum))

  helper.io.in_valid := packFields(io.in.map(_.valid.asUInt))
  helper.io.in_robIdx := packFields(io.in.map(_.robIdx))
  helper.io.in_robFlag := packFields(io.in.map(_.robFlag.asUInt))
  helper.io.in_cancelSource := packFields(io.in.map(_.cancelSource))
  helper.io.in_issued := packFields(io.in.map(_.issued.asUInt))
  helper.io.in_idealIssueTime := packFields(io.in.map(_.idealIssueTime.asUInt))

  for (idx <- 0 until RobEntriesNum) {
    io.out(idx).valid := unpackField(helper.io.out_valid, idx, 1).asBool
    io.out(idx).robIdx := unpackField(helper.io.out_robIdx, idx, 16)
    io.out(idx).robFlag := unpackField(helper.io.out_robFlag, idx, 1).asBool
    io.out(idx).cancelSource := unpackField(helper.io.out_cancelSource, idx, 3)
    io.out(idx).issued := unpackField(helper.io.out_issued, idx, 1).asBool
    io.out(idx).idealIssueTime := unpackField(helper.io.out_idealIssueTime, idx, 1).asBool
  }
}
