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

// XDMA AXI4-Lite Master Simulation Module (FPGA_SIM only)
// This module bridges AXI4-Lite transactions from C++ simulation to hardware
//
// In real FPGA: XDMA IP drives the AXI4-Lite interface directly
// In simulation: C++ code calls DPI-C functions which generate AXI4-Lite transactions
module xdma_axilite(
  input clock,
  input reset,

  // AXI4-Lite Write Address Channel
  output [31:0]  awaddr,
  output [2:0]   awprot,
  output        awvalid,
  input         awready,

  // AXI4-Lite Write Data Channel
  output [31:0]  wdata,
  output [3:0]   wstrb,
  output        wvalid,
  input         wready,

  // AXI4-Lite Write Response Channel
  input [1:0]    bresp,
  input          bvalid,
  output         bready,

  // AXI4-Lite Read Address Channel
  output [31:0]  araddr,
  output [2:0]   arprot,
  output        arvalid,
  input         arready,

  // AXI4-Lite Read Data Channel
  input [31:0]   rdata,
  input [1:0]    rresp,
  input          rvalid,
  output         rready
);

// DPI-C functions for C++ to check if transaction is ready
import "DPI-C" function bit v_xdma_axilite_aw_ready();
import "DPI-C" function bit v_xdma_axilite_w_ready();
import "DPI-C" function bit v_xdma_axilite_b_ready();
import "DPI-C" function bit v_xdma_axilite_ar_ready();
import "DPI-C" function bit v_xdma_axilite_r_ready();

// DPI-C functions for C++ to provide transaction data
import "DPI-C" function void v_xdma_axilite_get_aw(
  output [31:0] addr,
  output [2:0]  prot
);

import "DPI-C" function void v_xdma_axilite_get_w(
  output [31:0] data,
  output [3:0]  strb
);

import "DPI-C" function void v_xdma_axilite_get_ar(
  output [31:0] addr,
  output [2:0]  prot
);

// DPI-C functions for C++ to get transaction responses
import "DPI-C" function void v_xdma_axilite_set_b(
  input [1:0] resp
);

import "DPI-C" function void v_xdma_axilite_set_r(
  input [31:0] data,
  input [1:0] resp
);

// ===== AXI4-Lite Constants =====
assign awprot = 3'h0;
assign arprot = 3'h0;

// ===== Write Address Channel =====
reg [31:0] aw_addr_r;
reg aw_valid_r;

always @(posedge clock) begin
  if (reset) begin
    aw_valid_r <= 1'b0;
    aw_addr_r <= 32'h0;
  end
  else begin
    // Check if C++ has provided a write address
    if (!aw_valid_r && v_xdma_axilite_aw_ready()) begin
      v_xdma_axilite_get_aw(aw_addr_r, awprot);  // awprot is constant 0
      aw_valid_r <= 1'b1;
    end
    // Clear valid when handshake completes
    else if (aw_valid_r && awready) begin
      aw_valid_r <= 1'b0;
    end
  end
end

assign awvalid = aw_valid_r;
assign awaddr = aw_addr_r;

// ===== Write Data Channel =====
reg [31:0] w_data_r;
reg [3:0] w_strb_r;
reg w_valid_r;

always @(posedge clock) begin
  if (reset) begin
    w_valid_r <= 1'b0;
    w_data_r <= 32'h0;
    w_strb_r <= 4'h0;
  end
  else begin
    // Check if C++ has provided write data
    if (!w_valid_r && v_xdma_axilite_w_ready()) begin
      v_xdma_axilite_get_w(w_data_r, w_strb_r);
      w_valid_r <= 1'b1;
    end
    // Clear valid when handshake completes
    else if (w_valid_r && wready) begin
      w_valid_r <= 1'b0;
    end
  end
end

assign wvalid = w_valid_r;
assign wdata = w_data_r;
assign wstrb = w_strb_r;

// ===== Write Response Channel =====
// bvalid/bresp are inputs from slave (XDMAConfigBar)
// bready is output to slave when C++ is ready to accept response
reg b_ready_r;
assign bready = b_ready_r;

// Capture B response when handshake completes
always @(posedge clock) begin
  if (reset) begin
    b_ready_r <= 1'b0;
  end
  else begin
    // Keep the DPI call in procedural logic to avoid stale constant folding.
    b_ready_r <= v_xdma_axilite_b_ready();
    if (bvalid && b_ready_r) begin
      v_xdma_axilite_set_b(bresp);
    end
  end
end

// ===== Read Address Channel =====
reg [31:0] ar_addr_r;
reg ar_valid_r;

always @(posedge clock) begin
  if (reset) begin
    ar_valid_r <= 1'b0;
    ar_addr_r <= 32'h0;
  end
  else begin
    // Check if C++ has provided a read address
    if (!ar_valid_r && v_xdma_axilite_ar_ready()) begin
      v_xdma_axilite_get_ar(ar_addr_r, arprot);  // arprot is constant 0
      ar_valid_r <= 1'b1;
    end
    // Clear valid when handshake completes
    else if (ar_valid_r && arready) begin
      ar_valid_r <= 1'b0;
    end
  end
end

assign arvalid = ar_valid_r;
assign araddr = ar_addr_r;

// ===== Read Data Channel =====
// rvalid/rdata/rresp are inputs from slave (XDMAConfigBar)
// rready is output to slave when C++ is ready to accept response
reg r_ready_r;
assign rready = r_ready_r;

// Capture R response when handshake completes
always @(posedge clock) begin
  if (reset) begin
    r_ready_r <= 1'b0;
  end
  else begin
    r_ready_r <= v_xdma_axilite_r_ready();
    if (rvalid && r_ready_r) begin
      v_xdma_axilite_set_r(rdata, rresp);
    end
  end
end

endmodule
