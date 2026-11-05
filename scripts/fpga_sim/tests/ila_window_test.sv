module IlaWindowSimulation;
  import "DPI-C" function void ila_window_test_initialize();
  reg ref_clock = 0;
  reg pcie_clock = 0;
  reg reset = 1;
  reg [63:0] start_cycle = 0;
  reg [63:0] finish_cycle = 40;
  reg [7:0] commit_gap = 1;
  reg corrupt = 0;
  wire cpu_clock;
  wire clock_enable;
  wire host_reset;
  wire link_enable;
  wire [63:0] cycles;
  wire [255:0] c2h_data;
  wire c2h_last, c2h_ready, c2h_valid;
  wire [255:0] h2c_data;
  wire [31:0] h2c_keep;
  wire h2c_last, h2c_ready, h2c_valid;
  wire [31:0] awaddr, wdata, araddr, rdata;
  wire [3:0] wstrb;
  wire [1:0] bresp, rresp;
  wire awvalid, awready, wvalid, wready, bvalid, bready;
  wire arvalid, arready, rvalid, rready;

  always #5 ref_clock = ~ref_clock;
  always #7 pcie_clock = ~pcie_clock;
  initial begin
    ila_window_test_initialize();
    void'($value$plusargs("start=%d", start_cycle));
    void'($value$plusargs("finish=%d", finish_cycle));
    void'($value$plusargs("gap=%d", commit_gap));
    corrupt = $test$plusargs("corrupt");
    #200 reset = 0;
  end

  DifftestClockGate gate(.CK(ref_clock), .E(clock_enable), .Q(cpu_clock));

  SimTop dut(
    .clock(cpu_clock), .reset(reset),
    .difftest_ref_clock(ref_clock), .difftest_ref_reset(reset),
    .difftest_pcie_clock(pcie_clock),
    .difftest_clock_enable(clock_enable),
    .difftest_hostCtrl_reset(host_reset), .difftest_hostCtrl_diffEnable(link_enable),
    .difftest_to_host_axis_tdata(c2h_data), .difftest_to_host_axis_tlast(c2h_last),
    .difftest_to_host_axis_tready(c2h_ready), .difftest_to_host_axis_tvalid(c2h_valid),
    .difftest_from_host_axis_tdata(h2c_data), .difftest_from_host_axis_tkeep(h2c_keep),
    .difftest_from_host_axis_tlast(h2c_last), .difftest_from_host_axis_tready(h2c_ready),
    .difftest_from_host_axis_tvalid(h2c_valid),
    .difftest_cfg_axilite_awaddr(awaddr), .difftest_cfg_axilite_awvalid(awvalid),
    .difftest_cfg_axilite_awready(awready), .difftest_cfg_axilite_wdata(wdata),
    .difftest_cfg_axilite_wstrb(wstrb), .difftest_cfg_axilite_wvalid(wvalid),
    .difftest_cfg_axilite_wready(wready), .difftest_cfg_axilite_bresp(bresp),
    .difftest_cfg_axilite_bvalid(bvalid), .difftest_cfg_axilite_bready(bready),
    .difftest_cfg_axilite_araddr(araddr), .difftest_cfg_axilite_arvalid(arvalid),
    .difftest_cfg_axilite_arready(arready), .difftest_cfg_axilite_rdata(rdata),
    .difftest_cfg_axilite_rresp(rresp), .difftest_cfg_axilite_rvalid(rvalid),
    .difftest_cfg_axilite_rready(rready),
    .difftest_perfCtrl_clean(1'b0), .difftest_perfCtrl_dump(1'b0),
    .difftest_logCtrl_begin(64'b0), .difftest_logCtrl_end(64'b0),
    .difftest_logCtrl_level(64'b0), .difftest_uart_in_ch(8'b0),
    .startCycle(start_cycle), .finishCycle(finish_cycle), .commitGap(commit_gap),
    .corrupt(corrupt), .cycles(cycles)
  );

  xdma_wrapper xdma(
    .pcie_clock(pcie_clock), .axilite_clock(ref_clock), .reset(reset),
    .axi_c2h_tdata(c2h_data), .axi_c2h_tlast(c2h_last),
    .axi_c2h_tready(c2h_ready), .axi_c2h_tvalid(c2h_valid),
    .axi_h2c_tdata(h2c_data), .axi_h2c_tkeep(h2c_keep), .axi_h2c_tlast(h2c_last),
    .axi_h2c_tready(h2c_ready), .axi_h2c_tvalid(h2c_valid),
    .cfg_awaddr(awaddr), .cfg_awvalid(awvalid), .cfg_awready(awready),
    .cfg_wdata(wdata), .cfg_wstrb(wstrb), .cfg_wvalid(wvalid), .cfg_wready(wready),
    .cfg_bresp(bresp), .cfg_bvalid(bvalid), .cfg_bready(bready),
    .cfg_araddr(araddr), .cfg_arvalid(arvalid), .cfg_arready(arready),
    .cfg_rdata(rdata), .cfg_rresp(rresp), .cfg_rvalid(rvalid), .cfg_rready(rready)
  );

  reg was_stalled = 0;
  reg [63:0] stopped_cycle = 0;
  integer debug_cycles = 0;
  always @(negedge ref_clock) begin
    if (!reset && !host_reset && link_enable && debug_cycles < 50 && $test$plusargs("debug")) begin
      $display("ILA_DEBUG cycle=%d valid=%b pre=%b val=%b squash=%b batch=%b end=%b",
               cycles, dut.endpoint.valid, dut.endpoint.preprocessed_module.out_valid,
               dut.endpoint.validated_module.out_valid, dut.endpoint.squashed_module.out_valid,
               dut.endpoint.fpgaIO_valid, dut.endpoint.fpgaIO_windowEnd);
      debug_cycles = debug_cycles + 1;
    end
    if (!reset && !host_reset && link_enable && dut.difftest_cfg.io_ilaWindowSize != 0) begin
      assert ({1'b0, cycles} <= {1'b0, dut.difftest_cfg.io_ilaWindowBase} + {1'b0, dut.difftest_cfg.io_ilaWindowSize})
        else $fatal(1, "CPU exceeded ILA window: cycle=%d base=%d size=%d", cycles,
                    dut.difftest_cfg.io_ilaWindowBase, dut.difftest_cfg.io_ilaWindowSize);
      if (dut.difftest_cfg.io_ilaWindowStalled && !was_stalled) begin
        $display("ILA_STALL cycle=%d base=%d", cycles, dut.difftest_cfg.io_ilaWindowBase);
        stopped_cycle = cycles;
      end
      if (!dut.difftest_cfg.io_ilaWindowStalled && was_stalled)
        $display("ILA_RESUME cycle=%d base=%d", cycles, dut.difftest_cfg.io_ilaWindowBase);
      if (dut.difftest_cfg.io_ilaWindowStalled && was_stalled)
        assert (cycles == stopped_cycle) else $fatal(1, "CPU advanced while stalled");
      was_stalled = dut.difftest_cfg.io_ilaWindowStalled;
    end else begin
      was_stalled = 0;
    end
  end
endmodule
