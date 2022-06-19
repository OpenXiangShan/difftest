/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
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

import "DPI-C" function void set_bin_file(string bin);
import "DPI-C" function void set_flash_bin(string bin);
import "DPI-C" function void set_diff_ref_so(string diff_so);
import "DPI-C" function void set_no_diff();
import "DPI-C" function void set_max_cycles(int mc);
import "DPI-C" function void simv_init();
import "DPI-C" function int simv_step();

module tb_top();

reg         clock;
reg         reset;
reg  [63:0] io_logCtrl_log_begin;
reg  [63:0] io_logCtrl_log_end;
wire [63:0] io_logCtrl_log_level;
wire        io_perfInfo_clean;
wire        io_perfInfo_dump;
wire        io_uart_out_valid;
wire [ 7:0] io_uart_out_ch;
wire        io_uart_in_valid;
wire [ 7:0] io_uart_in_ch;

string bin_file;
string flash_bin_file;
string wave_type;
string diff_ref_so;
reg [31:0] max_cycles;

initial begin
  clock = 0;
  reset = 1;
  // enable waveform
  if ($test$plusargs("dump-wave")) begin
    $value$plusargs("dump-wave=%s", wave_type);
    if (wave_type == "vpd") begin
      $vcdplusfile("simv.vpd");
      $vcdpluson;
    end
`ifdef CONSIDER_FSDB
    else if (wave_type == "fsdb") begin
      $fsdbDumpfile("simv.fsdb");
      $fsdbDumpvars(0,"+mda");
    end
`endif
    else begin
      $display("unknown wave file format, want [vpd, fsdb] but:%s\n", wave_type);
      $finish();
    end
  end
  // log begin
  if ($test$plusargs("b")) begin
    $value$plusargs("b=%d", io_logCtrl_log_begin);
  end
  else begin
    io_logCtrl_log_begin = 0;
  end
  // log end
  if ($test$plusargs("e")) begin
    $value$plusargs("e=%d", io_logCtrl_log_end);
  end
  else begin
    io_logCtrl_log_end = 0;
  end
  // workload: bin file
  if ($test$plusargs("workload")) begin
    $value$plusargs("workload=%s", bin_file);
    set_bin_file(bin_file);
  end
  // boot flash image: bin file
  if ($test$plusargs("flash")) begin
    $value$plusargs("flash=%s", flash_bin_file);
    set_flash_bin(flash_bin_file);
  end
  // diff-test golden model: nemu-so
  if ($test$plusargs("diff")) begin
    $value$plusargs("diff=%s", diff_ref_so);
    set_diff_ref_so(diff_ref_so);
  end
  // disable diff-test
  if ($test$plusargs("no-diff")) begin
    set_no_diff();
  end
  // max cycles to execute, no limit for default
  if ($test$plusargs("max-cycles")) begin
    $value$plusargs("max-cycles=%d", max_cycles);
    set_max_cycles(max_cycles);
  end
  else begin
    max_cycles = 0;
  end

  // Note: reset delay #100 should be larger than RANDOMIZE_DELAY
  #100 reset = 0;
end
always #1 clock <= ~clock;

SimTop sim(
  .clock(clock),
  .reset(reset),
  .io_logCtrl_log_begin(io_logCtrl_log_begin),
  .io_logCtrl_log_end(io_logCtrl_log_end),
  .io_logCtrl_log_level(io_logCtrl_log_level),
  .io_perfInfo_clean(io_perfInfo_clean),
  .io_perfInfo_dump(io_perfInfo_dump),
  .io_uart_out_valid(io_uart_out_valid),
  .io_uart_out_ch(io_uart_out_ch),
  .io_uart_in_valid(io_uart_in_valid),
  .io_uart_in_ch(io_uart_in_ch)
);

assign io_logCtrl_log_level = 0;
assign io_perfInfo_clean = 0;
assign io_perfInfo_dump = 0;
assign io_uart_in_ch = 8'hff;

always @(posedge clock) begin
  if (!reset && io_uart_out_valid) begin
    $fwrite(32'h8000_0001, "%c", io_uart_out_ch);
    $fflush();
  end
end

reg has_init;
always @(posedge clock) begin
  if (reset) begin
    has_init <= 1'b0;
  end
  else if (!has_init) begin
    simv_init();
    has_init <= 1'b1;
  end

  // check errors
  if (!reset && has_init) begin
    if (simv_step()) begin
      $finish();
    end
  end

end

endmodule

