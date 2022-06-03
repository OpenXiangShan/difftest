module AXI4DummySD(
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
  output        auto_in_rlast
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
  reg [31:0] _RAND_2;
  reg [63:0] _RAND_3;
  reg [63:0] _RAND_4;
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
`endif // RANDOMIZE_REG_INIT
  wire  sdHelper_clk; // @[AXI4DummySD.scala 123:26]
  wire  sdHelper_ren; // @[AXI4DummySD.scala 123:26]
  wire [31:0] sdHelper_data; // @[AXI4DummySD.scala 123:26]
  wire  sdHelper_setAddr; // @[AXI4DummySD.scala 123:26]
  wire [31:0] sdHelper_addr; // @[AXI4DummySD.scala 123:26]
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
  wire  in_rready = auto_in_rready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_rvalid = state == 2'h1; // @[AXI4SlaveModule.scala 155:23]
  wire  _T_4 = in_rready & in_rvalid; // @[Decoupled.scala 50:35]
  wire [1:0] in_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [1:0] in_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg [7:0] value; // @[Counter.scala 62:40]
  wire [7:0] in_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
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
  reg [36:0] waddr_hold_data; // @[Reg.scala 16:16]
  wire [36:0] in_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [36:0] _GEN_13 = _T_1 ? in_awaddr : waddr_hold_data; // @[Reg.scala 16:16 17:{18,22}]
  reg  bundleIn_0_bid_r; // @[Reg.scala 16:16]
  wire  in_awid = auto_in_awid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg  bundleIn_0_rid_r; // @[Reg.scala 16:16]
  wire  in_arid = auto_in_arid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg [31:0] regs_0; // @[AXI4DummySD.scala 81:45]
  reg [31:0] regs_1; // @[AXI4DummySD.scala 81:45]
  reg [31:0] regs_4; // @[AXI4DummySD.scala 81:45]
  reg [31:0] regs_5; // @[AXI4DummySD.scala 81:45]
  reg [31:0] regs_6; // @[AXI4DummySD.scala 81:45]
  reg [31:0] regs_7; // @[AXI4DummySD.scala 81:45]
  reg [31:0] regs_8; // @[AXI4DummySD.scala 81:45]
  reg [31:0] regs_15; // @[AXI4DummySD.scala 81:45]
  reg [31:0] regs_20; // @[AXI4DummySD.scala 81:45]
  wire [3:0] strb = _GEN_13[2] ? in_wstrb[7:4] : in_wstrb[3:0]; // @[AXI4DummySD.scala 148:19]
  wire [63:0] in_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [7:0] _T_52 = strb[0] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_54 = strb[1] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_56 = strb[2] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_58 = strb[3] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [31:0] _T_59 = {_T_58,_T_56,_T_54,_T_52}; // @[Cat.scala 31:58]
  wire  _rdata_T = 13'h0 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_1 = 13'h38 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_2 = 13'h18 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_3 = 13'h34 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_4 = 13'h14 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_5 = 13'h1c == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_6 = 13'h20 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_7 = 13'h40 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_8 = 13'h50 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_9 = 13'h10 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire  _rdata_T_10 = 13'h4 == _GEN_10[12:0]; // @[LookupTree.scala 24:34]
  wire [31:0] _rdata_T_11 = _rdata_T ? regs_0 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_12 = _rdata_T_1 ? regs_15 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_13 = _rdata_T_2 ? regs_6 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_14 = _rdata_T_3 ? 32'h80 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_15 = _rdata_T_4 ? regs_5 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_16 = _rdata_T_5 ? regs_7 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_17 = _rdata_T_6 ? regs_8 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_18 = _rdata_T_7 ? sdHelper_data : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_19 = _rdata_T_8 ? regs_20 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_20 = _rdata_T_9 ? regs_4 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_21 = _rdata_T_10 ? regs_1 : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_22 = _rdata_T_11 | _rdata_T_12; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_23 = _rdata_T_22 | _rdata_T_13; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_24 = _rdata_T_23 | _rdata_T_14; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_25 = _rdata_T_24 | _rdata_T_15; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_26 = _rdata_T_25 | _rdata_T_16; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_27 = _rdata_T_26 | _rdata_T_17; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_28 = _rdata_T_27 | _rdata_T_18; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_29 = _rdata_T_28 | _rdata_T_19; // @[Mux.scala 27:73]
  wire [31:0] _rdata_T_30 = _rdata_T_29 | _rdata_T_20; // @[Mux.scala 27:73]
  wire [31:0] rdata = _rdata_T_30 | _rdata_T_21; // @[Mux.scala 27:73]
  wire [31:0] _regs_0_T = in_wdata[31:0] & _T_59; // @[BitUtils.scala 35:14]
  wire [31:0] _regs_0_T_1 = ~_T_59; // @[BitUtils.scala 35:39]
  wire [31:0] _regs_0_T_2 = regs_0 & _regs_0_T_1; // @[BitUtils.scala 35:37]
  wire [31:0] _regs_0_T_3 = _regs_0_T | _regs_0_T_2; // @[BitUtils.scala 35:26]
  wire [5:0] regs_0_cmd = _regs_0_T_3[5:0]; // @[AXI4DummySD.scala 93:22]
  wire [31:0] _GEN_19 = 6'hd == regs_0_cmd ? 32'h0 : regs_4; // @[AXI4DummySD.scala 94:19 111:24 81:45]
  wire [31:0] _GEN_20 = 6'hd == regs_0_cmd ? 32'h0 : regs_5; // @[AXI4DummySD.scala 94:19 112:24 81:45]
  wire [31:0] _GEN_21 = 6'hd == regs_0_cmd ? 32'h0 : regs_6; // @[AXI4DummySD.scala 94:19 113:24 81:45]
  wire [31:0] _GEN_22 = 6'hd == regs_0_cmd ? 32'h0 : regs_7; // @[AXI4DummySD.scala 94:19 114:24 81:45]
  wire  _GEN_23 = 6'hd == regs_0_cmd ? 1'h0 : 6'h12 == regs_0_cmd; // @[AXI4DummySD.scala 94:19]
  wire [31:0] _GEN_24 = 6'h9 == regs_0_cmd ? 32'h92404001 : _GEN_19; // @[AXI4DummySD.scala 94:19 105:24]
  wire [31:0] _GEN_25 = 6'h9 == regs_0_cmd ? 32'hd24b97e3 : _GEN_20; // @[AXI4DummySD.scala 94:19 106:24]
  wire [31:0] _GEN_26 = 6'h9 == regs_0_cmd ? 32'hf5f803f : _GEN_21; // @[AXI4DummySD.scala 94:19 107:24]
  wire [31:0] _GEN_27 = 6'h9 == regs_0_cmd ? 32'h8c26012a : _GEN_22; // @[AXI4DummySD.scala 94:19 108:24]
  wire  _GEN_28 = 6'h9 == regs_0_cmd ? 1'h0 : _GEN_23; // @[AXI4DummySD.scala 94:19]
  wire  _GEN_33 = 6'h2 == regs_0_cmd ? 1'h0 : _GEN_28; // @[AXI4DummySD.scala 94:19]
  wire  _GEN_38 = 6'h1 == regs_0_cmd ? 1'h0 : _GEN_33; // @[AXI4DummySD.scala 94:19]
  wire [31:0] _regs_15_T_2 = regs_15 & _regs_0_T_1; // @[BitUtils.scala 35:37]
  wire [31:0] _regs_15_T_3 = _regs_0_T | _regs_15_T_2; // @[BitUtils.scala 35:26]
  wire [31:0] _regs_8_T_2 = regs_8 & _regs_0_T_1; // @[BitUtils.scala 35:37]
  wire [31:0] _regs_8_T_3 = _regs_0_T | _regs_8_T_2; // @[BitUtils.scala 35:26]
  wire [31:0] _regs_20_T_2 = regs_20 & _regs_0_T_1; // @[BitUtils.scala 35:37]
  wire [31:0] _regs_20_T_3 = _regs_0_T | _regs_20_T_2; // @[BitUtils.scala 35:26]
  wire [31:0] _regs_1_T_2 = regs_1 & _regs_0_T_1; // @[BitUtils.scala 35:37]
  wire [31:0] _regs_1_T_3 = _regs_0_T | _regs_1_T_2; // @[BitUtils.scala 35:26]
  wire [7:0] in_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_bid = bundleIn_0_bid_r; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 177:16]
  wire [1:0] in_bresp = 2'h0; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 174:18]
  wire [2:0] in_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_rid = bundleIn_0_rid_r; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 179:16]
  wire [63:0] in_rdata = {rdata,rdata}; // @[Cat.scala 31:58]
  wire [1:0] in_rresp = 2'h0; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 154:18]
  SDHelper sdHelper ( // @[AXI4DummySD.scala 123:26]
    .clk(sdHelper_clk),
    .ren(sdHelper_ren),
    .data(sdHelper_data),
    .setAddr(sdHelper_setAddr),
    .addr(sdHelper_addr)
  );
  assign auto_in_awready = in_awready; // @[LazyModule.scala 309:16]
  assign auto_in_wready = in_wready; // @[LazyModule.scala 309:16]
  assign auto_in_bvalid = in_bvalid; // @[LazyModule.scala 309:16]
  assign auto_in_bid = in_bid; // @[LazyModule.scala 309:16]
  assign auto_in_bresp = in_bresp; // @[LazyModule.scala 309:16]
  assign auto_in_arready = in_arready; // @[LazyModule.scala 309:16]
  assign auto_in_rvalid = in_rvalid; // @[LazyModule.scala 309:16]
  assign auto_in_rid = in_rid; // @[LazyModule.scala 309:16]
  assign auto_in_rdata = in_rdata; // @[LazyModule.scala 309:16]
  assign auto_in_rresp = in_bresp; // @[LazyModule.scala 309:16]
  assign auto_in_rlast = in_rlast; // @[LazyModule.scala 309:16]
  assign sdHelper_clk = clock; // @[AXI4DummySD.scala 124:18]
  assign sdHelper_ren = _GEN_10[12:0] == 13'h40 & _T; // @[AXI4DummySD.scala 125:50]
  assign sdHelper_setAddr = _T_2 & _GEN_13[12:0] == 13'h0 & _GEN_38; // @[RegMap.scala 30:48]
  assign sdHelper_addr = regs_1; // @[AXI4DummySD.scala 127:19]
  always @(posedge clock) begin
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
    if (_T_1) begin // @[Reg.scala 17:18]
      waddr_hold_data <= in_awaddr; // @[Reg.scala 17:22]
    end
    if (_T_1) begin // @[Reg.scala 17:18]
      bundleIn_0_bid_r <= in_awid; // @[Reg.scala 17:22]
    end
    if (_T) begin // @[Reg.scala 17:18]
      bundleIn_0_rid_r <= in_arid; // @[Reg.scala 17:22]
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_0 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h0) begin // @[RegMap.scala 30:48]
      regs_0 <= _regs_0_T_3; // @[RegMap.scala 30:52]
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_1 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h4) begin // @[RegMap.scala 30:48]
      regs_1 <= _regs_1_T_3; // @[RegMap.scala 30:52]
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_4 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h0) begin // @[RegMap.scala 30:48]
      if (6'h1 == regs_0_cmd) begin // @[AXI4DummySD.scala 94:19]
        regs_4 <= 32'h80ff8000; // @[AXI4DummySD.scala 96:24]
      end else if (6'h2 == regs_0_cmd) begin // @[AXI4DummySD.scala 94:19]
        regs_4 <= 32'h1; // @[AXI4DummySD.scala 99:24]
      end else begin
        regs_4 <= _GEN_24;
      end
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_5 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h0) begin // @[RegMap.scala 30:48]
      if (!(6'h1 == regs_0_cmd)) begin // @[AXI4DummySD.scala 94:19]
        if (6'h2 == regs_0_cmd) begin // @[AXI4DummySD.scala 94:19]
          regs_5 <= 32'h0; // @[AXI4DummySD.scala 100:24]
        end else begin
          regs_5 <= _GEN_25;
        end
      end
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_6 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h0) begin // @[RegMap.scala 30:48]
      if (!(6'h1 == regs_0_cmd)) begin // @[AXI4DummySD.scala 94:19]
        if (6'h2 == regs_0_cmd) begin // @[AXI4DummySD.scala 94:19]
          regs_6 <= 32'h0; // @[AXI4DummySD.scala 101:24]
        end else begin
          regs_6 <= _GEN_26;
        end
      end
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_7 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h0) begin // @[RegMap.scala 30:48]
      if (!(6'h1 == regs_0_cmd)) begin // @[AXI4DummySD.scala 94:19]
        if (6'h2 == regs_0_cmd) begin // @[AXI4DummySD.scala 94:19]
          regs_7 <= 32'h15000000; // @[AXI4DummySD.scala 102:24]
        end else begin
          regs_7 <= _GEN_27;
        end
      end
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_8 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h20) begin // @[RegMap.scala 30:48]
      regs_8 <= _regs_8_T_3; // @[RegMap.scala 30:52]
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_15 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h38) begin // @[RegMap.scala 30:48]
      regs_15 <= _regs_15_T_3; // @[RegMap.scala 30:52]
    end
    if (reset) begin // @[AXI4DummySD.scala 81:45]
      regs_20 <= 32'h0; // @[AXI4DummySD.scala 81:45]
    end else if (_T_2 & _GEN_13[12:0] == 13'h50) begin // @[RegMap.scala 30:48]
      regs_20 <= _regs_20_T_3; // @[RegMap.scala 30:52]
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
  state = _RAND_0[1:0];
  _RAND_1 = {1{`RANDOM}};
  value = _RAND_1[7:0];
  _RAND_2 = {1{`RANDOM}};
  hold_data = _RAND_2[7:0];
  _RAND_3 = {2{`RANDOM}};
  raddr_hold_data = _RAND_3[36:0];
  _RAND_4 = {2{`RANDOM}};
  waddr_hold_data = _RAND_4[36:0];
  _RAND_5 = {1{`RANDOM}};
  bundleIn_0_bid_r = _RAND_5[0:0];
  _RAND_6 = {1{`RANDOM}};
  bundleIn_0_rid_r = _RAND_6[0:0];
  _RAND_7 = {1{`RANDOM}};
  regs_0 = _RAND_7[31:0];
  _RAND_8 = {1{`RANDOM}};
  regs_1 = _RAND_8[31:0];
  _RAND_9 = {1{`RANDOM}};
  regs_4 = _RAND_9[31:0];
  _RAND_10 = {1{`RANDOM}};
  regs_5 = _RAND_10[31:0];
  _RAND_11 = {1{`RANDOM}};
  regs_6 = _RAND_11[31:0];
  _RAND_12 = {1{`RANDOM}};
  regs_7 = _RAND_12[31:0];
  _RAND_13 = {1{`RANDOM}};
  regs_8 = _RAND_13[31:0];
  _RAND_14 = {1{`RANDOM}};
  regs_15 = _RAND_14[31:0];
  _RAND_15 = {1{`RANDOM}};
  regs_20 = _RAND_15[31:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

