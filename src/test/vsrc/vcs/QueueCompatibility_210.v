module QueueCompatibility_210(
  input        clock,
  input        reset,
  output       io_enq_ready,
  input        io_enq_valid,
  input  [3:0] io_enq_bits_tl_state_size,
  input  [1:0] io_enq_bits_tl_state_source,
  input        io_deq_ready,
  output       io_deq_valid,
  output [3:0] io_deq_bits_tl_state_size,
  output [1:0] io_deq_bits_tl_state_source
);
`ifdef RANDOMIZE_MEM_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
`endif // RANDOMIZE_MEM_INIT
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_2;
`endif // RANDOMIZE_REG_INIT
  reg [3:0] ram_tl_state_size [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_size_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_size_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [3:0] ram_tl_state_size_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [3:0] ram_tl_state_size_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_size_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_size_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_size_MPORT_en; // @[Decoupled.scala 259:95]
  reg [1:0] ram_tl_state_source [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_source_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_source_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [1:0] ram_tl_state_source_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [1:0] ram_tl_state_source_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_source_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_source_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_tl_state_source_MPORT_en; // @[Decoupled.scala 259:95]
  reg  maybe_full; // @[Decoupled.scala 262:27]
  wire  empty = ~maybe_full; // @[Decoupled.scala 264:28]
  wire  do_enq = io_enq_ready & io_enq_valid; // @[Decoupled.scala 50:35]
  wire  do_deq = io_deq_ready & io_deq_valid; // @[Decoupled.scala 50:35]
  assign ram_tl_state_size_io_deq_bits_MPORT_en = 1'h1;
  assign ram_tl_state_size_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_tl_state_size_io_deq_bits_MPORT_data = ram_tl_state_size[ram_tl_state_size_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_tl_state_size_MPORT_data = io_enq_bits_tl_state_size;
  assign ram_tl_state_size_MPORT_addr = 1'h0;
  assign ram_tl_state_size_MPORT_mask = 1'h1;
  assign ram_tl_state_size_MPORT_en = io_enq_ready & io_enq_valid;
  assign ram_tl_state_source_io_deq_bits_MPORT_en = 1'h1;
  assign ram_tl_state_source_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_tl_state_source_io_deq_bits_MPORT_data = ram_tl_state_source[ram_tl_state_source_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_tl_state_source_MPORT_data = io_enq_bits_tl_state_source;
  assign ram_tl_state_source_MPORT_addr = 1'h0;
  assign ram_tl_state_source_MPORT_mask = 1'h1;
  assign ram_tl_state_source_MPORT_en = io_enq_ready & io_enq_valid;
  assign io_enq_ready = ~maybe_full; // @[Decoupled.scala 289:19]
  assign io_deq_valid = ~empty; // @[Decoupled.scala 288:19]
  assign io_deq_bits_tl_state_size = ram_tl_state_size_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17]
  assign io_deq_bits_tl_state_source = ram_tl_state_source_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17]
  always @(posedge clock) begin
    if (ram_tl_state_size_MPORT_en & ram_tl_state_size_MPORT_mask) begin
      ram_tl_state_size[ram_tl_state_size_MPORT_addr] <= ram_tl_state_size_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_tl_state_source_MPORT_en & ram_tl_state_source_MPORT_mask) begin
      ram_tl_state_source[ram_tl_state_source_MPORT_addr] <= ram_tl_state_source_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (reset) begin // @[Decoupled.scala 262:27]
      maybe_full <= 1'h0; // @[Decoupled.scala 262:27]
    end else if (do_enq != do_deq) begin // @[Decoupled.scala 279:27]
      maybe_full <= do_enq; // @[Decoupled.scala 280:16]
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
`ifdef RANDOMIZE_MEM_INIT
  _RAND_0 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_tl_state_size[initvar] = _RAND_0[3:0];
  _RAND_1 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_tl_state_source[initvar] = _RAND_1[1:0];
`endif // RANDOMIZE_MEM_INIT
`ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  maybe_full = _RAND_2[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

