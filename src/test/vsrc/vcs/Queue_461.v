module Queue_461(
  input        clock,
  input        reset,
  output       io_enq_ready,
  input        io_enq_valid,
  input  [4:0] io_enq_bits_id,
  input  [1:0] io_enq_bits_resp,
  input        io_deq_ready,
  output       io_deq_valid,
  output [4:0] io_deq_bits_id,
  output [1:0] io_deq_bits_resp
);
`ifdef RANDOMIZE_MEM_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
`endif // RANDOMIZE_MEM_INIT
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_2;
`endif // RANDOMIZE_REG_INIT
  reg [4:0] ram_id [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_id_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_id_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [4:0] ram_id_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [4:0] ram_id_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_id_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_id_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_id_MPORT_en; // @[Decoupled.scala 259:95]
  reg [1:0] ram_resp [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_resp_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_resp_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [1:0] ram_resp_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [1:0] ram_resp_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_resp_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_resp_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_resp_MPORT_en; // @[Decoupled.scala 259:95]
  reg  maybe_full; // @[Decoupled.scala 262:27]
  wire  empty = ~maybe_full; // @[Decoupled.scala 264:28]
  wire  _do_enq_T = io_enq_ready & io_enq_valid; // @[Decoupled.scala 50:35]
  wire  _do_deq_T = io_deq_ready & io_deq_valid; // @[Decoupled.scala 50:35]
  wire  _GEN_10 = io_deq_ready ? 1'h0 : _do_enq_T; // @[Decoupled.scala 304:{26,35}]
  wire  do_enq = empty ? _GEN_10 : _do_enq_T; // @[Decoupled.scala 301:17]
  wire  do_deq = empty ? 1'h0 : _do_deq_T; // @[Decoupled.scala 301:17 303:14]
  assign ram_id_io_deq_bits_MPORT_en = 1'h1;
  assign ram_id_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_id_io_deq_bits_MPORT_data = ram_id[ram_id_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_id_MPORT_data = io_enq_bits_id;
  assign ram_id_MPORT_addr = 1'h0;
  assign ram_id_MPORT_mask = 1'h1;
  assign ram_id_MPORT_en = empty ? _GEN_10 : _do_enq_T;
  assign ram_resp_io_deq_bits_MPORT_en = 1'h1;
  assign ram_resp_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_resp_io_deq_bits_MPORT_data = ram_resp[ram_resp_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_resp_MPORT_data = io_enq_bits_resp;
  assign ram_resp_MPORT_addr = 1'h0;
  assign ram_resp_MPORT_mask = 1'h1;
  assign ram_resp_MPORT_en = empty ? _GEN_10 : _do_enq_T;
  assign io_enq_ready = ~maybe_full; // @[Decoupled.scala 289:19]
  assign io_deq_valid = io_enq_valid | ~empty; // @[Decoupled.scala 288:16 300:{24,39}]
  assign io_deq_bits_id = empty ? io_enq_bits_id : ram_id_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_resp = empty ? io_enq_bits_resp : ram_resp_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  always @(posedge clock) begin
    if (ram_id_MPORT_en & ram_id_MPORT_mask) begin
      ram_id[ram_id_MPORT_addr] <= ram_id_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_resp_MPORT_en & ram_resp_MPORT_mask) begin
      ram_resp[ram_resp_MPORT_addr] <= ram_resp_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (reset) begin // @[Decoupled.scala 262:27]
      maybe_full <= 1'h0; // @[Decoupled.scala 262:27]
    end else if (do_enq != do_deq) begin // @[Decoupled.scala 279:27]
      if (empty) begin // @[Decoupled.scala 301:17]
        if (io_deq_ready) begin // @[Decoupled.scala 304:26]
          maybe_full <= 1'h0; // @[Decoupled.scala 304:35]
        end else begin
          maybe_full <= _do_enq_T;
        end
      end else begin
        maybe_full <= _do_enq_T;
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
`ifdef RANDOMIZE_MEM_INIT
  _RAND_0 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_id[initvar] = _RAND_0[4:0];
  _RAND_1 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_resp[initvar] = _RAND_1[1:0];
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

