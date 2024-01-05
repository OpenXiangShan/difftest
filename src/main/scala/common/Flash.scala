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
       |`ifndef SYNTHESIS
       |import "DPI-C" function void flash_read
       |(
       |  input int addr,
       |  output longint data
       |);
       |`endif // SYNTHESIS
       |
       |module FlashHelper (
       |  input clock,
       |  input r_en,
       |  input [31:0] r_addr,
       |  output reg [63:0] r_data
       |);
       |
       |`ifndef SYNTHESIS
       |  always @(posedge clock) begin
       |    if (r_en) flash_read(r_addr, r_data);
       |  end
       |`else
       |  // 1K entries. 8KB size.
       |  `define FLASH_SIZE (8 * 1024)
       |  reg [7:0] flash_mem [0 : `FLASH_SIZE - 1];
       |
       |  for (genvar i = 0; i < 8; i++) begin
       |    always @(posedge clk) begin
       |      if (ren) r_data[8 * i + 7 : 8 * i] <= flash_mem[r_addr + i];
       |    end
       |  end
       |
       |`ifdef FLASH_IMAGE
       |  integer flash_image, n_read;
       |  // Create string-type FLASH_IMAGE
       |  `define STRINGIFY(x) `"x`"
       |  `define FLASH_IMAGE_S `STRINGIFY(`FLASH_IMAGE)
       |`endif // FLASH_IMAGE
       |
       |  initial begin
       |    for (integer i = 0; i < `FLASH_SIZE; i++) begin
       |      flash_mem[i] = 8'h0;
       |    end
       |`ifdef FLASH_IMAGE
       |    flash_image = $$fopen(`FLASH_IMAGE_S, "rb");
       |    n_read = $$fread(flash_mem, flash_image);
       |    $$fclose(flash_image);
       |    if (!n_read) begin
       |      $$fatal(1, "Flash: cannot load image from %s.", `FLASH_IMAGE_S);
       |    end
       |    else begin
       |      $$display("Flash: load %d bytes from %s.", n_read, `FLASH_IMAGE_S);
       |    end
       |`else
       |    `ifdef NANHU
       |      // Used for pc = 0x8000_0000
       |      // flash_mem[0] = 64'h01f292930010029b
       |      // flash_mem[1] = 64'h00028067
       |      reg [7:0] init_flash [0:11] = '{8'h9b, 8'h02, 8'h10, 8'h00, 8'h93, 8'h92, 8'hf2, 8'h01, 8'h67, 8'h80, 8'h02, 8'h00};
       |    `else
       |      // Used for pc = 0x20_0000_0000
       |      reg [7:0] init_flash [0:11] = '{8'h9b, 8'h02, 8'h10, 8'h00, 8'h93, 8'h92, 8'h52, 8'h02, 8'h67, 8'h80, 8'h02, 8'h00};
       |    `endif // NANHU
       |    for (integer i = 0; i < 12; i = i + 1) begin
       |        flash_mem[i] = init_flash[i];
       |    end
       |`endif // FLASH_IMAGE
       |  end
       |
       |`endif // SYNTHESIS
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
