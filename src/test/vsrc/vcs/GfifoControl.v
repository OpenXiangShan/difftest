`ifdef PALLADIUM_GFIFO
module GfifoControl(
  output reg simv_result
);

initial begin
  simv_result = 1'b0;
end

// For the C/C++ interface
export "DPI-C" task set_simv_result;
task set_simv_result(int res);
  simv_result = res;
endtask

endmodule;
`endif // PALLADIUM_GFIFO