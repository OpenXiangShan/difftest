module AXI4UART(
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
  output        io_extra_out_valid,
  output [7:0]  io_extra_out_ch,
  output        io_extra_in_valid,
  input  [7:0]  io_extra_in_ch
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
`endif // RANDOMIZE_REG_INIT
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
  reg [31:0] txfifo; // @[AXI4UART.scala 34:21]
  reg [31:0] stat; // @[AXI4UART.scala 35:23]
  reg [31:0] ctrl; // @[AXI4UART.scala 36:23]
  wire  _io_extra_out_valid_T_1 = _GEN_13[3:0] == 4'h4; // @[AXI4UART.scala 38:43]
  wire [63:0] in_wdata = auto_in_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  wire [7:0] _T_47 = in_wstrb >> _GEN_13[2:0]; // @[AXI4UART.scala 50:74]
  wire [7:0] _T_57 = _T_47[0] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_59 = _T_47[1] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_61 = _T_47[2] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_63 = _T_47[3] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_65 = _T_47[4] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_67 = _T_47[5] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_69 = _T_47[6] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [7:0] _T_71 = _T_47[7] ? 8'hff : 8'h0; // @[Bitwise.scala 74:12]
  wire [63:0] _T_72 = {_T_71,_T_69,_T_67,_T_65,_T_63,_T_61,_T_59,_T_57}; // @[Cat.scala 31:58]
  wire  _bundleIn_0_rdata_T = 4'h0 == _GEN_10[3:0]; // @[LookupTree.scala 24:34]
  wire  _bundleIn_0_rdata_T_1 = 4'h4 == _GEN_10[3:0]; // @[LookupTree.scala 24:34]
  wire  _bundleIn_0_rdata_T_2 = 4'h8 == _GEN_10[3:0]; // @[LookupTree.scala 24:34]
  wire  _bundleIn_0_rdata_T_3 = 4'hc == _GEN_10[3:0]; // @[LookupTree.scala 24:34]
  wire [7:0] _bundleIn_0_rdata_T_4 = _bundleIn_0_rdata_T ? io_extra_in_ch : 8'h0; // @[Mux.scala 27:73]
  wire [31:0] _bundleIn_0_rdata_T_5 = _bundleIn_0_rdata_T_1 ? txfifo : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _bundleIn_0_rdata_T_6 = _bundleIn_0_rdata_T_2 ? stat : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _bundleIn_0_rdata_T_7 = _bundleIn_0_rdata_T_3 ? ctrl : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _GEN_21 = {{24'd0}, _bundleIn_0_rdata_T_4}; // @[Mux.scala 27:73]
  wire [31:0] _bundleIn_0_rdata_T_8 = _GEN_21 | _bundleIn_0_rdata_T_5; // @[Mux.scala 27:73]
  wire [31:0] _bundleIn_0_rdata_T_9 = _bundleIn_0_rdata_T_8 | _bundleIn_0_rdata_T_6; // @[Mux.scala 27:73]
  wire [31:0] _bundleIn_0_rdata_T_10 = _bundleIn_0_rdata_T_9 | _bundleIn_0_rdata_T_7; // @[Mux.scala 27:73]
  wire [63:0] _txfifo_T = in_wdata & _T_72; // @[BitUtils.scala 35:14]
  wire [63:0] _txfifo_T_1 = ~_T_72; // @[BitUtils.scala 35:39]
  wire [63:0] _GEN_22 = {{32'd0}, txfifo}; // @[BitUtils.scala 35:37]
  wire [63:0] _txfifo_T_2 = _GEN_22 & _txfifo_T_1; // @[BitUtils.scala 35:37]
  wire [63:0] _txfifo_T_3 = _txfifo_T | _txfifo_T_2; // @[BitUtils.scala 35:26]
  wire [63:0] _GEN_18 = _T_2 & _io_extra_out_valid_T_1 ? _txfifo_T_3 : {{32'd0}, txfifo}; // @[RegMap.scala 30:{48,52} AXI4UART.scala 34:21]
  wire [63:0] _GEN_23 = {{32'd0}, stat}; // @[BitUtils.scala 35:37]
  wire [63:0] _stat_T_2 = _GEN_23 & _txfifo_T_1; // @[BitUtils.scala 35:37]
  wire [63:0] _stat_T_3 = _txfifo_T | _stat_T_2; // @[BitUtils.scala 35:26]
  wire [63:0] _GEN_19 = _T_2 & _GEN_13[3:0] == 4'h8 ? _stat_T_3 : {{32'd0}, stat}; // @[RegMap.scala 30:{48,52} AXI4UART.scala 35:23]
  wire [63:0] _GEN_24 = {{32'd0}, ctrl}; // @[BitUtils.scala 35:37]
  wire [63:0] _ctrl_T_2 = _GEN_24 & _txfifo_T_1; // @[BitUtils.scala 35:37]
  wire [63:0] _ctrl_T_3 = _txfifo_T | _ctrl_T_2; // @[BitUtils.scala 35:26]
  wire [63:0] _GEN_20 = _T_2 & _GEN_13[3:0] == 4'hc ? _ctrl_T_3 : {{32'd0}, ctrl}; // @[RegMap.scala 30:{48,52} AXI4UART.scala 36:23]
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
  wire [63:0] in_rdata = {{32'd0}, _bundleIn_0_rdata_T_10}; // @[Nodes.scala 1210:84 RegMap.scala 28:11]
  wire [1:0] in_rresp = 2'h0; // @[Nodes.scala 1210:84 AXI4SlaveModule.scala 154:18]
  wire [63:0] _GEN_25 = reset ? 64'h1 : _GEN_19; // @[AXI4UART.scala 35:{23,23}]
  wire [63:0] _GEN_26 = reset ? 64'h0 : _GEN_20; // @[AXI4UART.scala 36:{23,23}]
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
  assign io_extra_out_valid = _GEN_13[3:0] == 4'h4 & _T_2; // @[AXI4UART.scala 38:51]
  assign io_extra_out_ch = in_wdata[7:0]; // @[AXI4UART.scala 39:42]
  assign io_extra_in_valid = _GEN_10[3:0] == 4'h0 & _T_4; // @[AXI4UART.scala 40:50]
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
    txfifo <= _GEN_18[31:0];
    stat <= _GEN_25[31:0]; // @[AXI4UART.scala 35:{23,23}]
    ctrl <= _GEN_26[31:0]; // @[AXI4UART.scala 36:{23,23}]
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
  txfifo = _RAND_7[31:0];
  _RAND_8 = {1{`RANDOM}};
  stat = _RAND_8[31:0];
  _RAND_9 = {1{`RANDOM}};
  ctrl = _RAND_9[31:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

