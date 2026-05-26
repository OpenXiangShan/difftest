/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

// XDMA H2C AXI-Stream software model.
module xdma_axi_h2c(
  input clock,
  input reset,
  output reg [511:0] axi_tdata,
  output reg [63:0] axi_tkeep,
  output reg axi_tlast,
  input axi_tready,
  output reg axi_tvalid
);

import "DPI-C" context function void v_xdma_h2c_set_scope();

reg        pending_valid;
reg [511:0] pending_data;
reg [63:0] pending_keep;
reg        pending_last;

initial begin
  axi_tdata = 512'b0;
  axi_tkeep = 64'b0;
  axi_tlast = 1'b0;
  axi_tvalid = 1'b0;
  pending_valid = 1'b0;
  pending_data = 512'b0;
  pending_keep = 64'b0;
  pending_last = 1'b0;
  v_xdma_h2c_set_scope();
end

export "DPI-C" task v_xdma_h2c_write;
task v_xdma_h2c_write(
  input bit [511:0] data,
  input longint unsigned keep,
  input bit last,
  output byte unsigned accepted
);
begin
  if (!reset && !pending_valid) begin
    pending_data = data;
    pending_keep = keep;
    pending_last = last;
    pending_valid = 1'b1;
    accepted = 8'd1;
  end
  else begin
    accepted = 8'd0;
  end
end
endtask

always @(posedge clock) begin
  if (reset) begin
    axi_tdata <= 512'b0;
    axi_tkeep <= 64'b0;
    axi_tlast <= 1'b0;
    axi_tvalid <= 1'b0;
    pending_valid = 1'b0;
  end
  else if (!axi_tvalid || axi_tready) begin
    if (pending_valid) begin
      axi_tdata <= pending_data;
      axi_tkeep <= pending_keep;
      axi_tlast <= pending_last;
      axi_tvalid <= 1'b1;
      pending_valid = 1'b0;
    end
    else begin
      axi_tvalid <= 1'b0;
    end
  end
end

endmodule
