module xdma_wrapper(
  input clock,
  input reset,
  input [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] difftest_data,
  input difftest_enable,
  output core_clock
);

  wire core_clock_enable;
  wire [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] axi_tdata;
  wire [63:0] axi_tkeep;
  wire axi_tlast;
  wire axi_tready;
  wire axi_tvalid;
xdma_clock xclock(
  .clock(clock),
  .reset(reset),
  .core_clock_enable(core_clock_enable),
  .core_clock(core_clock)
);
xdma_ctrl xctrl(
  .clock(clock),
  .reset(reset),
  .difftest_data(difftest_data),
  .difftest_enable(difftest_enable),
  .core_clock_enable(core_clock_enable),
  .axi_tdata(axi_tdata),
  .axi_tkeep(axi_tkeep),
  .axi_tlast(axi_tlast),
  .axi_tready(axi_tready),
  .axi_tvalid(axi_tvalid)
);
xdma_axi xaxi(
  .clock(clock),
  .reset(reset),
  .axi_tdata(axi_tdata),
  .axi_tkeep(axi_tkeep),
  .axi_tlast(axi_tlast),
  .axi_tready(axi_tready),
  .axi_tvalid(axi_tvalid)
);

endmodule
