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
module axis_buffer #(
  parameter DATA_WIDTH = 16000,
  parameter NUM_PACKETS_PER_BUFFER = 8
)(
  input clock,
  input reset,
  input wr_en,
  input [$clog2(NUM_PACKETS_PER_BUFFER)-1:0] wr_addr,
  input [DATA_WIDTH-1:0] wr_data,
  input [$clog2(NUM_PACKETS_PER_BUFFER)-1:0] rd_addr,
  output [DATA_WIDTH-1:0] rd_data
);
  localparam ADDR_WIDTH = $clog2(NUM_PACKETS_PER_BUFFER);
  localparam BLOCK_RAM_DATA_WIDTH = 4000;
  localparam NUM_BLOCKS = DATA_WIDTH / BLOCK_RAM_DATA_WIDTH;

  wire [BLOCK_RAM_DATA_WIDTH-1:0] rd_data_split [0:NUM_BLOCKS-1];
  wire [BLOCK_RAM_DATA_WIDTH-1:0] wr_data_split [0:NUM_BLOCKS-1];

  genvar i;
  generate
    for (i = 0; i < NUM_BLOCKS; i = i + 1) begin : gen_bram
      assign wr_data_split[i] = wr_data[(i+1)*BLOCK_RAM_DATA_WIDTH-1 : i*BLOCK_RAM_DATA_WIDTH];
      bram_port #(
        .DATA_WIDTH(BLOCK_RAM_DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH)
      ) bram_inst (
        .clk(clock),
        .rst(reset),
        .wea(wr_en),
        .en(1'b1),
        .waddr(wr_addr),
        .raddr(rd_addr),
        .wdata(wr_data_split[i]),
        .rdata(rd_data_split[i])
      );
      assign rd_data[(i+1)*BLOCK_RAM_DATA_WIDTH-1 : i*BLOCK_RAM_DATA_WIDTH] = rd_data_split[i];
    end
  endgenerate

endmodule