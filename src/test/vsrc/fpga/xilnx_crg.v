module xilnx_crg(
   input          sys_clk,
   input          dev_clk,
   input          tmclk,
   input          cqetmclk,
   input          sys_rstn,

   output         axi_bus_clk,
   output         axi_bclk_sync_rstn,

   output         ddr_bus_clk, 
   output         ddr_bclk_sync_rstn,
   
   output         uart_pclk,
   output         uart_pclk_sync_rstn,
   output         uart_sclk,
   output         uart_sclk_sync_rstn,
  
   output         qspi_sclk,
   output         qspi_pclk,
   output         qspi_pclk_sync_rstn,
   output         qspi_hclk,   
   output         qspi_hclk_sync_rstn,   
   output         qspi_ref_clk,
   output         qspi_rclk_sync_rstn,   

   output         sd_axi_clk,
   output         sd_aclk_sync_rstn,
   output         sd_ahb_clk,
   output         sd_hclk_sync_rstn,
   output         sd_bclk,
   output         sd_bclk_sync_rstn,
   output         sd_tmclk,
   output         sd_tclk_sync_rstn,
   output         sd_cqetmclk,
   output         sd_cqetclk_sync_rstn

);

//------------- clock signals --------------------------

assign   ddr_bus_clk   =   sys_clk;
assign   uart_pclk     =   sys_clk;
assign   qspi_pclk     =   sys_clk;
assign   qspi_hclk     =   sys_clk;
assign   sd_axi_clk    =   sys_clk;
assign   sd_ahb_clk    =   sys_clk;
assign   sd_bclk       =   sys_clk;
assign   axi_bus_clk   =   sys_clk;

assign   uart_sclk     =   dev_clk;
assign   qspi_ref_clk  =   dev_clk;
assign   qspi_sclk     =   dev_clk;

assign   sd_tmclk      =   tmclk;
assign   sd_cqetmclk   =   cqetmclk;

//-------------- reset signals -------------------------
wire                        sysclk_sync_rstn;
wire                        devclk_sync_rstn;
wire                        tmclk_sync_rstn;
wire                        cqetmclk_sync_rstn;

RST_SYNC #(
   .SYNC_STAGES     (3),
   .PIPELINE_STAGES (1),
   .INIT            (1'b0)
) sysclk_rstn (
   .clk     (sys_clk),
   .async_in(sys_rstn),
   .sync_out(sysclk_sync_rstn)
);
assign    ddr_bclk_sync_rstn    =   sysclk_sync_rstn;
assign    uart_pclk_sync_rstn   =   sysclk_sync_rstn;
assign    qspi_pclk_sync_rstn   =   sysclk_sync_rstn;
assign    qspi_hclk_sync_rstn   =   sysclk_sync_rstn;
assign    uart_hclk_sync_rstn   =   sysclk_sync_rstn;
assign    sd_aclk_sync_rstn     =   sysclk_sync_rstn;
assign    sd_hclk_sync_rstn     =   sysclk_sync_rstn;
assign    sd_bclk_sync_rstn     =   sysclk_sync_rstn;
assign    axi_bclk_sync_rstn    =   sysclk_sync_rstn;

RST_SYNC #(
   .SYNC_STAGES     (3),
   .PIPELINE_STAGES (1),
   .INIT            (1'b0)
) devclk_rstn (
   .clk     (dev_clk),
   .async_in(sys_rstn),
   .sync_out(devclk_sync_rstn)
);

assign    uart_sclk_sync_rstn    =   devclk_sync_rstn;
assign    qspi_rclk_sync_rstn    =   devclk_sync_rstn;

RST_SYNC #(
   .SYNC_STAGES     (3),
   .PIPELINE_STAGES (1),
   .INIT            (1'b0)
) tmclk_rstn (
   .clk     (tmclk),
   .async_in(sys_rstn),
   .sync_out(tmclk_sync_rstn)
);
assign    sd_tclk_sync_rstn       =   tmclk_sync_rstn;

RST_SYNC #(
   .SYNC_STAGES     (3),
   .PIPELINE_STAGES (1),
   .INIT            (1'b0)
) pclk_slow_rstn (
   .clk     (cqetmclk),
   .async_in(sys_rstn),
   .sync_out(cqetmclk_sync_rstn)
);
assign    sd_cqetclk_sync_rstn    =   cqetmclk_sync_rstn;

endmodule
