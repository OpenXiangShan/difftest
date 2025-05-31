module xdma_clock(
  input clock,
  input reset,
  input core_clock_enable,
  output core_clock
);

`ifdef ASYNC_CLK_2N
// core_clock = clock / (2 * ASYNC_CLK_2N)
reg core_clock_r;
reg [7:0] clk_cnt;
initial begin
  core_clock_r = 0;
  clk_cnt = 0;
end
always @(posedge clock) begin
  if (clk_cnt == `ASYNC_CLK_2N - 1) begin
    clk_cnt <= 0;
    core_clock_r <= ~core_clock_r;
  end
  else begin
    clk_cnt <= clk_cnt + 1;
  end
end
assign core_clock = core_clock_r & core_clock_enable;
`else
assign core_clock = clock & core_clock_enable;
`endif // ASYNC_CLK_2N
endmodule
