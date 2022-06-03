module DMAFakeMSHR(
  input          clock,
  input          reset,
  input          io_enable,
  input          io_slave_wen,
  input  [3:0]   io_slave_addr,
  output [63:0]  io_slave_rdata,
  input  [63:0]  io_slave_wdata,
  output         io_master_req_valid,
  input          io_master_req_ready,
  output         io_master_req_is_write,
  output [63:0]  io_master_req_addr,
  output [63:0]  io_master_req_mask,
  output [511:0] io_master_req_data,
  input          io_master_resp_valid,
  input  [255:0] io_master_resp_bits
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [63:0] _RAND_1;
  reg [63:0] _RAND_2;
  reg [63:0] _RAND_3;
  reg [63:0] _RAND_4;
  reg [63:0] _RAND_5;
  reg [63:0] _RAND_6;
  reg [63:0] _RAND_7;
  reg [63:0] _RAND_8;
  reg [63:0] _RAND_9;
  reg [63:0] _RAND_10;
  reg [31:0] _RAND_11;
`endif // RANDOMIZE_REG_INIT
  wire  difftest_io_clock; // @[AXI4FakeDMA.scala 112:24]
  wire [7:0] difftest_io_coreid; // @[AXI4FakeDMA.scala 112:24]
  wire  difftest_io_valid; // @[AXI4FakeDMA.scala 112:24]
  wire  difftest_io_is_write; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_address; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_mask; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_data_0; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_data_1; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_data_2; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_data_3; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_data_4; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_data_5; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_data_6; // @[AXI4FakeDMA.scala 112:24]
  wire [63:0] difftest_io_data_7; // @[AXI4FakeDMA.scala 112:24]
  reg [7:0] state; // @[AXI4FakeDMA.scala 48:20]
  reg [63:0] address; // @[AXI4FakeDMA.scala 49:20]
  reg [63:0] mask; // @[AXI4FakeDMA.scala 50:20]
  reg [63:0] data_0; // @[AXI4FakeDMA.scala 51:20]
  reg [63:0] data_1; // @[AXI4FakeDMA.scala 51:20]
  reg [63:0] data_2; // @[AXI4FakeDMA.scala 51:20]
  reg [63:0] data_3; // @[AXI4FakeDMA.scala 51:20]
  reg [63:0] data_4; // @[AXI4FakeDMA.scala 51:20]
  reg [63:0] data_5; // @[AXI4FakeDMA.scala 51:20]
  reg [63:0] data_6; // @[AXI4FakeDMA.scala 51:20]
  reg [63:0] data_7; // @[AXI4FakeDMA.scala 51:20]
  wire  _T = state == 8'h1; // @[AXI4FakeDMA.scala 54:15]
  wire  _T_1 = io_master_req_valid & io_master_req_ready; // @[AXI4FakeDMA.scala 55:31]
  wire [7:0] _GEN_0 = io_master_req_valid & io_master_req_ready ? 8'h4 : state; // @[AXI4FakeDMA.scala 55:55 56:13 48:20]
  wire  _T_2 = state == 8'h2; // @[AXI4FakeDMA.scala 58:21]
  wire [7:0] _GEN_1 = _T_1 ? 8'h3 : state; // @[AXI4FakeDMA.scala 59:55 60:13 48:20]
  wire [7:0] _GEN_2 = io_master_resp_valid ? 8'h0 : state; // @[AXI4FakeDMA.scala 63:33 64:13 48:20]
  wire  _T_5 = state == 8'h4; // @[AXI4FakeDMA.scala 66:21]
  wire [7:0] _GEN_3 = io_master_resp_valid ? 8'h5 : state; // @[AXI4FakeDMA.scala 67:33 68:13 48:20]
  wire  _T_6 = state == 8'h5; // @[AXI4FakeDMA.scala 70:21]
  wire [7:0] _GEN_4 = state == 8'h5 ? _GEN_2 : state; // @[AXI4FakeDMA.scala 48:20 70:41]
  wire [7:0] _GEN_5 = state == 8'h4 ? _GEN_3 : _GEN_4; // @[AXI4FakeDMA.scala 66:41]
  wire [7:0] _GEN_6 = state == 8'h3 ? _GEN_2 : _GEN_5; // @[AXI4FakeDMA.scala 62:40]
  wire [7:0] _GEN_7 = state == 8'h2 ? _GEN_1 : _GEN_6; // @[AXI4FakeDMA.scala 58:34]
  wire [7:0] _GEN_8 = state == 8'h1 ? _GEN_0 : _GEN_7; // @[AXI4FakeDMA.scala 54:27]
  wire  _T_7 = io_slave_addr == 4'h8; // @[AXI4FakeDMA.scala 77:25]
  wire  _T_8 = io_slave_addr == 4'h9; // @[AXI4FakeDMA.scala 79:30]
  wire  _T_9 = io_slave_addr == 4'ha; // @[AXI4FakeDMA.scala 81:30]
  wire [63:0] _GEN_9 = 3'h0 == io_slave_addr[2:0] ? io_slave_wdata : data_0; // @[AXI4FakeDMA.scala 51:20 84:{27,27}]
  wire [63:0] _GEN_10 = 3'h1 == io_slave_addr[2:0] ? io_slave_wdata : data_1; // @[AXI4FakeDMA.scala 51:20 84:{27,27}]
  wire [63:0] _GEN_11 = 3'h2 == io_slave_addr[2:0] ? io_slave_wdata : data_2; // @[AXI4FakeDMA.scala 51:20 84:{27,27}]
  wire [63:0] _GEN_12 = 3'h3 == io_slave_addr[2:0] ? io_slave_wdata : data_3; // @[AXI4FakeDMA.scala 51:20 84:{27,27}]
  wire [63:0] _GEN_13 = 3'h4 == io_slave_addr[2:0] ? io_slave_wdata : data_4; // @[AXI4FakeDMA.scala 51:20 84:{27,27}]
  wire [63:0] _GEN_14 = 3'h5 == io_slave_addr[2:0] ? io_slave_wdata : data_5; // @[AXI4FakeDMA.scala 51:20 84:{27,27}]
  wire [63:0] _GEN_15 = 3'h6 == io_slave_addr[2:0] ? io_slave_wdata : data_6; // @[AXI4FakeDMA.scala 51:20 84:{27,27}]
  wire [63:0] _GEN_16 = 3'h7 == io_slave_addr[2:0] ? io_slave_wdata : data_7; // @[AXI4FakeDMA.scala 51:20 84:{27,27}]
  wire [63:0] _GEN_18 = io_slave_addr == 4'ha ? data_0 : _GEN_9; // @[AXI4FakeDMA.scala 51:20 81:40]
  wire [63:0] _GEN_19 = io_slave_addr == 4'ha ? data_1 : _GEN_10; // @[AXI4FakeDMA.scala 51:20 81:40]
  wire [63:0] _GEN_20 = io_slave_addr == 4'ha ? data_2 : _GEN_11; // @[AXI4FakeDMA.scala 51:20 81:40]
  wire [63:0] _GEN_21 = io_slave_addr == 4'ha ? data_3 : _GEN_12; // @[AXI4FakeDMA.scala 51:20 81:40]
  wire [63:0] _GEN_22 = io_slave_addr == 4'ha ? data_4 : _GEN_13; // @[AXI4FakeDMA.scala 51:20 81:40]
  wire [63:0] _GEN_23 = io_slave_addr == 4'ha ? data_5 : _GEN_14; // @[AXI4FakeDMA.scala 51:20 81:40]
  wire [63:0] _GEN_24 = io_slave_addr == 4'ha ? data_6 : _GEN_15; // @[AXI4FakeDMA.scala 51:20 81:40]
  wire [63:0] _GEN_25 = io_slave_addr == 4'ha ? data_7 : _GEN_16; // @[AXI4FakeDMA.scala 51:20 81:40]
  wire [63:0] _GEN_28 = io_slave_addr == 4'h9 ? data_0 : _GEN_18; // @[AXI4FakeDMA.scala 51:20 79:39]
  wire [63:0] _GEN_29 = io_slave_addr == 4'h9 ? data_1 : _GEN_19; // @[AXI4FakeDMA.scala 51:20 79:39]
  wire [63:0] _GEN_30 = io_slave_addr == 4'h9 ? data_2 : _GEN_20; // @[AXI4FakeDMA.scala 51:20 79:39]
  wire [63:0] _GEN_31 = io_slave_addr == 4'h9 ? data_3 : _GEN_21; // @[AXI4FakeDMA.scala 51:20 79:39]
  wire [63:0] _GEN_32 = io_slave_addr == 4'h9 ? data_4 : _GEN_22; // @[AXI4FakeDMA.scala 51:20 79:39]
  wire [63:0] _GEN_33 = io_slave_addr == 4'h9 ? data_5 : _GEN_23; // @[AXI4FakeDMA.scala 51:20 79:39]
  wire [63:0] _GEN_34 = io_slave_addr == 4'h9 ? data_6 : _GEN_24; // @[AXI4FakeDMA.scala 51:20 79:39]
  wire [63:0] _GEN_35 = io_slave_addr == 4'h9 ? data_7 : _GEN_25; // @[AXI4FakeDMA.scala 51:20 79:39]
  wire [63:0] _GEN_36 = io_slave_addr == 4'h8 ? io_slave_wdata : {{56'd0}, _GEN_8}; // @[AXI4FakeDMA.scala 77:34 78:13]
  wire [63:0] _GEN_39 = io_slave_addr == 4'h8 ? data_0 : _GEN_28; // @[AXI4FakeDMA.scala 51:20 77:34]
  wire [63:0] _GEN_40 = io_slave_addr == 4'h8 ? data_1 : _GEN_29; // @[AXI4FakeDMA.scala 51:20 77:34]
  wire [63:0] _GEN_41 = io_slave_addr == 4'h8 ? data_2 : _GEN_30; // @[AXI4FakeDMA.scala 51:20 77:34]
  wire [63:0] _GEN_42 = io_slave_addr == 4'h8 ? data_3 : _GEN_31; // @[AXI4FakeDMA.scala 51:20 77:34]
  wire [63:0] _GEN_43 = io_slave_addr == 4'h8 ? data_4 : _GEN_32; // @[AXI4FakeDMA.scala 51:20 77:34]
  wire [63:0] _GEN_44 = io_slave_addr == 4'h8 ? data_5 : _GEN_33; // @[AXI4FakeDMA.scala 51:20 77:34]
  wire [63:0] _GEN_45 = io_slave_addr == 4'h8 ? data_6 : _GEN_34; // @[AXI4FakeDMA.scala 51:20 77:34]
  wire [63:0] _GEN_46 = io_slave_addr == 4'h8 ? data_7 : _GEN_35; // @[AXI4FakeDMA.scala 51:20 77:34]
  wire [63:0] _GEN_47 = io_slave_wen ? _GEN_36 : {{56'd0}, _GEN_8}; // @[AXI4FakeDMA.scala 76:23]
  wire [63:0] _GEN_50 = io_slave_wen ? _GEN_39 : data_0; // @[AXI4FakeDMA.scala 51:20 76:23]
  wire [63:0] _GEN_51 = io_slave_wen ? _GEN_40 : data_1; // @[AXI4FakeDMA.scala 51:20 76:23]
  wire [63:0] _GEN_52 = io_slave_wen ? _GEN_41 : data_2; // @[AXI4FakeDMA.scala 51:20 76:23]
  wire [63:0] _GEN_53 = io_slave_wen ? _GEN_42 : data_3; // @[AXI4FakeDMA.scala 51:20 76:23]
  wire [63:0] _GEN_54 = io_slave_wen ? _GEN_43 : data_4; // @[AXI4FakeDMA.scala 51:20 76:23]
  wire [63:0] _GEN_55 = io_slave_wen ? _GEN_44 : data_5; // @[AXI4FakeDMA.scala 51:20 76:23]
  wire [63:0] _GEN_56 = io_slave_wen ? _GEN_45 : data_6; // @[AXI4FakeDMA.scala 51:20 76:23]
  wire [63:0] _GEN_57 = io_slave_wen ? _GEN_46 : data_7; // @[AXI4FakeDMA.scala 51:20 76:23]
  wire [63:0] _GEN_59 = 3'h1 == io_slave_addr[2:0] ? data_1 : data_0; // @[AXI4FakeDMA.scala 89:{10,10}]
  wire [63:0] _GEN_60 = 3'h2 == io_slave_addr[2:0] ? data_2 : _GEN_59; // @[AXI4FakeDMA.scala 89:{10,10}]
  wire [63:0] _GEN_61 = 3'h3 == io_slave_addr[2:0] ? data_3 : _GEN_60; // @[AXI4FakeDMA.scala 89:{10,10}]
  wire [63:0] _GEN_62 = 3'h4 == io_slave_addr[2:0] ? data_4 : _GEN_61; // @[AXI4FakeDMA.scala 89:{10,10}]
  wire [63:0] _GEN_63 = 3'h5 == io_slave_addr[2:0] ? data_5 : _GEN_62; // @[AXI4FakeDMA.scala 89:{10,10}]
  wire [63:0] _GEN_64 = 3'h6 == io_slave_addr[2:0] ? data_6 : _GEN_63; // @[AXI4FakeDMA.scala 89:{10,10}]
  wire [63:0] _GEN_65 = 3'h7 == io_slave_addr[2:0] ? data_7 : _GEN_64; // @[AXI4FakeDMA.scala 89:{10,10}]
  wire [63:0] _io_slave_rdata_T_4 = _T_9 ? mask : _GEN_65; // @[AXI4FakeDMA.scala 89:10]
  wire [63:0] _io_slave_rdata_T_5 = _T_8 ? address : _io_slave_rdata_T_4; // @[AXI4FakeDMA.scala 88:8]
  wire [255:0] io_master_req_data_lo = {data_3,data_2,data_1,data_0}; // @[AXI4FakeDMA.scala 95:30]
  wire [255:0] io_master_req_data_hi = {data_7,data_6,data_5,data_4}; // @[AXI4FakeDMA.scala 95:30]
  reg [7:0] last_state; // @[AXI4FakeDMA.scala 109:27]
  wire  _read_valid_T_1 = state == 8'h0; // @[AXI4FakeDMA.scala 110:59]
  wire  read_valid = last_state == 8'h5 & state == 8'h0; // @[AXI4FakeDMA.scala 110:50]
  wire  write_valid = last_state == 8'h3 & _read_valid_T_1; // @[AXI4FakeDMA.scala 111:50]
  DifftestDMATransaction difftest ( // @[AXI4FakeDMA.scala 112:24]
    .io_clock(difftest_io_clock),
    .io_coreid(difftest_io_coreid),
    .io_valid(difftest_io_valid),
    .io_is_write(difftest_io_is_write),
    .io_address(difftest_io_address),
    .io_mask(difftest_io_mask),
    .io_data_0(difftest_io_data_0),
    .io_data_1(difftest_io_data_1),
    .io_data_2(difftest_io_data_2),
    .io_data_3(difftest_io_data_3),
    .io_data_4(difftest_io_data_4),
    .io_data_5(difftest_io_data_5),
    .io_data_6(difftest_io_data_6),
    .io_data_7(difftest_io_data_7)
  );
  assign io_slave_rdata = _T_7 ? {{56'd0}, state} : _io_slave_rdata_T_5; // @[AXI4FakeDMA.scala 87:24]
  assign io_master_req_valid = io_enable & (_T | _T_2); // @[AXI4FakeDMA.scala 91:36]
  assign io_master_req_is_write = state == 8'h2; // @[AXI4FakeDMA.scala 92:35]
  assign io_master_req_addr = address; // @[AXI4FakeDMA.scala 93:22]
  assign io_master_req_mask = mask; // @[AXI4FakeDMA.scala 94:22]
  assign io_master_req_data = {io_master_req_data_hi,io_master_req_data_lo}; // @[AXI4FakeDMA.scala 95:30]
  assign difftest_io_clock = clock; // @[AXI4FakeDMA.scala 113:24]
  assign difftest_io_coreid = 8'h0; // @[AXI4FakeDMA.scala 114:24]
  assign difftest_io_valid = read_valid | write_valid; // @[AXI4FakeDMA.scala 115:38]
  assign difftest_io_is_write = last_state == 8'h3; // @[AXI4FakeDMA.scala 116:38]
  assign difftest_io_address = address; // @[AXI4FakeDMA.scala 117:24]
  assign difftest_io_mask = mask; // @[AXI4FakeDMA.scala 118:24]
  assign difftest_io_data_0 = data_0; // @[AXI4FakeDMA.scala 119:24]
  assign difftest_io_data_1 = data_1; // @[AXI4FakeDMA.scala 119:24]
  assign difftest_io_data_2 = data_2; // @[AXI4FakeDMA.scala 119:24]
  assign difftest_io_data_3 = data_3; // @[AXI4FakeDMA.scala 119:24]
  assign difftest_io_data_4 = data_4; // @[AXI4FakeDMA.scala 119:24]
  assign difftest_io_data_5 = data_5; // @[AXI4FakeDMA.scala 119:24]
  assign difftest_io_data_6 = data_6; // @[AXI4FakeDMA.scala 119:24]
  assign difftest_io_data_7 = data_7; // @[AXI4FakeDMA.scala 119:24]
  always @(posedge clock) begin
    state <= _GEN_47[7:0];
    if (io_slave_wen) begin // @[AXI4FakeDMA.scala 76:23]
      if (!(io_slave_addr == 4'h8)) begin // @[AXI4FakeDMA.scala 77:34]
        if (io_slave_addr == 4'h9) begin // @[AXI4FakeDMA.scala 79:39]
          address <= io_slave_wdata; // @[AXI4FakeDMA.scala 80:15]
        end
      end
    end
    if (io_slave_wen) begin // @[AXI4FakeDMA.scala 76:23]
      if (!(io_slave_addr == 4'h8)) begin // @[AXI4FakeDMA.scala 77:34]
        if (!(io_slave_addr == 4'h9)) begin // @[AXI4FakeDMA.scala 79:39]
          if (io_slave_addr == 4'ha) begin // @[AXI4FakeDMA.scala 81:40]
            mask <= io_slave_wdata; // @[AXI4FakeDMA.scala 82:12]
          end
        end
      end
    end
    if (io_master_resp_valid) begin // @[AXI4FakeDMA.scala 97:31]
      if (_T_5) begin // @[AXI4FakeDMA.scala 98:37]
        data_0 <= io_master_resp_bits[63:0]; // @[AXI4FakeDMA.scala 100:17]
      end else begin
        data_0 <= _GEN_50;
      end
    end else begin
      data_0 <= _GEN_50;
    end
    if (io_master_resp_valid) begin // @[AXI4FakeDMA.scala 97:31]
      if (_T_5) begin // @[AXI4FakeDMA.scala 98:37]
        data_1 <= io_master_resp_bits[127:64]; // @[AXI4FakeDMA.scala 100:17]
      end else begin
        data_1 <= _GEN_51;
      end
    end else begin
      data_1 <= _GEN_51;
    end
    if (io_master_resp_valid) begin // @[AXI4FakeDMA.scala 97:31]
      if (_T_5) begin // @[AXI4FakeDMA.scala 98:37]
        data_2 <= io_master_resp_bits[191:128]; // @[AXI4FakeDMA.scala 100:17]
      end else begin
        data_2 <= _GEN_52;
      end
    end else begin
      data_2 <= _GEN_52;
    end
    if (io_master_resp_valid) begin // @[AXI4FakeDMA.scala 97:31]
      if (_T_5) begin // @[AXI4FakeDMA.scala 98:37]
        data_3 <= io_master_resp_bits[255:192]; // @[AXI4FakeDMA.scala 100:17]
      end else begin
        data_3 <= _GEN_53;
      end
    end else begin
      data_3 <= _GEN_53;
    end
    if (io_master_resp_valid) begin // @[AXI4FakeDMA.scala 97:31]
      if (_T_5) begin // @[AXI4FakeDMA.scala 98:37]
        data_4 <= _GEN_54;
      end else if (_T_6) begin // @[AXI4FakeDMA.scala 102:42]
        data_4 <= io_master_resp_bits[63:0]; // @[AXI4FakeDMA.scala 104:21]
      end else begin
        data_4 <= _GEN_54;
      end
    end else begin
      data_4 <= _GEN_54;
    end
    if (io_master_resp_valid) begin // @[AXI4FakeDMA.scala 97:31]
      if (_T_5) begin // @[AXI4FakeDMA.scala 98:37]
        data_5 <= _GEN_55;
      end else if (_T_6) begin // @[AXI4FakeDMA.scala 102:42]
        data_5 <= io_master_resp_bits[127:64]; // @[AXI4FakeDMA.scala 104:21]
      end else begin
        data_5 <= _GEN_55;
      end
    end else begin
      data_5 <= _GEN_55;
    end
    if (io_master_resp_valid) begin // @[AXI4FakeDMA.scala 97:31]
      if (_T_5) begin // @[AXI4FakeDMA.scala 98:37]
        data_6 <= _GEN_56;
      end else if (_T_6) begin // @[AXI4FakeDMA.scala 102:42]
        data_6 <= io_master_resp_bits[191:128]; // @[AXI4FakeDMA.scala 104:21]
      end else begin
        data_6 <= _GEN_56;
      end
    end else begin
      data_6 <= _GEN_56;
    end
    if (io_master_resp_valid) begin // @[AXI4FakeDMA.scala 97:31]
      if (_T_5) begin // @[AXI4FakeDMA.scala 98:37]
        data_7 <= _GEN_57;
      end else if (_T_6) begin // @[AXI4FakeDMA.scala 102:42]
        data_7 <= io_master_resp_bits[255:192]; // @[AXI4FakeDMA.scala 104:21]
      end else begin
        data_7 <= _GEN_57;
      end
    end else begin
      data_7 <= _GEN_57;
    end
    if (reset) begin // @[AXI4FakeDMA.scala 109:27]
      last_state <= 8'h0; // @[AXI4FakeDMA.scala 109:27]
    end else begin
      last_state <= state; // @[AXI4FakeDMA.scala 109:27]
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
  state = _RAND_0[7:0];
  _RAND_1 = {2{`RANDOM}};
  address = _RAND_1[63:0];
  _RAND_2 = {2{`RANDOM}};
  mask = _RAND_2[63:0];
  _RAND_3 = {2{`RANDOM}};
  data_0 = _RAND_3[63:0];
  _RAND_4 = {2{`RANDOM}};
  data_1 = _RAND_4[63:0];
  _RAND_5 = {2{`RANDOM}};
  data_2 = _RAND_5[63:0];
  _RAND_6 = {2{`RANDOM}};
  data_3 = _RAND_6[63:0];
  _RAND_7 = {2{`RANDOM}};
  data_4 = _RAND_7[63:0];
  _RAND_8 = {2{`RANDOM}};
  data_5 = _RAND_8[63:0];
  _RAND_9 = {2{`RANDOM}};
  data_6 = _RAND_9[63:0];
  _RAND_10 = {2{`RANDOM}};
  data_7 = _RAND_10[63:0];
  _RAND_11 = {1{`RANDOM}};
  last_state = _RAND_11[7:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

