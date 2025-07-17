####### Main board
#clk



set_property PACKAGE_PIN AY50 [get_ports clk8_p]
set_property PACKAGE_PIN BA50 [get_ports clk8_n]

set_property PACKAGE_PIN R42 [get_ports clk6_p]
set_property PACKAGE_PIN R43 [get_ports clk6_n]

set_property PACKAGE_PIN C35 [get_ports clk5_p]
set_property PACKAGE_PIN C36 [get_ports clk5_n]

set_property PACKAGE_PIN BA11 [get_ports refclk_p]
set_property PACKAGE_PIN BA10 [get_ports refclk_n]

set_property PACKAGE_PIN AR11 [get_ports refclk2_p]
set_property PACKAGE_PIN AR10 [get_ports refclk2_n]

#set_property PACKAGE_PIN CA36 [get_ports clk4_p]
#set_property PACKAGE_PIN CA37 [get_ports clk4_n]

#set_property PACKAGE_PIN BY38 [get_ports clk3_p]
#set_property PACKAGE_PIN BY39 [get_ports clk3_n]

#set_property PACKAGE_PIN CB19 [get_ports clk2_p]
#set_property PACKAGE_PIN CB18 [get_ports clk2_n]

#set_property PACKAGE_PIN CA17 [get_ports clk1_p]
#set_property PACKAGE_PIN CA16 [get_ports clk1_n]

#reset
set_property PACKAGE_PIN E47 [get_ports rstn_sw6]
set_property PACKAGE_PIN BY53 [get_ports rstn_sw5]
set_property PACKAGE_PIN CA39 [get_ports rstn_sw4]

#uart
set_property PACKAGE_PIN AD13 [get_ports uart0_sout]

set_property PACKAGE_PIN AE13 [get_ports uart0_sin]

set_property PACKAGE_PIN AE14 [get_ports uart1_sout]

set_property PACKAGE_PIN AE15 [get_ports uart1_sin]

set_property PACKAGE_PIN V16 [get_ports uart2_sout]

set_property PACKAGE_PIN W15 [get_ports uart2_sin]
#pcie
set_property PACKAGE_PIN BY22 [get_ports pcie_ep_perstn]
set_property PACKAGE_PIN AU11 [get_ports pcie_ep_gt_ref_clk_p]
set_property PACKAGE_PIN AD15 [get_ports pcie_ep_lnk_up]

set_property PACKAGE_PIN AT4 [get_ports {pci_ep_rxp[7]} ]
set_property PACKAGE_PIN AR2 [get_ports {pci_ep_rxp[6]} ]
set_property PACKAGE_PIN AP4 [get_ports {pci_ep_rxp[5]} ]
set_property PACKAGE_PIN AN2 [get_ports {pci_ep_rxp[4]} ]
set_property PACKAGE_PIN AM4 [get_ports {pci_ep_rxp[3]} ]
set_property PACKAGE_PIN AL2 [get_ports {pci_ep_rxp[2]} ]
set_property PACKAGE_PIN AK4 [get_ports {pci_ep_rxp[1]} ]
set_property PACKAGE_PIN AJ2 [get_ports {pci_ep_rxp[0]} ]

set_property PACKAGE_PIN AV9 [get_ports {pci_ep_txp[7]} ]
set_property PACKAGE_PIN AU7 [get_ports {pci_ep_txp[6]} ]
set_property PACKAGE_PIN AT9 [get_ports {pci_ep_txp[5]} ]
set_property PACKAGE_PIN AR7 [get_ports {pci_ep_txp[4]} ]
set_property PACKAGE_PIN AP9 [get_ports {pci_ep_txp[3]} ]
set_property PACKAGE_PIN AN7 [get_ports {pci_ep_txp[2]} ]
set_property PACKAGE_PIN AM9 [get_ports {pci_ep_txp[1]} ]
set_property PACKAGE_PIN AL7 [get_ports {pci_ep_txp[0]} ]
set_property IOSTANDARD LVCMOS33 [get_ports pcie_ep_lnk_up]
set_property IOSTANDARD LVCMOS18 [get_ports pcie_ep_perstn]
create_clock -name PCIE_CLK -period 10.000 [get_ports pcie_sys_clk_p]
#gpio
#set_property PACKAGE_PIN AA15 [get_ports GPIO_O0]

#set_property PACKAGE_PIN AA16 [get_ports GPIO_O1]
#j5#
# set_property PACKAGE_PIN AL51 [get_ports SD_CLK]
#
# set_property PACKAGE_PIN AK49 [get_ports SD_CMD]
#
# set_property PACKAGE_PIN AL52 [get_ports SD_DATA0]
#
# set_property PACKAGE_PIN AK50 [get_ports SD_DATA1]
#
# set_property PACKAGE_PIN AM49 [get_ports SD_DATA2]
#
# set_property PACKAGE_PIN AK45 [get_ports SD_DATA3]
#
# set_property PACKAGE_PIN AN49 [get_ports SD_DECT]
#



####### J1
set_property PACKAGE_PIN CC39 [get_ports led0]

#set_property PACKAGE_PIN CB39 [get_ports led1]

set_property PACKAGE_PIN CB36 [get_ports led2]

set_property PACKAGE_PIN CC35 [get_ports led3]

#J7# # set_property PACKAGE_PIN BK40 [get_ports QSPI_CS]
#
# set_property PACKAGE_PIN BN40 [get_ports QSPI_CLK]
#
# set_property PACKAGE_PIN BP41 [get_ports QSPI_DAT_0]
#
# set_property PACKAGE_PIN BK39 [get_ports QSPI_DAT_1]
#
# set_property PACKAGE_PIN BP40 [get_ports QSPI_DAT_2]
#
# set_property PACKAGE_PIN BG39 [get_ports QSPI_DAT_3]
#
# ##
# set_property PACKAGE_PIN BL32 [get_ports SD_CLK]
#
# set_property PACKAGE_PIN BJ33 [get_ports SD_CMD]
#
# set_property PACKAGE_PIN BM32 [get_ports SD_DATA0]
#
# set_property PACKAGE_PIN BK33 [get_ports SD_DATA1]
#
# set_property PACKAGE_PIN BJ31 [get_ports SD_DATA2]
#
# set_property PACKAGE_PIN BG33 [get_ports SD_DATA3]
#
# set_property PACKAGE_PIN BJ30 [get_ports SD_DECT]
#
##
# set_property PACKAGE_PIN J5 [get_ports QSPI_CS]
#
# set_property PACKAGE_PIN J1 [get_ports QSPI_CLK]
#
# set_property PACKAGE_PIN J2 [get_ports QSPI_DAT_0]
#
# set_property PACKAGE_PIN J6 [get_ports QSPI_DAT_1]
#
# set_property PACKAGE_PIN H1 [get_ports QSPI_DAT_2]
#
# set_property PACKAGE_PIN K3 [get_ports QSPI_DAT_3]
#
# ##
##J4
#set_property PACKAGE_PIN G49 [get_ports QSPI_CS]
#set_property PACKAGE_PIN M46 [get_ports QSPI_CLK]
#set_property PACKAGE_PIN L47 [get_ports QSPI_DAT_0]
#set_property PACKAGE_PIN H49 [get_ports QSPI_DAT_1]
#set_property PACKAGE_PIN M47 [get_ports QSPI_DAT_2]
#set_property PACKAGE_PIN H51 [get_ports QSPI_DAT_3]

#create_clock -period 40.000 -name qspi_vclk [get_ports QSPI_CLK]


##J4
set_property PACKAGE_PIN F54 [get_ports JTAG_TCK]
set_property PACKAGE_PIN A54 [get_ports JTAG_TMS]
set_property PACKAGE_PIN H54 [get_ports JTAG_TDI]
set_property PACKAGE_PIN C54 [get_ports JTAG_TDO]
set_property PACKAGE_PIN G54 [get_ports JTAG_TRSTn]

set_property PACKAGE_PIN K9 [get_ports SD_CLK]

set_property PACKAGE_PIN H10 [get_ports SD_CMD]

set_property PACKAGE_PIN K8 [get_ports SD_DATA0]

set_property PACKAGE_PIN H9 [get_ports SD_DATA1]

set_property PACKAGE_PIN G11 [get_ports SD_DATA2]

set_property PACKAGE_PIN C8 [get_ports SD_DATA3]

set_property PACKAGE_PIN F11 [get_ports SD_DECT]

#set_property IOSTANDARD LVCMOS18 [get_ports SD_WP]
#set_property PACKAGE_PIN B8 [get_ports SD_WP]


######## J3
#set_property PACKAGE_PIN R40 [get_ports MDC]
#set_property PACKAGE_PIN R39 [get_ports MDIO]
#set_property PACKAGE_PIN P38 [get_ports RGMII_RXCLK]
#set_property PACKAGE_PIN L39 [get_ports RGMII_RXDV]
#set_property PACKAGE_PIN L40 [get_ports RGMII_RXD0]
#set_property PACKAGE_PIN R37 [get_ports RGMII_RXD1]
#set_property PACKAGE_PIN P37 [get_ports RGMII_RXD2]
#set_property PACKAGE_PIN N38 [get_ports RGMII_RXD3]
#set_property PACKAGE_PIN P40 [get_ports RGMII_TXCLK]
#set_property PACKAGE_PIN L37 [get_ports RGMII_TXEN]
#set_property PACKAGE_PIN N39 [get_ports RGMII_TXD0]
#set_property PACKAGE_PIN M39 [get_ports RGMII_TXD1]
#set_property PACKAGE_PIN N40 [get_ports RGMII_TXD2]
#set_property PACKAGE_PIN M37 [get_ports RGMII_TXD3]
#set_property PACKAGE_PIN W40 [get_ports PHY_RESET_B]

####### J1
#set_property PACKAGE_PIN R15 [get_ports MDC]
#set_property PACKAGE_PIN T15 [get_ports MDIO]
#set_property PACKAGE_PIN H13 [get_ports RGMII_RXCLK]
#set_property PACKAGE_PIN K15 [get_ports RGMII_RXDV]
#set_property PACKAGE_PIN J15 [get_ports RGMII_RXD0]
#set_property PACKAGE_PIN H15 [get_ports RGMII_RXD1]
#set_property PACKAGE_PIN H14 [get_ports RGMII_RXD2]
#set_property PACKAGE_PIN G13 [get_ports RGMII_RXD3]
#set_property PACKAGE_PIN J13 [get_ports RGMII_TXCLK]
#set_property PACKAGE_PIN K13 [get_ports RGMII_TXEN]
#set_property PACKAGE_PIN L12 [get_ports RGMII_TXD0]
#set_property PACKAGE_PIN K12 [get_ports RGMII_TXD1]
#set_property PACKAGE_PIN J12 [get_ports RGMII_TXD2]
#set_property PACKAGE_PIN K14 [get_ports RGMII_TXD3]
#set_property PACKAGE_PIN N14 [get_ports PHY_RESET_B]

####### J5
set_property PACKAGE_PIN AV58 [get_ports MDC]
set_property PACKAGE_PIN AU58 [get_ports MDIO]
set_property PACKAGE_PIN AM61 [get_ports RGMII_RXCLK]
set_property PACKAGE_PIN AN63 [get_ports RGMII_RXDV]
set_property PACKAGE_PIN AP63 [get_ports RGMII_RXD0]
set_property PACKAGE_PIN AN60 [get_ports RGMII_RXD1]
set_property PACKAGE_PIN AN61 [get_ports RGMII_RXD2]
set_property PACKAGE_PIN AM62 [get_ports RGMII_RXD3]
set_property PACKAGE_PIN AR62 [get_ports RGMII_TXCLK]
set_property PACKAGE_PIN AR63 [get_ports RGMII_TXEN]
set_property PACKAGE_PIN AP60 [get_ports RGMII_TXD0]
set_property PACKAGE_PIN AP61 [get_ports RGMII_TXD1]
set_property PACKAGE_PIN AT62 [get_ports RGMII_TXD2]
set_property PACKAGE_PIN AP62 [get_ports RGMII_TXD3]
set_property PACKAGE_PIN AR60 [get_ports PHY_RESET_B]

set_property IOB TRUE [get_ports MDC]
set_property IOB TRUE [get_ports MDIO]
set_property IOB TRUE [get_ports RGMII_RXCLK]
set_property IOB TRUE [get_ports RGMII_RXDV]
set_property IOB TRUE [get_ports RGMII_RXD0]
set_property IOB TRUE [get_ports RGMII_RXD1]
set_property IOB TRUE [get_ports RGMII_RXD2]
set_property IOB TRUE [get_ports RGMII_RXD3]
set_property IOB TRUE [get_ports RGMII_TXCLK]
set_property IOB TRUE [get_ports RGMII_TXEN]
set_property IOB TRUE [get_ports RGMII_TXD0]
set_property IOB TRUE [get_ports RGMII_TXD1]
set_property IOB TRUE [get_ports RGMII_TXD2]
set_property IOB TRUE [get_ports RGMII_TXD3]


#create_clock -period 40.00 -name mac_rx_clk [get_ports RGMII_RXCLK]
#create_clock -period 40.00 -name mac_tx_clk [get_ports RGMII_TXCLK]
#set_output_delay -clock mac_tx_clk -max -add_delay 9.200 [get_ports {RGMII_TXD0 RGMII_TXD1 RGMII_TXD2 RGMII_TXD3 RGMII_TXEN}]
#set_output_delay -clock mac_tx_clk -min -add_delay 0.800 [get_ports {RGMII_TXD0 RGMII_TXD1 RGMII_TXD2 RGMII_TXD3 RGMII_TXEN}]
#set_output_delay -clock mac_tx_clk -clock_fall -max -add_delay 9.200 [get_ports {RGMII_TXD0 RGMII_TXD1 RGMII_TXD2 RGMII_TXD3 RGMII_TXEN}]
#set_output_delay -clock mac_tx_clk -clock_fall -min -add_delay 0.800 [get_ports {RGMII_TXD0 RGMII_TXD1 RGMII_TXD2 RGMII_TXD3 RGMII_TXEN}]
#set_input_delay -clock mac_rx_clk -max -add_delay 9.200 [get_ports {RGMII_RXD0 RGMII_RXD1 RGMII_RXD2 RGMII_RXD3 RGMII_RXDV}]
#set_input_delay -clock mac_rx_clk -min -add_delay 0.700 [get_ports {RGMII_RXD0 RGMII_RXD1 RGMII_RXD2 RGMII_RXD3 RGMII_RXDV}]
#set_input_delay -clock mac_rx_clk -clock_fall -max -add_delay 9.200 [get_ports {RGMII_RXD0 RGMII_RXD1 RGMII_RXD2 RGMII_RXD3 RGMII_RXDV}]
#set_input_delay -clock mac_rx_clk -clock_fall -min -add_delay 0.700 [get_ports {RGMII_RXD0 RGMII_RXD1 RGMII_RXD2 RGMII_RXD3 RGMII_RXDV}]
#set_property DRIVE 12 [get_ports RGMII_TXCLK]
#set_property DRIVE 12 [get_ports RGMII_TXEN]
#set_property DRIVE 12 [get_ports RGMII_TXD0]
#set_property DRIVE 12 [get_ports RGMII_TXD1]
#set_property DRIVE 12 [get_ports RGMII_TXD2]
#set_property DRIVE 12 [get_ports RGMII_TXD3]


#set_property DELAY_VALUE 900  [get_cells {*/util_gmii_to_rgmii_m0/delay_rgmii_rx* */util_gmii_to_rgmii_m0/rxdata_bus[*].delay_rgmii_rx*}]

#set_property IODELAY_GROUP tri_mode_ethernet_mac_iodelay_grp [get_cells {*/util_gmii_to_rgmii_m0/delay_rgmii_tx* */util_gmii_to_rgmii_m0/txdata_out_bus[*].delay_rgmii_tx*}]
#set_property IODELAY_GROUP tri_mode_ethernet_mac_iodelay_grp [get_cells {*/util_gmii_to_rgmii_m0/delay_rgmii_rx* */util_gmii_to_rgmii_m0/rxdata_bus[*].delay_rgmii_rx*}]
#set_property IODELAY_GROUP tri_mode_ethernet_mac_iodelay_grp [get_cells  {IDELAYCTRL_inst}]

# IDELAYCTRL reset input is false path

#set_clock_groups -asynchronous -group [get_clocks -of_objects [get_ports clk8_p] -include_generated_clocks]
#set_clock_groups -asynchronous -group [get_clocks -of_objects [get_ports clk7_p] -include_generated_clocks]
#set_clock_groups -asynchronous -group [get_clocks -of_objects [get_ports clk6_p] -include_generated_clocks]
#set_clock_groups -asynchronous -group [get_clocks -of_objects [get_ports clk5_p] -include_generated_clocks]
#set_clock_groups -asynchronous -group [get_clocks -of_objects [get_ports refclk_p] -include_generated_clocks]

#create_generated_clock -name rgmii1_txc -divide_by 1 -source [get_pins {xs_core_def/u_gmac_top/DWC_gmac_top_u0/DWC_gmac_inst/DWC_gmac_rgmii_inst/DWC_gmac_rgmii_gmrt_inst/clk_tx_div2_neg_reg/C}] [get_ports RGMII_TXCLK]









#####################################################################################
# JX1-- PCIe x16 slot PCIe1
# board connector --  PCIe Gen2 x4
# phy_ip --  x1 Gen3
# set_property PACKAGE_PIN BF8 [get_ports PCIE_TXN]
# set_property PACKAGE_PIN BF9 [get_ports PCIE_TXP]
# set_property PACKAGE_PIN BD3 [get_ports PCIE_RXN]
# set_property PACKAGE_PIN BD4 [get_ports PCIE_RXP]
set_property PACKAGE_PIN CB13 [get_ports PERST_N]
set_property PACKAGE_PIN CB23 [get_ports PERST2_N]

#####################################################################################



####################################################################################
# Constraints from file : 'ddr.xdc'
####################################################################################

# set_property IOSTANDARD LVDS [get_ports clk1_p]
# set_property IOSTANDARD LVDS [get_ports clk1_n]
# set_property IOSTANDARD LVDS [get_ports clk2_p]
# set_property IOSTANDARD LVDS [get_ports clk2_n]
# set_property IOSTANDARD LVDS [get_ports clk3_p]
# set_property IOSTANDARD LVDS [get_ports clk3_n]
# set_property IOSTANDARD LVDS [get_ports clk4_p]
# set_property IOSTANDARD LVDS [get_ports clk4_n]

#set_property IOSTANDARD LVDS [get_ports refclk_p]
#set_property IOSTANDARD LVDS [get_ports refclk_n]
#set_property IOSTANDARD LVCMOS33 [get_ports GPIO_O0]
#set_property IOSTANDARD LVCMOS33 [get_ports GPIO_O1]
#set_property IOSTANDARD LVCMOS18 [get_ports led1]
#set_property IOSTANDARD LVCMOS18 [get_ports QSPI_CS]
#set_property IOSTANDARD LVCMOS18 [get_ports QSPI_CLK]
#set_property IOSTANDARD LVCMOS18 [get_ports QSPI_DAT_0]
#set_property IOSTANDARD LVCMOS18 [get_ports QSPI_DAT_1]
#set_property IOSTANDARD LVCMOS18 [get_ports QSPI_DAT_2]
#set_property IOSTANDARD LVCMOS18 [get_ports QSPI_DAT_3]



#create_clock -name MMCM_CLK_OUT [get_nets xs_core_def/U_JTAG_DDR_SUBSYS/jtag_ddr_subsys_i/ddr4_0/inst/u_ddr4_infrastructure/addn_ui_clkout1]

#set_clock_groups -group [get_clocks -include_generated_clocks CPU_CLK_IN] -group [get_clocks -include_generated_clocks SOC_SYS_CLK] -asynchronous
#set_clock_groups -group [get_clocks -include_generated_clocks TMCLK] -group [get_clocks -include_generated_clocks SOC_SYS_CLK] -asynchronous
#set_clock_groups -group [get_clocks -include_generated_clocks DEBUG_CLK_IN] -group [get_clocks -include_generated_clocks SOC_SYS_CLK] -asynchronous
#set_clock_groups -group [get_clocks -include_generated_clocks PCIE_CLK_IN] -group [get_clocks -include_generated_clocks SOC_SYS_CLK] -asynchronous
#set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks GCLK_RGMII_TXC] -group [get_clocks -include_generated_clocks SOC_SYS_CLK]
#set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks jtag_vclk] -group [get_clocks -include_generated_clocks SOC_SYS_CLK]





####################################################################################
# Constraints from file : 'constraints.xdc'
####################################################################################







#create_clock -period 12.500 -name DDR_CLK [get_ports clk7_p]

#create_generated_clock -name SOC_SYS_CLK -source [get_pins xs_core_def/U_JTAG_DDR_SUBSYS/jtag_ddr_subsys_i/SOC_CLK] -multiply_by 1
#create_generated_clock -name GCLK_RGMII_TXC -source [get_pins xs_core_def/U_JTAG_DDR_SUBSYS/jtag_ddr_subsys_i/MAC_CLK] -multiply_by 1


####################################################################################
# Constraints from file : 'jtag_ddr_subsys_s01_data_fifo_0_clocks.xdc'
####################################################################################


set_property PACKAGE_PIN Y52 [get_ports clk7_p]
set_property PACKAGE_PIN Y53 [get_ports clk7_n]

create_clock -period 40.000 -name mac_rx_clk [get_ports RGMII_RXCLK]
create_clock -period 40.000 -name mac_tx_clk [get_ports RGMII_TXCLK]
set_clock_groups -asynchronous -group [get_clocks mac_rx_clk -include_generated_clocks]
create_clock -period 40.000 -name rgmii_rx_vclk_1
set_false_path -setup -rise_from [get_clocks rgmii_rx_vclk_1] -fall_to [get_clocks mac_rx_clk]
set_false_path -setup -fall_from [get_clocks rgmii_rx_vclk_1] -rise_to [get_clocks mac_rx_clk]
set_false_path -hold -rise_from [get_clocks rgmii_rx_vclk_1] -rise_to [get_clocks mac_rx_clk]
set_false_path -hold -fall_from [get_clocks rgmii_rx_vclk_1] -fall_to [get_clocks mac_rx_clk]
set_multicycle_path -setup -from [get_clocks rgmii_rx_vclk_1] -to [get_clocks mac_rx_clk] 0
set_multicycle_path -hold -from [get_clocks rgmii_rx_vclk_1] -to [get_clocks mac_rx_clk] -1
set_input_delay -clock [get_clocks rgmii_rx_vclk_1] -max 5.000 [get_ports {RGMII_RXD0 RGMII_RXD1 RGMII_RXD2 RGMII_RXD3 RGMII_RXDV}]
set_input_delay -clock [get_clocks rgmii_rx_vclk_1] -min 4.500 [get_ports {RGMII_RXD0 RGMII_RXD1 RGMII_RXD2 RGMII_RXD3 RGMII_RXDV}]
set_input_delay -clock [get_clocks rgmii_rx_vclk_1] -clock_fall -max -add_delay 5.000 [get_ports {RGMII_RXD0 RGMII_RXD1 RGMII_RXD2 RGMII_RXD3 RGMII_RXDV}]
set_input_delay -clock [get_clocks rgmii_rx_vclk_1] -clock_fall -min -add_delay 4.500 [get_ports {RGMII_RXD0 RGMII_RXD1 RGMII_RXD2 RGMII_RXD3 RGMII_RXDV}]
set_output_delay -clock [get_clocks mac_tx_clk] -max 12.750 [get_ports {RGMII_TXD0 RGMII_TXD1 RGMII_TXD2 RGMII_TXD3 RGMII_TXEN}]
set_output_delay -clock [get_clocks mac_tx_clk] -min 11.500 [get_ports {RGMII_TXD0 RGMII_TXD1 RGMII_TXD2 RGMII_TXD3 RGMII_TXEN}]
set_output_delay -clock [get_clocks mac_tx_clk] -clock_fall -max -add_delay 12.750 [get_ports {RGMII_TXD0 RGMII_TXD1 RGMII_TXD2 RGMII_TXD3 RGMII_TXEN}]
set_output_delay -clock [get_clocks mac_tx_clk] -clock_fall -min -add_delay 11.500 [get_ports {RGMII_TXD0 RGMII_TXD1 RGMII_TXD2 RGMII_TXD3 RGMII_TXEN}]
set_property IOSTANDARD LVDS [get_ports clk7_p]
set_property IOSTANDARD LVDS [get_ports clk7_n]
set_property IOSTANDARD LVDS [get_ports clk8_p]
set_property IOSTANDARD LVDS [get_ports clk8_n]
set_property IOSTANDARD LVDS [get_ports clk6_p]
set_property IOSTANDARD LVDS [get_ports clk6_n]
set_property IOSTANDARD LVDS [get_ports clk5_p]
set_property IOSTANDARD LVDS [get_ports clk5_n]
set_property DIFF_TERM <true> [get_ports clk5_p]
set_property DIFF_TERM_ADV TERM_100  [get_ports clk5_p]
set_property DIFF_TERM <true> [get_ports clk6_p]
set_property DIFF_TERM_ADV TERM_100  [get_ports clk6_p]
set_property IOSTANDARD LVCMOS18 [get_ports PERST_N]
set_property IOSTANDARD LVCMOS18 [get_ports PERST2_N]
set_property IOSTANDARD LVCMOS18 [get_ports rstn_sw6]
set_property IOSTANDARD LVCMOS18 [get_ports rstn_sw5]
set_property IOSTANDARD LVCMOS18 [get_ports rstn_sw4]
set_property IOSTANDARD LVCMOS33 [get_ports uart0_sout]
set_property IOSTANDARD LVCMOS33 [get_ports uart0_sin]
set_property IOSTANDARD LVCMOS33 [get_ports uart1_sout]
set_property IOSTANDARD LVCMOS33 [get_ports uart1_sin]
set_property IOSTANDARD LVCMOS33 [get_ports uart2_sout]
set_property IOSTANDARD LVCMOS33 [get_ports uart2_sin]
set_property IOSTANDARD LVCMOS18 [get_ports led0]
set_property IOSTANDARD LVCMOS18 [get_ports led2]
set_property IOSTANDARD LVCMOS18 [get_ports led3]
set_property IOSTANDARD LVCMOS18 [get_ports JTAG_TCK]
set_property IOSTANDARD LVCMOS18 [get_ports JTAG_TMS]
set_property IOSTANDARD LVCMOS18 [get_ports JTAG_TDI]
set_property IOSTANDARD LVCMOS18 [get_ports JTAG_TDO]
set_property IOSTANDARD LVCMOS18 [get_ports JTAG_TRSTn]
set_property IOSTANDARD LVCMOS18 [get_ports SD_CLK]
set_property IOSTANDARD LVCMOS18 [get_ports SD_CMD]
set_property IOSTANDARD LVCMOS18 [get_ports SD_DATA0]
set_property IOSTANDARD LVCMOS18 [get_ports SD_DATA1]
set_property IOSTANDARD LVCMOS18 [get_ports SD_DATA2]
set_property IOSTANDARD LVCMOS18 [get_ports SD_DATA3]
set_property IOSTANDARD LVCMOS18 [get_ports SD_DECT]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_RXCLK]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_RXDV]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_RXD0]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_RXD1]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_RXD2]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_RXD3]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_TXCLK]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_TXEN]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_TXD0]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_TXD1]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_TXD2]
set_property IOSTANDARD LVCMOS18 [get_ports RGMII_TXD3]
set_property IOSTANDARD LVCMOS18 [get_ports MDC]
set_property IOSTANDARD LVCMOS18 [get_ports MDIO]
set_property IOSTANDARD LVCMOS18 [get_ports PHY_RESET_B]
create_clock -period 400.000 -name mdc_clk [get_ports MDC]
create_clock -period 5.000 -name CPU_CLK_IN [get_ports clk6_p]
create_clock -period 1000.000 -name TMCLK [get_ports clk8_p]
create_clock -period 20.000 -name DEBUG_CLK_IN [get_ports clk5_p]
create_clock -period 10.000 -name PCIE_CLK_IN [get_ports refclk_p]
create_clock -period 10.000 -name PCIE2_CLK_IN [get_ports refclk2_p]
create_clock -period 83.333 -name jtag_vclk [get_ports JTAG_TCK]
set_clock_groups -asynchronous -group [get_clocks jtag_vclk -include_generated_clocks]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks CPU_CLK_IN] -group [get_clocks -include_generated_clocks TMCLK]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks CPU_CLK_IN] -group [get_clocks -include_generated_clocks DEBUG_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks TMCLK] -group [get_clocks -include_generated_clocks DEBUG_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE_CLK_IN] -group [get_clocks -include_generated_clocks DEBUG_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE_CLK_IN] -group [get_clocks -include_generated_clocks TMCLK]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE_CLK_IN] -group [get_clocks -include_generated_clocks CPU_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks [list clk7_p mmcm_clkout0 mmcm_clkout1 mmcm_clkout2 mmcm_clkout3 mmcm_clkout4 mmcm_clkout5 mmcm_clkout6 {pll_clk[0]} {pll_clk[1]} {pll_clk[2]} {pll_clk[0]_DIV} {pll_clk[2]_DIV} {pll_clk[1]_DIV}]] -group [get_clocks -include_generated_clocks CPU_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks [list clk7_p mmcm_clkout0 mmcm_clkout1 mmcm_clkout2 mmcm_clkout3 mmcm_clkout4 mmcm_clkout5 mmcm_clkout6 {pll_clk[0]} {pll_clk[1]} {pll_clk[2]} {pll_clk[0]_DIV} {pll_clk[2]_DIV} {pll_clk[1]_DIV}]] -group [get_clocks -include_generated_clocks TMCLK]
set_clock_groups -asynchronous -group [get_clocks [list clk7_p mmcm_clkout0 mmcm_clkout1 mmcm_clkout2 mmcm_clkout3 mmcm_clkout4 mmcm_clkout5 mmcm_clkout6 {pll_clk[0]} {pll_clk[1]} {pll_clk[2]} {pll_clk[0]_DIV} {pll_clk[2]_DIV} {pll_clk[1]_DIV}]] -group [get_clocks -include_generated_clocks DEBUG_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks [list clk7_p mmcm_clkout0 mmcm_clkout1 mmcm_clkout2 mmcm_clkout3 mmcm_clkout4 mmcm_clkout5 mmcm_clkout6 {pll_clk[0]} {pll_clk[1]} {pll_clk[2]} {pll_clk[0]_DIV} {pll_clk[2]_DIV} {pll_clk[1]_DIV}]] -group [get_clocks -include_generated_clocks PCIE_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks jtag_vclk] -group [get_clocks -include_generated_clocks CPU_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks jtag_vclk] -group [get_clocks -include_generated_clocks TMCLK]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks jtag_vclk] -group [get_clocks -include_generated_clocks DEBUG_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks jtag_vclk] -group [get_clocks -include_generated_clocks PCIE_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks jtag_vclk] -group [get_clocks [list clk7_p mmcm_clkout0 mmcm_clkout1 mmcm_clkout2 mmcm_clkout3 mmcm_clkout4 mmcm_clkout5 mmcm_clkout6 {pll_clk[0]} {pll_clk[1]} {pll_clk[2]} {pll_clk[0]_DIV} {pll_clk[2]_DIV} {pll_clk[1]_DIV}]]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE2_CLK_IN] -group [get_clocks -include_generated_clocks jtag_vclk]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE2_CLK_IN] -group [get_clocks -include_generated_clocks CPU_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE2_CLK_IN] -group [get_clocks -include_generated_clocks TMCLK]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE2_CLK_IN] -group [get_clocks -include_generated_clocks DEBUG_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE2_CLK_IN] -group [get_clocks -include_generated_clocks PCIE_CLK_IN]
set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks PCIE2_CLK_IN] -group [get_clocks [list clk7_p mmcm_clkout0 mmcm_clkout1 mmcm_clkout2 mmcm_clkout3 mmcm_clkout4 mmcm_clkout5 mmcm_clkout6 {pll_clk[0]} {pll_clk[1]} {pll_clk[2]} {pll_clk[0]_DIV} {pll_clk[2]_DIV} {pll_clk[1]_DIV}]]
set_property PULLUP true [get_ports SD_CMD]
set_property PULLUP true [get_ports SD_DATA0]
set_property PULLUP true [get_ports SD_DATA1]
set_property PULLUP true [get_ports SD_DATA2]
set_property PULLUP true [get_ports SD_DATA3]
set_property PULLUP true [get_ports MDC]
set_property PULLUP true [get_ports MDIO]

####################################################################################
# Constraints from file : 'jtag_ddr_subsys_s01_data_fifo_0_clocks.xdc'
####################################################################################

