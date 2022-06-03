module Queue_463(
  input         clock,
  input         reset,
  output        io_enq_ready,
  input         io_enq_valid,
  input         io_enq_bits_id,
  input  [36:0] io_enq_bits_addr,
  input  [7:0]  io_enq_bits_len,
  input  [2:0]  io_enq_bits_size,
  input  [3:0]  io_enq_bits_cache,
  input  [2:0]  io_enq_bits_prot,
  input  [3:0]  io_enq_bits_echo_tl_state_size,
  input  [1:0]  io_enq_bits_echo_tl_state_source,
  input         io_enq_bits_wen,
  input         io_deq_ready,
  output        io_deq_valid,
  output        io_deq_bits_id,
  output [36:0] io_deq_bits_addr,
  output [7:0]  io_deq_bits_len,
  output [2:0]  io_deq_bits_size,
  output [1:0]  io_deq_bits_burst,
  output        io_deq_bits_lock,
  output [3:0]  io_deq_bits_cache,
  output [2:0]  io_deq_bits_prot,
  output [3:0]  io_deq_bits_qos,
  output [3:0]  io_deq_bits_echo_tl_state_size,
  output [1:0]  io_deq_bits_echo_tl_state_source,
  output        io_deq_bits_wen
);
`ifdef RANDOMIZE_MEM_INIT
  reg [31:0] _RAND_0;
  reg [63:0] _RAND_1;
  reg [31:0] _RAND_2;
  reg [31:0] _RAND_3;
  reg [31:0] _RAND_4;
  reg [31:0] _RAND_5;
  reg [31:0] _RAND_6;
  reg [31:0] _RAND_7;
  reg [31:0] _RAND_8;
  reg [31:0] _RAND_9;
  reg [31:0] _RAND_10;
  reg [31:0] _RAND_11;
`endif // RANDOMIZE_MEM_INIT
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_12;
`endif // RANDOMIZE_REG_INIT
  reg  ram_id [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_id_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_id_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_id_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_id_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_id_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_id_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_id_MPORT_en; // @[Decoupled.scala 259:95]
  reg [36:0] ram_addr [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_addr_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_addr_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [36:0] ram_addr_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [36:0] ram_addr_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_addr_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_addr_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_addr_MPORT_en; // @[Decoupled.scala 259:95]
  reg [7:0] ram_len [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_len_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_len_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [7:0] ram_len_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [7:0] ram_len_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_len_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_len_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_len_MPORT_en; // @[Decoupled.scala 259:95]
  reg [2:0] ram_size [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_size_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_size_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [2:0] ram_size_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [2:0] ram_size_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_size_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_size_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_size_MPORT_en; // @[Decoupled.scala 259:95]
  reg [1:0] ram_burst [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_burst_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_burst_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [1:0] ram_burst_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [1:0] ram_burst_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_burst_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_burst_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_burst_MPORT_en; // @[Decoupled.scala 259:95]
  reg  ram_lock [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_lock_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_lock_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_lock_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_lock_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_lock_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_lock_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_lock_MPORT_en; // @[Decoupled.scala 259:95]
  reg [3:0] ram_cache [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_cache_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_cache_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [3:0] ram_cache_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [3:0] ram_cache_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_cache_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_cache_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_cache_MPORT_en; // @[Decoupled.scala 259:95]
  reg [2:0] ram_prot [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_prot_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_prot_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [2:0] ram_prot_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [2:0] ram_prot_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_prot_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_prot_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_prot_MPORT_en; // @[Decoupled.scala 259:95]
  reg [3:0] ram_qos [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_qos_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_qos_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [3:0] ram_qos_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [3:0] ram_qos_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_qos_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_qos_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_qos_MPORT_en; // @[Decoupled.scala 259:95]
  reg [3:0] ram_echo_tl_state_size [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_size_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_size_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [3:0] ram_echo_tl_state_size_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [3:0] ram_echo_tl_state_size_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_size_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_size_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_size_MPORT_en; // @[Decoupled.scala 259:95]
  reg [1:0] ram_echo_tl_state_source [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_source_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_source_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire [1:0] ram_echo_tl_state_source_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire [1:0] ram_echo_tl_state_source_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_source_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_source_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_echo_tl_state_source_MPORT_en; // @[Decoupled.scala 259:95]
  reg  ram_wen [0:0]; // @[Decoupled.scala 259:95]
  wire  ram_wen_io_deq_bits_MPORT_en; // @[Decoupled.scala 259:95]
  wire  ram_wen_io_deq_bits_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_wen_io_deq_bits_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_wen_MPORT_data; // @[Decoupled.scala 259:95]
  wire  ram_wen_MPORT_addr; // @[Decoupled.scala 259:95]
  wire  ram_wen_MPORT_mask; // @[Decoupled.scala 259:95]
  wire  ram_wen_MPORT_en; // @[Decoupled.scala 259:95]
  reg  maybe_full; // @[Decoupled.scala 262:27]
  wire  empty = ~maybe_full; // @[Decoupled.scala 264:28]
  wire  _do_enq_T = io_enq_ready & io_enq_valid; // @[Decoupled.scala 50:35]
  wire  _do_deq_T = io_deq_ready & io_deq_valid; // @[Decoupled.scala 50:35]
  wire  _GEN_20 = io_deq_ready ? 1'h0 : _do_enq_T; // @[Decoupled.scala 304:{26,35}]
  wire  do_enq = empty ? _GEN_20 : _do_enq_T; // @[Decoupled.scala 301:17]
  wire  do_deq = empty ? 1'h0 : _do_deq_T; // @[Decoupled.scala 301:17 303:14]
  assign ram_id_io_deq_bits_MPORT_en = 1'h1;
  assign ram_id_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_id_io_deq_bits_MPORT_data = ram_id[ram_id_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_id_MPORT_data = io_enq_bits_id;
  assign ram_id_MPORT_addr = 1'h0;
  assign ram_id_MPORT_mask = 1'h1;
  assign ram_id_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_addr_io_deq_bits_MPORT_en = 1'h1;
  assign ram_addr_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_addr_io_deq_bits_MPORT_data = ram_addr[ram_addr_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_addr_MPORT_data = io_enq_bits_addr;
  assign ram_addr_MPORT_addr = 1'h0;
  assign ram_addr_MPORT_mask = 1'h1;
  assign ram_addr_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_len_io_deq_bits_MPORT_en = 1'h1;
  assign ram_len_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_len_io_deq_bits_MPORT_data = ram_len[ram_len_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_len_MPORT_data = io_enq_bits_len;
  assign ram_len_MPORT_addr = 1'h0;
  assign ram_len_MPORT_mask = 1'h1;
  assign ram_len_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_size_io_deq_bits_MPORT_en = 1'h1;
  assign ram_size_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_size_io_deq_bits_MPORT_data = ram_size[ram_size_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_size_MPORT_data = io_enq_bits_size;
  assign ram_size_MPORT_addr = 1'h0;
  assign ram_size_MPORT_mask = 1'h1;
  assign ram_size_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_burst_io_deq_bits_MPORT_en = 1'h1;
  assign ram_burst_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_burst_io_deq_bits_MPORT_data = ram_burst[ram_burst_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_burst_MPORT_data = 2'h1;
  assign ram_burst_MPORT_addr = 1'h0;
  assign ram_burst_MPORT_mask = 1'h1;
  assign ram_burst_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_lock_io_deq_bits_MPORT_en = 1'h1;
  assign ram_lock_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_lock_io_deq_bits_MPORT_data = ram_lock[ram_lock_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_lock_MPORT_data = 1'h0;
  assign ram_lock_MPORT_addr = 1'h0;
  assign ram_lock_MPORT_mask = 1'h1;
  assign ram_lock_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_cache_io_deq_bits_MPORT_en = 1'h1;
  assign ram_cache_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_cache_io_deq_bits_MPORT_data = ram_cache[ram_cache_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_cache_MPORT_data = io_enq_bits_cache;
  assign ram_cache_MPORT_addr = 1'h0;
  assign ram_cache_MPORT_mask = 1'h1;
  assign ram_cache_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_prot_io_deq_bits_MPORT_en = 1'h1;
  assign ram_prot_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_prot_io_deq_bits_MPORT_data = ram_prot[ram_prot_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_prot_MPORT_data = io_enq_bits_prot;
  assign ram_prot_MPORT_addr = 1'h0;
  assign ram_prot_MPORT_mask = 1'h1;
  assign ram_prot_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_qos_io_deq_bits_MPORT_en = 1'h1;
  assign ram_qos_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_qos_io_deq_bits_MPORT_data = ram_qos[ram_qos_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_qos_MPORT_data = 4'h0;
  assign ram_qos_MPORT_addr = 1'h0;
  assign ram_qos_MPORT_mask = 1'h1;
  assign ram_qos_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_echo_tl_state_size_io_deq_bits_MPORT_en = 1'h1;
  assign ram_echo_tl_state_size_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_echo_tl_state_size_io_deq_bits_MPORT_data =
    ram_echo_tl_state_size[ram_echo_tl_state_size_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_echo_tl_state_size_MPORT_data = io_enq_bits_echo_tl_state_size;
  assign ram_echo_tl_state_size_MPORT_addr = 1'h0;
  assign ram_echo_tl_state_size_MPORT_mask = 1'h1;
  assign ram_echo_tl_state_size_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_echo_tl_state_source_io_deq_bits_MPORT_en = 1'h1;
  assign ram_echo_tl_state_source_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_echo_tl_state_source_io_deq_bits_MPORT_data =
    ram_echo_tl_state_source[ram_echo_tl_state_source_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_echo_tl_state_source_MPORT_data = io_enq_bits_echo_tl_state_source;
  assign ram_echo_tl_state_source_MPORT_addr = 1'h0;
  assign ram_echo_tl_state_source_MPORT_mask = 1'h1;
  assign ram_echo_tl_state_source_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign ram_wen_io_deq_bits_MPORT_en = 1'h1;
  assign ram_wen_io_deq_bits_MPORT_addr = 1'h0;
  assign ram_wen_io_deq_bits_MPORT_data = ram_wen[ram_wen_io_deq_bits_MPORT_addr]; // @[Decoupled.scala 259:95]
  assign ram_wen_MPORT_data = io_enq_bits_wen;
  assign ram_wen_MPORT_addr = 1'h0;
  assign ram_wen_MPORT_mask = 1'h1;
  assign ram_wen_MPORT_en = empty ? _GEN_20 : _do_enq_T;
  assign io_enq_ready = ~maybe_full; // @[Decoupled.scala 289:19]
  assign io_deq_valid = io_enq_valid | ~empty; // @[Decoupled.scala 288:16 300:{24,39}]
  assign io_deq_bits_id = empty ? io_enq_bits_id : ram_id_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_addr = empty ? io_enq_bits_addr : ram_addr_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_len = empty ? io_enq_bits_len : ram_len_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_size = empty ? io_enq_bits_size : ram_size_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_burst = empty ? 2'h1 : ram_burst_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_lock = empty ? 1'h0 : ram_lock_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_cache = empty ? io_enq_bits_cache : ram_cache_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_prot = empty ? io_enq_bits_prot : ram_prot_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_qos = empty ? 4'h0 : ram_qos_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_echo_tl_state_size = empty ? io_enq_bits_echo_tl_state_size :
    ram_echo_tl_state_size_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_echo_tl_state_source = empty ? io_enq_bits_echo_tl_state_source :
    ram_echo_tl_state_source_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  assign io_deq_bits_wen = empty ? io_enq_bits_wen : ram_wen_io_deq_bits_MPORT_data; // @[Decoupled.scala 296:17 301:17 302:19]
  always @(posedge clock) begin
    if (ram_id_MPORT_en & ram_id_MPORT_mask) begin
      ram_id[ram_id_MPORT_addr] <= ram_id_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_addr_MPORT_en & ram_addr_MPORT_mask) begin
      ram_addr[ram_addr_MPORT_addr] <= ram_addr_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_len_MPORT_en & ram_len_MPORT_mask) begin
      ram_len[ram_len_MPORT_addr] <= ram_len_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_size_MPORT_en & ram_size_MPORT_mask) begin
      ram_size[ram_size_MPORT_addr] <= ram_size_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_burst_MPORT_en & ram_burst_MPORT_mask) begin
      ram_burst[ram_burst_MPORT_addr] <= ram_burst_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_lock_MPORT_en & ram_lock_MPORT_mask) begin
      ram_lock[ram_lock_MPORT_addr] <= ram_lock_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_cache_MPORT_en & ram_cache_MPORT_mask) begin
      ram_cache[ram_cache_MPORT_addr] <= ram_cache_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_prot_MPORT_en & ram_prot_MPORT_mask) begin
      ram_prot[ram_prot_MPORT_addr] <= ram_prot_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_qos_MPORT_en & ram_qos_MPORT_mask) begin
      ram_qos[ram_qos_MPORT_addr] <= ram_qos_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_echo_tl_state_size_MPORT_en & ram_echo_tl_state_size_MPORT_mask) begin
      ram_echo_tl_state_size[ram_echo_tl_state_size_MPORT_addr] <= ram_echo_tl_state_size_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_echo_tl_state_source_MPORT_en & ram_echo_tl_state_source_MPORT_mask) begin
      ram_echo_tl_state_source[ram_echo_tl_state_source_MPORT_addr] <= ram_echo_tl_state_source_MPORT_data; // @[Decoupled.scala 259:95]
    end
    if (ram_wen_MPORT_en & ram_wen_MPORT_mask) begin
      ram_wen[ram_wen_MPORT_addr] <= ram_wen_MPORT_data; // @[Decoupled.scala 259:95]
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
    ram_id[initvar] = _RAND_0[0:0];
  _RAND_1 = {2{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_addr[initvar] = _RAND_1[36:0];
  _RAND_2 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_len[initvar] = _RAND_2[7:0];
  _RAND_3 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_size[initvar] = _RAND_3[2:0];
  _RAND_4 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_burst[initvar] = _RAND_4[1:0];
  _RAND_5 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_lock[initvar] = _RAND_5[0:0];
  _RAND_6 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_cache[initvar] = _RAND_6[3:0];
  _RAND_7 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_prot[initvar] = _RAND_7[2:0];
  _RAND_8 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_qos[initvar] = _RAND_8[3:0];
  _RAND_9 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_echo_tl_state_size[initvar] = _RAND_9[3:0];
  _RAND_10 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_echo_tl_state_source[initvar] = _RAND_10[1:0];
  _RAND_11 = {1{`RANDOM}};
  for (initvar = 0; initvar < 1; initvar = initvar+1)
    ram_wen[initvar] = _RAND_11[0:0];
`endif // RANDOMIZE_MEM_INIT
`ifdef RANDOMIZE_REG_INIT
  _RAND_12 = {1{`RANDOM}};
  maybe_full = _RAND_12[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

