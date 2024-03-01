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
