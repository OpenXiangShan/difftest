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

import "DPI-C" function void v_xdma_write(
  input byte channel,
  input bit [511:0] axi_tdata,
  input bit axi_tlast
);

// Simulate random ready of tready
reg [63:0] ready_timer;
assign axi_tready = !reset && ready_timer == 64'b0;
// Count cycles from an accepted tlast to the next tvalid and print (simulation only)
reg counting_enable;
reg [31:0] cycle_cnt;
always @(posedge clock) begin
  if (reset) begin
    ready_timer <= 64'h0;
  end
  else begin
    if (ready_timer != 64'b0) begin
      ready_timer <= ready_timer - 1;
    end
    if (axi_tvalid & axi_tready) begin
      if (axi_tlast) begin
        ready_timer <= $urandom_range(20000, 30000);
      end
      else begin
        ready_timer <= $urandom_range(0, 2);
      end
    end
  end
end

always @(posedge clock) begin
  if (!reset & axi_tvalid & axi_tready) begin
    v_xdma_write(0, axi_tdata, axi_tlast);
  end
end

// Track cycles between an accepted tlast and the next tvalid, print in simulation
always @(posedge clock) begin
  if (reset) begin
    counting_enable <= 1'b0;
    cycle_cnt <= 32'b0;
  end
  else begin
    // start counting when a tlast beat is accepted
    if (axi_tvalid & axi_tready & axi_tlast) begin
      counting_enable <= 1'b1;
      cycle_cnt <= 32'b0;
    end
    else if (counting_enable) begin
      // when next tvalid appears, report and stop counting
      if (axi_tvalid) begin
        // `ifndef SYNTHESIS
        //   $display("[%0t] xdma_axi: cycles between last and next tvalid = %0d", $time, cycle_cnt);
        // `endif
        counting_enable <= 1'b0;
      end
      else begin
        cycle_cnt <= cycle_cnt + 1;
      end
    end
  end
end
endmodule
