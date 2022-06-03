module AXI4Xbar(
  input         clock,
  input         reset,
  output        auto_in_awready,
  input         auto_in_awvalid,
  input         auto_in_awid,
  input  [36:0] auto_in_awaddr,
  input  [7:0]  auto_in_awlen,
  input  [2:0]  auto_in_awsize,
  input  [1:0]  auto_in_awburst,
  input         auto_in_awlock,
  input  [3:0]  auto_in_awcache,
  input  [2:0]  auto_in_awprot,
  input  [3:0]  auto_in_awqos,
  output        auto_in_wready,
  input         auto_in_wvalid,
  input  [63:0] auto_in_wdata,
  input  [7:0]  auto_in_wstrb,
  input         auto_in_wlast,
  input         auto_in_bready,
  output        auto_in_bvalid,
  output        auto_in_bid,
  output [1:0]  auto_in_bresp,
  output        auto_in_arready,
  input         auto_in_arvalid,
  input         auto_in_arid,
  input  [36:0] auto_in_araddr,
  input  [7:0]  auto_in_arlen,
  input  [2:0]  auto_in_arsize,
  input  [1:0]  auto_in_arburst,
  input         auto_in_arlock,
  input  [3:0]  auto_in_arcache,
  input  [2:0]  auto_in_arprot,
  input  [3:0]  auto_in_arqos,
  input         auto_in_rready,
  output        auto_in_rvalid,
  output        auto_in_rid,
  output [63:0] auto_in_rdata,
  output [1:0]  auto_in_rresp,
  output        auto_in_rlast,
  input         auto_out_8_awready,
  output        auto_out_8_awvalid,
  output        auto_out_8_awid,
  output [36:0] auto_out_8_awaddr,
  output [7:0]  auto_out_8_awlen,
  output [2:0]  auto_out_8_awsize,
  output [1:0]  auto_out_8_awburst,
  output        auto_out_8_awlock,
  output [3:0]  auto_out_8_awcache,
  output [2:0]  auto_out_8_awprot,
  output [3:0]  auto_out_8_awqos,
  input         auto_out_8_wready,
  output        auto_out_8_wvalid,
  output [63:0] auto_out_8_wdata,
  output [7:0]  auto_out_8_wstrb,
  output        auto_out_8_wlast,
  output        auto_out_8_bready,
  input         auto_out_8_bvalid,
  input         auto_out_8_bid,
  input  [1:0]  auto_out_8_bresp,
  input         auto_out_8_arready,
  output        auto_out_8_arvalid,
  output        auto_out_8_arid,
  output [36:0] auto_out_8_araddr,
  output [7:0]  auto_out_8_arlen,
  output [2:0]  auto_out_8_arsize,
  output [1:0]  auto_out_8_arburst,
  output        auto_out_8_arlock,
  output [3:0]  auto_out_8_arcache,
  output [2:0]  auto_out_8_arprot,
  output [3:0]  auto_out_8_arqos,
  output        auto_out_8_rready,
  input         auto_out_8_rvalid,
  input         auto_out_8_rid,
  input  [63:0] auto_out_8_rdata,
  input  [1:0]  auto_out_8_rresp,
  input         auto_out_8_rlast,
  input         auto_out_7_awready,
  output        auto_out_7_awvalid,
  output        auto_out_7_awid,
  output [36:0] auto_out_7_awaddr,
  output [7:0]  auto_out_7_awlen,
  output [2:0]  auto_out_7_awsize,
  output [1:0]  auto_out_7_awburst,
  output        auto_out_7_awlock,
  output [3:0]  auto_out_7_awcache,
  output [2:0]  auto_out_7_awprot,
  output [3:0]  auto_out_7_awqos,
  input         auto_out_7_wready,
  output        auto_out_7_wvalid,
  output [63:0] auto_out_7_wdata,
  output [7:0]  auto_out_7_wstrb,
  output        auto_out_7_wlast,
  output        auto_out_7_bready,
  input         auto_out_7_bvalid,
  input         auto_out_7_bid,
  input  [1:0]  auto_out_7_bresp,
  input         auto_out_7_arready,
  output        auto_out_7_arvalid,
  output        auto_out_7_arid,
  output [36:0] auto_out_7_araddr,
  output [7:0]  auto_out_7_arlen,
  output [2:0]  auto_out_7_arsize,
  output [1:0]  auto_out_7_arburst,
  output        auto_out_7_arlock,
  output [3:0]  auto_out_7_arcache,
  output [2:0]  auto_out_7_arprot,
  output [3:0]  auto_out_7_arqos,
  output        auto_out_7_rready,
  input         auto_out_7_rvalid,
  input         auto_out_7_rid,
  input  [63:0] auto_out_7_rdata,
  input  [1:0]  auto_out_7_rresp,
  input         auto_out_7_rlast,
  input         auto_out_6_awready,
  output        auto_out_6_awvalid,
  output        auto_out_6_awid,
  output [36:0] auto_out_6_awaddr,
  output [7:0]  auto_out_6_awlen,
  output [2:0]  auto_out_6_awsize,
  output [1:0]  auto_out_6_awburst,
  output        auto_out_6_awlock,
  output [3:0]  auto_out_6_awcache,
  output [2:0]  auto_out_6_awprot,
  output [3:0]  auto_out_6_awqos,
  input         auto_out_6_wready,
  output        auto_out_6_wvalid,
  output [63:0] auto_out_6_wdata,
  output [7:0]  auto_out_6_wstrb,
  output        auto_out_6_wlast,
  output        auto_out_6_bready,
  input         auto_out_6_bvalid,
  input         auto_out_6_bid,
  input  [1:0]  auto_out_6_bresp,
  input         auto_out_6_arready,
  output        auto_out_6_arvalid,
  output        auto_out_6_arid,
  output [36:0] auto_out_6_araddr,
  output [7:0]  auto_out_6_arlen,
  output [2:0]  auto_out_6_arsize,
  output [1:0]  auto_out_6_arburst,
  output        auto_out_6_arlock,
  output [3:0]  auto_out_6_arcache,
  output [2:0]  auto_out_6_arprot,
  output [3:0]  auto_out_6_arqos,
  output        auto_out_6_rready,
  input         auto_out_6_rvalid,
  input         auto_out_6_rid,
  input  [63:0] auto_out_6_rdata,
  input  [1:0]  auto_out_6_rresp,
  input         auto_out_6_rlast,
  input         auto_out_5_awready,
  output        auto_out_5_awvalid,
  output        auto_out_5_awid,
  output [36:0] auto_out_5_awaddr,
  output [7:0]  auto_out_5_awlen,
  output [2:0]  auto_out_5_awsize,
  output [1:0]  auto_out_5_awburst,
  output        auto_out_5_awlock,
  output [3:0]  auto_out_5_awcache,
  output [2:0]  auto_out_5_awprot,
  output [3:0]  auto_out_5_awqos,
  input         auto_out_5_wready,
  output        auto_out_5_wvalid,
  output [63:0] auto_out_5_wdata,
  output [7:0]  auto_out_5_wstrb,
  output        auto_out_5_wlast,
  output        auto_out_5_bready,
  input         auto_out_5_bvalid,
  input         auto_out_5_bid,
  input  [1:0]  auto_out_5_bresp,
  input         auto_out_5_arready,
  output        auto_out_5_arvalid,
  output        auto_out_5_arid,
  output [36:0] auto_out_5_araddr,
  output [7:0]  auto_out_5_arlen,
  output [2:0]  auto_out_5_arsize,
  output [1:0]  auto_out_5_arburst,
  output        auto_out_5_arlock,
  output [3:0]  auto_out_5_arcache,
  output [2:0]  auto_out_5_arprot,
  output [3:0]  auto_out_5_arqos,
  output        auto_out_5_rready,
  input         auto_out_5_rvalid,
  input         auto_out_5_rid,
  input  [63:0] auto_out_5_rdata,
  input  [1:0]  auto_out_5_rresp,
  input         auto_out_5_rlast,
  input         auto_out_4_awready,
  output        auto_out_4_awvalid,
  output        auto_out_4_awid,
  output [36:0] auto_out_4_awaddr,
  output [7:0]  auto_out_4_awlen,
  output [2:0]  auto_out_4_awsize,
  output [1:0]  auto_out_4_awburst,
  output        auto_out_4_awlock,
  output [3:0]  auto_out_4_awcache,
  output [2:0]  auto_out_4_awprot,
  output [3:0]  auto_out_4_awqos,
  input         auto_out_4_wready,
  output        auto_out_4_wvalid,
  output [63:0] auto_out_4_wdata,
  output [7:0]  auto_out_4_wstrb,
  output        auto_out_4_wlast,
  output        auto_out_4_bready,
  input         auto_out_4_bvalid,
  input         auto_out_4_bid,
  input  [1:0]  auto_out_4_bresp,
  input         auto_out_4_arready,
  output        auto_out_4_arvalid,
  output        auto_out_4_arid,
  output [36:0] auto_out_4_araddr,
  output [7:0]  auto_out_4_arlen,
  output [2:0]  auto_out_4_arsize,
  output [1:0]  auto_out_4_arburst,
  output        auto_out_4_arlock,
  output [3:0]  auto_out_4_arcache,
  output [2:0]  auto_out_4_arprot,
  output [3:0]  auto_out_4_arqos,
  output        auto_out_4_rready,
  input         auto_out_4_rvalid,
  input         auto_out_4_rid,
  input  [63:0] auto_out_4_rdata,
  input  [1:0]  auto_out_4_rresp,
  input         auto_out_4_rlast,
  input         auto_out_3_awready,
  output        auto_out_3_awvalid,
  output        auto_out_3_awid,
  output [36:0] auto_out_3_awaddr,
  output [7:0]  auto_out_3_awlen,
  output [2:0]  auto_out_3_awsize,
  output [1:0]  auto_out_3_awburst,
  output        auto_out_3_awlock,
  output [3:0]  auto_out_3_awcache,
  output [2:0]  auto_out_3_awprot,
  output [3:0]  auto_out_3_awqos,
  input         auto_out_3_wready,
  output        auto_out_3_wvalid,
  output [63:0] auto_out_3_wdata,
  output [7:0]  auto_out_3_wstrb,
  output        auto_out_3_wlast,
  output        auto_out_3_bready,
  input         auto_out_3_bvalid,
  input         auto_out_3_bid,
  input  [1:0]  auto_out_3_bresp,
  output        auto_out_3_arvalid,
  output        auto_out_3_arid,
  output        auto_out_3_arlock,
  output [3:0]  auto_out_3_arcache,
  output [3:0]  auto_out_3_arqos,
  output        auto_out_3_rready,
  input         auto_out_3_rvalid,
  input         auto_out_3_rid,
  input         auto_out_3_rlast,
  input         auto_out_2_awready,
  output        auto_out_2_awvalid,
  output        auto_out_2_awid,
  output [36:0] auto_out_2_awaddr,
  output [7:0]  auto_out_2_awlen,
  output [2:0]  auto_out_2_awsize,
  output [1:0]  auto_out_2_awburst,
  output        auto_out_2_awlock,
  output [3:0]  auto_out_2_awcache,
  output [2:0]  auto_out_2_awprot,
  output [3:0]  auto_out_2_awqos,
  input         auto_out_2_wready,
  output        auto_out_2_wvalid,
  output [63:0] auto_out_2_wdata,
  output [7:0]  auto_out_2_wstrb,
  output        auto_out_2_wlast,
  output        auto_out_2_bready,
  input         auto_out_2_bvalid,
  input         auto_out_2_bid,
  input  [1:0]  auto_out_2_bresp,
  input         auto_out_2_arready,
  output        auto_out_2_arvalid,
  output        auto_out_2_arid,
  output [36:0] auto_out_2_araddr,
  output [7:0]  auto_out_2_arlen,
  output [2:0]  auto_out_2_arsize,
  output [1:0]  auto_out_2_arburst,
  output        auto_out_2_arlock,
  output [3:0]  auto_out_2_arcache,
  output [2:0]  auto_out_2_arprot,
  output [3:0]  auto_out_2_arqos,
  output        auto_out_2_rready,
  input         auto_out_2_rvalid,
  input         auto_out_2_rid,
  input  [63:0] auto_out_2_rdata,
  input  [1:0]  auto_out_2_rresp,
  input         auto_out_2_rlast,
  input         auto_out_1_awready,
  output        auto_out_1_awvalid,
  output        auto_out_1_awid,
  output [36:0] auto_out_1_awaddr,
  output [7:0]  auto_out_1_awlen,
  output [2:0]  auto_out_1_awsize,
  output [1:0]  auto_out_1_awburst,
  output        auto_out_1_awlock,
  output [3:0]  auto_out_1_awcache,
  output [2:0]  auto_out_1_awprot,
  output [3:0]  auto_out_1_awqos,
  input         auto_out_1_wready,
  output        auto_out_1_wvalid,
  output [63:0] auto_out_1_wdata,
  output [7:0]  auto_out_1_wstrb,
  output        auto_out_1_wlast,
  output        auto_out_1_bready,
  input         auto_out_1_bvalid,
  input         auto_out_1_bid,
  input  [1:0]  auto_out_1_bresp,
  input         auto_out_1_arready,
  output        auto_out_1_arvalid,
  output        auto_out_1_arid,
  output [36:0] auto_out_1_araddr,
  output [7:0]  auto_out_1_arlen,
  output [2:0]  auto_out_1_arsize,
  output [1:0]  auto_out_1_arburst,
  output        auto_out_1_arlock,
  output [3:0]  auto_out_1_arcache,
  output [2:0]  auto_out_1_arprot,
  output [3:0]  auto_out_1_arqos,
  output        auto_out_1_rready,
  input         auto_out_1_rvalid,
  input         auto_out_1_rid,
  input  [63:0] auto_out_1_rdata,
  input  [1:0]  auto_out_1_rresp,
  input         auto_out_1_rlast,
  input         auto_out_0_awready,
  output        auto_out_0_awvalid,
  output        auto_out_0_awid,
  output [36:0] auto_out_0_awaddr,
  output [7:0]  auto_out_0_awlen,
  output [2:0]  auto_out_0_awsize,
  output [1:0]  auto_out_0_awburst,
  output        auto_out_0_awlock,
  output [3:0]  auto_out_0_awcache,
  output [2:0]  auto_out_0_awprot,
  output [3:0]  auto_out_0_awqos,
  input         auto_out_0_wready,
  output        auto_out_0_wvalid,
  output [63:0] auto_out_0_wdata,
  output [7:0]  auto_out_0_wstrb,
  output        auto_out_0_wlast,
  output        auto_out_0_bready,
  input         auto_out_0_bvalid,
  input         auto_out_0_bid,
  input  [1:0]  auto_out_0_bresp,
  input         auto_out_0_arready,
  output        auto_out_0_arvalid,
  output        auto_out_0_arid,
  output [36:0] auto_out_0_araddr,
  output [7:0]  auto_out_0_arlen,
  output [2:0]  auto_out_0_arsize,
  output [1:0]  auto_out_0_arburst,
  output        auto_out_0_arlock,
  output [3:0]  auto_out_0_arcache,
  output [2:0]  auto_out_0_arprot,
  output [3:0]  auto_out_0_arqos,
  output        auto_out_0_rready,
  input         auto_out_0_rvalid,
  input         auto_out_0_rid,
  input  [63:0] auto_out_0_rdata,
  input  [1:0]  auto_out_0_rresp,
  input         auto_out_0_rlast
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
  reg [31:0] _RAND_2;
  reg [31:0] _RAND_3;
  reg [31:0] _RAND_4;
  reg [31:0] _RAND_5;
  reg [31:0] _RAND_6;
  reg [31:0] _RAND_7;
  reg [31:0] _RAND_8;
  reg [31:0] _RAND_9;
  reg [31:0] _RAND_10;
  reg [31:0] _RAND_11;
  reg [31:0] _RAND_12;
  reg [31:0] _RAND_13;
  reg [31:0] _RAND_14;
  reg [31:0] _RAND_15;
  reg [31:0] _RAND_16;
  reg [31:0] _RAND_17;
  reg [31:0] _RAND_18;
  reg [31:0] _RAND_19;
  reg [31:0] _RAND_20;
  reg [31:0] _RAND_21;
  reg [31:0] _RAND_22;
`endif // RANDOMIZE_REG_INIT
  wire  awIn_0_clock; // @[Xbar.scala 62:47]
  wire  awIn_0_reset; // @[Xbar.scala 62:47]
  wire  awIn_0_io_enq_ready; // @[Xbar.scala 62:47]
  wire  awIn_0_io_enq_valid; // @[Xbar.scala 62:47]
  wire [8:0] awIn_0_io_enq_bits; // @[Xbar.scala 62:47]
  wire  awIn_0_io_deq_ready; // @[Xbar.scala 62:47]
  wire  awIn_0_io_deq_valid; // @[Xbar.scala 62:47]
  wire [8:0] awIn_0_io_deq_bits; // @[Xbar.scala 62:47]
  wire [36:0] _requestARIO_T = auto_in_araddr ^ 37'h80000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_1 = {1'b0,$signed(_requestARIO_T)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_3 = $signed(_requestARIO_T_1) & 38'shc0000000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_0 = $signed(_requestARIO_T_3) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestARIO_T_5 = auto_in_araddr ^ 37'hc0000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_6 = {1'b0,$signed(_requestARIO_T_5)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_8 = $signed(_requestARIO_T_6) & 38'shd0000000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_1 = $signed(_requestARIO_T_8) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestARIO_T_10 = auto_in_araddr ^ 37'h10000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_11 = {1'b0,$signed(_requestARIO_T_10)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_13 = $signed(_requestARIO_T_11) & 38'shd0032000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_2 = $signed(_requestARIO_T_13) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestARIO_T_15 = auto_in_araddr ^ 37'h50000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_16 = {1'b0,$signed(_requestARIO_T_15)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_18 = $signed(_requestARIO_T_16) & 38'shd0000000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_3 = $signed(_requestARIO_T_18) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestARIO_T_20 = auto_in_araddr ^ 37'h40000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_21 = {1'b0,$signed(_requestARIO_T_20)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_23 = $signed(_requestARIO_T_21) & 38'shd0032000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_4 = $signed(_requestARIO_T_23) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestARIO_T_25 = auto_in_araddr ^ 37'hd0000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_26 = {1'b0,$signed(_requestARIO_T_25)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_28 = $signed(_requestARIO_T_26) & 38'shd0000000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_5 = $signed(_requestARIO_T_28) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestARIO_T_30 = auto_in_araddr ^ 37'h40002000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_31 = {1'b0,$signed(_requestARIO_T_30)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_33 = $signed(_requestARIO_T_31) & 38'shd0032000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_6 = $signed(_requestARIO_T_33) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestARIO_T_35 = auto_in_araddr ^ 37'h20000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_36 = {1'b0,$signed(_requestARIO_T_35)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_38 = $signed(_requestARIO_T_36) & 38'shd0030000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_7 = $signed(_requestARIO_T_38) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestARIO_T_40 = auto_in_araddr ^ 37'h30000; // @[Parameters.scala 137:31]
  wire [37:0] _requestARIO_T_41 = {1'b0,$signed(_requestARIO_T_40)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestARIO_T_43 = $signed(_requestARIO_T_41) & 38'shd0030000; // @[Parameters.scala 137:52]
  wire  requestARIO_0_8 = $signed(_requestARIO_T_43) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T = auto_in_awaddr ^ 37'h80000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_1 = {1'b0,$signed(_requestAWIO_T)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_3 = $signed(_requestAWIO_T_1) & 38'shc0000000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_0 = $signed(_requestAWIO_T_3) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T_5 = auto_in_awaddr ^ 37'hc0000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_6 = {1'b0,$signed(_requestAWIO_T_5)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_8 = $signed(_requestAWIO_T_6) & 38'shd0000000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_1 = $signed(_requestAWIO_T_8) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T_10 = auto_in_awaddr ^ 37'h10000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_11 = {1'b0,$signed(_requestAWIO_T_10)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_13 = $signed(_requestAWIO_T_11) & 38'shd0032000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_2 = $signed(_requestAWIO_T_13) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T_15 = auto_in_awaddr ^ 37'h50000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_16 = {1'b0,$signed(_requestAWIO_T_15)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_18 = $signed(_requestAWIO_T_16) & 38'shd0000000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_3 = $signed(_requestAWIO_T_18) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T_20 = auto_in_awaddr ^ 37'h40000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_21 = {1'b0,$signed(_requestAWIO_T_20)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_23 = $signed(_requestAWIO_T_21) & 38'shd0032000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_4 = $signed(_requestAWIO_T_23) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T_25 = auto_in_awaddr ^ 37'hd0000000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_26 = {1'b0,$signed(_requestAWIO_T_25)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_28 = $signed(_requestAWIO_T_26) & 38'shd0000000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_5 = $signed(_requestAWIO_T_28) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T_30 = auto_in_awaddr ^ 37'h40002000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_31 = {1'b0,$signed(_requestAWIO_T_30)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_33 = $signed(_requestAWIO_T_31) & 38'shd0032000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_6 = $signed(_requestAWIO_T_33) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T_35 = auto_in_awaddr ^ 37'h20000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_36 = {1'b0,$signed(_requestAWIO_T_35)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_38 = $signed(_requestAWIO_T_36) & 38'shd0030000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_7 = $signed(_requestAWIO_T_38) == 38'sh0; // @[Parameters.scala 137:67]
  wire [36:0] _requestAWIO_T_40 = auto_in_awaddr ^ 37'h30000; // @[Parameters.scala 137:31]
  wire [37:0] _requestAWIO_T_41 = {1'b0,$signed(_requestAWIO_T_40)}; // @[Parameters.scala 137:49]
  wire [37:0] _requestAWIO_T_43 = $signed(_requestAWIO_T_41) & 38'shd0030000; // @[Parameters.scala 137:52]
  wire  requestAWIO_0_8 = $signed(_requestAWIO_T_43) == 38'sh0; // @[Parameters.scala 137:67]
  wire [3:0] awIn_0_io_enq_bits_lo = {requestAWIO_0_3,requestAWIO_0_2,requestAWIO_0_1,requestAWIO_0_0}; // @[Xbar.scala 71:75]
  wire [4:0] awIn_0_io_enq_bits_hi = {requestAWIO_0_8,requestAWIO_0_7,requestAWIO_0_6,requestAWIO_0_5,requestAWIO_0_4}; // @[Xbar.scala 71:75]
  wire  requestWIO_0_0 = awIn_0_io_deq_bits[0]; // @[Xbar.scala 72:73]
  wire  requestWIO_0_1 = awIn_0_io_deq_bits[1]; // @[Xbar.scala 72:73]
  wire  requestWIO_0_2 = awIn_0_io_deq_bits[2]; // @[Xbar.scala 72:73]
  wire  requestWIO_0_3 = awIn_0_io_deq_bits[3]; // @[Xbar.scala 72:73]
  wire  requestWIO_0_4 = awIn_0_io_deq_bits[4]; // @[Xbar.scala 72:73]
  wire  requestWIO_0_5 = awIn_0_io_deq_bits[5]; // @[Xbar.scala 72:73]
  wire  requestWIO_0_6 = awIn_0_io_deq_bits[6]; // @[Xbar.scala 72:73]
  wire  requestWIO_0_7 = awIn_0_io_deq_bits[7]; // @[Xbar.scala 72:73]
  wire  requestWIO_0_8 = awIn_0_io_deq_bits[8]; // @[Xbar.scala 72:73]
  reg  idle_9; // @[Xbar.scala 249:23]
  wire [8:0] readys_valid = {auto_out_8_rvalid,auto_out_7_rvalid,auto_out_6_rvalid,auto_out_5_rvalid,
    auto_out_4_rvalid,auto_out_3_rvalid,auto_out_2_rvalid,auto_out_1_rvalid,auto_out_0_rvalid}; // @[Cat.scala 31:58]
  reg [8:0] readys_mask; // @[Arbiter.scala 23:23]
  wire [8:0] _readys_filter_T = ~readys_mask; // @[Arbiter.scala 24:30]
  wire [8:0] _readys_filter_T_1 = readys_valid & _readys_filter_T; // @[Arbiter.scala 24:28]
  wire [17:0] readys_filter = {_readys_filter_T_1,auto_out_8_rvalid,auto_out_7_rvalid,auto_out_6_rvalid,
    auto_out_5_rvalid,auto_out_4_rvalid,auto_out_3_rvalid,auto_out_2_rvalid,auto_out_1_rvalid,auto_out_0_rvalid}; // @[Cat.scala 31:58]
  wire [17:0] _GEN_52 = {{1'd0}, readys_filter[17:1]}; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_1 = readys_filter | _GEN_52; // @[package.scala 253:43]
  wire [17:0] _GEN_53 = {{2'd0}, _readys_unready_T_1[17:2]}; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_3 = _readys_unready_T_1 | _GEN_53; // @[package.scala 253:43]
  wire [17:0] _GEN_54 = {{4'd0}, _readys_unready_T_3[17:4]}; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_5 = _readys_unready_T_3 | _GEN_54; // @[package.scala 253:43]
  wire [17:0] _GEN_55 = {{8'd0}, _readys_unready_T_5[17:8]}; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_7 = _readys_unready_T_5 | _GEN_55; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_10 = {readys_mask, 9'h0}; // @[Arbiter.scala 25:66]
  wire [17:0] _GEN_56 = {{1'd0}, _readys_unready_T_7[17:1]}; // @[Arbiter.scala 25:58]
  wire [17:0] readys_unready = _GEN_56 | _readys_unready_T_10; // @[Arbiter.scala 25:58]
  wire [8:0] _readys_readys_T_2 = readys_unready[17:9] & readys_unready[8:0]; // @[Arbiter.scala 26:39]
  wire [8:0] readys_readys = ~_readys_readys_T_2; // @[Arbiter.scala 26:18]
  wire  readys_9_0 = readys_readys[0]; // @[Xbar.scala 255:69]
  wire  winner_9_0 = readys_9_0 & auto_out_0_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_0; // @[Xbar.scala 268:24]
  wire  muxState_9_0 = idle_9 ? winner_9_0 : state_9_0; // @[Xbar.scala 269:23]
  wire  readys_9_1 = readys_readys[1]; // @[Xbar.scala 255:69]
  wire  winner_9_1 = readys_9_1 & auto_out_1_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_1; // @[Xbar.scala 268:24]
  wire  muxState_9_1 = idle_9 ? winner_9_1 : state_9_1; // @[Xbar.scala 269:23]
  wire  readys_9_2 = readys_readys[2]; // @[Xbar.scala 255:69]
  wire  winner_9_2 = readys_9_2 & auto_out_2_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_2; // @[Xbar.scala 268:24]
  wire  muxState_9_2 = idle_9 ? winner_9_2 : state_9_2; // @[Xbar.scala 269:23]
  wire  readys_9_3 = readys_readys[3]; // @[Xbar.scala 255:69]
  wire  winner_9_3 = readys_9_3 & auto_out_3_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_3; // @[Xbar.scala 268:24]
  wire  muxState_9_3 = idle_9 ? winner_9_3 : state_9_3; // @[Xbar.scala 269:23]
  wire  readys_9_4 = readys_readys[4]; // @[Xbar.scala 255:69]
  wire  winner_9_4 = readys_9_4 & auto_out_4_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_4; // @[Xbar.scala 268:24]
  wire  muxState_9_4 = idle_9 ? winner_9_4 : state_9_4; // @[Xbar.scala 269:23]
  wire  readys_9_5 = readys_readys[5]; // @[Xbar.scala 255:69]
  wire  winner_9_5 = readys_9_5 & auto_out_5_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_5; // @[Xbar.scala 268:24]
  wire  muxState_9_5 = idle_9 ? winner_9_5 : state_9_5; // @[Xbar.scala 269:23]
  wire  readys_9_6 = readys_readys[6]; // @[Xbar.scala 255:69]
  wire  winner_9_6 = readys_9_6 & auto_out_6_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_6; // @[Xbar.scala 268:24]
  wire  muxState_9_6 = idle_9 ? winner_9_6 : state_9_6; // @[Xbar.scala 269:23]
  wire  readys_9_7 = readys_readys[7]; // @[Xbar.scala 255:69]
  wire  winner_9_7 = readys_9_7 & auto_out_7_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_7; // @[Xbar.scala 268:24]
  wire  muxState_9_7 = idle_9 ? winner_9_7 : state_9_7; // @[Xbar.scala 269:23]
  wire  readys_9_8 = readys_readys[8]; // @[Xbar.scala 255:69]
  wire  winner_9_8 = readys_9_8 & auto_out_8_rvalid; // @[Xbar.scala 257:63]
  reg  state_9_8; // @[Xbar.scala 268:24]
  wire  muxState_9_8 = idle_9 ? winner_9_8 : state_9_8; // @[Xbar.scala 269:23]
  reg  idle_10; // @[Xbar.scala 249:23]
  wire [8:0] readys_valid_1 = {auto_out_8_bvalid,auto_out_7_bvalid,auto_out_6_bvalid,auto_out_5_bvalid,
    auto_out_4_bvalid,auto_out_3_bvalid,auto_out_2_bvalid,auto_out_1_bvalid,auto_out_0_bvalid}; // @[Cat.scala 31:58]
  reg [8:0] readys_mask_1; // @[Arbiter.scala 23:23]
  wire [8:0] _readys_filter_T_2 = ~readys_mask_1; // @[Arbiter.scala 24:30]
  wire [8:0] _readys_filter_T_3 = readys_valid_1 & _readys_filter_T_2; // @[Arbiter.scala 24:28]
  wire [17:0] readys_filter_1 = {_readys_filter_T_3,auto_out_8_bvalid,auto_out_7_bvalid,auto_out_6_bvalid,
    auto_out_5_bvalid,auto_out_4_bvalid,auto_out_3_bvalid,auto_out_2_bvalid,auto_out_1_bvalid,auto_out_0_bvalid}; // @[Cat.scala 31:58]
  wire [17:0] _GEN_57 = {{1'd0}, readys_filter_1[17:1]}; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_12 = readys_filter_1 | _GEN_57; // @[package.scala 253:43]
  wire [17:0] _GEN_58 = {{2'd0}, _readys_unready_T_12[17:2]}; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_14 = _readys_unready_T_12 | _GEN_58; // @[package.scala 253:43]
  wire [17:0] _GEN_59 = {{4'd0}, _readys_unready_T_14[17:4]}; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_16 = _readys_unready_T_14 | _GEN_59; // @[package.scala 253:43]
  wire [17:0] _GEN_60 = {{8'd0}, _readys_unready_T_16[17:8]}; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_18 = _readys_unready_T_16 | _GEN_60; // @[package.scala 253:43]
  wire [17:0] _readys_unready_T_21 = {readys_mask_1, 9'h0}; // @[Arbiter.scala 25:66]
  wire [17:0] _GEN_61 = {{1'd0}, _readys_unready_T_18[17:1]}; // @[Arbiter.scala 25:58]
  wire [17:0] readys_unready_1 = _GEN_61 | _readys_unready_T_21; // @[Arbiter.scala 25:58]
  wire [8:0] _readys_readys_T_5 = readys_unready_1[17:9] & readys_unready_1[8:0]; // @[Arbiter.scala 26:39]
  wire [8:0] readys_readys_1 = ~_readys_readys_T_5; // @[Arbiter.scala 26:18]
  wire  readys_10_0 = readys_readys_1[0]; // @[Xbar.scala 255:69]
  wire  winner_10_0 = readys_10_0 & auto_out_0_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_0; // @[Xbar.scala 268:24]
  wire  muxState_10_0 = idle_10 ? winner_10_0 : state_10_0; // @[Xbar.scala 269:23]
  wire  readys_10_1 = readys_readys_1[1]; // @[Xbar.scala 255:69]
  wire  winner_10_1 = readys_10_1 & auto_out_1_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_1; // @[Xbar.scala 268:24]
  wire  muxState_10_1 = idle_10 ? winner_10_1 : state_10_1; // @[Xbar.scala 269:23]
  wire  readys_10_2 = readys_readys_1[2]; // @[Xbar.scala 255:69]
  wire  winner_10_2 = readys_10_2 & auto_out_2_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_2; // @[Xbar.scala 268:24]
  wire  muxState_10_2 = idle_10 ? winner_10_2 : state_10_2; // @[Xbar.scala 269:23]
  wire  readys_10_3 = readys_readys_1[3]; // @[Xbar.scala 255:69]
  wire  winner_10_3 = readys_10_3 & auto_out_3_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_3; // @[Xbar.scala 268:24]
  wire  muxState_10_3 = idle_10 ? winner_10_3 : state_10_3; // @[Xbar.scala 269:23]
  wire  readys_10_4 = readys_readys_1[4]; // @[Xbar.scala 255:69]
  wire  winner_10_4 = readys_10_4 & auto_out_4_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_4; // @[Xbar.scala 268:24]
  wire  muxState_10_4 = idle_10 ? winner_10_4 : state_10_4; // @[Xbar.scala 269:23]
  wire  readys_10_5 = readys_readys_1[5]; // @[Xbar.scala 255:69]
  wire  winner_10_5 = readys_10_5 & auto_out_5_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_5; // @[Xbar.scala 268:24]
  wire  muxState_10_5 = idle_10 ? winner_10_5 : state_10_5; // @[Xbar.scala 269:23]
  wire  readys_10_6 = readys_readys_1[6]; // @[Xbar.scala 255:69]
  wire  winner_10_6 = readys_10_6 & auto_out_6_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_6; // @[Xbar.scala 268:24]
  wire  muxState_10_6 = idle_10 ? winner_10_6 : state_10_6; // @[Xbar.scala 269:23]
  wire  readys_10_7 = readys_readys_1[7]; // @[Xbar.scala 255:69]
  wire  winner_10_7 = readys_10_7 & auto_out_7_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_7; // @[Xbar.scala 268:24]
  wire  muxState_10_7 = idle_10 ? winner_10_7 : state_10_7; // @[Xbar.scala 269:23]
  wire  readys_10_8 = readys_readys_1[8]; // @[Xbar.scala 255:69]
  wire  winner_10_8 = readys_10_8 & auto_out_8_bvalid; // @[Xbar.scala 257:63]
  reg  state_10_8; // @[Xbar.scala 268:24]
  wire  muxState_10_8 = idle_10 ? winner_10_8 : state_10_8; // @[Xbar.scala 269:23]
  wire  anyValid = auto_out_0_rvalid | auto_out_1_rvalid | auto_out_2_rvalid | auto_out_3_rvalid |
    auto_out_4_rvalid | auto_out_5_rvalid | auto_out_6_rvalid | auto_out_7_rvalid | auto_out_8_rvalid; // @[Xbar.scala 253:36]
  wire  _in_0_rvalid_T_16 = state_9_0 & auto_out_0_rvalid | state_9_1 & auto_out_1_rvalid | state_9_2 &
    auto_out_2_rvalid | state_9_3 & auto_out_3_rvalid | state_9_4 & auto_out_4_rvalid | state_9_5 &
    auto_out_5_rvalid | state_9_6 & auto_out_6_rvalid | state_9_7 & auto_out_7_rvalid | state_9_8 &
    auto_out_8_rvalid; // @[Mux.scala 27:73]
  wire  in_0_rvalid = idle_9 ? anyValid : _in_0_rvalid_T_16; // @[Xbar.scala 285:22]
  wire  _arFIFOMap_0_T_4 = auto_in_rready & in_0_rvalid; // @[Decoupled.scala 50:35]
  wire  in_0_awready = requestAWIO_0_0 & auto_out_0_awready | requestAWIO_0_1 & auto_out_1_awready | requestAWIO_0_2
     & auto_out_2_awready | requestAWIO_0_3 & auto_out_3_awready | requestAWIO_0_4 & auto_out_4_awready |
    requestAWIO_0_5 & auto_out_5_awready | requestAWIO_0_6 & auto_out_6_awready | requestAWIO_0_7 &
    auto_out_7_awready | requestAWIO_0_8 & auto_out_8_awready; // @[Mux.scala 27:73]
  reg  latched; // @[Xbar.scala 144:30]
  wire  _bundleIn_0_awready_T = latched | awIn_0_io_enq_ready; // @[Xbar.scala 146:57]
  wire  anyValid_1 = auto_out_0_bvalid | auto_out_1_bvalid | auto_out_2_bvalid | auto_out_3_bvalid |
    auto_out_4_bvalid | auto_out_5_bvalid | auto_out_6_bvalid | auto_out_7_bvalid | auto_out_8_bvalid; // @[Xbar.scala 253:36]
  wire  _in_0_bvalid_T_16 = state_10_0 & auto_out_0_bvalid | state_10_1 & auto_out_1_bvalid | state_10_2 &
    auto_out_2_bvalid | state_10_3 & auto_out_3_bvalid | state_10_4 & auto_out_4_bvalid | state_10_5 &
    auto_out_5_bvalid | state_10_6 & auto_out_6_bvalid | state_10_7 & auto_out_7_bvalid | state_10_8 &
    auto_out_8_bvalid; // @[Mux.scala 27:73]
  wire  in_0_bvalid = idle_10 ? anyValid_1 : _in_0_bvalid_T_16; // @[Xbar.scala 285:22]
  wire  _awFIFOMap_0_T_4 = auto_in_bready & in_0_bvalid; // @[Decoupled.scala 50:35]
  wire  in_0_awvalid = auto_in_awvalid & _bundleIn_0_awready_T; // @[Xbar.scala 145:45]
  wire  _T = awIn_0_io_enq_ready & awIn_0_io_enq_valid; // @[Decoupled.scala 50:35]
  wire  _GEN_8 = _T | latched; // @[Xbar.scala 144:30 148:{38,48}]
  wire  _T_1 = in_0_awready & in_0_awvalid; // @[Decoupled.scala 50:35]
  wire  in_0_wvalid = auto_in_wvalid & awIn_0_io_deq_valid; // @[Xbar.scala 152:43]
  wire  in_0_wready = requestWIO_0_0 & auto_out_0_wready | requestWIO_0_1 & auto_out_1_wready | requestWIO_0_2 &
    auto_out_2_wready | requestWIO_0_3 & auto_out_3_wready | requestWIO_0_4 & auto_out_4_wready | requestWIO_0_5 &
    auto_out_5_wready | requestWIO_0_6 & auto_out_6_wready | requestWIO_0_7 & auto_out_7_wready | requestWIO_0_8 &
    auto_out_8_wready; // @[Mux.scala 27:73]
  wire [8:0] _readys_mask_T = readys_readys & readys_valid; // @[Arbiter.scala 28:29]
  wire [9:0] _readys_mask_T_1 = {_readys_mask_T, 1'h0}; // @[package.scala 244:48]
  wire [8:0] _readys_mask_T_3 = _readys_mask_T | _readys_mask_T_1[8:0]; // @[package.scala 244:43]
  wire [10:0] _readys_mask_T_4 = {_readys_mask_T_3, 2'h0}; // @[package.scala 244:48]
  wire [8:0] _readys_mask_T_6 = _readys_mask_T_3 | _readys_mask_T_4[8:0]; // @[package.scala 244:43]
  wire [12:0] _readys_mask_T_7 = {_readys_mask_T_6, 4'h0}; // @[package.scala 244:48]
  wire [8:0] _readys_mask_T_9 = _readys_mask_T_6 | _readys_mask_T_7[8:0]; // @[package.scala 244:43]
  wire [16:0] _readys_mask_T_10 = {_readys_mask_T_9, 8'h0}; // @[package.scala 244:48]
  wire [8:0] _readys_mask_T_12 = _readys_mask_T_9 | _readys_mask_T_10[8:0]; // @[package.scala 244:43]
  wire  _GEN_47 = anyValid ? 1'h0 : idle_9; // @[Xbar.scala 273:21 249:23 273:28]
  wire  _GEN_48 = _arFIFOMap_0_T_4 | _GEN_47; // @[Xbar.scala 274:{24,31}]
  wire  allowed__0 = idle_9 ? readys_9_0 : state_9_0; // @[Xbar.scala 277:24]
  wire  allowed__1 = idle_9 ? readys_9_1 : state_9_1; // @[Xbar.scala 277:24]
  wire  allowed__2 = idle_9 ? readys_9_2 : state_9_2; // @[Xbar.scala 277:24]
  wire  allowed__3 = idle_9 ? readys_9_3 : state_9_3; // @[Xbar.scala 277:24]
  wire  allowed__4 = idle_9 ? readys_9_4 : state_9_4; // @[Xbar.scala 277:24]
  wire  allowed__5 = idle_9 ? readys_9_5 : state_9_5; // @[Xbar.scala 277:24]
  wire  allowed__6 = idle_9 ? readys_9_6 : state_9_6; // @[Xbar.scala 277:24]
  wire  allowed__7 = idle_9 ? readys_9_7 : state_9_7; // @[Xbar.scala 277:24]
  wire  allowed__8 = idle_9 ? readys_9_8 : state_9_8; // @[Xbar.scala 277:24]
  wire [1:0] _T_188 = muxState_9_0 ? auto_out_0_rresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_189 = muxState_9_1 ? auto_out_1_rresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_190 = muxState_9_2 ? auto_out_2_rresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_192 = muxState_9_4 ? auto_out_4_rresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_193 = muxState_9_5 ? auto_out_5_rresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_194 = muxState_9_6 ? auto_out_6_rresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_195 = muxState_9_7 ? auto_out_7_rresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_196 = muxState_9_8 ? auto_out_8_rresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_197 = _T_188 | _T_189; // @[Mux.scala 27:73]
  wire [1:0] _T_198 = _T_197 | _T_190; // @[Mux.scala 27:73]
  wire [1:0] _T_200 = _T_198 | _T_192; // @[Mux.scala 27:73]
  wire [1:0] _T_201 = _T_200 | _T_193; // @[Mux.scala 27:73]
  wire [1:0] _T_202 = _T_201 | _T_194; // @[Mux.scala 27:73]
  wire [1:0] _T_203 = _T_202 | _T_195; // @[Mux.scala 27:73]
  wire [63:0] _T_205 = muxState_9_0 ? auto_out_0_rdata : 64'h0; // @[Mux.scala 27:73]
  wire [63:0] _T_206 = muxState_9_1 ? auto_out_1_rdata : 64'h0; // @[Mux.scala 27:73]
  wire [63:0] _T_207 = muxState_9_2 ? auto_out_2_rdata : 64'h0; // @[Mux.scala 27:73]
  wire [63:0] _T_209 = muxState_9_4 ? auto_out_4_rdata : 64'h0; // @[Mux.scala 27:73]
  wire [63:0] _T_210 = muxState_9_5 ? auto_out_5_rdata : 64'h0; // @[Mux.scala 27:73]
  wire [63:0] _T_211 = muxState_9_6 ? auto_out_6_rdata : 64'h0; // @[Mux.scala 27:73]
  wire [63:0] _T_212 = muxState_9_7 ? auto_out_7_rdata : 64'h0; // @[Mux.scala 27:73]
  wire [63:0] _T_213 = muxState_9_8 ? auto_out_8_rdata : 64'h0; // @[Mux.scala 27:73]
  wire [63:0] _T_214 = _T_205 | _T_206; // @[Mux.scala 27:73]
  wire [63:0] _T_215 = _T_214 | _T_207; // @[Mux.scala 27:73]
  wire [63:0] _T_217 = _T_215 | _T_209; // @[Mux.scala 27:73]
  wire [63:0] _T_218 = _T_217 | _T_210; // @[Mux.scala 27:73]
  wire [63:0] _T_219 = _T_218 | _T_211; // @[Mux.scala 27:73]
  wire [63:0] _T_220 = _T_219 | _T_212; // @[Mux.scala 27:73]
  wire [8:0] _readys_mask_T_14 = readys_readys_1 & readys_valid_1; // @[Arbiter.scala 28:29]
  wire [9:0] _readys_mask_T_15 = {_readys_mask_T_14, 1'h0}; // @[package.scala 244:48]
  wire [8:0] _readys_mask_T_17 = _readys_mask_T_14 | _readys_mask_T_15[8:0]; // @[package.scala 244:43]
  wire [10:0] _readys_mask_T_18 = {_readys_mask_T_17, 2'h0}; // @[package.scala 244:48]
  wire [8:0] _readys_mask_T_20 = _readys_mask_T_17 | _readys_mask_T_18[8:0]; // @[package.scala 244:43]
  wire [12:0] _readys_mask_T_21 = {_readys_mask_T_20, 4'h0}; // @[package.scala 244:48]
  wire [8:0] _readys_mask_T_23 = _readys_mask_T_20 | _readys_mask_T_21[8:0]; // @[package.scala 244:43]
  wire [16:0] _readys_mask_T_24 = {_readys_mask_T_23, 8'h0}; // @[package.scala 244:48]
  wire [8:0] _readys_mask_T_26 = _readys_mask_T_23 | _readys_mask_T_24[8:0]; // @[package.scala 244:43]
  wire  _GEN_50 = anyValid_1 ? 1'h0 : idle_10; // @[Xbar.scala 273:21 249:23 273:28]
  wire  _GEN_51 = _awFIFOMap_0_T_4 | _GEN_50; // @[Xbar.scala 274:{24,31}]
  wire  allowed_1_0 = idle_10 ? readys_10_0 : state_10_0; // @[Xbar.scala 277:24]
  wire  allowed_1_1 = idle_10 ? readys_10_1 : state_10_1; // @[Xbar.scala 277:24]
  wire  allowed_1_2 = idle_10 ? readys_10_2 : state_10_2; // @[Xbar.scala 277:24]
  wire  allowed_1_3 = idle_10 ? readys_10_3 : state_10_3; // @[Xbar.scala 277:24]
  wire  allowed_1_4 = idle_10 ? readys_10_4 : state_10_4; // @[Xbar.scala 277:24]
  wire  allowed_1_5 = idle_10 ? readys_10_5 : state_10_5; // @[Xbar.scala 277:24]
  wire  allowed_1_6 = idle_10 ? readys_10_6 : state_10_6; // @[Xbar.scala 277:24]
  wire  allowed_1_7 = idle_10 ? readys_10_7 : state_10_7; // @[Xbar.scala 277:24]
  wire  allowed_1_8 = idle_10 ? readys_10_8 : state_10_8; // @[Xbar.scala 277:24]
  wire [1:0] _T_291 = muxState_10_0 ? auto_out_0_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_292 = muxState_10_1 ? auto_out_1_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_293 = muxState_10_2 ? auto_out_2_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_294 = muxState_10_3 ? auto_out_3_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_295 = muxState_10_4 ? auto_out_4_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_296 = muxState_10_5 ? auto_out_5_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_297 = muxState_10_6 ? auto_out_6_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_298 = muxState_10_7 ? auto_out_7_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_299 = muxState_10_8 ? auto_out_8_bresp : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _T_300 = _T_291 | _T_292; // @[Mux.scala 27:73]
  wire [1:0] _T_301 = _T_300 | _T_293; // @[Mux.scala 27:73]
  wire [1:0] _T_302 = _T_301 | _T_294; // @[Mux.scala 27:73]
  wire [1:0] _T_303 = _T_302 | _T_295; // @[Mux.scala 27:73]
  wire [1:0] _T_304 = _T_303 | _T_296; // @[Mux.scala 27:73]
  wire [1:0] _T_305 = _T_304 | _T_297; // @[Mux.scala 27:73]
  wire [1:0] _T_306 = _T_305 | _T_298; // @[Mux.scala 27:73]
  QueueCompatibility_196 awIn_0 ( // @[Xbar.scala 62:47]
    .clock(awIn_0_clock),
    .reset(awIn_0_reset),
    .io_enq_ready(awIn_0_io_enq_ready),
    .io_enq_valid(awIn_0_io_enq_valid),
    .io_enq_bits(awIn_0_io_enq_bits),
    .io_deq_ready(awIn_0_io_deq_ready),
    .io_deq_valid(awIn_0_io_deq_valid),
    .io_deq_bits(awIn_0_io_deq_bits)
  );
  assign auto_in_awready = in_0_awready & (latched | awIn_0_io_enq_ready); // @[Xbar.scala 146:45]
  assign auto_in_wready = in_0_wready & awIn_0_io_deq_valid; // @[Xbar.scala 153:43]
  assign auto_in_bvalid = idle_10 ? anyValid_1 : _in_0_bvalid_T_16; // @[Xbar.scala 285:22]
  assign auto_in_bid = muxState_10_0 & auto_out_0_bid | muxState_10_1 & auto_out_1_bid | muxState_10_2
     & auto_out_2_bid | muxState_10_3 & auto_out_3_bid | muxState_10_4 & auto_out_4_bid |
    muxState_10_5 & auto_out_5_bid | muxState_10_6 & auto_out_6_bid | muxState_10_7 & auto_out_7_bid
     | muxState_10_8 & auto_out_8_bid; // @[Mux.scala 27:73]
  assign auto_in_bresp = _T_306 | _T_299; // @[Mux.scala 27:73]
  assign auto_in_arready = requestARIO_0_0 & auto_out_0_arready | requestARIO_0_1 & auto_out_1_arready |
    requestARIO_0_2 & auto_out_2_arready | requestARIO_0_3 | requestARIO_0_4 & auto_out_4_arready | requestARIO_0_5 &
    auto_out_5_arready | requestARIO_0_6 & auto_out_6_arready | requestARIO_0_7 & auto_out_7_arready |
    requestARIO_0_8 & auto_out_8_arready; // @[Mux.scala 27:73]
  assign auto_in_rvalid = idle_9 ? anyValid : _in_0_rvalid_T_16; // @[Xbar.scala 285:22]
  assign auto_in_rid = muxState_9_0 & auto_out_0_rid | muxState_9_1 & auto_out_1_rid | muxState_9_2 &
    auto_out_2_rid | muxState_9_3 & auto_out_3_rid | muxState_9_4 & auto_out_4_rid | muxState_9_5 &
    auto_out_5_rid | muxState_9_6 & auto_out_6_rid | muxState_9_7 & auto_out_7_rid | muxState_9_8 &
    auto_out_8_rid; // @[Mux.scala 27:73]
  assign auto_in_rdata = _T_220 | _T_213; // @[Mux.scala 27:73]
  assign auto_in_rresp = _T_203 | _T_196; // @[Mux.scala 27:73]
  assign auto_in_rlast = muxState_9_0 & auto_out_0_rlast | muxState_9_1 & auto_out_1_rlast |
    muxState_9_2 & auto_out_2_rlast | muxState_9_3 & auto_out_3_rlast | muxState_9_4 &
    auto_out_4_rlast | muxState_9_5 & auto_out_5_rlast | muxState_9_6 & auto_out_6_rlast |
    muxState_9_7 & auto_out_7_rlast | muxState_9_8 & auto_out_8_rlast; // @[Mux.scala 27:73]
  assign auto_out_8_awvalid = in_0_awvalid & requestAWIO_0_8; // @[Xbar.scala 229:40]
  assign auto_out_8_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_8_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_wvalid = in_0_wvalid & requestWIO_0_8; // @[Xbar.scala 229:40]
  assign auto_out_8_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_bready = auto_in_bready & allowed_1_8; // @[Xbar.scala 279:31]
  assign auto_out_8_arvalid = auto_in_arvalid & requestARIO_0_8; // @[Xbar.scala 229:40]
  assign auto_out_8_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_8_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_8_rready = auto_in_rready & allowed__8; // @[Xbar.scala 279:31]
  assign auto_out_7_awvalid = in_0_awvalid & requestAWIO_0_7; // @[Xbar.scala 229:40]
  assign auto_out_7_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_7_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_wvalid = in_0_wvalid & requestWIO_0_7; // @[Xbar.scala 229:40]
  assign auto_out_7_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_bready = auto_in_bready & allowed_1_7; // @[Xbar.scala 279:31]
  assign auto_out_7_arvalid = auto_in_arvalid & requestARIO_0_7; // @[Xbar.scala 229:40]
  assign auto_out_7_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_7_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_7_rready = auto_in_rready & allowed__7; // @[Xbar.scala 279:31]
  assign auto_out_6_awvalid = in_0_awvalid & requestAWIO_0_6; // @[Xbar.scala 229:40]
  assign auto_out_6_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_6_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_wvalid = in_0_wvalid & requestWIO_0_6; // @[Xbar.scala 229:40]
  assign auto_out_6_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_bready = auto_in_bready & allowed_1_6; // @[Xbar.scala 279:31]
  assign auto_out_6_arvalid = auto_in_arvalid & requestARIO_0_6; // @[Xbar.scala 229:40]
  assign auto_out_6_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_6_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_6_rready = auto_in_rready & allowed__6; // @[Xbar.scala 279:31]
  assign auto_out_5_awvalid = in_0_awvalid & requestAWIO_0_5; // @[Xbar.scala 229:40]
  assign auto_out_5_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_5_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_wvalid = in_0_wvalid & requestWIO_0_5; // @[Xbar.scala 229:40]
  assign auto_out_5_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_bready = auto_in_bready & allowed_1_5; // @[Xbar.scala 279:31]
  assign auto_out_5_arvalid = auto_in_arvalid & requestARIO_0_5; // @[Xbar.scala 229:40]
  assign auto_out_5_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_5_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_5_rready = auto_in_rready & allowed__5; // @[Xbar.scala 279:31]
  assign auto_out_4_awvalid = in_0_awvalid & requestAWIO_0_4; // @[Xbar.scala 229:40]
  assign auto_out_4_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_4_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_wvalid = in_0_wvalid & requestWIO_0_4; // @[Xbar.scala 229:40]
  assign auto_out_4_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_bready = auto_in_bready & allowed_1_4; // @[Xbar.scala 279:31]
  assign auto_out_4_arvalid = auto_in_arvalid & requestARIO_0_4; // @[Xbar.scala 229:40]
  assign auto_out_4_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_4_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_4_rready = auto_in_rready & allowed__4; // @[Xbar.scala 279:31]
  assign auto_out_3_awvalid = in_0_awvalid & requestAWIO_0_3; // @[Xbar.scala 229:40]
  assign auto_out_3_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_3_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_wvalid = in_0_wvalid & requestWIO_0_3; // @[Xbar.scala 229:40]
  assign auto_out_3_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_bready = auto_in_bready & allowed_1_3; // @[Xbar.scala 279:31]
  assign auto_out_3_arvalid = auto_in_arvalid & requestARIO_0_3; // @[Xbar.scala 229:40]
  assign auto_out_3_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_3_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_3_rready = auto_in_rready & allowed__3; // @[Xbar.scala 279:31]
  assign auto_out_2_awvalid = in_0_awvalid & requestAWIO_0_2; // @[Xbar.scala 229:40]
  assign auto_out_2_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_2_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_wvalid = in_0_wvalid & requestWIO_0_2; // @[Xbar.scala 229:40]
  assign auto_out_2_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_bready = auto_in_bready & allowed_1_2; // @[Xbar.scala 279:31]
  assign auto_out_2_arvalid = auto_in_arvalid & requestARIO_0_2; // @[Xbar.scala 229:40]
  assign auto_out_2_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_2_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_2_rready = auto_in_rready & allowed__2; // @[Xbar.scala 279:31]
  assign auto_out_1_awvalid = in_0_awvalid & requestAWIO_0_1; // @[Xbar.scala 229:40]
  assign auto_out_1_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_1_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_wvalid = in_0_wvalid & requestWIO_0_1; // @[Xbar.scala 229:40]
  assign auto_out_1_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_bready = auto_in_bready & allowed_1_1; // @[Xbar.scala 279:31]
  assign auto_out_1_arvalid = auto_in_arvalid & requestARIO_0_1; // @[Xbar.scala 229:40]
  assign auto_out_1_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_1_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_rready = auto_in_rready & allowed__1; // @[Xbar.scala 279:31]
  assign auto_out_0_awvalid = in_0_awvalid & requestAWIO_0_0; // @[Xbar.scala 229:40]
  assign auto_out_0_awid = auto_in_awid; // @[Xbar.scala 86:47]
  assign auto_out_0_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_wvalid = in_0_wvalid & requestWIO_0_0; // @[Xbar.scala 229:40]
  assign auto_out_0_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_bready = auto_in_bready & allowed_1_0; // @[Xbar.scala 279:31]
  assign auto_out_0_arvalid = auto_in_arvalid & requestARIO_0_0; // @[Xbar.scala 229:40]
  assign auto_out_0_arid = auto_in_arid; // @[Xbar.scala 87:47]
  assign auto_out_0_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_rready = auto_in_rready & allowed__0; // @[Xbar.scala 279:31]
  assign awIn_0_clock = clock;
  assign awIn_0_reset = reset;
  assign awIn_0_io_enq_valid = auto_in_awvalid & ~latched; // @[Xbar.scala 147:51]
  assign awIn_0_io_enq_bits = {awIn_0_io_enq_bits_hi,awIn_0_io_enq_bits_lo}; // @[Xbar.scala 71:75]
  assign awIn_0_io_deq_ready = auto_in_wvalid & auto_in_wlast & in_0_wready; // @[Xbar.scala 154:74]
  always @(posedge clock) begin
    idle_9 <= reset | _GEN_48; // @[Xbar.scala 249:{23,23}]
    if (reset) begin // @[Arbiter.scala 23:23]
      readys_mask <= 9'h1ff; // @[Arbiter.scala 23:23]
    end else if (idle_9 & |readys_valid) begin // @[Arbiter.scala 27:32]
      readys_mask <= _readys_mask_T_12; // @[Arbiter.scala 28:12]
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_0 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_0 <= winner_9_0;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_1 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_1 <= winner_9_1;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_2 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_2 <= winner_9_2;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_3 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_3 <= winner_9_3;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_4 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_4 <= winner_9_4;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_5 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_5 <= winner_9_5;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_6 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_6 <= winner_9_6;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_7 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_7 <= winner_9_7;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_9_8 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_9) begin // @[Xbar.scala 269:23]
      state_9_8 <= winner_9_8;
    end
    idle_10 <= reset | _GEN_51; // @[Xbar.scala 249:{23,23}]
    if (reset) begin // @[Arbiter.scala 23:23]
      readys_mask_1 <= 9'h1ff; // @[Arbiter.scala 23:23]
    end else if (idle_10 & |readys_valid_1) begin // @[Arbiter.scala 27:32]
      readys_mask_1 <= _readys_mask_T_26; // @[Arbiter.scala 28:12]
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_0 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_0 <= winner_10_0;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_1 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_1 <= winner_10_1;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_2 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_2 <= winner_10_2;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_3 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_3 <= winner_10_3;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_4 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_4 <= winner_10_4;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_5 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_5 <= winner_10_5;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_6 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_6 <= winner_10_6;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_7 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_7 <= winner_10_7;
    end
    if (reset) begin // @[Xbar.scala 268:24]
      state_10_8 <= 1'h0; // @[Xbar.scala 268:24]
    end else if (idle_10) begin // @[Xbar.scala 269:23]
      state_10_8 <= winner_10_8;
    end
    if (reset) begin // @[Xbar.scala 144:30]
      latched <= 1'h0; // @[Xbar.scala 144:30]
    end else if (_T_1) begin // @[Xbar.scala 149:32]
      latched <= 1'h0; // @[Xbar.scala 149:42]
    end else begin
      latched <= _GEN_8;
    end
  end
// Register and memory initialization
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE_MEM_INIT
  integer initvar;
`endif
`ifndef SYNTHESIS
`ifdef FIRRTL_BEFORE_INITIAL
`FIRRTL_BEFORE_INITIAL
`endif
initial begin
  `ifdef RANDOMIZE
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      `ifdef RANDOMIZE_DELAY
        #`RANDOMIZE_DELAY begin end
      `else
        #0.002 begin end
      `endif
    `endif
`ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  idle_9 = _RAND_0[0:0];
  _RAND_1 = {1{`RANDOM}};
  readys_mask = _RAND_1[8:0];
  _RAND_2 = {1{`RANDOM}};
  state_9_0 = _RAND_2[0:0];
  _RAND_3 = {1{`RANDOM}};
  state_9_1 = _RAND_3[0:0];
  _RAND_4 = {1{`RANDOM}};
  state_9_2 = _RAND_4[0:0];
  _RAND_5 = {1{`RANDOM}};
  state_9_3 = _RAND_5[0:0];
  _RAND_6 = {1{`RANDOM}};
  state_9_4 = _RAND_6[0:0];
  _RAND_7 = {1{`RANDOM}};
  state_9_5 = _RAND_7[0:0];
  _RAND_8 = {1{`RANDOM}};
  state_9_6 = _RAND_8[0:0];
  _RAND_9 = {1{`RANDOM}};
  state_9_7 = _RAND_9[0:0];
  _RAND_10 = {1{`RANDOM}};
  state_9_8 = _RAND_10[0:0];
  _RAND_11 = {1{`RANDOM}};
  idle_10 = _RAND_11[0:0];
  _RAND_12 = {1{`RANDOM}};
  readys_mask_1 = _RAND_12[8:0];
  _RAND_13 = {1{`RANDOM}};
  state_10_0 = _RAND_13[0:0];
  _RAND_14 = {1{`RANDOM}};
  state_10_1 = _RAND_14[0:0];
  _RAND_15 = {1{`RANDOM}};
  state_10_2 = _RAND_15[0:0];
  _RAND_16 = {1{`RANDOM}};
  state_10_3 = _RAND_16[0:0];
  _RAND_17 = {1{`RANDOM}};
  state_10_4 = _RAND_17[0:0];
  _RAND_18 = {1{`RANDOM}};
  state_10_5 = _RAND_18[0:0];
  _RAND_19 = {1{`RANDOM}};
  state_10_6 = _RAND_19[0:0];
  _RAND_20 = {1{`RANDOM}};
  state_10_7 = _RAND_20[0:0];
  _RAND_21 = {1{`RANDOM}};
  state_10_8 = _RAND_21[0:0];
  _RAND_22 = {1{`RANDOM}};
  latched = _RAND_22[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

