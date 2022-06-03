module SimTop(
  input         clock,
  input         reset,
  input  [63:0] io_logCtrl_log_begin,
  input  [63:0] io_logCtrl_log_end,
  input  [63:0] io_logCtrl_log_level,
  input         io_perfInfo_clean,
  input         io_perfInfo_dump,
  output        io_uart_out_valid,
  output [7:0]  io_uart_out_ch,
  output        io_uart_in_valid,
  input  [7:0]  io_uart_in_ch
);
`ifdef RANDOMIZE_REG_INIT
  reg [31:0] _RAND_0;
  reg [31:0] _RAND_1;
`endif // RANDOMIZE_REG_INIT
  wire  l_soc_dma_0_awready; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_awvalid; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_dma_0_awid; // @[SimTop.scala 36:19]
  wire [37:0] l_soc_dma_0_awaddr; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_dma_0_awlen; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_dma_0_awsize; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_dma_0_awburst; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_awlock; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_dma_0_awcache; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_dma_0_awprot; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_dma_0_awqos; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_wready; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_wvalid; // @[SimTop.scala 36:19]
  wire [255:0] l_soc_dma_0_wdata; // @[SimTop.scala 36:19]
  wire [31:0] l_soc_dma_0_wstrb; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_wlast; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_bready; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_bvalid; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_dma_0_bid; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_dma_0_bresp; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_arready; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_arvalid; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_dma_0_arid; // @[SimTop.scala 36:19]
  wire [37:0] l_soc_dma_0_araddr; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_dma_0_arlen; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_dma_0_arsize; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_dma_0_arburst; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_arlock; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_dma_0_arcache; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_dma_0_arprot; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_dma_0_arqos; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_rready; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_rvalid; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_dma_0_rid; // @[SimTop.scala 36:19]
  wire [255:0] l_soc_dma_0_rdata; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_dma_0_rresp; // @[SimTop.scala 36:19]
  wire  l_soc_dma_0_rlast; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_awready; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_awvalid; // @[SimTop.scala 36:19]
  wire [4:0] l_soc_peripheral_0_awid; // @[SimTop.scala 36:19]
  wire [36:0] l_soc_peripheral_0_awaddr; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_peripheral_0_awlen; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_peripheral_0_awsize; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_peripheral_0_awburst; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_awlock; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_peripheral_0_awcache; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_peripheral_0_awprot; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_peripheral_0_awqos; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_wready; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_wvalid; // @[SimTop.scala 36:19]
  wire [255:0] l_soc_peripheral_0_wdata; // @[SimTop.scala 36:19]
  wire [31:0] l_soc_peripheral_0_wstrb; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_wlast; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_bready; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_bvalid; // @[SimTop.scala 36:19]
  wire [4:0] l_soc_peripheral_0_bid; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_peripheral_0_bresp; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_arready; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_arvalid; // @[SimTop.scala 36:19]
  wire [4:0] l_soc_peripheral_0_arid; // @[SimTop.scala 36:19]
  wire [36:0] l_soc_peripheral_0_araddr; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_peripheral_0_arlen; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_peripheral_0_arsize; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_peripheral_0_arburst; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_arlock; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_peripheral_0_arcache; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_peripheral_0_arprot; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_peripheral_0_arqos; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_rready; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_rvalid; // @[SimTop.scala 36:19]
  wire [4:0] l_soc_peripheral_0_rid; // @[SimTop.scala 36:19]
  wire [255:0] l_soc_peripheral_0_rdata; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_peripheral_0_rresp; // @[SimTop.scala 36:19]
  wire  l_soc_peripheral_0_rlast; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_awready; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_awvalid; // @[SimTop.scala 36:19]
  wire [5:0] l_soc_memory_0_awid; // @[SimTop.scala 36:19]
  wire [37:0] l_soc_memory_0_awaddr; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_memory_0_awlen; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_memory_0_awsize; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_memory_0_awburst; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_awlock; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_memory_0_awcache; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_memory_0_awprot; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_memory_0_awqos; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_wready; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_wvalid; // @[SimTop.scala 36:19]
  wire [255:0] l_soc_memory_0_wdata; // @[SimTop.scala 36:19]
  wire [31:0] l_soc_memory_0_wstrb; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_wlast; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_bready; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_bvalid; // @[SimTop.scala 36:19]
  wire [5:0] l_soc_memory_0_bid; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_memory_0_bresp; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_arready; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_arvalid; // @[SimTop.scala 36:19]
  wire [5:0] l_soc_memory_0_arid; // @[SimTop.scala 36:19]
  wire [37:0] l_soc_memory_0_araddr; // @[SimTop.scala 36:19]
  wire [7:0] l_soc_memory_0_arlen; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_memory_0_arsize; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_memory_0_arburst; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_arlock; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_memory_0_arcache; // @[SimTop.scala 36:19]
  wire [2:0] l_soc_memory_0_arprot; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_memory_0_arqos; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_rready; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_rvalid; // @[SimTop.scala 36:19]
  wire [5:0] l_soc_memory_0_rid; // @[SimTop.scala 36:19]
  wire [255:0] l_soc_memory_0_rdata; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_memory_0_rresp; // @[SimTop.scala 36:19]
  wire  l_soc_memory_0_rlast; // @[SimTop.scala 36:19]
  wire  l_soc_io_clock; // @[SimTop.scala 36:19]
  wire  l_soc_io_reset; // @[SimTop.scala 36:19]
  wire [255:0] l_soc_io_extIntrs; // @[SimTop.scala 36:19]
  wire  l_soc_io_systemjtag_jtag_TCK; // @[SimTop.scala 36:19]
  wire  l_soc_io_systemjtag_jtag_TMS; // @[SimTop.scala 36:19]
  wire  l_soc_io_systemjtag_jtag_TDI; // @[SimTop.scala 36:19]
  wire  l_soc_io_systemjtag_jtag_TDO_data; // @[SimTop.scala 36:19]
  wire  l_soc_io_systemjtag_jtag_TDO_driven; // @[SimTop.scala 36:19]
  wire  l_soc_io_systemjtag_reset; // @[SimTop.scala 36:19]
  wire [10:0] l_soc_io_systemjtag_mfr_id; // @[SimTop.scala 36:19]
  wire [15:0] l_soc_io_systemjtag_part_number; // @[SimTop.scala 36:19]
  wire [3:0] l_soc_io_systemjtag_version; // @[SimTop.scala 36:19]
  wire  l_soc_io_debug_reset; // @[SimTop.scala 36:19]
  wire  l_soc_io_rtc_clock; // @[SimTop.scala 36:19]
  wire  l_soc_io_riscv_halt_0; // @[SimTop.scala 36:19]
  wire [37:0] l_soc_io_riscv_rst_vec_0; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_fdfx_powergood; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_capture; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_reset_b; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_select; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_shift; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_si; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_tck; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_update; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_ijtag_so; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_fdfx_powergood; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_capture; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_reset_b; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_select; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_shift; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_si; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_tck; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_update; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_ijtag_so; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_uscan_state; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_uscan_edt_update; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_uscan_mode; // @[SimTop.scala 36:19]
  wire  l_soc_xsx_ultiscan_uscan_scanclk; // @[SimTop.scala 36:19]
  wire [10:0] l_soc_xsx_ultiscan_uscan_si; // @[SimTop.scala 36:19]
  wire [10:0] l_soc_xsx_ultiscan_uscan_so; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_uscan_state; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_uscan_edt_update; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_uscan_mode; // @[SimTop.scala 36:19]
  wire  l_soc_xsl2_ultiscan_uscan_scanclk; // @[SimTop.scala 36:19]
  wire [10:0] l_soc_xsl2_ultiscan_uscan_si; // @[SimTop.scala 36:19]
  wire [10:0] l_soc_xsl2_ultiscan_uscan_so; // @[SimTop.scala 36:19]
  wire [10:0] l_soc_hd2prf_in_trim_fuse; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_hd2prf_in_sleep_fuse; // @[SimTop.scala 36:19]
  wire [10:0] l_soc_hsuspsr_in_trim_fuse; // @[SimTop.scala 36:19]
  wire [1:0] l_soc_hsuspsr_in_sleep_fuse; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_tck; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_reset; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_ce; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_se; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_ue; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_sel; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_si; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_so; // @[SimTop.scala 36:19]
  wire  l_soc_l1l2_mbist_jtag_diag_done; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_tck; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_reset; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_ce; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_se; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_ue; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_sel; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_si; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_so; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_0_diag_done; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_tck; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_reset; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_ce; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_se; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_ue; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_sel; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_si; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_so; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_1_diag_done; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_tck; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_reset; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_ce; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_se; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_ue; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_sel; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_si; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_so; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_2_diag_done; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_tck; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_reset; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_ce; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_se; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_ue; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_sel; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_si; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_so; // @[SimTop.scala 36:19]
  wire  l_soc_l3_mbist_ijtag_3_diag_done; // @[SimTop.scala 36:19]
  wire  l_simMMIO_clock; // @[SimTop.scala 39:23]
  wire  l_simMMIO_reset; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_awready; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_awvalid; // @[SimTop.scala 39:23]
  wire [4:0] l_simMMIO_io_axi4_0_awid; // @[SimTop.scala 39:23]
  wire [37:0] l_simMMIO_io_axi4_0_awaddr; // @[SimTop.scala 39:23]
  wire [7:0] l_simMMIO_io_axi4_0_awlen; // @[SimTop.scala 39:23]
  wire [2:0] l_simMMIO_io_axi4_0_awsize; // @[SimTop.scala 39:23]
  wire [3:0] l_simMMIO_io_axi4_0_awcache; // @[SimTop.scala 39:23]
  wire [2:0] l_simMMIO_io_axi4_0_awprot; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_wready; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_wvalid; // @[SimTop.scala 39:23]
  wire [255:0] l_simMMIO_io_axi4_0_wdata; // @[SimTop.scala 39:23]
  wire [31:0] l_simMMIO_io_axi4_0_wstrb; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_wlast; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_bready; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_bvalid; // @[SimTop.scala 39:23]
  wire [4:0] l_simMMIO_io_axi4_0_bid; // @[SimTop.scala 39:23]
  wire [1:0] l_simMMIO_io_axi4_0_bresp; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_arready; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_arvalid; // @[SimTop.scala 39:23]
  wire [4:0] l_simMMIO_io_axi4_0_arid; // @[SimTop.scala 39:23]
  wire [37:0] l_simMMIO_io_axi4_0_araddr; // @[SimTop.scala 39:23]
  wire [7:0] l_simMMIO_io_axi4_0_arlen; // @[SimTop.scala 39:23]
  wire [2:0] l_simMMIO_io_axi4_0_arsize; // @[SimTop.scala 39:23]
  wire [3:0] l_simMMIO_io_axi4_0_arcache; // @[SimTop.scala 39:23]
  wire [2:0] l_simMMIO_io_axi4_0_arprot; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_rready; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_rvalid; // @[SimTop.scala 39:23]
  wire [4:0] l_simMMIO_io_axi4_0_rid; // @[SimTop.scala 39:23]
  wire [255:0] l_simMMIO_io_axi4_0_rdata; // @[SimTop.scala 39:23]
  wire [1:0] l_simMMIO_io_axi4_0_rresp; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_axi4_0_rlast; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_awready; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_awvalid; // @[SimTop.scala 39:23]
  wire [7:0] l_simMMIO_io_dma_0_awid; // @[SimTop.scala 39:23]
  wire [37:0] l_simMMIO_io_dma_0_awaddr; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_wready; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_wvalid; // @[SimTop.scala 39:23]
  wire [255:0] l_simMMIO_io_dma_0_wdata; // @[SimTop.scala 39:23]
  wire [31:0] l_simMMIO_io_dma_0_wstrb; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_wlast; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_bvalid; // @[SimTop.scala 39:23]
  wire [7:0] l_simMMIO_io_dma_0_bid; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_arready; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_arvalid; // @[SimTop.scala 39:23]
  wire [7:0] l_simMMIO_io_dma_0_arid; // @[SimTop.scala 39:23]
  wire [37:0] l_simMMIO_io_dma_0_araddr; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_dma_0_rvalid; // @[SimTop.scala 39:23]
  wire [7:0] l_simMMIO_io_dma_0_rid; // @[SimTop.scala 39:23]
  wire [255:0] l_simMMIO_io_dma_0_rdata; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_uart_out_valid; // @[SimTop.scala 39:23]
  wire [7:0] l_simMMIO_io_uart_out_ch; // @[SimTop.scala 39:23]
  wire  l_simMMIO_io_uart_in_valid; // @[SimTop.scala 39:23]
  wire [7:0] l_simMMIO_io_uart_in_ch; // @[SimTop.scala 39:23]
  wire [63:0] l_simMMIO_io_interrupt_intrVec; // @[SimTop.scala 39:23]
  wire  l_simAXIMem_clock; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_reset; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_awready; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_awvalid; // @[SimTop.scala 48:27]
  wire [5:0] l_simAXIMem_io_axi4_0_awid; // @[SimTop.scala 48:27]
  wire [37:0] l_simAXIMem_io_axi4_0_awaddr; // @[SimTop.scala 48:27]
  wire [7:0] l_simAXIMem_io_axi4_0_awlen; // @[SimTop.scala 48:27]
  wire [2:0] l_simAXIMem_io_axi4_0_awsize; // @[SimTop.scala 48:27]
  wire [1:0] l_simAXIMem_io_axi4_0_awburst; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_awlock; // @[SimTop.scala 48:27]
  wire [3:0] l_simAXIMem_io_axi4_0_awcache; // @[SimTop.scala 48:27]
  wire [2:0] l_simAXIMem_io_axi4_0_awprot; // @[SimTop.scala 48:27]
  wire [3:0] l_simAXIMem_io_axi4_0_awqos; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_wready; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_wvalid; // @[SimTop.scala 48:27]
  wire [255:0] l_simAXIMem_io_axi4_0_wdata; // @[SimTop.scala 48:27]
  wire [31:0] l_simAXIMem_io_axi4_0_wstrb; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_wlast; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_bready; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_bvalid; // @[SimTop.scala 48:27]
  wire [5:0] l_simAXIMem_io_axi4_0_bid; // @[SimTop.scala 48:27]
  wire [1:0] l_simAXIMem_io_axi4_0_bresp; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_arready; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_arvalid; // @[SimTop.scala 48:27]
  wire [5:0] l_simAXIMem_io_axi4_0_arid; // @[SimTop.scala 48:27]
  wire [37:0] l_simAXIMem_io_axi4_0_araddr; // @[SimTop.scala 48:27]
  wire [7:0] l_simAXIMem_io_axi4_0_arlen; // @[SimTop.scala 48:27]
  wire [2:0] l_simAXIMem_io_axi4_0_arsize; // @[SimTop.scala 48:27]
  wire [1:0] l_simAXIMem_io_axi4_0_arburst; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_arlock; // @[SimTop.scala 48:27]
  wire [3:0] l_simAXIMem_io_axi4_0_arcache; // @[SimTop.scala 48:27]
  wire [2:0] l_simAXIMem_io_axi4_0_arprot; // @[SimTop.scala 48:27]
  wire [3:0] l_simAXIMem_io_axi4_0_arqos; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_rready; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_rvalid; // @[SimTop.scala 48:27]
  wire [5:0] l_simAXIMem_io_axi4_0_rid; // @[SimTop.scala 48:27]
  wire [255:0] l_simAXIMem_io_axi4_0_rdata; // @[SimTop.scala 48:27]
  wire [1:0] l_simAXIMem_io_axi4_0_rresp; // @[SimTop.scala 48:27]
  wire  l_simAXIMem_io_axi4_0_rlast; // @[SimTop.scala 48:27]
  wire  SimJTAG_clock; // @[SimTop.scala 69:20]
  wire  SimJTAG_reset; // @[SimTop.scala 69:20]
  wire  SimJTAG_jtag_TRSTn; // @[SimTop.scala 69:20]
  wire  SimJTAG_jtag_TCK; // @[SimTop.scala 69:20]
  wire  SimJTAG_jtag_TMS; // @[SimTop.scala 69:20]
  wire  SimJTAG_jtag_TDI; // @[SimTop.scala 69:20]
  wire  SimJTAG_jtag_TDO_data; // @[SimTop.scala 69:20]
  wire  SimJTAG_jtag_TDO_driven; // @[SimTop.scala 69:20]
  wire  SimJTAG_enable; // @[SimTop.scala 69:20]
  wire  SimJTAG_init_done; // @[SimTop.scala 69:20]
  wire [31:0] SimJTAG_exit; // @[SimTop.scala 69:20]
  reg [5:0] rtcCounter; // @[SimTop.scala 60:27]
  wire [5:0] _rtcCounter_T_2 = rtcCounter + 6'h1; // @[SimTop.scala 61:74]
  reg  rtcClock; // @[SimTop.scala 62:25]
  bosc_XSTop l_soc ( // @[SimTop.scala 36:19]
    .dma_0_awready(l_soc_dma_0_awready),
    .dma_0_awvalid(l_soc_dma_0_awvalid),
    .dma_0_awid(l_soc_dma_0_awid),
    .dma_0_awaddr(l_soc_dma_0_awaddr),
    .dma_0_awlen(l_soc_dma_0_awlen),
    .dma_0_awsize(l_soc_dma_0_awsize),
    .dma_0_awburst(l_soc_dma_0_awburst),
    .dma_0_awlock(l_soc_dma_0_awlock),
    .dma_0_awcache(l_soc_dma_0_awcache),
    .dma_0_awprot(l_soc_dma_0_awprot),
    .dma_0_awqos(l_soc_dma_0_awqos),
    .dma_0_wready(l_soc_dma_0_wready),
    .dma_0_wvalid(l_soc_dma_0_wvalid),
    .dma_0_wdata(l_soc_dma_0_wdata),
    .dma_0_wstrb(l_soc_dma_0_wstrb),
    .dma_0_wlast(l_soc_dma_0_wlast),
    .dma_0_bready(l_soc_dma_0_bready),
    .dma_0_bvalid(l_soc_dma_0_bvalid),
    .dma_0_bid(l_soc_dma_0_bid),
    .dma_0_bresp(l_soc_dma_0_bresp),
    .dma_0_arready(l_soc_dma_0_arready),
    .dma_0_arvalid(l_soc_dma_0_arvalid),
    .dma_0_arid(l_soc_dma_0_arid),
    .dma_0_araddr(l_soc_dma_0_araddr),
    .dma_0_arlen(l_soc_dma_0_arlen),
    .dma_0_arsize(l_soc_dma_0_arsize),
    .dma_0_arburst(l_soc_dma_0_arburst),
    .dma_0_arlock(l_soc_dma_0_arlock),
    .dma_0_arcache(l_soc_dma_0_arcache),
    .dma_0_arprot(l_soc_dma_0_arprot),
    .dma_0_arqos(l_soc_dma_0_arqos),
    .dma_0_rready(l_soc_dma_0_rready),
    .dma_0_rvalid(l_soc_dma_0_rvalid),
    .dma_0_rid(l_soc_dma_0_rid),
    .dma_0_rdata(l_soc_dma_0_rdata),
    .dma_0_rresp(l_soc_dma_0_rresp),
    .dma_0_rlast(l_soc_dma_0_rlast),
    .peripheral_0_awready(l_soc_peripheral_0_awready),
    .peripheral_0_awvalid(l_soc_peripheral_0_awvalid),
    .peripheral_0_awid(l_soc_peripheral_0_awid),
    .peripheral_0_awaddr(l_soc_peripheral_0_awaddr),
    .peripheral_0_awlen(l_soc_peripheral_0_awlen),
    .peripheral_0_awsize(l_soc_peripheral_0_awsize),
    .peripheral_0_awburst(l_soc_peripheral_0_awburst),
    .peripheral_0_awlock(l_soc_peripheral_0_awlock),
    .peripheral_0_awcache(l_soc_peripheral_0_awcache),
    .peripheral_0_awprot(l_soc_peripheral_0_awprot),
    .peripheral_0_awqos(l_soc_peripheral_0_awqos),
    .peripheral_0_wready(l_soc_peripheral_0_wready),
    .peripheral_0_wvalid(l_soc_peripheral_0_wvalid),
    .peripheral_0_wdata(l_soc_peripheral_0_wdata),
    .peripheral_0_wstrb(l_soc_peripheral_0_wstrb),
    .peripheral_0_wlast(l_soc_peripheral_0_wlast),
    .peripheral_0_bready(l_soc_peripheral_0_bready),
    .peripheral_0_bvalid(l_soc_peripheral_0_bvalid),
    .peripheral_0_bid(l_soc_peripheral_0_bid),
    .peripheral_0_bresp(l_soc_peripheral_0_bresp),
    .peripheral_0_arready(l_soc_peripheral_0_arready),
    .peripheral_0_arvalid(l_soc_peripheral_0_arvalid),
    .peripheral_0_arid(l_soc_peripheral_0_arid),
    .peripheral_0_araddr(l_soc_peripheral_0_araddr),
    .peripheral_0_arlen(l_soc_peripheral_0_arlen),
    .peripheral_0_arsize(l_soc_peripheral_0_arsize),
    .peripheral_0_arburst(l_soc_peripheral_0_arburst),
    .peripheral_0_arlock(l_soc_peripheral_0_arlock),
    .peripheral_0_arcache(l_soc_peripheral_0_arcache),
    .peripheral_0_arprot(l_soc_peripheral_0_arprot),
    .peripheral_0_arqos(l_soc_peripheral_0_arqos),
    .peripheral_0_rready(l_soc_peripheral_0_rready),
    .peripheral_0_rvalid(l_soc_peripheral_0_rvalid),
    .peripheral_0_rid(l_soc_peripheral_0_rid),
    .peripheral_0_rdata(l_soc_peripheral_0_rdata),
    .peripheral_0_rresp(l_soc_peripheral_0_rresp),
    .peripheral_0_rlast(l_soc_peripheral_0_rlast),
    .memory_0_awready(l_soc_memory_0_awready),
    .memory_0_awvalid(l_soc_memory_0_awvalid),
    .memory_0_awid(l_soc_memory_0_awid),
    .memory_0_awaddr(l_soc_memory_0_awaddr),
    .memory_0_awlen(l_soc_memory_0_awlen),
    .memory_0_awsize(l_soc_memory_0_awsize),
    .memory_0_awburst(l_soc_memory_0_awburst),
    .memory_0_awlock(l_soc_memory_0_awlock),
    .memory_0_awcache(l_soc_memory_0_awcache),
    .memory_0_awprot(l_soc_memory_0_awprot),
    .memory_0_awqos(l_soc_memory_0_awqos),
    .memory_0_wready(l_soc_memory_0_wready),
    .memory_0_wvalid(l_soc_memory_0_wvalid),
    .memory_0_wdata(l_soc_memory_0_wdata),
    .memory_0_wstrb(l_soc_memory_0_wstrb),
    .memory_0_wlast(l_soc_memory_0_wlast),
    .memory_0_bready(l_soc_memory_0_bready),
    .memory_0_bvalid(l_soc_memory_0_bvalid),
    .memory_0_bid(l_soc_memory_0_bid),
    .memory_0_bresp(l_soc_memory_0_bresp),
    .memory_0_arready(l_soc_memory_0_arready),
    .memory_0_arvalid(l_soc_memory_0_arvalid),
    .memory_0_arid(l_soc_memory_0_arid),
    .memory_0_araddr(l_soc_memory_0_araddr),
    .memory_0_arlen(l_soc_memory_0_arlen),
    .memory_0_arsize(l_soc_memory_0_arsize),
    .memory_0_arburst(l_soc_memory_0_arburst),
    .memory_0_arlock(l_soc_memory_0_arlock),
    .memory_0_arcache(l_soc_memory_0_arcache),
    .memory_0_arprot(l_soc_memory_0_arprot),
    .memory_0_arqos(l_soc_memory_0_arqos),
    .memory_0_rready(l_soc_memory_0_rready),
    .memory_0_rvalid(l_soc_memory_0_rvalid),
    .memory_0_rid(l_soc_memory_0_rid),
    .memory_0_rdata(l_soc_memory_0_rdata),
    .memory_0_rresp(l_soc_memory_0_rresp),
    .memory_0_rlast(l_soc_memory_0_rlast),
    .io_clock(l_soc_io_clock),
    .io_reset(l_soc_io_reset),
    .io_extIntrs(l_soc_io_extIntrs),
    .io_systemjtag_jtag_TCK(l_soc_io_systemjtag_jtag_TCK),
    .io_systemjtag_jtag_TMS(l_soc_io_systemjtag_jtag_TMS),
    .io_systemjtag_jtag_TDI(l_soc_io_systemjtag_jtag_TDI),
    .io_systemjtag_jtag_TDO_data(l_soc_io_systemjtag_jtag_TDO_data),
    .io_systemjtag_jtag_TDO_driven(l_soc_io_systemjtag_jtag_TDO_driven),
    .io_systemjtag_reset(l_soc_io_systemjtag_reset),
    .io_systemjtag_mfr_id(l_soc_io_systemjtag_mfr_id),
    .io_systemjtag_part_number(l_soc_io_systemjtag_part_number),
    .io_systemjtag_version(l_soc_io_systemjtag_version),
    .io_debug_reset(l_soc_io_debug_reset),
    .io_rtc_clock(l_soc_io_rtc_clock),
    .io_riscv_halt_0(l_soc_io_riscv_halt_0),
    .io_riscv_rst_vec_0(l_soc_io_riscv_rst_vec_0),
    .xsx_ultiscan_ijtag_fdfx_powergood(l_soc_xsx_ultiscan_ijtag_fdfx_powergood),
    .xsx_ultiscan_ijtag_capture(l_soc_xsx_ultiscan_ijtag_capture),
    .xsx_ultiscan_ijtag_reset_b(l_soc_xsx_ultiscan_ijtag_reset_b),
    .xsx_ultiscan_ijtag_select(l_soc_xsx_ultiscan_ijtag_select),
    .xsx_ultiscan_ijtag_shift(l_soc_xsx_ultiscan_ijtag_shift),
    .xsx_ultiscan_ijtag_si(l_soc_xsx_ultiscan_ijtag_si),
    .xsx_ultiscan_ijtag_tck(l_soc_xsx_ultiscan_ijtag_tck),
    .xsx_ultiscan_ijtag_update(l_soc_xsx_ultiscan_ijtag_update),
    .xsx_ultiscan_ijtag_so(l_soc_xsx_ultiscan_ijtag_so),
    .xsl2_ultiscan_ijtag_fdfx_powergood(l_soc_xsl2_ultiscan_ijtag_fdfx_powergood),
    .xsl2_ultiscan_ijtag_capture(l_soc_xsl2_ultiscan_ijtag_capture),
    .xsl2_ultiscan_ijtag_reset_b(l_soc_xsl2_ultiscan_ijtag_reset_b),
    .xsl2_ultiscan_ijtag_select(l_soc_xsl2_ultiscan_ijtag_select),
    .xsl2_ultiscan_ijtag_shift(l_soc_xsl2_ultiscan_ijtag_shift),
    .xsl2_ultiscan_ijtag_si(l_soc_xsl2_ultiscan_ijtag_si),
    .xsl2_ultiscan_ijtag_tck(l_soc_xsl2_ultiscan_ijtag_tck),
    .xsl2_ultiscan_ijtag_update(l_soc_xsl2_ultiscan_ijtag_update),
    .xsl2_ultiscan_ijtag_so(l_soc_xsl2_ultiscan_ijtag_so),
    .xsx_ultiscan_uscan_state(l_soc_xsx_ultiscan_uscan_state),
    .xsx_ultiscan_uscan_edt_update(l_soc_xsx_ultiscan_uscan_edt_update),
    .xsx_ultiscan_uscan_mode(l_soc_xsx_ultiscan_uscan_mode),
    .xsx_ultiscan_uscan_scanclk(l_soc_xsx_ultiscan_uscan_scanclk),
    .xsx_ultiscan_uscan_si(l_soc_xsx_ultiscan_uscan_si),
    .xsx_ultiscan_uscan_so(l_soc_xsx_ultiscan_uscan_so),
    .xsl2_ultiscan_uscan_state(l_soc_xsl2_ultiscan_uscan_state),
    .xsl2_ultiscan_uscan_edt_update(l_soc_xsl2_ultiscan_uscan_edt_update),
    .xsl2_ultiscan_uscan_mode(l_soc_xsl2_ultiscan_uscan_mode),
    .xsl2_ultiscan_uscan_scanclk(l_soc_xsl2_ultiscan_uscan_scanclk),
    .xsl2_ultiscan_uscan_si(l_soc_xsl2_ultiscan_uscan_si),
    .xsl2_ultiscan_uscan_so(l_soc_xsl2_ultiscan_uscan_so),
    .hd2prf_in_trim_fuse(l_soc_hd2prf_in_trim_fuse),
    .hd2prf_in_sleep_fuse(l_soc_hd2prf_in_sleep_fuse),
    .hsuspsr_in_trim_fuse(l_soc_hsuspsr_in_trim_fuse),
    .hsuspsr_in_sleep_fuse(l_soc_hsuspsr_in_sleep_fuse),
    .l1l2_mbist_jtag_tck(l_soc_l1l2_mbist_jtag_tck),
    .l1l2_mbist_jtag_reset(l_soc_l1l2_mbist_jtag_reset),
    .l1l2_mbist_jtag_ce(l_soc_l1l2_mbist_jtag_ce),
    .l1l2_mbist_jtag_se(l_soc_l1l2_mbist_jtag_se),
    .l1l2_mbist_jtag_ue(l_soc_l1l2_mbist_jtag_ue),
    .l1l2_mbist_jtag_sel(l_soc_l1l2_mbist_jtag_sel),
    .l1l2_mbist_jtag_si(l_soc_l1l2_mbist_jtag_si),
    .l1l2_mbist_jtag_so(l_soc_l1l2_mbist_jtag_so),
    .l1l2_mbist_jtag_diag_done(l_soc_l1l2_mbist_jtag_diag_done),
    .l3_mbist_ijtag_0_tck(l_soc_l3_mbist_ijtag_0_tck),
    .l3_mbist_ijtag_0_reset(l_soc_l3_mbist_ijtag_0_reset),
    .l3_mbist_ijtag_0_ce(l_soc_l3_mbist_ijtag_0_ce),
    .l3_mbist_ijtag_0_se(l_soc_l3_mbist_ijtag_0_se),
    .l3_mbist_ijtag_0_ue(l_soc_l3_mbist_ijtag_0_ue),
    .l3_mbist_ijtag_0_sel(l_soc_l3_mbist_ijtag_0_sel),
    .l3_mbist_ijtag_0_si(l_soc_l3_mbist_ijtag_0_si),
    .l3_mbist_ijtag_0_so(l_soc_l3_mbist_ijtag_0_so),
    .l3_mbist_ijtag_0_diag_done(l_soc_l3_mbist_ijtag_0_diag_done),
    .l3_mbist_ijtag_1_tck(l_soc_l3_mbist_ijtag_1_tck),
    .l3_mbist_ijtag_1_reset(l_soc_l3_mbist_ijtag_1_reset),
    .l3_mbist_ijtag_1_ce(l_soc_l3_mbist_ijtag_1_ce),
    .l3_mbist_ijtag_1_se(l_soc_l3_mbist_ijtag_1_se),
    .l3_mbist_ijtag_1_ue(l_soc_l3_mbist_ijtag_1_ue),
    .l3_mbist_ijtag_1_sel(l_soc_l3_mbist_ijtag_1_sel),
    .l3_mbist_ijtag_1_si(l_soc_l3_mbist_ijtag_1_si),
    .l3_mbist_ijtag_1_so(l_soc_l3_mbist_ijtag_1_so),
    .l3_mbist_ijtag_1_diag_done(l_soc_l3_mbist_ijtag_1_diag_done),
    .l3_mbist_ijtag_2_tck(l_soc_l3_mbist_ijtag_2_tck),
    .l3_mbist_ijtag_2_reset(l_soc_l3_mbist_ijtag_2_reset),
    .l3_mbist_ijtag_2_ce(l_soc_l3_mbist_ijtag_2_ce),
    .l3_mbist_ijtag_2_se(l_soc_l3_mbist_ijtag_2_se),
    .l3_mbist_ijtag_2_ue(l_soc_l3_mbist_ijtag_2_ue),
    .l3_mbist_ijtag_2_sel(l_soc_l3_mbist_ijtag_2_sel),
    .l3_mbist_ijtag_2_si(l_soc_l3_mbist_ijtag_2_si),
    .l3_mbist_ijtag_2_so(l_soc_l3_mbist_ijtag_2_so),
    .l3_mbist_ijtag_2_diag_done(l_soc_l3_mbist_ijtag_2_diag_done),
    .l3_mbist_ijtag_3_tck(l_soc_l3_mbist_ijtag_3_tck),
    .l3_mbist_ijtag_3_reset(l_soc_l3_mbist_ijtag_3_reset),
    .l3_mbist_ijtag_3_ce(l_soc_l3_mbist_ijtag_3_ce),
    .l3_mbist_ijtag_3_se(l_soc_l3_mbist_ijtag_3_se),
    .l3_mbist_ijtag_3_ue(l_soc_l3_mbist_ijtag_3_ue),
    .l3_mbist_ijtag_3_sel(l_soc_l3_mbist_ijtag_3_sel),
    .l3_mbist_ijtag_3_si(l_soc_l3_mbist_ijtag_3_si),
    .l3_mbist_ijtag_3_so(l_soc_l3_mbist_ijtag_3_so),
    .l3_mbist_ijtag_3_diag_done(l_soc_l3_mbist_ijtag_3_diag_done)
  );
  SimMMIO l_simMMIO ( // @[SimTop.scala 39:23]
    .clock(l_simMMIO_clock),
    .reset(l_simMMIO_reset),
    .io_axi4_0_awready(l_simMMIO_io_axi4_0_awready),
    .io_axi4_0_awvalid(l_simMMIO_io_axi4_0_awvalid),
    .io_axi4_0_awid(l_simMMIO_io_axi4_0_awid),
    .io_axi4_0_awaddr(l_simMMIO_io_axi4_0_awaddr),
    .io_axi4_0_awlen(l_simMMIO_io_axi4_0_awlen),
    .io_axi4_0_awsize(l_simMMIO_io_axi4_0_awsize),
    .io_axi4_0_awcache(l_simMMIO_io_axi4_0_awcache),
    .io_axi4_0_awprot(l_simMMIO_io_axi4_0_awprot),
    .io_axi4_0_wready(l_simMMIO_io_axi4_0_wready),
    .io_axi4_0_wvalid(l_simMMIO_io_axi4_0_wvalid),
    .io_axi4_0_wdata(l_simMMIO_io_axi4_0_wdata),
    .io_axi4_0_wstrb(l_simMMIO_io_axi4_0_wstrb),
    .io_axi4_0_wlast(l_simMMIO_io_axi4_0_wlast),
    .io_axi4_0_bready(l_simMMIO_io_axi4_0_bready),
    .io_axi4_0_bvalid(l_simMMIO_io_axi4_0_bvalid),
    .io_axi4_0_bid(l_simMMIO_io_axi4_0_bid),
    .io_axi4_0_bresp(l_simMMIO_io_axi4_0_bresp),
    .io_axi4_0_arready(l_simMMIO_io_axi4_0_arready),
    .io_axi4_0_arvalid(l_simMMIO_io_axi4_0_arvalid),
    .io_axi4_0_arid(l_simMMIO_io_axi4_0_arid),
    .io_axi4_0_araddr(l_simMMIO_io_axi4_0_araddr),
    .io_axi4_0_arlen(l_simMMIO_io_axi4_0_arlen),
    .io_axi4_0_arsize(l_simMMIO_io_axi4_0_arsize),
    .io_axi4_0_arcache(l_simMMIO_io_axi4_0_arcache),
    .io_axi4_0_arprot(l_simMMIO_io_axi4_0_arprot),
    .io_axi4_0_rready(l_simMMIO_io_axi4_0_rready),
    .io_axi4_0_rvalid(l_simMMIO_io_axi4_0_rvalid),
    .io_axi4_0_rid(l_simMMIO_io_axi4_0_rid),
    .io_axi4_0_rdata(l_simMMIO_io_axi4_0_rdata),
    .io_axi4_0_rresp(l_simMMIO_io_axi4_0_rresp),
    .io_axi4_0_rlast(l_simMMIO_io_axi4_0_rlast),
    .io_dma_0_awready(l_simMMIO_io_dma_0_awready),
    .io_dma_0_awvalid(l_simMMIO_io_dma_0_awvalid),
    .io_dma_0_awid(l_simMMIO_io_dma_0_awid),
    .io_dma_0_awaddr(l_simMMIO_io_dma_0_awaddr),
    .io_dma_0_wready(l_simMMIO_io_dma_0_wready),
    .io_dma_0_wvalid(l_simMMIO_io_dma_0_wvalid),
    .io_dma_0_wdata(l_simMMIO_io_dma_0_wdata),
    .io_dma_0_wstrb(l_simMMIO_io_dma_0_wstrb),
    .io_dma_0_wlast(l_simMMIO_io_dma_0_wlast),
    .io_dma_0_bvalid(l_simMMIO_io_dma_0_bvalid),
    .io_dma_0_bid(l_simMMIO_io_dma_0_bid),
    .io_dma_0_arready(l_simMMIO_io_dma_0_arready),
    .io_dma_0_arvalid(l_simMMIO_io_dma_0_arvalid),
    .io_dma_0_arid(l_simMMIO_io_dma_0_arid),
    .io_dma_0_araddr(l_simMMIO_io_dma_0_araddr),
    .io_dma_0_rvalid(l_simMMIO_io_dma_0_rvalid),
    .io_dma_0_rid(l_simMMIO_io_dma_0_rid),
    .io_dma_0_rdata(l_simMMIO_io_dma_0_rdata),
    .io_uart_out_valid(l_simMMIO_io_uart_out_valid),
    .io_uart_out_ch(l_simMMIO_io_uart_out_ch),
    .io_uart_in_valid(l_simMMIO_io_uart_in_valid),
    .io_uart_in_ch(l_simMMIO_io_uart_in_ch),
    .io_interrupt_intrVec(l_simMMIO_io_interrupt_intrVec)
  );
  AXI4RAMWrapper l_simAXIMem ( // @[SimTop.scala 48:27]
    .clock(l_simAXIMem_clock),
    .reset(l_simAXIMem_reset),
    .io_axi4_0_awready(l_simAXIMem_io_axi4_0_awready),
    .io_axi4_0_awvalid(l_simAXIMem_io_axi4_0_awvalid),
    .io_axi4_0_awid(l_simAXIMem_io_axi4_0_awid),
    .io_axi4_0_awaddr(l_simAXIMem_io_axi4_0_awaddr),
    .io_axi4_0_awlen(l_simAXIMem_io_axi4_0_awlen),
    .io_axi4_0_awsize(l_simAXIMem_io_axi4_0_awsize),
    .io_axi4_0_awburst(l_simAXIMem_io_axi4_0_awburst),
    .io_axi4_0_awlock(l_simAXIMem_io_axi4_0_awlock),
    .io_axi4_0_awcache(l_simAXIMem_io_axi4_0_awcache),
    .io_axi4_0_awprot(l_simAXIMem_io_axi4_0_awprot),
    .io_axi4_0_awqos(l_simAXIMem_io_axi4_0_awqos),
    .io_axi4_0_wready(l_simAXIMem_io_axi4_0_wready),
    .io_axi4_0_wvalid(l_simAXIMem_io_axi4_0_wvalid),
    .io_axi4_0_wdata(l_simAXIMem_io_axi4_0_wdata),
    .io_axi4_0_wstrb(l_simAXIMem_io_axi4_0_wstrb),
    .io_axi4_0_wlast(l_simAXIMem_io_axi4_0_wlast),
    .io_axi4_0_bready(l_simAXIMem_io_axi4_0_bready),
    .io_axi4_0_bvalid(l_simAXIMem_io_axi4_0_bvalid),
    .io_axi4_0_bid(l_simAXIMem_io_axi4_0_bid),
    .io_axi4_0_bresp(l_simAXIMem_io_axi4_0_bresp),
    .io_axi4_0_arready(l_simAXIMem_io_axi4_0_arready),
    .io_axi4_0_arvalid(l_simAXIMem_io_axi4_0_arvalid),
    .io_axi4_0_arid(l_simAXIMem_io_axi4_0_arid),
    .io_axi4_0_araddr(l_simAXIMem_io_axi4_0_araddr),
    .io_axi4_0_arlen(l_simAXIMem_io_axi4_0_arlen),
    .io_axi4_0_arsize(l_simAXIMem_io_axi4_0_arsize),
    .io_axi4_0_arburst(l_simAXIMem_io_axi4_0_arburst),
    .io_axi4_0_arlock(l_simAXIMem_io_axi4_0_arlock),
    .io_axi4_0_arcache(l_simAXIMem_io_axi4_0_arcache),
    .io_axi4_0_arprot(l_simAXIMem_io_axi4_0_arprot),
    .io_axi4_0_arqos(l_simAXIMem_io_axi4_0_arqos),
    .io_axi4_0_rready(l_simAXIMem_io_axi4_0_rready),
    .io_axi4_0_rvalid(l_simAXIMem_io_axi4_0_rvalid),
    .io_axi4_0_rid(l_simAXIMem_io_axi4_0_rid),
    .io_axi4_0_rdata(l_simAXIMem_io_axi4_0_rdata),
    .io_axi4_0_rresp(l_simAXIMem_io_axi4_0_rresp),
    .io_axi4_0_rlast(l_simAXIMem_io_axi4_0_rlast)
  );
  SimJTAG #(.TICK_DELAY(3)) SimJTAG ( // @[SimTop.scala 69:20]
    .clock(SimJTAG_clock),
    .reset(SimJTAG_reset),
    .jtag_TRSTn(SimJTAG_jtag_TRSTn),
    .jtag_TCK(SimJTAG_jtag_TCK),
    .jtag_TMS(SimJTAG_jtag_TMS),
    .jtag_TDI(SimJTAG_jtag_TDI),
    .jtag_TDO_data(SimJTAG_jtag_TDO_data),
    .jtag_TDO_driven(SimJTAG_jtag_TDO_driven),
    .enable(SimJTAG_enable),
    .init_done(SimJTAG_init_done),
    .exit(SimJTAG_exit)
  );
  assign io_uart_out_valid = l_simMMIO_io_uart_out_valid; // @[SimTop.scala 82:19]
  assign io_uart_out_ch = l_simMMIO_io_uart_out_ch; // @[SimTop.scala 82:19]
  assign io_uart_in_valid = l_simMMIO_io_uart_in_valid; // @[SimTop.scala 82:19]
  assign l_soc_dma_0_awvalid = l_simMMIO_io_dma_0_awvalid; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awid = l_simMMIO_io_dma_0_awid; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awaddr = l_simMMIO_io_dma_0_awaddr; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awlen = 8'h1; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awsize = 3'h5; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awburst = 2'h1; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awlock = 1'h0; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awcache = 4'h0; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awprot = 3'h0; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_awqos = 4'h0; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_wvalid = l_simMMIO_io_dma_0_wvalid; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_wdata = l_simMMIO_io_dma_0_wdata; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_wstrb = l_simMMIO_io_dma_0_wstrb; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_wlast = l_simMMIO_io_dma_0_wlast; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_bready = 1'h1; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arvalid = l_simMMIO_io_dma_0_arvalid; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arid = l_simMMIO_io_dma_0_arid; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_araddr = l_simMMIO_io_dma_0_araddr; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arlen = 8'h1; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arsize = 3'h5; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arburst = 2'h1; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arlock = 1'h0; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arcache = 4'h0; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arprot = 3'h0; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_arqos = 4'h0; // @[SimTop.scala 42:20]
  assign l_soc_dma_0_rready = 1'h1; // @[SimTop.scala 42:20]
  assign l_soc_peripheral_0_awready = l_simMMIO_io_axi4_0_awready; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_wready = l_simMMIO_io_axi4_0_wready; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_bvalid = l_simMMIO_io_axi4_0_bvalid; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_bid = l_simMMIO_io_axi4_0_bid; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_bresp = l_simMMIO_io_axi4_0_bresp; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_arready = l_simMMIO_io_axi4_0_arready; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_rvalid = l_simMMIO_io_axi4_0_rvalid; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_rid = l_simMMIO_io_axi4_0_rid; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_rdata = l_simMMIO_io_axi4_0_rdata; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_rresp = l_simMMIO_io_axi4_0_rresp; // @[SimTop.scala 41:21]
  assign l_soc_peripheral_0_rlast = l_simMMIO_io_axi4_0_rlast; // @[SimTop.scala 41:21]
  assign l_soc_memory_0_awready = l_simAXIMem_io_axi4_0_awready; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_wready = l_simAXIMem_io_axi4_0_wready; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_bvalid = l_simAXIMem_io_axi4_0_bvalid; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_bid = l_simAXIMem_io_axi4_0_bid; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_bresp = l_simAXIMem_io_axi4_0_bresp; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_arready = l_simAXIMem_io_axi4_0_arready; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_rvalid = l_simAXIMem_io_axi4_0_rvalid; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_rid = l_simAXIMem_io_axi4_0_rid; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_rdata = l_simAXIMem_io_axi4_0_rdata; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_rresp = l_simAXIMem_io_axi4_0_rresp; // @[SimTop.scala 49:25]
  assign l_soc_memory_0_rlast = l_simAXIMem_io_axi4_0_rlast; // @[SimTop.scala 49:25]
  assign l_soc_io_clock = clock; // @[SimTop.scala 53:16]
  assign l_soc_io_reset = reset; // @[SimTop.scala 54:25]
  assign l_soc_io_extIntrs = {{192'd0}, l_simMMIO_io_interrupt_intrVec}; // @[SimTop.scala 55:19]
  assign l_soc_io_systemjtag_jtag_TCK = SimJTAG_jtag_TCK; // @[RocketDebugWrapper.scala 126:15]
  assign l_soc_io_systemjtag_jtag_TMS = SimJTAG_jtag_TMS; // @[RocketDebugWrapper.scala 127:15]
  assign l_soc_io_systemjtag_jtag_TDI = SimJTAG_jtag_TDI; // @[RocketDebugWrapper.scala 128:15]
  assign l_soc_io_systemjtag_reset = reset; // @[SimTop.scala 70:36]
  assign l_soc_io_systemjtag_mfr_id = 11'h0; // @[SimTop.scala 71:28]
  assign l_soc_io_systemjtag_part_number = 16'h0; // @[SimTop.scala 72:33]
  assign l_soc_io_systemjtag_version = 4'h0; // @[SimTop.scala 73:29]
  assign l_soc_io_rtc_clock = rtcClock; // @[SimTop.scala 66:20]
  assign l_soc_io_riscv_rst_vec_0 = 38'h1ffff80000; // @[SimTop.scala 56:34]
  assign l_soc_xsx_ultiscan_ijtag_fdfx_powergood = 1'h0;
  assign l_soc_xsx_ultiscan_ijtag_capture = 1'h0;
  assign l_soc_xsx_ultiscan_ijtag_reset_b = 1'h0;
  assign l_soc_xsx_ultiscan_ijtag_select = 1'h0;
  assign l_soc_xsx_ultiscan_ijtag_shift = 1'h0;
  assign l_soc_xsx_ultiscan_ijtag_si = 1'h0;
  assign l_soc_xsx_ultiscan_ijtag_tck = 1'h0;
  assign l_soc_xsx_ultiscan_ijtag_update = 1'h0;
  assign l_soc_xsl2_ultiscan_ijtag_fdfx_powergood = 1'h0;
  assign l_soc_xsl2_ultiscan_ijtag_capture = 1'h0;
  assign l_soc_xsl2_ultiscan_ijtag_reset_b = 1'h0;
  assign l_soc_xsl2_ultiscan_ijtag_select = 1'h0;
  assign l_soc_xsl2_ultiscan_ijtag_shift = 1'h0;
  assign l_soc_xsl2_ultiscan_ijtag_si = 1'h0;
  assign l_soc_xsl2_ultiscan_ijtag_tck = 1'h0;
  assign l_soc_xsl2_ultiscan_ijtag_update = 1'h0;
  assign l_soc_xsx_ultiscan_uscan_state = 1'h0;
  assign l_soc_xsx_ultiscan_uscan_edt_update = 1'h0;
  assign l_soc_xsx_ultiscan_uscan_mode = 1'h0;
  assign l_soc_xsx_ultiscan_uscan_scanclk = 1'h0;
  assign l_soc_xsx_ultiscan_uscan_si = 11'h0;
  assign l_soc_xsl2_ultiscan_uscan_state = 1'h0;
  assign l_soc_xsl2_ultiscan_uscan_edt_update = 1'h0;
  assign l_soc_xsl2_ultiscan_uscan_mode = 1'h0;
  assign l_soc_xsl2_ultiscan_uscan_scanclk = 1'h0;
  assign l_soc_xsl2_ultiscan_uscan_si = 11'h0;
  assign l_soc_hd2prf_in_trim_fuse = 11'h0;
  assign l_soc_hd2prf_in_sleep_fuse = 2'h0;
  assign l_soc_hsuspsr_in_trim_fuse = 11'h0;
  assign l_soc_hsuspsr_in_sleep_fuse = 2'h0;
  assign l_soc_l1l2_mbist_jtag_tck = 1'h0;
  assign l_soc_l1l2_mbist_jtag_reset = 1'h0;
  assign l_soc_l1l2_mbist_jtag_ce = 1'h0;
  assign l_soc_l1l2_mbist_jtag_se = 1'h0;
  assign l_soc_l1l2_mbist_jtag_ue = 1'h0;
  assign l_soc_l1l2_mbist_jtag_sel = 1'h0;
  assign l_soc_l1l2_mbist_jtag_si = 1'h0;
  assign l_soc_l1l2_mbist_jtag_so = 1'h0;
  assign l_soc_l3_mbist_ijtag_0_tck = 1'h0;
  assign l_soc_l3_mbist_ijtag_0_reset = 1'h0;
  assign l_soc_l3_mbist_ijtag_0_ce = 1'h0;
  assign l_soc_l3_mbist_ijtag_0_se = 1'h0;
  assign l_soc_l3_mbist_ijtag_0_ue = 1'h0;
  assign l_soc_l3_mbist_ijtag_0_sel = 1'h0;
  assign l_soc_l3_mbist_ijtag_0_si = 1'h0;
  assign l_soc_l3_mbist_ijtag_0_so = 1'h0;
  assign l_soc_l3_mbist_ijtag_1_tck = 1'h0;
  assign l_soc_l3_mbist_ijtag_1_reset = 1'h0;
  assign l_soc_l3_mbist_ijtag_1_ce = 1'h0;
  assign l_soc_l3_mbist_ijtag_1_se = 1'h0;
  assign l_soc_l3_mbist_ijtag_1_ue = 1'h0;
  assign l_soc_l3_mbist_ijtag_1_sel = 1'h0;
  assign l_soc_l3_mbist_ijtag_1_si = 1'h0;
  assign l_soc_l3_mbist_ijtag_1_so = 1'h0;
  assign l_soc_l3_mbist_ijtag_2_tck = 1'h0;
  assign l_soc_l3_mbist_ijtag_2_reset = 1'h0;
  assign l_soc_l3_mbist_ijtag_2_ce = 1'h0;
  assign l_soc_l3_mbist_ijtag_2_se = 1'h0;
  assign l_soc_l3_mbist_ijtag_2_ue = 1'h0;
  assign l_soc_l3_mbist_ijtag_2_sel = 1'h0;
  assign l_soc_l3_mbist_ijtag_2_si = 1'h0;
  assign l_soc_l3_mbist_ijtag_2_so = 1'h0;
  assign l_soc_l3_mbist_ijtag_3_tck = 1'h0;
  assign l_soc_l3_mbist_ijtag_3_reset = 1'h0;
  assign l_soc_l3_mbist_ijtag_3_ce = 1'h0;
  assign l_soc_l3_mbist_ijtag_3_se = 1'h0;
  assign l_soc_l3_mbist_ijtag_3_ue = 1'h0;
  assign l_soc_l3_mbist_ijtag_3_sel = 1'h0;
  assign l_soc_l3_mbist_ijtag_3_si = 1'h0;
  assign l_soc_l3_mbist_ijtag_3_so = 1'h0;
  assign l_simMMIO_clock = clock;
  assign l_simMMIO_reset = reset;
  assign l_simMMIO_io_axi4_0_awvalid = l_soc_peripheral_0_awvalid; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_awid = l_soc_peripheral_0_awid; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_awaddr = {{1'd0}, l_soc_peripheral_0_awaddr}; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_awlen = l_soc_peripheral_0_awlen; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_awsize = l_soc_peripheral_0_awsize; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_awcache = l_soc_peripheral_0_awcache; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_awprot = l_soc_peripheral_0_awprot; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_wvalid = l_soc_peripheral_0_wvalid; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_wdata = l_soc_peripheral_0_wdata; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_wstrb = l_soc_peripheral_0_wstrb; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_wlast = l_soc_peripheral_0_wlast; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_bready = l_soc_peripheral_0_bready; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_arvalid = l_soc_peripheral_0_arvalid; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_arid = l_soc_peripheral_0_arid; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_araddr = {{1'd0}, l_soc_peripheral_0_araddr}; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_arlen = l_soc_peripheral_0_arlen; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_arsize = l_soc_peripheral_0_arsize; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_arcache = l_soc_peripheral_0_arcache; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_arprot = l_soc_peripheral_0_arprot; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_axi4_0_rready = l_soc_peripheral_0_rready; // @[SimTop.scala 41:21]
  assign l_simMMIO_io_dma_0_awready = l_soc_dma_0_awready; // @[SimTop.scala 42:20]
  assign l_simMMIO_io_dma_0_wready = l_soc_dma_0_wready; // @[SimTop.scala 42:20]
  assign l_simMMIO_io_dma_0_bvalid = l_soc_dma_0_bvalid; // @[SimTop.scala 42:20]
  assign l_simMMIO_io_dma_0_bid = l_soc_dma_0_bid; // @[SimTop.scala 42:20]
  assign l_simMMIO_io_dma_0_arready = l_soc_dma_0_arready; // @[SimTop.scala 42:20]
  assign l_simMMIO_io_dma_0_rvalid = l_soc_dma_0_rvalid; // @[SimTop.scala 42:20]
  assign l_simMMIO_io_dma_0_rid = l_soc_dma_0_rid; // @[SimTop.scala 42:20]
  assign l_simMMIO_io_dma_0_rdata = l_soc_dma_0_rdata; // @[SimTop.scala 42:20]
  assign l_simMMIO_io_uart_in_ch = io_uart_in_ch; // @[SimTop.scala 82:19]
  assign l_simAXIMem_clock = clock;
  assign l_simAXIMem_reset = reset;
  assign l_simAXIMem_io_axi4_0_awvalid = l_soc_memory_0_awvalid; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awid = l_soc_memory_0_awid; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awaddr = l_soc_memory_0_awaddr; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awlen = l_soc_memory_0_awlen; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awsize = l_soc_memory_0_awsize; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awburst = l_soc_memory_0_awburst; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awlock = l_soc_memory_0_awlock; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awcache = l_soc_memory_0_awcache; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awprot = l_soc_memory_0_awprot; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_awqos = l_soc_memory_0_awqos; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_wvalid = l_soc_memory_0_wvalid; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_wdata = l_soc_memory_0_wdata; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_wstrb = l_soc_memory_0_wstrb; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_wlast = l_soc_memory_0_wlast; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_bready = l_soc_memory_0_bready; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arvalid = l_soc_memory_0_arvalid; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arid = l_soc_memory_0_arid; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_araddr = l_soc_memory_0_araddr; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arlen = l_soc_memory_0_arlen; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arsize = l_soc_memory_0_arsize; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arburst = l_soc_memory_0_arburst; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arlock = l_soc_memory_0_arlock; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arcache = l_soc_memory_0_arcache; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arprot = l_soc_memory_0_arprot; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_arqos = l_soc_memory_0_arqos; // @[SimTop.scala 49:25]
  assign l_simAXIMem_io_axi4_0_rready = l_soc_memory_0_rready; // @[SimTop.scala 49:25]
  assign SimJTAG_clock = clock; // @[RocketDebugWrapper.scala 131:11]
  assign SimJTAG_reset = reset; // @[SimTop.scala 69:95]
  assign SimJTAG_jtag_TDO_data = l_soc_io_systemjtag_jtag_TDO_data; // @[RocketDebugWrapper.scala 129:14]
  assign SimJTAG_jtag_TDO_driven = l_soc_io_systemjtag_jtag_TDO_driven; // @[RocketDebugWrapper.scala 129:14]
  assign SimJTAG_enable = 1'h1; // @[RocketDebugWrapper.scala 134:15]
  assign SimJTAG_init_done = ~reset; // @[SimTop.scala 69:103]
  always @(posedge clock) begin
    if (reset) begin // @[SimTop.scala 60:27]
      rtcCounter <= 6'h0; // @[SimTop.scala 60:27]
    end else if (rtcCounter == 6'h31) begin // @[SimTop.scala 61:20]
      rtcCounter <= 6'h0;
    end else begin
      rtcCounter <= _rtcCounter_T_2;
    end
    if (reset) begin // @[SimTop.scala 62:25]
      rtcClock <= 1'h0; // @[SimTop.scala 62:25]
    end else if (rtcCounter == 6'h0) begin // @[SimTop.scala 63:29]
      rtcClock <= ~rtcClock; // @[SimTop.scala 64:14]
    end
  end
// Register and memory initialization
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE_MEM_INIT
  integer initvar;
`endif
`ifndef SYNTHESIS
`ifdef FIRRTL_BEFORE_INITIAL
`FIRRTL_BEFORE_INITIAL
`endif
initial begin
  `ifdef RANDOMIZE
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      `ifdef RANDOMIZE_DELAY
        #`RANDOMIZE_DELAY begin end
      `else
        #0.002 begin end
      `endif
    `endif
`ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  rtcCounter = _RAND_0[5:0];
  _RAND_1 = {1{`RANDOM}};
  rtcClock = _RAND_1[0:0];
`endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end // initial
`ifdef FIRRTL_AFTER_INITIAL
`FIRRTL_AFTER_INITIAL
`endif
`endif // SYNTHESIS
endmodule

