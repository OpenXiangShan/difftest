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
module xdma_axi_h2c(
  input clock,
  input reset,
  output [511:0] axi_tdata,
  input axi_tready,
  output axi_tvalid
);

import "DPI-C" function bit v_xdma_h2c_tvalid();
import "DPI-C" function void v_xdma_h2c_read(
  input byte channel,
  input bit [511:0] axi_tdata,
);

reg axi_tvalid_r;
assign axi_tvalid = axi_tvalid_r;
always @(posedge clock) begin
  if (reset) begin
    axi_tvalid_r <= 1'b0;
  end
  else begin
    axi_tvalid_r <= v_xdma_h2c_tvalid();
    if (axi_tvalid & axi_tready) begin
      v_xdma_h2c_read(0, axi_tdata);
      $display("[%t] %m: axi_tdata = %x", $time, axi_tdata);
    end
  end
end

endmodule
