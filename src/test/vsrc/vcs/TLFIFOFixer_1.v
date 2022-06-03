module TLFIFOFixer_1(
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
  input         auto_out_a_ready,
  output        auto_out_a_valid,
  output [2:0]  auto_out_a_bits_opcode,
  output [2:0]  auto_out_a_bits_size,
  output [1:0]  auto_out_a_bits_source,
  output [37:0] auto_out_a_bits_address,
  output        auto_out_a_bits_user_amba_prot_bufferable,
  output        auto_out_a_bits_user_amba_prot_modifiable,
  output        auto_out_a_bits_user_amba_prot_readalloc,
  output        auto_out_a_bits_user_amba_prot_writealloc,
  output        auto_out_a_bits_user_amba_prot_privileged,
  output        auto_out_a_bits_user_amba_prot_secure,
  output        auto_out_a_bits_user_amba_prot_fetch,
  output [7:0]  auto_out_a_bits_mask,
  output [63:0] auto_out_a_bits_data,
  output        auto_out_d_ready,
  input         auto_out_d_valid,
  input  [2:0]  auto_out_d_bits_opcode,
  input  [2:0]  auto_out_d_bits_size,
  input  [1:0]  auto_out_d_bits_source,
  input         auto_out_d_bits_denied,
  input  [63:0] auto_out_d_bits_data,
  input         auto_out_d_bits_corrupt
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
`endif // RANDOMIZE_REG_INIT
  wire [38:0] _a_notFIFO_T_1 = {1'b0,$signed(auto_in_a_bits_address)}; // @[Parameters.scala 137:49]
  wire [38:0] _a_id_T_3 = $signed(_a_notFIFO_T_1) & 39'sh3000000000; // @[Parameters.scala 137:52]
  wire  _a_id_T_4 = $signed(_a_id_T_3) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _a_id_T_5 = auto_in_a_bits_address ^ 38'h1000000000; // @[Parameters.scala 137:31]
  wire [38:0] _a_id_T_6 = {1'b0,$signed(_a_id_T_5)}; // @[Parameters.scala 137:49]
  wire [38:0] _a_id_T_8 = $signed(_a_id_T_6) & 39'sh3800000000; // @[Parameters.scala 137:52]
  wire  _a_id_T_9 = $signed(_a_id_T_8) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _a_id_T_10 = auto_in_a_bits_address ^ 38'h1800000000; // @[Parameters.scala 137:31]
  wire [38:0] _a_id_T_11 = {1'b0,$signed(_a_id_T_10)}; // @[Parameters.scala 137:49]
  wire [38:0] _a_id_T_13 = $signed(_a_id_T_11) & 39'sh3c00000000; // @[Parameters.scala 137:52]
  wire  _a_id_T_14 = $signed(_a_id_T_13) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _a_id_T_15 = auto_in_a_bits_address ^ 38'h1c00000000; // @[Parameters.scala 137:31]
  wire [38:0] _a_id_T_16 = {1'b0,$signed(_a_id_T_15)}; // @[Parameters.scala 137:49]
  wire [38:0] _a_id_T_18 = $signed(_a_id_T_16) & 39'sh3e00000000; // @[Parameters.scala 137:52]
  wire  _a_id_T_19 = $signed(_a_id_T_18) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _a_id_T_20 = auto_in_a_bits_address ^ 38'h1e00000000; // @[Parameters.scala 137:31]
  wire [38:0] _a_id_T_21 = {1'b0,$signed(_a_id_T_20)}; // @[Parameters.scala 137:49]
  wire [38:0] _a_id_T_23 = $signed(_a_id_T_21) & 39'sh3f00000000; // @[Parameters.scala 137:52]
  wire  _a_id_T_24 = $signed(_a_id_T_23) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _a_id_T_25 = auto_in_a_bits_address ^ 38'h2000000000; // @[Parameters.scala 137:31]
  wire [38:0] _a_id_T_26 = {1'b0,$signed(_a_id_T_25)}; // @[Parameters.scala 137:49]
  wire [38:0] _a_id_T_28 = $signed(_a_id_T_26) & 39'sh2000000000; // @[Parameters.scala 137:52]
  wire  _a_id_T_29 = $signed(_a_id_T_28) == 39'sh0; // @[Parameters.scala 137:67]
  wire  _a_id_T_34 = _a_id_T_4 | _a_id_T_9 | _a_id_T_14 | _a_id_T_19 | _a_id_T_24 | _a_id_T_29; // @[Parameters.scala 615:89]
  wire [37:0] _a_id_T_35 = auto_in_a_bits_address ^ 38'h1f00000000; // @[Parameters.scala 137:31]
  wire [38:0] _a_id_T_36 = {1'b0,$signed(_a_id_T_35)}; // @[Parameters.scala 137:49]
  wire [38:0] _a_id_T_38 = $signed(_a_id_T_36) & 39'sh3f00000000; // @[Parameters.scala 137:52]
  wire  _a_id_T_39 = $signed(_a_id_T_38) == 39'sh0; // @[Parameters.scala 137:67]
  wire [1:0] _a_id_T_41 = _a_id_T_39 ? 2'h2 : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] _GEN_22 = {{1'd0}, _a_id_T_34}; // @[Mux.scala 27:73]
  wire [1:0] a_id = _GEN_22 | _a_id_T_41; // @[Mux.scala 27:73]
  wire  a_noDomain = a_id == 2'h0; // @[FIFOFixer.scala 55:29]
  wire  stalls_a_sel = ~auto_in_a_bits_source[1]; // @[Parameters.scala 54:32]
  reg [3:0] a_first_counter; // @[Edges.scala 228:27]
  wire  a_first = a_first_counter == 4'h0; // @[Edges.scala 230:25]
  reg  flight_0; // @[FIFOFixer.scala 71:27]
  reg  flight_1; // @[FIFOFixer.scala 71:27]
  reg [1:0] stalls_id; // @[Reg.scala 16:16]
  wire  stalls_0 = stalls_a_sel & a_first & (flight_0 | flight_1) & (a_noDomain | stalls_id != a_id); // @[FIFOFixer.scala 80:50]
  reg  flight_2; // @[FIFOFixer.scala 71:27]
  reg  flight_3; // @[FIFOFixer.scala 71:27]
  reg [1:0] stalls_id_1; // @[Reg.scala 16:16]
  wire  stalls_1 = auto_in_a_bits_source[1] & a_first & (flight_2 | flight_3) & (a_noDomain | stalls_id_1 != a_id); // @[FIFOFixer.scala 80:50]
  wire  stall = stalls_0 | stalls_1; // @[FIFOFixer.scala 83:49]
  wire  _bundleIn_0_a_ready_T = ~stall; // @[FIFOFixer.scala 88:50]
  wire  bundleIn_0_a_ready = auto_out_a_ready & ~stall; // @[FIFOFixer.scala 88:33]
  wire  _a_first_T = bundleIn_0_a_ready & auto_in_a_valid; // @[Decoupled.scala 50:35]
  wire [13:0] _a_first_beats1_decode_T_1 = 14'h7f << auto_in_a_bits_size; // @[package.scala 234:77]
  wire [6:0] _a_first_beats1_decode_T_3 = ~_a_first_beats1_decode_T_1[6:0]; // @[package.scala 234:46]
  wire [3:0] a_first_beats1_decode = _a_first_beats1_decode_T_3[6:3]; // @[Edges.scala 219:59]
  wire  a_first_beats1_opdata = ~auto_in_a_bits_opcode[2]; // @[Edges.scala 91:28]
  wire [3:0] a_first_counter1 = a_first_counter - 4'h1; // @[Edges.scala 229:28]
  wire  _d_first_T = auto_in_d_ready & auto_out_d_valid; // @[Decoupled.scala 50:35]
  wire [13:0] _d_first_beats1_decode_T_1 = 14'h7f << auto_out_d_bits_size; // @[package.scala 234:77]
  wire [6:0] _d_first_beats1_decode_T_3 = ~_d_first_beats1_decode_T_1[6:0]; // @[package.scala 234:46]
  wire [3:0] d_first_beats1_decode = _d_first_beats1_decode_T_3[6:3]; // @[Edges.scala 219:59]
  wire  d_first_beats1_opdata = auto_out_d_bits_opcode[0]; // @[Edges.scala 105:36]
  reg [3:0] d_first_counter; // @[Edges.scala 228:27]
  wire [3:0] d_first_counter1 = d_first_counter - 4'h1; // @[Edges.scala 229:28]
  wire  d_first_first = d_first_counter == 4'h0; // @[Edges.scala 230:25]
  wire  d_first = d_first_first & auto_out_d_bits_opcode != 3'h6; // @[FIFOFixer.scala 67:42]
  wire  _GEN_6 = a_first & _a_first_T ? 2'h0 == auto_in_a_bits_source | flight_0 : flight_0; // @[FIFOFixer.scala 71:27 72:37]
  wire  _GEN_7 = a_first & _a_first_T ? 2'h1 == auto_in_a_bits_source | flight_1 : flight_1; // @[FIFOFixer.scala 71:27 72:37]
  wire  _GEN_8 = a_first & _a_first_T ? 2'h2 == auto_in_a_bits_source | flight_2 : flight_2; // @[FIFOFixer.scala 71:27 72:37]
  wire  _GEN_9 = a_first & _a_first_T ? 2'h3 == auto_in_a_bits_source | flight_3 : flight_3; // @[FIFOFixer.scala 71:27 72:37]
  wire  _stalls_id_T_1 = _a_first_T & stalls_a_sel; // @[FIFOFixer.scala 77:49]
  wire  _stalls_id_T_5 = _a_first_T & auto_in_a_bits_source[1]; // @[FIFOFixer.scala 77:49]
  assign auto_in_a_ready = auto_out_a_ready & ~stall; // @[FIFOFixer.scala 88:33]
  assign auto_in_d_valid = auto_out_d_valid; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_opcode = auto_out_d_bits_opcode; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_size = auto_out_d_bits_size; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_source = auto_out_d_bits_source; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_denied = auto_out_d_bits_denied; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_data = auto_out_d_bits_data; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_corrupt = auto_out_d_bits_corrupt; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_out_a_valid = auto_in_a_valid & _bundleIn_0_a_ready_T; // @[FIFOFixer.scala 87:33]
  assign auto_out_a_bits_opcode = auto_in_a_bits_opcode; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_size = auto_in_a_bits_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_source = auto_in_a_bits_source; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_address = auto_in_a_bits_address; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_user_amba_prot_bufferable = auto_in_a_bits_user_amba_prot_bufferable; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_user_amba_prot_modifiable = auto_in_a_bits_user_amba_prot_modifiable; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_user_amba_prot_readalloc = auto_in_a_bits_user_amba_prot_readalloc; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_user_amba_prot_writealloc = auto_in_a_bits_user_amba_prot_writealloc; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_user_amba_prot_privileged = auto_in_a_bits_user_amba_prot_privileged; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_user_amba_prot_secure = auto_in_a_bits_user_amba_prot_secure; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_user_amba_prot_fetch = auto_in_a_bits_user_amba_prot_fetch; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_mask = auto_in_a_bits_mask; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_a_bits_data = auto_in_a_bits_data; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign auto_out_d_ready = auto_in_d_ready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  always @(posedge clock) begin
    if (reset) begin // @[Edges.scala 228:27]
      a_first_counter <= 4'h0; // @[Edges.scala 228:27]
    end else if (_a_first_T) begin // @[Edges.scala 234:17]
      if (a_first) begin // @[Edges.scala 235:21]
        if (a_first_beats1_opdata) begin // @[Edges.scala 220:14]
          a_first_counter <= a_first_beats1_decode;
        end else begin
          a_first_counter <= 4'h0;
        end
      end else begin
        a_first_counter <= a_first_counter1;
      end
    end
    if (reset) begin // @[FIFOFixer.scala 71:27]
      flight_0 <= 1'h0; // @[FIFOFixer.scala 71:27]
    end else if (d_first & _d_first_T) begin // @[FIFOFixer.scala 73:37]
      if (2'h0 == auto_out_d_bits_source) begin // @[FIFOFixer.scala 73:64]
        flight_0 <= 1'h0; // @[FIFOFixer.scala 73:64]
      end else begin
        flight_0 <= _GEN_6;
      end
    end else begin
      flight_0 <= _GEN_6;
    end
    if (reset) begin // @[FIFOFixer.scala 71:27]
      flight_1 <= 1'h0; // @[FIFOFixer.scala 71:27]
    end else if (d_first & _d_first_T) begin // @[FIFOFixer.scala 73:37]
      if (2'h1 == auto_out_d_bits_source) begin // @[FIFOFixer.scala 73:64]
        flight_1 <= 1'h0; // @[FIFOFixer.scala 73:64]
      end else begin
        flight_1 <= _GEN_7;
      end
    end else begin
      flight_1 <= _GEN_7;
    end
    if (_stalls_id_T_1) begin // @[Reg.scala 17:18]
      stalls_id <= a_id; // @[Reg.scala 17:22]
    end
    if (reset) begin // @[FIFOFixer.scala 71:27]
      flight_2 <= 1'h0; // @[FIFOFixer.scala 71:27]
    end else if (d_first & _d_first_T) begin // @[FIFOFixer.scala 73:37]
      if (2'h2 == auto_out_d_bits_source) begin // @[FIFOFixer.scala 73:64]
        flight_2 <= 1'h0; // @[FIFOFixer.scala 73:64]
      end else begin
        flight_2 <= _GEN_8;
      end
    end else begin
      flight_2 <= _GEN_8;
    end
    if (reset) begin // @[FIFOFixer.scala 71:27]
      flight_3 <= 1'h0; // @[FIFOFixer.scala 71:27]
    end else if (d_first & _d_first_T) begin // @[FIFOFixer.scala 73:37]
      if (2'h3 == auto_out_d_bits_source) begin // @[FIFOFixer.scala 73:64]
        flight_3 <= 1'h0; // @[FIFOFixer.scala 73:64]
      end else begin
        flight_3 <= _GEN_9;
      end
    end else begin
      flight_3 <= _GEN_9;
    end
    if (_stalls_id_T_5) begin // @[Reg.scala 17:18]
      stalls_id_1 <= a_id; // @[Reg.scala 17:22]
    end
    if (reset) begin // @[Edges.scala 228:27]
      d_first_counter <= 4'h0; // @[Edges.scala 228:27]
    end else if (_d_first_T) begin // @[Edges.scala 234:17]
      if (d_first_first) begin // @[Edges.scala 235:21]
        if (d_first_beats1_opdata) begin // @[Edges.scala 220:14]
          d_first_counter <= d_first_beats1_decode;
        end else begin
          d_first_counter <= 4'h0;
        end
      end else begin
        d_first_counter <= d_first_counter1;
      end
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
  a_first_counter = _RAND_0[3:0];
  _RAND_1 = {1{`RANDOM}};
  flight_0 = _RAND_1[0:0];
  _RAND_2 = {1{`RANDOM}};
  flight_1 = _RAND_2[0:0];
  _RAND_3 = {1{`RANDOM}};
  stalls_id = _RAND_3[1:0];
  _RAND_4 = {1{`RANDOM}};
  flight_2 = _RAND_4[0:0];
  _RAND_5 = {1{`RANDOM}};
  flight_3 = _RAND_5[0:0];
  _RAND_6 = {1{`RANDOM}};
  stalls_id_1 = _RAND_6[1:0];
  _RAND_7 = {1{`RANDOM}};
  d_first_counter = _RAND_7[3:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

