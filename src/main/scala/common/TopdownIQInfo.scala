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

object TopdownIQInfoPacking {
  val inputFieldSpecs: Seq[(String, Int)] = Seq(
    "valid" -> 1,
    "robIdx" -> 16,
    "robFlag" -> 1,
    "pipeNum" -> 8,
    "cancelSource" -> 3,
    "srcReady" -> 1,
    "futype" -> 8,
    "issued" -> 1,
  )

  val outputFieldSpecs: Seq[(String, Int)] = Seq(
    "idealIssueTime" -> 1
  )
}

class TopdownIQInfoPackedBundle(val entriesNum: Int) extends Bundle {
  import TopdownDPI.packedWidth

  val in_valid = Input(UInt(packedWidth(entriesNum, 1).W))
  val in_robIdx = Input(UInt(packedWidth(entriesNum, 16).W))
  val in_robFlag = Input(UInt(packedWidth(entriesNum, 1).W))
  val in_pipeNum = Input(UInt(packedWidth(entriesNum, 8).W))
  val in_cancelSource = Input(UInt(packedWidth(entriesNum, 3).W))
  val in_srcReady = Input(UInt(packedWidth(entriesNum, 1).W))
  val in_futype = Input(UInt(packedWidth(entriesNum, 8).W))
  val in_issued = Input(UInt(packedWidth(entriesNum, 1).W))

  val out_idealIssueTime = Output(UInt(packedWidth(entriesNum, 1).W))
}

class TopdownIQInfoHelper(val entriesNum: Int) extends ExtModule with HasExtModuleInline with TopdownDPI {
  import TopdownDPI._
  import TopdownIQInfoPacking._

  override def desiredName: String = s"TopdownIQInfoHelper_$entriesNum"

  protected val dpiFuncName: String = s"topdown_iq_info_$entriesNum"
  protected val header: String = "\"topdown_iq_info.h\""
  protected val hasClockPort: Boolean = true

  val clock = IO(Input(Clock()))
  val io = IO(new TopdownIQInfoPackedBundle(entriesNum))

  protected val inputArgs: Seq[PackedArg] = makePackedArgs("io_in", entriesNum, inputFieldSpecs)
  protected val outputArgs: Seq[PackedArg] = makePackedArgs("io_out", entriesNum, outputFieldSpecs)

  protected val wrapperBody: String =
    s"""
       |  TopdownIQInfoFrame in[$entriesNum] = {};
       |  TopdownExtendedIQInfoFrame out[$entriesNum] = {};
       |  ${packedBitHelpers(inputArgs ++ outputArgs).mkString("\n  ")}
       |  ${scalarOutputAccums(outputArgs).mkString("\n  ")}
       |  ${wideOutputZeros(outputArgs).mkString("\n  ")}
       |  ${inputAssigns(entriesNum).mkString("\n  ")}
       |  topdown_iq_info_apply($entriesNum, in, out);
       |  ${outputAssigns(entriesNum).mkString("\n  ")}
       |  ${scalarOutputWrites(outputArgs).mkString("\n  ")}
       |""".stripMargin

  emitTopdownDPI()
}

class TopdownIQInfoCollect(val entriesNum: Int) extends Module {
  import TopdownDPI._

  val io = IO(new TopdownInfo(entriesNum))

  val helper = Module(new TopdownIQInfoHelper(entriesNum))
  helper.clock := clock

  helper.io.in_valid := packFields(io.in.map(_.valid.asUInt))
  helper.io.in_robIdx := packFields(io.in.map(_.robIdx))
  helper.io.in_robFlag := packFields(io.in.map(_.robFlag.asUInt))
  helper.io.in_pipeNum := packFields(io.in.map(_.pipeNum))
  helper.io.in_cancelSource := packFields(io.in.map(_.cancelSource))
  helper.io.in_srcReady := packFields(io.in.map(_.srcReady.asUInt))
  helper.io.in_futype := packFields(io.in.map(_.futype))
  helper.io.in_issued := packFields(io.in.map(_.issued.asUInt))

  for (idx <- 0 until entriesNum) {
    io.out(idx).idealIssueTime := unpackField(helper.io.out_idealIssueTime, idx, 1).asBool
  }
}
