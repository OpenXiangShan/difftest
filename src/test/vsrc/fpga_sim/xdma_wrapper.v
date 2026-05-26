/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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

// FPGA_SIM software XDMA IP model.
//
// Each XDMA-facing bus is represented by a small Verilog/DPI wrapper:
//   - C2H AXI-Stream writes DiffTest packets into shared memory.
//   - H2C AXI-Stream replays host workload packets into the Chisel memory path.
//   - AXI4-Lite replays fpga-host BAR writes into the Chisel ConfigBar.
module xdma_wrapper(
  input pcie_clock,
  input axilite_clock,
  input reset,

  input [511:0] axi_c2h_tdata,
  input         axi_c2h_tlast,
  output        axi_c2h_tready,
  input         axi_c2h_tvalid,

  output [511:0] axi_h2c_tdata,
  output [63:0]  axi_h2c_tkeep,
  output         axi_h2c_tlast,
  input          axi_h2c_tready,
  output         axi_h2c_tvalid,

  output [31:0] cfg_awaddr,
  output        cfg_awvalid,
  input         cfg_awready,
  output [31:0] cfg_wdata,
  output [3:0]  cfg_wstrb,
  output        cfg_wvalid,
  input         cfg_wready,
  input  [1:0]  cfg_bresp,
  input         cfg_bvalid,
  output        cfg_bready,
  output [31:0] cfg_araddr,
  output        cfg_arvalid,
  input         cfg_arready,
  input  [31:0] cfg_rdata,
  input  [1:0]  cfg_rresp,
  input         cfg_rvalid,
  output        cfg_rready
);

xdma_axi_h2c xdma_h2c(
  .clock(pcie_clock),
  .reset(reset),
  .axi_tdata(axi_h2c_tdata),
  .axi_tkeep(axi_h2c_tkeep),
  .axi_tlast(axi_h2c_tlast),
  .axi_tready(axi_h2c_tready),
  .axi_tvalid(axi_h2c_tvalid)
);

xdma_axi_c2h xdma_c2h(
  .clock(pcie_clock),
  .reset(reset),
  .axi_tdata(axi_c2h_tdata),
  .axi_tlast(axi_c2h_tlast),
  .axi_tready(axi_c2h_tready),
  .axi_tvalid(axi_c2h_tvalid)
);

xdma_axilite xdma_cfg(
  .clock(axilite_clock),
  .reset(reset),
  .awaddr(cfg_awaddr),
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
  .arvalid(cfg_arvalid),
  .arready(cfg_arready),
  .rdata(cfg_rdata),
  .rresp(cfg_rresp),
  .rvalid(cfg_rvalid),
  .rready(cfg_rready)
);

endmodule
