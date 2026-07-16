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

  private val inWidth = IQEntriesNum * TopdownDPI.robInfoWidth
  private val outWidth = RobEntriesNum * TopdownDPI.robInfoWidth
  private val inPaddedWidth = TopdownDPI.gsimPaddedWidth(inWidth)
  private val outPaddedWidth = TopdownDPI.gsimPaddedWidth(outWidth)
  private val cppExtModule =
    s"""
       |extern "C" void topdown_rob_info_dpic(
       |  unsigned int iq_entries_num,
       |  unsigned int rob_entries_num,
       |  const uint32_t *in_bits,
       |  uint32_t *out_bits
       |);
       |
       |void $desiredName(
       |  int INFO_WIDTH,
       |  int IQ_ENTRY_NUM,
       |  int ROB_ENTRY_NUM,
       |  ${TopdownDPI.gsimBitIntType(inWidth)} in,
       |  ${TopdownDPI.gsimBitIntType(outWidth)}& out
       |) {
       |  constexpr int in_words = ($inPaddedWidth + 31) / 32;
       |  constexpr int out_words = ($outPaddedWidth + 31) / 32;
       |  uint32_t in_bits[in_words];
       |  uint32_t out_bits[out_words] = {};
       |  std::memcpy(in_bits, &in, sizeof(in_bits));
       |  topdown_rob_info_dpic(IQ_ENTRY_NUM, ROB_ENTRY_NUM, in_bits, out_bits);
       |  std::memcpy(&out, out_bits, sizeof(out_bits));
       |}
       |""".stripMargin
  difftest.DifftestModule.createCppExtModule(desiredName, cppExtModule, Some("<cstring>"))
}

class TopdownRobInfoCollect(val IQEntriesNum: Int, val RobEntriesNum: Int) extends Module {
  val io = IO(new TopdownRobInfoBundle(IQEntriesNum, RobEntriesNum))

  val helper = Module(new TopdownRobInfoHelper(IQEntriesNum, RobEntriesNum))

  helper.io.in := VecInit(io.in.map(info => TopdownRobInfoDPI.from(info))).asUInt
  val out = helper.io.out.asTypeOf(Vec(RobEntriesNum, new TopdownRobInfoDPI))
  io.out := VecInit(out.map(_.toTopdown))
}
