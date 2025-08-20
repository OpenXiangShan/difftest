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
module bram_port #(
  parameter DATA_WIDTH = 4000,
  parameter ADDR_WIDTH = 3,
  parameter RAM_DEPTH = 1 << ADDR_WIDTH
) (
  input                    clk,
  input                    rst,
  input                    wea,
  input                    en,
  input [ADDR_WIDTH - 1:0] waddr,
  input [ADDR_WIDTH - 1:0] raddr,
  input [DATA_WIDTH - 1:0] wdata,
  output reg [DATA_WIDTH - 1:0] rdata
);

  (* ram_style = "block" *) reg [DATA_WIDTH - 1:0] mem [RAM_DEPTH - 1:0];

`ifndef SYNTHESIS
  integer initvar;
  initial begin
    for (initvar = 0; initvar < RAM_DEPTH; initvar = initvar + 1)
      mem[initvar] = {DATA_WIDTH{1'b0}};
  end
`endif

  always @(posedge clk) begin
    if (en) begin
      rdata <= mem[raddr];
    end
  end

  always @(posedge clk) begin
    if (en && wea) begin
      mem[waddr] <= wdata;
    end
  end
endmodule
