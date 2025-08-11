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
module dual_buffer_bram #(
  parameter DATA_WIDTH = 16000,
  parameter NUM_PACKETS_PER_BUFFER = 8,
  parameter ADDR_WIDTH = 3 // log2(NUM_PACKETS_PER_BUFFER)
)(
  input clk,
  input rst,
  input wr_en,
  input [ADDR_WIDTH-1:0] wr_addr,
  input [DATA_WIDTH-1:0] wr_data,
  input [ADDR_WIDTH-1:0] rd_addr,
  output [DATA_WIDTH-1:0] rd_data
);
  // TODO: Implement automatic BRAM stitching when more width configurations are available

  localparam BLOCK_RAM_DATA_WIDTH = 4000;

  wire [BLOCK_RAM_DATA_WIDTH - 1:0] rd_data_split [3:0];
  wire [BLOCK_RAM_DATA_WIDTH - 1:0] wr_data_split [3:0];
  assign wr_data_split[0] = wr_data[ 3999:    0];
  assign wr_data_split[1] = wr_data[ 7999: 4000];
  assign wr_data_split[2] = wr_data[11999: 8000];
  assign wr_data_split[3] = wr_data[15999:12000];

  wire [ADDR_WIDTH-1:0] addr;
  assign addr = wr_en ? wr_addr : rd_addr;

  bram_single_port #(.DATA_WIDTH(BLOCK_RAM_DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) bram0 (
    .clk(clk), .rst(rst), .wea(wr_en), .addr(addr), .wdata(wr_data_split[0]), .rdata(rd_data_split[0])
  );
  bram_single_port #(.DATA_WIDTH(BLOCK_RAM_DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) bram1 (
    .clk(clk), .rst(rst), .wea(wr_en), .addr(addr), .wdata(wr_data_split[1]), .rdata(rd_data_split[1])
  );
  bram_single_port #(.DATA_WIDTH(BLOCK_RAM_DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) bram2 (
    .clk(clk), .rst(rst), .wea(wr_en), .addr(addr), .wdata(wr_data_split[2]), .rdata(rd_data_split[2])
  );
  bram_single_port #(.DATA_WIDTH(BLOCK_RAM_DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) bram3 (
    .clk(clk), .rst(rst), .wea(wr_en), .addr(addr), .wdata(wr_data_split[3]), .rdata(rd_data_split[3])
  );

  assign rd_data = {rd_data_split[3], rd_data_split[2], rd_data_split[1], rd_data_split[0]};

endmodule
