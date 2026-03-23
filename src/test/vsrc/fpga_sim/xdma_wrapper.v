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
`include "DifftestMacros.svh"
module xdma_wrapper(
  input clock,
  input reset,
  input core_clock_enable,
  input axi_c2h_tlast,
  output axi_c2h_tready,
  input axi_c2h_tvalid,
  input [511:0] axi_c2h_tdata,
  output core_clock,
  // H2C signals
  output h2c_axis_tvalid,
  output [63:0] h2c_axis_tdata,
  output h2c_axis_tlast,
  input h2c_axis_tready,
  // AXI4-Lite Config BAR interface (master side)
  // Master outputs:
  output [31:0]  cfg_awaddr,
  output [2:0]   cfg_awprot,
  output         cfg_awvalid,
  input          cfg_awready,
  output [31:0]  cfg_wdata,
  output [3:0]   cfg_wstrb,
  output         cfg_wvalid,
  input          cfg_wready,
  // Master inputs (from slave):
  input  [1:0]   cfg_bresp,
  input          cfg_bvalid,
  output         cfg_bready,
  output [31:0]  cfg_araddr,
  output [2:0]   cfg_arprot,
  output         cfg_arvalid,
  input          cfg_arready,
  // Master inputs (from slave):
  input  [31:0]  cfg_rdata,
  input  [1:0]   cfg_rresp,
  input          cfg_rvalid,
  output         cfg_rready
);

  wire [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] axi_tdata;
  wire [63:0] axi_tkeep;
  wire axi_tlast;
  wire axi_tready;
  wire axi_tvalid;
xdma_clock xclock(
  .clock(clock),
  .clock_out(core_clock)
);

xdma_axi xaxi(
  .clock(clock),
  .reset(reset),
  .axi_tdata(axi_c2h_tdata),
  .axi_tlast(axi_c2h_tlast),
  .axi_tready(axi_c2h_tready),
  .axi_tvalid(axi_c2h_tvalid)
);

// H2C: Host to Card (for DDR initialization)
xdma_axi_h2c xh2c(
  .clock(clock),
  .reset(reset),
  .axi_tdata(h2c_axis_tdata),
  .axi_tready(h2c_axis_tready),
  .axi_tvalid(h2c_axis_tvalid),
  .axi_tlast(h2c_axis_tlast)
);

// AXI4-Lite Config BAR (for H2C control)
xdma_axilite xaxilite(
  .clock(clock),
  .reset(reset),
  .awaddr(cfg_awaddr),
  .awprot(cfg_awprot),
  .awvalid(cfg_awvalid),
  .awready(cfg_awready),
  .wdata(cfg_wdata),
  .wstrb(cfg_wstrb),
  .wvalid(cfg_wvalid),
  .wready(cfg_wready),
  .bresp(cfg_bresp),
  .bvalid(cfg_bvalid),
  .bready(cfg_bready),
  .araddr(cfg_araddr),
  .arprot(cfg_arprot),
  .arvalid(cfg_arvalid),
  .arready(cfg_arready),
  .rdata(cfg_rdata),
  .rresp(cfg_rresp),
  .rvalid(cfg_rvalid),
  .rready(cfg_rready)
);

endmodule
