/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
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

// XDMA AXI4-Lite master software model.
// The simulator does not support timing controls in DPI-exported tasks, so the DPI
// call submits one command and reports whether the local AXI-Lite FSM accepted it.
module xdma_axilite(
  input clock,
  input reset,

  output [31:0] awaddr,
  output        awvalid,
  input         awready,

  output [31:0] wdata,
  output [3:0]  wstrb,
  output        wvalid,
  input         wready,

  input  [1:0]  bresp,
  input         bvalid,
  output        bready,

  output [31:0] araddr,
  output        arvalid,
  input         arready,

  input  [31:0] rdata,
  input  [1:0]  rresp,
  input         rvalid,
  output        rready
);

import "DPI-C" context function void v_xdma_axilite_set_scope();

localparam [1:0] AXIL_IDLE = 2'd0;
localparam [1:0] AXIL_WRITE = 2'd1;
localparam [1:0] AXIL_RESP = 2'd2;
localparam [1:0] AXIL_READ = 2'd3;

reg [1:0]  state;
reg [31:0] cmd_addr;
reg [31:0] cmd_data;
reg [3:0]  cmd_strb;
reg        cmd_request;
reg        cmd_ack;
reg        read_pending;
reg        read_done;
reg        pending_is_read;
reg [31:0] pending_addr;
reg [31:0] pending_data;
reg [3:0]  pending_strb;
reg        awvalid_r;
reg        wvalid_r;
reg        bready_r;
reg        arvalid_r;
reg        rready_r;
reg [31:0] read_data_r;

assign awaddr = cmd_addr;
assign awvalid = awvalid_r;
assign wdata = cmd_data;
assign wstrb = cmd_strb;
assign wvalid = wvalid_r;
assign bready = bready_r;

assign araddr = cmd_addr;
assign arvalid = arvalid_r;
assign rready = rready_r;

/* verilator lint_off UNUSED */
wire [1:0]  unused_bresp = bresp;
wire [1:0]  unused_rresp = rresp;
/* verilator lint_on UNUSED */

initial begin
  state = AXIL_IDLE;
  cmd_addr = 32'h0;
  cmd_data = 32'h0;
  cmd_strb = 4'h0;
  cmd_request = 1'b0;
  cmd_ack = 1'b0;
  read_pending = 1'b0;
  read_done = 1'b0;
  pending_is_read = 1'b0;
  pending_addr = 32'h0;
  pending_data = 32'h0;
  pending_strb = 4'h0;
  awvalid_r = 1'b0;
  wvalid_r = 1'b0;
  bready_r = 1'b0;
  arvalid_r = 1'b0;
  rready_r = 1'b0;
  read_data_r = 32'h0;
  v_xdma_axilite_set_scope();
end

export "DPI-C" task v_xdma_axilite_write;
task v_xdma_axilite_write(
  input int unsigned addr,
  input int unsigned data,
  input byte unsigned strb,
  output byte unsigned accepted
);
begin
  if (!reset && (cmd_request == cmd_ack)) begin
    pending_is_read = 1'b0;
    pending_addr = addr;
    pending_data = data;
    pending_strb = strb[3:0];
    cmd_request = ~cmd_request;
    accepted = 8'd1;
  end
  else begin
    accepted = 8'd0;
  end
end
endtask

export "DPI-C" task v_xdma_axilite_read;
task v_xdma_axilite_read(
  input int unsigned addr,
  output int unsigned data,
  output byte unsigned accepted
);
begin
  if (!reset && read_pending && read_done) begin
    read_pending = 1'b0;
    read_done = 1'b0;
    data = read_data_r;
    accepted = 8'd1;
  end
  else if (!reset && !read_pending && (cmd_request == cmd_ack)) begin
    read_pending = 1'b1;
    pending_is_read = 1'b1;
    pending_addr = addr;
    pending_data = 32'h0;
    pending_strb = 4'h0;
    cmd_request = ~cmd_request;
    data = read_data_r;
    accepted = 8'd0;
  end
  else begin
    data = read_data_r;
    accepted = 8'd0;
  end
end
endtask

always @(posedge clock) begin
  if (reset) begin
    state <= AXIL_IDLE;
    cmd_addr <= 32'h0;
    cmd_data <= 32'h0;
    cmd_strb <= 4'h0;
    awvalid_r <= 1'b0;
    wvalid_r <= 1'b0;
    bready_r <= 1'b0;
    arvalid_r <= 1'b0;
    rready_r <= 1'b0;
    read_data_r = 32'h0;
    read_pending = 1'b0;
    read_done = 1'b0;
    pending_is_read = 1'b0;
    cmd_ack <= cmd_request;
  end
  else begin
    case (state)
      AXIL_IDLE: begin
        awvalid_r <= 1'b0;
        wvalid_r <= 1'b0;
        bready_r <= 1'b0;
        arvalid_r <= 1'b0;
        rready_r <= 1'b0;
        if (cmd_request != cmd_ack) begin
          cmd_addr <= pending_addr;
          cmd_data <= pending_data;
          cmd_strb <= pending_strb;
          cmd_ack <= cmd_request;
          if (pending_is_read) begin
            arvalid_r <= 1'b1;
            state <= AXIL_READ;
          end
          else begin
            awvalid_r <= 1'b1;
            wvalid_r <= 1'b1;
            state <= AXIL_WRITE;
          end
        end
      end
      AXIL_WRITE: begin
        if (awvalid_r && awready) begin
          awvalid_r <= 1'b0;
        end
        if (wvalid_r && wready) begin
          wvalid_r <= 1'b0;
        end
        if ((!awvalid_r || awready) && (!wvalid_r || wready)) begin
          bready_r <= 1'b1;
          state <= AXIL_RESP;
        end
      end
      AXIL_RESP: begin
        if (bvalid) begin
          bready_r <= 1'b0;
          state <= AXIL_IDLE;
        end
      end
      AXIL_READ: begin
        if (arvalid_r && arready) begin
          arvalid_r <= 1'b0;
          rready_r <= 1'b1;
        end
        if (rvalid) begin
          read_data_r = rdata;
          read_done = 1'b1;
          rready_r <= 1'b0;
          state <= AXIL_IDLE;
        end
      end
      default: begin
        state <= AXIL_IDLE;
      end
    endcase
  end
end

endmodule
