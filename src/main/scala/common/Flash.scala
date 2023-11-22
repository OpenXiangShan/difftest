/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
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

class DifftestFlashRead extends Bundle {
  val en = Input(Bool())
  val addr = Input(UInt(31.W))
  val data = Output(UInt(64.W))
}

class FlashHelper extends ExtModule with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val r = IO(new DifftestFlashRead)

  setInline("FlashHelper.v",
    s"""
       |import "DPI-C" function void flash_read
       |(
       |  input int addr,
       |  output longint data
       |);
       |
       |module FlashHelper (
       |  input clock,
       |  input r_en,
       |  input [31:0] r_addr,
       |  output reg [63:0] r_data
       |);
       |
       |  always @(posedge clock) begin
       |    if (r_en) flash_read(r_addr, r_data);
       |  end
       |
       |endmodule
     """.stripMargin)
}

class DifftestFlash extends Module {
  val io = IO(new DifftestFlashRead)

  val helper = Module(new FlashHelper)
  helper.clock := clock

  io <> helper.r
}

object DifftestFlash {
  def apply(): DifftestFlashRead = {
    Module(new DifftestFlash).io
  }
}
