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
  val addr = Input(UInt(32.W))
  val data = Output(UInt(64.W))

  def read(enable: Bool, address: UInt): UInt = {
    en := enable
    addr := address
    data
  }
}

class FlashHelper extends ExtModule with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val r = IO(new DifftestFlashRead)

  val cppExtModule =
    """
      |void FlashHelper (
      |  uint8_t   r_en,
      |  uint32_t  r_addr,
      |  uint64_t& r_data
      |) {
      |  if (r_en) flash_read(r_addr, &r_data);
      |}
      |""".stripMargin
  difftest.DifftestModule.createCppExtModule("FlashHelper", cppExtModule, Some("\"flash.h\""))

  setInline(
    "FlashHelper.v",
    """
      |`ifdef SYNTHESIS
      |  `define DISABLE_DIFFTEST_FLASH_DPIC
      |`endif // SYNTHESIS
      |`ifndef DISABLE_DIFFTEST_FLASH_DPIC
      |import "DPI-C" function void flash_read
      |(
      |  input int unsigned addr,
      |  output longint unsigned data
      |);
      |`endif // DISABLE_DIFFTEST_FLASH_DPIC
      |
      |module FlashHelper (
      |  input clock,
      |  input r_en,
      |  input [31:0] r_addr,
      |  output reg [63:0] r_data
      |);
      |
      |`ifndef DISABLE_DIFFTEST_FLASH_DPIC
      |  always @(posedge clock) begin
      |    if (r_en) flash_read(r_addr, r_data);
      |  end
      |`else
      |`ifdef PALLADIUM
      |  initial $ixc_ctrl("tb_import", "$display");
      |`endif // PALLADIUM
      |  // 512K entries. 4MB size.
      |  `define FLASH_SIZE (4 * 1024 * 1024)
      |  reg [7:0] flash_mem [0 : `FLASH_SIZE - 1];
      |
      |  for (genvar i = 0; i < 8; i++) begin
      |    always @(posedge clock) begin
      |      if (r_en) r_data[8 * i + 7 : 8 * i] <= flash_mem[r_addr + i];
      |    end
      |  end
      |
      |  string  bin_file;
      |  integer flash_image = 0, n_read = 0, byte_read = 0;
      |  byte data;
      |  reg [7:0] flash_initval [0:11]; // Use when flash is not specified
      |
      |  initial begin
      |    for (integer i = 0; i < `FLASH_SIZE; i++) begin
      |      flash_mem[i] = 8'h0;
      |    end
      |    if ($test$plusargs("flash")) begin
      |      $value$plusargs("flash=%s", bin_file);
      |      flash_image = $fopen(bin_file, "rb");
      |      if (flash_image == 0) begin
      |        $display("Error: failed to open %s", bin_file);
      |      end
      |      for (integer i = 0; i < `FLASH_SIZE; i++) begin
      |        byte_read = $fread(data, flash_image);
      |        if (byte_read == 0) break;
      |        n_read += 1;
      |        flash_mem[i] = data;
      |      end
      |      $fclose(flash_image);
      |      $display("Flash: load %d bytes from %s.", n_read, bin_file);
      |    end
      |    else begin
      |      /** no specified flash_path, use defualt 3 instructions **/
      |      // addiw   t0,zero,1
      |      // slli    to,to,  0x1f
      |      // jr      t0
      |      // Used for pc = 0x8000_0000
      |      // flash_mem[0] = 64'h01f292930010029b
      |      // flash_mem[1] = 64'h00028067
      |      flash_initval = '{8'h9b, 8'h02, 8'h10, 8'h00, 8'h93, 8'h92, 8'hf2, 8'h01, 8'h67, 8'h80, 8'h02, 8'h00};
      |      for (integer i = 0; i < 12; i = i + 1) begin
      |          flash_mem[i] = flash_initval[i];
      |      end
      |    end
      |  end
      |`endif // DISABLE_DIFFTEST_FLASH_DPIC
      |
      |endmodule
     """.stripMargin,
  )
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
