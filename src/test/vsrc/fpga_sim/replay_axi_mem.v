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

// Extra DDR model for FPGA replay AXI writes. Write-only; unused read
// channels stay idle. Random ready stalls exercise HostEndpoint-style backpressure.
module replay_axi_mem(
  input         clock,
  input         reset,
  input         awvalid,
  output        awready,
  input  [31:0] awaddr,
  input         awid,
  input  [7:0]  awlen,
  input  [2:0]  awsize,
  input  [1:0]  awburst,
  input         awlock,
  input  [3:0]  awcache,
  input  [2:0]  awprot,
  input  [3:0]  awqos,
  input         awuser,
  input         wvalid,
  output        wready,
  input  [63:0] wdata,
  input  [7:0]  wstrb,
  input         wlast,
  output        bvalid,
  input         bready,
  output [1:0]  bresp,
  output        bid,
  output        buser,
  input         arvalid,
  output        arready,
  input  [31:0] araddr,
  input         arid,
  input  [7:0]  arlen,
  input  [2:0]  arsize,
  input  [1:0]  arburst,
  input         arlock,
  input  [3:0]  arcache,
  input  [2:0]  arprot,
  input  [3:0]  arqos,
  input         aruser,
  output        rvalid,
  input         rready,
  output [63:0] rdata,
  output [1:0]  rresp,
  output        rlast,
  output        rid,
  output        ruser
);

localparam MEM_WORDS = 20'h20000; // 1MB window

reg [1:0]  state;
reg [31:0] addr;
reg [7:0]  beats_left;
reg        resp_id;
reg [3:0]  stall;
reg [31:0] wr_beats;
reg [31:0] wr_bursts;
reg [63:0] mem [0:MEM_WORDS-1];

wire accept = stall[1:0] != 2'b11;
wire aw_fire = awvalid && awready;
wire w_fire = wvalid && wready;
wire b_fire = bvalid && bready;
wire [19:0] wr_idx = addr[22:3];

assign awready = !reset && (state == 2'd0) && accept;
assign wready = !reset && (state == 2'd1) && accept;
assign bvalid = !reset && (state == 2'd2);
assign bresp = 2'b00;
assign bid = resp_id;
assign buser = 1'b0;
assign arready = 1'b1;
assign rvalid = 1'b0;
assign rdata = 64'b0;
assign rresp = 2'b00;
assign rlast = 1'b0;
assign rid = 1'b0;
assign ruser = 1'b0;

always @(posedge clock) begin
  if (reset) begin
    state <= 2'd0;
    addr <= 32'b0;
    beats_left <= 8'b0;
    resp_id <= 1'b0;
    stall <= 4'b0;
    wr_beats <= 32'b0;
    wr_bursts <= 32'b0;
  end
  else begin
    stall <= stall + 4'd1;
    if (w_fire) begin
      wr_beats <= wr_beats + 32'd1;
    end
    if (b_fire) begin
      wr_bursts <= wr_bursts + 32'd1;
    end
    if (aw_fire) begin
      addr <= awaddr;
      beats_left <= awlen;
      resp_id <= awid;
      state <= 2'd1;
    end
    if (w_fire) begin
      if (wr_idx < MEM_WORDS) begin
        mem[wr_idx] <= wdata;
      end
      addr <= addr + 32'd8;
      if (wlast) begin
        state <= 2'd2;
      end
      else begin
        beats_left <= beats_left - 8'd1;
      end
    end
    if (b_fire) begin
      state <= 2'd0;
      if (wr_bursts[19:0] == 20'd0) begin
        $display("[replay_axi_mem] bursts=%0d beats=%0d last_addr=%h", wr_bursts, wr_beats, addr);
      end
    end
  end
end

endmodule
