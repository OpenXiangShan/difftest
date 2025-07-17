//define the module need to be implement

`define XS_UART         // 11
`define XS_QSPI2ROM     // 10
`define XS_XMDA_EP
//`define XS_GMAC       // 18 cpu_int = {} xs_core_def(top_debug)

/* 
    ifdefine num:17 
    notice:(1) pcie0_m_arqos_mix 
           (2) nic400_cfg_sys_bridge 
           (3) U_DATA_CPU_BRIDGE 
           
           
    2023/3/1 modify:
           (1) cpu_int = {}
           (2) nic400_data_cpu_bridge U_DATA_CPU_BRIDGE -3012/3171
           
*/
//`define XS_XDMA       







