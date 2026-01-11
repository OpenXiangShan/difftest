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
module xdma_clock(
  input clock,
  output clock_out
);

`ifdef ASYNC_CLK_2N
// core_clock = clock / (2 * ASYNC_CLK_2N)
reg clock_r;
reg [7:0] clk_cnt;
initial begin
  clock_r = 0;
  clk_cnt = 0;
end
always @(posedge clock) begin
  if (clk_cnt == `ASYNC_CLK_2N - 1) begin
    clk_cnt <= 0;
    clock_r <= ~clock_r;
  end
  else begin
    clk_cnt <= clk_cnt + 1;
  end
end
assign clock_out = clock_r;
`else
assign clock_out = clock;
`endif // ASYNC_CLK_2N
endmodule
