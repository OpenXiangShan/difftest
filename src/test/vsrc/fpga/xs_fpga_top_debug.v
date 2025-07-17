`timescale 1ns/1ps

`include "sys_define.vh"

module xs_fpga_top_debug
(
   input                 clk8_p, // 1MHz clock
   input                 clk8_n,
   input                 clk7_p, // ddr 80MHz
   input                 clk7_n, 
   input                 clk6_p, // system 200MHz
   input                 clk6_n,
   input                 clk5_p, // debug 50MHz
   input                 clk5_n,
   input                 rstn_sw6,
   input                 rstn_sw5,
   input                 rstn_sw4,
   output                led0,
   output                led2,
   output                led3,
   // uart
`ifdef XS_UART
   output                uart0_sout,
   input                 uart0_sin,
   output                uart1_sout,
   input                 uart1_sin,
   output                uart2_sout,
   input                 uart2_sin,
`endif
   //PCIE 
   input                 refclk_p, // pcie 100MHz
   input                 refclk_n,
   output                PERST_N,
   
   input                 refclk2_p, // pcie 100MHz
   input                 refclk2_n,
   output                PERST2_N,
`ifdef XS_XMDA_EP
   input    [7:0]        pci_ep_rxn,
   input    [7:0]        pci_ep_rxp,
   output   [7:0]        pci_ep_txn,
   output   [7:0]        pci_ep_txp,
   input                 pcie_ep_gt_ref_clk_n,
   input                 pcie_ep_gt_ref_clk_p,
   output                pcie_ep_lnk_up,
   input                 pcie_ep_perstn,
`endif 
   //DDR
   output    [0:0]       DDR0_CK_T,
   output    [0:0]       DDR0_CK_C,
   output    [0:0]       DDR0_CKE,
   output    [0:0]       DDR0_CS_N,
   output    [0:0]       DDR0_ODT,
   output                DDR0_ACT_N,
   output    [1:0]       DDR0_BG,
   output    [1:0]       DDR0_BA,
   output    [16:0]      DDR0_A,
   output                DDR0_RESET_N,
   inout     [7:0]       DDR0_DM,
   inout     [63:0]      DDR0_DQ,
   inout     [7:0]       DDR0_DQS_T,
   inout     [7:0]       DDR0_DQS_C,
   
`ifdef XS_GMAC
   input                RGMII_RXCLK,
   input                RGMII_RXDV,
   input                RGMII_RXD0,
   input                RGMII_RXD1,
   input                RGMII_RXD2,
   input                RGMII_RXD3,
   output               RGMII_TXCLK,
   output               RGMII_TXEN,
   output               RGMII_TXD0,
   output               RGMII_TXD1,
   output               RGMII_TXD2,
   output               RGMII_TXD3,
   output               MDC,
   inout                MDIO,
   output               PHY_RESET_B,
`endif
   
   //==JTAG
   input                 JTAG_TCK,         // come from gpio
   input                 JTAG_TMS,         // come from gpio
   input                 JTAG_TDI,         // come from gpio
   output                JTAG_TDO,         // come from gpio
   input                 JTAG_TRSTn,       // come from gpio

   output                SD_CLK,
   inout                 SD_CMD,
   inout                 SD_DATA0,
   inout                 SD_DATA1,
   inout                 SD_DATA2,
   inout                 SD_DATA3,
   input                 SD_DECT
);

wire vio_sw6;
wire vio_sw5;
wire vio_sw4;
wire sys_rstn;
wire cpu_setn_buf;
wire ui_clk, sys_clk_i, dev_clk_i;
wire pcie_rstn;
(*mark_debug = "true"*) wire cpu_setn_rflag;
(*mark_debug = "true"*) reg  cpu_rstn;

IBUF sys_rstn_ibuf (.O(sys_rstn), .I(rstn_sw6));
IBUF cpu_rstn_ibuf (.O(cpu_setn_buf), .I(rstn_sw5));
OBUF pcie_rstn_obuf (.O(PERST_N), .I(vio_sw6));
OBUF pcie2_rstn_obuf (.O(PERST2_N), .I(vio_sw6));

button_debounce u_cpu_rstn(
   .clk             (sys_clk_i),
   .rstn            (sys_rstn && vio_sw6),
   .button_i        (cpu_setn_buf && vio_sw5),
   .button_rflag    (cpu_setn_rflag)
);

always@(posedge sys_clk_i) begin
   if ((sys_rstn == 1'b0)||(vio_sw6 == 1'b0))
      cpu_rstn <= 1'b0;
   else if (cpu_setn_rflag)
      cpu_rstn <= 1'b1;
   else
      cpu_rstn <= cpu_rstn;
end

// Suppose PHY_RESET_B active high.
// Although 88E1116R pin10 resetn is active low.
// But this pin is on connector, which might not have current by default?
assign PHY_RESET_B = cpu_rstn;

wire   tmclk;
wire   tmclk_buf;
IBUFGDS ibufgds_tmclk_1MHz
(
	.I              (clk8_p),
	.IB             (clk8_n),
	.O              (tmclk)
);
BUFG bufg_tmclk
(
    .I              (tmclk),
    .O              (tmclk_buf)
);

wire    cqetmclk;
wire    cqetmclk_buf;
IBUFGDS ibufgds_tmclk_200MHz
(
	.I              (clk6_p),
	.IB             (clk6_n),
	.O              (cqetmclk)
);

BUFG bufg_cqetmclk
(
    .I              (cqetmclk),
    .O              (cqetmclk_buf)
);

wire    dbg_clk;
wire    dbg_clk_buf;
IBUFGDS ibufgds_dbgclk_50MHz
(
	.I              (clk5_p),
	.IB             (clk5_n),
	.O              (dbg_clk)
);

BUFG bufg_dbgclk
(
    .I              (dbg_clk),
    .O              (dbg_clk_buf)
);

wire    pcie_sysclk_gt;
wire    pcie_sysclk;


IBUFDS_GTE4 refclk_ibuf (
    .O(pcie_sysclk_gt), 
    .ODIV2(pcie_sysclk), 
    .I(refclk_p), 
    .CEB(1'b0), 
    .IB(refclk_n)
);

IBUFDS_GTE4 refclk2_ibuf (
    .O(pcie2_sysclk_gt), 
    .ODIV2(pcie2_sysclk), 
    .I(refclk2_p), 
    .CEB(1'b0), 
    .IB(refclk2_n)
);

assign        sys_clk_i = ui_clk;
assign        dev_clk_i = ui_clk;

vio_0 u_vio(
   .clk        (dbg_clk_buf),

   .probe_out0 (vio_sw6),
   .probe_out1 (vio_sw5),
   .probe_out2 (vio_sw4)
);
//------------led test------------------

wire                sdmmc_cclk_card;
wire                sdmmc_card_detect_n;
wire                sdmmc_card_write_prot;
wire                sdmmc_sd_cmd_in;
wire   [3:0]        sdmmc_sd_dat_in;
wire                sdmmc_sd_cmd_out;
wire                sdmmc_sd_cmd_out_en;
wire   [3:0]        sdmmc_sd_dat_out;
wire   [3:0]        sdmmc_sd_dat_out_en;
wire   [1:0]        sdmmc_uhs1_drv_sth;
wire                sdmmc_uhs1_swvolt_en;
wire                sdmmc_sd_vdd1_on;
wire   [2:0]        sdmmc_sd_vdd1_sel;
wire                gmac_gmii_mdi_i;
wire                gmac_gmii_mdc_o;
wire                gmac_gmii_mdo_o;
wire                gmac_gmii_mdo_o_e;
wire                gmac_clk_rx_i;
wire   [3:0]        gmac_phy_rxd;
wire                gmac_phy_rxdv;
wire                gmac_clk_tx_o;
wire   [3:0]        gmac_phy_txd;
wire                gmac_phy_txen;

wire DDR0_ZQ = 'h0;

// qspi-io
//assign QSPI_CLK = qspi_sclk_out;
//assign QSPI_CS = qspi_n_ss_out;

//assign QSPI_DAT_3 = !qspi_n_mo_en[3] ? qspi_mo3 : 1'bz;
//assign QSPI_DAT_2 = !qspi_n_mo_en[2] ? qspi_mo2 : 1'bz;
//assign QSPI_DAT_1 = !qspi_n_mo_en[1] ? qspi_mo1 : 1'bz;
//assign QSPI_DAT_0 = !qspi_n_mo_en[0] ? qspi_mo0 : 1'bz;

//assign qspi_mi3 = QSPI_DAT_3;
//assign qspi_mi2 = QSPI_DAT_2;
//assign qspi_mi1 = QSPI_DAT_1;
//assign qspi_mi0 = QSPI_DAT_0;

// sd-io
assign SD_CLK = sdmmc_cclk_card;

assign sdmmc_sd_cmd_in = SD_CMD;
assign SD_CMD = sdmmc_sd_cmd_out_en ? sdmmc_sd_cmd_out : 1'bz; 

assign sdmmc_sd_dat_in[0] = SD_DATA0;
assign sdmmc_sd_dat_in[1] = SD_DATA1;
assign sdmmc_sd_dat_in[2] = SD_DATA2;
assign sdmmc_sd_dat_in[3] = SD_DATA3;

assign SD_DATA0 = sdmmc_sd_dat_out_en[0] ? sdmmc_sd_dat_out[0] : 1'bz;
assign SD_DATA1 = sdmmc_sd_dat_out_en[1] ? sdmmc_sd_dat_out[1] : 1'bz;
assign SD_DATA2 = sdmmc_sd_dat_out_en[2] ? sdmmc_sd_dat_out[2] : 1'bz;
assign SD_DATA3 = sdmmc_sd_dat_out_en[3] ? sdmmc_sd_dat_out[3] : 1'bz;

assign sdmmc_card_detect_n = SD_DECT;
assign sdmmc_card_write_prot = 0;

// gmac wiring
`ifdef XS_GMAC
wire       io_gmac_mdo_oe;
wire       io_gmac_mdo;
wire       io_gmac_mck_out;
wire       io_gmac_mdi;
wire       io_gmac_tx_clk;
wire       io_gmac_txd_en;
wire       io_gmac_rx_clk;
wire       io_gmac_rxd_vld;
wire [3:0] io_gmac_rxd;
wire [3:0] io_gmac_txd;

assign MDC = io_gmac_mck_out;
assign io_gmac_mdi = MDIO;
assign MDIO = io_gmac_mdo_oe ? io_gmac_mdo : 1'bz;


assign io_gmac_rx_clk = RGMII_RXCLK;
assign io_gmac_rxd_vld = RGMII_RXDV;
assign io_gmac_rxd[0] = RGMII_RXD0;
assign io_gmac_rxd[1] = RGMII_RXD1;
assign io_gmac_rxd[2] = RGMII_RXD2;
assign io_gmac_rxd[3] = RGMII_RXD3;

assign RGMII_TXCLK = io_gmac_tx_clk;
assign RGMII_TXEN = io_gmac_txd_en;
assign RGMII_TXD0 = io_gmac_txd[0];
assign RGMII_TXD1 = io_gmac_txd[1];
assign RGMII_TXD2 = io_gmac_txd[2];
assign RGMII_TXD3 = io_gmac_txd[3];
`endif

// JTAG
wire      io_systemjtag_jtag_TCK;
wire      io_systemjtag_jtag_TMS;
wire      io_systemjtag_jtag_TDI;
wire      io_systemjtag_jtag_TDO_data;
wire      io_systemjtag_jtag_TDO_driven;
wire      io_systemjtag_reset;

assign io_systemjtag_jtag_TCK = JTAG_TCK;
assign io_systemjtag_jtag_TMS = JTAG_TMS;
assign io_systemjtag_jtag_TDI = JTAG_TDI;
assign JTAG_TDO = io_systemjtag_jtag_TDO_driven ? io_systemjtag_jtag_TDO_data : 1'bz;
assign io_systemjtag_reset = ~JTAG_TRSTn;

// core
xs_core_def xs_core_def
(
  .ddr_clk_p            (clk7_p),
  .ddr_clk_n            (clk7_n),  
  .tmclk                (tmclk_buf),
  .cqetmclk             (cqetmclk_buf),
  .ui_clk               (ui_clk),
  .init_calib_complete  (led3),
  .cpu_rd_qspi_valid    (led2),
  .cpu_wr_ddr_valid     (led0),
  .sys_clk_i            (sys_clk_i),
  .dev_clk_i            (dev_clk_i),
  .sys_rstn             (sys_rstn && vio_sw6),
  .cpu_rstn             (cpu_rstn),
  .rstn_sw4             (rstn_sw4 && vio_sw4),
  .dft_lgc_rst_n        (1'b1),
  .dft_se               (1'b0),
  .chip_mode_i          (2'b00), // normal mode
  .dft_crg_rst_n        (1'b1),
  // pcie
`ifdef XS_XMDA_EP
  .pci_ep_rxn           (pci_ep_rxn),
  .pci_ep_rxp           (pci_ep_rxp),
  .pci_ep_txn           (pci_ep_txn),
  .pci_ep_txp           (pci_ep_txp),
  .pcie_ep_gt_ref_clk_n (pcie_ep_gt_ref_clk_n),
  .pcie_ep_gt_ref_clk_p (pcie_ep_gt_ref_clk_p),
  .pcie_ep_lnk_up       (pcie_ep_lnk_up),
  .pcie_ep_perstn       (pcie_ep_perstn),
`endif
`ifdef XS_UART
  // uart
  .uart0_sout           (uart0_sout),
  .uart1_sout           (uart1_sout),
  .uart2_sout           (uart2_sout),
  .uart0_sin            (uart0_sin),
  .uart1_sin            (uart1_sin),
  .uart2_sin            (uart2_sin),
`endif  
  // JTAG
  .io_systemjtag_jtag_TCK         (io_systemjtag_jtag_TCK ),
  .io_systemjtag_jtag_TMS         (io_systemjtag_jtag_TMS ),
  .io_systemjtag_jtag_TDI         (io_systemjtag_jtag_TDI ),
  .io_systemjtag_jtag_TDO_data    (io_systemjtag_jtag_TDO_data ),
  .io_systemjtag_jtag_TDO_driven  (io_systemjtag_jtag_TDO_driven ),
  .io_systemjtag_reset            (io_systemjtag_reset  ),

  // qspi
//  .qspi_mo0(qspi_mo0),
//  .qspi_mo1(qspi_mo1),
//  .qspi_mo2(qspi_mo2),
//  .qspi_mo3(qspi_mo3),
//  .qspi_mo_oe_n(qspi_n_mo_en),
//  .qspi_cs_out_n(qspi_n_ss_out),
//  .qspi_sclk_out(qspi_sclk_out),
//  .qspi_mi0(qspi_mi0),
//  .qspi_mi1(qspi_mi1),
//  .qspi_mi2(qspi_mi2),
//  .qspi_mi3(qspi_mi3),

   // sdmmc
  .sd_card_clk_out      (sdmmc_cclk_card),
  .sd_cmd_out           (sdmmc_sd_cmd_out),
  .sd_cmd_out_oe        (sdmmc_sd_cmd_out_en),
  .sd_dat_out           (sdmmc_sd_dat_out),
  .sd_dat_out_oe        (sdmmc_sd_dat_out_en),
  .uhs1_drv_sth         (sdmmc_uhs1_drv_sth),
  .uhs1_swvolt_en       (sdmmc_uhs1_swvolt_en),
  .sd_vdd1_on           (sdmmc_sd_vdd1_on),
  .sd_vdd1_sel          (sdmmc_sd_vdd1_sel),
  .sd_card_det_in       (sdmmc_card_detect_n),
  .sd_card_wp_in        (sdmmc_card_write_prot),
  .sd_cmd_in            (sdmmc_sd_cmd_in),
  .sd_dat_in            (sdmmc_sd_dat_in),
  .sd_led_control       (          ),
  
  `ifdef XS_GMAC
  // gmac
  .io_gmac_mdo_oe  ( io_gmac_mdo_oe  ),
  .io_gmac_mdo     ( io_gmac_mdo     ),
  .io_gmac_mck_out ( io_gmac_mck_out ),
  .io_gmac_mdi     ( io_gmac_mdi     ),
  .io_gmac_tx_clk  ( io_gmac_tx_clk  ),
  .io_gmac_txd_en  ( io_gmac_txd_en  ),
  .io_gmac_rx_clk  ( io_gmac_rx_clk  ),
  .io_gmac_rxd_vld ( io_gmac_rxd_vld ),
  .io_gmac_rxd     ( io_gmac_rxd     ),
  .io_gmac_txd     ( io_gmac_txd     ),
  `endif

  // ddr
  .DDR_CK_T             (DDR0_CK_T        ),   
  .DDR_CK_C             (DDR0_CK_C        ),   
  .DDR_CKE              (DDR0_CKE         ),    
  .DDR_CS_N             (DDR0_CS_N        ),   
  .DDR_ODT              (DDR0_ODT         ),    
  .DDR_ACT_N            (DDR0_ACT_N       ),  
  .DDR_BG               (DDR0_BG          ),     
  .DDR_BA               (DDR0_BA          ),     
  .DDR_A                (DDR0_A           ),      
  .DDR_RESET_N          (DDR0_RESET_N     ), 
  .DDR_DM_N             (DDR0_DM          ),     
  .DDR_DQ               (DDR0_DQ          ),     
  .DDR_DQS_T            (DDR0_DQS_T       ),  
  .DDR_DQS_C            (DDR0_DQS_C       )/*,  
  .DDR_ZQ               (DDR0_ZQ),
  .gmac_gmii_mdc_o(gmac_gmii_mdc_o),
  .gmac_gmii_mdo_o(gmac_gmii_mdo_o),
  .gmac_gmii_mdo_o_e(gmac_gmii_mdo_o_e),
  .gmac_clk_tx_o(gmac_clk_tx_o),
  .gmac_phy_txd(gmac_phy_txd),
  .gmac_phy_txen(gmac_phy_txen),
  .gmac_gmii_mdi_i(gmac_gmii_mdi_i),
  .gmac_clk_rx_i(gmac_clk_rx_i),
  .gmac_phy_rxd(gmac_phy_rxd),
  .gmac_phy_rxdv(gmac_phy_rxdv),
  
  .gpio_porta_ddr(gpio_porta_ddr),
  .gpio_porta_dr(gpio_porta_dr),
  .gpio_ext_porta(gpio_ext_porta),
  
  .jtag_tck(jtag_tck), 
  .jtag_tdi(jtag_tdi), 
  .jtag_tms(jtag_tms), 
  .jtag_trstn(jtag_trstn),
  .jtag_tdo(jtag_tdo), 
  
  */

);

//---
/*
wire                gmac_gmii_mdi_i;
wire                gmac_gmii_mdc_o;
wire                gmac_gmii_mdo_o;
wire                gmac_gmii_mdo_o_e;

wire                gmac_clk_rx_i;
wire                gmac_phy_rxdv;
wire   [3:0]        gmac_phy_rxd;

wire                gmac_clk_tx_o;
wire                gmac_phy_txen;
wire   [3:0]        gmac_phy_txd;


assign MDC = gmac_gmii_mdc_o;
assign gmac_gmii_mdi_i = MDIO;
assign MDIO = gmac_gmii_mdo_o_e ? gmac_gmii_mdo_o : 1'bz;

assign gmac_clk_rx_i = RGMII_RXCLK;
assign gmac_phy_rxdv = RGMII_RXDV;
assign gmac_phy_rxd[0] = RGMII_RXD0;
assign gmac_phy_rxd[1] = RGMII_RXD1;
assign gmac_phy_rxd[2] = RGMII_RXD2;
assign gmac_phy_rxd[3] = RGMII_RXD3;

assign RGMII_TXCLK = gmac_clk_tx_o;
assign RGMII_TXEN = gmac_phy_txen;
assign RGMII_TXD0 = gmac_phy_txd[0];
assign RGMII_TXD1 = gmac_phy_txd[1];
assign RGMII_TXD2 = gmac_phy_txd[2];
assign RGMII_TXD3 = gmac_phy_txd[3];


assign gpio_ext_porta = 32'h0;
assign GPIO_O0 = |gpio_porta_ddr;
assign GPIO_O1 = |gpio_porta_dr;
*/

endmodule
