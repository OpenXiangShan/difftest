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
module xdma_axi(
  input clock,
  input reset,
  input [511:0] axi_tdata,
  input axi_tlast,
  output axi_tready,
  input axi_tvalid
);

import "DPI-C" function bit v_xdma_tready();
import "DPI-C" function void v_xdma_write(
  input byte channel,
  input bit [511:0] axi_tdata,
  input bit axi_tlast
);

reg axi_tready_r;
assign axi_tready = axi_tready_r;
always @(posedge clock) begin
  if (reset) begin
    axi_tready_r <= 1'b0;
  end
  else begin
    axi_tready_r <= v_xdma_tready();
    if (axi_tvalid & axi_tready) begin
      v_xdma_write(0, axi_tdata, axi_tlast);
    end
  end
end

endmodule
