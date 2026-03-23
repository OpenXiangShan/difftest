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
  input  longint vsatp,
  input  longint hgatp,
  input  longint vpn,
  input  byte    s2xlate,
  output longint pte,
  output byte    level,
  output longint s1_pte,
  output longint s2_pte,
  output byte    s1_level
);
`endif // TB_NO_DPIC

module PTEHelper(
  input             clock,
  input             enable,
  input      [63:0] satp,
  input      [63:0] vsatp,
  input      [63:0] hgatp,
  input      [63:0] vpn,
  input      [ 7:0] s2xlate,
  output     [63:0] pte,
  output     [ 7:0] level,
  output     [ 7:0] pf,
  output     [63:0] s1_pte,
  output     [63:0] s2_pte,
  output     [ 7:0] s1_level
);

  reg [63:0] pte_comb;
  reg [ 7:0] level_comb;
  reg [63:0] s1_pte_comb;
  reg [63:0] s2_pte_comb;
  reg [ 7:0] s1_level_comb;
  reg [ 7:0] pf_comb;

`ifndef TB_NO_DPIC
  always @(*) begin
    if (enable) begin
      pf_comb = pte_helper(satp, vsatp, hgatp, vpn, s2xlate,
                           pte_comb, level_comb,
                           s1_pte_comb, s2_pte_comb, s1_level_comb);
    end else begin
      pf_comb       = 8'h0;
      pte_comb      = 64'h0;
      level_comb    = 8'h0;
      s1_pte_comb   = 64'h0;
      s2_pte_comb   = 64'h0;
      s1_level_comb = 8'h0;
    end
  end
`else
  always @(*) begin
    pf_comb       = 8'b0;
    pte_comb      = 64'b0;
    level_comb    = 8'b0;
    s1_pte_comb   = 64'b0;
    s2_pte_comb   = 64'b0;
    s1_level_comb = 8'b0;
  end
`endif // TB_NO_DPIC

  assign pte      = pte_comb;
  assign level    = level_comb;
  assign pf       = pf_comb;
  assign s1_pte   = s1_pte_comb;
  assign s2_pte   = s2_pte_comb;
  assign s1_level = s1_level_comb;

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
