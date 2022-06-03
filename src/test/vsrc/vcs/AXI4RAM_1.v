module AXI4RAM_1(
  input          clock,
  input          reset,
  output         auto_in_awready,
  input          auto_in_awvalid,
  input  [5:0]   auto_in_awid,
  input  [37:0]  auto_in_awaddr,
  input  [7:0]   auto_in_awlen,
  input  [2:0]   auto_in_awsize,
  input  [1:0]   auto_in_awburst,
  input          auto_in_awlock,
  input  [3:0]   auto_in_awcache,
  input  [2:0]   auto_in_awprot,
  input  [3:0]   auto_in_awqos,
  output         auto_in_wready,
  input          auto_in_wvalid,
  input  [255:0] auto_in_wdata,
  input  [31:0]  auto_in_wstrb,
  input          auto_in_wlast,
  input          auto_in_bready,
  output         auto_in_bvalid,
  output [5:0]   auto_in_bid,
  output [1:0]   auto_in_bresp,
  output         auto_in_arready,
  input          auto_in_arvalid,
  input  [5:0]   auto_in_arid,
  input  [37:0]  auto_in_araddr,
  input  [7:0]   auto_in_arlen,
  input  [2:0]   auto_in_arsize,
  input  [1:0]   auto_in_arburst,
  input          auto_in_arlock,
  input  [3:0]   auto_in_arcache,
  input  [2:0]   auto_in_arprot,
  input  [3:0]   auto_in_arqos,
  input          auto_in_rready,
  output         auto_in_rvalid,
  output [5:0]   auto_in_rid,
  output [255:0] auto_in_rdata,
  output [1:0]   auto_in_rresp,
  output         auto_in_rlast
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
  reg [31:0] _RAND_2;
  reg [63:0] _RAND_3;
  reg [31:0] _RAND_4;
  reg [63:0] _RAND_5;
  reg [31:0] _RAND_6;
  reg [31:0] _RAND_7;
`endif // RANDOMIZE_REG_INIT
  wire  rdata_mems_0_clk; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_0_en; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_0_rIdx; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_0_rdata; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_0_wIdx; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_0_wdata; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_0_wmask; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_0_wen; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_1_clk; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_1_en; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_1_rIdx; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_1_rdata; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_1_wIdx; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_1_wdata; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_1_wmask; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_1_wen; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_2_clk; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_2_en; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_2_rIdx; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_2_rdata; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_2_wIdx; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_2_wdata; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_2_wmask; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_2_wen; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_3_clk; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_3_en; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_3_rIdx; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_3_rdata; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_3_wIdx; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_3_wdata; // @[AXI4RAM.scala 70:50]
  wire [63:0] rdata_mems_3_wmask; // @[AXI4RAM.scala 70:50]
  wire  rdata_mems_3_wen; // @[AXI4RAM.scala 70:50]
  reg [1:0] state; // @[AXI4SlaveModule.scala 95:22]
  wire  _bundleIn_0_arready_T = state == 2'h0; // @[AXI4SlaveModule.scala 153:24]
  wire  in_arready = state == 2'h0; // @[AXI4SlaveModule.scala 153:24]
  wire  in_arvalid = auto_in_arvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  _T = in_arready & in_arvalid; // @[Decoupled.scala 50:35]
  wire  in_awready = _bundleIn_0_arready_T & ~in_arvalid; // @[AXI4SlaveModule.scala 171:35]
  wire  in_awvalid = auto_in_awvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  _T_1 = in_awready & in_awvalid; // @[Decoupled.scala 50:35]
  wire  _bundleIn_0_wready_T = state == 2'h2; // @[AXI4SlaveModule.scala 172:23]
  wire  in_wready = state == 2'h2; // @[AXI4SlaveModule.scala 172:23]
  wire  in_wvalid = auto_in_wvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  _T_2 = in_wready & in_wvalid; // @[Decoupled.scala 50:35]
  wire  in_bready = auto_in_bready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_bvalid = state == 2'h3; // @[AXI4SlaveModule.scala 175:22]
  wire  _T_3 = in_bready & in_bvalid; // @[Decoupled.scala 50:35]
  wire  in_rready = auto_in_rready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  _bundleIn_0_rvalid_T = state == 2'h1; // @[AXI4SlaveModule.scala 155:23]
  wire  in_rvalid = state == 2'h1; // @[AXI4SlaveModule.scala 155:23]
  wire  _T_4 = in_rready & in_rvalid; // @[Decoupled.scala 50:35]
  wire [1:0] in_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  _T_8 = ~reset; // @[AXI4SlaveModule.scala 87:11]
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
  wire [31:0] in_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg [37:0] raddr_hold_data; // @[Reg.scala 16:16]
  wire [37:0] in_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [37:0] _GEN_10 = _T ? in_araddr : raddr_hold_data; // @[Reg.scala 16:16 17:{18,22}]
  wire [7:0] _value_T_1 = value + 8'h1; // @[Counter.scala 78:24]
  reg [7:0] value_1; // @[Counter.scala 62:40]
  reg [37:0] waddr_hold_data; // @[Reg.scala 16:16]
  wire [37:0] in_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [37:0] _GEN_13 = _T_1 ? in_awaddr : waddr_hold_data; // @[Reg.scala 16:16 17:{18,22}]
  wire [7:0] _value_T_3 = value_1 + 8'h1; // @[Counter.scala 78:24]
  reg [5:0] bundleIn_0_bid_r; // @[Reg.scala 16:16]
  wire [5:0] in_awid = auto_in_awid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  reg [5:0] bundleIn_0_rid_r; // @[Reg.scala 16:16]
  wire [5:0] in_arid = auto_in_arid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [37:0] _wIdx_T_1 = _GEN_13 - 38'h2000000000; // @[AXI4RAM.scala 60:36]
  wire [27:0] _GEN_18 = {{20'd0}, value_1}; // @[AXI4RAM.scala 64:29]
  wire [27:0] wIdx = _wIdx_T_1[32:5] + _GEN_18; // @[AXI4RAM.scala 64:29]
  wire [37:0] _rIdx_T_1 = _GEN_10 - 38'h2000000000; // @[AXI4RAM.scala 60:36]
  wire [27:0] _GEN_19 = {{20'd0}, value}; // @[AXI4RAM.scala 65:29]
  wire [27:0] rIdx = _rIdx_T_1[32:5] + _GEN_19; // @[AXI4RAM.scala 65:29]
  wire [29:0] _rdata_mems_0_rIdx_T = {rIdx, 2'h0}; // @[AXI4RAM.scala 74:28]
  wire [30:0] _rdata_mems_0_rIdx_T_1 = {{1'd0}, _rdata_mems_0_rIdx_T}; // @[AXI4RAM.scala 74:46]
  wire [29:0] _rdata_mems_0_wIdx_T = {wIdx, 2'h0}; // @[AXI4RAM.scala 75:28]
  wire [30:0] _rdata_mems_0_wIdx_T_1 = {{1'd0}, _rdata_mems_0_wIdx_T}; // @[AXI4RAM.scala 75:46]
  wire [255:0] in_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [7:0] _rdata_mems_0_wmask_T_10 = in_wstrb[0] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_0_wmask_T_12 = in_wstrb[1] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_0_wmask_T_14 = in_wstrb[2] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_0_wmask_T_16 = in_wstrb[3] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_0_wmask_T_18 = in_wstrb[4] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_0_wmask_T_20 = in_wstrb[5] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_0_wmask_T_22 = in_wstrb[6] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_0_wmask_T_24 = in_wstrb[7] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [31:0] rdata_mems_0_wmask_lo = {_rdata_mems_0_wmask_T_16,_rdata_mems_0_wmask_T_14,_rdata_mems_0_wmask_T_12,
    _rdata_mems_0_wmask_T_10}; // @[Cat.scala 31:58]
  wire [31:0] rdata_mems_0_wmask_hi = {_rdata_mems_0_wmask_T_24,_rdata_mems_0_wmask_T_22,_rdata_mems_0_wmask_T_20,
    _rdata_mems_0_wmask_T_18}; // @[Cat.scala 31:58]
  wire [29:0] _rdata_mems_1_rIdx_T_2 = _rdata_mems_0_rIdx_T + 30'h1; // @[AXI4RAM.scala 74:46]
  wire [29:0] _rdata_mems_1_wIdx_T_2 = _rdata_mems_0_wIdx_T + 30'h1; // @[AXI4RAM.scala 75:46]
  wire [7:0] _rdata_mems_1_wmask_T_10 = in_wstrb[8] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_1_wmask_T_12 = in_wstrb[9] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_1_wmask_T_14 = in_wstrb[10] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_1_wmask_T_16 = in_wstrb[11] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_1_wmask_T_18 = in_wstrb[12] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_1_wmask_T_20 = in_wstrb[13] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_1_wmask_T_22 = in_wstrb[14] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_1_wmask_T_24 = in_wstrb[15] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [31:0] rdata_mems_1_wmask_lo = {_rdata_mems_1_wmask_T_16,_rdata_mems_1_wmask_T_14,_rdata_mems_1_wmask_T_12,
    _rdata_mems_1_wmask_T_10}; // @[Cat.scala 31:58]
  wire [31:0] rdata_mems_1_wmask_hi = {_rdata_mems_1_wmask_T_24,_rdata_mems_1_wmask_T_22,_rdata_mems_1_wmask_T_20,
    _rdata_mems_1_wmask_T_18}; // @[Cat.scala 31:58]
  wire [29:0] _rdata_mems_2_rIdx_T_2 = _rdata_mems_0_rIdx_T + 30'h2; // @[AXI4RAM.scala 74:46]
  wire [29:0] _rdata_mems_2_wIdx_T_2 = _rdata_mems_0_wIdx_T + 30'h2; // @[AXI4RAM.scala 75:46]
  wire [7:0] _rdata_mems_2_wmask_T_10 = in_wstrb[16] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_2_wmask_T_12 = in_wstrb[17] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_2_wmask_T_14 = in_wstrb[18] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_2_wmask_T_16 = in_wstrb[19] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_2_wmask_T_18 = in_wstrb[20] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_2_wmask_T_20 = in_wstrb[21] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_2_wmask_T_22 = in_wstrb[22] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_2_wmask_T_24 = in_wstrb[23] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [31:0] rdata_mems_2_wmask_lo = {_rdata_mems_2_wmask_T_16,_rdata_mems_2_wmask_T_14,_rdata_mems_2_wmask_T_12,
    _rdata_mems_2_wmask_T_10}; // @[Cat.scala 31:58]
  wire [31:0] rdata_mems_2_wmask_hi = {_rdata_mems_2_wmask_T_24,_rdata_mems_2_wmask_T_22,_rdata_mems_2_wmask_T_20,
    _rdata_mems_2_wmask_T_18}; // @[Cat.scala 31:58]
  wire [29:0] _rdata_mems_3_rIdx_T_2 = _rdata_mems_0_rIdx_T + 30'h3; // @[AXI4RAM.scala 74:46]
  wire [29:0] _rdata_mems_3_wIdx_T_2 = _rdata_mems_0_wIdx_T + 30'h3; // @[AXI4RAM.scala 75:46]
  wire [7:0] _rdata_mems_3_wmask_T_10 = in_wstrb[24] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_3_wmask_T_12 = in_wstrb[25] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_3_wmask_T_14 = in_wstrb[26] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_3_wmask_T_16 = in_wstrb[27] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_3_wmask_T_18 = in_wstrb[28] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_3_wmask_T_20 = in_wstrb[29] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_3_wmask_T_22 = in_wstrb[30] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _rdata_mems_3_wmask_T_24 = in_wstrb[31] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [31:0] rdata_mems_3_wmask_lo = {_rdata_mems_3_wmask_T_16,_rdata_mems_3_wmask_T_14,_rdata_mems_3_wmask_T_12,
    _rdata_mems_3_wmask_T_10}; // @[Cat.scala 31:58]
  wire [31:0] rdata_mems_3_wmask_hi = {_rdata_mems_3_wmask_T_24,_rdata_mems_3_wmask_T_22,_rdata_mems_3_wmask_T_20,
    _rdata_mems_3_wmask_T_18}; // @[Cat.scala 31:58]
  wire [255:0] rdata = {rdata_mems_3_rdata,rdata_mems_2_rdata,rdata_mems_1_rdata,rdata_mems_0_rdata}; // @[Cat.scala 31:58]
  wire [7:0] in_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [5:0] in_bid = bundleIn_0_bid_r; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 177:16]
  wire [1:0] in_bresp = 2'h0; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 174:18]
  wire [2:0] in_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire  in_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [2:0] in_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [3:0] in_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [5:0] in_rid = bundleIn_0_rid_r; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 179:16]
  wire [255:0] in_rdata = rdata; // @[Cat.scala 31:58]
  wire [1:0] in_rresp = 2'h0; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 154:18]
  RAMHelper rdata_mems_0 ( // @[AXI4RAM.scala 70:50]
    .clk(rdata_mems_0_clk),
    .en(rdata_mems_0_en),
    .rIdx(rdata_mems_0_rIdx),
    .rdata(rdata_mems_0_rdata),
    .wIdx(rdata_mems_0_wIdx),
    .wdata(rdata_mems_0_wdata),
    .wmask(rdata_mems_0_wmask),
    .wen(rdata_mems_0_wen)
  );
  RAMHelper rdata_mems_1 ( // @[AXI4RAM.scala 70:50]
    .clk(rdata_mems_1_clk),
    .en(rdata_mems_1_en),
    .rIdx(rdata_mems_1_rIdx),
    .rdata(rdata_mems_1_rdata),
    .wIdx(rdata_mems_1_wIdx),
    .wdata(rdata_mems_1_wdata),
    .wmask(rdata_mems_1_wmask),
    .wen(rdata_mems_1_wen)
  );
  RAMHelper rdata_mems_2 ( // @[AXI4RAM.scala 70:50]
    .clk(rdata_mems_2_clk),
    .en(rdata_mems_2_en),
    .rIdx(rdata_mems_2_rIdx),
    .rdata(rdata_mems_2_rdata),
    .wIdx(rdata_mems_2_wIdx),
    .wdata(rdata_mems_2_wdata),
    .wmask(rdata_mems_2_wmask),
    .wen(rdata_mems_2_wen)
  );
  RAMHelper rdata_mems_3 ( // @[AXI4RAM.scala 70:50]
    .clk(rdata_mems_3_clk),
    .en(rdata_mems_3_en),
    .rIdx(rdata_mems_3_rIdx),
    .rdata(rdata_mems_3_rdata),
    .wIdx(rdata_mems_3_wIdx),
    .wdata(rdata_mems_3_wdata),
    .wmask(rdata_mems_3_wmask),
    .wen(rdata_mems_3_wen)
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
  assign rdata_mems_0_clk = clock; // @[AXI4RAM.scala 72:19]
  assign rdata_mems_0_en = _T_8 & (_bundleIn_0_rvalid_T | _bundleIn_0_wready_T); // @[AXI4RAM.scala 73:38]
  assign rdata_mems_0_rIdx = {{34'd0}, _rdata_mems_0_rIdx_T_1[29:0]}; // @[AXI4RAM.scala 74:19]
  assign rdata_mems_0_wIdx = {{34'd0}, _rdata_mems_0_wIdx_T_1[29:0]}; // @[AXI4RAM.scala 75:19]
  assign rdata_mems_0_wdata = in_wdata[63:0]; // @[AXI4RAM.scala 76:36]
  assign rdata_mems_0_wmask = {rdata_mems_0_wmask_hi,rdata_mems_0_wmask_lo}; // @[Cat.scala 31:58]
  assign rdata_mems_0_wen = in_wready & in_wvalid; // @[Decoupled.scala 50:35]
  assign rdata_mems_1_clk = clock; // @[AXI4RAM.scala 72:19]
  assign rdata_mems_1_en = _T_8 & (_bundleIn_0_rvalid_T | _bundleIn_0_wready_T); // @[AXI4RAM.scala 73:38]
  assign rdata_mems_1_rIdx = {{34'd0}, _rdata_mems_1_rIdx_T_2}; // @[AXI4RAM.scala 74:19]
  assign rdata_mems_1_wIdx = {{34'd0}, _rdata_mems_1_wIdx_T_2}; // @[AXI4RAM.scala 75:19]
  assign rdata_mems_1_wdata = in_wdata[127:64]; // @[AXI4RAM.scala 76:36]
  assign rdata_mems_1_wmask = {rdata_mems_1_wmask_hi,rdata_mems_1_wmask_lo}; // @[Cat.scala 31:58]
  assign rdata_mems_1_wen = in_wready & in_wvalid; // @[Decoupled.scala 50:35]
  assign rdata_mems_2_clk = clock; // @[AXI4RAM.scala 72:19]
  assign rdata_mems_2_en = _T_8 & (_bundleIn_0_rvalid_T | _bundleIn_0_wready_T); // @[AXI4RAM.scala 73:38]
  assign rdata_mems_2_rIdx = {{34'd0}, _rdata_mems_2_rIdx_T_2}; // @[AXI4RAM.scala 74:19]
  assign rdata_mems_2_wIdx = {{34'd0}, _rdata_mems_2_wIdx_T_2}; // @[AXI4RAM.scala 75:19]
  assign rdata_mems_2_wdata = in_wdata[191:128]; // @[AXI4RAM.scala 76:36]
  assign rdata_mems_2_wmask = {rdata_mems_2_wmask_hi,rdata_mems_2_wmask_lo}; // @[Cat.scala 31:58]
  assign rdata_mems_2_wen = in_wready & in_wvalid; // @[Decoupled.scala 50:35]
  assign rdata_mems_3_clk = clock; // @[AXI4RAM.scala 72:19]
  assign rdata_mems_3_en = _T_8 & (_bundleIn_0_rvalid_T | _bundleIn_0_wready_T); // @[AXI4RAM.scala 73:38]
  assign rdata_mems_3_rIdx = {{34'd0}, _rdata_mems_3_rIdx_T_2}; // @[AXI4RAM.scala 74:19]
  assign rdata_mems_3_wIdx = {{34'd0}, _rdata_mems_3_wIdx_T_2}; // @[AXI4RAM.scala 75:19]
  assign rdata_mems_3_wdata = in_wdata[255:192]; // @[AXI4RAM.scala 76:36]
  assign rdata_mems_3_wmask = {rdata_mems_3_wmask_hi,rdata_mems_3_wmask_lo}; // @[Cat.scala 31:58]
  assign rdata_mems_3_wen = in_wready & in_wvalid; // @[Decoupled.scala 50:35]
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
`ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  state = _RAND_0[1:0];
  _RAND_1 = {1{`RANDOM}};
  value = _RAND_1[7:0];
  _RAND_2 = {1{`RANDOM}};
  hold_data = _RAND_2[7:0];
  _RAND_3 = {2{`RANDOM}};
  raddr_hold_data = _RAND_3[37:0];
  _RAND_4 = {1{`RANDOM}};
  value_1 = _RAND_4[7:0];
  _RAND_5 = {2{`RANDOM}};
  waddr_hold_data = _RAND_5[37:0];
  _RAND_6 = {1{`RANDOM}};
  bundleIn_0_bid_r = _RAND_6[5:0];
  _RAND_7 = {1{`RANDOM}};
  bundleIn_0_rid_r = _RAND_7[5:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

