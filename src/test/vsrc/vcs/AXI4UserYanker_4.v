module AXI4UserYanker_4(
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
  input  [3:0]  auto_in_awecho_tl_state_size,
  input  [1:0]  auto_in_awecho_tl_state_source,
  output        auto_in_wready,
  input         auto_in_wvalid,
  input  [63:0] auto_in_wdata,
  input  [7:0]  auto_in_wstrb,
  input         auto_in_wlast,
  input         auto_in_bready,
  output        auto_in_bvalid,
  output        auto_in_bid,
  output [1:0]  auto_in_bresp,
  output [3:0]  auto_in_becho_tl_state_size,
  output [1:0]  auto_in_becho_tl_state_source,
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
  input  [3:0]  auto_in_arecho_tl_state_size,
  input  [1:0]  auto_in_arecho_tl_state_source,
  input         auto_in_rready,
  output        auto_in_rvalid,
  output        auto_in_rid,
  output [63:0] auto_in_rdata,
  output [1:0]  auto_in_rresp,
  output [3:0]  auto_in_recho_tl_state_size,
  output [1:0]  auto_in_recho_tl_state_source,
  output        auto_in_rlast,
  input         auto_out_awready,
  output        auto_out_awvalid,
  output        auto_out_awid,
  output [36:0] auto_out_awaddr,
  output [7:0]  auto_out_awlen,
  output [2:0]  auto_out_awsize,
  output [1:0]  auto_out_awburst,
  output        auto_out_awlock,
  output [3:0]  auto_out_awcache,
  output [2:0]  auto_out_awprot,
  output [3:0]  auto_out_awqos,
  input         auto_out_wready,
  output        auto_out_wvalid,
  output [63:0] auto_out_wdata,
  output [7:0]  auto_out_wstrb,
  output        auto_out_wlast,
  output        auto_out_bready,
  input         auto_out_bvalid,
  input         auto_out_bid,
  input  [1:0]  auto_out_bresp,
  input         auto_out_arready,
  output        auto_out_arvalid,
  output        auto_out_arid,
  output [36:0] auto_out_araddr,
  output [7:0]  auto_out_arlen,
  output [2:0]  auto_out_arsize,
  output [1:0]  auto_out_arburst,
  output        auto_out_arlock,
  output [3:0]  auto_out_arcache,
  output [2:0]  auto_out_arprot,
  output [3:0]  auto_out_arqos,
  output        auto_out_rready,
  input         auto_out_rvalid,
  input         auto_out_rid,
  input  [63:0] auto_out_rdata,
  input  [1:0]  auto_out_rresp,
  input         auto_out_rlast
);
  wire  QueueCompatibility_clock; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_reset; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_io_enq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_io_enq_valid; // @[UserYanker.scala 47:17]
  wire [3:0] QueueCompatibility_io_enq_bits_tl_state_size; // @[UserYanker.scala 47:17]
  wire [1:0] QueueCompatibility_io_enq_bits_tl_state_source; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_io_deq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_io_deq_valid; // @[UserYanker.scala 47:17]
  wire [3:0] QueueCompatibility_io_deq_bits_tl_state_size; // @[UserYanker.scala 47:17]
  wire [1:0] QueueCompatibility_io_deq_bits_tl_state_source; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_clock; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_reset; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_io_enq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_io_enq_valid; // @[UserYanker.scala 47:17]
  wire [3:0] QueueCompatibility_1_io_enq_bits_tl_state_size; // @[UserYanker.scala 47:17]
  wire [1:0] QueueCompatibility_1_io_enq_bits_tl_state_source; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_io_deq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_io_deq_valid; // @[UserYanker.scala 47:17]
  wire [3:0] QueueCompatibility_1_io_deq_bits_tl_state_size; // @[UserYanker.scala 47:17]
  wire [1:0] QueueCompatibility_1_io_deq_bits_tl_state_source; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_clock; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_reset; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_io_enq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_io_enq_valid; // @[UserYanker.scala 47:17]
  wire [3:0] QueueCompatibility_2_io_enq_bits_tl_state_size; // @[UserYanker.scala 47:17]
  wire [1:0] QueueCompatibility_2_io_enq_bits_tl_state_source; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_io_deq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_io_deq_valid; // @[UserYanker.scala 47:17]
  wire [3:0] QueueCompatibility_2_io_deq_bits_tl_state_size; // @[UserYanker.scala 47:17]
  wire [1:0] QueueCompatibility_2_io_deq_bits_tl_state_source; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_clock; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_reset; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_io_enq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_io_enq_valid; // @[UserYanker.scala 47:17]
  wire [3:0] QueueCompatibility_3_io_enq_bits_tl_state_size; // @[UserYanker.scala 47:17]
  wire [1:0] QueueCompatibility_3_io_enq_bits_tl_state_source; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_io_deq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_io_deq_valid; // @[UserYanker.scala 47:17]
  wire [3:0] QueueCompatibility_3_io_deq_bits_tl_state_size; // @[UserYanker.scala 47:17]
  wire [1:0] QueueCompatibility_3_io_deq_bits_tl_state_source; // @[UserYanker.scala 47:17]
  wire  _arready_WIRE_0 = QueueCompatibility_io_enq_ready; // @[UserYanker.scala 55:{25,25}]
  wire  _arready_WIRE_1 = QueueCompatibility_1_io_enq_ready; // @[UserYanker.scala 55:{25,25}]
  wire  _GEN_1 = auto_in_arid ? _arready_WIRE_1 : _arready_WIRE_0; // @[UserYanker.scala 56:{36,36}]
  wire [1:0] _rWIRE_0_tl_state_source = QueueCompatibility_io_deq_bits_tl_state_source; // @[UserYanker.scala 62:{23,23}]
  wire [1:0] _rWIRE_1_tl_state_source = QueueCompatibility_1_io_deq_bits_tl_state_source; // @[UserYanker.scala 62:{23,23}]
  wire [3:0] _rWIRE_0_tl_state_size = QueueCompatibility_io_deq_bits_tl_state_size; // @[UserYanker.scala 62:{23,23}]
  wire [3:0] _rWIRE_1_tl_state_size = QueueCompatibility_1_io_deq_bits_tl_state_size; // @[UserYanker.scala 62:{23,23}]
  wire [1:0] _arsel_T = 2'h1 << auto_in_arid; // @[OneHot.scala 64:12]
  wire  arsel_0 = _arsel_T[0]; // @[UserYanker.scala 67:55]
  wire  arsel_1 = _arsel_T[1]; // @[UserYanker.scala 67:55]
  wire [1:0] _rsel_T = 2'h1 << auto_out_rid; // @[OneHot.scala 64:12]
  wire  rsel_0 = _rsel_T[0]; // @[UserYanker.scala 68:55]
  wire  rsel_1 = _rsel_T[1]; // @[UserYanker.scala 68:55]
  wire  _awready_WIRE_0 = QueueCompatibility_2_io_enq_ready; // @[UserYanker.scala 76:{25,25}]
  wire  _awready_WIRE_1 = QueueCompatibility_3_io_enq_ready; // @[UserYanker.scala 76:{25,25}]
  wire  _GEN_9 = auto_in_awid ? _awready_WIRE_1 : _awready_WIRE_0; // @[UserYanker.scala 77:{36,36}]
  wire [1:0] _bWIRE_0_tl_state_source = QueueCompatibility_2_io_deq_bits_tl_state_source; // @[UserYanker.scala 83:{23,23}]
  wire [1:0] _bWIRE_1_tl_state_source = QueueCompatibility_3_io_deq_bits_tl_state_source; // @[UserYanker.scala 83:{23,23}]
  wire [3:0] _bWIRE_0_tl_state_size = QueueCompatibility_2_io_deq_bits_tl_state_size; // @[UserYanker.scala 83:{23,23}]
  wire [3:0] _bWIRE_1_tl_state_size = QueueCompatibility_3_io_deq_bits_tl_state_size; // @[UserYanker.scala 83:{23,23}]
  wire [1:0] _awsel_T = 2'h1 << auto_in_awid; // @[OneHot.scala 64:12]
  wire  awsel_0 = _awsel_T[0]; // @[UserYanker.scala 88:55]
  wire  awsel_1 = _awsel_T[1]; // @[UserYanker.scala 88:55]
  wire [1:0] _bsel_T = 2'h1 << auto_out_bid; // @[OneHot.scala 64:12]
  wire  bsel_0 = _bsel_T[0]; // @[UserYanker.scala 89:55]
  wire  bsel_1 = _bsel_T[1]; // @[UserYanker.scala 89:55]
  QueueCompatibility_210 QueueCompatibility ( // @[UserYanker.scala 47:17]
    .clock(QueueCompatibility_clock),
    .reset(QueueCompatibility_reset),
    .io_enq_ready(QueueCompatibility_io_enq_ready),
    .io_enq_valid(QueueCompatibility_io_enq_valid),
    .io_enq_bits_tl_state_size(QueueCompatibility_io_enq_bits_tl_state_size),
    .io_enq_bits_tl_state_source(QueueCompatibility_io_enq_bits_tl_state_source),
    .io_deq_ready(QueueCompatibility_io_deq_ready),
    .io_deq_valid(QueueCompatibility_io_deq_valid),
    .io_deq_bits_tl_state_size(QueueCompatibility_io_deq_bits_tl_state_size),
    .io_deq_bits_tl_state_source(QueueCompatibility_io_deq_bits_tl_state_source)
  );
  QueueCompatibility_210 QueueCompatibility_1 ( // @[UserYanker.scala 47:17]
    .clock(QueueCompatibility_1_clock),
    .reset(QueueCompatibility_1_reset),
    .io_enq_ready(QueueCompatibility_1_io_enq_ready),
    .io_enq_valid(QueueCompatibility_1_io_enq_valid),
    .io_enq_bits_tl_state_size(QueueCompatibility_1_io_enq_bits_tl_state_size),
    .io_enq_bits_tl_state_source(QueueCompatibility_1_io_enq_bits_tl_state_source),
    .io_deq_ready(QueueCompatibility_1_io_deq_ready),
    .io_deq_valid(QueueCompatibility_1_io_deq_valid),
    .io_deq_bits_tl_state_size(QueueCompatibility_1_io_deq_bits_tl_state_size),
    .io_deq_bits_tl_state_source(QueueCompatibility_1_io_deq_bits_tl_state_source)
  );
  QueueCompatibility_210 QueueCompatibility_2 ( // @[UserYanker.scala 47:17]
    .clock(QueueCompatibility_2_clock),
    .reset(QueueCompatibility_2_reset),
    .io_enq_ready(QueueCompatibility_2_io_enq_ready),
    .io_enq_valid(QueueCompatibility_2_io_enq_valid),
    .io_enq_bits_tl_state_size(QueueCompatibility_2_io_enq_bits_tl_state_size),
    .io_enq_bits_tl_state_source(QueueCompatibility_2_io_enq_bits_tl_state_source),
    .io_deq_ready(QueueCompatibility_2_io_deq_ready),
    .io_deq_valid(QueueCompatibility_2_io_deq_valid),
    .io_deq_bits_tl_state_size(QueueCompatibility_2_io_deq_bits_tl_state_size),
    .io_deq_bits_tl_state_source(QueueCompatibility_2_io_deq_bits_tl_state_source)
  );
  QueueCompatibility_210 QueueCompatibility_3 ( // @[UserYanker.scala 47:17]
    .clock(QueueCompatibility_3_clock),
    .reset(QueueCompatibility_3_reset),
    .io_enq_ready(QueueCompatibility_3_io_enq_ready),
    .io_enq_valid(QueueCompatibility_3_io_enq_valid),
    .io_enq_bits_tl_state_size(QueueCompatibility_3_io_enq_bits_tl_state_size),
    .io_enq_bits_tl_state_source(QueueCompatibility_3_io_enq_bits_tl_state_source),
    .io_deq_ready(QueueCompatibility_3_io_deq_ready),
    .io_deq_valid(QueueCompatibility_3_io_deq_valid),
    .io_deq_bits_tl_state_size(QueueCompatibility_3_io_deq_bits_tl_state_size),
    .io_deq_bits_tl_state_source(QueueCompatibility_3_io_deq_bits_tl_state_source)
  );
  assign auto_in_awready = auto_out_awready & _GEN_9; // @[UserYanker.scala 77:36]
  assign auto_in_wready = auto_out_wready; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_bvalid = auto_out_bvalid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_bid = auto_out_bid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_bresp = auto_out_bresp; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_becho_tl_state_size = auto_out_bid ? _bWIRE_1_tl_state_size :
    _bWIRE_0_tl_state_size; // @[BundleMap.scala 247:{19,19}]
  assign auto_in_becho_tl_state_source = auto_out_bid ? _bWIRE_1_tl_state_source :
    _bWIRE_0_tl_state_source; // @[BundleMap.scala 247:{19,19}]
  assign auto_in_arready = auto_out_arready & _GEN_1; // @[UserYanker.scala 56:36]
  assign auto_in_rvalid = auto_out_rvalid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_rid = auto_out_rid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_rdata = auto_out_rdata; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_rresp = auto_out_rresp; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_recho_tl_state_size = auto_out_rid ? _rWIRE_1_tl_state_size :
    _rWIRE_0_tl_state_size; // @[BundleMap.scala 247:{19,19}]
  assign auto_in_recho_tl_state_source = auto_out_rid ? _rWIRE_1_tl_state_source :
    _rWIRE_0_tl_state_source; // @[BundleMap.scala 247:{19,19}]
  assign auto_in_rlast = auto_out_rlast; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_out_awvalid = auto_in_awvalid & _GEN_9; // @[UserYanker.scala 78:36]
  assign auto_out_awid = auto_in_awid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awburst = auto_in_awburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awlock = auto_in_awlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awqos = auto_in_awqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_wvalid = auto_in_wvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_bready = auto_in_bready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arvalid = auto_in_arvalid & _GEN_1; // @[UserYanker.scala 57:36]
  assign auto_out_arid = auto_in_arid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arburst = auto_in_arburst; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arlock = auto_in_arlock; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arqos = auto_in_arqos; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_rready = auto_in_rready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_clock = clock;
  assign QueueCompatibility_reset = reset;
  assign QueueCompatibility_io_enq_valid = auto_in_arvalid & auto_out_arready & arsel_0; // @[UserYanker.scala 71:53]
  assign QueueCompatibility_io_enq_bits_tl_state_size = auto_in_arecho_tl_state_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_io_enq_bits_tl_state_source = auto_in_arecho_tl_state_source; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_io_deq_ready = auto_out_rvalid & auto_in_rready & rsel_0 & auto_out_rlast; // @[UserYanker.scala 70:58]
  assign QueueCompatibility_1_clock = clock;
  assign QueueCompatibility_1_reset = reset;
  assign QueueCompatibility_1_io_enq_valid = auto_in_arvalid & auto_out_arready & arsel_1; // @[UserYanker.scala 71:53]
  assign QueueCompatibility_1_io_enq_bits_tl_state_size = auto_in_arecho_tl_state_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_1_io_enq_bits_tl_state_source = auto_in_arecho_tl_state_source; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_1_io_deq_ready = auto_out_rvalid & auto_in_rready & rsel_1 & auto_out_rlast; // @[UserYanker.scala 70:58]
  assign QueueCompatibility_2_clock = clock;
  assign QueueCompatibility_2_reset = reset;
  assign QueueCompatibility_2_io_enq_valid = auto_in_awvalid & auto_out_awready & awsel_0; // @[UserYanker.scala 92:53]
  assign QueueCompatibility_2_io_enq_bits_tl_state_size = auto_in_awecho_tl_state_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_2_io_enq_bits_tl_state_source = auto_in_awecho_tl_state_source; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_2_io_deq_ready = auto_out_bvalid & auto_in_bready & bsel_0; // @[UserYanker.scala 91:53]
  assign QueueCompatibility_3_clock = clock;
  assign QueueCompatibility_3_reset = reset;
  assign QueueCompatibility_3_io_enq_valid = auto_in_awvalid & auto_out_awready & awsel_1; // @[UserYanker.scala 92:53]
  assign QueueCompatibility_3_io_enq_bits_tl_state_size = auto_in_awecho_tl_state_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_3_io_enq_bits_tl_state_source = auto_in_awecho_tl_state_source; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_3_io_deq_ready = auto_out_bvalid & auto_in_bready & bsel_1; // @[UserYanker.scala 91:53]
endmodule

