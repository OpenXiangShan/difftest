/***************************************************************************************
 * Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
 * Copyright (c) 2026 Beijing Institute of Open Source Chip
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

package difftest.plugin.topdown

import chisel3._
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

class TopdownRobInfoHelper(val IQEntriesNum: Int, val RobEntriesNum: Int)
  extends BlackBox(
    Map("IQ_ENTRY_NUM" -> IQEntriesNum, "ROB_ENTRY_NUM" -> RobEntriesNum, "INFO_WIDTH" -> TopdownDPI.robInfoWidth)
  )
  with HasBlackBoxInline {
  val io = IO(new Bundle {
    val in = Input(UInt((IQEntriesNum * TopdownDPI.robInfoWidth).W))
    val out = Output(UInt((RobEntriesNum * TopdownDPI.robInfoWidth).W))
  })

  override def desiredName: String = s"TopdownRobInfoHelper_${IQEntriesNum}_${RobEntriesNum}"

  private val dpiFuncName: String = s"topdown_rob_info_dpic_${IQEntriesNum}_${RobEntriesNum}"

  setInline(s"$desiredName.v", TopdownDPI.robHelperVerilog(desiredName, dpiFuncName))
}

class TopdownRobInfoCollect(val IQEntriesNum: Int, val RobEntriesNum: Int) extends Module {
  val io = IO(new TopdownRobInfoBundle(IQEntriesNum, RobEntriesNum))

  val helper = Module(new TopdownRobInfoHelper(IQEntriesNum, RobEntriesNum))

  helper.io.in := VecInit(io.in.map(info => TopdownRobInfoDPI.from(info))).asUInt
  val out = helper.io.out.asTypeOf(Vec(RobEntriesNum, new TopdownRobInfoDPI))
  io.out := VecInit(out.map(_.toTopdown))
}
