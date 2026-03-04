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
module xdma_axi_h2c(
  input clock,
  input reset,
  output [511:0] axi_tdata,
  input axi_tready,
  output axi_tvalid,
  output axi_tlast
);

import "DPI-C" function bit v_xdma_h2c_tvalid();
import "DPI-C" function bit v_xdma_h2c_tlast();
import "DPI-C" function void v_xdma_h2c_read(
  input byte channel,
  output bit [511:0] axi_tdata
);
import "DPI-C" function void v_xdma_h2c_commit(
  input int beat_idx,
  input bit [511:0] axi_tdata
);

reg axi_tvalid_r;
reg [511:0] axi_tdata_r;
reg axi_tlast_r;
reg [31:0] h2c_beat_idx_r;

assign axi_tvalid = axi_tvalid_r;
assign axi_tdata = axi_tdata_r;
assign axi_tlast = axi_tlast_r;

always @(posedge clock) begin
  if (reset) begin
    axi_tvalid_r <= 1'b0;
    axi_tdata_r <= 512'h0;
    axi_tlast_r <= 1'b0;
    h2c_beat_idx_r <= 32'h0;
  end
  else begin
    // Read data first (if available), then update valid
    if (v_xdma_h2c_tvalid() && !axi_tvalid_r) begin
      // New data available and we're not already holding valid data
      axi_tlast_r <= v_xdma_h2c_tlast();
      v_xdma_h2c_read(0, axi_tdata_r);
      axi_tvalid_r <= 1'b1;
    end
    else if (axi_tvalid_r && axi_tready) begin
      v_xdma_h2c_commit(h2c_beat_idx_r, axi_tdata_r);
      if (axi_tlast_r) begin
        h2c_beat_idx_r <= 32'h0;
      end
      else begin
        h2c_beat_idx_r <= h2c_beat_idx_r + 32'h1;
      end
      // Handshake complete, clear valid
      axi_tvalid_r <= 1'b0;
      axi_tlast_r <= 1'b0;
    end
  end
end

endmodule
