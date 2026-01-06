/***************************************************************************************
* Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

`include "DifftestMacros.svh"
module DifftestEndpoint(
  input  wire        clock,
  input  wire        reset,

`ifdef ENABLE_WORKLOAD_SWITCH
  output wire        workload_switch,
`endif // ENABLE_WORKLOAD_SWITCH

  /* DifftestTopIO */
  output wire [63:0] difftest_logCtrl_begin,
  output wire [63:0] difftest_logCtrl_end,
  output wire [63:0] difftest_logCtrl_level,
  output wire        difftest_perfCtrl_clean,
  output wire        difftest_perfCtrl_dump,
  input  wire        difftest_uart_out_valid,
  input  wire [ 7:0] difftest_uart_out_ch,
  output wire        difftest_uart_in_valid,
  output wire [ 7:0] difftest_uart_in_ch,
  input  wire [63:0] difftest_exit,
  input  wire [`CONFIG_DIFFTEST_STEPWIDTH - 1:0] difftest_step
);

`ifndef TB_NO_DPIC
import "DPI-C" function void set_bin_file(string bin);
import "DPI-C" function void set_copy_ram_offset(string bin);
import "DPI-C" function void set_flash_bin(string bin);
import "DPI-C" function void set_gcpt_bin(string bin);
import "DPI-C" function void set_diff_ref_so(string diff_so);
import "DPI-C" function void set_no_diff();
import "DPI-C" function void set_simjtag();
import "DPI-C" function byte simv_init();
import "DPI-C" function void set_max_instrs(longint mc);
import "DPI-C" function longint get_stuck_limit();
import "DPI-C" function void set_overwrite_nbytes(longint len);
import "DPI-C" function void set_ram_size(string size);
import "DPI-C" function void set_overwrite_autoset();
import "DPI-C" function void set_warmup_instr(longint instrs);
import "DPI-C" function void set_ref_trace();
import "DPI-C" function void set_commit_trace();
`ifdef WITH_DRAMSIM3
import "DPI-C" function void simv_tick();
`endif // WITH_DRAMSIM3
`ifdef ENABLE_WORKLOAD_SWITCH
import "DPI-C" function void set_workload_list(string path);
`endif // ENABLE_WORKLOAD_SWITCH
import "DPI-C" function byte workload_list_completed();
`ifndef CONFIG_DIFFTEST_DEFERRED_RESULT
import "DPI-C" function byte simv_nstep(byte step);
`endif // CONFIG_DIFFTEST_DEFERRED_RESULT
`ifdef CONFIG_DIFFTEST_IOTRACE
import "DPI-C" function void set_iotrace_name(string name);
`endif // CONFIG_DIFFTEST_IOTRACE
`endif // TB_NO_DPIC

`define SIMV_GOODTRAP 8'h1
`define SIMV_EXCEED   8'h2
`define SIMV_FAIL     8'h3
`define SIMV_WARMUP   8'h4

reg  [63:0] difftest_logCtrl_begin_r;
reg  [63:0] difftest_logCtrl_end_r;
reg difftest_perfCtrl_clean_r;

string bin_file;
string copy_ram_offset;
string flash_bin_file;
string gcpt_bin_file;
string diff_ref_so;
string workload_list;
string iotrace_name;
longint overwrite_nbytes;
string ram_size;

reg [63:0] max_instrs;
reg [63:0] max_cycles;
reg [63:0] warmup_instr;
reg [63:0] stuck_limit;

initial begin
  // log begin
  if ($test$plusargs("b")) begin
    $value$plusargs("b=%d", difftest_logCtrl_begin_r);
  end
  else begin
    difftest_logCtrl_begin_r = 0;
  end
  // log end
  if ($test$plusargs("e")) begin
    $value$plusargs("e=%d", difftest_logCtrl_end_r);
  end
  else begin
    difftest_logCtrl_end_r = 0;
  end
  stuck_limit = 0;
`ifndef TB_NO_DPIC
  stuck_limit = get_stuck_limit();
  // workload: bin file
  if ($test$plusargs("workload")) begin
    $value$plusargs("workload=%s", bin_file);
    set_bin_file(bin_file);
  end
  // copy the ram to offset
  if ($test$plusargs("copy-ram")) begin
    $value$plusargs("copy-ram=%s", copy_ram_offset);
    set_copy_ram_offset(copy_ram_offset);
  end
  // warmup for instrs
  warmup_instr = 0;
  if ($test$plusargs("warmup_instr")) begin
    $value$plusargs("warmup_instr=%d", warmup_instr);
    set_warmup_instr(warmup_instr);
  end
  // boot flash image: bin file
  if ($test$plusargs("flash")) begin
    $value$plusargs("flash=%s", flash_bin_file);
    set_flash_bin(flash_bin_file);
  end
  // size of the gcpt used
  if ($test$plusargs("overwrite_nbytes")) begin
    $value$plusargs("overwrite_nbytes=%d", overwrite_nbytes);
    set_overwrite_nbytes(overwrite_nbytes);
  end
  // size of the gcpt used
  if ($test$plusargs("ram_size")) begin
    $value$plusargs("ram_size=%s", ram_size);
    set_ram_size(ram_size);
  end
  // auto set gcpt used size
  if ($test$plusargs("overwrite_autoset")) begin
    set_overwrite_autoset();
  end
  // overwrite gcpt on ram: bin file
  if ($test$plusargs("gcpt-restore")) begin
    $value$plusargs("gcpt-restore=%s", gcpt_bin_file);
    set_gcpt_bin(gcpt_bin_file);
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
  // enable sim-jtag
  if ($test$plusargs("enable-jtag")) begin
    set_simjtag();
  end
  // set exit instrs const
  if ($test$plusargs("max-instrs")) begin
    $value$plusargs("max-instrs=%d", max_instrs);
    set_max_instrs(max_instrs);
  end
  // set trace debug support
  if ($test$plusargs("dump-ref-trace")) begin
    set_ref_trace();
  end
  if ($test$plusargs("dump-commit-trace")) begin
    set_commit_trace();
  end
`ifdef CONFIG_DIFFTEST_IOTRACE
  // set difftest iotrace directory path
  if ($test$plusargs("iotrace-name")) begin
    $value$plusargs("iotrace-name=%s", iotrace_name);
    set_iotrace_name(iotrace_name);
  end
`endif // CONFIG_DIFFTEST_IOTRACE
`ifdef ENABLE_WORKLOAD_SWITCH
  // set workload list
  if ($test$plusargs("workload-list")) begin
    $value$plusargs("workload-list=%s", workload_list);
    set_workload_list(workload_list);
  end
  else begin
    $display("workload switch is enabled but the workload list is not set");
    $fatal;
  end
`endif // ENABLE_WORKLOAD_SWITCH
`endif // TB_NO_DPIC
  // max cycles to execute, no limit for default
  max_cycles = 0;
  if ($test$plusargs("max-cycles")) begin
    $value$plusargs("max-cycles=%d", max_cycles);
    $display("set max cycles: %d", max_cycles);
  end
end

/*
 * cycle counter and stuck/max-cycle detect
 */
reg [63:0] n_cycles;
reg [63:0] stuck_timer;
always @(posedge clock) begin
  if (reset) begin
    n_cycles <= 64'h0;
    stuck_timer <= 64'h0;
  end
  else begin
    n_cycles <= n_cycles + 64'h1;

    // max cycles
    if (max_cycles > 0 && n_cycles >= max_cycles) begin
      $display("EXCEEDED MAX CYCLE: %d", max_cycles);
      $finish();
    end

    // stuck check
    if (difftest_step)
      stuck_timer <= 0;
    else
      stuck_timer <= stuck_timer + 64'h1;

    if (stuck_limit > 0 && stuck_timer >= stuck_limit) begin
      $display("No difftest Check for more than %d cycles, maybe get stuck", stuck_limit);
      $fatal;
    end
  end
end

/*
 * difftest exit signal check
 */
always @(posedge clock) begin
    // exit signal: all 1's for normal exit; others are error
    if (difftest_exit == 64'hffff_ffff_ffff_ffff) begin
      $display("The simulation exits normally");
      $finish();
    end
    else if (difftest_exit != 0) begin
      $display("The simulation aborts: error code 0x%x", difftest_exit);
      $fatal;
    end
end

/*
 * progress simulation
 */
`ifndef TB_NO_DPIC
always @(posedge clock) begin
  if (!reset) begin
`ifdef WITH_DRAMSIM3
    /* tick DRAMSIM3 if required */
    if (n_cycles) begin
      simv_tick();
    end
`endif // WITH_DRAMSIM3

    // difftest
    if (!n_cycles) begin
      if (simv_init()) begin
        if (workload_list_completed()) begin
          $display("DIFFTEST WORKLOAD LIST DONE at cycle %d", n_cycles);
          $finish();
        end
        else begin
          $display("DIFFTEST INIT FAILED");
          $fatal;
        end
      end
    end
  end
end

/* simulation step control */
`ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
wire [7:0] simv_result;
DeferredControl deferred(
  .clock(clock),
  .reset(reset),
`ifndef CONFIG_DIFFTEST_INTERNAL_STEP
  .step(difftest_step),
`endif // CONFIG_DIFFTEST_INTERNAL_STEP
  .simv_result(simv_result)
);
`else
reg [7:0] simv_result;
`ifdef PALLADIUM
/*
 * In PALLADIUM, we delay the step signal to next cycle to make sure
 * `simv_step` was triggered after other DPI calls, which needs more
 * trick to be correct. Such as introducing `ping-pong buffer` to
 * handle the delay-step, and dpics at the next cycle coming together.
 */
always @(posedge clock) begin
  if (reset || simv_result == `SIMV_GOODTRAP || simv_result == `SIMV_EXCEED) begin
    simv_result <= 8'b0;
  end
  else begin
    if (n_cycles && |difftest_step) begin
      simv_result <= simv_nstep(difftest_step);
    end
  end
end
`else
/*
 * for other platform, we introduce a delayed difftest step
 * mechanism to make sure all difftest state was updated properly
 * before `simv_step()` called.
 */
reg [7:0] _res;
event simv_step_event;
// check difftest_step
always @(posedge clock) begin
  if (!reset) begin
    if (n_cycles && |difftest_step) begin
      // delay a little before trigger the simv step event
      #0.1 -> simv_step_event;
    end
  end
end
// all difftest state was updated, step difftest by `simv_nstep()`
always @(simv_step_event or posedge reset) begin
  if (reset)
    _res <= 8'b0;
  else
    _res <= simv_nstep(difftest_step);
end
// update to `simv_result` at next cycle
always @(posedge clock) begin
  if (reset || simv_result == `SIMV_GOODTRAP || simv_result == `SIMV_EXCEED) begin
    simv_result <= 8'b0;
  end
  else begin
    simv_result <= _res;
  end
end
`endif // PALLADIUM
`endif // CONFIG_DIFFTEST_DEFERRED_RESULT

/*
 * difftest result check
 */
always @(posedge clock) begin
  if (!reset) begin
    if (simv_result == `SIMV_FAIL) begin
      $display("DIFFTEST FAILED at cycle %d", n_cycles);
      $fatal;
    end
    else if (simv_result == `SIMV_GOODTRAP || simv_result == `SIMV_EXCEED) begin
      $display("DIFFTEST WORKLOAD DONE at cycle %d", n_cycles);
`ifndef ENABLE_WORKLOAD_SWITCH
`ifndef NO_FINISH_AFTER_WORKLOAD
      $finish();
`endif // NO_FINISH_AFTER_WORKLOAD
`endif // ENABLE_WORKLOAD_SWITCH
    end
  end
end
`endif // TB_NO_DPIC

/*
 * workload switch
 */
`ifdef ENABLE_WORKLOAD_SWITCH
assign workload_switch = simv_result == `SIMV_GOODTRAP || simv_result == `SIMV_EXCEED;
`endif // ENABLE_WORKLOAD_SWITCH

/*
 * uart output
 */
assign difftest_uart_in_ch = 8'hff;
always @(posedge clock) begin
  if (!reset && difftest_uart_out_valid) begin
    $fwrite(32'h8000_0001, "%c", difftest_uart_out_ch);
    $fflush();
  end
end

/*
 * perf/log ctrl
 */
assign difftest_logCtrl_begin = difftest_logCtrl_begin_r;
assign difftest_logCtrl_end = difftest_logCtrl_end_r;
assign difftest_logCtrl_level = 0;

`ifndef TB_NO_DPIC
assign difftest_perfCtrl_clean = simv_result == `SIMV_WARMUP;
assign difftest_perfCtrl_dump = simv_result == `SIMV_GOODTRAP || simv_result == `SIMV_EXCEED || simv_result == `SIMV_FAIL;
`else
assign difftest_perfCtrl_clean = 0;
assign difftest_perfCtrl_dump = 0;
`endif // TB_NO_DPIC

endmodule
