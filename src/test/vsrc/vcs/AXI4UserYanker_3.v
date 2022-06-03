module AXI4UserYanker_3(
  input          clock,
  input          reset,
  output         auto_in_awready,
  input          auto_in_awvalid,
  input  [4:0]   auto_in_awid,
  input  [37:0]  auto_in_awaddr,
  input  [7:0]   auto_in_awlen,
  input  [2:0]   auto_in_awsize,
  input  [3:0]   auto_in_awcache,
  input  [2:0]   auto_in_awprot,
  output         auto_in_wready,
  input          auto_in_wvalid,
  input  [255:0] auto_in_wdata,
  input  [31:0]  auto_in_wstrb,
  input          auto_in_wlast,
  input          auto_in_bready,
  output         auto_in_bvalid,
  output [4:0]   auto_in_bid,
  output [1:0]   auto_in_bresp,
  output         auto_in_arready,
  input          auto_in_arvalid,
  input  [4:0]   auto_in_arid,
  input  [37:0]  auto_in_araddr,
  input  [7:0]   auto_in_arlen,
  input  [2:0]   auto_in_arsize,
  input  [3:0]   auto_in_arcache,
  input  [2:0]   auto_in_arprot,
  input          auto_in_rready,
  output         auto_in_rvalid,
  output [4:0]   auto_in_rid,
  output [255:0] auto_in_rdata,
  output [1:0]   auto_in_rresp,
  output         auto_in_rlast,
  input          auto_out_awready,
  output         auto_out_awvalid,
  output [4:0]   auto_out_awid,
  output [37:0]  auto_out_awaddr,
  output [7:0]   auto_out_awlen,
  output [2:0]   auto_out_awsize,
  output [3:0]   auto_out_awcache,
  output [2:0]   auto_out_awprot,
  input          auto_out_wready,
  output         auto_out_wvalid,
  output [255:0] auto_out_wdata,
  output [31:0]  auto_out_wstrb,
  output         auto_out_wlast,
  output         auto_out_bready,
  input          auto_out_bvalid,
  input  [4:0]   auto_out_bid,
  input  [1:0]   auto_out_bresp,
  input          auto_out_arready,
  output         auto_out_arvalid,
  output [4:0]   auto_out_arid,
  output [37:0]  auto_out_araddr,
  output [7:0]   auto_out_arlen,
  output [2:0]   auto_out_arsize,
  output [3:0]   auto_out_arcache,
  output [2:0]   auto_out_arprot,
  output         auto_out_rready,
  input          auto_out_rvalid,
  input  [4:0]   auto_out_rid,
  input  [255:0] auto_out_rdata,
  input  [1:0]   auto_out_rresp,
  input          auto_out_rlast
);
  wire  QueueCompatibility_clock; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_reset; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_io_enq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_io_enq_valid; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_io_deq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_io_deq_valid; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_clock; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_reset; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_io_enq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_io_enq_valid; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_io_deq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_1_io_deq_valid; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_clock; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_reset; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_io_enq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_io_enq_valid; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_io_deq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_2_io_deq_valid; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_clock; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_reset; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_io_enq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_io_enq_valid; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_io_deq_ready; // @[UserYanker.scala 47:17]
  wire  QueueCompatibility_3_io_deq_valid; // @[UserYanker.scala 47:17]
  wire  _arready_WIRE_0 = QueueCompatibility_io_enq_ready; // @[UserYanker.scala 55:{25,25}]
  wire  _arready_WIRE_1 = QueueCompatibility_1_io_enq_ready; // @[UserYanker.scala 55:{25,25}]
  wire  _GEN_1 = 5'h1 == auto_in_arid ? _arready_WIRE_1 : _arready_WIRE_0; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_2 = 5'h2 == auto_in_arid ? 1'h0 : _GEN_1; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_3 = 5'h3 == auto_in_arid ? 1'h0 : _GEN_2; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_4 = 5'h4 == auto_in_arid ? 1'h0 : _GEN_3; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_5 = 5'h5 == auto_in_arid ? 1'h0 : _GEN_4; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_6 = 5'h6 == auto_in_arid ? 1'h0 : _GEN_5; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_7 = 5'h7 == auto_in_arid ? 1'h0 : _GEN_6; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_8 = 5'h8 == auto_in_arid ? 1'h0 : _GEN_7; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_9 = 5'h9 == auto_in_arid ? 1'h0 : _GEN_8; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_10 = 5'ha == auto_in_arid ? 1'h0 : _GEN_9; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_11 = 5'hb == auto_in_arid ? 1'h0 : _GEN_10; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_12 = 5'hc == auto_in_arid ? 1'h0 : _GEN_11; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_13 = 5'hd == auto_in_arid ? 1'h0 : _GEN_12; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_14 = 5'he == auto_in_arid ? 1'h0 : _GEN_13; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_15 = 5'hf == auto_in_arid ? 1'h0 : _GEN_14; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_16 = 5'h10 == auto_in_arid ? 1'h0 : _GEN_15; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_17 = 5'h11 == auto_in_arid ? 1'h0 : _GEN_16; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_18 = 5'h12 == auto_in_arid ? 1'h0 : _GEN_17; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_19 = 5'h13 == auto_in_arid ? 1'h0 : _GEN_18; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_20 = 5'h14 == auto_in_arid ? 1'h0 : _GEN_19; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_21 = 5'h15 == auto_in_arid ? 1'h0 : _GEN_20; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_22 = 5'h16 == auto_in_arid ? 1'h0 : _GEN_21; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_23 = 5'h17 == auto_in_arid ? 1'h0 : _GEN_22; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_24 = 5'h18 == auto_in_arid ? 1'h0 : _GEN_23; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_25 = 5'h19 == auto_in_arid ? 1'h0 : _GEN_24; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_26 = 5'h1a == auto_in_arid ? 1'h0 : _GEN_25; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_27 = 5'h1b == auto_in_arid ? 1'h0 : _GEN_26; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_28 = 5'h1c == auto_in_arid ? 1'h0 : _GEN_27; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_29 = 5'h1d == auto_in_arid ? 1'h0 : _GEN_28; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_30 = 5'h1e == auto_in_arid ? 1'h0 : _GEN_29; // @[UserYanker.scala 56:{36,36}]
  wire  _GEN_31 = 5'h1f == auto_in_arid ? 1'h0 : _GEN_30; // @[UserYanker.scala 56:{36,36}]
  wire [31:0] _arsel_T = 32'h1 << auto_in_arid; // @[OneHot.scala 64:12]
  wire  arsel_0 = _arsel_T[0]; // @[UserYanker.scala 67:55]
  wire  arsel_1 = _arsel_T[1]; // @[UserYanker.scala 67:55]
  wire [31:0] _rsel_T = 32'h1 << auto_out_rid; // @[OneHot.scala 64:12]
  wire  rsel_0 = _rsel_T[0]; // @[UserYanker.scala 68:55]
  wire  rsel_1 = _rsel_T[1]; // @[UserYanker.scala 68:55]
  wire  _awready_WIRE_0 = QueueCompatibility_2_io_enq_ready; // @[UserYanker.scala 76:{25,25}]
  wire  _awready_WIRE_1 = QueueCompatibility_3_io_enq_ready; // @[UserYanker.scala 76:{25,25}]
  wire  _GEN_65 = 5'h1 == auto_in_awid ? _awready_WIRE_1 : _awready_WIRE_0; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_66 = 5'h2 == auto_in_awid ? 1'h0 : _GEN_65; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_67 = 5'h3 == auto_in_awid ? 1'h0 : _GEN_66; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_68 = 5'h4 == auto_in_awid ? 1'h0 : _GEN_67; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_69 = 5'h5 == auto_in_awid ? 1'h0 : _GEN_68; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_70 = 5'h6 == auto_in_awid ? 1'h0 : _GEN_69; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_71 = 5'h7 == auto_in_awid ? 1'h0 : _GEN_70; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_72 = 5'h8 == auto_in_awid ? 1'h0 : _GEN_71; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_73 = 5'h9 == auto_in_awid ? 1'h0 : _GEN_72; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_74 = 5'ha == auto_in_awid ? 1'h0 : _GEN_73; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_75 = 5'hb == auto_in_awid ? 1'h0 : _GEN_74; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_76 = 5'hc == auto_in_awid ? 1'h0 : _GEN_75; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_77 = 5'hd == auto_in_awid ? 1'h0 : _GEN_76; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_78 = 5'he == auto_in_awid ? 1'h0 : _GEN_77; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_79 = 5'hf == auto_in_awid ? 1'h0 : _GEN_78; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_80 = 5'h10 == auto_in_awid ? 1'h0 : _GEN_79; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_81 = 5'h11 == auto_in_awid ? 1'h0 : _GEN_80; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_82 = 5'h12 == auto_in_awid ? 1'h0 : _GEN_81; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_83 = 5'h13 == auto_in_awid ? 1'h0 : _GEN_82; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_84 = 5'h14 == auto_in_awid ? 1'h0 : _GEN_83; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_85 = 5'h15 == auto_in_awid ? 1'h0 : _GEN_84; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_86 = 5'h16 == auto_in_awid ? 1'h0 : _GEN_85; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_87 = 5'h17 == auto_in_awid ? 1'h0 : _GEN_86; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_88 = 5'h18 == auto_in_awid ? 1'h0 : _GEN_87; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_89 = 5'h19 == auto_in_awid ? 1'h0 : _GEN_88; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_90 = 5'h1a == auto_in_awid ? 1'h0 : _GEN_89; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_91 = 5'h1b == auto_in_awid ? 1'h0 : _GEN_90; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_92 = 5'h1c == auto_in_awid ? 1'h0 : _GEN_91; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_93 = 5'h1d == auto_in_awid ? 1'h0 : _GEN_92; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_94 = 5'h1e == auto_in_awid ? 1'h0 : _GEN_93; // @[UserYanker.scala 77:{36,36}]
  wire  _GEN_95 = 5'h1f == auto_in_awid ? 1'h0 : _GEN_94; // @[UserYanker.scala 77:{36,36}]
  wire [31:0] _awsel_T = 32'h1 << auto_in_awid; // @[OneHot.scala 64:12]
  wire  awsel_0 = _awsel_T[0]; // @[UserYanker.scala 88:55]
  wire  awsel_1 = _awsel_T[1]; // @[UserYanker.scala 88:55]
  wire [31:0] _bsel_T = 32'h1 << auto_out_bid; // @[OneHot.scala 64:12]
  wire  bsel_0 = _bsel_T[0]; // @[UserYanker.scala 89:55]
  wire  bsel_1 = _bsel_T[1]; // @[UserYanker.scala 89:55]
  QueueCompatibility_206 QueueCompatibility ( // @[UserYanker.scala 47:17]
    .clock(QueueCompatibility_clock),
    .reset(QueueCompatibility_reset),
    .io_enq_ready(QueueCompatibility_io_enq_ready),
    .io_enq_valid(QueueCompatibility_io_enq_valid),
    .io_deq_ready(QueueCompatibility_io_deq_ready),
    .io_deq_valid(QueueCompatibility_io_deq_valid)
  );
  QueueCompatibility_206 QueueCompatibility_1 ( // @[UserYanker.scala 47:17]
    .clock(QueueCompatibility_1_clock),
    .reset(QueueCompatibility_1_reset),
    .io_enq_ready(QueueCompatibility_1_io_enq_ready),
    .io_enq_valid(QueueCompatibility_1_io_enq_valid),
    .io_deq_ready(QueueCompatibility_1_io_deq_ready),
    .io_deq_valid(QueueCompatibility_1_io_deq_valid)
  );
  QueueCompatibility_206 QueueCompatibility_2 ( // @[UserYanker.scala 47:17]
    .clock(QueueCompatibility_2_clock),
    .reset(QueueCompatibility_2_reset),
    .io_enq_ready(QueueCompatibility_2_io_enq_ready),
    .io_enq_valid(QueueCompatibility_2_io_enq_valid),
    .io_deq_ready(QueueCompatibility_2_io_deq_ready),
    .io_deq_valid(QueueCompatibility_2_io_deq_valid)
  );
  QueueCompatibility_206 QueueCompatibility_3 ( // @[UserYanker.scala 47:17]
    .clock(QueueCompatibility_3_clock),
    .reset(QueueCompatibility_3_reset),
    .io_enq_ready(QueueCompatibility_3_io_enq_ready),
    .io_enq_valid(QueueCompatibility_3_io_enq_valid),
    .io_deq_ready(QueueCompatibility_3_io_deq_ready),
    .io_deq_valid(QueueCompatibility_3_io_deq_valid)
  );
  assign auto_in_awready = auto_out_awready & _GEN_95; // @[UserYanker.scala 77:36]
  assign auto_in_wready = auto_out_wready; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_bvalid = auto_out_bvalid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_bid = auto_out_bid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_bresp = auto_out_bresp; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_arready = auto_out_arready & _GEN_31; // @[UserYanker.scala 56:36]
  assign auto_in_rvalid = auto_out_rvalid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_rid = auto_out_rid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_rdata = auto_out_rdata; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_rresp = auto_out_rresp; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_rlast = auto_out_rlast; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_out_awvalid = auto_in_awvalid & _GEN_95; // @[UserYanker.scala 78:36]
  assign auto_out_awid = auto_in_awid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awaddr = auto_in_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awlen = auto_in_awlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awsize = auto_in_awsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awcache = auto_in_awcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_awprot = auto_in_awprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_wvalid = auto_in_wvalid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_wstrb = auto_in_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_wlast = auto_in_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_bready = auto_in_bready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arvalid = auto_in_arvalid & _GEN_31; // @[UserYanker.scala 57:36]
  assign auto_out_arid = auto_in_arid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_araddr = auto_in_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arlen = auto_in_arlen; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arsize = auto_in_arsize; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arcache = auto_in_arcache; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_arprot = auto_in_arprot; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_rready = auto_in_rready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign QueueCompatibility_clock = clock;
  assign QueueCompatibility_reset = reset;
  assign QueueCompatibility_io_enq_valid = auto_in_arvalid & auto_out_arready & arsel_0; // @[UserYanker.scala 71:53]
  assign QueueCompatibility_io_deq_ready = auto_out_rvalid & auto_in_rready & rsel_0 & auto_out_rlast; // @[UserYanker.scala 70:58]
  assign QueueCompatibility_1_clock = clock;
  assign QueueCompatibility_1_reset = reset;
  assign QueueCompatibility_1_io_enq_valid = auto_in_arvalid & auto_out_arready & arsel_1; // @[UserYanker.scala 71:53]
  assign QueueCompatibility_1_io_deq_ready = auto_out_rvalid & auto_in_rready & rsel_1 & auto_out_rlast; // @[UserYanker.scala 70:58]
  assign QueueCompatibility_2_clock = clock;
  assign QueueCompatibility_2_reset = reset;
  assign QueueCompatibility_2_io_enq_valid = auto_in_awvalid & auto_out_awready & awsel_0; // @[UserYanker.scala 92:53]
  assign QueueCompatibility_2_io_deq_ready = auto_out_bvalid & auto_in_bready & bsel_0; // @[UserYanker.scala 91:53]
  assign QueueCompatibility_3_clock = clock;
  assign QueueCompatibility_3_reset = reset;
  assign QueueCompatibility_3_io_enq_valid = auto_in_awvalid & auto_out_awready & awsel_1; // @[UserYanker.scala 92:53]
  assign QueueCompatibility_3_io_deq_ready = auto_out_bvalid & auto_in_bready & bsel_1; // @[UserYanker.scala 91:53]
endmodule

