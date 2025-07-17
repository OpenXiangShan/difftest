//Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2020.2 (lin64) Build 3064766 Wed Nov 18 09:12:47 MST 2020
//Date        : Mon Dec 27 09:35:12 2021
//Host        : xiangshan-10 running 64-bit Ubuntu 20.04.3 LTS
//Command     : generate_target jtag_ddr_subsys_wrapper.bd
//Design      : jtag_ddr_subsys_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module jtag_ddr_subsys_wrapper
   (DDR4_act_n,
    DDR4_adr,
    DDR4_ba,
    DDR4_bg,
    DDR4_ck_c,
    DDR4_ck_t,
    DDR4_cke,
    DDR4_cs_n,
    DDR4_dm_n,
    DDR4_dq,
    DDR4_dqs_c,
    DDR4_dqs_t,
    DDR4_odt,
    DDR4_reset_n,
    OSC_SYS_CLK_clk_n,
    OSC_SYS_CLK_clk_p,
    SOC_CLK,
    MAC_CLK,
    SOC_M_AXI_araddr,
    SOC_M_AXI_arburst,
    SOC_M_AXI_arcache,
    SOC_M_AXI_arid,
    SOC_M_AXI_arlen,
    SOC_M_AXI_arlock,
    SOC_M_AXI_arprot,
    SOC_M_AXI_arqos,
    SOC_M_AXI_arready,
    SOC_M_AXI_arregion,
    SOC_M_AXI_arsize,
    SOC_M_AXI_arvalid,
    SOC_M_AXI_awaddr,
    SOC_M_AXI_awburst,
    SOC_M_AXI_awcache,
    SOC_M_AXI_awid,
    SOC_M_AXI_awlen,
    SOC_M_AXI_awlock,
    SOC_M_AXI_awprot,
    SOC_M_AXI_awqos,
    SOC_M_AXI_awready,
    SOC_M_AXI_awregion,
    SOC_M_AXI_awsize,
    SOC_M_AXI_awvalid,
    SOC_M_AXI_bid,
    SOC_M_AXI_bready,
    SOC_M_AXI_bresp,
    SOC_M_AXI_bvalid,
    SOC_M_AXI_rdata,
    SOC_M_AXI_rid,
    SOC_M_AXI_rlast,
    SOC_M_AXI_rready,
    SOC_M_AXI_rresp,
    SOC_M_AXI_rvalid,
    SOC_M_AXI_wdata,
    SOC_M_AXI_wlast,
    SOC_M_AXI_wready,
    SOC_M_AXI_wstrb,
    SOC_M_AXI_wvalid,
    SOC_RESETN,
    calib_complete,
    ddr_rstn,
    soc_rstn);
  output DDR4_act_n;
  output [16:0]DDR4_adr;
  output [1:0]DDR4_ba;
  output [1:0]DDR4_bg;
  output [0:0]DDR4_ck_c;
  output [0:0]DDR4_ck_t;
  output [0:0]DDR4_cke;
  output [0:0]DDR4_cs_n;
  inout [7:0]DDR4_dm_n;
  inout [63:0]DDR4_dq;
  inout [7:0]DDR4_dqs_c;
  inout [7:0]DDR4_dqs_t;
  output [0:0]DDR4_odt;
  output DDR4_reset_n;
  input OSC_SYS_CLK_clk_n;
  input OSC_SYS_CLK_clk_p;
  output SOC_CLK;
  output MAC_CLK;
  input [32:0]SOC_M_AXI_araddr;
  input [1:0]SOC_M_AXI_arburst;
  input [3:0]SOC_M_AXI_arcache;
  input [17:0]SOC_M_AXI_arid;
  input [7:0]SOC_M_AXI_arlen;
  input [0:0]SOC_M_AXI_arlock;
  input [2:0]SOC_M_AXI_arprot;
  input [3:0]SOC_M_AXI_arqos;
  output SOC_M_AXI_arready;
  input [3:0]SOC_M_AXI_arregion;
  input [2:0]SOC_M_AXI_arsize;
  input SOC_M_AXI_arvalid;
  input [32:0]SOC_M_AXI_awaddr;
  input [1:0]SOC_M_AXI_awburst;
  input [3:0]SOC_M_AXI_awcache;
  input [17:0]SOC_M_AXI_awid;
  input [7:0]SOC_M_AXI_awlen;
  input [0:0]SOC_M_AXI_awlock;
  input [2:0]SOC_M_AXI_awprot;
  input [3:0]SOC_M_AXI_awqos;
  output SOC_M_AXI_awready;
  input [3:0]SOC_M_AXI_awregion;
  input [2:0]SOC_M_AXI_awsize;
  input SOC_M_AXI_awvalid;
  output [17:0]SOC_M_AXI_bid;
  input SOC_M_AXI_bready;
  output [1:0]SOC_M_AXI_bresp;
  output SOC_M_AXI_bvalid;
  output [255:0]SOC_M_AXI_rdata;
  output [17:0]SOC_M_AXI_rid;
  output SOC_M_AXI_rlast;
  input SOC_M_AXI_rready;
  output [1:0]SOC_M_AXI_rresp;
  output SOC_M_AXI_rvalid;
  input [255:0]SOC_M_AXI_wdata;
  input SOC_M_AXI_wlast;
  output SOC_M_AXI_wready;
  input [31:0]SOC_M_AXI_wstrb;
  input SOC_M_AXI_wvalid;
  output [0:0]SOC_RESETN;
  output calib_complete;
  input ddr_rstn;
  input soc_rstn;

  wire DDR4_act_n;
  wire [16:0]DDR4_adr;
  wire [1:0]DDR4_ba;
  wire [1:0]DDR4_bg;
  wire [0:0]DDR4_ck_c;
  wire [0:0]DDR4_ck_t;
  wire [0:0]DDR4_cke;
  wire [0:0]DDR4_cs_n;
  wire [7:0]DDR4_dm_n;
  wire [63:0]DDR4_dq;
  wire [7:0]DDR4_dqs_c;
  wire [7:0]DDR4_dqs_t;
  wire [0:0]DDR4_odt;
  wire DDR4_reset_n;
  wire OSC_SYS_CLK_clk_n;
  wire OSC_SYS_CLK_clk_p;
  wire SOC_CLK;
  wire [32:0]SOC_M_AXI_araddr;
  wire [1:0]SOC_M_AXI_arburst;
  wire [3:0]SOC_M_AXI_arcache;
  wire [17:0]SOC_M_AXI_arid;
  wire [7:0]SOC_M_AXI_arlen;
  wire [0:0]SOC_M_AXI_arlock;
  wire [2:0]SOC_M_AXI_arprot;
  wire [3:0]SOC_M_AXI_arqos;
  wire SOC_M_AXI_arready;
  wire [3:0]SOC_M_AXI_arregion;
  wire [2:0]SOC_M_AXI_arsize;
  wire SOC_M_AXI_arvalid;
  wire [32:0]SOC_M_AXI_awaddr;
  wire [1:0]SOC_M_AXI_awburst;
  wire [3:0]SOC_M_AXI_awcache;
  wire [17:0]SOC_M_AXI_awid;
  wire [7:0]SOC_M_AXI_awlen;
  wire [0:0]SOC_M_AXI_awlock;
  wire [2:0]SOC_M_AXI_awprot;
  wire [3:0]SOC_M_AXI_awqos;
  wire SOC_M_AXI_awready;
  wire [3:0]SOC_M_AXI_awregion;
  wire [2:0]SOC_M_AXI_awsize;
  wire SOC_M_AXI_awvalid;
  wire [17:0]SOC_M_AXI_bid;
  wire SOC_M_AXI_bready;
  wire [1:0]SOC_M_AXI_bresp;
  wire SOC_M_AXI_bvalid;
  wire [255:0]SOC_M_AXI_rdata;
  wire [17:0]SOC_M_AXI_rid;
  wire SOC_M_AXI_rlast;
  wire SOC_M_AXI_rready;
  wire [1:0]SOC_M_AXI_rresp;
  wire SOC_M_AXI_rvalid;
  wire [255:0]SOC_M_AXI_wdata;
  wire SOC_M_AXI_wlast;
  wire SOC_M_AXI_wready;
  wire [31:0]SOC_M_AXI_wstrb;
  wire SOC_M_AXI_wvalid;
  wire [0:0]SOC_RESETN;
  wire calib_complete;
  wire ddr_rstn;
  wire soc_rstn;

  jtag_ddr_subsys jtag_ddr_subsys_i
       (.DDR4_act_n(DDR4_act_n),
        .DDR4_adr(DDR4_adr),
        .DDR4_ba(DDR4_ba),
        .DDR4_bg(DDR4_bg),
        .DDR4_ck_c(DDR4_ck_c),
        .DDR4_ck_t(DDR4_ck_t),
        .DDR4_cke(DDR4_cke),
        .DDR4_cs_n(DDR4_cs_n),
        .DDR4_dm_n(DDR4_dm_n),
        .DDR4_dq(DDR4_dq),
        .DDR4_dqs_c(DDR4_dqs_c),
        .DDR4_dqs_t(DDR4_dqs_t),
        .DDR4_odt(DDR4_odt),
        .DDR4_reset_n(DDR4_reset_n),
        .OSC_SYS_CLK_clk_n(OSC_SYS_CLK_clk_n),
        .OSC_SYS_CLK_clk_p(OSC_SYS_CLK_clk_p),
        .SOC_CLK(SOC_CLK),
        .MAC_CLK(MAC_CLK),
        .SOC_M_AXI_araddr(SOC_M_AXI_araddr),
        .SOC_M_AXI_arburst(SOC_M_AXI_arburst),
        .SOC_M_AXI_arcache(SOC_M_AXI_arcache),
        .SOC_M_AXI_arid(SOC_M_AXI_arid),
        .SOC_M_AXI_arlen(SOC_M_AXI_arlen),
        .SOC_M_AXI_arlock(SOC_M_AXI_arlock),
        .SOC_M_AXI_arprot(SOC_M_AXI_arprot),
        .SOC_M_AXI_arqos(SOC_M_AXI_arqos),
        .SOC_M_AXI_arready(SOC_M_AXI_arready),
        .SOC_M_AXI_arregion(SOC_M_AXI_arregion),
        .SOC_M_AXI_arsize(SOC_M_AXI_arsize),
        .SOC_M_AXI_arvalid(SOC_M_AXI_arvalid),
        .SOC_M_AXI_awaddr(SOC_M_AXI_awaddr),
        .SOC_M_AXI_awburst(SOC_M_AXI_awburst),
        .SOC_M_AXI_awcache(SOC_M_AXI_awcache),
        .SOC_M_AXI_awid(SOC_M_AXI_awid),
        .SOC_M_AXI_awlen(SOC_M_AXI_awlen),
        .SOC_M_AXI_awlock(SOC_M_AXI_awlock),
        .SOC_M_AXI_awprot(SOC_M_AXI_awprot),
        .SOC_M_AXI_awqos(SOC_M_AXI_awqos),
        .SOC_M_AXI_awready(SOC_M_AXI_awready),
        .SOC_M_AXI_awregion(SOC_M_AXI_awregion),
        .SOC_M_AXI_awsize(SOC_M_AXI_awsize),
        .SOC_M_AXI_awvalid(SOC_M_AXI_awvalid),
        .SOC_M_AXI_bid(SOC_M_AXI_bid),
        .SOC_M_AXI_bready(SOC_M_AXI_bready),
        .SOC_M_AXI_bresp(SOC_M_AXI_bresp),
        .SOC_M_AXI_bvalid(SOC_M_AXI_bvalid),
        .SOC_M_AXI_rdata(SOC_M_AXI_rdata),
        .SOC_M_AXI_rid(SOC_M_AXI_rid),
        .SOC_M_AXI_rlast(SOC_M_AXI_rlast),
        .SOC_M_AXI_rready(SOC_M_AXI_rready),
        .SOC_M_AXI_rresp(SOC_M_AXI_rresp),
        .SOC_M_AXI_rvalid(SOC_M_AXI_rvalid),
        .SOC_M_AXI_wdata(SOC_M_AXI_wdata),
        .SOC_M_AXI_wlast(SOC_M_AXI_wlast),
        .SOC_M_AXI_wready(SOC_M_AXI_wready),
        .SOC_M_AXI_wstrb(SOC_M_AXI_wstrb),
        .SOC_M_AXI_wvalid(SOC_M_AXI_wvalid),
        .SOC_RESETN(SOC_RESETN),
        .calib_complete(calib_complete),
        .ddr_rstn(ddr_rstn),
        .soc_rstn(soc_rstn));
endmodule
