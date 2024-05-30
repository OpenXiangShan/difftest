/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

`ifndef TB_NO_DPIC
import "DPI-C" function byte pte_helper (
  input  longint satp,
  input  longint vpn,
  output longint pte,
  output byte    level
);
`endif // TB_NO_DPIC

module PTEHelper(
  input             clock,
  input             enable,
  input      [63:0] satp,
  input      [63:0] vpn,
  output reg [63:0] pte,
  output reg [ 7:0] level,
  output reg [ 7:0] pf
);
  always @(posedge clock) begin
    if (enable) begin
`ifndef TB_NO_DPIC
      pf <= pte_helper(satp, vpn, pte, level);
`endif // TB_NO_DPIC
    end
  end
endmodule

`ifndef TB_NO_DPIC
// Read Half CacheLine 256bits
import "DPI-C" function void trace_icache_helper(
  input longint addr,
  output byte    legal_addr,
  output longint data0,
  output longint data1,
  output longint data2,
  output longint data3,
  output longint data4,
  output longint data5,
  output longint data6,
  output longint data7
);

import "DPI-C" function longint trace_icache_dword_helper(
  input longint addr
);
import "DPI-C" function byte trace_icache_legal_addr(
  input longint addr
);
`endif // TB_NO_DPIC

module TraceICacheHelper(
  input             clock,
  input             enable,
  input      [63:0] addr,
  output reg [63:0] data0,
  output reg [63:0] data1,
  output reg [63:0] data2,
  output reg [63:0] data3,
  output reg [63:0] data4,
  output reg [63:0] data5,
  output reg [63:0] data6,
  output reg [63:0] data7,
  output reg [7:0]  legal_addr
);

  wire [63:0] addr_align = addr & 64'hffffffffffffffe0;
  always @(posedge clock) begin
    if (enable) begin
`ifndef TB_NO_DPIC
      legal_addr <= trace_icache_legal_addr(addr_align);
      data0 <= trace_icache_dword_helper(addr_align + 0 * 8);
      data1 <= trace_icache_dword_helper(addr_align + 1 * 8);
      data2 <= trace_icache_dword_helper(addr_align + 2 * 8);
      data3 <= trace_icache_dword_helper(addr_align + 3 * 8);
      data4 <= trace_icache_dword_helper(addr_align + 4 * 8);
      data5 <= trace_icache_dword_helper(addr_align + 5 * 8);
      data6 <= trace_icache_dword_helper(addr_align + 6 * 8);
      data7 <= trace_icache_dword_helper(addr_align + 7 * 8);
      // $display("TraceICacheHelper: addr=%h, legal_addr=%h, data0=%h, data1=%h, data2=%h, data3=%h\n", addr, legal_addr, data0, data1, data2, data3);
`endif // TB_NO_DPIC
    end
  end
endmodule

`ifndef TB_NO_DPIC
// Read Trace Instruction
import "DPI-C" function void trace_read_one_instr(
  input byte enable,
  output longint pc,
  output int instr
);
`endif // TB_NO_DPIC

module TraceReaderHelper(
  input             clock,
  input             reset,
  input             enable,
  output [63:0] pc_0,
  output [63:0] pc_1,
  output [63:0] pc_2,
  output [63:0] pc_3,
  output [63:0] pc_4,
  output [63:0] pc_5,
  output [63:0] pc_6,
  output [63:0] pc_7,
  output [63:0] pc_8,
  output [63:0] pc_9,
  output [63:0] pc_10,
  output [63:0] pc_11,
  output [63:0] pc_12,
  output [63:0] pc_13,
  output [63:0] pc_14,
  output [63:0] pc_15,
  output [31:0] instr_0,
  output [31:0] instr_1,
  output [31:0] instr_2,
  output [31:0] instr_3,
  output [31:0] instr_4,
  output [31:0] instr_5,
  output [31:0] instr_6,
  output [31:0] instr_7,
  output [31:0] instr_8,
  output [31:0] instr_9,
  output [31:0] instr_10,
  output [31:0] instr_11,
  output [31:0] instr_12,
  output [31:0] instr_13,
  output [31:0] instr_14,
  output [31:0] instr_15
);

  always @(posedge clock) begin
    if (enable && !reset) begin
      // FIXME: Trace Read is not thread-safe, so we need read within one method
      trace_read_one_instr(enable, pc_0,  instr_0);
      trace_read_one_instr(enable, pc_1,  instr_1);
      trace_read_one_instr(enable, pc_2,  instr_2);
      trace_read_one_instr(enable, pc_3,  instr_3);
      trace_read_one_instr(enable, pc_4,  instr_4);
      trace_read_one_instr(enable, pc_5,  instr_5);
      trace_read_one_instr(enable, pc_6,  instr_6);
      trace_read_one_instr(enable, pc_7,  instr_7);
      trace_read_one_instr(enable, pc_8,  instr_8);
      trace_read_one_instr(enable, pc_9,  instr_9);
      trace_read_one_instr(enable, pc_10, instr_10);
      trace_read_one_instr(enable, pc_11, instr_11);
      trace_read_one_instr(enable, pc_12, instr_12);
      trace_read_one_instr(enable, pc_13, instr_13);
      trace_read_one_instr(enable, pc_14, instr_14);
      trace_read_one_instr(enable, pc_15, instr_15);
    end
  end
endmodule


`ifndef TB_NO_DPIC
import "DPI-C" function longint amo_helper(
  input byte    cmd,
  input longint addr,
  input longint wdata,
  input byte    mask
);
`endif // TB_NO_DPIC

module AMOHelper(
  input             clock,
  input             enable,
  input      [ 4:0] cmd,
  input      [63:0] addr,
  input      [63:0] wdata,
  input      [ 7:0] mask,
  output reg [63:0] rdata
);

  always @(posedge clock) begin
    if (enable) begin
`ifndef TB_NO_DPIC
      rdata <= amo_helper(cmd, addr, wdata, mask);
`endif // TB_NO_DPIC
    end
  end

endmodule
