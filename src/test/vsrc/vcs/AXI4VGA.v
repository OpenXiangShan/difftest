module AXI4VGA(
  input         clock,
  input         reset,
  output        auto_in_1_awready,
  input         auto_in_1_awvalid,
  input         auto_in_1_awid,
  input  [36:0] auto_in_1_awaddr,
  input  [7:0]  auto_in_1_awlen,
  input  [2:0]  auto_in_1_awsize,
  input  [1:0]  auto_in_1_awburst,
  input         auto_in_1_awlock,
  input  [3:0]  auto_in_1_awcache,
  input  [2:0]  auto_in_1_awprot,
  input  [3:0]  auto_in_1_awqos,
  output        auto_in_1_wready,
  input         auto_in_1_wvalid,
  input  [63:0] auto_in_1_wdata,
  input  [7:0]  auto_in_1_wstrb,
  input         auto_in_1_wlast,
  input         auto_in_1_bready,
  output        auto_in_1_bvalid,
  output        auto_in_1_bid,
  output [1:0]  auto_in_1_bresp,
  output        auto_in_1_arready,
  input         auto_in_1_arvalid,
  input         auto_in_1_arid,
  input  [36:0] auto_in_1_araddr,
  input  [7:0]  auto_in_1_arlen,
  input  [2:0]  auto_in_1_arsize,
  input  [1:0]  auto_in_1_arburst,
  input         auto_in_1_arlock,
  input  [3:0]  auto_in_1_arcache,
  input  [2:0]  auto_in_1_arprot,
  input  [3:0]  auto_in_1_arqos,
  input         auto_in_1_rready,
  output        auto_in_1_rvalid,
  output        auto_in_1_rid,
  output [63:0] auto_in_1_rdata,
  output [1:0]  auto_in_1_rresp,
  output        auto_in_1_rlast,
  output        auto_in_0_awready,
  input         auto_in_0_awvalid,
  input         auto_in_0_awid,
  input  [36:0] auto_in_0_awaddr,
  input  [7:0]  auto_in_0_awlen,
  input  [2:0]  auto_in_0_awsize,
  input  [1:0]  auto_in_0_awburst,
  input         auto_in_0_awlock,
  input  [3:0]  auto_in_0_awcache,
  input  [2:0]  auto_in_0_awprot,
  input  [3:0]  auto_in_0_awqos,
  output        auto_in_0_wready,
  input         auto_in_0_wvalid,
  input  [63:0] auto_in_0_wdata,
  input  [7:0]  auto_in_0_wstrb,
  input         auto_in_0_wlast,
  input         auto_in_0_bready,
  output        auto_in_0_bvalid,
  output        auto_in_0_bid,
  output [1:0]  auto_in_0_bresp,
  input         auto_in_0_arvalid,
  input         auto_in_0_arid,
  input         auto_in_0_arlock,
  input  [3:0]  auto_in_0_arcache,
  input  [3:0]  auto_in_0_arqos,
  input         auto_in_0_rready,
  output        auto_in_0_rvalid,
  output        auto_in_0_rid,
  output        auto_in_0_rlast
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
  reg [31:0] _RAND_2;
  reg [31:0] _RAND_3;
  reg [31:0] _RAND_4;
  reg [31:0] _RAND_5;
`endif // RANDOMIZE_REG_INIT
  wire  fb_clock; // @[AXI4VGA.scala 130:30]
  wire  fb_reset; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_awready; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_awvalid; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_awid; // @[AXI4VGA.scala 130:30]
  wire [36:0] fb_auto_in_awaddr; // @[AXI4VGA.scala 130:30]
  wire [7:0] fb_auto_in_awlen; // @[AXI4VGA.scala 130:30]
  wire [2:0] fb_auto_in_awsize; // @[AXI4VGA.scala 130:30]
  wire [1:0] fb_auto_in_awburst; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_awlock; // @[AXI4VGA.scala 130:30]
  wire [3:0] fb_auto_in_awcache; // @[AXI4VGA.scala 130:30]
  wire [2:0] fb_auto_in_awprot; // @[AXI4VGA.scala 130:30]
  wire [3:0] fb_auto_in_awqos; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_wready; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_wvalid; // @[AXI4VGA.scala 130:30]
  wire [63:0] fb_auto_in_wdata; // @[AXI4VGA.scala 130:30]
  wire [7:0] fb_auto_in_wstrb; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_wlast; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_bready; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_bvalid; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_bid; // @[AXI4VGA.scala 130:30]
  wire [1:0] fb_auto_in_bresp; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_arvalid; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_arid; // @[AXI4VGA.scala 130:30]
  wire [36:0] fb_auto_in_araddr; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_arlock; // @[AXI4VGA.scala 130:30]
  wire [3:0] fb_auto_in_arcache; // @[AXI4VGA.scala 130:30]
  wire [3:0] fb_auto_in_arqos; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_rid; // @[AXI4VGA.scala 130:30]
  wire  fb_auto_in_rlast; // @[AXI4VGA.scala 130:30]
  wire  ctrl_clock; // @[AXI4VGA.scala 136:32]
  wire  ctrl_reset; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_awready; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_awvalid; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_awid; // @[AXI4VGA.scala 136:32]
  wire [36:0] ctrl_auto_in_awaddr; // @[AXI4VGA.scala 136:32]
  wire [7:0] ctrl_auto_in_awlen; // @[AXI4VGA.scala 136:32]
  wire [2:0] ctrl_auto_in_awsize; // @[AXI4VGA.scala 136:32]
  wire [1:0] ctrl_auto_in_awburst; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_awlock; // @[AXI4VGA.scala 136:32]
  wire [3:0] ctrl_auto_in_awcache; // @[AXI4VGA.scala 136:32]
  wire [2:0] ctrl_auto_in_awprot; // @[AXI4VGA.scala 136:32]
  wire [3:0] ctrl_auto_in_awqos; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_wready; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_wvalid; // @[AXI4VGA.scala 136:32]
  wire [63:0] ctrl_auto_in_wdata; // @[AXI4VGA.scala 136:32]
  wire [7:0] ctrl_auto_in_wstrb; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_wlast; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_bready; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_bvalid; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_bid; // @[AXI4VGA.scala 136:32]
  wire [1:0] ctrl_auto_in_bresp; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_arready; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_arvalid; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_arid; // @[AXI4VGA.scala 136:32]
  wire [36:0] ctrl_auto_in_araddr; // @[AXI4VGA.scala 136:32]
  wire [7:0] ctrl_auto_in_arlen; // @[AXI4VGA.scala 136:32]
  wire [2:0] ctrl_auto_in_arsize; // @[AXI4VGA.scala 136:32]
  wire [1:0] ctrl_auto_in_arburst; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_arlock; // @[AXI4VGA.scala 136:32]
  wire [3:0] ctrl_auto_in_arcache; // @[AXI4VGA.scala 136:32]
  wire [2:0] ctrl_auto_in_arprot; // @[AXI4VGA.scala 136:32]
  wire [3:0] ctrl_auto_in_arqos; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_rready; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_rvalid; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_rid; // @[AXI4VGA.scala 136:32]
  wire [63:0] ctrl_auto_in_rdata; // @[AXI4VGA.scala 136:32]
  wire [1:0] ctrl_auto_in_rresp; // @[AXI4VGA.scala 136:32]
  wire  ctrl_auto_in_rlast; // @[AXI4VGA.scala 136:32]
  reg  bundleIn_0_rvalid_r; // @[StopWatch.scala 23:20]
  wire  _bundleIn_0_rvalid_T_1 = auto_in_0_rready & bundleIn_0_rvalid_r; // @[Decoupled.scala 50:35]
  wire  _GEN_0 = _bundleIn_0_rvalid_T_1 ? 1'h0 : bundleIn_0_rvalid_r; // @[StopWatch.scala 25:19 23:20 25:23]
  wire  _GEN_1 = auto_in_0_arvalid | _GEN_0; // @[StopWatch.scala 26:{20,24}]
  reg [10:0] hCounter; // @[Counter.scala 62:40]
  wire  wrap_wrap = hCounter == 11'h41f; // @[Counter.scala 74:24]
  wire [10:0] _wrap_value_T_1 = hCounter + 11'h1; // @[Counter.scala 78:24]
  reg [9:0] vCounter; // @[Counter.scala 62:40]
  wire  wrap_wrap_1 = vCounter == 10'h273; // @[Counter.scala 74:24]
  wire [9:0] _wrap_value_T_3 = vCounter + 10'h1; // @[Counter.scala 78:24]
  wire  vInRange = vCounter >= 10'h5 & vCounter < 10'h25d; // @[AXI4VGA.scala 159:65]
  wire  hCounterIsOdd = hCounter[0]; // @[AXI4VGA.scala 170:33]
  wire  hCounterIs2 = hCounter[1:0] == 2'h2; // @[AXI4VGA.scala 171:38]
  wire  vCounterIsOdd = vCounter[0]; // @[AXI4VGA.scala 172:33]
  wire  _nextPixel_T_2 = hCounter >= 11'ha7 & hCounter < 11'h3c7; // @[AXI4VGA.scala 159:65]
  wire  nextPixel = _nextPixel_T_2 & vInRange & hCounterIsOdd; // @[AXI4VGA.scala 175:80]
  wire  _fbPixelAddrV0_T_1 = nextPixel & ~vCounterIsOdd; // @[AXI4VGA.scala 176:43]
  reg [16:0] fbPixelAddrV0; // @[Counter.scala 62:40]
  wire  fbPixelAddrV0_wrap_wrap = fbPixelAddrV0 == 17'h1d4bf; // @[Counter.scala 74:24]
  wire [16:0] _fbPixelAddrV0_wrap_value_T_1 = fbPixelAddrV0 + 17'h1; // @[Counter.scala 78:24]
  wire  _fbPixelAddrV1_T = nextPixel & vCounterIsOdd; // @[AXI4VGA.scala 177:43]
  reg [16:0] fbPixelAddrV1; // @[Counter.scala 62:40]
  wire  fbPixelAddrV1_wrap_wrap = fbPixelAddrV1 == 17'h1d4bf; // @[Counter.scala 74:24]
  wire [16:0] _fbPixelAddrV1_wrap_value_T_1 = fbPixelAddrV1 + 17'h1; // @[Counter.scala 78:24]
  wire [16:0] _bundleOut_0_araddr_T = vCounterIsOdd ? fbPixelAddrV1 : fbPixelAddrV0; // @[AXI4VGA.scala 184:35]
  wire [18:0] _bundleOut_0_araddr_T_1 = {_bundleOut_0_araddr_T,2'h0}; // @[Cat.scala 31:58]
  reg  bundleOut_0_arvalid_REG; // @[AXI4VGA.scala 185:31]
  AXI4RAM fb ( // @[AXI4VGA.scala 130:30]
    .clock(fb_clock),
    .reset(fb_reset),
    .auto_in_awready(fb_auto_in_awready),
    .auto_in_awvalid(fb_auto_in_awvalid),
    .auto_in_awid(fb_auto_in_awid),
    .auto_in_awaddr(fb_auto_in_awaddr),
    .auto_in_awlen(fb_auto_in_awlen),
    .auto_in_awsize(fb_auto_in_awsize),
    .auto_in_awburst(fb_auto_in_awburst),
    .auto_in_awlock(fb_auto_in_awlock),
    .auto_in_awcache(fb_auto_in_awcache),
    .auto_in_awprot(fb_auto_in_awprot),
    .auto_in_awqos(fb_auto_in_awqos),
    .auto_in_wready(fb_auto_in_wready),
    .auto_in_wvalid(fb_auto_in_wvalid),
    .auto_in_wdata(fb_auto_in_wdata),
    .auto_in_wstrb(fb_auto_in_wstrb),
    .auto_in_wlast(fb_auto_in_wlast),
    .auto_in_bready(fb_auto_in_bready),
    .auto_in_bvalid(fb_auto_in_bvalid),
    .auto_in_bid(fb_auto_in_bid),
    .auto_in_bresp(fb_auto_in_bresp),
    .auto_in_arvalid(fb_auto_in_arvalid),
    .auto_in_arid(fb_auto_in_arid),
    .auto_in_araddr(fb_auto_in_araddr),
    .auto_in_arlock(fb_auto_in_arlock),
    .auto_in_arcache(fb_auto_in_arcache),
    .auto_in_arqos(fb_auto_in_arqos),
    .auto_in_rid(fb_auto_in_rid),
    .auto_in_rlast(fb_auto_in_rlast)
  );
  VGACtrl ctrl ( // @[AXI4VGA.scala 136:32]
    .clock(ctrl_clock),
    .reset(ctrl_reset),
    .auto_in_awready(ctrl_auto_in_awready),
    .auto_in_awvalid(ctrl_auto_in_awvalid),
    .auto_in_awid(ctrl_auto_in_awid),
    .auto_in_awaddr(ctrl_auto_in_awaddr),
    .auto_in_awlen(ctrl_auto_in_awlen),
    .auto_in_awsize(ctrl_auto_in_awsize),
    .auto_in_awburst(ctrl_auto_in_awburst),
    .auto_in_awlock(ctrl_auto_in_awlock),
    .auto_in_awcache(ctrl_auto_in_awcache),
    .auto_in_awprot(ctrl_auto_in_awprot),
    .auto_in_awqos(ctrl_auto_in_awqos),
    .auto_in_wready(ctrl_auto_in_wready),
    .auto_in_wvalid(ctrl_auto_in_wvalid),
    .auto_in_wdata(ctrl_auto_in_wdata),
    .auto_in_wstrb(ctrl_auto_in_wstrb),
    .auto_in_wlast(ctrl_auto_in_wlast),
    .auto_in_bready(ctrl_auto_in_bready),
    .auto_in_bvalid(ctrl_auto_in_bvalid),
    .auto_in_bid(ctrl_auto_in_bid),
    .auto_in_bresp(ctrl_auto_in_bresp),
    .auto_in_arready(ctrl_auto_in_arready),
    .auto_in_arvalid(ctrl_auto_in_arvalid),
    .auto_in_arid(ctrl_auto_in_arid),
    .auto_in_araddr(ctrl_auto_in_araddr),
    .auto_in_arlen(ctrl_auto_in_arlen),
    .auto_in_arsize(ctrl_auto_in_arsize),
    .auto_in_arburst(ctrl_auto_in_arburst),
    .auto_in_arlock(ctrl_auto_in_arlock),
    .auto_in_arcache(ctrl_auto_in_arcache),
    .auto_in_arprot(ctrl_auto_in_arprot),
    .auto_in_arqos(ctrl_auto_in_arqos),
    .auto_in_rready(ctrl_auto_in_rready),
    .auto_in_rvalid(ctrl_auto_in_rvalid),
    .auto_in_rid(ctrl_auto_in_rid),
    .auto_in_rdata(ctrl_auto_in_rdata),
    .auto_in_rresp(ctrl_auto_in_rresp),
    .auto_in_rlast(ctrl_auto_in_rlast)
  );
  assign auto_in_1_awready = ctrl_auto_in_awready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_wready = ctrl_auto_in_wready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_bvalid = ctrl_auto_in_bvalid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_bid = ctrl_auto_in_bid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_bresp = ctrl_auto_in_bresp; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_arready = ctrl_auto_in_arready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_rvalid = ctrl_auto_in_rvalid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_rid = ctrl_auto_in_rid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_rdata = ctrl_auto_in_rdata; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_rresp = ctrl_auto_in_rresp; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_1_rlast = ctrl_auto_in_rlast; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_0_awready = fb_auto_in_awready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_0_wready = fb_auto_in_wready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_0_bvalid = fb_auto_in_bvalid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_0_bid = fb_auto_in_bid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_0_bresp = fb_auto_in_bresp; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_0_rvalid = bundleIn_0_rvalid_r; // @[Nodes.scala 1210:84 AXI4VGA.scala 157:19]
  assign auto_in_0_rid = fb_auto_in_rid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign auto_in_0_rlast = fb_auto_in_rlast; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign fb_clock = clock;
  assign fb_reset = reset;
  assign fb_auto_in_awvalid = auto_in_0_awvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awid = auto_in_0_awid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awaddr = auto_in_0_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awlen = auto_in_0_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awsize = auto_in_0_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awburst = auto_in_0_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awlock = auto_in_0_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awcache = auto_in_0_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awprot = auto_in_0_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_awqos = auto_in_0_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_wvalid = auto_in_0_wvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_wdata = auto_in_0_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_wstrb = auto_in_0_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_wlast = auto_in_0_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_bready = auto_in_0_bready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_arvalid = bundleOut_0_arvalid_REG & hCounterIs2; // @[AXI4VGA.scala 185:43]
  assign fb_auto_in_arid = auto_in_0_arid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_araddr = {{18'd0}, _bundleOut_0_araddr_T_1}; // @[Nodes.scala 1207:84 AXI4VGA.scala 184:25]
  assign fb_auto_in_arlock = auto_in_0_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_arcache = auto_in_0_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign fb_auto_in_arqos = auto_in_0_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_clock = clock;
  assign ctrl_reset = reset;
  assign ctrl_auto_in_awvalid = auto_in_1_awvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awid = auto_in_1_awid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awaddr = auto_in_1_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awlen = auto_in_1_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awsize = auto_in_1_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awburst = auto_in_1_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awlock = auto_in_1_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awcache = auto_in_1_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awprot = auto_in_1_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_awqos = auto_in_1_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_wvalid = auto_in_1_wvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_wdata = auto_in_1_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_wstrb = auto_in_1_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_wlast = auto_in_1_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_bready = auto_in_1_bready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arvalid = auto_in_1_arvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arid = auto_in_1_arid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_araddr = auto_in_1_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arlen = auto_in_1_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arsize = auto_in_1_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arburst = auto_in_1_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arlock = auto_in_1_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arcache = auto_in_1_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arprot = auto_in_1_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_arqos = auto_in_1_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign ctrl_auto_in_rready = auto_in_1_rready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  always @(posedge clock) begin
    if (reset) begin // @[StopWatch.scala 23:20]
      bundleIn_0_rvalid_r <= 1'h0; // @[StopWatch.scala 23:20]
    end else begin
      bundleIn_0_rvalid_r <= _GEN_1;
    end
    if (reset) begin // @[Counter.scala 62:40]
      hCounter <= 11'h0; // @[Counter.scala 62:40]
    end else if (wrap_wrap) begin // @[Counter.scala 88:20]
      hCounter <= 11'h0; // @[Counter.scala 88:28]
    end else begin
      hCounter <= _wrap_value_T_1; // @[Counter.scala 78:15]
    end
    if (reset) begin // @[Counter.scala 62:40]
      vCounter <= 10'h0; // @[Counter.scala 62:40]
    end else if (wrap_wrap) begin // @[Counter.scala 120:16]
      if (wrap_wrap_1) begin // @[Counter.scala 88:20]
        vCounter <= 10'h0; // @[Counter.scala 88:28]
      end else begin
        vCounter <= _wrap_value_T_3; // @[Counter.scala 78:15]
      end
    end
    if (reset) begin // @[Counter.scala 62:40]
      fbPixelAddrV0 <= 17'h0; // @[Counter.scala 62:40]
    end else if (_fbPixelAddrV0_T_1) begin // @[Counter.scala 120:16]
      if (fbPixelAddrV0_wrap_wrap) begin // @[Counter.scala 88:20]
        fbPixelAddrV0 <= 17'h0; // @[Counter.scala 88:28]
      end else begin
        fbPixelAddrV0 <= _fbPixelAddrV0_wrap_value_T_1; // @[Counter.scala 78:15]
      end
    end
    if (reset) begin // @[Counter.scala 62:40]
      fbPixelAddrV1 <= 17'h0; // @[Counter.scala 62:40]
    end else if (_fbPixelAddrV1_T) begin // @[Counter.scala 120:16]
      if (fbPixelAddrV1_wrap_wrap) begin // @[Counter.scala 88:20]
        fbPixelAddrV1 <= 17'h0; // @[Counter.scala 88:28]
      end else begin
        fbPixelAddrV1 <= _fbPixelAddrV1_wrap_value_T_1; // @[Counter.scala 78:15]
      end
    end
    bundleOut_0_arvalid_REG <= _nextPixel_T_2 & vInRange & hCounterIsOdd; // @[AXI4VGA.scala 175:80]
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
  bundleIn_0_rvalid_r = _RAND_0[0:0];
  _RAND_1 = {1{`RANDOM}};
  hCounter = _RAND_1[10:0];
  _RAND_2 = {1{`RANDOM}};
  vCounter = _RAND_2[9:0];
  _RAND_3 = {1{`RANDOM}};
  fbPixelAddrV0 = _RAND_3[16:0];
  _RAND_4 = {1{`RANDOM}};
  fbPixelAddrV1 = _RAND_4[16:0];
  _RAND_5 = {1{`RANDOM}};
  bundleOut_0_arvalid_REG = _RAND_5[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

