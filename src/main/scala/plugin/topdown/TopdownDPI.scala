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

private[topdown] class TopdownIQInfoDPI extends Bundle {
  // Bundle.asUInt places earlier fields at higher bits. Keep declarations in
  // reverse C struct order so low bits match the packed C++ frame layout.
  val issued = UInt(8.W)
  val futype = UInt(8.W)
  val srcReady = UInt(8.W)
  val cancelSource = UInt(8.W)
  val pipeNum = UInt(8.W)
  val robFlag = UInt(8.W)
  val robIdx = UInt(16.W)
  val valid = UInt(8.W)
}

private[topdown] object TopdownIQInfoDPI {
  def from(info: TopdownIQInfo): TopdownIQInfoDPI = {
    val dpi = Wire(new TopdownIQInfoDPI)
    TopdownDPI.connectToDPIFrame(dpi, info, TopdownDPI.iqInfoFields)
    dpi
  }
}

private[topdown] class TopdownExtendedIQInfoDPI extends Bundle {
  val idealIssueTime = UInt(8.W)

  def toTopdown: TopdownExtendedIQInfo = {
    val info = Wire(new TopdownExtendedIQInfo)
    TopdownDPI.connectFromDPIFrame(info, this, TopdownDPI.extendedIQInfoFields)
    info
  }
}

private[topdown] class TopdownRobInfoDPI extends Bundle {
  val idealIssueTime = UInt(8.W)
  val issued = UInt(8.W)
  val cancelSource = UInt(8.W)
  val robFlag = UInt(8.W)
  val robIdx = UInt(16.W)
  val valid = UInt(8.W)

  def toTopdown: TopdownRobInfo = {
    val info = Wire(new TopdownRobInfo)
    TopdownDPI.connectFromDPIFrame(info, this, TopdownDPI.robInfoFields)
    info
  }
}

private[topdown] object TopdownRobInfoDPI {
  def from(info: TopdownRobInfo): TopdownRobInfoDPI = {
    val dpi = Wire(new TopdownRobInfoDPI)
    TopdownDPI.connectToDPIFrame(dpi, info, TopdownDPI.robInfoFields)
    dpi
  }
}

private[topdown] object TopdownDPI {
  val iqInfoWidth: Int = (new TopdownIQInfoDPI).getWidth
  val extendedIQInfoWidth: Int = (new TopdownExtendedIQInfoDPI).getWidth
  val robInfoWidth: Int = (new TopdownRobInfoDPI).getWidth

  def gsimPaddedWidth(width: Int): Int = ((width + 63) / 64) * 64

  private val commonInfoFields = Seq("valid", "robIdx", "robFlag", "cancelSource", "issued")
  val iqInfoFields: Seq[String] = commonInfoFields ++ Seq("pipeNum", "srcReady", "futype")
  val extendedIQInfoFields: Seq[String] = Seq("idealIssueTime")
  val robInfoFields: Seq[String] = commonInfoFields ++ extendedIQInfoFields

  def connectToDPIFrame(dst: Record, src: Record, fields: Seq[String]): Unit = {
    fields.foreach { name =>
      dst.elements(name) := src.elements(name).asUInt
    }
  }

  def connectFromDPIFrame(dst: Record, src: Record, fields: Seq[String]): Unit = {
    fields.foreach { name =>
      val srcField = src.elements(name).asUInt
      dst.elements(name) match {
        case bool: Bool => bool := srcField =/= 0.U
        case uint: UInt => uint := srcField
        case other =>
          throw new IllegalArgumentException(s"Unsupported TopDown DPI field $name: ${other.getClass.getName}")
      }
    }
  }

  def iqHelperVerilog(
    moduleName: String,
    dpiFuncName: String,
    inPaddedWidth: Int,
    outPaddedWidth: Int,
  ): String =
    s"""
       |module $moduleName #(
       |  parameter integer ENTRY_NUM = 1,
       |  parameter integer INFO_WIDTH = $iqInfoWidth,
       |  parameter integer OUT_WIDTH = $extendedIQInfoWidth
       |) (
       |  input                         clock,
       |  input  [${inPaddedWidth - 1}:0] in,
       |  output reg [${outPaddedWidth - 1}:0] out
       |);
       |
       |  wire _unused_clock = clock;
       |  reg [ENTRY_NUM*OUT_WIDTH-1:0] dpi_out;
       |
       |`ifndef SYNTHESIS
       |  import "DPI-C" function void $dpiFuncName(
       |    input  bit [ENTRY_NUM*INFO_WIDTH-1:0] in,
       |    output bit [ENTRY_NUM*OUT_WIDTH-1:0] out
       |  );
       |`endif // SYNTHESIS
       |
       |  always @(*) begin
       |    /* verilator lint_off WIDTHCONCAT */
       |    out = '0;
       |    dpi_out = '0;
       |    /* verilator lint_on WIDTHCONCAT */
       |`ifndef SYNTHESIS
       |    $dpiFuncName(
       |      in[ENTRY_NUM*INFO_WIDTH-1:0],
       |      dpi_out
       |    );
       |`endif // SYNTHESIS
       |    out[ENTRY_NUM*OUT_WIDTH-1:0] = dpi_out;
       |  end
       |
       |endmodule
       |""".stripMargin

  def robHelperVerilog(
    moduleName: String,
    dpiFuncName: String,
    inPaddedWidth: Int,
    outPaddedWidth: Int,
  ): String =
    s"""
       |module $moduleName #(
       |  parameter integer IQ_ENTRY_NUM = 1,
       |  parameter integer ROB_ENTRY_NUM = 1,
       |  parameter integer INFO_WIDTH = $robInfoWidth
       |) (
       |  input  [${inPaddedWidth - 1}:0] in,
       |  output reg [${outPaddedWidth - 1}:0] out
       |);
       |
       |  reg [ROB_ENTRY_NUM*INFO_WIDTH-1:0] dpi_out;
       |
       |`ifndef SYNTHESIS
       |  import "DPI-C" function void $dpiFuncName(
       |    input  bit [IQ_ENTRY_NUM*INFO_WIDTH-1:0] in,
       |    output bit [ROB_ENTRY_NUM*INFO_WIDTH-1:0] out
       |  );
       |`endif // SYNTHESIS
       |
       |  always @(*) begin
       |    /* verilator lint_off WIDTHCONCAT */
       |    out = '0;
       |    dpi_out = '0;
       |    /* verilator lint_on WIDTHCONCAT */
       |`ifndef SYNTHESIS
       |    $dpiFuncName(
       |      in[IQ_ENTRY_NUM*INFO_WIDTH-1:0],
       |      dpi_out
       |    );
       |`endif // SYNTHESIS
       |    out[ROB_ENTRY_NUM*INFO_WIDTH-1:0] = dpi_out;
       |  end
       |
       |endmodule
       |""".stripMargin
}
