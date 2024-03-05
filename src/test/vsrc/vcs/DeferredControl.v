`include "DifftestMacros.v"
`ifndef TB_NO_DPIC
`ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
module DeferredControl(
  input clock,
  input reset,
`ifndef CONFIG_DIFFTEST_INTERNAL_STEP
  input [`CONFIG_DIFFTEST_STEPWIDTH - 1:0] step,
`endif // CONFIG_DIFFTEST_INTERNAL_STEP
  output reg [7:0] simv_result
);

import "DPI-C" context function void set_deferred_result_scope();

initial begin
  set_deferred_result_scope();
  simv_result = 8'b0;
end

export "DPI-C" function set_deferred_result;
function void set_deferred_result(byte result);
  simv_result = result;
endfunction

`ifdef PALLADIUM
initial $ixc_ctrl("sfifo", "set_deferred_result");
`endif // PALLADIUM

`ifndef CONFIG_DIFFTEST_INTERNAL_STEP
import "DPI-C" function void simv_nstep(int step);

`ifdef CONFIG_DIFFTEST_NONBLOCK
`ifdef PALLADIUM
initial $ixc_ctrl("gfifo", "simv_nstep");
`endif // PALLADIUM
`endif // CONFIG_DIFFTEST_NONBLOCK

always @(posedge clock) begin
  if (reset) begin
    simv_result <= 8'b0;
  end
  else if (step != 0) begin
    simv_nstep(step);
  end
end
`endif // CONFIG_DIFFTEST_INTERNAL_STEP

endmodule;
`endif // CONFIG_DIFFTEST_DEFERRED_RESULT
`endif // TB_NO_DPIC
