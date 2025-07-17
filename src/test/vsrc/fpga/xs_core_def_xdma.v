`include "sys_define.vh"

module xs_core_def (
      input                                      ddr_clk_p,
      input                                      ddr_clk_n, 
      input                                      tmclk,
      input                                      cqetmclk,
      output                                     ui_clk,
      output                                     init_calib_complete,
      output                                     cpu_rd_qspi_valid,
      output                                     cpu_wr_ddr_valid,
      input                                      sys_clk_i,
      input                                      dev_clk_i,
      input                                      sys_rstn,
      input                                      cpu_rstn,
      input                                      rstn_sw4,
`ifdef  XS_XMDA_EP      
      input       [7:0]                          pci_ep_rxn,
      input       [7:0]                          pci_ep_rxp,
      output      [7:0]                          pci_ep_txn,
      output      [7:0]                          pci_ep_txp,
      input                                      pcie_ep_gt_ref_clk_n,
      input                                      pcie_ep_gt_ref_clk_p,
      output                                     pcie_ep_lnk_up,
      input                                      pcie_ep_perstn,
`endif  
`ifdef  XS_UART   
      input                                      uart0_sin,
      input                                      uart1_sin,
      input                                      uart2_sin,
      output                                     uart0_sout,
      output                                     uart1_sout,
      output                                     uart2_sout,
`endif
      output                                     sd_card_clk_out,   
      output                                     sd_cmd_out     ,       
      output                                     sd_cmd_out_oe  ,       
      output          [3:0]                      sd_dat_out     ,       
      output          [3:0]                      sd_dat_out_oe  ,       
      input                                      sd_cmd_in      ,
      input           [3:0]                      sd_dat_in      ,
      input                                      sd_card_det_in ,
      input                                      sd_card_wp_in  ,
      output      [2:0]                          sd_vdd1_sel                    ,
      output                                     sd_vdd1_on                     ,
      output      [1:0]                          uhs1_drv_sth                   ,
      output                                     uhs1_swvolt_en                 ,
      output                                     sd_led_control                 ,
      output      [0:0]                          DDR_CK_T                       ,
      output      [0:0]                          DDR_CK_C                       ,
      output      [0:0]                          DDR_CKE                        ,
      output      [0:0]                          DDR_CS_N                       ,
      output      [0:0]                          DDR_ODT                        ,
      output                                     DDR_ACT_N                      ,
      output      [1:0]                          DDR_BG                         ,
      output      [1:0]                          DDR_BA                         ,
      output      [16:0]                         DDR_A                          ,
      output                                     DDR_RESET_N                    ,
      inout           [7:0]                      DDR_DM_N                       ,
      inout           [63:0]                     DDR_DQ                         ,
      inout           [7:0]                      DDR_DQS_T                      ,
      inout           [7:0]                      DDR_DQS_C                      ,
      
      //==JTAG
      
      input           io_systemjtag_jtag_TCK,         // come from gpio
      input           io_systemjtag_jtag_TMS,         // come from gpio
      input           io_systemjtag_jtag_TDI,         // come from gpio
      output          io_systemjtag_jtag_TDO_data,    // come from gpio
      output          io_systemjtag_jtag_TDO_driven,  // come from gpio
      input           io_systemjtag_reset,            // come from gpio
      
      //==gmac
`ifdef  XS_GMAC
      output                                     io_gmac_mdo_oe                 ,
      output                                     io_gmac_mdo                    ,
      output                                     io_gmac_mck_out                ,
      input                                      io_gmac_mdi                    ,
      output                                     io_gmac_tx_clk                 ,
      output                                     io_gmac_txd_en                 ,
      input                                      io_gmac_rx_clk                 ,
      input                                      io_gmac_rxd_vld                ,
      input             [3:0]                    io_gmac_rxd                    ,
      output            [3:0]                    io_gmac_txd                    ,
`endif

      //==input
      input                                      dft_lgc_rst_n                  ,
      input                                      dft_se                         ,
      //input                                      xtal_clk_24m                 ,
      input           [1:0]                      chip_mode_i                    ,
      //input           [53-1:0]                   pad_c                        ,
      input                                      dft_crg_rst_n   
);

// Unbind useless output port {{{
assign cpu_rd_qspi_valid = 0;
assign cpu_wr_ddr_valid = 0;
assign uart1_sout = 0;
assign uart2_sout = 0;
assign sd_card_clk_out = 0;
assign sd_cmd_out = 0;
assign sd_cmd_out_oe = 0;
assign sd_dat_out = 0;
assign sd_dat_out_oe = 0;
assign sd_vdd1_sel = 0;
assign sd_vdd1_on = 0;
assign uhs1_drv_sth = 0;
assign uhs1_swvolt_en = 0;
assign sd_led_control = 0;
// }}} Unbind useless output port

wire                       inter_soc_clk               ; 
wire                       axi_bclk_sync_rstn        ; 
wire                       ddr_bus_clk               ; 
wire                       ddr_bclk_sync_rstn        ; 
`ifdef  XS_UART 
wire                       uart_pclk                 ; 
wire                       uart_pclk_sync_rstn       ; 
wire                       uart_sclk                 ; 
wire                       uart_sclk_sync_rstn       ; 
`endif
wire                       qspi_sclk                 ; 
wire                       qspi_pclk                 ; 
wire                       qspi_pclk_sync_rstn       ; 
wire                       qspi_hclk                 ; 
wire                       qspi_hclk_sync_rstn       ; 
wire                       qspi_ref_clk              ; 
wire                       qspi_rclk_sync_rstn       ; 
wire                       sd_axi_clk                ; 
wire                       sd_aclk_sync_rstn         ; 
wire                       sd_ahb_clk                ; 
wire                       sd_hclk_sync_rstn         ; 
wire                       sd_bclk                   ; 
wire                       sd_bclk_sync_rstn         ; 
wire                       sd_tmclk                  ; 
wire                       sd_tclk_sync_rstn         ; 
wire                       sd_cqetmclk               ; 
wire                       sd_cqetclk_sync_rstn      ; 
wire                                           apb_bus_clk_100m               ;
wire                                           apb_bus_rst_n                  ;
wire            [31:0]                         hpm_dig_result                 ;
wire            [15:0]                         syscfg_paddr_mix               ;
wire                                           syscfg_psel                    ;
wire                                           syscfg_penable                 ;
wire                                           syscfg_pwrite                  ;
wire            [31:0]                         syscfg_pwdata                  ;
wire                                           syscfg_pready                  ;
wire            [31:0]                         syscfg_prdata                  ;
wire                                           syscfg_pslverr                 ;
wire                                           sd_qos_sel_cfg                 ;
wire                                           dp_qos_sel_cfg                 ;
wire                                           gpu_qos_sel_cfg                ;
wire                                           gmac_qos_sel_cfg               ;
wire                                           dma_qos_sel_cfg                ;
wire                                           usb_qos_sel_cfg                ;
wire            [11:0]                         hpm_data_svt                   ;
wire            [11:0]                         hpm_data_lvt                   ;
wire            [11:0]                         hpm_data_ulvt                  ;
wire            [3:0]                          cfg_gpu_m_awqos                ;
wire            [3:0]                          cfg_gpu_m_arqos                ;
wire            [3:0]                          cfg_dp_m_awqos                 ;
wire            [3:0]                          cfg_dp_m_arqos                 ;
wire            [3:0]                          cfg_gmac_m_awqos               ;
wire            [3:0]                          cfg_gmac_m_arqos               ;
wire            [3:0]                          cfg_sd_m_awqos                 ;
wire            [3:0]                          cfg_sd_m_arqos                 ;
wire            [3:0]                          cfg_dma_m_awqos                ;
wire            [3:0]                          cfg_dma_m_arqos                ;
wire                                           cfg_gpu_addr_offset_en         ;
wire                                           cfg_gmac_addr_offset_en        ;
wire                                           sdio_voltage_sw_cfg            ;
wire            [3:0]                          gpio_test_mux_cfg              ;
wire                                           ddr_rrb_sram_rme_cfg           ;
wire            [3:0]                          ddr_rrb_sram_rm_cfg            ;
wire                                           ddr_data_sram_rme_cfg          ;
wire            [3:0]                          ddr_data_sram_rm_cfg           ;
wire            [15:0]                         cpu_sram_cfg                   ;
wire            [1:0]                          usb3_timing_opt_cfg            ;
wire                                           dp_frame_start                 ;
wire                                           ahb_bus_clk_200m               ;
wire                                           axi_bus_clk_400m               ;
wire                                           ahb_bus_rst_n                  ;
wire                                           axi_bus_rst_n                  ;
wire                                           gpio0_fun_sel                  ;
wire                                           gpio1_fun_sel                  ;
wire                                           gpio2_fun_sel                  ;
wire                                           gpio3_fun_sel                  ;
wire                                           gpio4_fun_sel                  ;
wire                                           gpio5_fun_sel                  ;
wire                                           gpio6_fun_sel                  ;
wire                                           gpio7_fun_sel                  ;
wire                                           gpio8_fun_sel                  ;
wire                                           gpio9_fun_sel                  ;
wire                                           gpio10_fun_sel                 ;
wire                                           gpio11_fun_sel                 ;
wire                                           gpio12_fun_sel                 ;
wire                                           gpio13_fun_sel                 ;
wire                                           gpio14_fun_sel                 ;
wire                                           gpio15_fun_sel                 ;
wire                                           gpio16_fun_sel                 ;
wire                                           gpio17_fun_sel                 ;
wire                                           gpio18_fun_sel                 ;
wire                                           gpio19_fun_sel                 ;
wire                                           gpio20_fun_sel                 ;
wire                                           gpio21_fun_sel                 ;
wire                                           gpio22_fun_sel                 ;
wire                                           gpio23_fun_sel                 ;
wire                                           gpio24_fun_sel                 ;
wire                                           gpio25_fun_sel                 ;
wire                                           gpio26_fun_sel                 ;
wire                                           gpio27_fun_sel                 ;
wire                                           gpio28_fun_sel                 ;
wire                                           gpio29_fun_sel                 ;
wire                                           gpio30_fun_sel                 ;
wire                                           gpio31_fun_sel                 ;
wire                                           gpio32_fun_sel                 ;
wire                                           gpio33_fun_sel                 ;
wire                                           gpio34_fun_sel                 ;
wire                                           gpio35_fun_sel                 ;
wire                                           gpio36_fun_sel                 ;
wire                                           gpio37_fun_sel                 ;
wire                                           gpio38_fun_sel                 ;
wire                                           gpio39_fun_sel                 ;
wire                                           gpio40_fun_sel                 ;
wire                                           gpio41_fun_sel                 ;
wire                                           gpio42_fun_sel                 ;
wire                                           gpio43_fun_sel                 ;
wire                                           gpio44_fun_sel                 ;
wire                                           gpio45_fun_sel                 ;
wire                                           gpio46_fun_sel                 ;
wire                                           gpio47_fun_sel                 ;
wire                                           gpio48_fun_sel                 ;
wire                                           gpio49_fun_sel                 ;
wire                                           gpio50_fun_sel                 ;
wire                                           gpio51_fun_sel                 ;
wire                                           gpio52_fun_sel                 ;
`ifdef  XS_GMAC
wire            [31:0]                         gmac_m_awaddr                  ;
wire            [31:0]                         gmac_m_araddr                  ;
`endif
wire            [31:0]                         gpu_m_araddr                   ;
wire            [31:0]                         gpu_m_awaddr                   ;
wire            [31:0]                         qspi_haddr                     ;
wire            [31:0]                         gpu_haddr                      ;
wire            [31:0]                         dma_haddr                      ;
wire            [31:0]                         sd_haddr                       ;
wire            [31:0]                         usb_haddr                      ;
wire            [31:0]                         qos_haddr                      ;
wire            [52:0]                         io_function_select             ;
`ifdef  XS_GMAC
wire            [39:0]                         gmac_m_awaddr_mix              ;
wire            [39:0]                         gmac_m_araddr_mix              ;
`endif
wire            [39:0]                         gpu_m_araddr_mix               ;
wire            [39:0]                         gpu_m_awaddr_mix               ;
`ifdef  XS_QSPI2ROM
wire            [31:0]                         qspi_haddr_mix_pre                 ;
(*mark_debug = "true"*) wire            [31:0]                         qspi_haddr_mix                 ;
`endif
wire            [31:0]                         gpu_haddr_mix                  ;
wire            [31:0]                         dma_haddr_mix                  ;
wire            [31:0]                         sd_haddr_mix                   ;
wire            [63:0]                         usb_haddr_mix                  ;
wire            [31:0]                         qos_haddr_mix                  ;
wire                                           sd_wakeup_int                  ;
wire                                           sd_int                         ;
wire                                           dp_de_int                      ;
wire                                           dp_se_int                      ;
wire                                           hdmiphy_int                    ;
wire                                           hdmitx_wakeup                  ;
wire                                           hdmitx_int                     ;
wire                                           wdt_int                        ;
wire                                           gpu_int                        ;
//`ifdef  XS_GMAC
wire                                           gmac_lpi_int                   ;
wire                                           gmac_sbd_int                   ;
wire                                           gmac_pmt_int                   ;
//`endif
(*mark_debug = "true"*) wire                                           qspi_int                       ;
wire                                           i2s_int                        ;
//`ifdef  XS_UART 
wire                                           uart2_int                      ;
wire                                           uart1_int                      ;
(*mark_debug = "true"*) wire                                           uart0_int                      ;
//`endif
wire                                           i2c2_int                       ;
wire                                           i2c1_int                       ;
wire                                           i2c0_int                       ;
wire            [31:0]                         gpio_int                       ;
wire                                           dma_int                        ;
wire            [63:0]                         cpu_int_mix                        ;
wire            [3:0]                          sd_m_awqos                     ;
wire            [3:0]                          sd_m_arqos                     ;
wire            [3:0]                          dp_m_awqos                     ;
wire            [3:0]                          dp_m_arqos                     ;
wire            [3:0]                          dma_m_awqos                    ;
wire            [3:0]                          dma_m_arqos                    ;
wire            [3:0]                          gmac_m_awqos_mix               ;
wire            [3:0]                          gmac_m_arqos_mix               ;
wire            [3:0]                          sd_m_awqos_mix                 ;
wire            [3:0]                          sd_m_arqos_mix                 ;
wire            [3:0]                          dp_m_awqos_mix                 ;
wire            [3:0]                          dp_m_arqos_mix                 ;
wire            [3:0]                          dma_m_awqos_mix                ;
wire            [3:0]                          dma_m_arqos_mix                ;
wire            [3:0]                          gpu_m_awqos_mix                ;
wire            [3:0]                          gpu_m_arqos_mix                ;
wire                                           cpu_pll_clk_test               ;
wire                                           cpu_pll_lock_test              ;
wire            [1:0]                          soc_pll_clk_test               ;
wire            [4:0]                          soc_pll_lock_test              ;
wire                                           ddr_pll_lock_test              ;
wire                                           dft_mode                       ;
wire                                           scan_mode                      ;
wire                                           sys_peri_rst_n                 ;
wire            [13:0]                         data_cpu_bridge_m2s_awid       ;
wire            [35:0]                         data_cpu_bridge_m2s_awaddr     ;
wire            [7:0]                          data_cpu_bridge_m2s_awlen      ;
wire            [2:0]                          data_cpu_bridge_m2s_awsize     ;
wire            [1:0]                          data_cpu_bridge_m2s_awburst    ;
wire                                           data_cpu_bridge_m2s_awlock     ;
wire            [3:0]                          data_cpu_bridge_m2s_awcache    ;
wire            [2:0]                          data_cpu_bridge_m2s_awprot     ;
wire                                           data_cpu_bridge_m2s_awvalid    ;
wire            [255:0]                        data_cpu_bridge_m2s_wdata      ;
wire            [31:0]                         data_cpu_bridge_m2s_wstrb      ;
wire                                           data_cpu_bridge_m2s_wlast      ;
wire                                           data_cpu_bridge_m2s_wvalid     ;
wire                                           data_cpu_bridge_m2s_bready     ;
wire            [13:0]                         data_cpu_bridge_m2s_arid       ;
wire            [35:0]                         data_cpu_bridge_m2s_araddr     ;
wire            [7:0]                          data_cpu_bridge_m2s_arlen      ;
wire            [2:0]                          data_cpu_bridge_m2s_arsize     ;
wire            [1:0]                          data_cpu_bridge_m2s_arburst    ;
wire                                           data_cpu_bridge_m2s_arlock     ;
wire            [3:0]                          data_cpu_bridge_m2s_arcache    ;
wire            [2:0]                          data_cpu_bridge_m2s_arprot     ;
wire                                           data_cpu_bridge_m2s_arvalid    ;
wire                                           data_cpu_bridge_m2s_rready     ;
wire                                           data_cpu_bridge_s2m_awready    ;
wire                                           data_cpu_bridge_s2m_wready     ;
wire            [13:0]                         data_cpu_bridge_s2m_bid        ;
wire            [1:0]                          data_cpu_bridge_s2m_bresp      ;
wire                                           data_cpu_bridge_s2m_bvalid     ;
wire                                           data_cpu_bridge_s2m_arready    ;
wire            [13:0]                         data_cpu_bridge_s2m_rid        ;
wire            [255:0]                        data_cpu_bridge_s2m_rdata      ;
wire            [1:0]                          data_cpu_bridge_s2m_rresp      ;
wire                                           data_cpu_bridge_s2m_rlast      ;
wire                                           data_cpu_bridge_s2m_rvalid     ;
wire            [3:0]                          data_cpu_bridge_m2s_awqos      ;
wire            [3:0]                          data_cpu_bridge_m2s_arqos      ;
wire                                           dft_glb_gt_se                  ;
wire                                           dft_dp_rst_disable             ;
wire                                           dft_dp_ram_hold                ;
wire                                           dft_dp_cg_en                   ;
wire                                           dft_dp_pclk_disable            ;
wire                                           dft_dp_aclk_disable            ;
wire                                           dft_dp_mclk_disable            ;
wire                                           dft_dp_pixlclk_disable         ;
wire                                           sys_bus_rst_n                  ;
wire                                           cpu_bak_clk                    ;
wire                                           sys_cpu_rst                    ;
wire            [3:0]                          cpu_pll0_bypass_cfg            ;
wire                                           cpu_jtag_tck                   ;
wire                                           cpu_jtag_tms                   ;
wire                                           cpu_jtag_tdi                   ;
wire                                           cpu_jtag_trst                  ;
wire                                           cpu_jtag_tdo                   ;
wire                                           cpu_jtag_tdo_oen               ;
wire            [11:0]                         cpu_pll_test_info              ;

(*mark_debug = "true"*) wire                                       cpu2ddr_s2m_awready            ;
(*mark_debug = "true"*) wire                                       cpu2ddr_s2m_wready             ;
wire            [13:0]                          cpu2ddr_s2m_bid_mix            ;
(*mark_debug = "true"*) wire                                       cpu2ddr_s2m_bvalid             ;
(*mark_debug = "true"*) wire                                       cpu2ddr_s2m_arready            ;
wire            [13:0]                          cpu2ddr_s2m_rid_mix            ;
(*mark_debug = "true"*) wire                                       cpu2ddr_s2m_rlast              ;
(*mark_debug = "true"*) wire                                       cpu2ddr_s2m_rvalid             ;
wire            [13:0]                          cpu2ddr_m2s_awid               ;
(*mark_debug = "true"*) wire            [35:0]                     cpu2ddr_m2s_awaddr             ;
wire                                            cpu2ddr_m2s_awlock             ;
(*mark_debug = "true"*) wire                                       cpu2ddr_m2s_awvalid            ;
(*mark_debug = "true"*) wire                                       cpu2ddr_m2s_wlast              ;
(*mark_debug = "true"*) wire                                       cpu2ddr_m2s_wvalid             ;
(*mark_debug = "true"*) wire                                       cpu2ddr_m2s_bready             ;
wire            [13:0]                          cpu2ddr_m2s_arid               ;
(*mark_debug = "true"*) wire            [35:0]                     cpu2ddr_m2s_araddr             ;
wire                                            cpu2ddr_m2s_arlock             ;
(*mark_debug = "true"*) wire                                       cpu2ddr_m2s_arvalid            ;
(*mark_debug = "true"*) wire                                       cpu2ddr_m2s_rready             ;

(*mark_debug = "true"*) wire                                           cpu2cfg_s2m_awready            ;
(*mark_debug = "true"*) wire                                           cpu2cfg_s2m_wready             ;
(*mark_debug = "true"*) wire            [1:0]                          cpu2cfg_s2m_bid                ;
(*mark_debug = "true"*) wire            [1:0]                          cpu2cfg_s2m_bresp              ;
(*mark_debug = "true"*) wire                                           cpu2cfg_s2m_bvalid             ;
(*mark_debug = "true"*) wire                                           cpu2cfg_s2m_arready            ;
(*mark_debug = "true"*) wire            [1:0]                          cpu2cfg_s2m_rid                ;
(*mark_debug = "true"*) wire            [63:0]                         cpu2cfg_s2m_rdata              ;
(*mark_debug = "true"*) wire            [1:0]                          cpu2cfg_s2m_rresp              ;
(*mark_debug = "true"*) wire                                           cpu2cfg_s2m_rlast              ;
(*mark_debug = "true"*) wire                                           cpu2cfg_s2m_rvalid             ;
(*mark_debug = "true"*) wire            [1:0]                          cpu2cfg_m2s_awid               ;
(*mark_debug = "true"*) wire            [30:0]                         cpu2cfg_m2s_awaddr             ;
(*mark_debug = "true"*) wire            [3:0]                          cpu2cfg_m2s_awregion           ;
(*mark_debug = "true"*) wire            [7:0]                          cpu2cfg_m2s_awlen              ;
(*mark_debug = "true"*) wire            [2:0]                          cpu2cfg_m2s_awsize             ;
(*mark_debug = "true"*) wire            [1:0]                          cpu2cfg_m2s_awburst            ;
(*mark_debug = "true"*) wire                                           cpu2cfg_m2s_awlock             ;
(*mark_debug = "true"*) wire            [3:0]                          cpu2cfg_m2s_awcache            ;
(*mark_debug = "true"*) wire            [2:0]                          cpu2cfg_m2s_awprot             ;
(*mark_debug = "true"*) wire            [3:0]                          cpu2cfg_m2s_awqos              ;
(*mark_debug = "true"*) wire                                           cpu2cfg_m2s_awvalid            ;
(*mark_debug = "true"*) wire            [63:0]                         cpu2cfg_m2s_wdata              ;
(*mark_debug = "true"*) wire            [7:0]                          cpu2cfg_m2s_wstrb              ;
(*mark_debug = "true"*) wire                                           cpu2cfg_m2s_wlast              ;
(*mark_debug = "true"*) wire                                           cpu2cfg_m2s_wvalid             ;
(*mark_debug = "true"*) wire                                           cpu2cfg_m2s_bready             ;
(*mark_debug = "true"*) wire            [1:0]                          cpu2cfg_m2s_arid               ;
(*mark_debug = "true"*) wire            [30:0]                         cpu2cfg_m2s_araddr             ;
(*mark_debug = "true"*) wire            [3:0]                          cpu2cfg_m2s_arregion           ;
(*mark_debug = "true"*) wire            [7:0]                          cpu2cfg_m2s_arlen              ;
(*mark_debug = "true"*) wire            [2:0]                          cpu2cfg_m2s_arsize             ;
(*mark_debug = "true"*) wire            [1:0]                          cpu2cfg_m2s_arburst            ;
(*mark_debug = "true"*) wire                                           cpu2cfg_m2s_arlock             ;
(*mark_debug = "true"*) wire            [3:0]                          cpu2cfg_m2s_arcache            ;
(*mark_debug = "true"*) wire            [2:0]                          cpu2cfg_m2s_arprot             ;
(*mark_debug = "true"*) wire            [3:0]                          cpu2cfg_m2s_arqos              ;
(*mark_debug = "true"*) wire                                           cpu2cfg_m2s_arvalid            ;
(*mark_debug = "true"*) wire                                           cpu2cfg_m2s_rready             ;

wire                                           axi_bus_clk_800m               ;
wire                                           adb_bus_rst_n                  ;
wire            [13:0]                         data_x2x_bridge_m2s_awid       ;
wire            [35:0]                         data_x2x_bridge_m2s_awaddr_mix ;
wire            [7:0]                          data_x2x_bridge_m2s_awlen      ;
wire            [2:0]                          data_x2x_bridge_m2s_awsize     ;
wire            [1:0]                          data_x2x_bridge_m2s_awburst    ;
wire                                           data_x2x_bridge_m2s_awlock     ;
wire            [3:0]                          data_x2x_bridge_m2s_awcache    ;
wire            [2:0]                          data_x2x_bridge_m2s_awprot     ;
wire            [3:0]                          data_x2x_bridge_m2s_awqos      ;
wire                                           data_x2x_bridge_m2s_awvalid    ;
wire            [127:0]                        data_x2x_bridge_m2s_wdata      ;
wire            [15:0]                         data_x2x_bridge_m2s_wstrb      ;
wire                                           data_x2x_bridge_m2s_wlast      ;
wire                                           data_x2x_bridge_m2s_wvalid     ;
wire                                           data_x2x_bridge_m2s_bready     ;
wire            [13:0]                         data_x2x_bridge_m2s_arid       ;
wire            [35:0]                         data_x2x_bridge_m2s_araddr_mix ;
wire            [7:0]                          data_x2x_bridge_m2s_arlen      ;
wire            [2:0]                          data_x2x_bridge_m2s_arsize     ;
wire            [1:0]                          data_x2x_bridge_m2s_arburst    ;
wire                                           data_x2x_bridge_m2s_arlock     ;
wire            [3:0]                          data_x2x_bridge_m2s_arcache    ;
wire            [2:0]                          data_x2x_bridge_m2s_arprot     ;
wire            [3:0]                          data_x2x_bridge_m2s_arqos      ;
wire                                           data_x2x_bridge_m2s_arvalid    ;
wire                                           data_x2x_bridge_m2s_rready     ;
wire                                           data_x2x_bridge_s2m_awready    ;
wire                                           data_x2x_bridge_s2m_wready     ;
wire            [13:0]                         data_x2x_bridge_s2m_bid        ;
wire            [1:0]                          data_x2x_bridge_s2m_bresp      ;
wire                                           data_x2x_bridge_s2m_bvalid     ;
wire                                           data_x2x_bridge_s2m_arready    ;
wire            [13:0]                         data_x2x_bridge_s2m_rid        ;
wire            [127:0]                        data_x2x_bridge_s2m_rdata      ;
wire            [1:0]                          data_x2x_bridge_s2m_rresp      ;
wire                                           data_x2x_bridge_s2m_rlast      ;
wire                                           data_x2x_bridge_s2m_rvalid     ;
wire            [39:0]                         data_x2x_bridge_m2s_awaddr     ;
wire            [39:0]                         data_x2x_bridge_m2s_araddr     ;
(*mark_debug = "true"*) wire           [17:0]                       cpu2ddr_m2s_awid_mix           ;
(*mark_debug = "true"*) wire           [39:0]                      cpu2ddr_m2s_awaddr_mix         ;
(*mark_debug = "true"*) wire           [7:0]                       cpu2ddr_m2s_awlen              ;
(*mark_debug = "true"*) wire           [2:0]                       cpu2ddr_m2s_awsize             ;
(*mark_debug = "true"*) wire           [1:0]                       cpu2ddr_m2s_awburst            ;
wire           [3:0]                       cpu2ddr_m2s_awcache            ;
wire           [2:0]                       cpu2ddr_m2s_awprot             ;
wire           [3:0]                       cpu2ddr_m2s_awqos              ;
wire           [3:0]                       cpu2ddr_m2s_awregion           ;
(*mark_debug = "true"*) wire           [255:0]                     cpu2ddr_m2s_wdata              ;
(*mark_debug = "true"*) wire           [31:0]                      cpu2ddr_m2s_wstrb              ;
(*mark_debug = "true"*) wire           [17:0]                       cpu2ddr_m2s_arid_mix           ;
(*mark_debug = "true"*) wire           [39:0]                      cpu2ddr_m2s_araddr_mix         ;
(*mark_debug = "true"*) wire           [7:0]                       cpu2ddr_m2s_arlen              ;
(*mark_debug = "true"*) wire           [2:0]                       cpu2ddr_m2s_arsize             ;
(*mark_debug = "true"*) wire           [1:0]                       cpu2ddr_m2s_arburst            ;
wire           [3:0]                       cpu2ddr_m2s_arcache            ;
wire           [2:0]                       cpu2ddr_m2s_arprot             ;
wire           [3:0]                       cpu2ddr_m2s_arqos              ;
wire           [3:0]                       cpu2ddr_m2s_arregion           ;
(*mark_debug = "true"*) wire           [17:0]                      cpu2ddr_s2m_bid                ;
(*mark_debug = "true"*) wire           [1:0]                       cpu2ddr_s2m_bresp              ;
(*mark_debug = "true"*) wire           [17:0]                      cpu2ddr_s2m_rid                ;
(*mark_debug = "true"*) wire           [255:0]                     cpu2ddr_s2m_rdata              ;
(*mark_debug = "true"*) wire           [1:0]                       cpu2ddr_s2m_rresp              ;
wire                                           ddr_core_clk                   ;
wire                                           sys_ddr_rst_n                  ;
wire                                           normal_mode                    ;
wire                                           phy_bist_mode                  ;
wire                                           mbist_mode                     ;
wire            [1:0]                          dft_qspi_clk_sel               ;
wire                                           dft_dp_clk_sel                 ;
wire            [1:0]                          dft_test_clk_sel               ;
wire                                           dft_crg_pre_gt_se              ;
wire                                           dft_rstn_sel                   ;
wire            [4:0]                          dft_pll_clksel                 ;
wire            [1:0]                          dft_gmac_clk_sel               ;
wire            [1:0]                          dft_uart0_clk_sel              ;
wire            [1:0]                          dft_uart1_clk_sel              ;
wire            [1:0]                          dft_uart2_clk_sel              ;
wire                                           dft_bclk_sel                   ;
wire            [2:0]                          io_phy_test_sel                ;
wire                                           io_phy_ate_rst_n               ;
wire                                           io_phy_ate_paddr               ;
wire                                           io_phy_ate_pclk                ;
wire                                           io_phy_ate_prst_n              ;
wire                                           io_phy_ate_psel                ;
wire                                           io_phy_ate_pwdata              ;
wire                                           io_phy_ate_pwrite              ;
wire                                           io_phy_ate_prdata              ;
wire                                           io_phy_ate_pready              ;
wire            [31:0]                         io_gpio_out_oe                 ;
wire            [31:0]                         io_gpio_out                    ;
wire            [31:0]                         io_gpio_in                     ;
wire                                           io_i2c0_clk_oe                 ;
wire                                           io_i2c0_data_oe                ;
wire                                           io_i2c1_clk_oe                 ;
wire                                           io_i2c1_data_oe                ;
wire                                           io_i2c2_clk_oe                 ;
wire                                           io_i2c2_data_oe                ;
wire                                           io_i2c0_clk_in                 ;
wire                                           io_i2c0_data_in                ;
wire                                           io_i2c1_clk_in                 ;
wire                                           io_i2c1_data_in                ;
wire                                           io_i2c2_clk_in                 ;
wire                                           io_i2c2_data_in                ;
wire                                           io_uart0_txd                   ;
wire                                           io_uart0_rxd                   ;
wire                                           io_uart1_txd                   ;
wire                                           io_uart1_rxd                   ;
wire                                           io_uart2_txd                   ;
wire                                           io_uart2_rxd                   ;
wire                                           io_qspi_sclk_out               ;
wire                                           io_qspi_cs_out_n_mix           ;
wire                                           io_qspi_mo0                    ;
wire                                           io_qspi_mo1                    ;
wire                                           io_qspi_mo2                    ;
wire                                           io_qspi_mo3                    ;
wire            [3:0]                          io_qspi_mo_oe_n                ;
wire                                           io_qspi_mi0                    ;
wire                                           io_qspi_mi1                    ;
wire                                           io_qspi_mi2                    ;
wire                                           io_qspi_mi3                    ;
`ifdef  XS_GMAC
wire                                           io_gmac_mdo_oe                 ;
wire                                           io_gmac_mdo                    ;
wire                                           io_gmac_mck_out                ;
wire                                           io_gmac_mdi                    ;
wire                                           io_gmac_tx_clk                 ;
wire                                           io_gmac_txd_en                 ;
wire            [3:0]                          io_gmac_txd_mix                ;
wire                                           io_gmac_rx_clk                 ;
wire                                           io_gmac_rxd_vld                ;
wire            [3:0]                          io_gmac_rxd                    ;
`endif
wire                                           cpu_jtag_trst_n                ;
wire            [31:0]                         crg_dbug_out                   ;
wire            [15:0]                         crg_paddr_mix                  ;
wire                                           crg_psel                       ;
wire                                           crg_penable                    ;
wire                                           crg_pwrite                     ;
wire            [31:0]                         crg_pwdata                     ;
wire                                           crg_pready                     ;
wire            [31:0]                         crg_prdata                     ;
wire                                           crg_pslverr                    ;
wire                                           sys_glb_rst_n                  ;
wire                                           sys_crg_rst_n                  ;
wire                                           sys_cpu_rst_n                  ;
wire                                           wdt_sys_rst_req_inv            ;
wire            [1:0]                          gmac_speed_mode                ;
wire                                           gpio53_fun_sel                 ;
wire                                           i2c0_clk_src                   ;
wire                                           i2c1_clk_src                   ;
wire                                           i2c2_clk_src                   ;
wire                                           i2c2_cken                      ;
wire                                           i2c1_cken                      ;
wire                                           i2c0_cken                      ;
wire                                           i2c2_apb_cken                  ;
wire                                           i2c1_apb_cken                  ;
wire                                           i2c0_apb_cken                  ;
wire                                           i2c2_srst_req                  ;
wire                                           i2c1_srst_req                  ;
wire                                           i2c0_srst_req                  ;
wire                                           uart2_sys_clk_src              ;
wire                                           uart1_sys_clk_src              ;
wire                                           uart0_sys_clk_src              ;
wire                                           uart2_apb_cken                 ;
wire                                           uart1_apb_cken                 ;
wire                                           uart0_apb_cken                 ;
wire                                           uart2_sys_cken                 ;
wire                                           uart1_sys_cken                 ;
wire                                           uart0_sys_cken                 ;
wire                                           uart2_srst_req                 ;
wire                                           uart1_srst_req                 ;
wire                                           uart0_srst_req                 ;
wire                                           qspi_ref_clk_src               ;
wire                                           qspi_ref_cken                  ;
wire                                           qspi_ahb_cken                  ;
wire                                           qspi_apb_cken                  ;
wire                                           qspi_srst_req                  ;
wire                                           dma_ahb_cken                   ;
wire                                           dma_axi_cken                   ;
wire                                           dma_srst_req                   ;
wire                                           gmac_tx_clk                    ;
wire                                           gmac_tx_cken                   ;
wire                                           gmac_rx_cken                   ;
wire                                           gmac_apb_cken                  ;
wire                                           gmac_axi_cken                  ;
wire                                           gpio_db_clk_src                ;
wire                                           gpio_apb_cken                  ;
wire                                           gpio_db_cken                   ;
wire                                           gpio_srst_req                  ;
wire                                           i2s_core_clk_src               ;
wire                                           i2s_core_cken                  ;
wire                                           i2s_apb_cken                   ;
wire                                           i2s_srst_req                   ;
wire                                           dft_clk_60m                    ;
wire                                           dft_clk_125m                   ;
wire                                           ddr_cken                       ;
wire                                           ddr_pcken                      ;
wire                                           ddr_acken                      ;
wire            [1:0]                          dma_breq_mix                   ;
wire            [1:0]                          dma_blast_mix                  ;
wire            [1:0]                          dma_ack                        ;
wire                                           gpio_penable                   ;
wire                                           gpio_pwrite                    ;
wire            [31:0]                         gpio_pwdata                    ;
wire            [6:0]                          gpio_paddr_mix                 ;
wire                                           gpio_psel                      ;
wire            [31:0]                         gpio_prdata                    ;
wire                                           dma_ack_rx                     ;
wire                                           dma_ack_tx                     ;
wire                                           dma_breq_rx                    ;
wire                                           dma_blast_rx                   ;
wire                                           dma_breq_tx                    ;
wire                                           dma_blast_tx                   ;
wire            [3:0]                          io_qspi_cs_out_n               ;
`ifdef  XS_QSPI2ROM
(*mark_debug = "true"*) wire                                           qspi_hsel                      ;
(*mark_debug = "true"*) wire                                           qspi_hready_from_bus           ;
(*mark_debug = "true"*) wire            [1:0]                          qspi_htrans                    ;
(*mark_debug = "true"*) wire                                           qspi_hwrite                    ;
(*mark_debug = "true"*) wire            [2:0]                          qspi_hsize                     ;
(*mark_debug = "true"*) wire            [2:0]                          qspi_hburst                    ;
(*mark_debug = "true"*) wire            [31:0]                         qspi_hwdata                    ;
(*mark_debug = "true"*) wire            [31:0]                         qspi_hrdata                    ;
(*mark_debug = "true"*) wire                                           qspi_hresp                     ;
(*mark_debug = "true"*) wire                                           qspi_hready                    ;
`endif
wire                                           qspi_psel                      ;
wire                                           qspi_penable                   ;
wire                                           qspi_pwrite                    ;
wire            [7:0]                          qspi_paddr_mix                 ;
wire            [31:0]                         qspi_pwdata                    ;
wire            [31:0]                         qspi_prdata                    ;
wire                                           qspi_pready                    ;
wire                                           qspi_pslverr                   ;
`ifdef  XS_GMAC
wire            [7:0]                          io_gmac_rxd_mix                ;
wire            [7:0]                          io_gmac_txd                    ;
wire                                           gmac_psel                      ;
wire            [13:0]                         gmac_paddr_mix                 ;
wire                                           gmac_pwrite                    ;
wire            [31:0]                         gmac_pwdata                    ;
wire                                           gmac_penable                   ;
wire            [31:0]                         gmac_prdata                    ;
wire                                           gmac_m_awready                 ;
wire                                           gmac_m_wready                  ;
wire            [3:0]                          gmac_m_bid                     ;
wire            [1:0]                          gmac_m_bresp                   ;
wire                                           gmac_m_bvalid                  ;
wire                                           gmac_m_arready                 ;
wire            [3:0]                          gmac_m_rid                     ;
wire            [1:0]                          gmac_m_rresp                   ;
wire            [63:0]                         gmac_m_rdata                   ;
wire                                           gmac_m_rvalid                  ;
wire                                           gmac_m_rlast                   ;
wire            [3:0]                          gmac_m_awlen                   ;
wire            [3:0]                          gmac_m_awid                    ;
wire            [1:0]                          gmac_m_awburst                 ;
wire                                           gmac_m_awvalid                 ;
wire            [2:0]                          gmac_m_awsize                  ;
wire            [1:0]                          gmac_m_awlock                  ;
wire            [3:0]                          gmac_m_awcache                 ;
wire            [2:0]                          gmac_m_awprot                  ;
wire            [3:0]                          gmac_m_wid                     ;
wire            [63:0]                         gmac_m_wdata                   ;
wire            [7:0]                          gmac_m_wstrb                   ;
wire                                           gmac_m_wlast                   ;
wire                                           gmac_m_wvalid                  ;
wire                                           gmac_m_bready                  ;
wire            [3:0]                          gmac_m_arlen                   ;
wire            [3:0]                          gmac_m_arid                    ;
wire            [1:0]                          gmac_m_arburst                 ;
wire                                           gmac_m_arvalid                 ;
wire            [2:0]                          gmac_m_arsize                  ;
wire            [1:0]                          gmac_m_arlock                  ;
wire            [3:0]                          gmac_m_arcache                 ;
wire            [2:0]                          gmac_m_arprot                  ;
wire                                           gmac_m_rready                  ;
`endif
`ifdef  XS_UART
wire                                           uart2_penable                  ;
wire                                           uart2_pwrite                   ;
wire            [31:0]                         uart2_pwdata                   ;
wire            [7:0]                          uart2_paddr_mix                ;
wire                                           uart2_psel                     ;
wire            [31:0]                         uart2_prdata                   ;
wire                                           uart1_penable                  ;
wire                                           uart1_pwrite                   ;
wire            [31:0]                         uart1_pwdata                   ;
wire            [7:0]                          uart1_paddr_mix                ;
wire                                           uart1_psel                     ;
wire            [31:0]                         uart1_prdata                   ;
(*mark_debug = "true"*) wire                                           uart0_penable                  ;
(*mark_debug = "true"*) wire                                           uart0_pwrite                   ;
(*mark_debug = "true"*) wire            [31:0]                         uart0_pwdata                   ;
(*mark_debug = "true"*) wire            [7:0]                          uart0_paddr_mix                ;
(*mark_debug = "true"*) wire                                           uart0_psel                     ;
(*mark_debug = "true"*) wire            [31:0]                         uart0_prdata                   ;
`endif
wire                                           i2c2_psel                      ;
wire                                           i2c2_penable                   ;
wire                                           i2c2_pwrite                    ;
wire            [7:0]                          i2c2_paddr_mix                 ;
wire            [31:0]                         i2c2_pwdata                    ;
wire            [31:0]                         i2c2_prdata                    ;
wire                                           i2c1_psel                      ;
wire                                           i2c1_penable                   ;
wire                                           i2c1_pwrite                    ;
wire            [7:0]                          i2c1_paddr_mix                 ;
wire            [31:0]                         i2c1_pwdata                    ;
wire            [31:0]                         i2c1_prdata                    ;
wire                                           i2c0_psel                      ;
wire                                           i2c0_penable                   ;
wire                                           i2c0_pwrite                    ;
wire            [7:0]                          i2c0_paddr_mix                 ;
wire            [31:0]                         i2c0_pwdata                    ;
wire            [31:0]                         i2c0_prdata                    ;


wire                                           dma_bridge_data_m_awready      ;
wire                                           dma_bridge_data_m_wready       ;
wire            [3:0]                          dma_bridge_data_m_bid          ;
wire            [1:0]                          dma_bridge_data_m_bresp        ;
wire                                           dma_bridge_data_m_bvalid       ;
wire                                           dma_bridge_data_m_arready      ;
wire            [3:0]                          dma_bridge_data_m_rid          ;
wire            [255:0]                        dma_bridge_data_m_rdata        ;
wire            [1:0]                          dma_bridge_data_m_rresp        ;
wire                                           dma_bridge_data_m_rlast        ;
wire                                           dma_bridge_data_m_rvalid       ;
wire            [3:0]                          dma_bridge_data_m_awid         ;
wire            [39:0]                         dma_bridge_data_m_awaddr       ;
wire            [7:0]                          dma_bridge_data_m_awlen        ;
wire            [2:0]                          dma_bridge_data_m_awsize       ;
wire            [1:0]                          dma_bridge_data_m_awburst      ;
wire                                           dma_bridge_data_m_awlock       ;
wire            [3:0]                          dma_bridge_data_m_awcache      ;
wire            [2:0]                          dma_bridge_data_m_awprot       ;
wire                                           dma_bridge_data_m_awvalid      ;
wire            [255:0]                        dma_bridge_data_m_wdata        ;
wire            [31:0]                         dma_bridge_data_m_wstrb        ;
wire                                           dma_bridge_data_m_wlast        ;
wire                                           dma_bridge_data_m_wvalid       ;
wire                                           dma_bridge_data_m_bready       ;
wire            [3:0]                          dma_bridge_data_m_arid         ;
wire            [39:0]                         dma_bridge_data_m_araddr       ;
wire            [7:0]                          dma_bridge_data_m_arlen        ;
wire            [2:0]                          dma_bridge_data_m_arsize       ;
wire            [1:0]                          dma_bridge_data_m_arburst      ;
wire                                           dma_bridge_data_m_arlock       ;
wire            [3:0]                          dma_bridge_data_m_arcache      ;
wire            [2:0]                          dma_bridge_data_m_arprot       ;
wire                                           dma_bridge_data_m_arvalid      ;
wire                                           dma_bridge_data_m_rready       ;
wire            [3:0]                          dma_bridge_data_m_awqos        ;
wire            [3:0]                          dma_bridge_data_m_arqos        ;
wire            [2:0]                          qos_hburst                     ;
wire            [3:0]                          qos_hprot                      ;
wire            [2:0]                          qos_hsize                      ;
wire            [1:0]                          qos_htrans                     ;
wire            [31:0]                         qos_hwdata                     ;
wire                                           qos_hwrite                     ;
wire            [31:0]                         qos_hrdata                     ;
wire                                           qos_hready                     ;
wire                                           qos_hresp                      ;
wire                                           qos_hsel                       ;
wire                                           qos_hready_from_bus            ;
wire            [7:0]                          peri_bridge_m_awid             ;
wire            [39:0]                         peri_bridge_m_awaddr           ;
wire            [3:0]                          peri_bridge_m_awlen            ;
wire            [2:0]                          peri_bridge_m_awsize           ;
wire            [1:0]                          peri_bridge_m_awburst          ;
wire            [1:0]                          peri_bridge_m_awlock           ;
wire            [3:0]                          peri_bridge_m_awcache          ;
wire            [2:0]                          peri_bridge_m_awprot           ;
wire                                           peri_bridge_m_awvalid          ;
wire            [7:0]                          peri_bridge_m_wid              ;
wire            [63:0]                         peri_bridge_m_wdata            ;
wire            [7:0]                          peri_bridge_m_wstrb            ;
wire                                           peri_bridge_m_wlast            ;
wire                                           peri_bridge_m_wvalid           ;
wire                                           peri_bridge_m_bready           ;
wire            [7:0]                          peri_bridge_m_arid             ;
wire            [39:0]                         peri_bridge_m_araddr           ;
wire            [3:0]                          peri_bridge_m_arlen            ;
wire            [2:0]                          peri_bridge_m_arsize           ;
wire            [1:0]                          peri_bridge_m_arburst          ;
wire            [1:0]                          peri_bridge_m_arlock           ;
wire            [3:0]                          peri_bridge_m_arcache          ;
wire            [2:0]                          peri_bridge_m_arprot           ;
wire                                           peri_bridge_m_arvalid          ;
wire                                           peri_bridge_m_rready           ;
wire            [3:0]                          peri_bridge_m_awqos            ;
wire            [3:0]                          peri_bridge_m_arqos            ;
wire                                           peri_bridge_m_awready          ;
wire                                           peri_bridge_m_wready           ;
wire            [7:0]                          peri_bridge_m_bid              ;
wire            [1:0]                          peri_bridge_m_bresp            ;
wire                                           peri_bridge_m_bvalid           ;
wire                                           peri_bridge_m_arready          ;
wire            [7:0]                          peri_bridge_m_rid              ;
wire            [63:0]                         peri_bridge_m_rdata            ;
wire            [1:0]                          peri_bridge_m_rresp            ;
wire                                           peri_bridge_m_rlast            ;
wire                                           peri_bridge_m_rvalid           ;
`ifdef  XS_GMAC
wire            [31:0]                         gmac_paddr                     ;
`endif
wire            [31:0]                         gpio_paddr                     ;
wire            [31:0]                         i2c2_paddr                     ;
wire            [31:0]                         i2c1_paddr                     ;
wire            [31:0]                         i2c0_paddr                     ;
`ifdef  XS_UART
wire            [31:0]                         uart2_paddr                    ;
wire            [31:0]                         uart1_paddr                    ;
wire            [31:0]                         uart0_paddr                    ;
`endif
wire            [31:0]                         qspi_paddr                     ;
wire            [31:0]                         syscfg_paddr                   ;
wire                                           gpu_hresp_mix                  ;
wire                                           dma_hresp_mix                  ;
wire            [2:0]                          dma_hburst                     ;
wire            [3:0]                          dma_hprot                      ;
`ifdef  XS_QSPI2ROM
wire            [3:0]                          qspi_hprot                     ;
`endif
wire                                           sd_hresp_mix                   ;
wire            [2:0]                          sd_hburst                      ;
wire            [3:0]                          sd_hprot                       ;

assign dma_ack_rx = dma_ack[1] ;
assign dma_ack_tx = dma_ack[0] ;
assign dma_breq_mix  = {dma_breq_rx ,dma_breq_tx } ;
assign dma_blast_mix = {dma_blast_rx,dma_blast_tx} ;

wire [58:0] cpu_int ;  // 11-26 add sd 2 int
wire [104:0] cpu_pll_config ;

assign cpu_int = {
      0   ,
      0   ,
      0   ,
	  sd_wakeup_int   ,
	  sd_int          ,
      pcie1_int       ,
//`ifdef  XS_XDMA
      0   ,
      0   ,
      0  ,
//`endif
      dp_de_int       ,
      dp_se_int       ,
      hdmiphy_int     ,
      hdmitx_int      ,
      wdt_int         ,
      gpu_int         ,
      qspi_int        ,
      i2s_int         ,
//`ifdef  XS_UART
      uart2_int       ,
      uart1_int       ,
      uart0_int       ,
//`endif
      i2c2_int        ,
      i2c1_int        ,
      i2c0_int        ,
      gpio_int[31:0]  ,
      dma_int         ,
//`ifdef  XS_GMAC      
      gmac_lpi_int    ,
      gmac_sbd_int    ,
      gmac_pmt_int
//`endif
      
};
assign cpu_int_mix = {5'b0, cpu_int}; //1-14:not match to 100NL SoC
assign i2c2_paddr_mix  = i2c2_paddr[7:0] ;
assign i2c1_paddr_mix  = i2c1_paddr[7:0] ;
assign i2c0_paddr_mix  = i2c0_paddr[7:0] ;
assign gpio_paddr_mix  = gpio_paddr[6:0] ;
`ifdef  XS_GMAC   
assign gmac_paddr_mix = gmac_paddr[13:0];
`endif
assign syscfg_paddr_mix = syscfg_paddr[15:0] ;
`ifdef  XS_QSPI2ROM
assign qspi_haddr_mix_pre = qspi_haddr - 32'h1000_0000 ;
assign qspi_haddr_mix = (qspi_haddr_mix_pre == 0) ? 32'h00000088 : qspi_haddr_mix_pre;
`endif
`ifdef  XS_GMAC   
assign gmac_m_awaddr_mix = cfg_gmac_addr_offset_en ? {{8'h0,gmac_m_awaddr[31:0]}+32'h8000_0000} : {8'h0,gmac_m_awaddr[31:0]};
assign gmac_m_araddr_mix = cfg_gmac_addr_offset_en ? {{8'h0,gmac_m_araddr[31:0]}+32'h8000_0000} : {8'h0,gmac_m_araddr[31:0]};
`endif
assign cpu2ddr_m2s_awaddr_mix = cpu2ddr_m2s_awaddr - 36'h8000_0000;
assign cpu2ddr_m2s_araddr_mix = cpu2ddr_m2s_araddr - 36'h8000_0000;
assign cpu2ddr_m2s_awid_mix = cpu2ddr_m2s_awid[13:0] ;
assign cpu2ddr_m2s_arid_mix = cpu2ddr_m2s_arid[13:0] ;
assign cpu2ddr_s2m_bid_mix  = cpu2ddr_s2m_bid[13:0];
assign cpu2ddr_s2m_rid_mix  = cpu2ddr_s2m_rid[13:0];
assign io_qspi_cs_out_n_mix = io_qspi_cs_out_n[0];
`ifdef  XS_GMAC   
assign io_gmac_txd_mix = io_gmac_txd[3:0];
assign io_gmac_rxd_mix = {4'b0,io_gmac_rxd[3:0]};
`endif
assign sys_cpu_rst   = ~sys_cpu_rst_n ;
assign cpu_jtag_trst = ~cpu_jtag_trst_n ;
assign soc_pll_lock_test = crg_dbug_out[4:0] ;

`ifdef  XS_QSPI2ROM
wire [7 : 0]                rom_axi_awlen    ;
wire [2 : 0]                rom_axi_awsize   ;
wire [1 : 0]                rom_axi_awburst  ;
wire [3 : 0]                rom_axi_awcache  ;
(*mark_debug = "true"*) wire [31 : 0]               rom_axi_awaddr   ;
wire [2 : 0]                rom_axi_awprot   ;
(*mark_debug = "true"*) wire                        rom_axi_awvalid  ;
(*mark_debug = "true"*) wire                        rom_axi_awready  ;
wire                        rom_axi_awlock   ;
(*mark_debug = "true"*) wire [31 : 0]               rom_axi_wdata    ;
wire [3 : 0]                rom_axi_wstrb    ;
(*mark_debug = "true"*) wire                        rom_axi_wlast    ;
(*mark_debug = "true"*) wire                        rom_axi_wvalid   ;
(*mark_debug = "true"*) wire                        rom_axi_wready   ;
(*mark_debug = "true"*) wire [1 : 0]                rom_axi_bresp    ;
(*mark_debug = "true"*) wire                        rom_axi_bvalid   ;
(*mark_debug = "true"*) wire                        rom_axi_bready   ;
wire [7 : 0]                rom_axi_arlen    ;
wire [2 : 0]                rom_axi_arsize   ;
wire [1 : 0]                rom_axi_arburst  ;
wire [2 : 0]                rom_axi_arprot   ;
wire [3 : 0]                rom_axi_arcache  ;
(*mark_debug = "true"*) wire                        rom_axi_arvalid  ;
(*mark_debug = "true"*) wire [31 : 0]               rom_axi_araddr   ;
wire                        rom_axi_arlock   ;
(*mark_debug = "true"*) wire                        rom_axi_arready  ;
(*mark_debug = "true"*) wire [31 : 0]               rom_axi_rdata    ;
wire [1 : 0]                rom_axi_rresp    ;
(*mark_debug = "true"*) wire                        rom_axi_rvalid   ;
(*mark_debug = "true"*) wire                        rom_axi_rlast    ;
(*mark_debug = "true"*) wire                        rom_axi_rready   ;
`endif

`ifdef  XS_QSPI2ROM
blk_mem_gen_0 u_rom (
  .rsta_busy      (rsta_busy),          // output wire rsta_busy
  .rstb_busy      (rstb_busy),          // output wire rstb_busy
  .s_aclk         (soc_clk_i),                // input wire s_aclk
  .s_aresetn      (axi_bclk_sync_rstn ),          // input wire s_aresetn
  .s_axi_awaddr   (rom_axi_awaddr     ),    // input wire [31 : 0] s_axi_awaddr
  .s_axi_awlen    (rom_axi_awlen      ),      // input wire [7 : 0] s_axi_awlen
  .s_axi_awvalid  (rom_axi_awvalid    ),  // input wire s_axi_awvalid
  .s_axi_awready  (rom_axi_awready    ),  // output wire s_axi_awready
  .s_axi_wdata    (rom_axi_wdata      ),      // input wire [31 : 0] s_axi_wdata
  .s_axi_wstrb    (rom_axi_wstrb      ),      // input wire [3 : 0] s_axi_wstrb
  .s_axi_wlast    (rom_axi_wlast      ),      // input wire s_axi_wlast
  .s_axi_wvalid   (rom_axi_wvalid     ),    // input wire s_axi_wvalid
  .s_axi_wready   (rom_axi_wready     ),    // output wire s_axi_wready
  .s_axi_bresp    (rom_axi_bresp      ),      // output wire [1 : 0] s_axi_bresp
  .s_axi_bvalid   (rom_axi_bvalid     ),    // output wire s_axi_bvalid
  .s_axi_bready   (rom_axi_bready     ),    // input wire s_axi_bready
  .s_axi_araddr   (rom_axi_araddr     ),    // input wire [31 : 0] s_axi_araddr
  .s_axi_arlen    (rom_axi_arlen      ),      // input wire [7 : 0] s_axi_arlen
  .s_axi_arvalid  (rom_axi_arvalid    ),  // input wire s_axi_arvalid
  .s_axi_arready  (rom_axi_arready    ),  // output wire s_axi_arready
  .s_axi_rdata    (rom_axi_rdata      ),      // output wire [31 : 0] s_axi_rdata
  .s_axi_rresp    (rom_axi_rresp      ),      // output wire [1 : 0] s_axi_rresp
  .s_axi_rlast    (rom_axi_rlast      ),      // output wire s_axi_rlast
  .s_axi_rvalid   (rom_axi_rvalid     ),    // output wire s_axi_rvalid
  .s_axi_rready   (rom_axi_rready     )    // input wire s_axi_rready
);
`endif

`ifndef  XS_XDMA
assign cfg_pcie0_s2m_arready = 1;
assign cfg_pcie0_s2m_awready = 1;
assign cfg_pcie0_s2m_wready = 1;
assign cfg_pcie0_s2m_bvalid = 1;
assign cfg_pcie0_s2m_bresp = 0;
assign cfg_pcie0_s2m_rdata = 0;
assign cfg_pcie0_s2m_rvalid = 1;

assign cfg_pcie1_s2m_arready = 1;
assign cfg_pcie1_s2m_awready = 1;
assign cfg_pcie1_s2m_wready = 1;
assign cfg_pcie1_s2m_bvalid = 1;
assign cfg_pcie1_s2m_bresp = 0;
assign cfg_pcie1_s2m_rdata = 0;
assign cfg_pcie1_s2m_rvalid = 1;
`endif

assign pcie1_int = 0;
assign gpu_m_arvalid = 0;
assign gpu_m_awvalid = 0;
assign gpu_m_wvalid = 0;
assign gpu_hready = 1;
assign gpu_hrdata = 0;
assign gpu_int = 0;
assign i2c0_int = 0;
assign i2c0_prdata = 0;
assign i2c1_int = 0;
assign i2c1_prdata = 0;
assign i2c2_int = 0;
assign i2c2_prdata = 0;

  wire [511:0] PCIE_S00_AXIS_0_tdata;
  wire PCIE_S00_AXIS_0_tlast;
  wire PCIE_S00_AXIS_0_tready;
  wire PCIE_S00_AXIS_0_tvalid;

  wire [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0] gateway_out_data;
  wire gateway_out_enable;
  wire inter_dev_clk;
  wire inter_soc_clk;
  wire inter_tmclk;
  xdma_ep xdma_ep_i(
    .cpu_clk(sys_clk_i),
    .cpu_rstn(sys_rstn),
    .S00_AXIS_0_tdata(PCIE_S00_AXIS_0_tdata),
    .S00_AXIS_0_tkeep(64'hffffffff_ffffffff),
    .S00_AXIS_0_tlast(PCIE_S00_AXIS_0_tlast),
    .S00_AXIS_0_tready(PCIE_S00_AXIS_0_tready),
    .S00_AXIS_0_tvalid(PCIE_S00_AXIS_0_tvalid),

    .pci_exp_rxn(pci_ep_rxn),
    .pci_exp_rxp(pci_ep_rxp),
    .pci_exp_txn(pci_ep_txn),
    .pci_exp_txp(pci_ep_txp),
    .pcie_ep_gt_ref_clk_n(pcie_ep_gt_ref_clk_n),
    .pcie_ep_gt_ref_clk_p(pcie_ep_gt_ref_clk_p),
    .pcie_ep_lnk_up(pcie_ep_lnk_up),
    .pcie_ep_perstn(pcie_ep_perstn)
  );

  axis_data_packge  #(
    .DATA_WIDTH(`CONFIG_DIFFTEST_BATCH_IO_WITDH),
    .AXIS_DATA_WIDTH(512)
  ) axis_data_packge_i (
    .m_axis_c2h_aclk   (sys_clk_i),
    .m_axis_c2h_aresetn(sys_rstn),
    .m_axis_c2h_tdata  (PCIE_S00_AXIS_0_tdata),
    .m_axis_c2h_tkeep  (),
    .m_axis_c2h_tlast  (PCIE_S00_AXIS_0_tlast),
    .m_axis_c2h_tready (PCIE_S00_AXIS_0_tready),
    .m_axis_c2h_tvalid (PCIE_S00_AXIS_0_tvalid),

    .data_valid (difftest_enable),
    .data_next  (data_need_next),
    .data       (difftest_data)
  );
  interrupt_gen interrupt_gen_diff(
        .soc_clk_i  (sys_clk_i),
        .soc_clk_o  (inter_soc_clk),
        .dev_clk_i  (dev_clk_i),
        .dev_clk_o  (inter_dev_clk),
        .tmclk_i    (tmclk),
        .tmclk_o    (inter_tmclk),
        .data_next  (data_need_next),
        .rstn       (data_rst_ok)
  );

xilnx_crg xilnx_crg(
   .sys_clk                         (sys_clk_i                     ),
   .dev_clk                         (dev_clk_i                     ),
   .tmclk                           (tmclk                         ),
   .cqetmclk                        (cqetmclk                      ),
   .sys_rstn                        (sys_rstn                      ),
   .axi_bus_clk                     (axi_bus_clk                   ),
   .axi_bclk_sync_rstn              (axi_bclk_sync_rstn            ),
   .ddr_bus_clk                     (ddr_bus_clk                   ),
   .ddr_bclk_sync_rstn              (ddr_bclk_sync_rstn            ),
 `ifdef  XS_UART  
   .uart_pclk                       (uart_pclk                     ),
   .uart_pclk_sync_rstn             (uart_pclk_sync_rstn           ),
   .uart_sclk                       (uart_sclk                     ),
   .uart_sclk_sync_rstn             (uart_sclk_sync_rstn           ),
 `endif  
   .qspi_sclk                       (qspi_sclk                     ),
   .qspi_pclk                       (qspi_pclk                     ),
   .qspi_pclk_sync_rstn             (qspi_pclk_sync_rstn           ),
   .qspi_hclk                       (qspi_hclk                     ),
   .qspi_hclk_sync_rstn             (qspi_hclk_sync_rstn           ),
   .qspi_ref_clk                    (qspi_ref_clk                  ),
   .qspi_rclk_sync_rstn             (qspi_rclk_sync_rstn           ),
   .sd_axi_clk                      (sd_axi_clk                    ),
   .sd_aclk_sync_rstn               (sd_aclk_sync_rstn             ),
   .sd_ahb_clk                      (sd_ahb_clk                    ),
   .sd_hclk_sync_rstn               (sd_hclk_sync_rstn             ),
   .sd_bclk                         (sd_bclk                       ),
   .sd_bclk_sync_rstn               (sd_bclk_sync_rstn             ),
   .sd_tmclk                        (sd_tmclk                      ),
   .sd_tclk_sync_rstn               (sd_tclk_sync_rstn             ),
   .sd_cqetmclk                     (sd_cqetmclk                   ),
   .sd_cqetclk_sync_rstn            (sd_cqetclk_sync_rstn          )
);

assign dma_m_arvalid = 0;
assign dma_m_awvalid = 0;
assign dma_m_wvalid = 0;
assign dma_hready = 1;
assign dma_hrdata = 0;
assign dma_int = 0;
mode_ctrl U_MODE_CTRL(
    .chip_mode_i                    (chip_mode_i                   ),
    .normal_mode                    (normal_mode                   ),
    .phy_bist_mode                  (phy_bist_mode                 ),
    .mbist_mode                     (mbist_mode                    ),
    .scan_mode                      (                              )
);
jtag_ddr_subsys_wrapper U_JTAG_DDR_SUBSYS(
    .DDR4_act_n             (DDR_ACT_N),
    .DDR4_adr               (DDR_A),
    .DDR4_ba                (DDR_BA),
    .DDR4_bg                (DDR_BG),
    .DDR4_ck_c              (DDR_CK_C),
    .DDR4_ck_t              (DDR_CK_T),
    .DDR4_cke               (DDR_CKE),
    .DDR4_cs_n              (DDR_CS_N),
    .DDR4_dm_n              (DDR_DM_N),
    .DDR4_dq                (DDR_DQ),
    .DDR4_dqs_c             (DDR_DQS_C),
    .DDR4_dqs_t             (DDR_DQS_T),
    .DDR4_odt               (DDR_ODT),
    .DDR4_reset_n           (DDR_RESET_N),
    .OSC_SYS_CLK_clk_n      (ddr_clk_n),
    .OSC_SYS_CLK_clk_p      (ddr_clk_p),
    .SOC_CLK                (inter_soc_clk),
    .MAC_CLK                (mac_clk),
    .SOC_RESETN             (),
    .SOC_M_AXI_awid         (cpu2ddr_m2s_awid_mix          ),  
    .SOC_M_AXI_awaddr       (cpu2ddr_m2s_awaddr_mix        ),   
    .SOC_M_AXI_awlen        (cpu2ddr_m2s_awlen             ),    
    .SOC_M_AXI_awsize       (cpu2ddr_m2s_awsize            ),       
    .SOC_M_AXI_awburst      (cpu2ddr_m2s_awburst           ),       
    .SOC_M_AXI_awlock       (cpu2ddr_m2s_awlock            ),       
    .SOC_M_AXI_awcache      (cpu2ddr_m2s_awcache           ),       
    .SOC_M_AXI_awprot       (cpu2ddr_m2s_awprot            ),       
    .SOC_M_AXI_awqos        (cpu2ddr_m2s_awqos             ),       
    .SOC_M_AXI_awvalid      (cpu2ddr_m2s_awvalid           ),       
    .SOC_M_AXI_awready      (cpu2ddr_s2m_awready           ),       
    .SOC_M_AXI_wdata        (cpu2ddr_m2s_wdata             ),       
    .SOC_M_AXI_wstrb        (cpu2ddr_m2s_wstrb             ),       
    .SOC_M_AXI_wlast        (cpu2ddr_m2s_wlast             ),       
    .SOC_M_AXI_wvalid       (cpu2ddr_m2s_wvalid            ),       
    .SOC_M_AXI_wready       (cpu2ddr_s2m_wready            ),       
    .SOC_M_AXI_bid          (cpu2ddr_s2m_bid               ),       
    .SOC_M_AXI_bresp        (cpu2ddr_s2m_bresp             ),       
    .SOC_M_AXI_bvalid       (cpu2ddr_s2m_bvalid            ),       
    .SOC_M_AXI_bready       (cpu2ddr_m2s_bready            ),       
    .SOC_M_AXI_arid         (cpu2ddr_m2s_arid_mix          ),       
    .SOC_M_AXI_araddr       (cpu2ddr_m2s_araddr_mix        ),       
    .SOC_M_AXI_arlen        (cpu2ddr_m2s_arlen             ),       
    .SOC_M_AXI_arsize       (cpu2ddr_m2s_arsize            ),       
    .SOC_M_AXI_arburst      (cpu2ddr_m2s_arburst           ),       
    .SOC_M_AXI_arlock       (cpu2ddr_m2s_arlock            ),       
    .SOC_M_AXI_arcache      (cpu2ddr_m2s_arcache           ),       
    .SOC_M_AXI_arprot       (cpu2ddr_m2s_arprot            ),       
    .SOC_M_AXI_arqos        (cpu2ddr_m2s_arqos             ),       
    .SOC_M_AXI_arvalid      (cpu2ddr_m2s_arvalid           ),       
    .SOC_M_AXI_arready      (cpu2ddr_s2m_arready           ),       
    .SOC_M_AXI_rid          (cpu2ddr_s2m_rid               ),       
    .SOC_M_AXI_rdata        (cpu2ddr_s2m_rdata             ),       
    .SOC_M_AXI_rresp        (cpu2ddr_s2m_rresp             ),       
    .SOC_M_AXI_rlast        (cpu2ddr_s2m_rlast             ),       
    .SOC_M_AXI_rvalid       (cpu2ddr_s2m_rvalid            ),       
    .SOC_M_AXI_rready       (cpu2ddr_m2s_rready            ),        
    .ddr_rstn               (rstn_sw4),
    .soc_rstn               (rstn_sw4),
    .calib_complete         (init_calib_complete)
);

XSTop_wrapper U_CPU_TOP(
    .gateway_out_enable             (gateway_out_enable),
    .gateway_out_data               (gateway_out_data),

    .sys_clk_i                      (inter_soc_clk    ),
    .sys_rstn_i                     (cpu_rstn     ),
    .tmclk                          (inter_tmclk),
    .osc_clock                      (inter_soc_clk ),
    .outer_clock                    (inter_soc_clk ),
    .global_reset                   (cpu_rstn                  ),
    .pll_bypass_sel                 (4'b0 ),
    .pll0_lock                      (),
    .pll0_clk_div_1024              (),
    .pll0_test_calout               (),
    .soc_to_cpu                     (16'b0                         ),
    .cpu_to_soc                     (                              ),
    .io_systemjtag_jtag_TCK         (io_systemjtag_jtag_TCK),
    .io_systemjtag_jtag_TMS         (io_systemjtag_jtag_TMS),
    .io_systemjtag_jtag_TDI         (io_systemjtag_jtag_TDI),
    .io_systemjtag_jtag_TDO_data    (io_systemjtag_jtag_TDO_data),
    .io_systemjtag_jtag_TDO_driven  (io_systemjtag_jtag_TDO_driven),
//    .io_systemjtag_reset            (io_systemjtag_reset),
    .io_systemjtag_reset            (~sys_rstn),
    .io_sram_config                 (5'b0  ),
    
    .dma_core_awready               (data_cpu_bridge_s2m_awready),
    .dma_core_awvalid               (data_cpu_bridge_m2s_awvalid),
    .dma_core_awid                  (data_cpu_bridge_m2s_awid),
    .dma_core_awaddr                (data_cpu_bridge_m2s_awaddr),
    .dma_core_awlen                 (data_cpu_bridge_m2s_awlen),
    .dma_core_awsize                (data_cpu_bridge_m2s_awsize),
    .dma_core_awburst               (data_cpu_bridge_m2s_awburst),
    .dma_core_awlock                (data_cpu_bridge_m2s_awlock),
    .dma_core_awcache               (data_cpu_bridge_m2s_awcache),
    .dma_core_awprot                (data_cpu_bridge_m2s_awprot),
    .dma_core_awqos                 (data_cpu_bridge_m2s_awqos),
    .dma_core_wready                (data_cpu_bridge_s2m_wready),
    .dma_core_wvalid                (data_cpu_bridge_m2s_wvalid),
    .dma_core_wdata                 (data_cpu_bridge_m2s_wdata),
    .dma_core_wstrb                 (data_cpu_bridge_m2s_wstrb),
    .dma_core_wlast                 (data_cpu_bridge_m2s_wlast),
    .dma_core_bready                (data_cpu_bridge_m2s_bready),
    .dma_core_bvalid                (data_cpu_bridge_s2m_bvalid),
    .dma_core_bid                   (data_cpu_bridge_s2m_bid),
    .dma_core_bresp                 (data_cpu_bridge_s2m_bresp),
    .dma_core_arready               (data_cpu_bridge_s2m_arready),
    .dma_core_arvalid               (data_cpu_bridge_m2s_arvalid),
    .dma_core_arid                  (data_cpu_bridge_m2s_arid),
    .dma_core_araddr                (data_cpu_bridge_m2s_araddr),
    .dma_core_arlen                 (data_cpu_bridge_m2s_arlen),
    .dma_core_arsize                (data_cpu_bridge_m2s_arsize),
    .dma_core_arburst               (data_cpu_bridge_m2s_arburst),
    .dma_core_arlock                (data_cpu_bridge_m2s_arlock),
    .dma_core_arcache               (data_cpu_bridge_m2s_arcache),
    .dma_core_arprot                (data_cpu_bridge_m2s_arprot),
    .dma_core_arqos                 (data_cpu_bridge_m2s_arqos),
    .dma_core_rready                (data_cpu_bridge_m2s_rready),
    .dma_core_rvalid                (data_cpu_bridge_s2m_rvalid),
    .dma_core_rid                   (data_cpu_bridge_s2m_rid),
    .dma_core_rdata                 (data_cpu_bridge_s2m_rdata),
    .dma_core_rresp                 (data_cpu_bridge_s2m_rresp),
    .dma_core_rlast                 (data_cpu_bridge_s2m_rlast),
     
    .peri_awready                   (cpu2cfg_s2m_awready),
    .peri_awvalid                   (cpu2cfg_m2s_awvalid),
    .peri_awid                      (cpu2cfg_m2s_awid),
    .peri_awaddr                    (cpu2cfg_m2s_awaddr),
    .peri_awlen                     (cpu2cfg_m2s_awlen),
    .peri_awsize                    (cpu2cfg_m2s_awsize),
    .peri_awburst                   (cpu2cfg_m2s_awburst),
    .peri_awlock                    (cpu2cfg_m2s_awlock),
    .peri_awcache                   (cpu2cfg_m2s_awcache),
    .peri_awprot                    (cpu2cfg_m2s_awprot),
    .peri_awqos                     (cpu2cfg_m2s_awqos),
    .peri_wready                    (cpu2cfg_s2m_wready),
    .peri_wvalid                    (cpu2cfg_m2s_wvalid),
    .peri_wdata                     (cpu2cfg_m2s_wdata),
    .peri_wstrb                     (cpu2cfg_m2s_wstrb),
    .peri_wlast                     (cpu2cfg_m2s_wlast),
    .peri_bready                    (cpu2cfg_m2s_bready),
    .peri_bvalid                    (cpu2cfg_s2m_bvalid),
    .peri_bid                       (cpu2cfg_s2m_bid),
    .peri_bresp                     (cpu2cfg_s2m_bresp),
    .peri_arready                   (cpu2cfg_s2m_arready),
    .peri_arvalid                   (cpu2cfg_m2s_arvalid),
    .peri_arid                      (cpu2cfg_m2s_arid),
    .peri_araddr                    (cpu2cfg_m2s_araddr),
    .peri_arlen                     (cpu2cfg_m2s_arlen),
    .peri_arsize                    (cpu2cfg_m2s_arsize),
    .peri_arburst                   (cpu2cfg_m2s_arburst),
    .peri_arlock                    (cpu2cfg_m2s_arlock),
    .peri_arcache                   (cpu2cfg_m2s_arcache),
    .peri_arprot                    (cpu2cfg_m2s_arprot),
    .peri_arqos                     (cpu2cfg_m2s_arqos),
    .peri_rready                    (cpu2cfg_m2s_rready),
    .peri_rvalid                    (cpu2cfg_s2m_rvalid),
    .peri_rid                       (cpu2cfg_s2m_rid),
    .peri_rdata                     (cpu2cfg_s2m_rdata),
    .peri_rresp                     (cpu2cfg_s2m_rresp),
    .peri_rlast                     (cpu2cfg_s2m_rlast),     

    .mem_core_awready               (cpu2ddr_s2m_awready),
    .mem_core_awvalid               (cpu2ddr_m2s_awvalid),
    .mem_core_awid                  (cpu2ddr_m2s_awid),
    .mem_core_awaddr                (cpu2ddr_m2s_awaddr),
    .mem_core_awlen                 (cpu2ddr_m2s_awlen),
    .mem_core_awsize                (cpu2ddr_m2s_awsize),
    .mem_core_awburst               (cpu2ddr_m2s_awburst),
    .mem_core_awlock                (cpu2ddr_m2s_awlock),
    .mem_core_awcache               (cpu2ddr_m2s_awcache),
    .mem_core_awprot                (cpu2ddr_m2s_awprot),
    .mem_core_awqos                 (cpu2ddr_m2s_awqos),
    .mem_core_wready                (cpu2ddr_s2m_wready),
    .mem_core_wvalid                (cpu2ddr_m2s_wvalid),
    .mem_core_wdata                 (cpu2ddr_m2s_wdata),
    .mem_core_wstrb                 (cpu2ddr_m2s_wstrb),
    .mem_core_wlast                 (cpu2ddr_m2s_wlast),
    .mem_core_bready                (cpu2ddr_m2s_bready),
    .mem_core_bvalid                (cpu2ddr_s2m_bvalid),
    .mem_core_bid                   (cpu2ddr_s2m_bid),
    .mem_core_bresp                 (cpu2ddr_s2m_bresp),
    .mem_core_arready               (cpu2ddr_s2m_arready),
    .mem_core_arvalid               (cpu2ddr_m2s_arvalid),
    .mem_core_arid                  (cpu2ddr_m2s_arid),
    .mem_core_araddr                (cpu2ddr_m2s_araddr),
    .mem_core_arlen                 (cpu2ddr_m2s_arlen),
    .mem_core_arsize                (cpu2ddr_m2s_arsize),
    .mem_core_arburst               (cpu2ddr_m2s_arburst),
    .mem_core_arlock                (cpu2ddr_m2s_arlock),
    .mem_core_arcache               (cpu2ddr_m2s_arcache),
    .mem_core_arprot                (cpu2ddr_m2s_arprot),
    .mem_core_arqos                 (cpu2ddr_m2s_arqos),
    .mem_core_rready                (cpu2ddr_m2s_rready),
    .mem_core_rvalid                (cpu2ddr_s2m_rvalid),
    .mem_core_rid                   (cpu2ddr_s2m_rid),
    .mem_core_rdata                 (cpu2ddr_s2m_rdata),
    .mem_core_rresp                 (cpu2ddr_s2m_rresp),
    .mem_core_rlast                 (cpu2ddr_s2m_rlast),
    
    .io_extIntrs                    (cpu_int_mix)
    
);

assign hpm_data_ulvt = 0;
assign hpm_data_lvt = 0;
assign hpm_data_svt = 0;
syscfg U_SYS_CFG(
    .clk                            (axi_bus_clk            ),
    .rst_n                          (axi_bclk_sync_rstn            ),
    .apb_addr                       (syscfg_paddr_mix              ),
    .apb_selx                       (syscfg_psel                   ),
    .apb_enable                     (syscfg_penable                ),
    .apb_write                      (syscfg_pwrite                 ),
    .apb_wdata                      (syscfg_pwdata                 ),
    .syscfg_version                 (32'h20230609                  ),
    .apb_ready                      (syscfg_pready                 ),
    .apb_rdata                      (syscfg_prdata                 ),
    .apb_slverr                     (syscfg_pslverr                )
);
assign hpm_dig_result = 0;

AXI_bridge CFG_AXI_bridge_i
       (.SYS_INTER_CLK          (inter_soc_clk),          
        .ACLK                   (sys_clk_i),
        .ARESETN                (axi_bclk_sync_rstn),    
    
        .S00_AXI_araddr         (cpu2cfg_m2s_araddr),
        .S00_AXI_arburst        (cpu2cfg_m2s_arburst),
        .S00_AXI_arcache        (cpu2cfg_m2s_arcache),
        .S00_AXI_arid           (cpu2cfg_m2s_arid),
        .S00_AXI_arlen          (cpu2cfg_m2s_arlen),
        .S00_AXI_arlock         (cpu2cfg_m2s_arlock),
        .S00_AXI_arprot         (cpu2cfg_m2s_arprot),
        .S00_AXI_arqos          (cpu2cfg_m2s_arqos),
        .S00_AXI_arready        (cpu2cfg_s2m_arready),
        .S00_AXI_arsize         (cpu2cfg_m2s_arsize),
        .S00_AXI_arvalid        (cpu2cfg_m2s_arvalid),
        .S00_AXI_awaddr         (cpu2cfg_m2s_awaddr),
        .S00_AXI_awburst        (cpu2cfg_m2s_awburst),
        .S00_AXI_awcache        (cpu2cfg_m2s_awcache),
        .S00_AXI_awid           (cpu2cfg_m2s_awid),
        .S00_AXI_awlen          (cpu2cfg_m2s_awlen),
        .S00_AXI_awlock         (cpu2cfg_m2s_awlock),
        .S00_AXI_awprot         (cpu2cfg_m2s_awprot),
        .S00_AXI_awqos          (cpu2cfg_m2s_awqos),
        .S00_AXI_awready        (cpu2cfg_s2m_awready),
        .S00_AXI_awsize         (cpu2cfg_m2s_awsize),
        .S00_AXI_awvalid        (cpu2cfg_m2s_awvalid),
        .S00_AXI_bid            (cpu2cfg_s2m_bid),
        .S00_AXI_bready         (cpu2cfg_m2s_bready),
        .S00_AXI_bresp          (cpu2cfg_s2m_bresp),
        .S00_AXI_bvalid         (cpu2cfg_s2m_bvalid),
        .S00_AXI_rdata          (cpu2cfg_s2m_rdata),
        .S00_AXI_rid            (cpu2cfg_s2m_rid),
        .S00_AXI_rlast          (cpu2cfg_s2m_rlast),
        .S00_AXI_rready         (cpu2cfg_m2s_rready),
        .S00_AXI_rresp          (cpu2cfg_s2m_rresp),
        .S00_AXI_rvalid         (cpu2cfg_s2m_rvalid),
        .S00_AXI_wdata          (cpu2cfg_m2s_wdata),
        .S00_AXI_wlast          (cpu2cfg_m2s_wlast),
        .S00_AXI_wready         (cpu2cfg_s2m_wready),
        .S00_AXI_wstrb          (cpu2cfg_m2s_wstrb),
        .S00_AXI_wvalid         (cpu2cfg_m2s_wvalid),
        
        .SYS_CFG_APB_paddr      (syscfg_paddr_mix),
        .SYS_CFG_APB_penable    (syscfg_penable),
        .SYS_CFG_APB_prdata     (syscfg_prdata),
        .SYS_CFG_APB_pready     (syscfg_pready),
        .SYS_CFG_APB_psel       (syscfg_psel),
        .SYS_CFG_APB_pslverr    (syscfg_pslverr),
        .SYS_CFG_APB_pwdata     (syscfg_pwdata),
        .SYS_CFG_APB_pwrite     (syscfg_pwrite),
           
        .UART_0_baudoutn        (),
        .UART_0_ctsn            (1'b1),
        .UART_0_dcdn            (1'b0),
        .UART_0_ddis            (),
        .UART_0_dsrn            (1'b0),
        .UART_0_dtrn            (),
        .UART_0_out1n           (),
        .UART_0_out2n           (),
        .UART_0_ri              (1'b1),
        .UART_0_rtsn            (),
        .UART_0_rxd             (uart0_sin),
        .UART_0_rxrdyn          (),
        .UART_0_txd             (uart0_sout),
        .UART_0_txrdyn          (),
        .uart0_intc             (uart0_int),
        
        .rom_axi_araddr         (rom_axi_araddr),
        .rom_axi_arburst        (),
        .rom_axi_arcache        (),
        .rom_axi_arlen          (rom_axi_arlen),
        .rom_axi_arlock         (),
        .rom_axi_arprot         (),
        .rom_axi_arqos          (),
        .rom_axi_arready        (rom_axi_arready),
        .rom_axi_arregion       (),
        .rom_axi_arsize         (),
        .rom_axi_arvalid        (rom_axi_arvalid),
        .rom_axi_awaddr         (rom_axi_awaddr),
        .rom_axi_awburst        (),
        .rom_axi_awcache        (),
        .rom_axi_awlen          (rom_axi_awlen),
        .rom_axi_awlock         (),
        .rom_axi_awprot         (),
        .rom_axi_awqos          (),
        .rom_axi_awready        (rom_axi_awready),
        .rom_axi_awregion       (),
        .rom_axi_awsize         (),
        .rom_axi_awvalid        (rom_axi_awvalid),
        .rom_axi_bready         (rom_axi_bready),
        .rom_axi_bresp          (rom_axi_bresp),
        .rom_axi_bvalid         (rom_axi_bvalid),
        .rom_axi_rdata          (rom_axi_rdata),
        .rom_axi_rlast          (rom_axi_rlast),
        .rom_axi_rready         (rom_axi_rready),
        .rom_axi_rresp          (rom_axi_rresp),
        .rom_axi_rvalid         (rom_axi_rvalid),
        .rom_axi_wdata          (rom_axi_wdata),
        .rom_axi_wlast          (rom_axi_wlast),
        .rom_axi_wready         (rom_axi_wready),
        .rom_axi_wstrb          (rom_axi_wstrb),
        .rom_axi_wvalid         (rom_axi_wvalid)
        );

  data_bridge data_bridge_i
       (.ACLK                   (axi_bus_clk),
        .ARESETN                (axi_bclk_sync_rstn),
        .M00_AXI_araddr         (data_cpu_bridge_m2s_araddr),
        .M00_AXI_arburst        (data_cpu_bridge_m2s_arburst),
        .M00_AXI_arcache        (data_cpu_bridge_m2s_arcache),
        .M00_AXI_arid           (data_cpu_bridge_m2s_arid),
        .M00_AXI_arlen          (data_cpu_bridge_m2s_arlen),
        .M00_AXI_arlock         (data_cpu_bridge_m2s_arlock),
        .M00_AXI_arprot         (data_cpu_bridge_m2s_arprot),
        .M00_AXI_arqos          (data_cpu_bridge_m2s_arqos),
        .M00_AXI_arready        (data_cpu_bridge_s2m_arready),
        .M00_AXI_arregion       (),
        .M00_AXI_arsize         (data_cpu_bridge_m2s_arsize),
        .M00_AXI_arvalid        (data_cpu_bridge_m2s_arvalid),
        .M00_AXI_awaddr         (data_cpu_bridge_m2s_awaddr),
        .M00_AXI_awburst        (data_cpu_bridge_m2s_awburst),
        .M00_AXI_awcache        (data_cpu_bridge_m2s_awcache),
        .M00_AXI_awid           (data_cpu_bridge_m2s_awid),
        .M00_AXI_awlen          (data_cpu_bridge_m2s_awlen),
        .M00_AXI_awlock         (data_cpu_bridge_m2s_awlock),
        .M00_AXI_awprot         (data_cpu_bridge_m2s_awprot),
        .M00_AXI_awqos          (data_cpu_bridge_m2s_awqos),
        .M00_AXI_awready        (data_cpu_bridge_s2m_awready),
        .M00_AXI_awregion       (),
        .M00_AXI_awsize         (data_cpu_bridge_m2s_awsize),
        .M00_AXI_awvalid        (data_cpu_bridge_m2s_awvalid),
        .M00_AXI_bid            (data_cpu_bridge_s2m_bid),
        .M00_AXI_bready         (data_cpu_bridge_m2s_bready),
        .M00_AXI_bresp          (data_cpu_bridge_s2m_bresp),
        .M00_AXI_bvalid         (data_cpu_bridge_s2m_bvalid),
        .M00_AXI_rdata          (data_cpu_bridge_s2m_rdata),
        .M00_AXI_rid            (data_cpu_bridge_s2m_rid),
        .M00_AXI_rlast          (data_cpu_bridge_s2m_rlast),
        .M00_AXI_rready         (data_cpu_bridge_m2s_rready),
        .M00_AXI_rresp          (data_cpu_bridge_s2m_rresp),
        .M00_AXI_rvalid         (data_cpu_bridge_s2m_rvalid),
        .M00_AXI_wdata          (data_cpu_bridge_m2s_wdata),
        .M00_AXI_wlast          (data_cpu_bridge_m2s_wlast),
        .M00_AXI_wready         (data_cpu_bridge_s2m_wready),
        .M00_AXI_wstrb          (data_cpu_bridge_m2s_wstrb),
        .M00_AXI_wvalid         (data_cpu_bridge_m2s_wvalid),
        
      //   .S00_AXI_araddr         (pcie_bridge_m_araddr),
      //   .S00_AXI_arburst        (pcie_bridge_m_arburst),
      //   .S00_AXI_arcache        (pcie_bridge_m_arcache),
      //   .S00_AXI_arid           (pcie_bridge_m_arid),
      //   .S00_AXI_arlen          (pcie_bridge_m_arlen),
      //   .S00_AXI_arlock         (pcie_bridge_m_arlock),
      //   .S00_AXI_arprot         (pcie_bridge_m_arprot),
      //   .S00_AXI_arqos          (pcie_bridge_m_arqos),
      //   .S00_AXI_arready        (pcie_bridge_m_arready),
      //   .S00_AXI_arregion       (pcie_bridge_m_arregion),
      //   .S00_AXI_arsize         (pcie_bridge_m_arsize),
      //   .S00_AXI_arvalid        (pcie_bridge_m_arvalid),
      //   .S00_AXI_awaddr         (pcie_bridge_m_awaddr),
      //   .S00_AXI_awburst        (pcie_bridge_m_awburst),
      //   .S00_AXI_awcache        (pcie_bridge_m_awcache),
      //   .S00_AXI_awid           (pcie_bridge_m_awid),
      //   .S00_AXI_awlen          (pcie_bridge_m_awlen),
      //   .S00_AXI_awlock         (pcie_bridge_m_awlock),
      //   .S00_AXI_awprot         (pcie_bridge_m_awprot),
      //   .S00_AXI_awqos          (pcie_bridge_m_awqos),
      //   .S00_AXI_awready        (pcie_bridge_m_awready),
      //   .S00_AXI_awregion       (pcie_bridge_m_awregion),
      //   .S00_AXI_awsize         (pcie_bridge_m_awsize),
      //   .S00_AXI_awvalid        (pcie_bridge_m_awvalid),
      //   .S00_AXI_bid            (pcie_bridge_m_bid_mix),
      //   .S00_AXI_bready         (pcie_bridge_m_bready),
      //   .S00_AXI_bresp          (pcie_bridge_m_bresp),
      //   .S00_AXI_bvalid         (pcie_bridge_m_bvalid),
      //   .S00_AXI_rdata          (pcie_bridge_m_rdata),
      //   .S00_AXI_rid            (pcie_bridge_m_rid_mix),
      //   .S00_AXI_rlast          (pcie_bridge_m_rlast),
      //   .S00_AXI_rready         (pcie_bridge_m_rready),
      //   .S00_AXI_rresp          (pcie_bridge_m_rresp),
      //   .S00_AXI_rvalid         (pcie_bridge_m_rvalid),
      //   .S00_AXI_wdata          (pcie_bridge_m_wdata),
      //   .S00_AXI_wlast          (pcie_bridge_m_wlast),
      //   .S00_AXI_wready         (pcie_bridge_m_wready),
      //   .S00_AXI_wstrb          (pcie_bridge_m_wstrb),
      //   .S00_AXI_wvalid         (pcie_bridge_m_wvalid),
        
        .S01_AXI_araddr         (gmac_m_araddr),
        .S01_AXI_arburst        (gmac_m_arburst),
        .S01_AXI_arcache        (gmac_m_arcache),
        .S01_AXI_arid           (gmac_m_arid),
        .S01_AXI_arlen          (gmac_m_arlen),
        .S01_AXI_arlock         (gmac_m_arlock),
        .S01_AXI_arprot         (gmac_m_arprot),
        .S01_AXI_arqos          (),
        .S01_AXI_arready        (gmac_m_arready),
        .S01_AXI_arregion       (),
        .S01_AXI_arsize         (gmac_m_arsize),
        .S01_AXI_arvalid        (gmac_m_arvalid),
        .S01_AXI_awaddr         (gmac_m_awaddr),
        .S01_AXI_awburst        (gmac_m_awburst),
        .S01_AXI_awcache        (gmac_m_awcache),
        .S01_AXI_awid           (gmac_m_awid),
        .S01_AXI_awlen          (gmac_m_awlen),
        .S01_AXI_awlock         (gmac_m_awlock),
        .S01_AXI_awprot         (gmac_m_awprot),
        .S01_AXI_awqos          (),
        .S01_AXI_awready        (gmac_m_awready),
        .S01_AXI_awregion       (),
        .S01_AXI_awsize         (gmac_m_awsize),
        .S01_AXI_awvalid        (gmac_m_awvalid),
        .S01_AXI_bid            (gmac_m_bid),
        .S01_AXI_bready         (gmac_m_bready),
        .S01_AXI_bresp          (gmac_m_bresp),
        .S01_AXI_bvalid         (gmac_m_bvalid),
        .S01_AXI_rdata          (gmac_m_rdata),
        .S01_AXI_rid            (gmac_m_rid),
        .S01_AXI_rlast          (gmac_m_rlast),
        .S01_AXI_rready         (gmac_m_rready),
        .S01_AXI_rresp          (gmac_m_rresp),
        .S01_AXI_rvalid         (gmac_m_rvalid),
        .S01_AXI_wdata          (gmac_m_wdata),
        .S01_AXI_wlast          (gmac_m_wlast),
        .S01_AXI_wready         (gmac_m_wready),
        .S01_AXI_wstrb          (gmac_m_wstrb),
        .S01_AXI_wvalid         (gmac_m_wvalid));


endmodule
