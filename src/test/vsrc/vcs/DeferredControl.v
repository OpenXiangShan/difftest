`include "DifftestMacros.v"
`ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
module DeferredControl(
  input clock,
  input reset,
  input [`CONFIG_DIFFTEST_STEPWIDTH - 1:0] step,
  output reg simv_result
);

import "DPI-C" function void simv_nstep(int step);
import "DPI-C" context function void set_deferred_result_scope();

initial begin
  set_deferred_result_scope();
  simv_result = 1'b0;
end

export "DPI-C" function set_deferred_result;
function void set_deferred_result();
  simv_result = 1'b1;
endfunction

`ifdef CONFIG_DIFFTEST_NONBLOCK
`ifdef PALLADIUM
initial $ixc_ctrl("gfifo", "simv_nstep");
`endif // PALLADIUM
`endif // CONFIG_DIFFTEST_NONBLOCK

`ifdef PALLADIUM
initial $ixc_ctrl("sfifo", "set_deferred_result");
`endif // PALLADIUM

always @(posedge clock) begin
  if (!reset && !simv_result && step != 0) begin
    simv_nstep(step);
  end
end

endmodule;
`endif // CONFIG_DIFFTEST_DEFERRED_RESULT
