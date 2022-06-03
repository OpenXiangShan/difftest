module TLXbar_9(
  input         clock,
  input         reset,
  output        auto_in_a_ready,
  input         auto_in_a_valid,
  input  [2:0]  auto_in_a_bits_opcode,
  input  [2:0]  auto_in_a_bits_size,
  input  [1:0]  auto_in_a_bits_source,
  input  [37:0] auto_in_a_bits_address,
  input         auto_in_a_bits_user_amba_prot_bufferable,
  input         auto_in_a_bits_user_amba_prot_modifiable,
  input         auto_in_a_bits_user_amba_prot_readalloc,
  input         auto_in_a_bits_user_amba_prot_writealloc,
  input         auto_in_a_bits_user_amba_prot_privileged,
  input         auto_in_a_bits_user_amba_prot_secure,
  input         auto_in_a_bits_user_amba_prot_fetch,
  input  [7:0]  auto_in_a_bits_mask,
  input  [63:0] auto_in_a_bits_data,
  input         auto_in_d_ready,
  output        auto_in_d_valid,
  output [2:0]  auto_in_d_bits_opcode,
  output [2:0]  auto_in_d_bits_size,
  output [1:0]  auto_in_d_bits_source,
  output        auto_in_d_bits_denied,
  output [63:0] auto_in_d_bits_data,
  output        auto_in_d_bits_corrupt,
  input         auto_out_1_a_ready,
  output        auto_out_1_a_valid,
  output [2:0]  auto_out_1_a_bits_opcode,
  output [2:0]  auto_out_1_a_bits_size,
  output [1:0]  auto_out_1_a_bits_source,
  output [36:0] auto_out_1_a_bits_address,
  output        auto_out_1_a_bits_user_amba_prot_bufferable,
  output        auto_out_1_a_bits_user_amba_prot_modifiable,
  output        auto_out_1_a_bits_user_amba_prot_readalloc,
  output        auto_out_1_a_bits_user_amba_prot_writealloc,
  output        auto_out_1_a_bits_user_amba_prot_privileged,
  output        auto_out_1_a_bits_user_amba_prot_secure,
  output        auto_out_1_a_bits_user_amba_prot_fetch,
  output [7:0]  auto_out_1_a_bits_mask,
  output [63:0] auto_out_1_a_bits_data,
  output        auto_out_1_d_ready,
  input         auto_out_1_d_valid,
  input  [2:0]  auto_out_1_d_bits_opcode,
  input  [2:0]  auto_out_1_d_bits_size,
  input  [1:0]  auto_out_1_d_bits_source,
  input         auto_out_1_d_bits_denied,
  input  [63:0] auto_out_1_d_bits_data,
  input         auto_out_1_d_bits_corrupt,
  input         auto_out_0_a_ready,
  output        auto_out_0_a_valid,
  output [2:0]  auto_out_0_a_bits_opcode,
  output [2:0]  auto_out_0_a_bits_size,
  output [1:0]  auto_out_0_a_bits_source,
  output        auto_out_0_d_ready,
  input         auto_out_0_d_valid,
  input  [2:0]  auto_out_0_d_bits_opcode,
  input  [2:0]  auto_out_0_d_bits_size,
  input  [1:0]  auto_out_0_d_bits_source,
  input         auto_out_0_d_bits_corrupt
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
  reg [31:0] _RAND_2;
  reg [31:0] _RAND_3;
`endif // RANDOMIZE_REG_INIT
  reg [3:0] beatsLeft; // @[Arbiter.scala 87:30]
  wire  idle = beatsLeft == 4'h0; // @[Arbiter.scala 88:28]
  wire [1:0] readys_valid = {auto_out_1_d_valid,auto_out_0_d_valid}; // @[Cat.scala 31:58]
  reg [1:0] readys_mask; // @[Arbiter.scala 23:23]
  wire [1:0] _readys_filter_T = ~readys_mask; // @[Arbiter.scala 24:30]
  wire [1:0] _readys_filter_T_1 = readys_valid & _readys_filter_T; // @[Arbiter.scala 24:28]
  wire [3:0] readys_filter = {_readys_filter_T_1,auto_out_1_d_valid,auto_out_0_d_valid}; // @[Cat.scala 31:58]
  wire [3:0] _GEN_1 = {{1'd0}, readys_filter[3:1]}; // @[package.scala 253:43]
  wire [3:0] _readys_unready_T_1 = readys_filter | _GEN_1; // @[package.scala 253:43]
  wire [3:0] _readys_unready_T_4 = {readys_mask, 2'h0}; // @[Arbiter.scala 25:66]
  wire [3:0] _GEN_2 = {{1'd0}, _readys_unready_T_1[3:1]}; // @[Arbiter.scala 25:58]
  wire [3:0] readys_unready = _GEN_2 | _readys_unready_T_4; // @[Arbiter.scala 25:58]
  wire [1:0] _readys_readys_T_2 = readys_unready[3:2] & readys_unready[1:0]; // @[Arbiter.scala 26:39]
  wire [1:0] readys_readys = ~_readys_readys_T_2; // @[Arbiter.scala 26:18]
  wire  readys_0 = readys_readys[0]; // @[Arbiter.scala 95:86]
  wire  earlyWinner_0 = readys_0 & auto_out_0_d_valid; // @[Arbiter.scala 97:79]
  reg  state_0; // @[Arbiter.scala 116:26]
  wire  muxStateEarly_0 = idle ? earlyWinner_0 : state_0; // @[Arbiter.scala 117:30]
  wire [1:0] _T_36 = muxStateEarly_0 ? auto_out_0_d_bits_source : 2'h0; // @[Mux.scala 27:73]
  wire  readys_1 = readys_readys[1]; // @[Arbiter.scala 95:86]
  wire  earlyWinner_1 = readys_1 & auto_out_1_d_valid; // @[Arbiter.scala 97:79]
  reg  state_1; // @[Arbiter.scala 116:26]
  wire  muxStateEarly_1 = idle ? earlyWinner_1 : state_1; // @[Arbiter.scala 117:30]
  wire [1:0] _T_37 = muxStateEarly_1 ? auto_out_1_d_bits_source : 2'h0; // @[Mux.scala 27:73]
  wire [38:0] _requestAIO_T_1 = {1'b0,$signed(auto_in_a_bits_address)}; // @[Parameters.scala 137:49]
  wire [38:0] _requestAIO_T_3 = $signed(_requestAIO_T_1) & 39'sh3000000000; // @[Parameters.scala 137:52]
  wire  _requestAIO_T_4 = $signed(_requestAIO_T_3) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _requestAIO_T_5 = auto_in_a_bits_address ^ 38'h1000000000; // @[Parameters.scala 137:31]
  wire [38:0] _requestAIO_T_6 = {1'b0,$signed(_requestAIO_T_5)}; // @[Parameters.scala 137:49]
  wire [38:0] _requestAIO_T_8 = $signed(_requestAIO_T_6) & 39'sh3800000000; // @[Parameters.scala 137:52]
  wire  _requestAIO_T_9 = $signed(_requestAIO_T_8) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _requestAIO_T_10 = auto_in_a_bits_address ^ 38'h1800000000; // @[Parameters.scala 137:31]
  wire [38:0] _requestAIO_T_11 = {1'b0,$signed(_requestAIO_T_10)}; // @[Parameters.scala 137:49]
  wire [38:0] _requestAIO_T_13 = $signed(_requestAIO_T_11) & 39'sh3c00000000; // @[Parameters.scala 137:52]
  wire  _requestAIO_T_14 = $signed(_requestAIO_T_13) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _requestAIO_T_15 = auto_in_a_bits_address ^ 38'h1c00000000; // @[Parameters.scala 137:31]
  wire [38:0] _requestAIO_T_16 = {1'b0,$signed(_requestAIO_T_15)}; // @[Parameters.scala 137:49]
  wire [38:0] _requestAIO_T_18 = $signed(_requestAIO_T_16) & 39'sh3e00000000; // @[Parameters.scala 137:52]
  wire  _requestAIO_T_19 = $signed(_requestAIO_T_18) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _requestAIO_T_20 = auto_in_a_bits_address ^ 38'h1e00000000; // @[Parameters.scala 137:31]
  wire [38:0] _requestAIO_T_21 = {1'b0,$signed(_requestAIO_T_20)}; // @[Parameters.scala 137:49]
  wire [38:0] _requestAIO_T_23 = $signed(_requestAIO_T_21) & 39'sh3f00000000; // @[Parameters.scala 137:52]
  wire  _requestAIO_T_24 = $signed(_requestAIO_T_23) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _requestAIO_T_25 = auto_in_a_bits_address ^ 38'h2000000000; // @[Parameters.scala 137:31]
  wire [38:0] _requestAIO_T_26 = {1'b0,$signed(_requestAIO_T_25)}; // @[Parameters.scala 137:49]
  wire [38:0] _requestAIO_T_28 = $signed(_requestAIO_T_26) & 39'sh2000000000; // @[Parameters.scala 137:52]
  wire  _requestAIO_T_29 = $signed(_requestAIO_T_28) == 39'sh0; // @[Parameters.scala 137:67]
  wire  requestAIO_0_0 = _requestAIO_T_4 | _requestAIO_T_9 | _requestAIO_T_14 | _requestAIO_T_19 | _requestAIO_T_24 |
    _requestAIO_T_29; // @[Xbar.scala 363:92]
  wire [37:0] _requestAIO_T_35 = auto_in_a_bits_address ^ 38'h1f00000000; // @[Parameters.scala 137:31]
  wire [38:0] _requestAIO_T_36 = {1'b0,$signed(_requestAIO_T_35)}; // @[Parameters.scala 137:49]
  wire [38:0] _requestAIO_T_38 = $signed(_requestAIO_T_36) & 39'sh3f00000000; // @[Parameters.scala 137:52]
  wire  requestAIO_0_1 = $signed(_requestAIO_T_38) == 39'sh0; // @[Parameters.scala 137:67]
  wire [13:0] _beatsDO_decode_T_1 = 14'h7f << auto_out_0_d_bits_size; // @[package.scala 234:77]
  wire [6:0] _beatsDO_decode_T_3 = ~_beatsDO_decode_T_1[6:0]; // @[package.scala 234:46]
  wire [3:0] beatsDO_decode = _beatsDO_decode_T_3[6:3]; // @[Edges.scala 219:59]
  wire  beatsDO_opdata = auto_out_0_d_bits_opcode[0]; // @[Edges.scala 105:36]
  wire [3:0] beatsDO_0 = beatsDO_opdata ? beatsDO_decode : 4'h0; // @[Edges.scala 220:14]
  wire [13:0] _beatsDO_decode_T_5 = 14'h7f << auto_out_1_d_bits_size; // @[package.scala 234:77]
  wire [6:0] _beatsDO_decode_T_7 = ~_beatsDO_decode_T_5[6:0]; // @[package.scala 234:46]
  wire [3:0] beatsDO_decode_1 = _beatsDO_decode_T_7[6:3]; // @[Edges.scala 219:59]
  wire  beatsDO_opdata_1 = auto_out_1_d_bits_opcode[0]; // @[Edges.scala 105:36]
  wire [3:0] beatsDO_1 = beatsDO_opdata_1 ? beatsDO_decode_1 : 4'h0; // @[Edges.scala 220:14]
  wire  latch = idle & auto_in_d_ready; // @[Arbiter.scala 89:24]
  wire [1:0] _readys_mask_T = readys_readys & readys_valid; // @[Arbiter.scala 28:29]
  wire [2:0] _readys_mask_T_1 = {_readys_mask_T, 1'h0}; // @[package.scala 244:48]
  wire [1:0] _readys_mask_T_3 = _readys_mask_T | _readys_mask_T_1[1:0]; // @[package.scala 244:43]
  wire  _T_10 = auto_out_0_d_valid | auto_out_1_d_valid; // @[Arbiter.scala 107:36]
  wire [3:0] maskedBeats_0 = earlyWinner_0 ? beatsDO_0 : 4'h0; // @[Arbiter.scala 111:73]
  wire [3:0] maskedBeats_1 = earlyWinner_1 ? beatsDO_1 : 4'h0; // @[Arbiter.scala 111:73]
  wire [3:0] initBeats = maskedBeats_0 | maskedBeats_1; // @[Arbiter.scala 112:44]
  wire  _sink_ACancel_earlyValid_T_3 = state_0 & auto_out_0_d_valid | state_1 & auto_out_1_d_valid; // @[Mux.scala 27:73]
  wire  sink_ACancel_5_earlyValid = idle ? _T_10 : _sink_ACancel_earlyValid_T_3; // @[Arbiter.scala 125:29]
  wire  _beatsLeft_T_2 = auto_in_d_ready & sink_ACancel_5_earlyValid; // @[ReadyValidCancel.scala 49:33]
  wire [3:0] _GEN_3 = {{3'd0}, _beatsLeft_T_2}; // @[Arbiter.scala 113:52]
  wire [3:0] _beatsLeft_T_4 = beatsLeft - _GEN_3; // @[Arbiter.scala 113:52]
  wire  allowed_0 = idle ? readys_0 : state_0; // @[Arbiter.scala 121:24]
  wire  allowed_1 = idle ? readys_1 : state_1; // @[Arbiter.scala 121:24]
  wire [2:0] _T_39 = muxStateEarly_0 ? auto_out_0_d_bits_size : 3'h0; // @[Mux.scala 27:73]
  wire [2:0] _T_40 = muxStateEarly_1 ? auto_out_1_d_bits_size : 3'h0; // @[Mux.scala 27:73]
  wire [2:0] _T_45 = muxStateEarly_0 ? auto_out_0_d_bits_opcode : 3'h0; // @[Mux.scala 27:73]
  wire [2:0] _T_46 = muxStateEarly_1 ? auto_out_1_d_bits_opcode : 3'h0; // @[Mux.scala 27:73]
  assign auto_in_a_ready = requestAIO_0_0 & auto_out_0_a_ready | requestAIO_0_1 & auto_out_1_a_ready; // @[Mux.scala 27:73]
  assign auto_in_d_valid = idle ? _T_10 : _sink_ACancel_earlyValid_T_3; // @[Arbiter.scala 125:29]
  assign auto_in_d_bits_opcode = _T_45 | _T_46; // @[Mux.scala 27:73]
  assign auto_in_d_bits_size = _T_39 | _T_40; // @[Mux.scala 27:73]
  assign auto_in_d_bits_source = _T_36 | _T_37; // @[Mux.scala 27:73]
  assign auto_in_d_bits_denied = muxStateEarly_0 | muxStateEarly_1 & auto_out_1_d_bits_denied; // @[Mux.scala 27:73]
  assign auto_in_d_bits_data = muxStateEarly_1 ? auto_out_1_d_bits_data : 64'h0; // @[Mux.scala 27:73]
  assign auto_in_d_bits_corrupt = muxStateEarly_0 & auto_out_0_d_bits_corrupt | muxStateEarly_1 &
    auto_out_1_d_bits_corrupt; // @[Mux.scala 27:73]
  assign auto_out_1_a_valid = auto_in_a_valid & requestAIO_0_1; // @[Xbar.scala 428:50]
  assign auto_out_1_a_bits_opcode = auto_in_a_bits_opcode; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_size = auto_in_a_bits_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_source = auto_in_a_bits_source; // @[Xbar.scala 237:55]
  assign auto_out_1_a_bits_address = auto_in_a_bits_address[36:0]; // @[Xbar.scala 132:50 BundleMap.scala 247:19]
  assign auto_out_1_a_bits_user_amba_prot_bufferable = auto_in_a_bits_user_amba_prot_bufferable; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_user_amba_prot_modifiable = auto_in_a_bits_user_amba_prot_modifiable; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_user_amba_prot_readalloc = auto_in_a_bits_user_amba_prot_readalloc; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_user_amba_prot_writealloc = auto_in_a_bits_user_amba_prot_writealloc; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_user_amba_prot_privileged = auto_in_a_bits_user_amba_prot_privileged; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_user_amba_prot_secure = auto_in_a_bits_user_amba_prot_secure; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_user_amba_prot_fetch = auto_in_a_bits_user_amba_prot_fetch; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_mask = auto_in_a_bits_mask; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_a_bits_data = auto_in_a_bits_data; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_1_d_ready = auto_in_d_ready & allowed_1; // @[Arbiter.scala 123:31]
  assign auto_out_0_a_valid = auto_in_a_valid & requestAIO_0_0; // @[Xbar.scala 428:50]
  assign auto_out_0_a_bits_opcode = auto_in_a_bits_opcode; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_a_bits_size = auto_in_a_bits_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_0_a_bits_source = auto_in_a_bits_source; // @[Xbar.scala 237:55]
  assign auto_out_0_d_ready = auto_in_d_ready & allowed_0; // @[Arbiter.scala 123:31]
  always @(posedge clock) begin
    if (reset) begin // @[Arbiter.scala 87:30]
      beatsLeft <= 4'h0; // @[Arbiter.scala 87:30]
    end else if (latch) begin // @[Arbiter.scala 113:23]
      beatsLeft <= initBeats;
    end else begin
      beatsLeft <= _beatsLeft_T_4;
    end
    if (reset) begin // @[Arbiter.scala 23:23]
      readys_mask <= 2'h3; // @[Arbiter.scala 23:23]
    end else if (latch & |readys_valid) begin // @[Arbiter.scala 27:32]
      readys_mask <= _readys_mask_T_3; // @[Arbiter.scala 28:12]
    end
    if (reset) begin // @[Arbiter.scala 116:26]
      state_0 <= 1'h0; // @[Arbiter.scala 116:26]
    end else if (idle) begin // @[Arbiter.scala 117:30]
      state_0 <= earlyWinner_0;
    end
    if (reset) begin // @[Arbiter.scala 116:26]
      state_1 <= 1'h0; // @[Arbiter.scala 116:26]
    end else if (idle) begin // @[Arbiter.scala 117:30]
      state_1 <= earlyWinner_1;
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
  beatsLeft = _RAND_0[3:0];
  _RAND_1 = {1{`RANDOM}};
  readys_mask = _RAND_1[1:0];
  _RAND_2 = {1{`RANDOM}};
  state_0 = _RAND_2[0:0];
  _RAND_3 = {1{`RANDOM}};
  state_1 = _RAND_3[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

