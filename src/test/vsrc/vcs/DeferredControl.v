`include "DifftestMacros.svh"
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

reg [7:0] _res;
initial begin
  set_deferred_result_scope();
  _res = 8'b0;
end

export "DPI-C" function set_deferred_result;
function void set_deferred_result(byte result);
  _res = result;
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
`endif // CONFIG_DIFFTEST_INTERNAL_STEP


always @(posedge clock) begin
  if (reset) begin
    simv_result <= 8'b0;
  end
  else begin
    simv_result <= _res;
`ifndef CONFIG_DIFFTEST_INTERNAL_STEP
    if (step != 0) begin
      simv_nstep(step);
    end
`endif // CONFIG_DIFFTEST_INTERNAL_STEP
  end
end

endmodule;
`endif // CONFIG_DIFFTEST_DEFERRED_RESULT
`endif // TB_NO_DPIC
