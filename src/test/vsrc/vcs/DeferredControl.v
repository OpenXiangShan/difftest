`include "DifftestMacros.v"
`ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
module DeferredControl(
  input clock,
  input reset,
  input [`CONFIG_DIFFTEST_STEPWIDTH - 1:0] step,
  output reg simv_result
);

import "DPI-C" function int simv_result_fetch();
import "DPI-C" function void simv_nstep(int step);

`ifdef CONFIG_DIFFTEST_NONBLOCK
`ifdef PALLADIUM
initial $ixc_ctrl("gfifo", "simv_nstep");
`endif // PALLADIUM
`endif // CONFIG_DIFFTEST_NONBLOCK

reg [63:0] fetch_cycles;
initial fetch_cycles = 4999;

reg [63:0] fetch_timer;
always @(posedge clock) begin
  if (reset) begin
    simv_result <= 1'b0;
    fetch_timer <= 64'b0;
  end
  else begin
    if (fetch_timer == fetch_cycles) begin
      fetch_timer <= 1'b0;
      if (simv_result_fetch()) begin
        simv_result <= 1'b1;
      end
    end
    else begin
      fetch_timer <= fetch_timer + 64'b1;
    end

    if ((~simv_result) && (|step)) begin
      simv_nstep(step);
    end
  end
end

endmodule;
`endif // CONFIG_DIFFTEST_DEFERRED_RESULT
