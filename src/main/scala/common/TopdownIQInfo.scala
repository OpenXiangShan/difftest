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
}

class TopdownExtendedIQInfo extends Bundle {
  val idealIssueTime = Bool()
}

class TopdownInfo(val entriesNum: Int) extends Bundle {
  val in = Input(Vec(entriesNum, new TopdownIQInfo))
  val out = Output(Vec(entriesNum, new TopdownExtendedIQInfo))
}

class TopdownIQInfoHelper(val entriesNum: Int) extends ExtModule with HasExtModuleInline {

  override def desiredName: String = s"TopdownIQInfoHelper_${entriesNum}"

  private val dpiFuncName = s"topdown_iq_info_${entriesNum}"

  val clock = IO(Input(Clock()))
  val io = IO(new TopdownInfo(entriesNum))

  private def flattenLeaves(prefix: String, data: Data): Seq[(String, Data)] = data match {
    case bundle: Bundle =>
      bundle.elements.toSeq.reverse.flatMap { case (name, elem) =>
        flattenLeaves(s"${prefix}_$name", elem)
      }
    case vector: Vec[_] =>
      vector.zipWithIndex.flatMap { case (elem, idx) =>
        flattenLeaves(s"${prefix}_$idx", elem)
      }
    case uint: UInt =>
      Seq(prefix -> uint)
    case other =>
      throw new Exception(s"Unsupported data type: $other")
  }

  private def cType(data: Data): String = data.getWidth match {
    case 1                                  => "uint8_t"
    case width if width > 1 && width <= 8   => "uint8_t"
    case width if width > 8 && width <= 16  => "uint16_t"
    case width if width > 16 && width <= 32 => "uint32_t"
    case width if width > 32 && width <= 64 => "uint64_t"
    case width                              => throw new Exception(s"Unsupported C width: $width")
  }

  private def svType(data: Data): String = data.getWidth match {
    case 1                                  => "bit"
    case width if width > 1 && width <= 8   => "byte"
    case width if width > 8 && width <= 16  => "shortint"
    case width if width > 16 && width <= 32 => "int"
    case width if width > 32 && width <= 64 => "longint"
    case width                              => throw new Exception(s"Unsupported SV width: $width")
  }

  private def modulePortArg(name: String, data: Data, isOutput: Boolean): String = {
    val direction = if (isOutput) "output reg" else "input"
    val widthString = if (data.getWidth == 1) "      " else f"[${data.getWidth - 1}%2d:0]"
    Seq(direction, widthString, name).mkString(" ")
  }

  private def dpiArg(name: String, data: Data, isOutput: Boolean): String = {
    val direction = if (isOutput) "output" else "input"
    f"$direction ${svType(data)}%8s $name"
  }

  private def cppArg(name: String, data: Data, isOutput: Boolean): String = {
    val refPrefix = if (isOutput) "&" else ""
    f"${cType(data)}%-8s $refPrefix$name"
  }

  private val inputLeafPorts = (0 until entriesNum).flatMap(i => flattenLeaves(s"io_in_$i", io.in(i)))
  private val outputLeafPorts = (0 until entriesNum).flatMap(i => flattenLeaves(s"io_out_$i", io.out(i)))
  private val cppArgs = inputLeafPorts.map { case (name, data) => cppArg(name, data, isOutput = false) } ++
    outputLeafPorts.map { case (name, data) => cppArg(name, data, isOutput = true) }
  private val dpiArgs = inputLeafPorts.map { case (name, data) => dpiArg(name, data, isOutput = false) } ++
    outputLeafPorts.map { case (name, data) => dpiArg(name, data, isOutput = true) }
  private val modulePorts = Seq("input clock") ++
    inputLeafPorts.map { case (name, data) => modulePortArg(name, data, isOutput = false) } ++
    outputLeafPorts.map { case (name, data) => modulePortArg(name, data, isOutput = true) }
  private val dpiCallArgs = (inputLeafPorts ++ outputLeafPorts).map(_._1)
  private val synthDefaults = outputLeafPorts.map { case (name, data) =>
    if (data.getWidth == 1) s"$name = 1'b0;" else s"$name = ${data.getWidth}'b0;"
  }
  private val inputFieldNames = (new TopdownIQInfo).elements.toSeq.reverse.map(_._1)
  private val outputFieldNames = (new TopdownExtendedIQInfo).elements.toSeq.reverse.map(_._1)
  private val inputAssigns = (0 until entriesNum).flatMap { idx =>
    inputFieldNames.map(field => s"in[$idx].$field = io_in_${idx}_$field;")
  }
  private val outputAssigns = (0 until entriesNum).flatMap { idx =>
    outputFieldNames.map(field => s"io_out_${idx}_$field = out[$idx].$field;")
  }

  private val wrapperBody =
    s"""
      |  TopdownIQInfoFrame in[$entriesNum] = {};
      |  TopdownExtendedIQInfoFrame out[$entriesNum] = {};
      |  ${inputAssigns.mkString("\n  ")}
      |  topdown_iq_info_apply($entriesNum, in, out);
      |  ${outputAssigns.mkString("\n  ")}
      |""".stripMargin

  private val cppExtModule =
    s"""
      |extern "C" void $dpiFuncName (
      |  ${cppArgs.mkString(",\n  ")}
      |) {
      |$wrapperBody
      |}
      |
      |void $desiredName (
      |  ${cppArgs.mkString(",\n  ")}
      |) {
      |  $dpiFuncName(
      |    ${dpiCallArgs.mkString(",\n    ")}
      |  );
      |}
      |""".stripMargin
  difftest.DifftestModule.createCppExtModule(desiredName, cppExtModule, Some("\"topdown_iq_info.h\""))

  setInline(
    s"$desiredName.v",
    s"""
      |`ifndef SYNTHESIS
      |import "DPI-C" function void $dpiFuncName(
      |  ${dpiArgs.mkString(",\n  ")}
      |);
      |`endif // SYNTHESIS
      |
      |module $desiredName (
      |  ${modulePorts.mkString(",\n  ")}
      |);
      |
      |`ifdef SYNTHESIS
      |  always @(*) begin
      |    ${synthDefaults.mkString("\n    ")}
      |  end
      |`else
      |  always @(posedge clock) begin
      |    $dpiFuncName(
      |      ${dpiCallArgs.mkString(",\n      ")}
      |    );
      |  end
      |`endif // SYNTHESIS
      |
      |endmodule
     """.stripMargin,
  )
}

class TopdownIQInfoCollect(val entriesNum: Int) extends Module {
  val io = IO(new TopdownInfo(entriesNum))

  val helper = Module(new TopdownIQInfoHelper(entriesNum))
  helper.clock := clock

  io <> helper.io
}