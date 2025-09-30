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
  input [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] difftest_data,
  input difftest_enable,
  output xdma_axis_valid,
  output [511:0] xdma_axis_data,
  input xdma_axis_ready,
  input dpi_clock_enable,
  output core_clock
);

  wire core_clock_enable;
  wire [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] axi_c2h_tdata;
  wire [63:0] axi_c2h_tkeep;
  wire axi_c2h_tlast;
  wire axi_c2h_tready;
  wire axi_c2h_tvalid;

  wire [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] axi_h2c_tdata;
  wire [63:0] axi_h2c_tkeep;
  wire axi_h2c_tlast;
  wire axi_h2c_tready;
  wire axi_h2c_tvalid;

  assign axi_h2c_tready = xdma_axis_ready;
  assign xdma_axis_valid = axi_h2c_tvalid;
  assign xdma_axis_data = axi_h2c_tdata;

xdma_clock xclock(
  .clock(clock),
  .reset(reset),
  .core_clock_enable(core_clock_enable || dpi_clock_enable),
  .core_clock(core_clock)
);
Difftest2AXI #(
  .DATA_WIDTH(`CONFIG_DIFFTEST_BATCH_IO_WITDH),
  .AXIS_DATA_WIDTH(512)
) diff2axi(
  .clock(clock),
  .reset(reset),
  .difftest_data(difftest_data),
  .difftest_enable(difftest_enable),
  .core_clock_enable(core_clock_enable),
  .axi_tdata(axi_c2h_tdata),
  .axi_tkeep(axi_c2h_tkeep),
  .axi_tlast(axi_c2h_tlast),
  .axi_tready(axi_c2h_tready),
  .axi_tvalid(axi_c2h_tvalid)
);
xdma_axi_c2h xaxi_c2h(
  .clock(clock),
  .reset(reset),
  .axi_tdata(axi_c2h_tdata),
  .axi_tkeep(axi_c2h_tkeep),
  .axi_tlast(axi_c2h_tlast),
  .axi_tready(axi_c2h_tready),
  .axi_tvalid(axi_c2h_tvalid)
);

xdma_axi_h2c xaxi_h2c(
  .clock(clock),
  .reset(reset),
  .axi_tdata(axi_h2c_tdata),
  .axi_tready(axi_h2c_tready),
  .axi_tvalid(axi_h2c_tvalid)
);

endmodule
