module TLError_2(
  input        clock,
  input        reset,
  output       auto_in_a_ready,
  input        auto_in_a_valid,
  input  [2:0] auto_in_a_bits_opcode,
  input  [2:0] auto_in_a_bits_size,
  input  [1:0] auto_in_a_bits_source,
  input        auto_in_d_ready,
  output       auto_in_d_valid,
  output [2:0] auto_in_d_bits_opcode,
  output [2:0] auto_in_d_bits_size,
  output [1:0] auto_in_d_bits_source,
  output       auto_in_d_bits_corrupt
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
`endif // RANDOMIZE_REG_INIT
  wire  a_clock; // @[Decoupled.scala 361:21]
  wire  a_reset; // @[Decoupled.scala 361:21]
  wire  a_io_enq_ready; // @[Decoupled.scala 361:21]
  wire  a_io_enq_valid; // @[Decoupled.scala 361:21]
  wire [2:0] a_io_enq_bits_opcode; // @[Decoupled.scala 361:21]
  wire [2:0] a_io_enq_bits_size; // @[Decoupled.scala 361:21]
  wire [1:0] a_io_enq_bits_source; // @[Decoupled.scala 361:21]
  wire  a_io_deq_ready; // @[Decoupled.scala 361:21]
  wire  a_io_deq_valid; // @[Decoupled.scala 361:21]
  wire [2:0] a_io_deq_bits_opcode; // @[Decoupled.scala 361:21]
  wire [2:0] a_io_deq_bits_size; // @[Decoupled.scala 361:21]
  wire [1:0] a_io_deq_bits_source; // @[Decoupled.scala 361:21]
  wire  _a_last_T = a_io_deq_ready & a_io_deq_valid; // @[Decoupled.scala 50:35]
  wire [13:0] _a_last_beats1_decode_T_1 = 14'h7f << a_io_deq_bits_size; // @[package.scala 234:77]
  wire [6:0] _a_last_beats1_decode_T_3 = ~_a_last_beats1_decode_T_1[6:0]; // @[package.scala 234:46]
  wire [3:0] a_last_beats1_decode = _a_last_beats1_decode_T_3[6:3]; // @[Edges.scala 219:59]
  wire  a_last_beats1_opdata = ~a_io_deq_bits_opcode[2]; // @[Edges.scala 91:28]
  wire [3:0] a_last_beats1 = a_last_beats1_opdata ? a_last_beats1_decode : 4'h0; // @[Edges.scala 220:14]
  reg [3:0] a_last_counter; // @[Edges.scala 228:27]
  wire [3:0] a_last_counter1 = a_last_counter - 4'h1; // @[Edges.scala 229:28]
  wire  a_last_first = a_last_counter == 4'h0; // @[Edges.scala 230:25]
  wire  a_last = a_last_counter == 4'h1 | a_last_beats1 == 4'h0; // @[Edges.scala 231:37]
  wire  da_valid = a_io_deq_valid & a_last; // @[Error.scala 51:25]
  wire  _T = auto_in_d_ready & da_valid; // @[Decoupled.scala 50:35]
  wire [2:0] da_bits_size = a_io_deq_bits_size; // @[Error.scala 43:18 55:21]
  wire [13:0] _beats1_decode_T_1 = 14'h7f << da_bits_size; // @[package.scala 234:77]
  wire [6:0] _beats1_decode_T_3 = ~_beats1_decode_T_1[6:0]; // @[package.scala 234:46]
  wire [3:0] beats1_decode = _beats1_decode_T_3[6:3]; // @[Edges.scala 219:59]
  wire [2:0] _GEN_4 = 3'h2 == a_io_deq_bits_opcode ? 3'h1 : 3'h0; // @[Error.scala 53:{21,21}]
  wire [2:0] _GEN_5 = 3'h3 == a_io_deq_bits_opcode ? 3'h1 : _GEN_4; // @[Error.scala 53:{21,21}]
  wire [2:0] _GEN_6 = 3'h4 == a_io_deq_bits_opcode ? 3'h1 : _GEN_5; // @[Error.scala 53:{21,21}]
  wire [2:0] _GEN_7 = 3'h5 == a_io_deq_bits_opcode ? 3'h2 : _GEN_6; // @[Error.scala 53:{21,21}]
  wire [2:0] _GEN_8 = 3'h6 == a_io_deq_bits_opcode ? 3'h4 : _GEN_7; // @[Error.scala 53:{21,21}]
  wire [2:0] da_bits_opcode = 3'h7 == a_io_deq_bits_opcode ? 3'h4 : _GEN_8; // @[Error.scala 53:{21,21}]
  wire  beats1_opdata = da_bits_opcode[0]; // @[Edges.scala 105:36]
  wire [3:0] beats1 = beats1_opdata ? beats1_decode : 4'h0; // @[Edges.scala 220:14]
  reg [3:0] counter; // @[Edges.scala 228:27]
  wire [3:0] counter1 = counter - 4'h1; // @[Edges.scala 229:28]
  wire  da_first = counter == 4'h0; // @[Edges.scala 230:25]
  wire  da_last = counter == 4'h1 | beats1 == 4'h0; // @[Edges.scala 231:37]
  Queue_459 a ( // @[Decoupled.scala 361:21]
    .clock(a_clock),
    .reset(a_reset),
    .io_enq_ready(a_io_enq_ready),
    .io_enq_valid(a_io_enq_valid),
    .io_enq_bits_opcode(a_io_enq_bits_opcode),
    .io_enq_bits_size(a_io_enq_bits_size),
    .io_enq_bits_source(a_io_enq_bits_source),
    .io_deq_ready(a_io_deq_ready),
    .io_deq_valid(a_io_deq_valid),
    .io_deq_bits_opcode(a_io_deq_bits_opcode),
    .io_deq_bits_size(a_io_deq_bits_size),
    .io_deq_bits_source(a_io_deq_bits_source)
  );
  assign auto_in_a_ready = a_io_enq_ready; // @[Nodes.scala 1210:84 Decoupled.scala 365:17]
  assign auto_in_d_valid = a_io_deq_valid & a_last; // @[Error.scala 51:25]
  assign auto_in_d_bits_opcode = 3'h7 == a_io_deq_bits_opcode ? 3'h4 : _GEN_8; // @[Error.scala 53:{21,21}]
  assign auto_in_d_bits_size = a_io_deq_bits_size; // @[Error.scala 43:18 55:21]
  assign auto_in_d_bits_source = a_io_deq_bits_source; // @[Error.scala 43:18 56:21]
  assign auto_in_d_bits_corrupt = da_bits_opcode[0]; // @[Edges.scala 105:36]
  assign a_clock = clock;
  assign a_reset = reset;
  assign a_io_enq_valid = auto_in_a_valid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign a_io_enq_bits_opcode = auto_in_a_bits_opcode; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign a_io_enq_bits_size = auto_in_a_bits_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign a_io_enq_bits_source = auto_in_a_bits_source; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign a_io_deq_ready = auto_in_d_ready & da_last | ~a_last; // @[Error.scala 50:46]
  always @(posedge clock) begin
    if (reset) begin // @[Edges.scala 228:27]
      a_last_counter <= 4'h0; // @[Edges.scala 228:27]
    end else if (_a_last_T) begin // @[Edges.scala 234:17]
      if (a_last_first) begin // @[Edges.scala 235:21]
        if (a_last_beats1_opdata) begin // @[Edges.scala 220:14]
          a_last_counter <= a_last_beats1_decode;
        end else begin
          a_last_counter <= 4'h0;
        end
      end else begin
        a_last_counter <= a_last_counter1;
      end
    end
    if (reset) begin // @[Edges.scala 228:27]
      counter <= 4'h0; // @[Edges.scala 228:27]
    end else if (_T) begin // @[Edges.scala 234:17]
      if (da_first) begin // @[Edges.scala 235:21]
        if (beats1_opdata) begin // @[Edges.scala 220:14]
          counter <= beats1_decode;
        end else begin
          counter <= 4'h0;
        end
      end else begin
        counter <= counter1;
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
  a_last_counter = _RAND_0[3:0];
  _RAND_1 = {1{`RANDOM}};
  counter = _RAND_1[3:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

