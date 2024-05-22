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
      // data0 <= 0;
      // data1 <= 0;
      // data2 <= 0;
      // data3 <= 0;
      // data4 <= 0;
      // data5 <= 0;
      // data6 <= 0;
      // data7 <= 0;
      // trace_icache_helper(addr, legal_addr, data0, data1, data2, data3, data4, data5, data6, data7);
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
