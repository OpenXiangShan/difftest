`include "DifftestMacros.v"
`include "gateway_interface.svh"

module XSTop_wrapper(
  input           sys_clk_i,    
  input           sys_rstn_i, 
  input           tmclk,
  input           osc_clock,        //24MHz
  input           outer_clock,      //Max : 2.4GHz
  input           global_reset,     //24MHz

  input  [3:0]    pll_bypass_sel,   //apb clk : 100MHz
  output          pll0_lock,
  output          pll0_clk_div_1024,
  output [11:0]   pll0_test_calout,

  input  [15:0]   soc_to_cpu,   // none
  output [15:0]   cpu_to_soc,   //none

  input  [63:0]   io_extIntrs,   // come from IPs, Max : 600MHz

  input  [15:0]   io_sram_config,  //apb clk : 100MHz

  input           io_systemjtag_jtag_TCK,         // come from gpio
  input           io_systemjtag_jtag_TMS,         // come from gpio
  input           io_systemjtag_jtag_TDI,         // come from gpio
  output          io_systemjtag_jtag_TDO_data,    // come from gpio
  output          io_systemjtag_jtag_TDO_driven,  // come from gpio
  input           io_systemjtag_reset,            // come from gpio

// peri bus   //400MHz
// dma bus    //800MHz
// mem bus    //800MHz

  (*mark_debug="true"*) output          dma_core_awready,
  (*mark_debug="true"*) input           dma_core_awvalid,
  (*mark_debug="true"*) input  [13:0]   dma_core_awid,
  (*mark_debug="true"*) input  [35:0]   dma_core_awaddr,
  (*mark_debug="true"*) input  [7:0]    dma_core_awlen,
  (*mark_debug="true"*) input  [2:0]    dma_core_awsize,
  (*mark_debug="true"*) input  [1:0]    dma_core_awburst,
  (*mark_debug="true"*) input           dma_core_awlock,
  (*mark_debug="true"*) input  [3:0]    dma_core_awcache,
  (*mark_debug="true"*) input  [2:0]    dma_core_awprot,
  (*mark_debug="true"*) input  [3:0]    dma_core_awqos,
  (*mark_debug="true"*) output          dma_core_wready,
  (*mark_debug="true"*) input           dma_core_wvalid,
  (*mark_debug="true"*) input  [255:0]  dma_core_wdata,
  (*mark_debug="true"*) input  [31:0]   dma_core_wstrb,
  (*mark_debug="true"*) input           dma_core_wlast,
  (*mark_debug="true"*) input           dma_core_bready,
  (*mark_debug="true"*) output          dma_core_bvalid,
  (*mark_debug="true"*) output [13:0]   dma_core_bid,
  (*mark_debug="true"*) output [1:0]    dma_core_bresp,
  (*mark_debug="true"*) output          dma_core_arready,
  (*mark_debug="true"*) input           dma_core_arvalid,
  (*mark_debug="true"*) input  [13:0]   dma_core_arid,
  (*mark_debug="true"*) input  [35:0]   dma_core_araddr,
  (*mark_debug="true"*) input  [7:0]    dma_core_arlen,
  (*mark_debug="true"*) input  [2:0]    dma_core_arsize,
  (*mark_debug="true"*) input  [1:0]    dma_core_arburst,
  (*mark_debug="true"*) input           dma_core_arlock,
  (*mark_debug="true"*) input  [3:0]    dma_core_arcache,
  (*mark_debug="true"*) input  [2:0]    dma_core_arprot,
  (*mark_debug="true"*) input  [3:0]    dma_core_arqos,
  (*mark_debug="true"*) input           dma_core_rready,
  (*mark_debug="true"*) output          dma_core_rvalid,
  (*mark_debug="true"*) output [13:0]   dma_core_rid,
  (*mark_debug="true"*) output [255:0]  dma_core_rdata,
  (*mark_debug="true"*) output [1:0]    dma_core_rresp,
  (*mark_debug="true"*) output          dma_core_rlast,
  
  (*mark_debug="true"*) input           peri_awready,
  (*mark_debug="true"*) output          peri_awvalid,
  (*mark_debug="true"*) output [1:0]    peri_awid,
  (*mark_debug="true"*) output [30:0]   peri_awaddr,
  (*mark_debug="true"*) output [7:0]    peri_awlen,
  (*mark_debug="true"*) output [2:0]    peri_awsize,
  (*mark_debug="true"*) output [1:0]    peri_awburst,
  (*mark_debug="true"*) output          peri_awlock,
  (*mark_debug="true"*) output [3:0]    peri_awcache,
  (*mark_debug="true"*) output [2:0]    peri_awprot,
  (*mark_debug="true"*) output [3:0]    peri_awqos,
  (*mark_debug="true"*) input           peri_wready,
  (*mark_debug="true"*) output          peri_wvalid,
  (*mark_debug="true"*) output [63:0]   peri_wdata,
  (*mark_debug="true"*) output [7:0]    peri_wstrb,
  (*mark_debug="true"*) output          peri_wlast,
  (*mark_debug="true"*) output          peri_bready,
  (*mark_debug="true"*) input           peri_bvalid,
  (*mark_debug="true"*) input  [1:0]    peri_bid,
  (*mark_debug="true"*) input  [1:0]    peri_bresp,
  (*mark_debug="true"*) input           peri_arready,
  (*mark_debug="true"*) output          peri_arvalid,
  (*mark_debug="true"*) output [1:0]    peri_arid,
  (*mark_debug="true"*) output [30:0]   peri_araddr,
  (*mark_debug="true"*) output [7:0]    peri_arlen,
  (*mark_debug="true"*) output [2:0]    peri_arsize,
  (*mark_debug="true"*) output [1:0]    peri_arburst,
  (*mark_debug="true"*) output          peri_arlock,
  (*mark_debug="true"*) output [3:0]    peri_arcache,
  (*mark_debug="true"*) output [2:0]    peri_arprot,
  (*mark_debug="true"*) output [3:0]    peri_arqos,
  (*mark_debug="true"*) output          peri_rready,
  (*mark_debug="true"*) input           peri_rvalid,
  (*mark_debug="true"*) input  [1:0]    peri_rid,
  (*mark_debug="true"*) input  [63:0]   peri_rdata,
  (*mark_debug="true"*) input  [1:0]    peri_rresp,
  (*mark_debug="true"*) input           peri_rlast,
  
  (*mark_debug="true"*) input           mem_core_awready,
  (*mark_debug="true"*) output          mem_core_awvalid,
  (*mark_debug="true"*) output [13:0]   mem_core_awid,
  (*mark_debug="true"*) output [35:0]   mem_core_awaddr,
  (*mark_debug="true"*) output [7:0]    mem_core_awlen,
  (*mark_debug="true"*) output [2:0]    mem_core_awsize,
  (*mark_debug="true"*) output [1:0]    mem_core_awburst,
  (*mark_debug="true"*) output          mem_core_awlock,
  (*mark_debug="true"*) output [3:0]    mem_core_awcache,
  (*mark_debug="true"*) output [2:0]    mem_core_awprot,
  (*mark_debug="true"*) output [3:0]    mem_core_awqos,
  (*mark_debug="true"*) input           mem_core_wready,
  (*mark_debug="true"*) output          mem_core_wvalid,
  (*mark_debug="true"*) output [255:0]  mem_core_wdata,
  (*mark_debug="true"*) output [31:0]   mem_core_wstrb,
  (*mark_debug="true"*) output          mem_core_wlast,
  (*mark_debug="true"*) output          mem_core_bready,
  (*mark_debug="true"*) input           mem_core_bvalid,
  (*mark_debug="true"*) input  [13:0]   mem_core_bid,
  (*mark_debug="true"*) input  [1:0]    mem_core_bresp,
  (*mark_debug="true"*) input           mem_core_arready,
  (*mark_debug="true"*) output          mem_core_arvalid,
  (*mark_debug="true"*) output [13:0]   mem_core_arid,
  (*mark_debug="true"*) output [35:0]   mem_core_araddr,
  (*mark_debug="true"*) output [7:0]    mem_core_arlen,
  (*mark_debug="true"*) output [2:0]    mem_core_arsize,
  (*mark_debug="true"*) output [1:0]    mem_core_arburst,
  (*mark_debug="true"*) output          mem_core_arlock,
  (*mark_debug="true"*) output [3:0]    mem_core_arcache,
  (*mark_debug="true"*) output [2:0]    mem_core_arprot,
  (*mark_debug="true"*) output [3:0]    mem_core_arqos,
  (*mark_debug="true"*) output          mem_core_rready,
  (*mark_debug="true"*) input           mem_core_rvalid,
  (*mark_debug="true"*) input  [13:0]   mem_core_rid,
  (*mark_debug="true"*) input  [255:0]  mem_core_rdata,
  (*mark_debug="true"*) input  [1:0]    mem_core_rresp,
  (*mark_debug="true"*) input           mem_core_rlast,
input [1:0] memory_0_rresp,
input memory_0_rlast,
input io_clock,
input io_reset,
input io_pll0_lock,
output [31:0] io_pll0_ctrl_0,
output [31:0] io_pll0_ctrl_1,
output [31:0] io_pll0_ctrl_2,
output [31:0] io_pll0_ctrl_3,
output [31:0] io_pll0_ctrl_4,
output [31:0] io_pll0_ctrl_5,
input [10:0] io_systemjtag_mfr_id,
input [15:0] io_systemjtag_part_number,
input [3:0] io_systemjtag_version,
output io_debug_reset,
input io_cacheable_check_req_0_valid,
input [35:0] io_cacheable_check_req_0_bits_addr,
input [1:0] io_cacheable_check_req_0_bits_size,
input [2:0] io_cacheable_check_req_0_bits_cmd,
input io_cacheable_check_req_1_bits_valid,
input [35:0] io_cacheable_check_req_1_bits_addr,
input [1:0] io_cacheable_check_req_1_bits_size,
input [2:0] io_cacheable_check_req_1_bits_cmd,
output io_cacheable_check_resp_0_1d,
output io_cacheable_check_resp_0_st,
output io_cacheable_check_resp_0_instr,
output io_cacheable_check_resp_0_mmio,
output io_cacheable_check_resp_1_1d,
output io_cacheable_check_resp_1_st,
output io_cacheable_check_resp_1_instr,
output io_cacheable_check_resp_1_mmio,
output io_riscv_halt_0,
output io_riscv_halt_1,

output [`CONFIG_DIFFTEST_BATCH_IO_WITDH - 1:0]gateway_out_data,
output gateway_out_enable
);

  wire          cpu_clock       ;
  wire          cpu_global_reset;
  wire          global_reset_sync;

  wire [31:0]   pll0_config_0;
  wire [31:0]   pll0_config_1;
  wire [31:0]   pll0_config_2;
  wire [31:0]   pll0_config_3;
  wire [31:0]   pll0_config_4;
  wire [31:0]   pll0_config_5;
//  wire [31:0]   pll0_config_5 = 32'h3;

assign cpu_to_soc = 32'h0;

  gateway_if   gateway_if_in();
  core_if      core_if_out[0]();

  CoreToGateway u_CoreToGateway(
    .gateway_out (gateway_if_in.out),
    .core_in     (core_if_out)
  );

  GatewayEndpoint u_GatewayEndpoint(
    .clock       (cpu_clk),
    .reset       (sys_rstn_i),

    .gateway_in  (gateway_if_in.in),

    .fpgaIO_data    (gateway_out_data),
    .fpgaIO_enable  (gateway_out_enable),
    .step        ()
  );

XSTop  u_XSTop(
  .memory_awready                (mem_core_awready )                        ,
  .memory_awvalid                (mem_core_awvalid )                        ,
  .memory_awid                   (mem_core_awid    )                          ,
  .memory_awaddr                 (mem_core_awaddr  )                            ,
  .memory_awlen                  (mem_core_awlen   )                           ,
  .memory_awsize                 (mem_core_awsize  )                            ,
  .memory_awburst                (mem_core_awburst )                             ,
  .memory_awlock                 (mem_core_awlock  )                            ,
  .memory_awcache                (mem_core_awcache )                             ,
  .memory_awprot                 (mem_core_awprot  )                            ,
  .memory_awqos                  (mem_core_awqos   )                           ,
  .memory_wready                 (mem_core_wready  )                       ,
  .memory_wvalid                 (mem_core_wvalid  )                       ,
  .memory_wdata                  (mem_core_wdata   )                           ,
  .memory_wstrb                  (mem_core_wstrb   )                           ,
  .memory_wlast                  (mem_core_wlast   )                           ,
  .memory_bready                 (mem_core_bready  )                       ,
  .memory_bvalid                 (mem_core_bvalid  )                       ,
  .memory_bid                    (mem_core_bid     )                         ,
  .memory_bresp                  (mem_core_bresp   )                           ,
  .memory_arready                (mem_core_arready )                        ,
  .memory_arvalid                (mem_core_arvalid )                        ,
  .memory_arid                   (mem_core_arid    )                          ,
  .memory_araddr                 (mem_core_araddr  )                            ,
  .memory_arlen                  (mem_core_arlen   )                           ,
  .memory_arsize                 (mem_core_arsize  )                            ,
  .memory_arburst                (mem_core_arburst )                             ,
  .memory_arlock                 (mem_core_arlock  )                            ,
  .memory_arcache                (mem_core_arcache )                             ,
  .memory_arprot                 (mem_core_arprot  )                            ,
  .memory_arqos                  (mem_core_arqos   )                           ,
  .memory_rready                 (mem_core_rready  )                       ,
  .memory_rvalid                 (mem_core_rvalid  )                       ,
  .memory_rid                    (mem_core_rid     )                         ,
  .memory_rdata                  (mem_core_rdata   )                           ,
  .memory_rresp                  (mem_core_rresp   )                           ,
  .memory_rlast                  (mem_core_rlast   )                           ,
  .peripheral_awready            (peri_awready  )                            ,
  .peripheral_awvalid            (peri_awvalid  )                            ,
  .peripheral_awid               (peri_awid     )                              ,
  .peripheral_awaddr             (peri_awaddr   )                                ,
  .peripheral_awlen              (peri_awlen    )                               ,
  .peripheral_awsize             (peri_awsize   )                                ,
  .peripheral_awburst            (peri_awburst  )                                 ,
  .peripheral_awlock             (peri_awlock   )                                ,
  .peripheral_awcache            (peri_awcache  )                                 ,
  .peripheral_awprot             (peri_awprot   )                                ,
  .peripheral_awqos              (peri_awqos    )                               ,
  .peripheral_wready             (peri_wready   )                           ,
  .peripheral_wvalid             (peri_wvalid   )                           ,
  .peripheral_wdata              (peri_wdata    )                               ,
  .peripheral_wstrb              (peri_wstrb    )                               ,
  .peripheral_wlast              (peri_wlast    )                               ,
  .peripheral_bready             (peri_bready   )                           ,
  .peripheral_bvalid             (peri_bvalid   )                           ,
  .peripheral_bid                (peri_bid      )                             ,
  .peripheral_bresp              (peri_bresp    )                               ,
  .peripheral_arready            (peri_arready  )                            ,
  .peripheral_arvalid            (peri_arvalid  )                            ,
  .peripheral_arid               (peri_arid     )                              ,
  .peripheral_araddr             (peri_araddr   )                                ,
  .peripheral_arlen              (peri_arlen    )                               ,
  .peripheral_arsize             (peri_arsize   )                                ,
  .peripheral_arburst            (peri_arburst  )                                 ,
  .peripheral_arlock             (peri_arlock   )                                ,
  .peripheral_arcache            (peri_arcache  )                                 ,
  .peripheral_arprot             (peri_arprot   )                                ,
  .peripheral_arqos              (peri_arqos    )                               ,
  .peripheral_rready             (peri_rready   )                           ,
  .peripheral_rvalid             (peri_rvalid   )                           ,
  .peripheral_rid                (peri_rid      )                             ,
  .peripheral_rdata              (peri_rdata    )                               ,
  .peripheral_rresp              (peri_rresp    )                               ,
  .peripheral_rlast              (peri_rlast    )                               ,
  .dma_awready                   (dma_core_awready )                     ,
  .dma_awvalid                   (dma_core_awvalid )                     ,
  .dma_awid                      (dma_core_awid    )                       ,
  .dma_awaddr                    (dma_core_awaddr[35:0]  )                         ,
  .dma_awlen                     (dma_core_awlen   )                        ,
  .dma_awsize                    (dma_core_awsize  )                         ,
  .dma_awburst                   (dma_core_awburst )                          ,
  .dma_awlock                    (dma_core_awlock  )                         ,
  .dma_awcache                   (dma_core_awcache )                          ,
  .dma_awprot                    (dma_core_awprot  )                         ,
  .dma_awqos                     (dma_core_awqos   )                        ,
  .dma_wready                    (dma_core_wready  )                    ,
  .dma_wvalid                    (dma_core_wvalid  )                    ,
  .dma_wdata                     (dma_core_wdata   )                        ,
  .dma_wstrb                     (dma_core_wstrb   )                        ,
  .dma_wlast                     (dma_core_wlast   )                        ,
  .dma_bready                    (dma_core_bready  )                    ,
  .dma_bvalid                    (dma_core_bvalid  )                    ,
  .dma_bid                       (dma_core_bid     )                      ,
  .dma_bresp                     (dma_core_bresp   )                        ,
  .dma_arready                   (dma_core_arready )                     ,
  .dma_arvalid                   (dma_core_arvalid )                     ,
  .dma_arid                      (dma_core_arid    )                       ,
  .dma_araddr                    (dma_core_araddr[35:0]  )                         ,
  .dma_arlen                     (dma_core_arlen   )                        ,
  .dma_arsize                    (dma_core_arsize  )                         ,
  .dma_arburst                   (dma_core_arburst )                          ,
  .dma_arlock                    (dma_core_arlock  )                         ,
  .dma_arcache                   (dma_core_arcache )                          ,
  .dma_arprot                    (dma_core_arprot  )                         ,
  .dma_arqos                     (dma_core_arqos   )                        ,
  .dma_rready                    (dma_core_rready  )                    ,
  .dma_rvalid                    (dma_core_rvalid  )                    ,
  .dma_rid                       (dma_core_rid     )                      ,
  .dma_rdata                     (dma_core_rdata   )                        ,
  .dma_rresp                     (dma_core_rresp   )                        ,
  .dma_rlast                     (dma_core_rlast   )                        ,
  
  .io_systemjtag_jtag_TCK          (io_systemjtag_jtag_TCK),
  .io_systemjtag_jtag_TMS          (io_systemjtag_jtag_TMS),
  .io_systemjtag_jtag_TDI          (io_systemjtag_jtag_TDI),
  .io_systemjtag_jtag_TDO_data     (io_systemjtag_jtag_TDO_data),
  .io_systemjtag_jtag_TDO_driven   (io_systemjtag_jtag_TDO_driven),
  .io_systemjtag_reset             (io_systemjtag_reset),
  .io_systemjtag_mfr_id            (11'h11),
  .io_systemjtag_part_number       (16'h16),
  .io_systemjtag_version           (4'h4),


  .io_clock                        (sys_clk_i/*cpu_clock  */                        ),
  .io_reset                        (~sys_rstn_i/*cpu_global_reset  */                 ),
  .io_extIntrs                     (io_extIntrs  ),
  .io_rtc_clock                    (tmclk),
  .io_riscv_rst_vec_0              (38'h10000000),

  .gateway_out                     (core_if_out[0]),
);


endmodule
