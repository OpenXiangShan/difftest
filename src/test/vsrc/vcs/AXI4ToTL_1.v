module AXI4ToTL_1(
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
  output [31:0]  auto_out_a_bits_mask,
  output [255:0] auto_out_a_bits_data,
  output         auto_out_d_ready,
  input          auto_out_d_valid,
  input  [2:0]   auto_out_d_bits_opcode,
  input  [2:0]   auto_out_d_bits_size,
  input  [1:0]   auto_out_d_bits_source,
  input          auto_out_d_bits_denied,
  input  [255:0] auto_out_d_bits_data,
  input          auto_out_d_bits_corrupt
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
  reg [31:0] _RAND_10;
  reg [31:0] _RAND_11;
  reg [31:0] _RAND_12;
  reg [31:0] _RAND_13;
  reg [31:0] _RAND_14;
  reg [31:0] _RAND_15;
  reg [31:0] _RAND_16;
  reg [31:0] _RAND_17;
  reg [31:0] _RAND_18;
  reg [31:0] _RAND_19;
  reg [31:0] _RAND_20;
  reg [31:0] _RAND_21;
  reg [31:0] _RAND_22;
  reg [31:0] _RAND_23;
  reg [31:0] _RAND_24;
  reg [31:0] _RAND_25;
  reg [31:0] _RAND_26;
  reg [31:0] _RAND_27;
  reg [31:0] _RAND_28;
  reg [31:0] _RAND_29;
  reg [31:0] _RAND_30;
  reg [31:0] _RAND_31;
  reg [31:0] _RAND_32;
  reg [31:0] _RAND_33;
  reg [31:0] _RAND_34;
  reg [31:0] _RAND_35;
  reg [31:0] _RAND_36;
  reg [31:0] _RAND_37;
  reg [31:0] _RAND_38;
  reg [31:0] _RAND_39;
  reg [31:0] _RAND_40;
  reg [31:0] _RAND_41;
  reg [31:0] _RAND_42;
  reg [31:0] _RAND_43;
  reg [31:0] _RAND_44;
  reg [31:0] _RAND_45;
  reg [31:0] _RAND_46;
  reg [31:0] _RAND_47;
  reg [31:0] _RAND_48;
  reg [31:0] _RAND_49;
  reg [31:0] _RAND_50;
  reg [31:0] _RAND_51;
  reg [31:0] _RAND_52;
  reg [31:0] _RAND_53;
  reg [31:0] _RAND_54;
  reg [31:0] _RAND_55;
  reg [31:0] _RAND_56;
  reg [31:0] _RAND_57;
  reg [31:0] _RAND_58;
  reg [31:0] _RAND_59;
  reg [31:0] _RAND_60;
  reg [31:0] _RAND_61;
  reg [31:0] _RAND_62;
  reg [31:0] _RAND_63;
  reg [31:0] _RAND_64;
  reg [31:0] _RAND_65;
  reg [31:0] _RAND_66;
  reg [31:0] _RAND_67;
  reg [31:0] _RAND_68;
`endif // RANDOMIZE_REG_INIT
  wire  deq_clock; // @[Decoupled.scala 361:21]
  wire  deq_reset; // @[Decoupled.scala 361:21]
  wire  deq_io_enq_ready; // @[Decoupled.scala 361:21]
  wire  deq_io_enq_valid; // @[Decoupled.scala 361:21]
  wire [4:0] deq_io_enq_bits_id; // @[Decoupled.scala 361:21]
  wire [255:0] deq_io_enq_bits_data; // @[Decoupled.scala 361:21]
  wire [1:0] deq_io_enq_bits_resp; // @[Decoupled.scala 361:21]
  wire  deq_io_enq_bits_last; // @[Decoupled.scala 361:21]
  wire  deq_io_deq_ready; // @[Decoupled.scala 361:21]
  wire  deq_io_deq_valid; // @[Decoupled.scala 361:21]
  wire [4:0] deq_io_deq_bits_id; // @[Decoupled.scala 361:21]
  wire [255:0] deq_io_deq_bits_data; // @[Decoupled.scala 361:21]
  wire [1:0] deq_io_deq_bits_resp; // @[Decoupled.scala 361:21]
  wire  deq_io_deq_bits_last; // @[Decoupled.scala 361:21]
  wire  q_bdeq_clock; // @[Decoupled.scala 361:21]
  wire  q_bdeq_reset; // @[Decoupled.scala 361:21]
  wire  q_bdeq_io_enq_ready; // @[Decoupled.scala 361:21]
  wire  q_bdeq_io_enq_valid; // @[Decoupled.scala 361:21]
  wire [4:0] q_bdeq_io_enq_bits_id; // @[Decoupled.scala 361:21]
  wire [1:0] q_bdeq_io_enq_bits_resp; // @[Decoupled.scala 361:21]
  wire  q_bdeq_io_deq_ready; // @[Decoupled.scala 361:21]
  wire  q_bdeq_io_deq_valid; // @[Decoupled.scala 361:21]
  wire [4:0] q_bdeq_io_deq_bits_id; // @[Decoupled.scala 361:21]
  wire [1:0] q_bdeq_io_deq_bits_resp; // @[Decoupled.scala 361:21]
  wire [15:0] _rsize1_T = {auto_in_arlen,8'hff}; // @[Cat.scala 31:58]
  wire [22:0] _GEN_0 = {{7'd0}, _rsize1_T}; // @[Bundles.scala 31:21]
  wire [22:0] _rsize1_T_1 = _GEN_0 << auto_in_arsize; // @[Bundles.scala 31:21]
  wire [14:0] r_size1 = _rsize1_T_1[22:8]; // @[Bundles.scala 31:30]
  wire [15:0] _rsize_T = {r_size1, 1'h0}; // @[package.scala 232:35]
  wire [15:0] _rsize_T_1 = _rsize_T | 16'h1; // @[package.scala 232:40]
  wire [15:0] _rsize_T_2 = {1'h0,r_size1}; // @[Cat.scala 31:58]
  wire [15:0] _rsize_T_3 = ~_rsize_T_2; // @[package.scala 232:53]
  wire [15:0] _rsize_T_4 = _rsize_T_1 & _rsize_T_3; // @[package.scala 232:51]
  wire [7:0] r_size_hi = _rsize_T_4[15:8]; // @[OneHot.scala 30:18]
  wire [7:0] r_size_lo = _rsize_T_4[7:0]; // @[OneHot.scala 31:18]
  wire  _rsize_T_5 = |r_size_hi; // @[OneHot.scala 32:14]
  wire [7:0] _rsize_T_6 = r_size_hi | r_size_lo; // @[OneHot.scala 32:28]
  wire [3:0] r_size_hi_1 = _rsize_T_6[7:4]; // @[OneHot.scala 30:18]
  wire [3:0] r_size_lo_1 = _rsize_T_6[3:0]; // @[OneHot.scala 31:18]
  wire  _rsize_T_7 = |r_size_hi_1; // @[OneHot.scala 32:14]
  wire [3:0] _rsize_T_8 = r_size_hi_1 | r_size_lo_1; // @[OneHot.scala 32:28]
  wire [1:0] r_size_hi_2 = _rsize_T_8[3:2]; // @[OneHot.scala 30:18]
  wire [1:0] r_size_lo_2 = _rsize_T_8[1:0]; // @[OneHot.scala 31:18]
  wire  _rsize_T_9 = |r_size_hi_2; // @[OneHot.scala 32:14]
  wire [1:0] _rsize_T_10 = r_size_hi_2 | r_size_lo_2; // @[OneHot.scala 32:28]
  wire [3:0] r_size = {_rsize_T_5,_rsize_T_7,_rsize_T_9,_rsize_T_10[1]}; // @[Cat.scala 31:58]
  wire  _rok_T_1 = r_size <= 4'h7; // @[Parameters.scala 92:42]
  wire [38:0] _rok_T_5 = {1'b0,$signed(auto_in_araddr)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_7 = $signed(_rok_T_5) & -39'sh1000000000; // @[Parameters.scala 137:52]
  wire  _rok_T_8 = $signed(_rok_T_7) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_9 = auto_in_araddr ^ 38'h1000000000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_10 = {1'b0,$signed(_rok_T_9)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_12 = $signed(_rok_T_10) & -39'sh800000000; // @[Parameters.scala 137:52]
  wire  _rok_T_13 = $signed(_rok_T_12) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_14 = auto_in_araddr ^ 38'h1800000000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_15 = {1'b0,$signed(_rok_T_14)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_17 = $signed(_rok_T_15) & -39'sh400000000; // @[Parameters.scala 137:52]
  wire  _rok_T_18 = $signed(_rok_T_17) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_19 = auto_in_araddr ^ 38'h1c00000000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_20 = {1'b0,$signed(_rok_T_19)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_22 = $signed(_rok_T_20) & -39'sh200000000; // @[Parameters.scala 137:52]
  wire  _rok_T_23 = $signed(_rok_T_22) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_24 = auto_in_araddr ^ 38'h1e00000000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_25 = {1'b0,$signed(_rok_T_24)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_27 = $signed(_rok_T_25) & -39'sh100000000; // @[Parameters.scala 137:52]
  wire  _rok_T_28 = $signed(_rok_T_27) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_29 = auto_in_araddr ^ 38'h1f50000000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_30 = {1'b0,$signed(_rok_T_29)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_32 = $signed(_rok_T_30) & -39'sh400000; // @[Parameters.scala 137:52]
  wire  _rok_T_33 = $signed(_rok_T_32) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_34 = auto_in_araddr ^ 38'h2000000000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_35 = {1'b0,$signed(_rok_T_34)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_37 = $signed(_rok_T_35) & -39'sh2000000000; // @[Parameters.scala 137:52]
  wire  _rok_T_38 = $signed(_rok_T_37) == 39'sh0; // @[Parameters.scala 137:67]
  wire  _rok_T_44 = _rok_T_8 | _rok_T_13 | _rok_T_18 | _rok_T_23 | _rok_T_28 | _rok_T_33 | _rok_T_38; // @[Parameters.scala 671:42]
  wire  _rok_T_45 = _rok_T_1 & _rok_T_44; // @[Parameters.scala 670:56]
  wire  _rok_T_47 = r_size <= 4'h3; // @[Parameters.scala 92:42]
  wire [37:0] _rok_T_50 = auto_in_araddr ^ 38'h1f00050000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_51 = {1'b0,$signed(_rok_T_50)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_53 = $signed(_rok_T_51) & -39'sh10; // @[Parameters.scala 137:52]
  wire  _rok_T_54 = $signed(_rok_T_53) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_55 = auto_in_araddr ^ 38'h1f00060000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_56 = {1'b0,$signed(_rok_T_55)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_58 = $signed(_rok_T_56) & -39'sh20000; // @[Parameters.scala 137:52]
  wire  _rok_T_59 = $signed(_rok_T_58) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_60 = auto_in_araddr ^ 38'h1f40001000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_61 = {1'b0,$signed(_rok_T_60)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_63 = $signed(_rok_T_61) & -39'sh8; // @[Parameters.scala 137:52]
  wire  _rok_T_64 = $signed(_rok_T_63) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_65 = auto_in_araddr ^ 38'h1f40002000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_66 = {1'b0,$signed(_rok_T_65)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_68 = $signed(_rok_T_66) & -39'sh1000; // @[Parameters.scala 137:52]
  wire  _rok_T_69 = $signed(_rok_T_68) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_70 = auto_in_araddr ^ 38'h1f80000000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_71 = {1'b0,$signed(_rok_T_70)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_73 = $signed(_rok_T_71) & -39'sh40000000; // @[Parameters.scala 137:52]
  wire  _rok_T_74 = $signed(_rok_T_73) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_75 = auto_in_araddr ^ 38'h1fe2000000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_76 = {1'b0,$signed(_rok_T_75)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_78 = $signed(_rok_T_76) & -39'sh200000; // @[Parameters.scala 137:52]
  wire  _rok_T_79 = $signed(_rok_T_78) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _rok_T_80 = auto_in_araddr ^ 38'h1ffff80000; // @[Parameters.scala 137:31]
  wire [38:0] _rok_T_81 = {1'b0,$signed(_rok_T_80)}; // @[Parameters.scala 137:49]
  wire [38:0] _rok_T_83 = $signed(_rok_T_81) & -39'sh40000; // @[Parameters.scala 137:52]
  wire  _rok_T_84 = $signed(_rok_T_83) == 39'sh0; // @[Parameters.scala 137:67]
  wire  _rok_T_90 = _rok_T_54 | _rok_T_59 | _rok_T_64 | _rok_T_69 | _rok_T_74 | _rok_T_79 | _rok_T_84; // @[Parameters.scala 671:42]
  wire  _rok_T_91 = _rok_T_47 & _rok_T_90; // @[Parameters.scala 670:56]
  wire  r_ok = _rok_T_45 | _rok_T_91; // @[Parameters.scala 672:30]
  wire [36:0] _GEN_162 = {{32'd0}, auto_in_araddr[4:0]}; // @[ToTL.scala 90:59]
  wire [36:0] _raddr_T_1 = 37'h1e00000000 | _GEN_162; // @[ToTL.scala 90:59]
  wire [37:0] r_addr = r_ok ? auto_in_araddr : {{1'd0}, _raddr_T_1}; // @[ToTL.scala 90:23]
  wire [5:0] r_id = {auto_in_arid,1'h0}; // @[Cat.scala 31:58]
  wire [4:0] _a_mask_sizeOH_T = {{1'd0}, r_size}; // @[Misc.scala 201:34]
  wire [2:0] a_mask_sizeOH_shiftAmount = _a_mask_sizeOH_T[2:0]; // @[OneHot.scala 63:49]
  wire [7:0] _a_mask_sizeOH_T_1 = 8'h1 << a_mask_sizeOH_shiftAmount; // @[OneHot.scala 64:12]
  wire [4:0] a_mask_sizeOH = _a_mask_sizeOH_T_1[4:0] | 5'h1; // @[Misc.scala 201:81]
  wire  _a_mask_T = r_size >= 4'h5; // @[Misc.scala 205:21]
  wire  a_mask_size = a_mask_sizeOH[4]; // @[Misc.scala 208:26]
  wire  a_mask_bit = r_addr[4]; // @[Misc.scala 209:26]
  wire  a_mask_nbit = ~a_mask_bit; // @[Misc.scala 210:20]
  wire  a_mask_acc = _a_mask_T | a_mask_size & a_mask_nbit; // @[Misc.scala 214:29]
  wire  a_mask_acc_1 = _a_mask_T | a_mask_size & a_mask_bit; // @[Misc.scala 214:29]
  wire  a_mask_size_1 = a_mask_sizeOH[3]; // @[Misc.scala 208:26]
  wire  a_mask_bit_1 = r_addr[3]; // @[Misc.scala 209:26]
  wire  a_mask_nbit_1 = ~a_mask_bit_1; // @[Misc.scala 210:20]
  wire  a_mask_eq_2 = a_mask_nbit & a_mask_nbit_1; // @[Misc.scala 213:27]
  wire  a_mask_acc_2 = a_mask_acc | a_mask_size_1 & a_mask_eq_2; // @[Misc.scala 214:29]
  wire  a_mask_eq_3 = a_mask_nbit & a_mask_bit_1; // @[Misc.scala 213:27]
  wire  a_mask_acc_3 = a_mask_acc | a_mask_size_1 & a_mask_eq_3; // @[Misc.scala 214:29]
  wire  a_mask_eq_4 = a_mask_bit & a_mask_nbit_1; // @[Misc.scala 213:27]
  wire  a_mask_acc_4 = a_mask_acc_1 | a_mask_size_1 & a_mask_eq_4; // @[Misc.scala 214:29]
  wire  a_mask_eq_5 = a_mask_bit & a_mask_bit_1; // @[Misc.scala 213:27]
  wire  a_mask_acc_5 = a_mask_acc_1 | a_mask_size_1 & a_mask_eq_5; // @[Misc.scala 214:29]
  wire  a_mask_size_2 = a_mask_sizeOH[2]; // @[Misc.scala 208:26]
  wire  a_mask_bit_2 = r_addr[2]; // @[Misc.scala 209:26]
  wire  a_mask_nbit_2 = ~a_mask_bit_2; // @[Misc.scala 210:20]
  wire  a_mask_eq_6 = a_mask_eq_2 & a_mask_nbit_2; // @[Misc.scala 213:27]
  wire  a_mask_acc_6 = a_mask_acc_2 | a_mask_size_2 & a_mask_eq_6; // @[Misc.scala 214:29]
  wire  a_mask_eq_7 = a_mask_eq_2 & a_mask_bit_2; // @[Misc.scala 213:27]
  wire  a_mask_acc_7 = a_mask_acc_2 | a_mask_size_2 & a_mask_eq_7; // @[Misc.scala 214:29]
  wire  a_mask_eq_8 = a_mask_eq_3 & a_mask_nbit_2; // @[Misc.scala 213:27]
  wire  a_mask_acc_8 = a_mask_acc_3 | a_mask_size_2 & a_mask_eq_8; // @[Misc.scala 214:29]
  wire  a_mask_eq_9 = a_mask_eq_3 & a_mask_bit_2; // @[Misc.scala 213:27]
  wire  a_mask_acc_9 = a_mask_acc_3 | a_mask_size_2 & a_mask_eq_9; // @[Misc.scala 214:29]
  wire  a_mask_eq_10 = a_mask_eq_4 & a_mask_nbit_2; // @[Misc.scala 213:27]
  wire  a_mask_acc_10 = a_mask_acc_4 | a_mask_size_2 & a_mask_eq_10; // @[Misc.scala 214:29]
  wire  a_mask_eq_11 = a_mask_eq_4 & a_mask_bit_2; // @[Misc.scala 213:27]
  wire  a_mask_acc_11 = a_mask_acc_4 | a_mask_size_2 & a_mask_eq_11; // @[Misc.scala 214:29]
  wire  a_mask_eq_12 = a_mask_eq_5 & a_mask_nbit_2; // @[Misc.scala 213:27]
  wire  a_mask_acc_12 = a_mask_acc_5 | a_mask_size_2 & a_mask_eq_12; // @[Misc.scala 214:29]
  wire  a_mask_eq_13 = a_mask_eq_5 & a_mask_bit_2; // @[Misc.scala 213:27]
  wire  a_mask_acc_13 = a_mask_acc_5 | a_mask_size_2 & a_mask_eq_13; // @[Misc.scala 214:29]
  wire  a_mask_size_3 = a_mask_sizeOH[1]; // @[Misc.scala 208:26]
  wire  a_mask_bit_3 = r_addr[1]; // @[Misc.scala 209:26]
  wire  a_mask_nbit_3 = ~a_mask_bit_3; // @[Misc.scala 210:20]
  wire  a_mask_eq_14 = a_mask_eq_6 & a_mask_nbit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_14 = a_mask_acc_6 | a_mask_size_3 & a_mask_eq_14; // @[Misc.scala 214:29]
  wire  a_mask_eq_15 = a_mask_eq_6 & a_mask_bit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_15 = a_mask_acc_6 | a_mask_size_3 & a_mask_eq_15; // @[Misc.scala 214:29]
  wire  a_mask_eq_16 = a_mask_eq_7 & a_mask_nbit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_16 = a_mask_acc_7 | a_mask_size_3 & a_mask_eq_16; // @[Misc.scala 214:29]
  wire  a_mask_eq_17 = a_mask_eq_7 & a_mask_bit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_17 = a_mask_acc_7 | a_mask_size_3 & a_mask_eq_17; // @[Misc.scala 214:29]
  wire  a_mask_eq_18 = a_mask_eq_8 & a_mask_nbit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_18 = a_mask_acc_8 | a_mask_size_3 & a_mask_eq_18; // @[Misc.scala 214:29]
  wire  a_mask_eq_19 = a_mask_eq_8 & a_mask_bit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_19 = a_mask_acc_8 | a_mask_size_3 & a_mask_eq_19; // @[Misc.scala 214:29]
  wire  a_mask_eq_20 = a_mask_eq_9 & a_mask_nbit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_20 = a_mask_acc_9 | a_mask_size_3 & a_mask_eq_20; // @[Misc.scala 214:29]
  wire  a_mask_eq_21 = a_mask_eq_9 & a_mask_bit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_21 = a_mask_acc_9 | a_mask_size_3 & a_mask_eq_21; // @[Misc.scala 214:29]
  wire  a_mask_eq_22 = a_mask_eq_10 & a_mask_nbit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_22 = a_mask_acc_10 | a_mask_size_3 & a_mask_eq_22; // @[Misc.scala 214:29]
  wire  a_mask_eq_23 = a_mask_eq_10 & a_mask_bit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_23 = a_mask_acc_10 | a_mask_size_3 & a_mask_eq_23; // @[Misc.scala 214:29]
  wire  a_mask_eq_24 = a_mask_eq_11 & a_mask_nbit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_24 = a_mask_acc_11 | a_mask_size_3 & a_mask_eq_24; // @[Misc.scala 214:29]
  wire  a_mask_eq_25 = a_mask_eq_11 & a_mask_bit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_25 = a_mask_acc_11 | a_mask_size_3 & a_mask_eq_25; // @[Misc.scala 214:29]
  wire  a_mask_eq_26 = a_mask_eq_12 & a_mask_nbit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_26 = a_mask_acc_12 | a_mask_size_3 & a_mask_eq_26; // @[Misc.scala 214:29]
  wire  a_mask_eq_27 = a_mask_eq_12 & a_mask_bit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_27 = a_mask_acc_12 | a_mask_size_3 & a_mask_eq_27; // @[Misc.scala 214:29]
  wire  a_mask_eq_28 = a_mask_eq_13 & a_mask_nbit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_28 = a_mask_acc_13 | a_mask_size_3 & a_mask_eq_28; // @[Misc.scala 214:29]
  wire  a_mask_eq_29 = a_mask_eq_13 & a_mask_bit_3; // @[Misc.scala 213:27]
  wire  a_mask_acc_29 = a_mask_acc_13 | a_mask_size_3 & a_mask_eq_29; // @[Misc.scala 214:29]
  wire  a_mask_size_4 = a_mask_sizeOH[0]; // @[Misc.scala 208:26]
  wire  a_mask_bit_4 = r_addr[0]; // @[Misc.scala 209:26]
  wire  a_mask_nbit_4 = ~a_mask_bit_4; // @[Misc.scala 210:20]
  wire  a_mask_eq_30 = a_mask_eq_14 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_30 = a_mask_acc_14 | a_mask_size_4 & a_mask_eq_30; // @[Misc.scala 214:29]
  wire  a_mask_eq_31 = a_mask_eq_14 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_31 = a_mask_acc_14 | a_mask_size_4 & a_mask_eq_31; // @[Misc.scala 214:29]
  wire  a_mask_eq_32 = a_mask_eq_15 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_32 = a_mask_acc_15 | a_mask_size_4 & a_mask_eq_32; // @[Misc.scala 214:29]
  wire  a_mask_eq_33 = a_mask_eq_15 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_33 = a_mask_acc_15 | a_mask_size_4 & a_mask_eq_33; // @[Misc.scala 214:29]
  wire  a_mask_eq_34 = a_mask_eq_16 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_34 = a_mask_acc_16 | a_mask_size_4 & a_mask_eq_34; // @[Misc.scala 214:29]
  wire  a_mask_eq_35 = a_mask_eq_16 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_35 = a_mask_acc_16 | a_mask_size_4 & a_mask_eq_35; // @[Misc.scala 214:29]
  wire  a_mask_eq_36 = a_mask_eq_17 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_36 = a_mask_acc_17 | a_mask_size_4 & a_mask_eq_36; // @[Misc.scala 214:29]
  wire  a_mask_eq_37 = a_mask_eq_17 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_37 = a_mask_acc_17 | a_mask_size_4 & a_mask_eq_37; // @[Misc.scala 214:29]
  wire  a_mask_eq_38 = a_mask_eq_18 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_38 = a_mask_acc_18 | a_mask_size_4 & a_mask_eq_38; // @[Misc.scala 214:29]
  wire  a_mask_eq_39 = a_mask_eq_18 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_39 = a_mask_acc_18 | a_mask_size_4 & a_mask_eq_39; // @[Misc.scala 214:29]
  wire  a_mask_eq_40 = a_mask_eq_19 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_40 = a_mask_acc_19 | a_mask_size_4 & a_mask_eq_40; // @[Misc.scala 214:29]
  wire  a_mask_eq_41 = a_mask_eq_19 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_41 = a_mask_acc_19 | a_mask_size_4 & a_mask_eq_41; // @[Misc.scala 214:29]
  wire  a_mask_eq_42 = a_mask_eq_20 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_42 = a_mask_acc_20 | a_mask_size_4 & a_mask_eq_42; // @[Misc.scala 214:29]
  wire  a_mask_eq_43 = a_mask_eq_20 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_43 = a_mask_acc_20 | a_mask_size_4 & a_mask_eq_43; // @[Misc.scala 214:29]
  wire  a_mask_eq_44 = a_mask_eq_21 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_44 = a_mask_acc_21 | a_mask_size_4 & a_mask_eq_44; // @[Misc.scala 214:29]
  wire  a_mask_eq_45 = a_mask_eq_21 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_45 = a_mask_acc_21 | a_mask_size_4 & a_mask_eq_45; // @[Misc.scala 214:29]
  wire  a_mask_eq_46 = a_mask_eq_22 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_46 = a_mask_acc_22 | a_mask_size_4 & a_mask_eq_46; // @[Misc.scala 214:29]
  wire  a_mask_eq_47 = a_mask_eq_22 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_47 = a_mask_acc_22 | a_mask_size_4 & a_mask_eq_47; // @[Misc.scala 214:29]
  wire  a_mask_eq_48 = a_mask_eq_23 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_48 = a_mask_acc_23 | a_mask_size_4 & a_mask_eq_48; // @[Misc.scala 214:29]
  wire  a_mask_eq_49 = a_mask_eq_23 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_49 = a_mask_acc_23 | a_mask_size_4 & a_mask_eq_49; // @[Misc.scala 214:29]
  wire  a_mask_eq_50 = a_mask_eq_24 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_50 = a_mask_acc_24 | a_mask_size_4 & a_mask_eq_50; // @[Misc.scala 214:29]
  wire  a_mask_eq_51 = a_mask_eq_24 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_51 = a_mask_acc_24 | a_mask_size_4 & a_mask_eq_51; // @[Misc.scala 214:29]
  wire  a_mask_eq_52 = a_mask_eq_25 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_52 = a_mask_acc_25 | a_mask_size_4 & a_mask_eq_52; // @[Misc.scala 214:29]
  wire  a_mask_eq_53 = a_mask_eq_25 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_53 = a_mask_acc_25 | a_mask_size_4 & a_mask_eq_53; // @[Misc.scala 214:29]
  wire  a_mask_eq_54 = a_mask_eq_26 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_54 = a_mask_acc_26 | a_mask_size_4 & a_mask_eq_54; // @[Misc.scala 214:29]
  wire  a_mask_eq_55 = a_mask_eq_26 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_55 = a_mask_acc_26 | a_mask_size_4 & a_mask_eq_55; // @[Misc.scala 214:29]
  wire  a_mask_eq_56 = a_mask_eq_27 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_56 = a_mask_acc_27 | a_mask_size_4 & a_mask_eq_56; // @[Misc.scala 214:29]
  wire  a_mask_eq_57 = a_mask_eq_27 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_57 = a_mask_acc_27 | a_mask_size_4 & a_mask_eq_57; // @[Misc.scala 214:29]
  wire  a_mask_eq_58 = a_mask_eq_28 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_58 = a_mask_acc_28 | a_mask_size_4 & a_mask_eq_58; // @[Misc.scala 214:29]
  wire  a_mask_eq_59 = a_mask_eq_28 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_59 = a_mask_acc_28 | a_mask_size_4 & a_mask_eq_59; // @[Misc.scala 214:29]
  wire  a_mask_eq_60 = a_mask_eq_29 & a_mask_nbit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_60 = a_mask_acc_29 | a_mask_size_4 & a_mask_eq_60; // @[Misc.scala 214:29]
  wire  a_mask_eq_61 = a_mask_eq_29 & a_mask_bit_4; // @[Misc.scala 213:27]
  wire  a_mask_acc_61 = a_mask_acc_29 | a_mask_size_4 & a_mask_eq_61; // @[Misc.scala 214:29]
  wire [7:0] a_mask_lo_lo = {a_mask_acc_37,a_mask_acc_36,a_mask_acc_35,a_mask_acc_34,a_mask_acc_33,a_mask_acc_32,
    a_mask_acc_31,a_mask_acc_30}; // @[Cat.scala 31:58]
  wire [15:0] a_mask_lo = {a_mask_acc_45,a_mask_acc_44,a_mask_acc_43,a_mask_acc_42,a_mask_acc_41,a_mask_acc_40,
    a_mask_acc_39,a_mask_acc_38,a_mask_lo_lo}; // @[Cat.scala 31:58]
  wire [7:0] a_mask_hi_lo = {a_mask_acc_53,a_mask_acc_52,a_mask_acc_51,a_mask_acc_50,a_mask_acc_49,a_mask_acc_48,
    a_mask_acc_47,a_mask_acc_46}; // @[Cat.scala 31:58]
  wire [31:0] a_mask = {a_mask_acc_61,a_mask_acc_60,a_mask_acc_59,a_mask_acc_58,a_mask_acc_57,a_mask_acc_56,
    a_mask_acc_55,a_mask_acc_54,a_mask_hi_lo,a_mask_lo}; // @[Cat.scala 31:58]
  wire  r_out_bits_user_amba_prot_privileged = auto_in_arprot[0]; // @[ToTL.scala 105:45]
  wire  r_out_bits_user_amba_prot_secure = ~auto_in_arprot[1]; // @[ToTL.scala 106:29]
  wire  r_out_bits_user_amba_prot_fetch = auto_in_arprot[2]; // @[ToTL.scala 107:45]
  wire  r_out_bits_user_amba_prot_bufferable = auto_in_arcache[0]; // @[ToTL.scala 108:46]
  wire  r_out_bits_user_amba_prot_modifiable = auto_in_arcache[1]; // @[ToTL.scala 109:46]
  wire  r_out_bits_user_amba_prot_readalloc = auto_in_arcache[2]; // @[ToTL.scala 110:46]
  wire  r_out_bits_user_amba_prot_writealloc = auto_in_arcache[3]; // @[ToTL.scala 111:46]
  reg [7:0] beatsLeft; // @[Arbiter.scala 87:30]
  wire  idle = beatsLeft == 8'h0; // @[Arbiter.scala 88:28]
  wire  w_out_valid = auto_in_awvalid & auto_in_wvalid; // @[ToTL.scala 135:34]
  wire [1:0] readys_valid = {w_out_valid,auto_in_arvalid}; // @[Cat.scala 31:58]
  reg [1:0] readys_mask; // @[Arbiter.scala 23:23]
  wire [1:0] _readys_filter_T = ~readys_mask; // @[Arbiter.scala 24:30]
  wire [1:0] _readys_filter_T_1 = readys_valid & _readys_filter_T; // @[Arbiter.scala 24:28]
  wire [3:0] readys_filter = {_readys_filter_T_1,w_out_valid,auto_in_arvalid}; // @[Cat.scala 31:58]
  wire [3:0] _GEN_163 = {{1'd0}, readys_filter[3:1]}; // @[package.scala 253:43]
  wire [3:0] _readys_unready_T_1 = readys_filter | _GEN_163; // @[package.scala 253:43]
  wire [3:0] _readys_unready_T_4 = {readys_mask, 2'h0}; // @[Arbiter.scala 25:66]
  wire [3:0] _GEN_164 = {{1'd0}, _readys_unready_T_1[3:1]}; // @[Arbiter.scala 25:58]
  wire [3:0] readys_unready = _GEN_164 | _readys_unready_T_4; // @[Arbiter.scala 25:58]
  wire [1:0] _readys_readys_T_2 = readys_unready[3:2] & readys_unready[1:0]; // @[Arbiter.scala 26:39]
  wire [1:0] readys_readys = ~_readys_readys_T_2; // @[Arbiter.scala 26:18]
  wire  readys_0 = readys_readys[0]; // @[Arbiter.scala 95:86]
  reg  state_0; // @[Arbiter.scala 116:26]
  wire  allowed_0 = idle ? readys_0 : state_0; // @[Arbiter.scala 121:24]
  wire [15:0] _wsize1_T = {auto_in_awlen,8'hff}; // @[Cat.scala 31:58]
  wire [22:0] _GEN_1 = {{7'd0}, _wsize1_T}; // @[Bundles.scala 31:21]
  wire [22:0] _wsize1_T_1 = _GEN_1 << auto_in_awsize; // @[Bundles.scala 31:21]
  wire [14:0] w_size1 = _wsize1_T_1[22:8]; // @[Bundles.scala 31:30]
  wire [15:0] _wsize_T = {w_size1, 1'h0}; // @[package.scala 232:35]
  wire [15:0] _wsize_T_1 = _wsize_T | 16'h1; // @[package.scala 232:40]
  wire [15:0] _wsize_T_2 = {1'h0,w_size1}; // @[Cat.scala 31:58]
  wire [15:0] _wsize_T_3 = ~_wsize_T_2; // @[package.scala 232:53]
  wire [15:0] _wsize_T_4 = _wsize_T_1 & _wsize_T_3; // @[package.scala 232:51]
  wire [7:0] w_size_hi = _wsize_T_4[15:8]; // @[OneHot.scala 30:18]
  wire [7:0] w_size_lo = _wsize_T_4[7:0]; // @[OneHot.scala 31:18]
  wire  _wsize_T_5 = |w_size_hi; // @[OneHot.scala 32:14]
  wire [7:0] _wsize_T_6 = w_size_hi | w_size_lo; // @[OneHot.scala 32:28]
  wire [3:0] w_size_hi_1 = _wsize_T_6[7:4]; // @[OneHot.scala 30:18]
  wire [3:0] w_size_lo_1 = _wsize_T_6[3:0]; // @[OneHot.scala 31:18]
  wire  _wsize_T_7 = |w_size_hi_1; // @[OneHot.scala 32:14]
  wire [3:0] _wsize_T_8 = w_size_hi_1 | w_size_lo_1; // @[OneHot.scala 32:28]
  wire [1:0] w_size_hi_2 = _wsize_T_8[3:2]; // @[OneHot.scala 30:18]
  wire [1:0] w_size_lo_2 = _wsize_T_8[1:0]; // @[OneHot.scala 31:18]
  wire  _wsize_T_9 = |w_size_hi_2; // @[OneHot.scala 32:14]
  wire [1:0] _wsize_T_10 = w_size_hi_2 | w_size_lo_2; // @[OneHot.scala 32:28]
  wire [3:0] w_size = {_wsize_T_5,_wsize_T_7,_wsize_T_9,_wsize_T_10[1]}; // @[Cat.scala 31:58]
  wire  _wok_T_1 = w_size <= 4'h7; // @[Parameters.scala 92:42]
  wire [38:0] _wok_T_5 = {1'b0,$signed(auto_in_awaddr)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_7 = $signed(_wok_T_5) & -39'sh1000000000; // @[Parameters.scala 137:52]
  wire  _wok_T_8 = $signed(_wok_T_7) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_9 = auto_in_awaddr ^ 38'h1000000000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_10 = {1'b0,$signed(_wok_T_9)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_12 = $signed(_wok_T_10) & -39'sh800000000; // @[Parameters.scala 137:52]
  wire  _wok_T_13 = $signed(_wok_T_12) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_14 = auto_in_awaddr ^ 38'h1800000000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_15 = {1'b0,$signed(_wok_T_14)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_17 = $signed(_wok_T_15) & -39'sh400000000; // @[Parameters.scala 137:52]
  wire  _wok_T_18 = $signed(_wok_T_17) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_19 = auto_in_awaddr ^ 38'h1c00000000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_20 = {1'b0,$signed(_wok_T_19)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_22 = $signed(_wok_T_20) & -39'sh200000000; // @[Parameters.scala 137:52]
  wire  _wok_T_23 = $signed(_wok_T_22) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_24 = auto_in_awaddr ^ 38'h1e00000000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_25 = {1'b0,$signed(_wok_T_24)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_27 = $signed(_wok_T_25) & -39'sh100000000; // @[Parameters.scala 137:52]
  wire  _wok_T_28 = $signed(_wok_T_27) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_29 = auto_in_awaddr ^ 38'h1f50000000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_30 = {1'b0,$signed(_wok_T_29)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_32 = $signed(_wok_T_30) & -39'sh400000; // @[Parameters.scala 137:52]
  wire  _wok_T_33 = $signed(_wok_T_32) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_34 = auto_in_awaddr ^ 38'h2000000000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_35 = {1'b0,$signed(_wok_T_34)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_37 = $signed(_wok_T_35) & -39'sh2000000000; // @[Parameters.scala 137:52]
  wire  _wok_T_38 = $signed(_wok_T_37) == 39'sh0; // @[Parameters.scala 137:67]
  wire  _wok_T_44 = _wok_T_8 | _wok_T_13 | _wok_T_18 | _wok_T_23 | _wok_T_28 | _wok_T_33 | _wok_T_38; // @[Parameters.scala 671:42]
  wire  _wok_T_45 = _wok_T_1 & _wok_T_44; // @[Parameters.scala 670:56]
  wire  _wok_T_47 = w_size <= 4'h3; // @[Parameters.scala 92:42]
  wire [37:0] _wok_T_50 = auto_in_awaddr ^ 38'h1f00050000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_51 = {1'b0,$signed(_wok_T_50)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_53 = $signed(_wok_T_51) & -39'sh10; // @[Parameters.scala 137:52]
  wire  _wok_T_54 = $signed(_wok_T_53) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_55 = auto_in_awaddr ^ 38'h1f00060000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_56 = {1'b0,$signed(_wok_T_55)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_58 = $signed(_wok_T_56) & -39'sh20000; // @[Parameters.scala 137:52]
  wire  _wok_T_59 = $signed(_wok_T_58) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_60 = auto_in_awaddr ^ 38'h1f40001000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_61 = {1'b0,$signed(_wok_T_60)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_63 = $signed(_wok_T_61) & -39'sh8; // @[Parameters.scala 137:52]
  wire  _wok_T_64 = $signed(_wok_T_63) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_65 = auto_in_awaddr ^ 38'h1f40002000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_66 = {1'b0,$signed(_wok_T_65)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_68 = $signed(_wok_T_66) & -39'sh1000; // @[Parameters.scala 137:52]
  wire  _wok_T_69 = $signed(_wok_T_68) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_70 = auto_in_awaddr ^ 38'h1f80000000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_71 = {1'b0,$signed(_wok_T_70)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_73 = $signed(_wok_T_71) & -39'sh40000000; // @[Parameters.scala 137:52]
  wire  _wok_T_74 = $signed(_wok_T_73) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_75 = auto_in_awaddr ^ 38'h1fe2000000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_76 = {1'b0,$signed(_wok_T_75)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_78 = $signed(_wok_T_76) & -39'sh200000; // @[Parameters.scala 137:52]
  wire  _wok_T_79 = $signed(_wok_T_78) == 39'sh0; // @[Parameters.scala 137:67]
  wire [37:0] _wok_T_80 = auto_in_awaddr ^ 38'h1ffff80000; // @[Parameters.scala 137:31]
  wire [38:0] _wok_T_81 = {1'b0,$signed(_wok_T_80)}; // @[Parameters.scala 137:49]
  wire [38:0] _wok_T_83 = $signed(_wok_T_81) & -39'sh40000; // @[Parameters.scala 137:52]
  wire  _wok_T_84 = $signed(_wok_T_83) == 39'sh0; // @[Parameters.scala 137:67]
  wire  _wok_T_90 = _wok_T_54 | _wok_T_59 | _wok_T_64 | _wok_T_69 | _wok_T_74 | _wok_T_79 | _wok_T_84; // @[Parameters.scala 671:42]
  wire  _wok_T_91 = _wok_T_47 & _wok_T_90; // @[Parameters.scala 670:56]
  wire  w_ok = _wok_T_45 | _wok_T_91; // @[Parameters.scala 672:30]
  wire [36:0] _GEN_165 = {{32'd0}, auto_in_awaddr[4:0]}; // @[ToTL.scala 123:59]
  wire [36:0] _waddr_T_1 = 37'h1e00000000 | _GEN_165; // @[ToTL.scala 123:59]
  wire [37:0] w_addr = w_ok ? auto_in_awaddr : {{1'd0}, _waddr_T_1}; // @[ToTL.scala 123:23]
  reg  w_count_0; // @[ToTL.scala 124:28]
  reg  w_count_1; // @[ToTL.scala 124:28]
  reg  w_count_2; // @[ToTL.scala 124:28]
  reg  w_count_3; // @[ToTL.scala 124:28]
  reg  w_count_4; // @[ToTL.scala 124:28]
  reg  w_count_5; // @[ToTL.scala 124:28]
  reg  w_count_6; // @[ToTL.scala 124:28]
  reg  w_count_7; // @[ToTL.scala 124:28]
  reg  w_count_8; // @[ToTL.scala 124:28]
  reg  w_count_9; // @[ToTL.scala 124:28]
  reg  w_count_10; // @[ToTL.scala 124:28]
  reg  w_count_11; // @[ToTL.scala 124:28]
  reg  w_count_12; // @[ToTL.scala 124:28]
  reg  w_count_13; // @[ToTL.scala 124:28]
  reg  w_count_14; // @[ToTL.scala 124:28]
  reg  w_count_15; // @[ToTL.scala 124:28]
  reg  w_count_16; // @[ToTL.scala 124:28]
  reg  w_count_17; // @[ToTL.scala 124:28]
  reg  w_count_18; // @[ToTL.scala 124:28]
  reg  w_count_19; // @[ToTL.scala 124:28]
  reg  w_count_20; // @[ToTL.scala 124:28]
  reg  w_count_21; // @[ToTL.scala 124:28]
  reg  w_count_22; // @[ToTL.scala 124:28]
  reg  w_count_23; // @[ToTL.scala 124:28]
  reg  w_count_24; // @[ToTL.scala 124:28]
  reg  w_count_25; // @[ToTL.scala 124:28]
  reg  w_count_26; // @[ToTL.scala 124:28]
  reg  w_count_27; // @[ToTL.scala 124:28]
  reg  w_count_28; // @[ToTL.scala 124:28]
  reg  w_count_29; // @[ToTL.scala 124:28]
  reg  w_count_30; // @[ToTL.scala 124:28]
  reg  w_count_31; // @[ToTL.scala 124:28]
  wire [5:0] w_id = {auto_in_awid,1'h1}; // @[Cat.scala 31:58]
  wire  readys_1 = readys_readys[1]; // @[Arbiter.scala 95:86]
  reg  state_1; // @[Arbiter.scala 116:26]
  wire  allowed_1 = idle ? readys_1 : state_1; // @[Arbiter.scala 121:24]
  wire  out_1_ready = auto_out_a_ready & allowed_1; // @[Arbiter.scala 123:31]
  wire  bundleIn_0_awready = out_1_ready & auto_in_wvalid & auto_in_wlast; // @[ToTL.scala 133:48]
  wire  w_out_bits_user_amba_prot_privileged = auto_in_awprot[0]; // @[ToTL.scala 141:45]
  wire  w_out_bits_user_amba_prot_secure = ~auto_in_awprot[1]; // @[ToTL.scala 142:29]
  wire  w_out_bits_user_amba_prot_fetch = auto_in_awprot[2]; // @[ToTL.scala 143:45]
  wire  w_out_bits_user_amba_prot_bufferable = auto_in_awcache[0]; // @[ToTL.scala 144:46]
  wire  w_out_bits_user_amba_prot_modifiable = auto_in_awcache[1]; // @[ToTL.scala 145:46]
  wire  w_out_bits_user_amba_prot_readalloc = auto_in_awcache[2]; // @[ToTL.scala 146:46]
  wire  w_out_bits_user_amba_prot_writealloc = auto_in_awcache[3]; // @[ToTL.scala 147:46]
  wire [31:0] w_sel = 32'h1 << auto_in_awid; // @[OneHot.scala 64:12]
  wire  _T_156 = bundleIn_0_awready & auto_in_awvalid; // @[Decoupled.scala 50:35]
  wire  latch = idle & auto_out_a_ready; // @[Arbiter.scala 89:24]
  wire [1:0] _readys_mask_T = readys_readys & readys_valid; // @[Arbiter.scala 28:29]
  wire [2:0] _readys_mask_T_1 = {_readys_mask_T, 1'h0}; // @[package.scala 244:48]
  wire [1:0] _readys_mask_T_3 = _readys_mask_T | _readys_mask_T_1[1:0]; // @[package.scala 244:43]
  wire  earlyWinner_0 = readys_0 & auto_in_arvalid; // @[Arbiter.scala 97:79]
  wire  earlyWinner_1 = readys_1 & w_out_valid; // @[Arbiter.scala 97:79]
  wire  _T_230 = auto_in_arvalid | w_out_valid; // @[Arbiter.scala 107:36]
  wire  muxStateEarly_0 = idle ? earlyWinner_0 : state_0; // @[Arbiter.scala 117:30]
  wire  muxStateEarly_1 = idle ? earlyWinner_1 : state_1; // @[Arbiter.scala 117:30]
  wire  _sink_ACancel_earlyValid_T_3 = state_0 & auto_in_arvalid | state_1 & w_out_valid; // @[Mux.scala 27:73]
  wire  sink_ACancel_earlyValid = idle ? _T_230 : _sink_ACancel_earlyValid_T_3; // @[Arbiter.scala 125:29]
  wire  _beatsLeft_T_2 = auto_out_a_ready & sink_ACancel_earlyValid; // @[ReadyValidCancel.scala 49:33]
  wire [7:0] _GEN_166 = {{7'd0}, _beatsLeft_T_2}; // @[Arbiter.scala 113:52]
  wire [7:0] _beatsLeft_T_4 = beatsLeft - _GEN_166; // @[Arbiter.scala 113:52]
  wire [31:0] _T_250 = muxStateEarly_0 ? a_mask : 32'h0; // @[Mux.scala 27:73]
  wire [31:0] _T_251 = muxStateEarly_1 ? auto_in_wstrb : 32'h0; // @[Mux.scala 27:73]
  wire [37:0] _T_274 = muxStateEarly_0 ? r_addr : 38'h0; // @[Mux.scala 27:73]
  wire [37:0] _T_275 = muxStateEarly_1 ? w_addr : 38'h0; // @[Mux.scala 27:73]
  wire [1:0] a_source = r_id[1:0]; // @[Edges.scala 447:17 451:15]
  wire [1:0] _T_277 = muxStateEarly_0 ? a_source : 2'h0; // @[Mux.scala 27:73]
  wire [1:0] a_1_source = w_id[1:0]; // @[Edges.scala 483:17 487:15]
  wire [1:0] _T_278 = muxStateEarly_1 ? a_1_source : 2'h0; // @[Mux.scala 27:73]
  wire [2:0] a_size = r_size[2:0]; // @[Edges.scala 447:17 450:15]
  wire [2:0] _T_280 = muxStateEarly_0 ? a_size : 3'h0; // @[Mux.scala 27:73]
  wire [2:0] a_1_size = w_size[2:0]; // @[Edges.scala 483:17 486:15]
  wire [2:0] _T_281 = muxStateEarly_1 ? a_1_size : 3'h0; // @[Mux.scala 27:73]
  wire [2:0] _T_286 = muxStateEarly_0 ? 3'h4 : 3'h0; // @[Mux.scala 27:73]
  wire [2:0] _T_287 = muxStateEarly_1 ? 3'h1 : 3'h0; // @[Mux.scala 27:73]
  wire  d_hasData = auto_out_d_bits_opcode[0]; // @[Edges.scala 105:36]
  wire  ok_rready = deq_io_enq_ready; // @[ToTL.scala 158:23 Decoupled.scala 365:17]
  wire  ok_bready = q_bdeq_io_enq_ready; // @[ToTL.scala 157:23 Decoupled.scala 365:17]
  wire  bundleOut_0_d_ready = d_hasData ? ok_rready : ok_bready; // @[ToTL.scala 164:25]
  wire  _d_last_T = bundleOut_0_d_ready & auto_out_d_valid; // @[Decoupled.scala 50:35]
  wire [13:0] _d_last_beats1_decode_T_1 = 14'h7f << auto_out_d_bits_size; // @[package.scala 234:77]
  wire [6:0] _d_last_beats1_decode_T_3 = ~_d_last_beats1_decode_T_1[6:0]; // @[package.scala 234:46]
  wire [1:0] d_last_beats1_decode = _d_last_beats1_decode_T_3[6:5]; // @[Edges.scala 219:59]
  wire [1:0] d_last_beats1 = d_hasData ? d_last_beats1_decode : 2'h0; // @[Edges.scala 220:14]
  reg [1:0] d_last_counter; // @[Edges.scala 228:27]
  wire [1:0] d_last_counter1 = d_last_counter - 2'h1; // @[Edges.scala 229:28]
  wire  d_last_first = d_last_counter == 2'h0; // @[Edges.scala 230:25]
  reg  b_count_0; // @[ToTL.scala 186:28]
  reg  b_count_1; // @[ToTL.scala 186:28]
  reg  b_count_2; // @[ToTL.scala 186:28]
  reg  b_count_3; // @[ToTL.scala 186:28]
  reg  b_count_4; // @[ToTL.scala 186:28]
  reg  b_count_5; // @[ToTL.scala 186:28]
  reg  b_count_6; // @[ToTL.scala 186:28]
  reg  b_count_7; // @[ToTL.scala 186:28]
  reg  b_count_8; // @[ToTL.scala 186:28]
  reg  b_count_9; // @[ToTL.scala 186:28]
  reg  b_count_10; // @[ToTL.scala 186:28]
  reg  b_count_11; // @[ToTL.scala 186:28]
  reg  b_count_12; // @[ToTL.scala 186:28]
  reg  b_count_13; // @[ToTL.scala 186:28]
  reg  b_count_14; // @[ToTL.scala 186:28]
  reg  b_count_15; // @[ToTL.scala 186:28]
  reg  b_count_16; // @[ToTL.scala 186:28]
  reg  b_count_17; // @[ToTL.scala 186:28]
  reg  b_count_18; // @[ToTL.scala 186:28]
  reg  b_count_19; // @[ToTL.scala 186:28]
  reg  b_count_20; // @[ToTL.scala 186:28]
  reg  b_count_21; // @[ToTL.scala 186:28]
  reg  b_count_22; // @[ToTL.scala 186:28]
  reg  b_count_23; // @[ToTL.scala 186:28]
  reg  b_count_24; // @[ToTL.scala 186:28]
  reg  b_count_25; // @[ToTL.scala 186:28]
  reg  b_count_26; // @[ToTL.scala 186:28]
  reg  b_count_27; // @[ToTL.scala 186:28]
  reg  b_count_28; // @[ToTL.scala 186:28]
  reg  b_count_29; // @[ToTL.scala 186:28]
  reg  b_count_30; // @[ToTL.scala 186:28]
  reg  b_count_31; // @[ToTL.scala 186:28]
  wire [4:0] q_bid = q_bdeq_io_deq_bits_id; // @[Decoupled.scala 401:19 402:14]
  wire  _GEN_67 = 5'h1 == q_bid ? b_count_1 : b_count_0; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_68 = 5'h2 == q_bid ? b_count_2 : _GEN_67; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_69 = 5'h3 == q_bid ? b_count_3 : _GEN_68; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_70 = 5'h4 == q_bid ? b_count_4 : _GEN_69; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_71 = 5'h5 == q_bid ? b_count_5 : _GEN_70; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_72 = 5'h6 == q_bid ? b_count_6 : _GEN_71; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_73 = 5'h7 == q_bid ? b_count_7 : _GEN_72; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_74 = 5'h8 == q_bid ? b_count_8 : _GEN_73; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_75 = 5'h9 == q_bid ? b_count_9 : _GEN_74; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_76 = 5'ha == q_bid ? b_count_10 : _GEN_75; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_77 = 5'hb == q_bid ? b_count_11 : _GEN_76; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_78 = 5'hc == q_bid ? b_count_12 : _GEN_77; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_79 = 5'hd == q_bid ? b_count_13 : _GEN_78; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_80 = 5'he == q_bid ? b_count_14 : _GEN_79; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_81 = 5'hf == q_bid ? b_count_15 : _GEN_80; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_82 = 5'h10 == q_bid ? b_count_16 : _GEN_81; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_83 = 5'h11 == q_bid ? b_count_17 : _GEN_82; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_84 = 5'h12 == q_bid ? b_count_18 : _GEN_83; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_85 = 5'h13 == q_bid ? b_count_19 : _GEN_84; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_86 = 5'h14 == q_bid ? b_count_20 : _GEN_85; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_87 = 5'h15 == q_bid ? b_count_21 : _GEN_86; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_88 = 5'h16 == q_bid ? b_count_22 : _GEN_87; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_89 = 5'h17 == q_bid ? b_count_23 : _GEN_88; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_90 = 5'h18 == q_bid ? b_count_24 : _GEN_89; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_91 = 5'h19 == q_bid ? b_count_25 : _GEN_90; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_92 = 5'h1a == q_bid ? b_count_26 : _GEN_91; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_93 = 5'h1b == q_bid ? b_count_27 : _GEN_92; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_94 = 5'h1c == q_bid ? b_count_28 : _GEN_93; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_95 = 5'h1d == q_bid ? b_count_29 : _GEN_94; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_96 = 5'h1e == q_bid ? b_count_30 : _GEN_95; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_97 = 5'h1f == q_bid ? b_count_31 : _GEN_96; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_99 = 5'h1 == q_bid ? w_count_1 : w_count_0; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_100 = 5'h2 == q_bid ? w_count_2 : _GEN_99; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_101 = 5'h3 == q_bid ? w_count_3 : _GEN_100; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_102 = 5'h4 == q_bid ? w_count_4 : _GEN_101; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_103 = 5'h5 == q_bid ? w_count_5 : _GEN_102; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_104 = 5'h6 == q_bid ? w_count_6 : _GEN_103; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_105 = 5'h7 == q_bid ? w_count_7 : _GEN_104; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_106 = 5'h8 == q_bid ? w_count_8 : _GEN_105; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_107 = 5'h9 == q_bid ? w_count_9 : _GEN_106; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_108 = 5'ha == q_bid ? w_count_10 : _GEN_107; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_109 = 5'hb == q_bid ? w_count_11 : _GEN_108; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_110 = 5'hc == q_bid ? w_count_12 : _GEN_109; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_111 = 5'hd == q_bid ? w_count_13 : _GEN_110; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_112 = 5'he == q_bid ? w_count_14 : _GEN_111; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_113 = 5'hf == q_bid ? w_count_15 : _GEN_112; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_114 = 5'h10 == q_bid ? w_count_16 : _GEN_113; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_115 = 5'h11 == q_bid ? w_count_17 : _GEN_114; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_116 = 5'h12 == q_bid ? w_count_18 : _GEN_115; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_117 = 5'h13 == q_bid ? w_count_19 : _GEN_116; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_118 = 5'h14 == q_bid ? w_count_20 : _GEN_117; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_119 = 5'h15 == q_bid ? w_count_21 : _GEN_118; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_120 = 5'h16 == q_bid ? w_count_22 : _GEN_119; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_121 = 5'h17 == q_bid ? w_count_23 : _GEN_120; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_122 = 5'h18 == q_bid ? w_count_24 : _GEN_121; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_123 = 5'h19 == q_bid ? w_count_25 : _GEN_122; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_124 = 5'h1a == q_bid ? w_count_26 : _GEN_123; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_125 = 5'h1b == q_bid ? w_count_27 : _GEN_124; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_126 = 5'h1c == q_bid ? w_count_28 : _GEN_125; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_127 = 5'h1d == q_bid ? w_count_29 : _GEN_126; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_128 = 5'h1e == q_bid ? w_count_30 : _GEN_127; // @[ToTL.scala 187:{43,43}]
  wire  _GEN_129 = 5'h1f == q_bid ? w_count_31 : _GEN_128; // @[ToTL.scala 187:{43,43}]
  wire  b_allow = _GEN_97 != _GEN_129; // @[ToTL.scala 187:43]
  wire [31:0] b_sel = 32'h1 << q_bid; // @[OneHot.scala 64:12]
  wire  q_bvalid = q_bdeq_io_deq_valid; // @[Decoupled.scala 401:19 403:15]
  wire  bundleIn_0_bvalid = q_bvalid & b_allow; // @[ToTL.scala 195:31]
  wire  _T_321 = auto_in_bready & bundleIn_0_bvalid; // @[Decoupled.scala 50:35]
  Queue_460 deq ( // @[Decoupled.scala 361:21]
    .clock(deq_clock),
    .reset(deq_reset),
    .io_enq_ready(deq_io_enq_ready),
    .io_enq_valid(deq_io_enq_valid),
    .io_enq_bits_id(deq_io_enq_bits_id),
    .io_enq_bits_data(deq_io_enq_bits_data),
    .io_enq_bits_resp(deq_io_enq_bits_resp),
    .io_enq_bits_last(deq_io_enq_bits_last),
    .io_deq_ready(deq_io_deq_ready),
    .io_deq_valid(deq_io_deq_valid),
    .io_deq_bits_id(deq_io_deq_bits_id),
    .io_deq_bits_data(deq_io_deq_bits_data),
    .io_deq_bits_resp(deq_io_deq_bits_resp),
    .io_deq_bits_last(deq_io_deq_bits_last)
  );
  Queue_461 q_bdeq ( // @[Decoupled.scala 361:21]
    .clock(q_bdeq_clock),
    .reset(q_bdeq_reset),
    .io_enq_ready(q_bdeq_io_enq_ready),
    .io_enq_valid(q_bdeq_io_enq_valid),
    .io_enq_bits_id(q_bdeq_io_enq_bits_id),
    .io_enq_bits_resp(q_bdeq_io_enq_bits_resp),
    .io_deq_ready(q_bdeq_io_deq_ready),
    .io_deq_valid(q_bdeq_io_deq_valid),
    .io_deq_bits_id(q_bdeq_io_deq_bits_id),
    .io_deq_bits_resp(q_bdeq_io_deq_bits_resp)
  );
  assign auto_in_awready = out_1_ready & auto_in_wvalid & auto_in_wlast; // @[ToTL.scala 133:48]
  assign auto_in_wready = out_1_ready & auto_in_awvalid; // @[ToTL.scala 134:34]
  assign auto_in_bvalid = q_bvalid & b_allow; // @[ToTL.scala 195:31]
  assign auto_in_bid = q_bdeq_io_deq_bits_id; // @[Decoupled.scala 401:19 402:14]
  assign auto_in_bresp = q_bdeq_io_deq_bits_resp; // @[Decoupled.scala 401:19 402:14]
  assign auto_in_arready = auto_out_a_ready & allowed_0; // @[Arbiter.scala 123:31]
  assign auto_in_rvalid = deq_io_deq_valid; // @[Decoupled.scala 401:19 403:15]
  assign auto_in_rid = deq_io_deq_bits_id; // @[Decoupled.scala 401:19 402:14]
  assign auto_in_rdata = deq_io_deq_bits_data; // @[Decoupled.scala 401:19 402:14]
  assign auto_in_rresp = deq_io_deq_bits_resp; // @[Decoupled.scala 401:19 402:14]
  assign auto_in_rlast = deq_io_deq_bits_last; // @[Decoupled.scala 401:19 402:14]
  assign auto_out_a_valid = idle ? _T_230 : _sink_ACancel_earlyValid_T_3; // @[Arbiter.scala 125:29]
  assign auto_out_a_bits_opcode = _T_286 | _T_287; // @[Mux.scala 27:73]
  assign auto_out_a_bits_size = _T_280 | _T_281; // @[Mux.scala 27:73]
  assign auto_out_a_bits_source = _T_277 | _T_278; // @[Mux.scala 27:73]
  assign auto_out_a_bits_address = _T_274 | _T_275; // @[Mux.scala 27:73]
  assign auto_out_a_bits_user_amba_prot_bufferable = muxStateEarly_0 & r_out_bits_user_amba_prot_bufferable |
    muxStateEarly_1 & w_out_bits_user_amba_prot_bufferable; // @[Mux.scala 27:73]
  assign auto_out_a_bits_user_amba_prot_modifiable = muxStateEarly_0 & r_out_bits_user_amba_prot_modifiable |
    muxStateEarly_1 & w_out_bits_user_amba_prot_modifiable; // @[Mux.scala 27:73]
  assign auto_out_a_bits_user_amba_prot_readalloc = muxStateEarly_0 & r_out_bits_user_amba_prot_readalloc |
    muxStateEarly_1 & w_out_bits_user_amba_prot_readalloc; // @[Mux.scala 27:73]
  assign auto_out_a_bits_user_amba_prot_writealloc = muxStateEarly_0 & r_out_bits_user_amba_prot_writealloc |
    muxStateEarly_1 & w_out_bits_user_amba_prot_writealloc; // @[Mux.scala 27:73]
  assign auto_out_a_bits_user_amba_prot_privileged = muxStateEarly_0 & r_out_bits_user_amba_prot_privileged |
    muxStateEarly_1 & w_out_bits_user_amba_prot_privileged; // @[Mux.scala 27:73]
  assign auto_out_a_bits_user_amba_prot_secure = muxStateEarly_0 & r_out_bits_user_amba_prot_secure | muxStateEarly_1 &
    w_out_bits_user_amba_prot_secure; // @[Mux.scala 27:73]
  assign auto_out_a_bits_user_amba_prot_fetch = muxStateEarly_0 & r_out_bits_user_amba_prot_fetch | muxStateEarly_1 &
    w_out_bits_user_amba_prot_fetch; // @[Mux.scala 27:73]
  assign auto_out_a_bits_mask = _T_250 | _T_251; // @[Mux.scala 27:73]
  assign auto_out_a_bits_data = muxStateEarly_1 ? auto_in_wdata : 256'h0; // @[Mux.scala 27:73]
  assign auto_out_d_ready = d_hasData ? ok_rready : ok_bready; // @[ToTL.scala 164:25]
  assign deq_clock = clock;
  assign deq_reset = reset;
  assign deq_io_enq_valid = auto_out_d_valid & d_hasData; // @[ToTL.scala 165:33]
  assign deq_io_enq_bits_id = {{4'd0}, auto_out_d_bits_source[1]}; // @[ToTL.scala 158:23 168:22]
  assign deq_io_enq_bits_data = auto_out_d_bits_data; // @[Nodes.scala 1207:84 LazyModule.scala 311:12]
  assign deq_io_enq_bits_resp = auto_out_d_bits_denied | auto_out_d_bits_corrupt ? 2'h2 : 2'h0; // @[ToTL.scala 160:23]
  assign deq_io_enq_bits_last = d_last_counter == 2'h1 | d_last_beats1 == 2'h0; // @[Edges.scala 231:37]
  assign deq_io_deq_ready = auto_in_rready; // @[Nodes.scala 1210:84 LazyModule.scala 309:16]
  assign q_bdeq_clock = clock;
  assign q_bdeq_reset = reset;
  assign q_bdeq_io_enq_valid = auto_out_d_valid & ~d_hasData; // @[ToTL.scala 166:33]
  assign q_bdeq_io_enq_bits_id = {{4'd0}, auto_out_d_bits_source[1]}; // @[ToTL.scala 157:23 177:22]
  assign q_bdeq_io_enq_bits_resp = auto_out_d_bits_denied | auto_out_d_bits_corrupt ? 2'h2 : 2'h0; // @[ToTL.scala 160:23]
  assign q_bdeq_io_deq_ready = auto_in_bready & b_allow; // @[ToTL.scala 196:31]
  always @(posedge clock) begin
    if (reset) begin // @[Arbiter.scala 87:30]
      beatsLeft <= 8'h0; // @[Arbiter.scala 87:30]
    end else if (latch) begin // @[Arbiter.scala 113:23]
      if (earlyWinner_1) begin // @[Arbiter.scala 111:73]
        beatsLeft <= auto_in_awlen;
      end else begin
        beatsLeft <= 8'h0;
      end
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
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_0 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[0]) begin // @[ToTL.scala 152:34]
      w_count_0 <= w_count_0 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_1 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[1]) begin // @[ToTL.scala 152:34]
      w_count_1 <= w_count_1 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_2 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[2]) begin // @[ToTL.scala 152:34]
      w_count_2 <= w_count_2 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_3 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[3]) begin // @[ToTL.scala 152:34]
      w_count_3 <= w_count_3 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_4 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[4]) begin // @[ToTL.scala 152:34]
      w_count_4 <= w_count_4 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_5 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[5]) begin // @[ToTL.scala 152:34]
      w_count_5 <= w_count_5 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_6 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[6]) begin // @[ToTL.scala 152:34]
      w_count_6 <= w_count_6 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_7 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[7]) begin // @[ToTL.scala 152:34]
      w_count_7 <= w_count_7 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_8 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[8]) begin // @[ToTL.scala 152:34]
      w_count_8 <= w_count_8 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_9 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[9]) begin // @[ToTL.scala 152:34]
      w_count_9 <= w_count_9 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_10 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[10]) begin // @[ToTL.scala 152:34]
      w_count_10 <= w_count_10 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_11 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[11]) begin // @[ToTL.scala 152:34]
      w_count_11 <= w_count_11 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_12 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[12]) begin // @[ToTL.scala 152:34]
      w_count_12 <= w_count_12 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_13 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[13]) begin // @[ToTL.scala 152:34]
      w_count_13 <= w_count_13 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_14 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[14]) begin // @[ToTL.scala 152:34]
      w_count_14 <= w_count_14 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_15 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[15]) begin // @[ToTL.scala 152:34]
      w_count_15 <= w_count_15 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_16 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[16]) begin // @[ToTL.scala 152:34]
      w_count_16 <= w_count_16 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_17 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[17]) begin // @[ToTL.scala 152:34]
      w_count_17 <= w_count_17 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_18 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[18]) begin // @[ToTL.scala 152:34]
      w_count_18 <= w_count_18 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_19 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[19]) begin // @[ToTL.scala 152:34]
      w_count_19 <= w_count_19 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_20 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[20]) begin // @[ToTL.scala 152:34]
      w_count_20 <= w_count_20 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_21 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[21]) begin // @[ToTL.scala 152:34]
      w_count_21 <= w_count_21 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_22 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[22]) begin // @[ToTL.scala 152:34]
      w_count_22 <= w_count_22 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_23 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[23]) begin // @[ToTL.scala 152:34]
      w_count_23 <= w_count_23 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_24 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[24]) begin // @[ToTL.scala 152:34]
      w_count_24 <= w_count_24 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_25 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[25]) begin // @[ToTL.scala 152:34]
      w_count_25 <= w_count_25 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_26 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[26]) begin // @[ToTL.scala 152:34]
      w_count_26 <= w_count_26 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_27 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[27]) begin // @[ToTL.scala 152:34]
      w_count_27 <= w_count_27 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_28 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[28]) begin // @[ToTL.scala 152:34]
      w_count_28 <= w_count_28 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_29 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[29]) begin // @[ToTL.scala 152:34]
      w_count_29 <= w_count_29 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_30 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[30]) begin // @[ToTL.scala 152:34]
      w_count_30 <= w_count_30 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[ToTL.scala 124:28]
      w_count_31 <= 1'h0; // @[ToTL.scala 124:28]
    end else if (_T_156 & w_sel[31]) begin // @[ToTL.scala 152:34]
      w_count_31 <= w_count_31 + 1'h1; // @[ToTL.scala 152:38]
    end
    if (reset) begin // @[Arbiter.scala 116:26]
      state_1 <= 1'h0; // @[Arbiter.scala 116:26]
    end else if (idle) begin // @[Arbiter.scala 117:30]
      state_1 <= earlyWinner_1;
    end
    if (reset) begin // @[Edges.scala 228:27]
      d_last_counter <= 2'h0; // @[Edges.scala 228:27]
    end else if (_d_last_T) begin // @[Edges.scala 234:17]
      if (d_last_first) begin // @[Edges.scala 235:21]
        if (d_hasData) begin // @[Edges.scala 220:14]
          d_last_counter <= d_last_beats1_decode;
        end else begin
          d_last_counter <= 2'h0;
        end
      end else begin
        d_last_counter <= d_last_counter1;
      end
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_0 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[0]) begin // @[ToTL.scala 191:33]
      b_count_0 <= b_count_0 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_1 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[1]) begin // @[ToTL.scala 191:33]
      b_count_1 <= b_count_1 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_2 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[2]) begin // @[ToTL.scala 191:33]
      b_count_2 <= b_count_2 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_3 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[3]) begin // @[ToTL.scala 191:33]
      b_count_3 <= b_count_3 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_4 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[4]) begin // @[ToTL.scala 191:33]
      b_count_4 <= b_count_4 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_5 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[5]) begin // @[ToTL.scala 191:33]
      b_count_5 <= b_count_5 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_6 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[6]) begin // @[ToTL.scala 191:33]
      b_count_6 <= b_count_6 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_7 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[7]) begin // @[ToTL.scala 191:33]
      b_count_7 <= b_count_7 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_8 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[8]) begin // @[ToTL.scala 191:33]
      b_count_8 <= b_count_8 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_9 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[9]) begin // @[ToTL.scala 191:33]
      b_count_9 <= b_count_9 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_10 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[10]) begin // @[ToTL.scala 191:33]
      b_count_10 <= b_count_10 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_11 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[11]) begin // @[ToTL.scala 191:33]
      b_count_11 <= b_count_11 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_12 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[12]) begin // @[ToTL.scala 191:33]
      b_count_12 <= b_count_12 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_13 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[13]) begin // @[ToTL.scala 191:33]
      b_count_13 <= b_count_13 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_14 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[14]) begin // @[ToTL.scala 191:33]
      b_count_14 <= b_count_14 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_15 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[15]) begin // @[ToTL.scala 191:33]
      b_count_15 <= b_count_15 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_16 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[16]) begin // @[ToTL.scala 191:33]
      b_count_16 <= b_count_16 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_17 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[17]) begin // @[ToTL.scala 191:33]
      b_count_17 <= b_count_17 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_18 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[18]) begin // @[ToTL.scala 191:33]
      b_count_18 <= b_count_18 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_19 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[19]) begin // @[ToTL.scala 191:33]
      b_count_19 <= b_count_19 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_20 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[20]) begin // @[ToTL.scala 191:33]
      b_count_20 <= b_count_20 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_21 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[21]) begin // @[ToTL.scala 191:33]
      b_count_21 <= b_count_21 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_22 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[22]) begin // @[ToTL.scala 191:33]
      b_count_22 <= b_count_22 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_23 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[23]) begin // @[ToTL.scala 191:33]
      b_count_23 <= b_count_23 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_24 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[24]) begin // @[ToTL.scala 191:33]
      b_count_24 <= b_count_24 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_25 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[25]) begin // @[ToTL.scala 191:33]
      b_count_25 <= b_count_25 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_26 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[26]) begin // @[ToTL.scala 191:33]
      b_count_26 <= b_count_26 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_27 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[27]) begin // @[ToTL.scala 191:33]
      b_count_27 <= b_count_27 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_28 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[28]) begin // @[ToTL.scala 191:33]
      b_count_28 <= b_count_28 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_29 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[29]) begin // @[ToTL.scala 191:33]
      b_count_29 <= b_count_29 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_30 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[30]) begin // @[ToTL.scala 191:33]
      b_count_30 <= b_count_30 + 1'h1; // @[ToTL.scala 191:37]
    end
    if (reset) begin // @[ToTL.scala 186:28]
      b_count_31 <= 1'h0; // @[ToTL.scala 186:28]
    end else if (_T_321 & b_sel[31]) begin // @[ToTL.scala 191:33]
      b_count_31 <= b_count_31 + 1'h1; // @[ToTL.scala 191:37]
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
  beatsLeft = _RAND_0[7:0];
  _RAND_1 = {1{`RANDOM}};
  readys_mask = _RAND_1[1:0];
  _RAND_2 = {1{`RANDOM}};
  state_0 = _RAND_2[0:0];
  _RAND_3 = {1{`RANDOM}};
  w_count_0 = _RAND_3[0:0];
  _RAND_4 = {1{`RANDOM}};
  w_count_1 = _RAND_4[0:0];
  _RAND_5 = {1{`RANDOM}};
  w_count_2 = _RAND_5[0:0];
  _RAND_6 = {1{`RANDOM}};
  w_count_3 = _RAND_6[0:0];
  _RAND_7 = {1{`RANDOM}};
  w_count_4 = _RAND_7[0:0];
  _RAND_8 = {1{`RANDOM}};
  w_count_5 = _RAND_8[0:0];
  _RAND_9 = {1{`RANDOM}};
  w_count_6 = _RAND_9[0:0];
  _RAND_10 = {1{`RANDOM}};
  w_count_7 = _RAND_10[0:0];
  _RAND_11 = {1{`RANDOM}};
  w_count_8 = _RAND_11[0:0];
  _RAND_12 = {1{`RANDOM}};
  w_count_9 = _RAND_12[0:0];
  _RAND_13 = {1{`RANDOM}};
  w_count_10 = _RAND_13[0:0];
  _RAND_14 = {1{`RANDOM}};
  w_count_11 = _RAND_14[0:0];
  _RAND_15 = {1{`RANDOM}};
  w_count_12 = _RAND_15[0:0];
  _RAND_16 = {1{`RANDOM}};
  w_count_13 = _RAND_16[0:0];
  _RAND_17 = {1{`RANDOM}};
  w_count_14 = _RAND_17[0:0];
  _RAND_18 = {1{`RANDOM}};
  w_count_15 = _RAND_18[0:0];
  _RAND_19 = {1{`RANDOM}};
  w_count_16 = _RAND_19[0:0];
  _RAND_20 = {1{`RANDOM}};
  w_count_17 = _RAND_20[0:0];
  _RAND_21 = {1{`RANDOM}};
  w_count_18 = _RAND_21[0:0];
  _RAND_22 = {1{`RANDOM}};
  w_count_19 = _RAND_22[0:0];
  _RAND_23 = {1{`RANDOM}};
  w_count_20 = _RAND_23[0:0];
  _RAND_24 = {1{`RANDOM}};
  w_count_21 = _RAND_24[0:0];
  _RAND_25 = {1{`RANDOM}};
  w_count_22 = _RAND_25[0:0];
  _RAND_26 = {1{`RANDOM}};
  w_count_23 = _RAND_26[0:0];
  _RAND_27 = {1{`RANDOM}};
  w_count_24 = _RAND_27[0:0];
  _RAND_28 = {1{`RANDOM}};
  w_count_25 = _RAND_28[0:0];
  _RAND_29 = {1{`RANDOM}};
  w_count_26 = _RAND_29[0:0];
  _RAND_30 = {1{`RANDOM}};
  w_count_27 = _RAND_30[0:0];
  _RAND_31 = {1{`RANDOM}};
  w_count_28 = _RAND_31[0:0];
  _RAND_32 = {1{`RANDOM}};
  w_count_29 = _RAND_32[0:0];
  _RAND_33 = {1{`RANDOM}};
  w_count_30 = _RAND_33[0:0];
  _RAND_34 = {1{`RANDOM}};
  w_count_31 = _RAND_34[0:0];
  _RAND_35 = {1{`RANDOM}};
  state_1 = _RAND_35[0:0];
  _RAND_36 = {1{`RANDOM}};
  d_last_counter = _RAND_36[1:0];
  _RAND_37 = {1{`RANDOM}};
  b_count_0 = _RAND_37[0:0];
  _RAND_38 = {1{`RANDOM}};
  b_count_1 = _RAND_38[0:0];
  _RAND_39 = {1{`RANDOM}};
  b_count_2 = _RAND_39[0:0];
  _RAND_40 = {1{`RANDOM}};
  b_count_3 = _RAND_40[0:0];
  _RAND_41 = {1{`RANDOM}};
  b_count_4 = _RAND_41[0:0];
  _RAND_42 = {1{`RANDOM}};
  b_count_5 = _RAND_42[0:0];
  _RAND_43 = {1{`RANDOM}};
  b_count_6 = _RAND_43[0:0];
  _RAND_44 = {1{`RANDOM}};
  b_count_7 = _RAND_44[0:0];
  _RAND_45 = {1{`RANDOM}};
  b_count_8 = _RAND_45[0:0];
  _RAND_46 = {1{`RANDOM}};
  b_count_9 = _RAND_46[0:0];
  _RAND_47 = {1{`RANDOM}};
  b_count_10 = _RAND_47[0:0];
  _RAND_48 = {1{`RANDOM}};
  b_count_11 = _RAND_48[0:0];
  _RAND_49 = {1{`RANDOM}};
  b_count_12 = _RAND_49[0:0];
  _RAND_50 = {1{`RANDOM}};
  b_count_13 = _RAND_50[0:0];
  _RAND_51 = {1{`RANDOM}};
  b_count_14 = _RAND_51[0:0];
  _RAND_52 = {1{`RANDOM}};
  b_count_15 = _RAND_52[0:0];
  _RAND_53 = {1{`RANDOM}};
  b_count_16 = _RAND_53[0:0];
  _RAND_54 = {1{`RANDOM}};
  b_count_17 = _RAND_54[0:0];
  _RAND_55 = {1{`RANDOM}};
  b_count_18 = _RAND_55[0:0];
  _RAND_56 = {1{`RANDOM}};
  b_count_19 = _RAND_56[0:0];
  _RAND_57 = {1{`RANDOM}};
  b_count_20 = _RAND_57[0:0];
  _RAND_58 = {1{`RANDOM}};
  b_count_21 = _RAND_58[0:0];
  _RAND_59 = {1{`RANDOM}};
  b_count_22 = _RAND_59[0:0];
  _RAND_60 = {1{`RANDOM}};
  b_count_23 = _RAND_60[0:0];
  _RAND_61 = {1{`RANDOM}};
  b_count_24 = _RAND_61[0:0];
  _RAND_62 = {1{`RANDOM}};
  b_count_25 = _RAND_62[0:0];
  _RAND_63 = {1{`RANDOM}};
  b_count_26 = _RAND_63[0:0];
  _RAND_64 = {1{`RANDOM}};
  b_count_27 = _RAND_64[0:0];
  _RAND_65 = {1{`RANDOM}};
  b_count_28 = _RAND_65[0:0];
  _RAND_66 = {1{`RANDOM}};
  b_count_29 = _RAND_66[0:0];
  _RAND_67 = {1{`RANDOM}};
  b_count_30 = _RAND_67[0:0];
  _RAND_68 = {1{`RANDOM}};
  b_count_31 = _RAND_68[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

