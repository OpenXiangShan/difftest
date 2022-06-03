module TLWidthWidget_4(
  input          clock,
  input          reset,
  output         auto_in_a_ready,
  input          auto_in_a_valid,
  input  [2:0]   auto_in_a_bits_opcode,
  input  [2:0]   auto_in_a_bits_size,
  input  [1:0]   auto_in_a_bits_source,
  input  [37:0]  auto_in_a_bits_address,
  input          auto_in_a_bits_user_amba_prot_bufferable,
  input          auto_in_a_bits_user_amba_prot_modifiable,
  input          auto_in_a_bits_user_amba_prot_readalloc,
  input          auto_in_a_bits_user_amba_prot_writealloc,
  input          auto_in_a_bits_user_amba_prot_privileged,
  input          auto_in_a_bits_user_amba_prot_secure,
  input          auto_in_a_bits_user_amba_prot_fetch,
  input  [31:0]  auto_in_a_bits_mask,
  input  [255:0] auto_in_a_bits_data,
  input          auto_in_d_ready,
  output         auto_in_d_valid,
  output [2:0]   auto_in_d_bits_opcode,
  output [2:0]   auto_in_d_bits_size,
  output [1:0]   auto_in_d_bits_source,
  output         auto_in_d_bits_denied,
  output [255:0] auto_in_d_bits_data,
  output         auto_in_d_bits_corrupt,
  input          auto_out_a_ready,
  output         auto_out_a_valid,
  output [2:0]   auto_out_a_bits_opcode,
  output [2:0]   auto_out_a_bits_size,
  output [1:0]   auto_out_a_bits_source,
  output [37:0]  auto_out_a_bits_address,
  output         auto_out_a_bits_user_amba_prot_bufferable,
  output         auto_out_a_bits_user_amba_prot_modifiable,
  output         auto_out_a_bits_user_amba_prot_readalloc,
  output         auto_out_a_bits_user_amba_prot_writealloc,
  output         auto_out_a_bits_user_amba_prot_privileged,
  output         auto_out_a_bits_user_amba_prot_secure,
  output         auto_out_a_bits_user_amba_prot_fetch,
  output [7:0]   auto_out_a_bits_mask,
  output [63:0]  auto_out_a_bits_data,
  output         auto_out_d_ready,
  input          auto_out_d_valid,
  input  [2:0]   auto_out_d_bits_opcode,
  input  [2:0]   auto_out_d_bits_size,
  input  [1:0]   auto_out_d_bits_source,
  input          auto_out_d_bits_denied,
  input  [63:0]  auto_out_d_bits_data,
  input          auto_out_d_bits_corrupt
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
  reg [31:0] _RAND_2;
  reg [31:0] _RAND_3;
  reg [63:0] _RAND_4;
  reg [63:0] _RAND_5;
  reg [63:0] _RAND_6;
`endif // RANDOMIZE_REG_INIT
  wire  repeated_repeater_clock; // @[Repeater.scala 35:26]
  wire  repeated_repeater_reset; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_repeat; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_ready; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_valid; // @[Repeater.scala 35:26]
  wire [2:0] repeated_repeater_io_enq_bits_opcode; // @[Repeater.scala 35:26]
  wire [2:0] repeated_repeater_io_enq_bits_size; // @[Repeater.scala 35:26]
  wire [1:0] repeated_repeater_io_enq_bits_source; // @[Repeater.scala 35:26]
  wire [37:0] repeated_repeater_io_enq_bits_address; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_bits_user_amba_prot_bufferable; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_bits_user_amba_prot_modifiable; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_bits_user_amba_prot_readalloc; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_bits_user_amba_prot_writealloc; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_bits_user_amba_prot_privileged; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_bits_user_amba_prot_secure; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_enq_bits_user_amba_prot_fetch; // @[Repeater.scala 35:26]
  wire [31:0] repeated_repeater_io_enq_bits_mask; // @[Repeater.scala 35:26]
  wire [255:0] repeated_repeater_io_enq_bits_data; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_ready; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_valid; // @[Repeater.scala 35:26]
  wire [2:0] repeated_repeater_io_deq_bits_opcode; // @[Repeater.scala 35:26]
  wire [2:0] repeated_repeater_io_deq_bits_size; // @[Repeater.scala 35:26]
  wire [1:0] repeated_repeater_io_deq_bits_source; // @[Repeater.scala 35:26]
  wire [37:0] repeated_repeater_io_deq_bits_address; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_bits_user_amba_prot_bufferable; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_bits_user_amba_prot_modifiable; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_bits_user_amba_prot_readalloc; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_bits_user_amba_prot_writealloc; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_bits_user_amba_prot_privileged; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_bits_user_amba_prot_secure; // @[Repeater.scala 35:26]
  wire  repeated_repeater_io_deq_bits_user_amba_prot_fetch; // @[Repeater.scala 35:26]
  wire [31:0] repeated_repeater_io_deq_bits_mask; // @[Repeater.scala 35:26]
  wire [255:0] repeated_repeater_io_deq_bits_data; // @[Repeater.scala 35:26]
  wire [255:0] cated_bits_data = {repeated_repeater_io_deq_bits_data[255:64],auto_in_a_bits_data[63:0]}; // @[Cat.scala 31:58]
  wire [2:0] cated_bits_opcode = repeated_repeater_io_deq_bits_opcode; // @[WidthWidget.scala 155:25 156:15]
  wire  repeat_hasData = ~cated_bits_opcode[2]; // @[Edges.scala 91:28]
  wire [2:0] cated_bits_size = repeated_repeater_io_deq_bits_size; // @[WidthWidget.scala 155:25 156:15]
  wire [11:0] _repeat_limit_T_1 = 12'h1f << cated_bits_size; // @[package.scala 234:77]
  wire [4:0] _repeat_limit_T_3 = ~_repeat_limit_T_1[4:0]; // @[package.scala 234:46]
  wire [1:0] repeat_limit = _repeat_limit_T_3[4:3]; // @[WidthWidget.scala 97:47]
  reg [1:0] repeat_count; // @[WidthWidget.scala 99:26]
  wire  repeat_last = repeat_count == repeat_limit | ~repeat_hasData; // @[WidthWidget.scala 101:35]
  wire  cated_valid = repeated_repeater_io_deq_valid; // @[WidthWidget.scala 155:25 156:15]
  wire  _repeat_T = auto_out_a_ready & cated_valid; // @[Decoupled.scala 50:35]
  wire [1:0] _repeat_count_T_1 = repeat_count + 2'h1; // @[WidthWidget.scala 104:24]
  wire [37:0] cated_bits_address = repeated_repeater_io_deq_bits_address; // @[WidthWidget.scala 155:25 156:15]
  wire [1:0] repeat_sel = cated_bits_address[4:3]; // @[WidthWidget.scala 110:39]
  wire [1:0] repeat_index = repeat_sel | repeat_count; // @[WidthWidget.scala 120:24]
  wire [63:0] repeat_bundleOut_0_a_bits_data_mux_0 = cated_bits_data[63:0]; // @[WidthWidget.scala 122:55]
  wire [63:0] repeat_bundleOut_0_a_bits_data_mux_1 = cated_bits_data[127:64]; // @[WidthWidget.scala 122:55]
  wire [63:0] repeat_bundleOut_0_a_bits_data_mux_2 = cated_bits_data[191:128]; // @[WidthWidget.scala 122:55]
  wire [63:0] repeat_bundleOut_0_a_bits_data_mux_3 = cated_bits_data[255:192]; // @[WidthWidget.scala 122:55]
  wire [63:0] _GEN_3 = 2'h1 == repeat_index ? repeat_bundleOut_0_a_bits_data_mux_1 :
    repeat_bundleOut_0_a_bits_data_mux_0; // @[WidthWidget.scala 131:{30,30}]
  wire [63:0] _GEN_4 = 2'h2 == repeat_index ? repeat_bundleOut_0_a_bits_data_mux_2 : _GEN_3; // @[WidthWidget.scala 131:{30,30}]
  wire [31:0] cated_bits_mask = repeated_repeater_io_deq_bits_mask; // @[WidthWidget.scala 155:25 156:15]
  wire [7:0] repeat_bundleOut_0_a_bits_mask_mux_0 = cated_bits_mask[7:0]; // @[WidthWidget.scala 122:55]
  wire [7:0] repeat_bundleOut_0_a_bits_mask_mux_1 = cated_bits_mask[15:8]; // @[WidthWidget.scala 122:55]
  wire [7:0] repeat_bundleOut_0_a_bits_mask_mux_2 = cated_bits_mask[23:16]; // @[WidthWidget.scala 122:55]
  wire [7:0] repeat_bundleOut_0_a_bits_mask_mux_3 = cated_bits_mask[31:24]; // @[WidthWidget.scala 122:55]
  wire [7:0] _GEN_7 = 2'h1 == repeat_index ? repeat_bundleOut_0_a_bits_mask_mux_1 : repeat_bundleOut_0_a_bits_mask_mux_0
    ; // @[WidthWidget.scala 134:{53,53}]
  wire [7:0] _GEN_8 = 2'h2 == repeat_index ? repeat_bundleOut_0_a_bits_mask_mux_2 : _GEN_7; // @[WidthWidget.scala 134:{53,53}]
  wire  hasData = auto_out_d_bits_opcode[0]; // @[Edges.scala 105:36]
  wire [11:0] _limit_T_1 = 12'h1f << auto_out_d_bits_size; // @[package.scala 234:77]
  wire [4:0] _limit_T_3 = ~_limit_T_1[4:0]; // @[package.scala 234:46]
  wire [1:0] limit = _limit_T_3[4:3]; // @[WidthWidget.scala 32:47]
  reg [1:0] count; // @[WidthWidget.scala 34:27]
  wire  last = count == limit | ~hasData; // @[WidthWidget.scala 36:36]
  wire [1:0] _enable_T_1 = count & limit; // @[WidthWidget.scala 37:63]
  wire  enable_0 = ~(|_enable_T_1); // @[WidthWidget.scala 37:47]
  wire [1:0] _enable_T_3 = count ^ 2'h1; // @[WidthWidget.scala 37:56]
  wire [1:0] _enable_T_4 = _enable_T_3 & limit; // @[WidthWidget.scala 37:63]
  wire  enable_1 = ~(|_enable_T_4); // @[WidthWidget.scala 37:47]
  wire [1:0] _enable_T_6 = count ^ 2'h2; // @[WidthWidget.scala 37:56]
  wire [1:0] _enable_T_7 = _enable_T_6 & limit; // @[WidthWidget.scala 37:63]
  wire  enable_2 = ~(|_enable_T_7); // @[WidthWidget.scala 37:47]
  reg  corrupt_reg; // @[WidthWidget.scala 39:32]
  wire  corrupt_out = auto_out_d_bits_corrupt | corrupt_reg; // @[WidthWidget.scala 41:36]
  wire  _bundleOut_0_d_ready_T = ~last; // @[WidthWidget.scala 70:32]
  wire  bundleOut_0_d_ready = auto_in_d_ready | ~last; // @[WidthWidget.scala 70:29]
  wire  _T = bundleOut_0_d_ready & auto_out_d_valid; // @[Decoupled.scala 50:35]
  wire [1:0] _count_T_1 = count + 2'h1; // @[WidthWidget.scala 44:24]
  reg  bundleIn_0_d_bits_data_rdata_written_once; // @[WidthWidget.scala 56:41]
  wire  bundleIn_0_d_bits_data_masked_enable_0 = enable_0 | ~bundleIn_0_d_bits_data_rdata_written_once; // @[WidthWidget.scala 57:42]
  wire  bundleIn_0_d_bits_data_masked_enable_1 = enable_1 | ~bundleIn_0_d_bits_data_rdata_written_once; // @[WidthWidget.scala 57:42]
  wire  bundleIn_0_d_bits_data_masked_enable_2 = enable_2 | ~bundleIn_0_d_bits_data_rdata_written_once; // @[WidthWidget.scala 57:42]
  reg [63:0] bundleIn_0_d_bits_data_rdata_0; // @[WidthWidget.scala 60:24]
  reg [63:0] bundleIn_0_d_bits_data_rdata_1; // @[WidthWidget.scala 60:24]
  reg [63:0] bundleIn_0_d_bits_data_rdata_2; // @[WidthWidget.scala 60:24]
  wire [63:0] bundleIn_0_d_bits_data_mdata_0 = bundleIn_0_d_bits_data_masked_enable_0 ? auto_out_d_bits_data :
    bundleIn_0_d_bits_data_rdata_0; // @[WidthWidget.scala 62:88]
  wire [63:0] bundleIn_0_d_bits_data_mdata_1 = bundleIn_0_d_bits_data_masked_enable_1 ? auto_out_d_bits_data :
    bundleIn_0_d_bits_data_rdata_1; // @[WidthWidget.scala 62:88]
  wire [63:0] bundleIn_0_d_bits_data_mdata_2 = bundleIn_0_d_bits_data_masked_enable_2 ? auto_out_d_bits_data :
    bundleIn_0_d_bits_data_rdata_2; // @[WidthWidget.scala 62:88]
  wire  _GEN_14 = _T & _bundleOut_0_d_ready_T | bundleIn_0_d_bits_data_rdata_written_once; // @[WidthWidget.scala 63:35 64:30 56:41]
  wire [127:0] bundleIn_0_d_bits_data_lo = {bundleIn_0_d_bits_data_mdata_1,bundleIn_0_d_bits_data_mdata_0}; // @[Cat.scala 31:58]
  wire [127:0] bundleIn_0_d_bits_data_hi = {auto_out_d_bits_data,bundleIn_0_d_bits_data_mdata_2}; // @[Cat.scala 31:58]
  Repeater_2 repeated_repeater ( // @[Repeater.scala 35:26]
    .clock(repeated_repeater_clock),
    .reset(repeated_repeater_reset),
    .io_repeat(repeated_repeater_io_repeat),
    .io_enq_ready(repeated_repeater_io_enq_ready),
    .io_enq_valid(repeated_repeater_io_enq_valid),
    .io_enq_bits_opcode(repeated_repeater_io_enq_bits_opcode),
    .io_enq_bits_size(repeated_repeater_io_enq_bits_size),
    .io_enq_bits_source(repeated_repeater_io_enq_bits_source),
    .io_enq_bits_address(repeated_repeater_io_enq_bits_address),
    .io_enq_bits_user_amba_prot_bufferable(repeated_repeater_io_enq_bits_user_amba_prot_bufferable),
    .io_enq_bits_user_amba_prot_modifiable(repeated_repeater_io_enq_bits_user_amba_prot_modifiable),
    .io_enq_bits_user_amba_prot_readalloc(repeated_repeater_io_enq_bits_user_amba_prot_readalloc),
    .io_enq_bits_user_amba_prot_writealloc(repeated_repeater_io_enq_bits_user_amba_prot_writealloc),
    .io_enq_bits_user_amba_prot_privileged(repeated_repeater_io_enq_bits_user_amba_prot_privileged),
    .io_enq_bits_user_amba_prot_secure(repeated_repeater_io_enq_bits_user_amba_prot_secure),
    .io_enq_bits_user_amba_prot_fetch(repeated_repeater_io_enq_bits_user_amba_prot_fetch),
    .io_enq_bits_mask(repeated_repeater_io_enq_bits_mask),
    .io_enq_bits_data(repeated_repeater_io_enq_bits_data),
    .io_deq_ready(repeated_repeater_io_deq_ready),
    .io_deq_valid(repeated_repeater_io_deq_valid),
    .io_deq_bits_opcode(repeated_repeater_io_deq_bits_opcode),
    .io_deq_bits_size(repeated_repeater_io_deq_bits_size),
    .io_deq_bits_source(repeated_repeater_io_deq_bits_source),
    .io_deq_bits_address(repeated_repeater_io_deq_bits_address),
    .io_deq_bits_user_amba_prot_bufferable(repeated_repeater_io_deq_bits_user_amba_prot_bufferable),
    .io_deq_bits_user_amba_prot_modifiable(repeated_repeater_io_deq_bits_user_amba_prot_modifiable),
    .io_deq_bits_user_amba_prot_readalloc(repeated_repeater_io_deq_bits_user_amba_prot_readalloc),
    .io_deq_bits_user_amba_prot_writealloc(repeated_repeater_io_deq_bits_user_amba_prot_writealloc),
    .io_deq_bits_user_amba_prot_privileged(repeated_repeater_io_deq_bits_user_amba_prot_privileged),
    .io_deq_bits_user_amba_prot_secure(repeated_repeater_io_deq_bits_user_amba_prot_secure),
    .io_deq_bits_user_amba_prot_fetch(repeated_repeater_io_deq_bits_user_amba_prot_fetch),
    .io_deq_bits_mask(repeated_repeater_io_deq_bits_mask),
    .io_deq_bits_data(repeated_repeater_io_deq_bits_data)
  );
  assign auto_in_a_ready = repeated_repeater_io_enq_ready; // @[Nodes.scala 1210:84 Repeater.scala 37:21]
  assign auto_in_d_valid = auto_out_d_valid & last; // @[WidthWidget.scala 71:29]
  assign auto_in_d_bits_opcode = auto_out_d_bits_opcode; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_size = auto_out_d_bits_size; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_source = auto_out_d_bits_source; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_denied = auto_out_d_bits_denied; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign auto_in_d_bits_data = {bundleIn_0_d_bits_data_hi,bundleIn_0_d_bits_data_lo}; // @[Cat.scala 31:58]
  assign auto_in_d_bits_corrupt = auto_out_d_bits_corrupt | corrupt_reg; // @[WidthWidget.scala 41:36]
  assign auto_out_a_valid = repeated_repeater_io_deq_valid; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_opcode = repeated_repeater_io_deq_bits_opcode; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_size = repeated_repeater_io_deq_bits_size; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_source = repeated_repeater_io_deq_bits_source; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_address = repeated_repeater_io_deq_bits_address; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_user_amba_prot_bufferable = repeated_repeater_io_deq_bits_user_amba_prot_bufferable; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_user_amba_prot_modifiable = repeated_repeater_io_deq_bits_user_amba_prot_modifiable; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_user_amba_prot_readalloc = repeated_repeater_io_deq_bits_user_amba_prot_readalloc; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_user_amba_prot_writealloc = repeated_repeater_io_deq_bits_user_amba_prot_writealloc; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_user_amba_prot_privileged = repeated_repeater_io_deq_bits_user_amba_prot_privileged; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_user_amba_prot_secure = repeated_repeater_io_deq_bits_user_amba_prot_secure; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_user_amba_prot_fetch = repeated_repeater_io_deq_bits_user_amba_prot_fetch; // @[WidthWidget.scala 155:25 156:15]
  assign auto_out_a_bits_mask = 2'h3 == repeat_index ? repeat_bundleOut_0_a_bits_mask_mux_3 : _GEN_8; // @[WidthWidget.scala 134:{53,53}]
  assign auto_out_a_bits_data = 2'h3 == repeat_index ? repeat_bundleOut_0_a_bits_data_mux_3 : _GEN_4; // @[WidthWidget.scala 131:{30,30}]
  assign auto_out_d_ready = auto_in_d_ready | ~last; // @[WidthWidget.scala 70:29]
  assign repeated_repeater_clock = clock;
  assign repeated_repeater_reset = reset;
  assign repeated_repeater_io_repeat = ~repeat_last; // @[WidthWidget.scala 142:7]
  assign repeated_repeater_io_enq_valid = auto_in_a_valid; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_opcode = auto_in_a_bits_opcode; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_size = auto_in_a_bits_size; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_source = auto_in_a_bits_source; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_address = auto_in_a_bits_address; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_user_amba_prot_bufferable = auto_in_a_bits_user_amba_prot_bufferable; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_user_amba_prot_modifiable = auto_in_a_bits_user_amba_prot_modifiable; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_user_amba_prot_readalloc = auto_in_a_bits_user_amba_prot_readalloc; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_user_amba_prot_writealloc = auto_in_a_bits_user_amba_prot_writealloc; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_user_amba_prot_privileged = auto_in_a_bits_user_amba_prot_privileged; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_user_amba_prot_secure = auto_in_a_bits_user_amba_prot_secure; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_user_amba_prot_fetch = auto_in_a_bits_user_amba_prot_fetch; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_mask = auto_in_a_bits_mask; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_enq_bits_data = auto_in_a_bits_data; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign repeated_repeater_io_deq_ready = auto_out_a_ready; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  always @(posedge clock) begin
    if (reset) begin // @[WidthWidget.scala 99:26]
      repeat_count <= 2'h0; // @[WidthWidget.scala 99:26]
    end else if (_repeat_T) begin // @[WidthWidget.scala 103:25]
      if (repeat_last) begin // @[WidthWidget.scala 105:21]
        repeat_count <= 2'h0; // @[WidthWidget.scala 105:29]
      end else begin
        repeat_count <= _repeat_count_T_1; // @[WidthWidget.scala 104:15]
      end
    end
    if (reset) begin // @[WidthWidget.scala 34:27]
      count <= 2'h0; // @[WidthWidget.scala 34:27]
    end else if (_T) begin // @[WidthWidget.scala 43:24]
      if (last) begin // @[WidthWidget.scala 46:21]
        count <= 2'h0; // @[WidthWidget.scala 47:17]
      end else begin
        count <= _count_T_1; // @[WidthWidget.scala 44:15]
      end
    end
    if (reset) begin // @[WidthWidget.scala 39:32]
      corrupt_reg <= 1'h0; // @[WidthWidget.scala 39:32]
    end else if (_T) begin // @[WidthWidget.scala 43:24]
      if (last) begin // @[WidthWidget.scala 46:21]
        corrupt_reg <= 1'h0; // @[WidthWidget.scala 48:23]
      end else begin
        corrupt_reg <= corrupt_out; // @[WidthWidget.scala 45:21]
      end
    end
    if (reset) begin // @[WidthWidget.scala 56:41]
      bundleIn_0_d_bits_data_rdata_written_once <= 1'h0; // @[WidthWidget.scala 56:41]
    end else begin
      bundleIn_0_d_bits_data_rdata_written_once <= _GEN_14;
    end
    if (_T & _bundleOut_0_d_ready_T) begin // @[WidthWidget.scala 63:35]
      if (bundleIn_0_d_bits_data_masked_enable_0) begin // @[WidthWidget.scala 62:88]
        bundleIn_0_d_bits_data_rdata_0 <= auto_out_d_bits_data;
      end
    end
    if (_T & _bundleOut_0_d_ready_T) begin // @[WidthWidget.scala 63:35]
      if (bundleIn_0_d_bits_data_masked_enable_1) begin // @[WidthWidget.scala 62:88]
        bundleIn_0_d_bits_data_rdata_1 <= auto_out_d_bits_data;
      end
    end
    if (_T & _bundleOut_0_d_ready_T) begin // @[WidthWidget.scala 63:35]
      if (bundleIn_0_d_bits_data_masked_enable_2) begin // @[WidthWidget.scala 62:88]
        bundleIn_0_d_bits_data_rdata_2 <= auto_out_d_bits_data;
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
  repeat_count = _RAND_0[1:0];
  _RAND_1 = {1{`RANDOM}};
  count = _RAND_1[1:0];
  _RAND_2 = {1{`RANDOM}};
  corrupt_reg = _RAND_2[0:0];
  _RAND_3 = {1{`RANDOM}};
  bundleIn_0_d_bits_data_rdata_written_once = _RAND_3[0:0];
  _RAND_4 = {2{`RANDOM}};
  bundleIn_0_d_bits_data_rdata_0 = _RAND_4[63:0];
  _RAND_5 = {2{`RANDOM}};
  bundleIn_0_d_bits_data_rdata_1 = _RAND_5[63:0];
  _RAND_6 = {2{`RANDOM}};
  bundleIn_0_d_bits_data_rdata_2 = _RAND_6[63:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

