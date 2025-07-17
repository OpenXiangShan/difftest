
################################################################
# This is a generated script based on design: jtag_ddr_subsys
#
# Though there are limitations about the generated script,
# the main purpose of this utility is to make learning
# IP Integrator Tcl commands easier.
################################################################

namespace eval _tcl {
proc get_script_folder {} {
   set script_path [file normalize [info script]]
   set script_folder [file dirname $script_path]
   return $script_folder
}
}
variable script_folder
set script_folder [_tcl::get_script_folder]

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2020.2
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   catch {common::send_gid_msg -ssname BD::TCL -id 2041 -severity "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."}

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source jtag_ddr_subsys_script.tcl

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xcvu19p-fsva3824-2-e
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name jtag_ddr_subsys

# If you do not already have an existing IP Integrator design open,
# you can create a design using the following command:
#    create_bd_design $design_name

# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      common::send_gid_msg -ssname BD::TCL -id 2001 -severity "INFO" "Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   common::send_gid_msg -ssname BD::TCL -id 2002 -severity "INFO" "Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   common::send_gid_msg -ssname BD::TCL -id 2003 -severity "INFO" "Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   common::send_gid_msg -ssname BD::TCL -id 2004 -severity "INFO" "Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

common::send_gid_msg -ssname BD::TCL -id 2005 -severity "INFO" "Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   catch {common::send_gid_msg -ssname BD::TCL -id 2006 -severity "ERROR" $errMsg}
   return $nRet
}

set bCheckIPsPassed 1
##################################################################
# CHECK IPs
##################################################################
set bCheckIPs 1
if { $bCheckIPs == 1 } {
   set list_check_ips "\ 
xilinx.com:ip:ddr4:2.2\
xilinx.com:ip:jtag_axi:1.2\
xilinx.com:ip:ila:6.2\
xilinx.com:ip:util_vector_logic:2.0\
xilinx.com:ip:proc_sys_reset:5.0\
"

   set list_ips_missing ""
   common::send_gid_msg -ssname BD::TCL -id 2011 -severity "INFO" "Checking if the following IPs exist in the project's IP catalog: $list_check_ips ."

   foreach ip_vlnv $list_check_ips {
      set ip_obj [get_ipdefs -all $ip_vlnv]
      if { $ip_obj eq "" } {
         lappend list_ips_missing $ip_vlnv
      }
   }

   if { $list_ips_missing ne "" } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2012 -severity "ERROR" "The following IPs are not found in the IP Catalog:\n  $list_ips_missing\n\nResolution: Please add the repository containing the IP(s) to the project." }
      set bCheckIPsPassed 0
   }

}

if { $bCheckIPsPassed != 1 } {
  common::send_gid_msg -ssname BD::TCL -id 2023 -severity "WARNING" "Will not continue with creation of design due to the error(s) above."
  return 3
}

##################################################################
# DESIGN PROCs
##################################################################



# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  variable script_folder
  variable design_name

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2090 -severity "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2091 -severity "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set DDR4 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:ddr4_rtl:1.0 DDR4 ]

  set OSC_SYS_CLK [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 OSC_SYS_CLK ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {80000000} \
   ] $OSC_SYS_CLK

  set SOC_M_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 SOC_M_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {33} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {256} \
   CONFIG.FREQ_HZ {50000000} \
   CONFIG.HAS_BRESP {1} \
   CONFIG.HAS_BURST {1} \
   CONFIG.HAS_CACHE {1} \
   CONFIG.HAS_LOCK {1} \
   CONFIG.HAS_PROT {1} \
   CONFIG.HAS_QOS {1} \
   CONFIG.HAS_REGION {1} \
   CONFIG.HAS_RRESP {1} \
   CONFIG.HAS_WSTRB {1} \
   CONFIG.ID_WIDTH {18} \
   CONFIG.MAX_BURST_LENGTH {256} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {1} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $SOC_M_AXI


  # Create ports
  set MAC_CLK [ create_bd_port -dir O -type clk MAC_CLK ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {25000000} \
 ] $MAC_CLK
  set SOC_CLK [ create_bd_port -dir O -type clk SOC_CLK ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {SOC_M_AXI} \
   CONFIG.FREQ_HZ {50000000} \
 ] $SOC_CLK
  set SOC_RESETN [ create_bd_port -dir O -from 0 -to 0 -type rst SOC_RESETN ]
  set calib_complete [ create_bd_port -dir O calib_complete ]
  set ddr_rstn [ create_bd_port -dir I -type rst ddr_rstn ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_LOW} \
 ] $ddr_rstn
  set soc_rstn [ create_bd_port -dir I -type rst soc_rstn ]

  # Create instance: axi_interconnect_0, and set properties
  set axi_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0 ]
  set_property -dict [ list \
   CONFIG.ENABLE_ADVANCED_OPTIONS {1} \
   CONFIG.M00_HAS_DATA_FIFO {2} \
   CONFIG.M00_HAS_REGSLICE {3} \
   CONFIG.NUM_MI {1} \
   CONFIG.NUM_SI {2} \
   CONFIG.S00_HAS_DATA_FIFO {2} \
   CONFIG.S00_HAS_REGSLICE {4} \
   CONFIG.S01_HAS_DATA_FIFO {2} \
   CONFIG.S01_HAS_REGSLICE {4} \
   CONFIG.S02_HAS_DATA_FIFO {2} \
   CONFIG.STRATEGY {2} \
 ] $axi_interconnect_0

  # Create instance: ddr4_0, and set properties
  set ddr4_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ddr4:2.2 ddr4_0 ]
  set_property -dict [ list \
   CONFIG.ADDN_UI_CLKOUT1_FREQ_HZ {50} \
   CONFIG.ADDN_UI_CLKOUT2_FREQ_HZ {25} \
   CONFIG.ADDN_UI_CLKOUT3_FREQ_HZ {None} \
   CONFIG.ADDN_UI_CLKOUT4_FREQ_HZ {None} \
   CONFIG.C0.DDR4_AxiAddressWidth {33} \
   CONFIG.C0.DDR4_AxiDataWidth {64} \
   CONFIG.C0.DDR4_CLKFBOUT_MULT {15} \
   CONFIG.C0.DDR4_CLKOUT0_DIVIDE {6} \
   CONFIG.C0.DDR4_CasLatency {11} \
   CONFIG.C0.DDR4_CasWriteLatency {11} \
   CONFIG.C0.DDR4_DataWidth {64} \
   CONFIG.C0.DDR4_InputClockPeriod {12500} \
   CONFIG.C0.DDR4_MemoryPart {MTA8ATF1G64HZ-2G3} \
   CONFIG.C0.DDR4_MemoryType {SODIMMs} \
   CONFIG.C0.DDR4_TimePeriod {1250} \
 ] $ddr4_0

  # Create instance: jtag_axi_0, and set properties
  set jtag_axi_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:jtag_axi:1.2 jtag_axi_0 ]
  set_property -dict [ list \
   CONFIG.M_AXI_ID_WIDTH {1} \
   CONFIG.M_HAS_BURST {0} \
   CONFIG.RD_TXN_QUEUE_LENGTH {8} \
   CONFIG.WR_TXN_QUEUE_LENGTH {8} \
 ] $jtag_axi_0

  # Create instance: jtag_maxi_ila, and set properties
  set jtag_maxi_ila [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 jtag_maxi_ila ]
  set_property -dict [ list \
   CONFIG.ALL_PROBE_SAME_MU {false} \
   CONFIG.ALL_PROBE_SAME_MU_CNT {2} \
   CONFIG.C_DATA_DEPTH {2048} \
   CONFIG.C_EN_STRG_QUAL {0} \
   CONFIG.C_PROBE0_MU_CNT {2} \
   CONFIG.C_PROBE10_MU_CNT {2} \
   CONFIG.C_PROBE11_MU_CNT {2} \
   CONFIG.C_PROBE12_MU_CNT {2} \
   CONFIG.C_PROBE13_MU_CNT {2} \
   CONFIG.C_PROBE14_MU_CNT {2} \
   CONFIG.C_PROBE15_MU_CNT {2} \
   CONFIG.C_PROBE16_MU_CNT {2} \
   CONFIG.C_PROBE17_MU_CNT {2} \
   CONFIG.C_PROBE18_MU_CNT {2} \
   CONFIG.C_PROBE19_MU_CNT {2} \
   CONFIG.C_PROBE1_MU_CNT {2} \
   CONFIG.C_PROBE20_MU_CNT {2} \
   CONFIG.C_PROBE21_MU_CNT {2} \
   CONFIG.C_PROBE22_MU_CNT {2} \
   CONFIG.C_PROBE23_MU_CNT {2} \
   CONFIG.C_PROBE24_MU_CNT {2} \
   CONFIG.C_PROBE25_MU_CNT {2} \
   CONFIG.C_PROBE26_MU_CNT {2} \
   CONFIG.C_PROBE27_MU_CNT {2} \
   CONFIG.C_PROBE28_MU_CNT {2} \
   CONFIG.C_PROBE29_MU_CNT {2} \
   CONFIG.C_PROBE2_MU_CNT {2} \
   CONFIG.C_PROBE30_MU_CNT {2} \
   CONFIG.C_PROBE31_MU_CNT {2} \
   CONFIG.C_PROBE32_MU_CNT {2} \
   CONFIG.C_PROBE33_MU_CNT {2} \
   CONFIG.C_PROBE34_MU_CNT {2} \
   CONFIG.C_PROBE35_MU_CNT {2} \
   CONFIG.C_PROBE36_MU_CNT {2} \
   CONFIG.C_PROBE37_MU_CNT {2} \
   CONFIG.C_PROBE38_MU_CNT {2} \
   CONFIG.C_PROBE39_MU_CNT {2} \
   CONFIG.C_PROBE3_MU_CNT {2} \
   CONFIG.C_PROBE40_MU_CNT {2} \
   CONFIG.C_PROBE41_MU_CNT {2} \
   CONFIG.C_PROBE42_MU_CNT {2} \
   CONFIG.C_PROBE43_MU_CNT {2} \
   CONFIG.C_PROBE4_MU_CNT {2} \
   CONFIG.C_PROBE5_MU_CNT {2} \
   CONFIG.C_PROBE6_MU_CNT {2} \
   CONFIG.C_PROBE7_MU_CNT {2} \
   CONFIG.C_PROBE8_MU_CNT {2} \
   CONFIG.C_PROBE9_MU_CNT {2} \
 ] $jtag_maxi_ila

  # Create instance: logic_not, and set properties
  set logic_not [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 logic_not ]
  set_property -dict [ list \
   CONFIG.C_OPERATION {not} \
   CONFIG.C_SIZE {1} \
   CONFIG.LOGO_FILE {data/sym_notgate.png} \
 ] $logic_not

  # Create instance: logic_not1, and set properties
  set logic_not1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 logic_not1 ]
  set_property -dict [ list \
   CONFIG.C_OPERATION {not} \
   CONFIG.C_SIZE {1} \
   CONFIG.LOGO_FILE {data/sym_notgate.png} \
 ] $logic_not1

  # Create instance: rst_ddr4_200M, and set properties
  set rst_ddr4_200M [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_ddr4_200M ]

  # Create instance: rst_sys_50M, and set properties
  set rst_sys_50M [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_sys_50M ]
  set_property -dict [ list \
   CONFIG.C_EXT_RST_WIDTH {16} \
 ] $rst_sys_50M

  # Create interface connections
  connect_bd_intf_net -intf_net C0_SYS_CLK_0_1 [get_bd_intf_ports OSC_SYS_CLK] [get_bd_intf_pins ddr4_0/C0_SYS_CLK]
  connect_bd_intf_net -intf_net S00_AXI_1 [get_bd_intf_pins axi_interconnect_0/S00_AXI] [get_bd_intf_pins jtag_axi_0/M_AXI]
connect_bd_intf_net -intf_net [get_bd_intf_nets S00_AXI_1] [get_bd_intf_pins jtag_axi_0/M_AXI] [get_bd_intf_pins jtag_maxi_ila/SLOT_0_AXI]
  connect_bd_intf_net -intf_net SOC_M_AXI_1 [get_bd_intf_ports SOC_M_AXI] [get_bd_intf_pins axi_interconnect_0/S01_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M00_AXI [get_bd_intf_pins axi_interconnect_0/M00_AXI] [get_bd_intf_pins ddr4_0/C0_DDR4_S_AXI]
  connect_bd_intf_net -intf_net ddr4_0_C0_DDR4 [get_bd_intf_ports DDR4] [get_bd_intf_pins ddr4_0/C0_DDR4]

  # Create port connections
  connect_bd_net -net M00_ACLK_1 [get_bd_pins axi_interconnect_0/ACLK] [get_bd_pins axi_interconnect_0/M00_ACLK] [get_bd_pins axi_interconnect_0/S00_ACLK] [get_bd_pins ddr4_0/c0_ddr4_ui_clk] [get_bd_pins jtag_axi_0/aclk] [get_bd_pins jtag_maxi_ila/clk] [get_bd_pins rst_ddr4_200M/slowest_sync_clk]
  connect_bd_net -net ddr4_0_addn_ui_clkout1 [get_bd_ports SOC_CLK] [get_bd_pins axi_interconnect_0/S01_ACLK] [get_bd_pins ddr4_0/addn_ui_clkout1] [get_bd_pins rst_sys_50M/slowest_sync_clk]
  connect_bd_net -net ddr4_0_addn_ui_clkout2 [get_bd_ports MAC_CLK] [get_bd_pins ddr4_0/addn_ui_clkout2]
  connect_bd_net -net ddr4_0_c0_ddr4_ui_clk_sync_rst [get_bd_pins ddr4_0/c0_ddr4_ui_clk_sync_rst] [get_bd_pins rst_ddr4_200M/ext_reset_in]
  connect_bd_net -net ddr4_0_c0_init_calib_complete1 [get_bd_ports calib_complete] [get_bd_pins ddr4_0/c0_init_calib_complete] [get_bd_pins rst_ddr4_200M/dcm_locked] [get_bd_pins rst_sys_50M/dcm_locked]
  connect_bd_net -net ddr_rst_1 [get_bd_ports ddr_rstn] [get_bd_pins logic_not/Op1]
  connect_bd_net -net logic_not1_Res [get_bd_pins logic_not1/Res] [get_bd_pins rst_sys_50M/ext_reset_in]
  connect_bd_net -net rst_ddr4_0_200M_interconnect_aresetn [get_bd_pins axi_interconnect_0/ARESETN] [get_bd_pins axi_interconnect_0/M00_ARESETN] [get_bd_pins axi_interconnect_0/S00_ARESETN] [get_bd_pins rst_ddr4_200M/interconnect_aresetn]
  connect_bd_net -net rst_ddr4_0_200M_peripheral_aresetn [get_bd_pins ddr4_0/c0_ddr4_aresetn] [get_bd_pins jtag_axi_0/aresetn] [get_bd_pins rst_ddr4_200M/peripheral_aresetn]
  connect_bd_net -net rst_sys_50M_interconnect_aresetn [get_bd_pins axi_interconnect_0/S01_ARESETN] [get_bd_pins rst_sys_50M/interconnect_aresetn]
  connect_bd_net -net rst_sys_50M_peripheral_aresetn [get_bd_ports SOC_RESETN] [get_bd_pins rst_sys_50M/peripheral_aresetn]
  connect_bd_net -net soc_rstn_1 [get_bd_ports soc_rstn] [get_bd_pins logic_not1/Op1]
  connect_bd_net -net util_vector_logic_0_Res [get_bd_pins ddr4_0/sys_rst] [get_bd_pins logic_not/Res]

  # Create address segments
  assign_bd_address -offset 0x00000000 -range 0x000100000000 -target_address_space [get_bd_addr_spaces jtag_axi_0/Data] [get_bd_addr_segs ddr4_0/C0_DDR4_MEMORY_MAP/C0_DDR4_ADDRESS_BLOCK] -force
  assign_bd_address -offset 0x00000000 -range 0x000200000000 -target_address_space [get_bd_addr_spaces SOC_M_AXI] [get_bd_addr_segs ddr4_0/C0_DDR4_MEMORY_MAP/C0_DDR4_ADDRESS_BLOCK] -force


  # Restore current instance
  current_bd_instance $oldCurInst

  validate_bd_design
  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


