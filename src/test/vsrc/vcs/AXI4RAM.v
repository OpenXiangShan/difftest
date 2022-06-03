module AXI4RAM(
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
  input         auto_in_arvalid,
  input         auto_in_arid,
  input  [36:0] auto_in_araddr,
  input         auto_in_arlock,
  input  [3:0]  auto_in_arcache,
  input  [3:0]  auto_in_arqos,
  output        auto_in_rid,
  output        auto_in_rlast
);
`ifdef RANDOMIZE_GARBAGE_ASSIGN
  reg [31:0] _RAND_1;
  reg [31:0] _RAND_3;
  reg [31:0] _RAND_5;
  reg [31:0] _RAND_7;
  reg [31:0] _RAND_9;
  reg [31:0] _RAND_11;
  reg [31:0] _RAND_13;
  reg [31:0] _RAND_15;
`endif // RANDOMIZE_GARBAGE_ASSIGN
`ifdef RANDOMIZE_MEM_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_2;
  reg [31:0] _RAND_4;
  reg [31:0] _RAND_6;
  reg [31:0] _RAND_8;
  reg [31:0] _RAND_10;
  reg [31:0] _RAND_12;
  reg [31:0] _RAND_14;
`endif // RANDOMIZE_MEM_INIT
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_16;
  reg [31:0] _RAND_17;
  reg [31:0] _RAND_18;
  reg [63:0] _RAND_19;
  reg [31:0] _RAND_20;
  reg [63:0] _RAND_21;
  reg [31:0] _RAND_22;
  reg [31:0] _RAND_23;
`endif // RANDOMIZE_REG_INIT
  reg [7:0] rdata_mem_0 [0:59999]; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_0_rdata_MPORT_1_en; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_0_rdata_MPORT_1_addr; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_0_rdata_MPORT_1_data; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_0_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_0_rdata_MPORT_addr; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_0_rdata_MPORT_mask; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_0_rdata_MPORT_en; // @[AXI4RAM.scala 83:20]
  reg [7:0] rdata_mem_1 [0:59999]; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_1_rdata_MPORT_1_en; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_1_rdata_MPORT_1_addr; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_1_rdata_MPORT_1_data; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_1_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_1_rdata_MPORT_addr; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_1_rdata_MPORT_mask; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_1_rdata_MPORT_en; // @[AXI4RAM.scala 83:20]
  reg [7:0] rdata_mem_2 [0:59999]; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_2_rdata_MPORT_1_en; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_2_rdata_MPORT_1_addr; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_2_rdata_MPORT_1_data; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_2_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_2_rdata_MPORT_addr; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_2_rdata_MPORT_mask; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_2_rdata_MPORT_en; // @[AXI4RAM.scala 83:20]
  reg [7:0] rdata_mem_3 [0:59999]; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_3_rdata_MPORT_1_en; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_3_rdata_MPORT_1_addr; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_3_rdata_MPORT_1_data; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_3_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_3_rdata_MPORT_addr; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_3_rdata_MPORT_mask; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_3_rdata_MPORT_en; // @[AXI4RAM.scala 83:20]
  reg [7:0] rdata_mem_4 [0:59999]; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_4_rdata_MPORT_1_en; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_4_rdata_MPORT_1_addr; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_4_rdata_MPORT_1_data; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_4_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_4_rdata_MPORT_addr; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_4_rdata_MPORT_mask; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_4_rdata_MPORT_en; // @[AXI4RAM.scala 83:20]
  reg [7:0] rdata_mem_5 [0:59999]; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_5_rdata_MPORT_1_en; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_5_rdata_MPORT_1_addr; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_5_rdata_MPORT_1_data; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_5_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_5_rdata_MPORT_addr; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_5_rdata_MPORT_mask; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_5_rdata_MPORT_en; // @[AXI4RAM.scala 83:20]
  reg [7:0] rdata_mem_6 [0:59999]; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_6_rdata_MPORT_1_en; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_6_rdata_MPORT_1_addr; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_6_rdata_MPORT_1_data; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_6_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_6_rdata_MPORT_addr; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_6_rdata_MPORT_mask; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_6_rdata_MPORT_en; // @[AXI4RAM.scala 83:20]
  reg [7:0] rdata_mem_7 [0:59999]; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_7_rdata_MPORT_1_en; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_7_rdata_MPORT_1_addr; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_7_rdata_MPORT_1_data; // @[AXI4RAM.scala 83:20]
  wire [7:0] rdata_mem_7_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
  wire [15:0] rdata_mem_7_rdata_MPORT_addr; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_7_rdata_MPORT_mask; // @[AXI4RAM.scala 83:20]
  wire  rdata_mem_7_rdata_MPORT_en; // @[AXI4RAM.scala 83:20]
  reg [1:0] state; // @[AXI4SlaveModule.scala 95:22]
  wire  _bundleIn_0_arready_T = state == 2'h0; // @[AXI4SlaveModule.scala 153:24]
  wire  in_arready = state == 2'h0; // @[AXI4SlaveModule.scala 153:24]
  wire  in_arvalid = auto_in_arvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  _T = in_arready & in_arvalid; // @[Decoupled.scala 50:35]
  wire  in_awready = _bundleIn_0_arready_T & ~in_arvalid; // @[AXI4SlaveModule.scala 171:35]
  wire  in_awvalid = auto_in_awvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  _T_1 = in_awready & in_awvalid; // @[Decoupled.scala 50:35]
  wire  in_wready = state == 2'h2; // @[AXI4SlaveModule.scala 172:23]
  wire  in_wvalid = auto_in_wvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  _T_2 = in_wready & in_wvalid; // @[Decoupled.scala 50:35]
  wire  in_bready = auto_in_bready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_bvalid = state == 2'h3; // @[AXI4SlaveModule.scala 175:22]
  wire  _T_3 = in_bready & in_bvalid; // @[Decoupled.scala 50:35]
  wire  in_rready = 1'h1; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_rvalid = state == 2'h1; // @[AXI4SlaveModule.scala 155:23]
  wire  _T_4 = in_rready & in_rvalid; // @[Decoupled.scala 50:35]
  wire [1:0] in_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [1:0] in_arburst = 2'h1; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg [7:0] value; // @[Counter.scala 62:40]
  wire [7:0] in_arlen = 8'h0; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg [7:0] hold_data; // @[Reg.scala 16:16]
  wire [7:0] _T_27 = _T ? in_arlen : hold_data; // @[Hold.scala 25:8]
  wire  in_rlast = value == _T_27; // @[AXI4SlaveModule.scala 133:32]
  wire  in_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [1:0] _GEN_3 = _T_2 & in_wlast ? 2'h3 : state; // @[AXI4SlaveModule.scala 112:42 113:15 95:22]
  wire [1:0] _GEN_4 = _T_3 ? 2'h0 : state; // @[AXI4SlaveModule.scala 117:24 118:15 95:22]
  wire [1:0] _GEN_5 = 2'h3 == state ? _GEN_4 : state; // @[AXI4SlaveModule.scala 97:16 95:22]
  wire [7:0] in_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg [36:0] raddr_hold_data; // @[Reg.scala 16:16]
  wire [36:0] in_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [36:0] _GEN_10 = _T ? in_araddr : raddr_hold_data; // @[Reg.scala 16:16 17:{18,22}]
  wire [7:0] _value_T_1 = value + 8'h1; // @[Counter.scala 78:24]
  reg [7:0] value_1; // @[Counter.scala 62:40]
  reg [36:0] waddr_hold_data; // @[Reg.scala 16:16]
  wire [36:0] in_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [36:0] _GEN_13 = _T_1 ? in_awaddr : waddr_hold_data; // @[Reg.scala 16:16 17:{18,22}]
  wire [7:0] _value_T_3 = value_1 + 8'h1; // @[Counter.scala 78:24]
  reg  bundleIn_0_bid_r; // @[Reg.scala 16:16]
  wire  in_awid = auto_in_awid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg  bundleIn_0_rid_r; // @[Reg.scala 16:16]
  wire  in_arid = auto_in_arid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [36:0] _wIdx_T_1 = _GEN_13 - 37'h1f50000000; // @[AXI4RAM.scala 60:36]
  wire [15:0] _GEN_53 = {{8'd0}, value_1}; // @[AXI4RAM.scala 64:29]
  wire [15:0] wIdx = _wIdx_T_1[18:3] + _GEN_53; // @[AXI4RAM.scala 64:29]
  wire [36:0] _rIdx_T_1 = _GEN_10 - 37'h1f50000000; // @[AXI4RAM.scala 60:36]
  wire [15:0] _GEN_54 = {{8'd0}, value}; // @[AXI4RAM.scala 65:29]
  wire  _wen_T_1 = wIdx < 16'hea60; // @[AXI4RAM.scala 62:34]
  wire [63:0] in_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [63:0] rdata = {rdata_mem_7_rdata_MPORT_1_data,rdata_mem_6_rdata_MPORT_1_data,rdata_mem_5_rdata_MPORT_1_data,
    rdata_mem_4_rdata_MPORT_1_data,rdata_mem_3_rdata_MPORT_1_data,rdata_mem_2_rdata_MPORT_1_data,
    rdata_mem_1_rdata_MPORT_1_data,rdata_mem_0_rdata_MPORT_1_data}; // @[Cat.scala 31:58]
  wire [7:0] in_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_bid = bundleIn_0_bid_r; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 177:16]
  wire [1:0] in_bresp = 2'h0; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 174:18]
  wire [2:0] in_arsize = 3'h3; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_arprot = 3'h0; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_rid = bundleIn_0_rid_r; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 179:16]
  wire [63:0] in_rdata = rdata; // @[Cat.scala 31:58]
  wire [1:0] in_rresp = 2'h0; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 154:18]
  assign rdata_mem_0_rdata_MPORT_1_en = 1'h1;
  assign rdata_mem_0_rdata_MPORT_1_addr = _rIdx_T_1[18:3] + _GEN_54;
  `ifndef RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_0_rdata_MPORT_1_data = rdata_mem_0[rdata_mem_0_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `else
  assign rdata_mem_0_rdata_MPORT_1_data = rdata_mem_0_rdata_MPORT_1_addr >= 16'hea60 ? _RAND_1[7:0] :
    rdata_mem_0[rdata_mem_0_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `endif // RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_0_rdata_MPORT_data = in_wdata[7:0];
  assign rdata_mem_0_rdata_MPORT_addr = _wIdx_T_1[18:3] + _GEN_53;
  assign rdata_mem_0_rdata_MPORT_mask = in_wstrb[0];
  assign rdata_mem_0_rdata_MPORT_en = _T_2 & _wen_T_1;
  assign rdata_mem_1_rdata_MPORT_1_en = 1'h1;
  assign rdata_mem_1_rdata_MPORT_1_addr = _rIdx_T_1[18:3] + _GEN_54;
  `ifndef RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_1_rdata_MPORT_1_data = rdata_mem_1[rdata_mem_1_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `else
  assign rdata_mem_1_rdata_MPORT_1_data = rdata_mem_1_rdata_MPORT_1_addr >= 16'hea60 ? _RAND_3[7:0] :
    rdata_mem_1[rdata_mem_1_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `endif // RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_1_rdata_MPORT_data = in_wdata[15:8];
  assign rdata_mem_1_rdata_MPORT_addr = _wIdx_T_1[18:3] + _GEN_53;
  assign rdata_mem_1_rdata_MPORT_mask = in_wstrb[1];
  assign rdata_mem_1_rdata_MPORT_en = _T_2 & _wen_T_1;
  assign rdata_mem_2_rdata_MPORT_1_en = 1'h1;
  assign rdata_mem_2_rdata_MPORT_1_addr = _rIdx_T_1[18:3] + _GEN_54;
  `ifndef RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_2_rdata_MPORT_1_data = rdata_mem_2[rdata_mem_2_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `else
  assign rdata_mem_2_rdata_MPORT_1_data = rdata_mem_2_rdata_MPORT_1_addr >= 16'hea60 ? _RAND_5[7:0] :
    rdata_mem_2[rdata_mem_2_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `endif // RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_2_rdata_MPORT_data = in_wdata[23:16];
  assign rdata_mem_2_rdata_MPORT_addr = _wIdx_T_1[18:3] + _GEN_53;
  assign rdata_mem_2_rdata_MPORT_mask = in_wstrb[2];
  assign rdata_mem_2_rdata_MPORT_en = _T_2 & _wen_T_1;
  assign rdata_mem_3_rdata_MPORT_1_en = 1'h1;
  assign rdata_mem_3_rdata_MPORT_1_addr = _rIdx_T_1[18:3] + _GEN_54;
  `ifndef RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_3_rdata_MPORT_1_data = rdata_mem_3[rdata_mem_3_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `else
  assign rdata_mem_3_rdata_MPORT_1_data = rdata_mem_3_rdata_MPORT_1_addr >= 16'hea60 ? _RAND_7[7:0] :
    rdata_mem_3[rdata_mem_3_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `endif // RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_3_rdata_MPORT_data = in_wdata[31:24];
  assign rdata_mem_3_rdata_MPORT_addr = _wIdx_T_1[18:3] + _GEN_53;
  assign rdata_mem_3_rdata_MPORT_mask = in_wstrb[3];
  assign rdata_mem_3_rdata_MPORT_en = _T_2 & _wen_T_1;
  assign rdata_mem_4_rdata_MPORT_1_en = 1'h1;
  assign rdata_mem_4_rdata_MPORT_1_addr = _rIdx_T_1[18:3] + _GEN_54;
  `ifndef RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_4_rdata_MPORT_1_data = rdata_mem_4[rdata_mem_4_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `else
  assign rdata_mem_4_rdata_MPORT_1_data = rdata_mem_4_rdata_MPORT_1_addr >= 16'hea60 ? _RAND_9[7:0] :
    rdata_mem_4[rdata_mem_4_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `endif // RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_4_rdata_MPORT_data = in_wdata[39:32];
  assign rdata_mem_4_rdata_MPORT_addr = _wIdx_T_1[18:3] + _GEN_53;
  assign rdata_mem_4_rdata_MPORT_mask = in_wstrb[4];
  assign rdata_mem_4_rdata_MPORT_en = _T_2 & _wen_T_1;
  assign rdata_mem_5_rdata_MPORT_1_en = 1'h1;
  assign rdata_mem_5_rdata_MPORT_1_addr = _rIdx_T_1[18:3] + _GEN_54;
  `ifndef RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_5_rdata_MPORT_1_data = rdata_mem_5[rdata_mem_5_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `else
  assign rdata_mem_5_rdata_MPORT_1_data = rdata_mem_5_rdata_MPORT_1_addr >= 16'hea60 ? _RAND_11[7:0] :
    rdata_mem_5[rdata_mem_5_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `endif // RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_5_rdata_MPORT_data = in_wdata[47:40];
  assign rdata_mem_5_rdata_MPORT_addr = _wIdx_T_1[18:3] + _GEN_53;
  assign rdata_mem_5_rdata_MPORT_mask = in_wstrb[5];
  assign rdata_mem_5_rdata_MPORT_en = _T_2 & _wen_T_1;
  assign rdata_mem_6_rdata_MPORT_1_en = 1'h1;
  assign rdata_mem_6_rdata_MPORT_1_addr = _rIdx_T_1[18:3] + _GEN_54;
  `ifndef RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_6_rdata_MPORT_1_data = rdata_mem_6[rdata_mem_6_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `else
  assign rdata_mem_6_rdata_MPORT_1_data = rdata_mem_6_rdata_MPORT_1_addr >= 16'hea60 ? _RAND_13[7:0] :
    rdata_mem_6[rdata_mem_6_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `endif // RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_6_rdata_MPORT_data = in_wdata[55:48];
  assign rdata_mem_6_rdata_MPORT_addr = _wIdx_T_1[18:3] + _GEN_53;
  assign rdata_mem_6_rdata_MPORT_mask = in_wstrb[6];
  assign rdata_mem_6_rdata_MPORT_en = _T_2 & _wen_T_1;
  assign rdata_mem_7_rdata_MPORT_1_en = 1'h1;
  assign rdata_mem_7_rdata_MPORT_1_addr = _rIdx_T_1[18:3] + _GEN_54;
  `ifndef RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_7_rdata_MPORT_1_data = rdata_mem_7[rdata_mem_7_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `else
  assign rdata_mem_7_rdata_MPORT_1_data = rdata_mem_7_rdata_MPORT_1_addr >= 16'hea60 ? _RAND_15[7:0] :
    rdata_mem_7[rdata_mem_7_rdata_MPORT_1_addr]; // @[AXI4RAM.scala 83:20]
  `endif // RANDOMIZE_GARBAGE_ASSIGN
  assign rdata_mem_7_rdata_MPORT_data = in_wdata[63:56];
  assign rdata_mem_7_rdata_MPORT_addr = _wIdx_T_1[18:3] + _GEN_53;
  assign rdata_mem_7_rdata_MPORT_mask = in_wstrb[7];
  assign rdata_mem_7_rdata_MPORT_en = _T_2 & _wen_T_1;
  assign auto_in_awready = in_awready; // @[LazyModule.scala 309:16]
  assign auto_in_wready = in_wready; // @[LazyModule.scala 309:16]
  assign auto_in_bvalid = in_bvalid; // @[LazyModule.scala 309:16]
  assign auto_in_bid = in_bid; // @[LazyModule.scala 309:16]
  assign auto_in_bresp = in_bresp; // @[LazyModule.scala 309:16]
  assign auto_in_rid = in_rid; // @[LazyModule.scala 309:16]
  assign auto_in_rlast = in_rlast; // @[LazyModule.scala 309:16]
  always @(posedge clock) begin
    if (rdata_mem_0_rdata_MPORT_en & rdata_mem_0_rdata_MPORT_mask) begin
      rdata_mem_0[rdata_mem_0_rdata_MPORT_addr] <= rdata_mem_0_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
    end
    if (rdata_mem_1_rdata_MPORT_en & rdata_mem_1_rdata_MPORT_mask) begin
      rdata_mem_1[rdata_mem_1_rdata_MPORT_addr] <= rdata_mem_1_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
    end
    if (rdata_mem_2_rdata_MPORT_en & rdata_mem_2_rdata_MPORT_mask) begin
      rdata_mem_2[rdata_mem_2_rdata_MPORT_addr] <= rdata_mem_2_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
    end
    if (rdata_mem_3_rdata_MPORT_en & rdata_mem_3_rdata_MPORT_mask) begin
      rdata_mem_3[rdata_mem_3_rdata_MPORT_addr] <= rdata_mem_3_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
    end
    if (rdata_mem_4_rdata_MPORT_en & rdata_mem_4_rdata_MPORT_mask) begin
      rdata_mem_4[rdata_mem_4_rdata_MPORT_addr] <= rdata_mem_4_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
    end
    if (rdata_mem_5_rdata_MPORT_en & rdata_mem_5_rdata_MPORT_mask) begin
      rdata_mem_5[rdata_mem_5_rdata_MPORT_addr] <= rdata_mem_5_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
    end
    if (rdata_mem_6_rdata_MPORT_en & rdata_mem_6_rdata_MPORT_mask) begin
      rdata_mem_6[rdata_mem_6_rdata_MPORT_addr] <= rdata_mem_6_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
    end
    if (rdata_mem_7_rdata_MPORT_en & rdata_mem_7_rdata_MPORT_mask) begin
      rdata_mem_7[rdata_mem_7_rdata_MPORT_addr] <= rdata_mem_7_rdata_MPORT_data; // @[AXI4RAM.scala 83:20]
    end
    if (reset) begin // @[AXI4SlaveModule.scala 95:22]
      state <= 2'h0; // @[AXI4SlaveModule.scala 95:22]
    end else if (2'h0 == state) begin // @[AXI4SlaveModule.scala 97:16]
      if (_T_1) begin // @[AXI4SlaveModule.scala 102:25]
        state <= 2'h2; // @[AXI4SlaveModule.scala 103:15]
      end else if (_T) begin // @[AXI4SlaveModule.scala 99:25]
        state <= 2'h1; // @[AXI4SlaveModule.scala 100:15]
      end
    end else if (2'h1 == state) begin // @[AXI4SlaveModule.scala 97:16]
      if (_T_4 & in_rlast) begin // @[AXI4SlaveModule.scala 107:42]
        state <= 2'h0; // @[AXI4SlaveModule.scala 108:15]
      end
    end else if (2'h2 == state) begin // @[AXI4SlaveModule.scala 97:16]
      state <= _GEN_3;
    end else begin
      state <= _GEN_5;
    end
    if (reset) begin // @[Counter.scala 62:40]
      value <= 8'h0; // @[Counter.scala 62:40]
    end else if (_T_4) begin // @[AXI4SlaveModule.scala 135:23]
      if (in_rlast) begin // @[AXI4SlaveModule.scala 137:28]
        value <= 8'h0; // @[AXI4SlaveModule.scala 138:17]
      end else begin
        value <= _value_T_1; // @[Counter.scala 78:15]
      end
    end
    if (_T) begin // @[Reg.scala 17:18]
      hold_data <= in_arlen; // @[Reg.scala 17:22]
    end
    if (_T) begin // @[Reg.scala 17:18]
      raddr_hold_data <= in_araddr; // @[Reg.scala 17:22]
    end
    if (reset) begin // @[Counter.scala 62:40]
      value_1 <= 8'h0; // @[Counter.scala 62:40]
    end else if (_T_2) begin // @[AXI4SlaveModule.scala 162:23]
      if (in_wlast) begin // @[AXI4SlaveModule.scala 164:28]
        value_1 <= 8'h0; // @[AXI4SlaveModule.scala 165:17]
      end else begin
        value_1 <= _value_T_3; // @[Counter.scala 78:15]
      end
    end
    if (_T_1) begin // @[Reg.scala 17:18]
      waddr_hold_data <= in_awaddr; // @[Reg.scala 17:22]
    end
    if (_T_1) begin // @[Reg.scala 17:18]
      bundleIn_0_bid_r <= in_awid; // @[Reg.scala 17:22]
    end
    if (_T) begin // @[Reg.scala 17:18]
      bundleIn_0_rid_r <= in_arid; // @[Reg.scala 17:22]
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
`ifdef RANDOMIZE_GARBAGE_ASSIGN
  _RAND_1 = {1{`RANDOM}};
  _RAND_3 = {1{`RANDOM}};
  _RAND_5 = {1{`RANDOM}};
  _RAND_7 = {1{`RANDOM}};
  _RAND_9 = {1{`RANDOM}};
  _RAND_11 = {1{`RANDOM}};
  _RAND_13 = {1{`RANDOM}};
  _RAND_15 = {1{`RANDOM}};
`endif // RANDOMIZE_GARBAGE_ASSIGN
`ifdef RANDOMIZE_MEM_INIT
  _RAND_0 = {1{`RANDOM}};
  for (initvar = 0; initvar < 60000; initvar = initvar+1)
    rdata_mem_0[initvar] = _RAND_0[7:0];
  _RAND_2 = {1{`RANDOM}};
  for (initvar = 0; initvar < 60000; initvar = initvar+1)
    rdata_mem_1[initvar] = _RAND_2[7:0];
  _RAND_4 = {1{`RANDOM}};
  for (initvar = 0; initvar < 60000; initvar = initvar+1)
    rdata_mem_2[initvar] = _RAND_4[7:0];
  _RAND_6 = {1{`RANDOM}};
  for (initvar = 0; initvar < 60000; initvar = initvar+1)
    rdata_mem_3[initvar] = _RAND_6[7:0];
  _RAND_8 = {1{`RANDOM}};
  for (initvar = 0; initvar < 60000; initvar = initvar+1)
    rdata_mem_4[initvar] = _RAND_8[7:0];
  _RAND_10 = {1{`RANDOM}};
  for (initvar = 0; initvar < 60000; initvar = initvar+1)
    rdata_mem_5[initvar] = _RAND_10[7:0];
  _RAND_12 = {1{`RANDOM}};
  for (initvar = 0; initvar < 60000; initvar = initvar+1)
    rdata_mem_6[initvar] = _RAND_12[7:0];
  _RAND_14 = {1{`RANDOM}};
  for (initvar = 0; initvar < 60000; initvar = initvar+1)
    rdata_mem_7[initvar] = _RAND_14[7:0];
`endif // RANDOMIZE_MEM_INIT
`ifdef RANDOMIZE_REG_INIT
  _RAND_16 = {1{`RANDOM}};
  state = _RAND_16[1:0];
  _RAND_17 = {1{`RANDOM}};
  value = _RAND_17[7:0];
  _RAND_18 = {1{`RANDOM}};
  hold_data = _RAND_18[7:0];
  _RAND_19 = {2{`RANDOM}};
  raddr_hold_data = _RAND_19[36:0];
  _RAND_20 = {1{`RANDOM}};
  value_1 = _RAND_20[7:0];
  _RAND_21 = {2{`RANDOM}};
  waddr_hold_data = _RAND_21[36:0];
  _RAND_22 = {1{`RANDOM}};
  bundleIn_0_bid_r = _RAND_22[0:0];
  _RAND_23 = {1{`RANDOM}};
  bundleIn_0_rid_r = _RAND_23[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

