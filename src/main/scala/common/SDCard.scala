/***************************************************************************************
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
*
* XiangShan is licensed under Mulan PSL v2.
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

class DifftestSDCardRead extends Bundle {
  val setAddr = Input(Bool())
  val addr = Input(UInt(32.W))
  val ren = Input(Bool())
  val data = Output(UInt(32.W))
}

class SDCardHelper extends ExtModule with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val io = IO(new DifftestSDCardRead)

  val cppExtModule =
    """
      |void SDCardHelper (
      |  uint8_t   io_setAddr,
      |  uint32_t  io_addr,
      |  uint8_t   io_ren,
      |  uint32_t& io_data
      |) {
      |  if (io_ren) sd_read(&io_data);
      |  if (io_setAddr) sd_setaddr(io_addr);
      |}
      |""".stripMargin
  difftest.DifftestModule.createCppExtModule("SDCardHelper", cppExtModule, Some("\"sdcard.h\""))

  setInline(
    "SDCardHelper.v",
    s"""
       |`ifndef SYNTHESIS
       |import "DPI-C" function void sd_setaddr(input int addr);
       |import "DPI-C" function void sd_read(output int data);
       |`endif // SYNTHESIS
       |
       |module SDCardHelper (
       |  input clock,
       |  input io_setAddr,
       |  input [31:0] io_addr,
       |  input io_ren,
       |  output reg [31:0] io_data
       |);
       |
       |`ifndef SYNTHESIS
       |  always@(negedge clock) begin
       |    if (io_ren) sd_read(io_data);
       |  end
       |  always@(posedge clock) begin
       |    if (io_setAddr) sd_setaddr(io_addr);
       |  end
       |`endif // SYNTHESIS
       |
       |endmodule
     """.stripMargin,
  )
}

class DifftestSDCard extends Module {
  val io = IO(new DifftestSDCardRead)

  val helper = Module(new SDCardHelper)
  helper.clock := clock

  io <> helper.io
}

object DifftestSDCard {
  def apply(): DifftestSDCardRead = {
    Module(new DifftestSDCard).io
  }
}
