##################################################################
# CHECK VIVADO VERSION
##################################################################

set scripts_vivado_version 2020.2
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
  catch {common::send_msg_id "IPS_TCL-100" "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_ip_tcl to create an updated script."}
  return 1
}

##################################################################
# START
##################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source pcie4c_uscale_plus_0.tcl
# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./pcie4c_uscale_plus_0_ex/pcie4c_uscale_plus_0_ex.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
  create_project pcie4c_uscale_plus_0_ex pcie4c_uscale_plus_0_ex -part xcvu19p-fsva3824-2-e
  set_property target_language Verilog [current_project]
  set_property simulator_language Mixed [current_project]
}

##################################################################
# CHECK IPs
##################################################################

set bCheckIPs 1
set bCheckIPsPassed 1
if { $bCheckIPs == 1 } {
  set list_check_ips { xilinx.com:ip:pcie4c_uscale_plus:1.0 }
  set list_ips_missing ""
  common::send_msg_id "IPS_TCL-1001" "INFO" "Checking if the following IPs exist in the project's IP catalog: $list_check_ips ."

  foreach ip_vlnv $list_check_ips {
  set ip_obj [get_ipdefs -all $ip_vlnv]
  if { $ip_obj eq "" } {
    lappend list_ips_missing $ip_vlnv
    }
  }

  if { $list_ips_missing ne "" } {
    catch {common::send_msg_id "IPS_TCL-105" "ERROR" "The following IPs are not found in the IP Catalog:\n  $list_ips_missing\n\nResolution: Please add the repository containing the IP(s) to the project." }
    set bCheckIPsPassed 0
  }
}

if { $bCheckIPsPassed != 1 } {
  common::send_msg_id "IPS_TCL-102" "WARNING" "Will not continue with creation of design due to the error(s) above."
  return 1
}

##################################################################
# CREATE IP pcie4c_uscale_plus_0
##################################################################

set pcie4c_uscale_plus_0 [create_ip -name pcie4c_uscale_plus -vendor xilinx.com -library ip -version 1.0 -module_name pcie4c_uscale_plus_0]

set_property -dict { 
  CONFIG.device_port_type {Root_Port_of_PCI_Express_Root_Complex}
  CONFIG.PL_LINK_CAP_MAX_LINK_SPEED {5.0_GT/s}
  CONFIG.PL_LINK_CAP_MAX_LINK_WIDTH {X4}
  CONFIG.AXISTEN_IF_RC_STRADDLE {false}
  CONFIG.PF0_CLASS_CODE {060A00}
  CONFIG.PF0_DEVICE_ID {9124}
  CONFIG.PF1_CLASS_CODE {060A00}
  CONFIG.PF2_CLASS_CODE {060A00}
  CONFIG.PF2_DEVICE_ID {9524}
  CONFIG.PF3_CLASS_CODE {060A00}
  CONFIG.PF3_DEVICE_ID {9724}
  CONFIG.pf0_class_code_sub {0A}
  CONFIG.pcie_blk_locn {X0Y3}
  CONFIG.pf1_class_code_base {06}
  CONFIG.pf2_class_code_base {06}
  CONFIG.pf3_class_code_base {06}
  CONFIG.pf0_base_class_menu {Bridge_device}
  CONFIG.pf0_sub_class_interface_menu {InfiniBand_to_PCI_host_bridge}
  CONFIG.pf1_base_class_menu {Bridge_device}
  CONFIG.pf1_sub_class_interface_menu {InfiniBand_to_PCI_host_bridge}
  CONFIG.pf1_class_code_sub {0A}
  CONFIG.pf0_class_code_base {06}
  CONFIG.axisten_if_width {128_bit}
  CONFIG.pf2_base_class_menu {Bridge_device}
  CONFIG.pf2_sub_class_interface_menu {InfiniBand_to_PCI_host_bridge}
  CONFIG.pf2_class_code_sub {0A}
  CONFIG.pf3_base_class_menu {Bridge_device}
  CONFIG.pf3_sub_class_interface_menu {InfiniBand_to_PCI_host_bridge}
  CONFIG.pf3_class_code_sub {0A}
  CONFIG.mode_selection {Basic}
  CONFIG.coreclk_freq {125}
  CONFIG.plltype {QPLL1}
  CONFIG.axisten_freq {125}
  CONFIG.select_quad {GTY_Quad_227}
  CONFIG.PL_DISABLE_LANE_REVERSAL {false}
  CONFIG.X1_CH_EN {X0Y35}
  CONFIG.X2_CH_EN {X0Y35 X0Y34}
  CONFIG.X4_CH_EN {X0Y35 X0Y34 X0Y33 X0Y32}
  CONFIG.X8_CH_EN {X0Y35 X0Y34 X0Y33 X0Y32 X0Y31 X0Y30 X0Y29 X0Y28}
  CONFIG.TX_RX_MASTER_CHANNEL {X0Y35}
  CONFIG.axisten_if_enable_msg_route {2FFFF}
} [get_ips pcie4c_uscale_plus_0]

set_property -dict { 
  GENERATE_SYNTH_CHECKPOINT {1}
} $pcie4c_uscale_plus_0

##################################################################

