module TLToAXI4_2(
  input         clock,
  input         reset,
  output        auto_in_a_ready,
  input         auto_in_a_valid,
  input  [2:0]  auto_in_a_bits_opcode,
  input  [2:0]  auto_in_a_bits_size,
  input  [1:0]  auto_in_a_bits_source,
  input  [36:0] auto_in_a_bits_address,
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
  output [3:0]  auto_out_awecho_tl_state_size,
  output [1:0]  auto_out_awecho_tl_state_source,
  input         auto_out_wready,
  output        auto_out_wvalid,
  output [63:0] auto_out_wdata,
  output [7:0]  auto_out_wstrb,
  output        auto_out_wlast,
  output        auto_out_bready,
  input         auto_out_bvalid,
  input         auto_out_bid,
  input  [1:0]  auto_out_bresp,
  input  [3:0]  auto_out_becho_tl_state_size,
  input  [1:0]  auto_out_becho_tl_state_source,
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
  output [3:0]  auto_out_arecho_tl_state_size,
  output [1:0]  auto_out_arecho_tl_state_source,
  output        auto_out_rready,
  input         auto_out_rvalid,
  input         auto_out_rid,
  input  [63:0] auto_out_rdata,
  input  [1:0]  auto_out_rresp,
  input  [3:0]  auto_out_recho_tl_state_size,
  input  [1:0]  auto_out_recho_tl_state_source,
  input         auto_out_rlast
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
  reg [31:0] _RAND_8;
  reg [31:0] _RAND_9;
`endif // RANDOMIZE_REG_INIT
  wire  deq_clock; // @[Decoupled.scala 361:21]
  wire  deq_reset; // @[Decoupled.scala 361:21]
  wire  deq_io_enq_ready; // @[Decoupled.scala 361:21]
  wire  deq_io_enq_valid; // @[Decoupled.scala 361:21]
  wire [63:0] deq_io_enq_bits_data; // @[Decoupled.scala 361:21]
  wire [7:0] deq_io_enq_bits_strb; // @[Decoupled.scala 361:21]
  wire  deq_io_enq_bits_last; // @[Decoupled.scala 361:21]
  wire  deq_io_deq_ready; // @[Decoupled.scala 361:21]
  wire  deq_io_deq_valid; // @[Decoupled.scala 361:21]
  wire [63:0] deq_io_deq_bits_data; // @[Decoupled.scala 361:21]
  wire [7:0] deq_io_deq_bits_strb; // @[Decoupled.scala 361:21]
  wire  deq_io_deq_bits_last; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_clock; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_reset; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_enq_ready; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_enq_valid; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_enq_bits_id; // @[Decoupled.scala 361:21]
  wire [36:0] queue_arw_deq_io_enq_bits_addr; // @[Decoupled.scala 361:21]
  wire [7:0] queue_arw_deq_io_enq_bits_len; // @[Decoupled.scala 361:21]
  wire [2:0] queue_arw_deq_io_enq_bits_size; // @[Decoupled.scala 361:21]
  wire [3:0] queue_arw_deq_io_enq_bits_cache; // @[Decoupled.scala 361:21]
  wire [2:0] queue_arw_deq_io_enq_bits_prot; // @[Decoupled.scala 361:21]
  wire [3:0] queue_arw_deq_io_enq_bits_echo_tl_state_size; // @[Decoupled.scala 361:21]
  wire [1:0] queue_arw_deq_io_enq_bits_echo_tl_state_source; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_enq_bits_wen; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_deq_ready; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_deq_valid; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_deq_bits_id; // @[Decoupled.scala 361:21]
  wire [36:0] queue_arw_deq_io_deq_bits_addr; // @[Decoupled.scala 361:21]
  wire [7:0] queue_arw_deq_io_deq_bits_len; // @[Decoupled.scala 361:21]
  wire [2:0] queue_arw_deq_io_deq_bits_size; // @[Decoupled.scala 361:21]
  wire [1:0] queue_arw_deq_io_deq_bits_burst; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_deq_bits_lock; // @[Decoupled.scala 361:21]
  wire [3:0] queue_arw_deq_io_deq_bits_cache; // @[Decoupled.scala 361:21]
  wire [2:0] queue_arw_deq_io_deq_bits_prot; // @[Decoupled.scala 361:21]
  wire [3:0] queue_arw_deq_io_deq_bits_qos; // @[Decoupled.scala 361:21]
  wire [3:0] queue_arw_deq_io_deq_bits_echo_tl_state_size; // @[Decoupled.scala 361:21]
  wire [1:0] queue_arw_deq_io_deq_bits_echo_tl_state_source; // @[Decoupled.scala 361:21]
  wire  queue_arw_deq_io_deq_bits_wen; // @[Decoupled.scala 361:21]
  wire  a_isPut = ~auto_in_a_bits_opcode[2]; // @[Edges.scala 91:28]
  reg [1:0] count_2; // @[ToAXI4.scala 254:28]
  wire  idle_1 = count_2 == 2'h0; // @[ToAXI4.scala 256:26]
  reg  write_1; // @[ToAXI4.scala 255:24]
  wire  mismatch_1 = write_1 != a_isPut; // @[ToAXI4.scala 267:50]
  wire  idStall_1 = ~idle_1 & mismatch_1 | count_2 == 2'h2; // @[ToAXI4.scala 268:34]
  reg [1:0] count_1; // @[ToAXI4.scala 254:28]
  wire  idle = count_1 == 2'h0; // @[ToAXI4.scala 256:26]
  reg  write; // @[ToAXI4.scala 255:24]
  wire  mismatch = write != a_isPut; // @[ToAXI4.scala 267:50]
  wire  idStall_0 = ~idle & mismatch | count_1 == 2'h2; // @[ToAXI4.scala 268:34]
  wire  _GEN_8 = 2'h2 == auto_in_a_bits_source ? idStall_1 : idStall_0; // @[ToAXI4.scala 195:{49,49}]
  wire  _GEN_9 = 2'h3 == auto_in_a_bits_source ? idStall_1 : _GEN_8; // @[ToAXI4.scala 195:{49,49}]
  reg [3:0] counter; // @[Edges.scala 228:27]
  wire  a_first = counter == 4'h0; // @[Edges.scala 230:25]
  wire  stall = _GEN_9 & a_first; // @[ToAXI4.scala 195:49]
  wire  _bundleIn_0_a_ready_T = ~stall; // @[ToAXI4.scala 196:21]
  reg  doneAW; // @[ToAXI4.scala 161:30]
  wire  out_arw_ready = queue_arw_deq_io_enq_ready; // @[ToAXI4.scala 147:25 Decoupled.scala 365:17]
  wire  _bundleIn_0_a_ready_T_1 = doneAW | out_arw_ready; // @[ToAXI4.scala 196:52]
  wire  out_wready = deq_io_enq_ready; // @[ToAXI4.scala 148:23 Decoupled.scala 365:17]
  wire  _bundleIn_0_a_ready_T_3 = a_isPut ? (doneAW | out_arw_ready) & out_wready : out_arw_ready; // @[ToAXI4.scala 196:34]
  wire  bundleIn_0_a_ready = ~stall & _bundleIn_0_a_ready_T_3; // @[ToAXI4.scala 196:28]
  wire  _T = bundleIn_0_a_ready & auto_in_a_valid; // @[Decoupled.scala 50:35]
  wire [13:0] _beats1_decode_T_1 = 14'h7f << auto_in_a_bits_size; // @[package.scala 234:77]
  wire [6:0] _beats1_decode_T_3 = ~_beats1_decode_T_1[6:0]; // @[package.scala 234:46]
  wire [3:0] beats1_decode = _beats1_decode_T_3[6:3]; // @[Edges.scala 219:59]
  wire [3:0] beats1 = a_isPut ? beats1_decode : 4'h0; // @[Edges.scala 220:14]
  wire [3:0] counter1 = counter - 4'h1; // @[Edges.scala 229:28]
  wire  a_last = counter == 4'h1 | beats1 == 4'h0; // @[Edges.scala 231:37]
  wire  queue_arw_bits_wen = queue_arw_deq_io_deq_bits_wen; // @[Decoupled.scala 401:19 402:14]
  wire  queue_arw_valid = queue_arw_deq_io_deq_valid; // @[Decoupled.scala 401:19 403:15]
  wire  out_arw_bits_id = 2'h3 == auto_in_a_bits_source | 2'h2 == auto_in_a_bits_source; // @[ToAXI4.scala 166:{17,17}]
  wire [17:0] _out_arw_bits_len_T_1 = 18'h7ff << auto_in_a_bits_size; // @[package.scala 234:77]
  wire [10:0] _out_arw_bits_len_T_3 = ~_out_arw_bits_len_T_1[10:0]; // @[package.scala 234:46]
  wire  prot_1 = ~auto_in_a_bits_user_amba_prot_secure; // @[ToAXI4.scala 185:20]
  wire [1:0] out_arw_bits_prot_hi = {auto_in_a_bits_user_amba_prot_fetch,prot_1}; // @[Cat.scala 31:58]
  wire [1:0] out_arw_bits_cache_lo = {auto_in_a_bits_user_amba_prot_modifiable,auto_in_a_bits_user_amba_prot_bufferable}
    ; // @[Cat.scala 31:58]
  wire [1:0] out_arw_bits_cache_hi = {auto_in_a_bits_user_amba_prot_writealloc,auto_in_a_bits_user_amba_prot_readalloc}; // @[Cat.scala 31:58]
  wire  _out_arw_valid_T_1 = _bundleIn_0_a_ready_T & auto_in_a_valid; // @[ToAXI4.scala 197:31]
  wire  _out_arw_valid_T_4 = a_isPut ? ~doneAW & out_wready : 1'h1; // @[ToAXI4.scala 197:51]
  wire  out_arw_valid = _bundleIn_0_a_ready_T & auto_in_a_valid & _out_arw_valid_T_4; // @[ToAXI4.scala 197:45]
  reg  r_holds_d; // @[ToAXI4.scala 206:30]
  reg [2:0] b_delay; // @[ToAXI4.scala 209:24]
  wire  r_wins = auto_out_rvalid & b_delay != 3'h7 | r_holds_d; // @[ToAXI4.scala 215:57]
  wire  bundleOut_0_rready = auto_in_d_ready & r_wins; // @[ToAXI4.scala 217:33]
  wire  _T_2 = bundleOut_0_rready & auto_out_rvalid; // @[Decoupled.scala 50:35]
  wire  bundleOut_0_bready = auto_in_d_ready & ~r_wins; // @[ToAXI4.scala 218:33]
  wire [2:0] _bdelay_T_1 = b_delay + 3'h1; // @[ToAXI4.scala 211:28]
  wire  bundleIn_0_d_valid = r_wins ? auto_out_rvalid : auto_out_bvalid; // @[ToAXI4.scala 219:24]
  reg  r_first; // @[ToAXI4.scala 224:28]
  wire  _GEN_12 = _T_2 ? auto_out_rlast : r_first; // @[ToAXI4.scala 225:27 224:28 225:37]
  wire  _rdenied_T = auto_out_rresp == 2'h3; // @[ToAXI4.scala 226:39]
  reg  r_denied_r; // @[Reg.scala 16:16]
  wire  _GEN_13 = r_first ? _rdenied_T : r_denied_r; // @[Reg.scala 16:16 17:{18,22}]
  wire  r_corrupt = auto_out_rresp != 2'h0; // @[ToAXI4.scala 227:39]
  wire  b_denied = auto_out_bresp != 2'h0; // @[ToAXI4.scala 228:39]
  wire  r_d_corrupt = r_corrupt | _GEN_13; // @[ToAXI4.scala 230:100]
  wire [2:0] r_d_size = auto_out_recho_tl_state_size[2:0]; // @[Edges.scala 771:17 774:15]
  wire [2:0] b_d_size = auto_out_becho_tl_state_size[2:0]; // @[Edges.scala 755:17 758:15]
  wire [1:0] _a_sel_T = 2'h1 << out_arw_bits_id; // @[OneHot.scala 64:12]
  wire  a_sel_0 = _a_sel_T[0]; // @[ToAXI4.scala 242:58]
  wire  a_sel_1 = _a_sel_T[1]; // @[ToAXI4.scala 242:58]
  wire  d_sel_shiftAmount = r_wins ? auto_out_rid : auto_out_bid; // @[ToAXI4.scala 243:31]
  wire [1:0] _d_sel_T_1 = 2'h1 << d_sel_shiftAmount; // @[OneHot.scala 64:12]
  wire  d_sel_0 = _d_sel_T_1[0]; // @[ToAXI4.scala 243:93]
  wire  d_sel_1 = _d_sel_T_1[1]; // @[ToAXI4.scala 243:93]
  wire  d_last = r_wins ? auto_out_rlast : 1'h1; // @[ToAXI4.scala 244:23]
  wire  _inc_T = out_arw_ready & out_arw_valid; // @[Decoupled.scala 50:35]
  wire  inc = a_sel_0 & _inc_T; // @[ToAXI4.scala 258:22]
  wire  _dec_T_1 = auto_in_d_ready & bundleIn_0_d_valid; // @[Decoupled.scala 50:35]
  wire  dec = d_sel_0 & d_last & _dec_T_1; // @[ToAXI4.scala 259:32]
  wire [1:0] _GEN_17 = {{1'd0}, inc}; // @[ToAXI4.scala 260:24]
  wire [1:0] _count_T_2 = count_1 + _GEN_17; // @[ToAXI4.scala 260:24]
  wire [1:0] _GEN_18 = {{1'd0}, dec}; // @[ToAXI4.scala 260:37]
  wire [1:0] _count_T_4 = _count_T_2 - _GEN_18; // @[ToAXI4.scala 260:37]
  wire  inc_1 = a_sel_1 & _inc_T; // @[ToAXI4.scala 258:22]
  wire  dec_1 = d_sel_1 & d_last & _dec_T_1; // @[ToAXI4.scala 259:32]
  wire [1:0] _GEN_19 = {{1'd0}, inc_1}; // @[ToAXI4.scala 260:24]
  wire [1:0] _count_T_6 = count_2 + _GEN_19; // @[ToAXI4.scala 260:24]
  wire [1:0] _GEN_20 = {{1'd0}, dec_1}; // @[ToAXI4.scala 260:37]
  wire [1:0] _count_T_8 = _count_T_6 - _GEN_20; // @[ToAXI4.scala 260:37]
  Queue_462 deq ( // @[Decoupled.scala 361:21]
    .clock(deq_clock),
    .reset(deq_reset),
    .io_enq_ready(deq_io_enq_ready),
    .io_enq_valid(deq_io_enq_valid),
    .io_enq_bits_data(deq_io_enq_bits_data),
    .io_enq_bits_strb(deq_io_enq_bits_strb),
    .io_enq_bits_last(deq_io_enq_bits_last),
    .io_deq_ready(deq_io_deq_ready),
    .io_deq_valid(deq_io_deq_valid),
    .io_deq_bits_data(deq_io_deq_bits_data),
    .io_deq_bits_strb(deq_io_deq_bits_strb),
    .io_deq_bits_last(deq_io_deq_bits_last)
  );
  Queue_463 queue_arw_deq ( // @[Decoupled.scala 361:21]
    .clock(queue_arw_deq_clock),
    .reset(queue_arw_deq_reset),
    .io_enq_ready(queue_arw_deq_io_enq_ready),
    .io_enq_valid(queue_arw_deq_io_enq_valid),
    .io_enq_bits_id(queue_arw_deq_io_enq_bits_id),
    .io_enq_bits_addr(queue_arw_deq_io_enq_bits_addr),
    .io_enq_bits_len(queue_arw_deq_io_enq_bits_len),
    .io_enq_bits_size(queue_arw_deq_io_enq_bits_size),
    .io_enq_bits_cache(queue_arw_deq_io_enq_bits_cache),
    .io_enq_bits_prot(queue_arw_deq_io_enq_bits_prot),
    .io_enq_bits_echo_tl_state_size(queue_arw_deq_io_enq_bits_echo_tl_state_size),
    .io_enq_bits_echo_tl_state_source(queue_arw_deq_io_enq_bits_echo_tl_state_source),
    .io_enq_bits_wen(queue_arw_deq_io_enq_bits_wen),
    .io_deq_ready(queue_arw_deq_io_deq_ready),
    .io_deq_valid(queue_arw_deq_io_deq_valid),
    .io_deq_bits_id(queue_arw_deq_io_deq_bits_id),
    .io_deq_bits_addr(queue_arw_deq_io_deq_bits_addr),
    .io_deq_bits_len(queue_arw_deq_io_deq_bits_len),
    .io_deq_bits_size(queue_arw_deq_io_deq_bits_size),
    .io_deq_bits_burst(queue_arw_deq_io_deq_bits_burst),
    .io_deq_bits_lock(queue_arw_deq_io_deq_bits_lock),
    .io_deq_bits_cache(queue_arw_deq_io_deq_bits_cache),
    .io_deq_bits_prot(queue_arw_deq_io_deq_bits_prot),
    .io_deq_bits_qos(queue_arw_deq_io_deq_bits_qos),
    .io_deq_bits_echo_tl_state_size(queue_arw_deq_io_deq_bits_echo_tl_state_size),
    .io_deq_bits_echo_tl_state_source(queue_arw_deq_io_deq_bits_echo_tl_state_source),
    .io_deq_bits_wen(queue_arw_deq_io_deq_bits_wen)
  );
  assign auto_in_a_ready = ~stall & _bundleIn_0_a_ready_T_3; // @[ToAXI4.scala 196:28]
  assign auto_in_d_valid = r_wins ? auto_out_rvalid : auto_out_bvalid; // @[ToAXI4.scala 219:24]
  assign auto_in_d_bits_opcode = r_wins ? 3'h1 : 3'h0; // @[ToAXI4.scala 237:23]
  assign auto_in_d_bits_size = r_wins ? r_d_size : b_d_size; // @[ToAXI4.scala 237:23]
  assign auto_in_d_bits_source = r_wins ? auto_out_recho_tl_state_source : auto_out_becho_tl_state_source; // @[ToAXI4.scala 237:23]
  assign auto_in_d_bits_denied = r_wins ? _GEN_13 : b_denied; // @[ToAXI4.scala 237:23]
  assign auto_in_d_bits_data = auto_out_rdata; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_corrupt = r_wins & r_d_corrupt; // @[ToAXI4.scala 237:23]
  assign auto_out_awvalid = queue_arw_valid & queue_arw_bits_wen; // @[ToAXI4.scala 156:39]
  assign auto_out_awid = queue_arw_deq_io_deq_bits_id; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awaddr = queue_arw_deq_io_deq_bits_addr; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awlen = queue_arw_deq_io_deq_bits_len; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awsize = queue_arw_deq_io_deq_bits_size; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awburst = queue_arw_deq_io_deq_bits_burst; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awlock = queue_arw_deq_io_deq_bits_lock; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awcache = queue_arw_deq_io_deq_bits_cache; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awprot = queue_arw_deq_io_deq_bits_prot; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awqos = queue_arw_deq_io_deq_bits_qos; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awecho_tl_state_size = queue_arw_deq_io_deq_bits_echo_tl_state_size; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_awecho_tl_state_source = queue_arw_deq_io_deq_bits_echo_tl_state_source; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_wvalid = deq_io_deq_valid; // @[Decoupled.scala 401:19 403:15]
  assign auto_out_wdata = deq_io_deq_bits_data; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_wstrb = deq_io_deq_bits_strb; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_wlast = deq_io_deq_bits_last; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_bready = auto_in_d_ready & ~r_wins; // @[ToAXI4.scala 218:33]
  assign auto_out_arvalid = queue_arw_valid & ~queue_arw_bits_wen; // @[ToAXI4.scala 155:39]
  assign auto_out_arid = queue_arw_deq_io_deq_bits_id; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_araddr = queue_arw_deq_io_deq_bits_addr; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arlen = queue_arw_deq_io_deq_bits_len; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arsize = queue_arw_deq_io_deq_bits_size; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arburst = queue_arw_deq_io_deq_bits_burst; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arlock = queue_arw_deq_io_deq_bits_lock; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arcache = queue_arw_deq_io_deq_bits_cache; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arprot = queue_arw_deq_io_deq_bits_prot; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arqos = queue_arw_deq_io_deq_bits_qos; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arecho_tl_state_size = queue_arw_deq_io_deq_bits_echo_tl_state_size; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_arecho_tl_state_source = queue_arw_deq_io_deq_bits_echo_tl_state_source; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_rready = auto_in_d_ready & r_wins; // @[ToAXI4.scala 217:33]
  assign deq_clock = clock;
  assign deq_reset = reset;
  assign deq_io_enq_valid = _out_arw_valid_T_1 & a_isPut & _bundleIn_0_a_ready_T_1; // @[ToAXI4.scala 199:54]
  assign deq_io_enq_bits_data = auto_in_a_bits_data; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign deq_io_enq_bits_strb = auto_in_a_bits_mask; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign deq_io_enq_bits_last = counter == 4'h1 | beats1 == 4'h0; // @[Edges.scala 231:37]
  assign deq_io_deq_ready = auto_out_wready; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign queue_arw_deq_clock = clock;
  assign queue_arw_deq_reset = reset;
  assign queue_arw_deq_io_enq_valid = _bundleIn_0_a_ready_T & auto_in_a_valid & _out_arw_valid_T_4; // @[ToAXI4.scala 197:45]
  assign queue_arw_deq_io_enq_bits_id = 2'h3 == auto_in_a_bits_source | 2'h2 == auto_in_a_bits_source; // @[ToAXI4.scala 166:{17,17}]
  assign queue_arw_deq_io_enq_bits_addr = auto_in_a_bits_address; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign queue_arw_deq_io_enq_bits_len = _out_arw_bits_len_T_3[10:3]; // @[ToAXI4.scala 168:84]
  assign queue_arw_deq_io_enq_bits_size = auto_in_a_bits_size >= 3'h3 ? 3'h3 : auto_in_a_bits_size; // @[ToAXI4.scala 169:23]
  assign queue_arw_deq_io_enq_bits_cache = {out_arw_bits_cache_hi,out_arw_bits_cache_lo}; // @[Cat.scala 31:58]
  assign queue_arw_deq_io_enq_bits_prot = {out_arw_bits_prot_hi,auto_in_a_bits_user_amba_prot_privileged}; // @[Cat.scala 31:58]
  assign queue_arw_deq_io_enq_bits_echo_tl_state_size = {{1'd0}, auto_in_a_bits_size}; // @[ToAXI4.scala 147:25 179:22]
  assign queue_arw_deq_io_enq_bits_echo_tl_state_source = auto_in_a_bits_source; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign queue_arw_deq_io_enq_bits_wen = ~auto_in_a_bits_opcode[2]; // @[Edges.scala 91:28]
  assign queue_arw_deq_io_deq_ready = queue_arw_bits_wen ? auto_out_awready : auto_out_arready; // @[ToAXI4.scala 157:29]
  always @(posedge clock) begin
    if (reset) begin // @[ToAXI4.scala 254:28]
      count_2 <= 2'h0; // @[ToAXI4.scala 254:28]
    end else begin
      count_2 <= _count_T_8; // @[ToAXI4.scala 260:15]
    end
    if (inc_1) begin // @[ToAXI4.scala 265:20]
      write_1 <= a_isPut; // @[ToAXI4.scala 265:28]
    end
    if (reset) begin // @[ToAXI4.scala 254:28]
      count_1 <= 2'h0; // @[ToAXI4.scala 254:28]
    end else begin
      count_1 <= _count_T_4; // @[ToAXI4.scala 260:15]
    end
    if (inc) begin // @[ToAXI4.scala 265:20]
      write <= a_isPut; // @[ToAXI4.scala 265:28]
    end
    if (reset) begin // @[Edges.scala 228:27]
      counter <= 4'h0; // @[Edges.scala 228:27]
    end else if (_T) begin // @[Edges.scala 234:17]
      if (a_first) begin // @[Edges.scala 235:21]
        if (a_isPut) begin // @[Edges.scala 220:14]
          counter <= beats1_decode;
        end else begin
          counter <= 4'h0;
        end
      end else begin
        counter <= counter1;
      end
    end
    if (reset) begin // @[ToAXI4.scala 161:30]
      doneAW <= 1'h0; // @[ToAXI4.scala 161:30]
    end else if (_T) begin // @[ToAXI4.scala 162:26]
      doneAW <= ~a_last; // @[ToAXI4.scala 162:35]
    end
    if (reset) begin // @[ToAXI4.scala 206:30]
      r_holds_d <= 1'h0; // @[ToAXI4.scala 206:30]
    end else if (_T_2) begin // @[ToAXI4.scala 207:27]
      r_holds_d <= ~auto_out_rlast; // @[ToAXI4.scala 207:39]
    end
    if (auto_out_bvalid & ~bundleOut_0_bready) begin // @[ToAXI4.scala 210:42]
      b_delay <= _bdelay_T_1; // @[ToAXI4.scala 211:17]
    end else begin
      b_delay <= 3'h0; // @[ToAXI4.scala 213:17]
    end
    r_first <= reset | _GEN_12; // @[ToAXI4.scala 224:{28,28}]
    if (r_first) begin // @[Reg.scala 17:18]
      r_denied_r <= _rdenied_T; // @[Reg.scala 17:22]
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
  count_2 = _RAND_0[1:0];
  _RAND_1 = {1{`RANDOM}};
  write_1 = _RAND_1[0:0];
  _RAND_2 = {1{`RANDOM}};
  count_1 = _RAND_2[1:0];
  _RAND_3 = {1{`RANDOM}};
  write = _RAND_3[0:0];
  _RAND_4 = {1{`RANDOM}};
  counter = _RAND_4[3:0];
  _RAND_5 = {1{`RANDOM}};
  doneAW = _RAND_5[0:0];
  _RAND_6 = {1{`RANDOM}};
  r_holds_d = _RAND_6[0:0];
  _RAND_7 = {1{`RANDOM}};
  b_delay = _RAND_7[2:0];
  _RAND_8 = {1{`RANDOM}};
  r_first = _RAND_8[0:0];
  _RAND_9 = {1{`RANDOM}};
  r_denied_r = _RAND_9[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

