/***************************************************************************************
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

package difftest.fpga

import chisel3._
import chisel3.experimental.ExtModule
import chisel3.util.HasExtModuleInline

class DifftestResetSync(init: Int = 0, stages: Int = 3) extends ExtModule(
  Map(
    "INIT" -> init,
    "STAGES" -> stages,
  ),
) with HasExtModuleInline {
  val clk = IO(Input(Clock()))
  val async_in = IO(Input(Bool()))
  val sync_out = IO(Output(Bool()))

  setInline(
    "DifftestResetSync.v",
    s"""
       |module DifftestResetSync #(
       |  parameter INIT = 1'b0,
       |  parameter STAGES = 3
       |) (
       |  input  clk,
       |  input  async_in,
       |  output sync_out
       |);
       |
       |  (* ASYNC_REG = "TRUE", SHREG_EXTRACT = "NO" *) reg [STAGES-1:0] sreg = {STAGES{INIT[0]}};
       |
       |  always @(posedge clk) begin
       |    sreg <= {sreg[STAGES-2:0], async_in};
       |  end
       |
       |  assign sync_out = sreg[STAGES-1];
       |
       |endmodule
       |""".stripMargin,
  )
}

object DifftestResetSync {
  def apply(clock: Clock, asyncIn: Bool, init: Boolean = false, stages: Int = 3): Bool = {
    val sync = Module(new DifftestResetSync(if (init) 1 else 0, stages))
    sync.clk := clock
    sync.async_in := asyncIn
    sync.sync_out
  }
}
