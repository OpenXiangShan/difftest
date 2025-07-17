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
# source blk_mem_gen_0.tcl
# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./xs_v3/xs_v3.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
  create_project xs_v3 xs_v3 -part xcvu19p-fsva3824-2-e
  set_property target_language Verilog [current_project]
  set_property simulator_language Mixed [current_project]
}

##################################################################
# CHECK IPs
##################################################################

set bCheckIPs 1
set bCheckIPsPassed 1
if { $bCheckIPs == 1 } {
  set list_check_ips { xilinx.com:ip:blk_mem_gen:8.4 }
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
# blk_mem_gen_0 FILES
##################################################################

proc write_blk_mem_gen_qspi { blk_mem_gen_qspi_filepath } {
  set blk_mem_gen_qspi [open $blk_mem_gen_qspi_filepath  w+]

  puts $blk_mem_gen_qspi {MEMORY_INITIALIZATION_RADIX=16;}
  puts $blk_mem_gen_qspi {MEMORY_INITIALIZATION_VECTOR=}
  puts $blk_mem_gen_qspi {0010029b,}
  puts $blk_mem_gen_qspi {01f29293,}
  puts $blk_mem_gen_qspi {00028067;}

  flush $blk_mem_gen_qspi
  close $blk_mem_gen_qspi
}

##################################################################
# CREATE IP blk_mem_gen_0
##################################################################

set blk_mem_gen_0 [create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name blk_mem_gen_0]

write_blk_mem_gen_qspi  [file join [get_property IP_DIR [get_ips blk_mem_gen_0]] qspi.coe]
set_property -dict { 
  CONFIG.Interface_Type {AXI4}
  CONFIG.AXI_Slave_Type {Peripheral_Slave}
  CONFIG.Use_AXI_ID {false}
  CONFIG.Memory_Type {Simple_Dual_Port_RAM}
  CONFIG.RD_ADDR_CHNG_A {false}
  CONFIG.RD_ADDR_CHNG_B {false}
  CONFIG.Use_Byte_Write_Enable {true}
  CONFIG.Byte_Size {8}
  CONFIG.Assume_Synchronous_Clk {true}
  CONFIG.Write_Width_A {32}
  CONFIG.Write_Depth_A {1024}
  CONFIG.Read_Width_A {32}
  CONFIG.Operating_Mode_A {READ_FIRST}
  CONFIG.Write_Width_B {32}
  CONFIG.Read_Width_B {32}
  CONFIG.Operating_Mode_B {READ_FIRST}
  CONFIG.Enable_B {Use_ENB_Pin}
  CONFIG.Register_PortA_Output_of_Memory_Primitives {false}
  CONFIG.Load_Init_File {true}
  CONFIG.Coe_File {qspi.coe}
  CONFIG.Fill_Remaining_Memory_Locations {true}
  CONFIG.Use_RSTB_Pin {true}
  CONFIG.Reset_Type {ASYNC}
  CONFIG.Port_B_Clock {100}
  CONFIG.Port_B_Enable_Rate {100}
  CONFIG.EN_SAFETY_CKT {true}
} [get_ips blk_mem_gen_0]

set_property -dict { 
  GENERATE_SYNTH_CHECKPOINT {0}
} $blk_mem_gen_0

##################################################################

