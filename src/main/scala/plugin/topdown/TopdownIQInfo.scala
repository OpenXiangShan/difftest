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

class TopdownIQInfo extends Bundle {
  val valid = Bool()
  val robIdx = UInt(16.W)
  val robFlag = Bool()
  val pipeNum = UInt(8.W)
  val cancelSource = UInt(3.W)
  val srcReady = Bool()
  val futype = UInt(8.W) // not one-hot
  val issued = Bool()
}

class TopdownExtendedIQInfo extends Bundle {
  val idealIssueTime = Bool()
}

class TopdownInfo(val entriesNum: Int) extends Bundle {
  val in = Input(Vec(entriesNum, new TopdownIQInfo))
  val out = Output(Vec(entriesNum, new TopdownExtendedIQInfo))
}

class TopdownIQInfoHelper(val entriesNum: Int)
  extends BlackBox(
    Map(
      "ENTRY_NUM" -> entriesNum,
      "INFO_WIDTH" -> TopdownDPI.iqInfoWidth,
      "OUT_WIDTH" -> TopdownDPI.extendedIQInfoWidth,
    )
  )
  with HasBlackBoxInline {
  private val inWidth = entriesNum * TopdownDPI.iqInfoWidth
  private val outWidth = entriesNum * TopdownDPI.extendedIQInfoWidth
  private val inPaddedWidth = TopdownDPI.gsimPaddedWidth(inWidth)
  private val outPaddedWidth = TopdownDPI.gsimPaddedWidth(outWidth)

  val io = IO(new Bundle {
    val clock = Input(Clock())
    val in = Input(UInt(inPaddedWidth.W))
    val out = Output(UInt(outPaddedWidth.W))
  })

  override def desiredName: String = s"TopdownIQInfoHelper_$entriesNum"

  private val dpiFuncName: String = s"topdown_iq_info_dpic_$entriesNum"

  setInline(
    s"$desiredName.v",
    TopdownDPI.iqHelperVerilog(desiredName, dpiFuncName, inPaddedWidth, outPaddedWidth),
  )

  private val cppExtModule =
    s"""
       |extern "C" void topdown_iq_info_dpic(
       |  unsigned int entries_num,
       |  const uint32_t *in_bits,
       |  uint32_t *out_bits
       |);
       |
       |void $desiredName(
       |  int ENTRY_NUM,
       |  int INFO_WIDTH,
       |  int OUT_WIDTH,
       |  unsigned _BitInt($inPaddedWidth) in,
       |  unsigned _BitInt($outPaddedWidth)& out
       |) {
       |  constexpr int in_words = $inPaddedWidth / 32;
       |  constexpr int out_words = $outPaddedWidth / 32;
       |  uint32_t in_bits[in_words];
       |  uint32_t out_bits[out_words] = {};
       |  std::memcpy(in_bits, &in, sizeof(in_bits));
       |  topdown_iq_info_dpic(ENTRY_NUM, in_bits, out_bits);
       |  std::memcpy(&out, out_bits, sizeof(out_bits));
       |}
       |""".stripMargin
  difftest.DifftestModule.createCppExtModule(desiredName, cppExtModule, Some("<cstring>"))
}

class TopdownIQInfoCollect(val entriesNum: Int) extends Module {
  val io = IO(new TopdownInfo(entriesNum))

  val helper = Module(new TopdownIQInfoHelper(entriesNum))

  helper.io.clock := clock
  val packedIn = VecInit(io.in.map(info => TopdownIQInfoDPI.from(info))).asUInt
  helper.io.in := packedIn.pad(helper.io.in.getWidth)
  val packedOut = helper.io.out(entriesNum * TopdownDPI.extendedIQInfoWidth - 1, 0)
  val out = packedOut.asTypeOf(Vec(entriesNum, new TopdownExtendedIQInfoDPI))
  io.out := VecInit(out.map(_.toTopdown))
}
