/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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
`include "DifftestMacros.v"
module xdma_wrapper(
  input clock,
  input reset,
  input core_clock_enable,
  input axi_c2h_tlast,
  output axi_c2h_tready,
  input axi_c2h_tvalid,
  input [511:0] axi_c2h_tdata,
  output core_clock
);

  wire [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] axi_tdata;
  wire [63:0] axi_tkeep;
  wire axi_tlast;
  wire axi_tready;
  wire axi_tvalid;
xdma_clock xclock(
  .clock(clock),
  .core_clock_enable(core_clock_enable),
  .core_clock(core_clock)
);

xdma_axi xaxi(
  .clock(clock),
  .reset(reset),
  .axi_tdata(axi_c2h_tdata),
  .axi_tlast(axi_c2h_tlast),
  .axi_tready(axi_c2h_tready),
  .axi_tvalid(axi_c2h_tvalid)
);

endmodule
