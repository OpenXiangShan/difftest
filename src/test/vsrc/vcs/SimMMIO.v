module SimMMIO(
  input          clock,
  input          reset,
  output         io_axi4_0_awready,
  input          io_axi4_0_awvalid,
  input  [4:0]   io_axi4_0_awid,
  input  [37:0]  io_axi4_0_awaddr,
  input  [7:0]   io_axi4_0_awlen,
  input  [2:0]   io_axi4_0_awsize,
  input  [3:0]   io_axi4_0_awcache,
  input  [2:0]   io_axi4_0_awprot,
  output         io_axi4_0_wready,
  input          io_axi4_0_wvalid,
  input  [255:0] io_axi4_0_wdata,
  input  [31:0]  io_axi4_0_wstrb,
  input          io_axi4_0_wlast,
  input          io_axi4_0_bready,
  output         io_axi4_0_bvalid,
  output [4:0]   io_axi4_0_bid,
  output [1:0]   io_axi4_0_bresp,
  output         io_axi4_0_arready,
  input          io_axi4_0_arvalid,
  input  [4:0]   io_axi4_0_arid,
  input  [37:0]  io_axi4_0_araddr,
  input  [7:0]   io_axi4_0_arlen,
  input  [2:0]   io_axi4_0_arsize,
  input  [3:0]   io_axi4_0_arcache,
  input  [2:0]   io_axi4_0_arprot,
  input          io_axi4_0_rready,
  output         io_axi4_0_rvalid,
  output [4:0]   io_axi4_0_rid,
  output [255:0] io_axi4_0_rdata,
  output [1:0]   io_axi4_0_rresp,
  output         io_axi4_0_rlast,
  input          io_dma_0_awready,
  output         io_dma_0_awvalid,
  output [7:0]   io_dma_0_awid,
  output [37:0]  io_dma_0_awaddr,
  input          io_dma_0_wready,
  output         io_dma_0_wvalid,
  output [255:0] io_dma_0_wdata,
  output [31:0]  io_dma_0_wstrb,
  output         io_dma_0_wlast,
  input          io_dma_0_bvalid,
  input  [7:0]   io_dma_0_bid,
  input          io_dma_0_arready,
  output         io_dma_0_arvalid,
  output [7:0]   io_dma_0_arid,
  output [37:0]  io_dma_0_araddr,
  input          io_dma_0_rvalid,
  input  [7:0]   io_dma_0_rid,
  input  [255:0] io_dma_0_rdata,
  output         io_uart_out_valid,
  output [7:0]   io_uart_out_ch,
  output         io_uart_in_valid,
  input  [7:0]   io_uart_in_ch,
  output [63:0]  io_interrupt_intrVec
);
  wire  bootrom0_clock; // @[SimMMIO.scala 34:28]
  wire  bootrom0_reset; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_awready; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_awvalid; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_awid; // @[SimMMIO.scala 34:28]
  wire [36:0] bootrom0_auto_in_awaddr; // @[SimMMIO.scala 34:28]
  wire [7:0] bootrom0_auto_in_awlen; // @[SimMMIO.scala 34:28]
  wire [2:0] bootrom0_auto_in_awsize; // @[SimMMIO.scala 34:28]
  wire [1:0] bootrom0_auto_in_awburst; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_awlock; // @[SimMMIO.scala 34:28]
  wire [3:0] bootrom0_auto_in_awcache; // @[SimMMIO.scala 34:28]
  wire [2:0] bootrom0_auto_in_awprot; // @[SimMMIO.scala 34:28]
  wire [3:0] bootrom0_auto_in_awqos; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_wready; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_wvalid; // @[SimMMIO.scala 34:28]
  wire [63:0] bootrom0_auto_in_wdata; // @[SimMMIO.scala 34:28]
  wire [7:0] bootrom0_auto_in_wstrb; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_wlast; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_bready; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_bvalid; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_bid; // @[SimMMIO.scala 34:28]
  wire [1:0] bootrom0_auto_in_bresp; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_arready; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_arvalid; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_arid; // @[SimMMIO.scala 34:28]
  wire [36:0] bootrom0_auto_in_araddr; // @[SimMMIO.scala 34:28]
  wire [7:0] bootrom0_auto_in_arlen; // @[SimMMIO.scala 34:28]
  wire [2:0] bootrom0_auto_in_arsize; // @[SimMMIO.scala 34:28]
  wire [1:0] bootrom0_auto_in_arburst; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_arlock; // @[SimMMIO.scala 34:28]
  wire [3:0] bootrom0_auto_in_arcache; // @[SimMMIO.scala 34:28]
  wire [2:0] bootrom0_auto_in_arprot; // @[SimMMIO.scala 34:28]
  wire [3:0] bootrom0_auto_in_arqos; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_rready; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_rvalid; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_rid; // @[SimMMIO.scala 34:28]
  wire [63:0] bootrom0_auto_in_rdata; // @[SimMMIO.scala 34:28]
  wire [1:0] bootrom0_auto_in_rresp; // @[SimMMIO.scala 34:28]
  wire  bootrom0_auto_in_rlast; // @[SimMMIO.scala 34:28]
  wire  bootrom1_clock; // @[SimMMIO.scala 35:28]
  wire  bootrom1_reset; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_awready; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_awvalid; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_awid; // @[SimMMIO.scala 35:28]
  wire [36:0] bootrom1_auto_in_awaddr; // @[SimMMIO.scala 35:28]
  wire [7:0] bootrom1_auto_in_awlen; // @[SimMMIO.scala 35:28]
  wire [2:0] bootrom1_auto_in_awsize; // @[SimMMIO.scala 35:28]
  wire [1:0] bootrom1_auto_in_awburst; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_awlock; // @[SimMMIO.scala 35:28]
  wire [3:0] bootrom1_auto_in_awcache; // @[SimMMIO.scala 35:28]
  wire [2:0] bootrom1_auto_in_awprot; // @[SimMMIO.scala 35:28]
  wire [3:0] bootrom1_auto_in_awqos; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_wready; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_wvalid; // @[SimMMIO.scala 35:28]
  wire [63:0] bootrom1_auto_in_wdata; // @[SimMMIO.scala 35:28]
  wire [7:0] bootrom1_auto_in_wstrb; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_wlast; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_bready; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_bvalid; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_bid; // @[SimMMIO.scala 35:28]
  wire [1:0] bootrom1_auto_in_bresp; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_arready; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_arvalid; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_arid; // @[SimMMIO.scala 35:28]
  wire [36:0] bootrom1_auto_in_araddr; // @[SimMMIO.scala 35:28]
  wire [7:0] bootrom1_auto_in_arlen; // @[SimMMIO.scala 35:28]
  wire [2:0] bootrom1_auto_in_arsize; // @[SimMMIO.scala 35:28]
  wire [1:0] bootrom1_auto_in_arburst; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_arlock; // @[SimMMIO.scala 35:28]
  wire [3:0] bootrom1_auto_in_arcache; // @[SimMMIO.scala 35:28]
  wire [2:0] bootrom1_auto_in_arprot; // @[SimMMIO.scala 35:28]
  wire [3:0] bootrom1_auto_in_arqos; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_rready; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_rvalid; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_rid; // @[SimMMIO.scala 35:28]
  wire [63:0] bootrom1_auto_in_rdata; // @[SimMMIO.scala 35:28]
  wire [1:0] bootrom1_auto_in_rresp; // @[SimMMIO.scala 35:28]
  wire  bootrom1_auto_in_rlast; // @[SimMMIO.scala 35:28]
  wire  flash_clock; // @[SimMMIO.scala 36:25]
  wire  flash_reset; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_awready; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_awvalid; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_awid; // @[SimMMIO.scala 36:25]
  wire [36:0] flash_auto_in_awaddr; // @[SimMMIO.scala 36:25]
  wire [7:0] flash_auto_in_awlen; // @[SimMMIO.scala 36:25]
  wire [2:0] flash_auto_in_awsize; // @[SimMMIO.scala 36:25]
  wire [1:0] flash_auto_in_awburst; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_awlock; // @[SimMMIO.scala 36:25]
  wire [3:0] flash_auto_in_awcache; // @[SimMMIO.scala 36:25]
  wire [2:0] flash_auto_in_awprot; // @[SimMMIO.scala 36:25]
  wire [3:0] flash_auto_in_awqos; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_wready; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_wvalid; // @[SimMMIO.scala 36:25]
  wire [63:0] flash_auto_in_wdata; // @[SimMMIO.scala 36:25]
  wire [7:0] flash_auto_in_wstrb; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_wlast; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_bready; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_bvalid; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_bid; // @[SimMMIO.scala 36:25]
  wire [1:0] flash_auto_in_bresp; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_arready; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_arvalid; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_arid; // @[SimMMIO.scala 36:25]
  wire [36:0] flash_auto_in_araddr; // @[SimMMIO.scala 36:25]
  wire [7:0] flash_auto_in_arlen; // @[SimMMIO.scala 36:25]
  wire [2:0] flash_auto_in_arsize; // @[SimMMIO.scala 36:25]
  wire [1:0] flash_auto_in_arburst; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_arlock; // @[SimMMIO.scala 36:25]
  wire [3:0] flash_auto_in_arcache; // @[SimMMIO.scala 36:25]
  wire [2:0] flash_auto_in_arprot; // @[SimMMIO.scala 36:25]
  wire [3:0] flash_auto_in_arqos; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_rready; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_rvalid; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_rid; // @[SimMMIO.scala 36:25]
  wire [63:0] flash_auto_in_rdata; // @[SimMMIO.scala 36:25]
  wire [1:0] flash_auto_in_rresp; // @[SimMMIO.scala 36:25]
  wire  flash_auto_in_rlast; // @[SimMMIO.scala 36:25]
  wire  uart_clock; // @[SimMMIO.scala 37:24]
  wire  uart_reset; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_awready; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_awvalid; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_awid; // @[SimMMIO.scala 37:24]
  wire [36:0] uart_auto_in_awaddr; // @[SimMMIO.scala 37:24]
  wire [7:0] uart_auto_in_awlen; // @[SimMMIO.scala 37:24]
  wire [2:0] uart_auto_in_awsize; // @[SimMMIO.scala 37:24]
  wire [1:0] uart_auto_in_awburst; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_awlock; // @[SimMMIO.scala 37:24]
  wire [3:0] uart_auto_in_awcache; // @[SimMMIO.scala 37:24]
  wire [2:0] uart_auto_in_awprot; // @[SimMMIO.scala 37:24]
  wire [3:0] uart_auto_in_awqos; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_wready; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_wvalid; // @[SimMMIO.scala 37:24]
  wire [63:0] uart_auto_in_wdata; // @[SimMMIO.scala 37:24]
  wire [7:0] uart_auto_in_wstrb; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_wlast; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_bready; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_bvalid; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_bid; // @[SimMMIO.scala 37:24]
  wire [1:0] uart_auto_in_bresp; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_arready; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_arvalid; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_arid; // @[SimMMIO.scala 37:24]
  wire [36:0] uart_auto_in_araddr; // @[SimMMIO.scala 37:24]
  wire [7:0] uart_auto_in_arlen; // @[SimMMIO.scala 37:24]
  wire [2:0] uart_auto_in_arsize; // @[SimMMIO.scala 37:24]
  wire [1:0] uart_auto_in_arburst; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_arlock; // @[SimMMIO.scala 37:24]
  wire [3:0] uart_auto_in_arcache; // @[SimMMIO.scala 37:24]
  wire [2:0] uart_auto_in_arprot; // @[SimMMIO.scala 37:24]
  wire [3:0] uart_auto_in_arqos; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_rready; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_rvalid; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_rid; // @[SimMMIO.scala 37:24]
  wire [63:0] uart_auto_in_rdata; // @[SimMMIO.scala 37:24]
  wire [1:0] uart_auto_in_rresp; // @[SimMMIO.scala 37:24]
  wire  uart_auto_in_rlast; // @[SimMMIO.scala 37:24]
  wire  uart_io_extra_out_valid; // @[SimMMIO.scala 37:24]
  wire [7:0] uart_io_extra_out_ch; // @[SimMMIO.scala 37:24]
  wire  uart_io_extra_in_valid; // @[SimMMIO.scala 37:24]
  wire [7:0] uart_io_extra_in_ch; // @[SimMMIO.scala 37:24]
  wire  vga_clock; // @[SimMMIO.scala 38:23]
  wire  vga_reset; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_awready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_awvalid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_awid; // @[SimMMIO.scala 38:23]
  wire [36:0] vga_auto_in_1_awaddr; // @[SimMMIO.scala 38:23]
  wire [7:0] vga_auto_in_1_awlen; // @[SimMMIO.scala 38:23]
  wire [2:0] vga_auto_in_1_awsize; // @[SimMMIO.scala 38:23]
  wire [1:0] vga_auto_in_1_awburst; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_awlock; // @[SimMMIO.scala 38:23]
  wire [3:0] vga_auto_in_1_awcache; // @[SimMMIO.scala 38:23]
  wire [2:0] vga_auto_in_1_awprot; // @[SimMMIO.scala 38:23]
  wire [3:0] vga_auto_in_1_awqos; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_wready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_wvalid; // @[SimMMIO.scala 38:23]
  wire [63:0] vga_auto_in_1_wdata; // @[SimMMIO.scala 38:23]
  wire [7:0] vga_auto_in_1_wstrb; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_wlast; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_bready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_bvalid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_bid; // @[SimMMIO.scala 38:23]
  wire [1:0] vga_auto_in_1_bresp; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_arready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_arvalid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_arid; // @[SimMMIO.scala 38:23]
  wire [36:0] vga_auto_in_1_araddr; // @[SimMMIO.scala 38:23]
  wire [7:0] vga_auto_in_1_arlen; // @[SimMMIO.scala 38:23]
  wire [2:0] vga_auto_in_1_arsize; // @[SimMMIO.scala 38:23]
  wire [1:0] vga_auto_in_1_arburst; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_arlock; // @[SimMMIO.scala 38:23]
  wire [3:0] vga_auto_in_1_arcache; // @[SimMMIO.scala 38:23]
  wire [2:0] vga_auto_in_1_arprot; // @[SimMMIO.scala 38:23]
  wire [3:0] vga_auto_in_1_arqos; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_rready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_rvalid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_rid; // @[SimMMIO.scala 38:23]
  wire [63:0] vga_auto_in_1_rdata; // @[SimMMIO.scala 38:23]
  wire [1:0] vga_auto_in_1_rresp; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_1_rlast; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_awready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_awvalid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_awid; // @[SimMMIO.scala 38:23]
  wire [36:0] vga_auto_in_0_awaddr; // @[SimMMIO.scala 38:23]
  wire [7:0] vga_auto_in_0_awlen; // @[SimMMIO.scala 38:23]
  wire [2:0] vga_auto_in_0_awsize; // @[SimMMIO.scala 38:23]
  wire [1:0] vga_auto_in_0_awburst; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_awlock; // @[SimMMIO.scala 38:23]
  wire [3:0] vga_auto_in_0_awcache; // @[SimMMIO.scala 38:23]
  wire [2:0] vga_auto_in_0_awprot; // @[SimMMIO.scala 38:23]
  wire [3:0] vga_auto_in_0_awqos; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_wready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_wvalid; // @[SimMMIO.scala 38:23]
  wire [63:0] vga_auto_in_0_wdata; // @[SimMMIO.scala 38:23]
  wire [7:0] vga_auto_in_0_wstrb; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_wlast; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_bready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_bvalid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_bid; // @[SimMMIO.scala 38:23]
  wire [1:0] vga_auto_in_0_bresp; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_arvalid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_arid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_arlock; // @[SimMMIO.scala 38:23]
  wire [3:0] vga_auto_in_0_arcache; // @[SimMMIO.scala 38:23]
  wire [3:0] vga_auto_in_0_arqos; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_rready; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_rvalid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_rid; // @[SimMMIO.scala 38:23]
  wire  vga_auto_in_0_rlast; // @[SimMMIO.scala 38:23]
  wire  sd_clock; // @[SimMMIO.scala 43:22]
  wire  sd_reset; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_awready; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_awvalid; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_awid; // @[SimMMIO.scala 43:22]
  wire [36:0] sd_auto_in_awaddr; // @[SimMMIO.scala 43:22]
  wire [7:0] sd_auto_in_awlen; // @[SimMMIO.scala 43:22]
  wire [2:0] sd_auto_in_awsize; // @[SimMMIO.scala 43:22]
  wire [1:0] sd_auto_in_awburst; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_awlock; // @[SimMMIO.scala 43:22]
  wire [3:0] sd_auto_in_awcache; // @[SimMMIO.scala 43:22]
  wire [2:0] sd_auto_in_awprot; // @[SimMMIO.scala 43:22]
  wire [3:0] sd_auto_in_awqos; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_wready; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_wvalid; // @[SimMMIO.scala 43:22]
  wire [63:0] sd_auto_in_wdata; // @[SimMMIO.scala 43:22]
  wire [7:0] sd_auto_in_wstrb; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_wlast; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_bready; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_bvalid; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_bid; // @[SimMMIO.scala 43:22]
  wire [1:0] sd_auto_in_bresp; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_arready; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_arvalid; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_arid; // @[SimMMIO.scala 43:22]
  wire [36:0] sd_auto_in_araddr; // @[SimMMIO.scala 43:22]
  wire [7:0] sd_auto_in_arlen; // @[SimMMIO.scala 43:22]
  wire [2:0] sd_auto_in_arsize; // @[SimMMIO.scala 43:22]
  wire [1:0] sd_auto_in_arburst; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_arlock; // @[SimMMIO.scala 43:22]
  wire [3:0] sd_auto_in_arcache; // @[SimMMIO.scala 43:22]
  wire [2:0] sd_auto_in_arprot; // @[SimMMIO.scala 43:22]
  wire [3:0] sd_auto_in_arqos; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_rready; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_rvalid; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_rid; // @[SimMMIO.scala 43:22]
  wire [63:0] sd_auto_in_rdata; // @[SimMMIO.scala 43:22]
  wire [1:0] sd_auto_in_rresp; // @[SimMMIO.scala 43:22]
  wire  sd_auto_in_rlast; // @[SimMMIO.scala 43:22]
  wire  intrGen_clock; // @[SimMMIO.scala 44:27]
  wire  intrGen_reset; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_awready; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_awvalid; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_awid; // @[SimMMIO.scala 44:27]
  wire [36:0] intrGen_auto_in_awaddr; // @[SimMMIO.scala 44:27]
  wire [7:0] intrGen_auto_in_awlen; // @[SimMMIO.scala 44:27]
  wire [2:0] intrGen_auto_in_awsize; // @[SimMMIO.scala 44:27]
  wire [1:0] intrGen_auto_in_awburst; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_awlock; // @[SimMMIO.scala 44:27]
  wire [3:0] intrGen_auto_in_awcache; // @[SimMMIO.scala 44:27]
  wire [2:0] intrGen_auto_in_awprot; // @[SimMMIO.scala 44:27]
  wire [3:0] intrGen_auto_in_awqos; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_wready; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_wvalid; // @[SimMMIO.scala 44:27]
  wire [63:0] intrGen_auto_in_wdata; // @[SimMMIO.scala 44:27]
  wire [7:0] intrGen_auto_in_wstrb; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_wlast; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_bready; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_bvalid; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_bid; // @[SimMMIO.scala 44:27]
  wire [1:0] intrGen_auto_in_bresp; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_arready; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_arvalid; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_arid; // @[SimMMIO.scala 44:27]
  wire [36:0] intrGen_auto_in_araddr; // @[SimMMIO.scala 44:27]
  wire [7:0] intrGen_auto_in_arlen; // @[SimMMIO.scala 44:27]
  wire [2:0] intrGen_auto_in_arsize; // @[SimMMIO.scala 44:27]
  wire [1:0] intrGen_auto_in_arburst; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_arlock; // @[SimMMIO.scala 44:27]
  wire [3:0] intrGen_auto_in_arcache; // @[SimMMIO.scala 44:27]
  wire [2:0] intrGen_auto_in_arprot; // @[SimMMIO.scala 44:27]
  wire [3:0] intrGen_auto_in_arqos; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_rready; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_rvalid; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_rid; // @[SimMMIO.scala 44:27]
  wire [63:0] intrGen_auto_in_rdata; // @[SimMMIO.scala 44:27]
  wire [1:0] intrGen_auto_in_rresp; // @[SimMMIO.scala 44:27]
  wire  intrGen_auto_in_rlast; // @[SimMMIO.scala 44:27]
  wire [63:0] intrGen_io_extra_intrVec; // @[SimMMIO.scala 44:27]
  wire  dmaGen_clock; // @[SimMMIO.scala 45:26]
  wire  dmaGen_reset; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_awready; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_awvalid; // @[SimMMIO.scala 45:26]
  wire [7:0] dmaGen_auto_dma_out_awid; // @[SimMMIO.scala 45:26]
  wire [37:0] dmaGen_auto_dma_out_awaddr; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_wready; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_wvalid; // @[SimMMIO.scala 45:26]
  wire [255:0] dmaGen_auto_dma_out_wdata; // @[SimMMIO.scala 45:26]
  wire [31:0] dmaGen_auto_dma_out_wstrb; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_wlast; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_bvalid; // @[SimMMIO.scala 45:26]
  wire [7:0] dmaGen_auto_dma_out_bid; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_arready; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_arvalid; // @[SimMMIO.scala 45:26]
  wire [7:0] dmaGen_auto_dma_out_arid; // @[SimMMIO.scala 45:26]
  wire [37:0] dmaGen_auto_dma_out_araddr; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_dma_out_rvalid; // @[SimMMIO.scala 45:26]
  wire [7:0] dmaGen_auto_dma_out_rid; // @[SimMMIO.scala 45:26]
  wire [255:0] dmaGen_auto_dma_out_rdata; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_awready; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_awvalid; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_awid; // @[SimMMIO.scala 45:26]
  wire [36:0] dmaGen_auto_in_awaddr; // @[SimMMIO.scala 45:26]
  wire [7:0] dmaGen_auto_in_awlen; // @[SimMMIO.scala 45:26]
  wire [2:0] dmaGen_auto_in_awsize; // @[SimMMIO.scala 45:26]
  wire [1:0] dmaGen_auto_in_awburst; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_awlock; // @[SimMMIO.scala 45:26]
  wire [3:0] dmaGen_auto_in_awcache; // @[SimMMIO.scala 45:26]
  wire [2:0] dmaGen_auto_in_awprot; // @[SimMMIO.scala 45:26]
  wire [3:0] dmaGen_auto_in_awqos; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_wready; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_wvalid; // @[SimMMIO.scala 45:26]
  wire [63:0] dmaGen_auto_in_wdata; // @[SimMMIO.scala 45:26]
  wire [7:0] dmaGen_auto_in_wstrb; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_wlast; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_bready; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_bvalid; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_bid; // @[SimMMIO.scala 45:26]
  wire [1:0] dmaGen_auto_in_bresp; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_arready; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_arvalid; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_arid; // @[SimMMIO.scala 45:26]
  wire [36:0] dmaGen_auto_in_araddr; // @[SimMMIO.scala 45:26]
  wire [7:0] dmaGen_auto_in_arlen; // @[SimMMIO.scala 45:26]
  wire [2:0] dmaGen_auto_in_arsize; // @[SimMMIO.scala 45:26]
  wire [1:0] dmaGen_auto_in_arburst; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_arlock; // @[SimMMIO.scala 45:26]
  wire [3:0] dmaGen_auto_in_arcache; // @[SimMMIO.scala 45:26]
  wire [2:0] dmaGen_auto_in_arprot; // @[SimMMIO.scala 45:26]
  wire [3:0] dmaGen_auto_in_arqos; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_rready; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_rvalid; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_rid; // @[SimMMIO.scala 45:26]
  wire [63:0] dmaGen_auto_in_rdata; // @[SimMMIO.scala 45:26]
  wire [1:0] dmaGen_auto_in_rresp; // @[SimMMIO.scala 45:26]
  wire  dmaGen_auto_in_rlast; // @[SimMMIO.scala 45:26]
  wire  axi4xbar_clock; // @[Xbar.scala 218:30]
  wire  axi4xbar_reset; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_in_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_in_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_in_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_in_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_in_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_in_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_in_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_in_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_in_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_in_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_in_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_in_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_in_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_in_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_in_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_in_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_in_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_in_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_in_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_in_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_8_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_8_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_8_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_8_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_8_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_8_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_8_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_8_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_8_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_8_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_8_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_8_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_8_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_8_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_8_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_8_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_8_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_8_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_8_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_8_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_7_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_7_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_7_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_7_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_7_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_7_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_7_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_7_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_7_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_7_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_7_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_7_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_7_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_7_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_7_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_7_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_7_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_7_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_7_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_7_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_6_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_6_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_6_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_6_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_6_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_6_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_6_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_6_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_6_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_6_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_6_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_6_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_6_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_6_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_6_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_6_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_6_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_6_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_6_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_6_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_5_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_5_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_5_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_5_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_5_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_5_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_5_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_5_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_5_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_5_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_5_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_5_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_5_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_5_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_5_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_5_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_5_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_5_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_5_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_5_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_4_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_4_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_4_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_4_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_4_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_4_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_4_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_4_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_4_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_4_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_4_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_4_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_4_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_4_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_4_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_4_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_4_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_4_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_4_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_4_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_3_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_3_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_3_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_3_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_3_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_3_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_3_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_3_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_3_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_3_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_arid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_3_arcache; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_3_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_rid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_3_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_2_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_2_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_2_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_2_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_2_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_2_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_2_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_2_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_2_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_2_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_2_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_2_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_2_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_2_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_2_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_2_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_2_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_2_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_2_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_2_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_1_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_1_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_1_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_1_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_1_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_1_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_1_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_1_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_1_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_1_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_1_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_1_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_1_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_1_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_1_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_1_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_1_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_1_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_1_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_1_rlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_awready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_awvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_awid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_0_awaddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_0_awlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_0_awsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_0_awburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_awlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_0_awcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_0_awprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_0_awqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_wready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_wvalid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_0_wdata; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_0_wstrb; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_wlast; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_bready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_bvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_bid; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_0_bresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_arready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_arvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_arid; // @[Xbar.scala 218:30]
  wire [36:0] axi4xbar_auto_out_0_araddr; // @[Xbar.scala 218:30]
  wire [7:0] axi4xbar_auto_out_0_arlen; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_0_arsize; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_0_arburst; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_arlock; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_0_arcache; // @[Xbar.scala 218:30]
  wire [2:0] axi4xbar_auto_out_0_arprot; // @[Xbar.scala 218:30]
  wire [3:0] axi4xbar_auto_out_0_arqos; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_rready; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_rvalid; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_rid; // @[Xbar.scala 218:30]
  wire [63:0] axi4xbar_auto_out_0_rdata; // @[Xbar.scala 218:30]
  wire [1:0] axi4xbar_auto_out_0_rresp; // @[Xbar.scala 218:30]
  wire  axi4xbar_auto_out_0_rlast; // @[Xbar.scala 218:30]
  wire  errorDev_clock; // @[SimMMIO.scala 51:28]
  wire  errorDev_reset; // @[SimMMIO.scala 51:28]
  wire  errorDev_auto_in_a_ready; // @[SimMMIO.scala 51:28]
  wire  errorDev_auto_in_a_valid; // @[SimMMIO.scala 51:28]
  wire [2:0] errorDev_auto_in_a_bits_opcode; // @[SimMMIO.scala 51:28]
  wire [2:0] errorDev_auto_in_a_bits_size; // @[SimMMIO.scala 51:28]
  wire [1:0] errorDev_auto_in_a_bits_source; // @[SimMMIO.scala 51:28]
  wire  errorDev_auto_in_d_ready; // @[SimMMIO.scala 51:28]
  wire  errorDev_auto_in_d_valid; // @[SimMMIO.scala 51:28]
  wire [2:0] errorDev_auto_in_d_bits_opcode; // @[SimMMIO.scala 51:28]
  wire [2:0] errorDev_auto_in_d_bits_size; // @[SimMMIO.scala 51:28]
  wire [1:0] errorDev_auto_in_d_bits_source; // @[SimMMIO.scala 51:28]
  wire  errorDev_auto_in_d_bits_corrupt; // @[SimMMIO.scala 51:28]
  wire  xbar_clock; // @[Xbar.scala 142:26]
  wire  xbar_reset; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_ready; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_valid; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_in_a_bits_opcode; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_in_a_bits_size; // @[Xbar.scala 142:26]
  wire [1:0] xbar_auto_in_a_bits_source; // @[Xbar.scala 142:26]
  wire [37:0] xbar_auto_in_a_bits_address; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_bits_user_amba_prot_bufferable; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_bits_user_amba_prot_modifiable; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_bits_user_amba_prot_readalloc; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_bits_user_amba_prot_writealloc; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_bits_user_amba_prot_privileged; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_bits_user_amba_prot_secure; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_a_bits_user_amba_prot_fetch; // @[Xbar.scala 142:26]
  wire [7:0] xbar_auto_in_a_bits_mask; // @[Xbar.scala 142:26]
  wire [63:0] xbar_auto_in_a_bits_data; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_d_ready; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_d_valid; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_in_d_bits_opcode; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_in_d_bits_size; // @[Xbar.scala 142:26]
  wire [1:0] xbar_auto_in_d_bits_source; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_d_bits_denied; // @[Xbar.scala 142:26]
  wire [63:0] xbar_auto_in_d_bits_data; // @[Xbar.scala 142:26]
  wire  xbar_auto_in_d_bits_corrupt; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_ready; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_valid; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_out_1_a_bits_opcode; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_out_1_a_bits_size; // @[Xbar.scala 142:26]
  wire [1:0] xbar_auto_out_1_a_bits_source; // @[Xbar.scala 142:26]
  wire [36:0] xbar_auto_out_1_a_bits_address; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_bits_user_amba_prot_bufferable; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_bits_user_amba_prot_modifiable; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_bits_user_amba_prot_readalloc; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_bits_user_amba_prot_writealloc; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_bits_user_amba_prot_privileged; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_bits_user_amba_prot_secure; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_a_bits_user_amba_prot_fetch; // @[Xbar.scala 142:26]
  wire [7:0] xbar_auto_out_1_a_bits_mask; // @[Xbar.scala 142:26]
  wire [63:0] xbar_auto_out_1_a_bits_data; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_d_ready; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_d_valid; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_out_1_d_bits_opcode; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_out_1_d_bits_size; // @[Xbar.scala 142:26]
  wire [1:0] xbar_auto_out_1_d_bits_source; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_d_bits_denied; // @[Xbar.scala 142:26]
  wire [63:0] xbar_auto_out_1_d_bits_data; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_1_d_bits_corrupt; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_0_a_ready; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_0_a_valid; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_out_0_a_bits_opcode; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_out_0_a_bits_size; // @[Xbar.scala 142:26]
  wire [1:0] xbar_auto_out_0_a_bits_source; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_0_d_ready; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_0_d_valid; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_out_0_d_bits_opcode; // @[Xbar.scala 142:26]
  wire [2:0] xbar_auto_out_0_d_bits_size; // @[Xbar.scala 142:26]
  wire [1:0] xbar_auto_out_0_d_bits_source; // @[Xbar.scala 142:26]
  wire  xbar_auto_out_0_d_bits_corrupt; // @[Xbar.scala 142:26]
  wire  fixer_clock; // @[FIFOFixer.scala 144:27]
  wire  fixer_reset; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_ready; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_valid; // @[FIFOFixer.scala 144:27]
  wire [2:0] fixer_auto_in_a_bits_opcode; // @[FIFOFixer.scala 144:27]
  wire [2:0] fixer_auto_in_a_bits_size; // @[FIFOFixer.scala 144:27]
  wire [1:0] fixer_auto_in_a_bits_source; // @[FIFOFixer.scala 144:27]
  wire [37:0] fixer_auto_in_a_bits_address; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_bits_user_amba_prot_bufferable; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_bits_user_amba_prot_modifiable; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_bits_user_amba_prot_readalloc; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_bits_user_amba_prot_writealloc; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_bits_user_amba_prot_privileged; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_bits_user_amba_prot_secure; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_a_bits_user_amba_prot_fetch; // @[FIFOFixer.scala 144:27]
  wire [7:0] fixer_auto_in_a_bits_mask; // @[FIFOFixer.scala 144:27]
  wire [63:0] fixer_auto_in_a_bits_data; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_d_ready; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_d_valid; // @[FIFOFixer.scala 144:27]
  wire [2:0] fixer_auto_in_d_bits_opcode; // @[FIFOFixer.scala 144:27]
  wire [2:0] fixer_auto_in_d_bits_size; // @[FIFOFixer.scala 144:27]
  wire [1:0] fixer_auto_in_d_bits_source; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_d_bits_denied; // @[FIFOFixer.scala 144:27]
  wire [63:0] fixer_auto_in_d_bits_data; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_in_d_bits_corrupt; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_ready; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_valid; // @[FIFOFixer.scala 144:27]
  wire [2:0] fixer_auto_out_a_bits_opcode; // @[FIFOFixer.scala 144:27]
  wire [2:0] fixer_auto_out_a_bits_size; // @[FIFOFixer.scala 144:27]
  wire [1:0] fixer_auto_out_a_bits_source; // @[FIFOFixer.scala 144:27]
  wire [37:0] fixer_auto_out_a_bits_address; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_bits_user_amba_prot_bufferable; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_bits_user_amba_prot_modifiable; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_bits_user_amba_prot_readalloc; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_bits_user_amba_prot_writealloc; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_bits_user_amba_prot_privileged; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_bits_user_amba_prot_secure; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_a_bits_user_amba_prot_fetch; // @[FIFOFixer.scala 144:27]
  wire [7:0] fixer_auto_out_a_bits_mask; // @[FIFOFixer.scala 144:27]
  wire [63:0] fixer_auto_out_a_bits_data; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_d_ready; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_d_valid; // @[FIFOFixer.scala 144:27]
  wire [2:0] fixer_auto_out_d_bits_opcode; // @[FIFOFixer.scala 144:27]
  wire [2:0] fixer_auto_out_d_bits_size; // @[FIFOFixer.scala 144:27]
  wire [1:0] fixer_auto_out_d_bits_source; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_d_bits_denied; // @[FIFOFixer.scala 144:27]
  wire [63:0] fixer_auto_out_d_bits_data; // @[FIFOFixer.scala 144:27]
  wire  fixer_auto_out_d_bits_corrupt; // @[FIFOFixer.scala 144:27]
  wire  widget_clock; // @[WidthWidget.scala 219:28]
  wire  widget_reset; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_ready; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_valid; // @[WidthWidget.scala 219:28]
  wire [2:0] widget_auto_in_a_bits_opcode; // @[WidthWidget.scala 219:28]
  wire [2:0] widget_auto_in_a_bits_size; // @[WidthWidget.scala 219:28]
  wire [1:0] widget_auto_in_a_bits_source; // @[WidthWidget.scala 219:28]
  wire [37:0] widget_auto_in_a_bits_address; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_bits_user_amba_prot_bufferable; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_bits_user_amba_prot_modifiable; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_bits_user_amba_prot_readalloc; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_bits_user_amba_prot_writealloc; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_bits_user_amba_prot_privileged; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_bits_user_amba_prot_secure; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_a_bits_user_amba_prot_fetch; // @[WidthWidget.scala 219:28]
  wire [31:0] widget_auto_in_a_bits_mask; // @[WidthWidget.scala 219:28]
  wire [255:0] widget_auto_in_a_bits_data; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_d_ready; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_d_valid; // @[WidthWidget.scala 219:28]
  wire [2:0] widget_auto_in_d_bits_opcode; // @[WidthWidget.scala 219:28]
  wire [2:0] widget_auto_in_d_bits_size; // @[WidthWidget.scala 219:28]
  wire [1:0] widget_auto_in_d_bits_source; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_d_bits_denied; // @[WidthWidget.scala 219:28]
  wire [255:0] widget_auto_in_d_bits_data; // @[WidthWidget.scala 219:28]
  wire  widget_auto_in_d_bits_corrupt; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_ready; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_valid; // @[WidthWidget.scala 219:28]
  wire [2:0] widget_auto_out_a_bits_opcode; // @[WidthWidget.scala 219:28]
  wire [2:0] widget_auto_out_a_bits_size; // @[WidthWidget.scala 219:28]
  wire [1:0] widget_auto_out_a_bits_source; // @[WidthWidget.scala 219:28]
  wire [37:0] widget_auto_out_a_bits_address; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_bits_user_amba_prot_bufferable; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_bits_user_amba_prot_modifiable; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_bits_user_amba_prot_readalloc; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_bits_user_amba_prot_writealloc; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_bits_user_amba_prot_privileged; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_bits_user_amba_prot_secure; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_a_bits_user_amba_prot_fetch; // @[WidthWidget.scala 219:28]
  wire [7:0] widget_auto_out_a_bits_mask; // @[WidthWidget.scala 219:28]
  wire [63:0] widget_auto_out_a_bits_data; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_d_ready; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_d_valid; // @[WidthWidget.scala 219:28]
  wire [2:0] widget_auto_out_d_bits_opcode; // @[WidthWidget.scala 219:28]
  wire [2:0] widget_auto_out_d_bits_size; // @[WidthWidget.scala 219:28]
  wire [1:0] widget_auto_out_d_bits_source; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_d_bits_denied; // @[WidthWidget.scala 219:28]
  wire [63:0] widget_auto_out_d_bits_data; // @[WidthWidget.scala 219:28]
  wire  widget_auto_out_d_bits_corrupt; // @[WidthWidget.scala 219:28]
  wire  axi42tl_clock; // @[ToTL.scala 216:29]
  wire  axi42tl_reset; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_awready; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_awvalid; // @[ToTL.scala 216:29]
  wire [4:0] axi42tl_auto_in_awid; // @[ToTL.scala 216:29]
  wire [37:0] axi42tl_auto_in_awaddr; // @[ToTL.scala 216:29]
  wire [7:0] axi42tl_auto_in_awlen; // @[ToTL.scala 216:29]
  wire [2:0] axi42tl_auto_in_awsize; // @[ToTL.scala 216:29]
  wire [3:0] axi42tl_auto_in_awcache; // @[ToTL.scala 216:29]
  wire [2:0] axi42tl_auto_in_awprot; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_wready; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_wvalid; // @[ToTL.scala 216:29]
  wire [255:0] axi42tl_auto_in_wdata; // @[ToTL.scala 216:29]
  wire [31:0] axi42tl_auto_in_wstrb; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_wlast; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_bready; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_bvalid; // @[ToTL.scala 216:29]
  wire [4:0] axi42tl_auto_in_bid; // @[ToTL.scala 216:29]
  wire [1:0] axi42tl_auto_in_bresp; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_arready; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_arvalid; // @[ToTL.scala 216:29]
  wire [4:0] axi42tl_auto_in_arid; // @[ToTL.scala 216:29]
  wire [37:0] axi42tl_auto_in_araddr; // @[ToTL.scala 216:29]
  wire [7:0] axi42tl_auto_in_arlen; // @[ToTL.scala 216:29]
  wire [2:0] axi42tl_auto_in_arsize; // @[ToTL.scala 216:29]
  wire [3:0] axi42tl_auto_in_arcache; // @[ToTL.scala 216:29]
  wire [2:0] axi42tl_auto_in_arprot; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_rready; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_rvalid; // @[ToTL.scala 216:29]
  wire [4:0] axi42tl_auto_in_rid; // @[ToTL.scala 216:29]
  wire [255:0] axi42tl_auto_in_rdata; // @[ToTL.scala 216:29]
  wire [1:0] axi42tl_auto_in_rresp; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_in_rlast; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_ready; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_valid; // @[ToTL.scala 216:29]
  wire [2:0] axi42tl_auto_out_a_bits_opcode; // @[ToTL.scala 216:29]
  wire [2:0] axi42tl_auto_out_a_bits_size; // @[ToTL.scala 216:29]
  wire [1:0] axi42tl_auto_out_a_bits_source; // @[ToTL.scala 216:29]
  wire [37:0] axi42tl_auto_out_a_bits_address; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_bits_user_amba_prot_bufferable; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_bits_user_amba_prot_modifiable; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_bits_user_amba_prot_readalloc; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_bits_user_amba_prot_writealloc; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_bits_user_amba_prot_privileged; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_bits_user_amba_prot_secure; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_a_bits_user_amba_prot_fetch; // @[ToTL.scala 216:29]
  wire [31:0] axi42tl_auto_out_a_bits_mask; // @[ToTL.scala 216:29]
  wire [255:0] axi42tl_auto_out_a_bits_data; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_d_ready; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_d_valid; // @[ToTL.scala 216:29]
  wire [2:0] axi42tl_auto_out_d_bits_opcode; // @[ToTL.scala 216:29]
  wire [2:0] axi42tl_auto_out_d_bits_size; // @[ToTL.scala 216:29]
  wire [1:0] axi42tl_auto_out_d_bits_source; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_d_bits_denied; // @[ToTL.scala 216:29]
  wire [255:0] axi42tl_auto_out_d_bits_data; // @[ToTL.scala 216:29]
  wire  axi42tl_auto_out_d_bits_corrupt; // @[ToTL.scala 216:29]
  wire  axi4yank_clock; // @[UserYanker.scala 105:30]
  wire  axi4yank_reset; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_awready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_awvalid; // @[UserYanker.scala 105:30]
  wire [4:0] axi4yank_auto_in_awid; // @[UserYanker.scala 105:30]
  wire [37:0] axi4yank_auto_in_awaddr; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_auto_in_awlen; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_auto_in_awsize; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_auto_in_awcache; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_auto_in_awprot; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_wready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_wvalid; // @[UserYanker.scala 105:30]
  wire [255:0] axi4yank_auto_in_wdata; // @[UserYanker.scala 105:30]
  wire [31:0] axi4yank_auto_in_wstrb; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_wlast; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_bready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_bvalid; // @[UserYanker.scala 105:30]
  wire [4:0] axi4yank_auto_in_bid; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_auto_in_bresp; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_arready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_arvalid; // @[UserYanker.scala 105:30]
  wire [4:0] axi4yank_auto_in_arid; // @[UserYanker.scala 105:30]
  wire [37:0] axi4yank_auto_in_araddr; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_auto_in_arlen; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_auto_in_arsize; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_auto_in_arcache; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_auto_in_arprot; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_rready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_rvalid; // @[UserYanker.scala 105:30]
  wire [4:0] axi4yank_auto_in_rid; // @[UserYanker.scala 105:30]
  wire [255:0] axi4yank_auto_in_rdata; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_auto_in_rresp; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_in_rlast; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_awready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_awvalid; // @[UserYanker.scala 105:30]
  wire [4:0] axi4yank_auto_out_awid; // @[UserYanker.scala 105:30]
  wire [37:0] axi4yank_auto_out_awaddr; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_auto_out_awlen; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_auto_out_awsize; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_auto_out_awcache; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_auto_out_awprot; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_wready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_wvalid; // @[UserYanker.scala 105:30]
  wire [255:0] axi4yank_auto_out_wdata; // @[UserYanker.scala 105:30]
  wire [31:0] axi4yank_auto_out_wstrb; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_wlast; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_bready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_bvalid; // @[UserYanker.scala 105:30]
  wire [4:0] axi4yank_auto_out_bid; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_auto_out_bresp; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_arready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_arvalid; // @[UserYanker.scala 105:30]
  wire [4:0] axi4yank_auto_out_arid; // @[UserYanker.scala 105:30]
  wire [37:0] axi4yank_auto_out_araddr; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_auto_out_arlen; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_auto_out_arsize; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_auto_out_arcache; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_auto_out_arprot; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_rready; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_rvalid; // @[UserYanker.scala 105:30]
  wire [4:0] axi4yank_auto_out_rid; // @[UserYanker.scala 105:30]
  wire [255:0] axi4yank_auto_out_rdata; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_auto_out_rresp; // @[UserYanker.scala 105:30]
  wire  axi4yank_auto_out_rlast; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_clock; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_reset; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_awready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_awvalid; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_awid; // @[UserYanker.scala 105:30]
  wire [36:0] axi4yank_1_auto_in_awaddr; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_1_auto_in_awlen; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_1_auto_in_awsize; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_in_awburst; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_awlock; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_in_awcache; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_1_auto_in_awprot; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_in_awqos; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_in_awecho_tl_state_size; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_in_awecho_tl_state_source; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_wready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_wvalid; // @[UserYanker.scala 105:30]
  wire [63:0] axi4yank_1_auto_in_wdata; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_1_auto_in_wstrb; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_wlast; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_bready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_bvalid; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_bid; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_in_bresp; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_in_becho_tl_state_size; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_in_becho_tl_state_source; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_arready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_arvalid; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_arid; // @[UserYanker.scala 105:30]
  wire [36:0] axi4yank_1_auto_in_araddr; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_1_auto_in_arlen; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_1_auto_in_arsize; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_in_arburst; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_arlock; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_in_arcache; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_1_auto_in_arprot; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_in_arqos; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_in_arecho_tl_state_size; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_in_arecho_tl_state_source; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_rready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_rvalid; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_rid; // @[UserYanker.scala 105:30]
  wire [63:0] axi4yank_1_auto_in_rdata; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_in_rresp; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_in_recho_tl_state_size; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_in_recho_tl_state_source; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_in_rlast; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_awready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_awvalid; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_awid; // @[UserYanker.scala 105:30]
  wire [36:0] axi4yank_1_auto_out_awaddr; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_1_auto_out_awlen; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_1_auto_out_awsize; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_out_awburst; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_awlock; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_out_awcache; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_1_auto_out_awprot; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_out_awqos; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_wready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_wvalid; // @[UserYanker.scala 105:30]
  wire [63:0] axi4yank_1_auto_out_wdata; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_1_auto_out_wstrb; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_wlast; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_bready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_bvalid; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_bid; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_out_bresp; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_arready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_arvalid; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_arid; // @[UserYanker.scala 105:30]
  wire [36:0] axi4yank_1_auto_out_araddr; // @[UserYanker.scala 105:30]
  wire [7:0] axi4yank_1_auto_out_arlen; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_1_auto_out_arsize; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_out_arburst; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_arlock; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_out_arcache; // @[UserYanker.scala 105:30]
  wire [2:0] axi4yank_1_auto_out_arprot; // @[UserYanker.scala 105:30]
  wire [3:0] axi4yank_1_auto_out_arqos; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_rready; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_rvalid; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_rid; // @[UserYanker.scala 105:30]
  wire [63:0] axi4yank_1_auto_out_rdata; // @[UserYanker.scala 105:30]
  wire [1:0] axi4yank_1_auto_out_rresp; // @[UserYanker.scala 105:30]
  wire  axi4yank_1_auto_out_rlast; // @[UserYanker.scala 105:30]
  wire  tl2axi4_clock; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_reset; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_ready; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_valid; // @[ToAXI4.scala 283:29]
  wire [2:0] tl2axi4_auto_in_a_bits_opcode; // @[ToAXI4.scala 283:29]
  wire [2:0] tl2axi4_auto_in_a_bits_size; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_in_a_bits_source; // @[ToAXI4.scala 283:29]
  wire [36:0] tl2axi4_auto_in_a_bits_address; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_bits_user_amba_prot_bufferable; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_bits_user_amba_prot_modifiable; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_bits_user_amba_prot_readalloc; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_bits_user_amba_prot_writealloc; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_bits_user_amba_prot_privileged; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_bits_user_amba_prot_secure; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_a_bits_user_amba_prot_fetch; // @[ToAXI4.scala 283:29]
  wire [7:0] tl2axi4_auto_in_a_bits_mask; // @[ToAXI4.scala 283:29]
  wire [63:0] tl2axi4_auto_in_a_bits_data; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_d_ready; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_d_valid; // @[ToAXI4.scala 283:29]
  wire [2:0] tl2axi4_auto_in_d_bits_opcode; // @[ToAXI4.scala 283:29]
  wire [2:0] tl2axi4_auto_in_d_bits_size; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_in_d_bits_source; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_d_bits_denied; // @[ToAXI4.scala 283:29]
  wire [63:0] tl2axi4_auto_in_d_bits_data; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_in_d_bits_corrupt; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_awready; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_awvalid; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_awid; // @[ToAXI4.scala 283:29]
  wire [36:0] tl2axi4_auto_out_awaddr; // @[ToAXI4.scala 283:29]
  wire [7:0] tl2axi4_auto_out_awlen; // @[ToAXI4.scala 283:29]
  wire [2:0] tl2axi4_auto_out_awsize; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_out_awburst; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_awlock; // @[ToAXI4.scala 283:29]
  wire [3:0] tl2axi4_auto_out_awcache; // @[ToAXI4.scala 283:29]
  wire [2:0] tl2axi4_auto_out_awprot; // @[ToAXI4.scala 283:29]
  wire [3:0] tl2axi4_auto_out_awqos; // @[ToAXI4.scala 283:29]
  wire [3:0] tl2axi4_auto_out_awecho_tl_state_size; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_out_awecho_tl_state_source; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_wready; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_wvalid; // @[ToAXI4.scala 283:29]
  wire [63:0] tl2axi4_auto_out_wdata; // @[ToAXI4.scala 283:29]
  wire [7:0] tl2axi4_auto_out_wstrb; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_wlast; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_bready; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_bvalid; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_bid; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_out_bresp; // @[ToAXI4.scala 283:29]
  wire [3:0] tl2axi4_auto_out_becho_tl_state_size; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_out_becho_tl_state_source; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_arready; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_arvalid; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_arid; // @[ToAXI4.scala 283:29]
  wire [36:0] tl2axi4_auto_out_araddr; // @[ToAXI4.scala 283:29]
  wire [7:0] tl2axi4_auto_out_arlen; // @[ToAXI4.scala 283:29]
  wire [2:0] tl2axi4_auto_out_arsize; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_out_arburst; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_arlock; // @[ToAXI4.scala 283:29]
  wire [3:0] tl2axi4_auto_out_arcache; // @[ToAXI4.scala 283:29]
  wire [2:0] tl2axi4_auto_out_arprot; // @[ToAXI4.scala 283:29]
  wire [3:0] tl2axi4_auto_out_arqos; // @[ToAXI4.scala 283:29]
  wire [3:0] tl2axi4_auto_out_arecho_tl_state_size; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_out_arecho_tl_state_source; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_rready; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_rvalid; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_rid; // @[ToAXI4.scala 283:29]
  wire [63:0] tl2axi4_auto_out_rdata; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_out_rresp; // @[ToAXI4.scala 283:29]
  wire [3:0] tl2axi4_auto_out_recho_tl_state_size; // @[ToAXI4.scala 283:29]
  wire [1:0] tl2axi4_auto_out_recho_tl_state_source; // @[ToAXI4.scala 283:29]
  wire  tl2axi4_auto_out_rlast; // @[ToAXI4.scala 283:29]
  AXI4Flash bootrom0 ( // @[SimMMIO.scala 34:28]
    .clock(bootrom0_clock),
    .reset(bootrom0_reset),
    .auto_in_awready(bootrom0_auto_in_awready),
    .auto_in_awvalid(bootrom0_auto_in_awvalid),
    .auto_in_awid(bootrom0_auto_in_awid),
    .auto_in_awaddr(bootrom0_auto_in_awaddr),
    .auto_in_awlen(bootrom0_auto_in_awlen),
    .auto_in_awsize(bootrom0_auto_in_awsize),
    .auto_in_awburst(bootrom0_auto_in_awburst),
    .auto_in_awlock(bootrom0_auto_in_awlock),
    .auto_in_awcache(bootrom0_auto_in_awcache),
    .auto_in_awprot(bootrom0_auto_in_awprot),
    .auto_in_awqos(bootrom0_auto_in_awqos),
    .auto_in_wready(bootrom0_auto_in_wready),
    .auto_in_wvalid(bootrom0_auto_in_wvalid),
    .auto_in_wdata(bootrom0_auto_in_wdata),
    .auto_in_wstrb(bootrom0_auto_in_wstrb),
    .auto_in_wlast(bootrom0_auto_in_wlast),
    .auto_in_bready(bootrom0_auto_in_bready),
    .auto_in_bvalid(bootrom0_auto_in_bvalid),
    .auto_in_bid(bootrom0_auto_in_bid),
    .auto_in_bresp(bootrom0_auto_in_bresp),
    .auto_in_arready(bootrom0_auto_in_arready),
    .auto_in_arvalid(bootrom0_auto_in_arvalid),
    .auto_in_arid(bootrom0_auto_in_arid),
    .auto_in_araddr(bootrom0_auto_in_araddr),
    .auto_in_arlen(bootrom0_auto_in_arlen),
    .auto_in_arsize(bootrom0_auto_in_arsize),
    .auto_in_arburst(bootrom0_auto_in_arburst),
    .auto_in_arlock(bootrom0_auto_in_arlock),
    .auto_in_arcache(bootrom0_auto_in_arcache),
    .auto_in_arprot(bootrom0_auto_in_arprot),
    .auto_in_arqos(bootrom0_auto_in_arqos),
    .auto_in_rready(bootrom0_auto_in_rready),
    .auto_in_rvalid(bootrom0_auto_in_rvalid),
    .auto_in_rid(bootrom0_auto_in_rid),
    .auto_in_rdata(bootrom0_auto_in_rdata),
    .auto_in_rresp(bootrom0_auto_in_rresp),
    .auto_in_rlast(bootrom0_auto_in_rlast)
  );
  AXI4Flash bootrom1 ( // @[SimMMIO.scala 35:28]
    .clock(bootrom1_clock),
    .reset(bootrom1_reset),
    .auto_in_awready(bootrom1_auto_in_awready),
    .auto_in_awvalid(bootrom1_auto_in_awvalid),
    .auto_in_awid(bootrom1_auto_in_awid),
    .auto_in_awaddr(bootrom1_auto_in_awaddr),
    .auto_in_awlen(bootrom1_auto_in_awlen),
    .auto_in_awsize(bootrom1_auto_in_awsize),
    .auto_in_awburst(bootrom1_auto_in_awburst),
    .auto_in_awlock(bootrom1_auto_in_awlock),
    .auto_in_awcache(bootrom1_auto_in_awcache),
    .auto_in_awprot(bootrom1_auto_in_awprot),
    .auto_in_awqos(bootrom1_auto_in_awqos),
    .auto_in_wready(bootrom1_auto_in_wready),
    .auto_in_wvalid(bootrom1_auto_in_wvalid),
    .auto_in_wdata(bootrom1_auto_in_wdata),
    .auto_in_wstrb(bootrom1_auto_in_wstrb),
    .auto_in_wlast(bootrom1_auto_in_wlast),
    .auto_in_bready(bootrom1_auto_in_bready),
    .auto_in_bvalid(bootrom1_auto_in_bvalid),
    .auto_in_bid(bootrom1_auto_in_bid),
    .auto_in_bresp(bootrom1_auto_in_bresp),
    .auto_in_arready(bootrom1_auto_in_arready),
    .auto_in_arvalid(bootrom1_auto_in_arvalid),
    .auto_in_arid(bootrom1_auto_in_arid),
    .auto_in_araddr(bootrom1_auto_in_araddr),
    .auto_in_arlen(bootrom1_auto_in_arlen),
    .auto_in_arsize(bootrom1_auto_in_arsize),
    .auto_in_arburst(bootrom1_auto_in_arburst),
    .auto_in_arlock(bootrom1_auto_in_arlock),
    .auto_in_arcache(bootrom1_auto_in_arcache),
    .auto_in_arprot(bootrom1_auto_in_arprot),
    .auto_in_arqos(bootrom1_auto_in_arqos),
    .auto_in_rready(bootrom1_auto_in_rready),
    .auto_in_rvalid(bootrom1_auto_in_rvalid),
    .auto_in_rid(bootrom1_auto_in_rid),
    .auto_in_rdata(bootrom1_auto_in_rdata),
    .auto_in_rresp(bootrom1_auto_in_rresp),
    .auto_in_rlast(bootrom1_auto_in_rlast)
  );
  AXI4Flash flash ( // @[SimMMIO.scala 36:25]
    .clock(flash_clock),
    .reset(flash_reset),
    .auto_in_awready(flash_auto_in_awready),
    .auto_in_awvalid(flash_auto_in_awvalid),
    .auto_in_awid(flash_auto_in_awid),
    .auto_in_awaddr(flash_auto_in_awaddr),
    .auto_in_awlen(flash_auto_in_awlen),
    .auto_in_awsize(flash_auto_in_awsize),
    .auto_in_awburst(flash_auto_in_awburst),
    .auto_in_awlock(flash_auto_in_awlock),
    .auto_in_awcache(flash_auto_in_awcache),
    .auto_in_awprot(flash_auto_in_awprot),
    .auto_in_awqos(flash_auto_in_awqos),
    .auto_in_wready(flash_auto_in_wready),
    .auto_in_wvalid(flash_auto_in_wvalid),
    .auto_in_wdata(flash_auto_in_wdata),
    .auto_in_wstrb(flash_auto_in_wstrb),
    .auto_in_wlast(flash_auto_in_wlast),
    .auto_in_bready(flash_auto_in_bready),
    .auto_in_bvalid(flash_auto_in_bvalid),
    .auto_in_bid(flash_auto_in_bid),
    .auto_in_bresp(flash_auto_in_bresp),
    .auto_in_arready(flash_auto_in_arready),
    .auto_in_arvalid(flash_auto_in_arvalid),
    .auto_in_arid(flash_auto_in_arid),
    .auto_in_araddr(flash_auto_in_araddr),
    .auto_in_arlen(flash_auto_in_arlen),
    .auto_in_arsize(flash_auto_in_arsize),
    .auto_in_arburst(flash_auto_in_arburst),
    .auto_in_arlock(flash_auto_in_arlock),
    .auto_in_arcache(flash_auto_in_arcache),
    .auto_in_arprot(flash_auto_in_arprot),
    .auto_in_arqos(flash_auto_in_arqos),
    .auto_in_rready(flash_auto_in_rready),
    .auto_in_rvalid(flash_auto_in_rvalid),
    .auto_in_rid(flash_auto_in_rid),
    .auto_in_rdata(flash_auto_in_rdata),
    .auto_in_rresp(flash_auto_in_rresp),
    .auto_in_rlast(flash_auto_in_rlast)
  );
  AXI4UART uart ( // @[SimMMIO.scala 37:24]
    .clock(uart_clock),
    .reset(uart_reset),
    .auto_in_awready(uart_auto_in_awready),
    .auto_in_awvalid(uart_auto_in_awvalid),
    .auto_in_awid(uart_auto_in_awid),
    .auto_in_awaddr(uart_auto_in_awaddr),
    .auto_in_awlen(uart_auto_in_awlen),
    .auto_in_awsize(uart_auto_in_awsize),
    .auto_in_awburst(uart_auto_in_awburst),
    .auto_in_awlock(uart_auto_in_awlock),
    .auto_in_awcache(uart_auto_in_awcache),
    .auto_in_awprot(uart_auto_in_awprot),
    .auto_in_awqos(uart_auto_in_awqos),
    .auto_in_wready(uart_auto_in_wready),
    .auto_in_wvalid(uart_auto_in_wvalid),
    .auto_in_wdata(uart_auto_in_wdata),
    .auto_in_wstrb(uart_auto_in_wstrb),
    .auto_in_wlast(uart_auto_in_wlast),
    .auto_in_bready(uart_auto_in_bready),
    .auto_in_bvalid(uart_auto_in_bvalid),
    .auto_in_bid(uart_auto_in_bid),
    .auto_in_bresp(uart_auto_in_bresp),
    .auto_in_arready(uart_auto_in_arready),
    .auto_in_arvalid(uart_auto_in_arvalid),
    .auto_in_arid(uart_auto_in_arid),
    .auto_in_araddr(uart_auto_in_araddr),
    .auto_in_arlen(uart_auto_in_arlen),
    .auto_in_arsize(uart_auto_in_arsize),
    .auto_in_arburst(uart_auto_in_arburst),
    .auto_in_arlock(uart_auto_in_arlock),
    .auto_in_arcache(uart_auto_in_arcache),
    .auto_in_arprot(uart_auto_in_arprot),
    .auto_in_arqos(uart_auto_in_arqos),
    .auto_in_rready(uart_auto_in_rready),
    .auto_in_rvalid(uart_auto_in_rvalid),
    .auto_in_rid(uart_auto_in_rid),
    .auto_in_rdata(uart_auto_in_rdata),
    .auto_in_rresp(uart_auto_in_rresp),
    .auto_in_rlast(uart_auto_in_rlast),
    .io_extra_out_valid(uart_io_extra_out_valid),
    .io_extra_out_ch(uart_io_extra_out_ch),
    .io_extra_in_valid(uart_io_extra_in_valid),
    .io_extra_in_ch(uart_io_extra_in_ch)
  );
  AXI4VGA vga ( // @[SimMMIO.scala 38:23]
    .clock(vga_clock),
    .reset(vga_reset),
    .auto_in_1_awready(vga_auto_in_1_awready),
    .auto_in_1_awvalid(vga_auto_in_1_awvalid),
    .auto_in_1_awid(vga_auto_in_1_awid),
    .auto_in_1_awaddr(vga_auto_in_1_awaddr),
    .auto_in_1_awlen(vga_auto_in_1_awlen),
    .auto_in_1_awsize(vga_auto_in_1_awsize),
    .auto_in_1_awburst(vga_auto_in_1_awburst),
    .auto_in_1_awlock(vga_auto_in_1_awlock),
    .auto_in_1_awcache(vga_auto_in_1_awcache),
    .auto_in_1_awprot(vga_auto_in_1_awprot),
    .auto_in_1_awqos(vga_auto_in_1_awqos),
    .auto_in_1_wready(vga_auto_in_1_wready),
    .auto_in_1_wvalid(vga_auto_in_1_wvalid),
    .auto_in_1_wdata(vga_auto_in_1_wdata),
    .auto_in_1_wstrb(vga_auto_in_1_wstrb),
    .auto_in_1_wlast(vga_auto_in_1_wlast),
    .auto_in_1_bready(vga_auto_in_1_bready),
    .auto_in_1_bvalid(vga_auto_in_1_bvalid),
    .auto_in_1_bid(vga_auto_in_1_bid),
    .auto_in_1_bresp(vga_auto_in_1_bresp),
    .auto_in_1_arready(vga_auto_in_1_arready),
    .auto_in_1_arvalid(vga_auto_in_1_arvalid),
    .auto_in_1_arid(vga_auto_in_1_arid),
    .auto_in_1_araddr(vga_auto_in_1_araddr),
    .auto_in_1_arlen(vga_auto_in_1_arlen),
    .auto_in_1_arsize(vga_auto_in_1_arsize),
    .auto_in_1_arburst(vga_auto_in_1_arburst),
    .auto_in_1_arlock(vga_auto_in_1_arlock),
    .auto_in_1_arcache(vga_auto_in_1_arcache),
    .auto_in_1_arprot(vga_auto_in_1_arprot),
    .auto_in_1_arqos(vga_auto_in_1_arqos),
    .auto_in_1_rready(vga_auto_in_1_rready),
    .auto_in_1_rvalid(vga_auto_in_1_rvalid),
    .auto_in_1_rid(vga_auto_in_1_rid),
    .auto_in_1_rdata(vga_auto_in_1_rdata),
    .auto_in_1_rresp(vga_auto_in_1_rresp),
    .auto_in_1_rlast(vga_auto_in_1_rlast),
    .auto_in_0_awready(vga_auto_in_0_awready),
    .auto_in_0_awvalid(vga_auto_in_0_awvalid),
    .auto_in_0_awid(vga_auto_in_0_awid),
    .auto_in_0_awaddr(vga_auto_in_0_awaddr),
    .auto_in_0_awlen(vga_auto_in_0_awlen),
    .auto_in_0_awsize(vga_auto_in_0_awsize),
    .auto_in_0_awburst(vga_auto_in_0_awburst),
    .auto_in_0_awlock(vga_auto_in_0_awlock),
    .auto_in_0_awcache(vga_auto_in_0_awcache),
    .auto_in_0_awprot(vga_auto_in_0_awprot),
    .auto_in_0_awqos(vga_auto_in_0_awqos),
    .auto_in_0_wready(vga_auto_in_0_wready),
    .auto_in_0_wvalid(vga_auto_in_0_wvalid),
    .auto_in_0_wdata(vga_auto_in_0_wdata),
    .auto_in_0_wstrb(vga_auto_in_0_wstrb),
    .auto_in_0_wlast(vga_auto_in_0_wlast),
    .auto_in_0_bready(vga_auto_in_0_bready),
    .auto_in_0_bvalid(vga_auto_in_0_bvalid),
    .auto_in_0_bid(vga_auto_in_0_bid),
    .auto_in_0_bresp(vga_auto_in_0_bresp),
    .auto_in_0_arvalid(vga_auto_in_0_arvalid),
    .auto_in_0_arid(vga_auto_in_0_arid),
    .auto_in_0_arlock(vga_auto_in_0_arlock),
    .auto_in_0_arcache(vga_auto_in_0_arcache),
    .auto_in_0_arqos(vga_auto_in_0_arqos),
    .auto_in_0_rready(vga_auto_in_0_rready),
    .auto_in_0_rvalid(vga_auto_in_0_rvalid),
    .auto_in_0_rid(vga_auto_in_0_rid),
    .auto_in_0_rlast(vga_auto_in_0_rlast)
  );
  AXI4DummySD sd ( // @[SimMMIO.scala 43:22]
    .clock(sd_clock),
    .reset(sd_reset),
    .auto_in_awready(sd_auto_in_awready),
    .auto_in_awvalid(sd_auto_in_awvalid),
    .auto_in_awid(sd_auto_in_awid),
    .auto_in_awaddr(sd_auto_in_awaddr),
    .auto_in_awlen(sd_auto_in_awlen),
    .auto_in_awsize(sd_auto_in_awsize),
    .auto_in_awburst(sd_auto_in_awburst),
    .auto_in_awlock(sd_auto_in_awlock),
    .auto_in_awcache(sd_auto_in_awcache),
    .auto_in_awprot(sd_auto_in_awprot),
    .auto_in_awqos(sd_auto_in_awqos),
    .auto_in_wready(sd_auto_in_wready),
    .auto_in_wvalid(sd_auto_in_wvalid),
    .auto_in_wdata(sd_auto_in_wdata),
    .auto_in_wstrb(sd_auto_in_wstrb),
    .auto_in_wlast(sd_auto_in_wlast),
    .auto_in_bready(sd_auto_in_bready),
    .auto_in_bvalid(sd_auto_in_bvalid),
    .auto_in_bid(sd_auto_in_bid),
    .auto_in_bresp(sd_auto_in_bresp),
    .auto_in_arready(sd_auto_in_arready),
    .auto_in_arvalid(sd_auto_in_arvalid),
    .auto_in_arid(sd_auto_in_arid),
    .auto_in_araddr(sd_auto_in_araddr),
    .auto_in_arlen(sd_auto_in_arlen),
    .auto_in_arsize(sd_auto_in_arsize),
    .auto_in_arburst(sd_auto_in_arburst),
    .auto_in_arlock(sd_auto_in_arlock),
    .auto_in_arcache(sd_auto_in_arcache),
    .auto_in_arprot(sd_auto_in_arprot),
    .auto_in_arqos(sd_auto_in_arqos),
    .auto_in_rready(sd_auto_in_rready),
    .auto_in_rvalid(sd_auto_in_rvalid),
    .auto_in_rid(sd_auto_in_rid),
    .auto_in_rdata(sd_auto_in_rdata),
    .auto_in_rresp(sd_auto_in_rresp),
    .auto_in_rlast(sd_auto_in_rlast)
  );
  AXI4IntrGenerator intrGen ( // @[SimMMIO.scala 44:27]
    .clock(intrGen_clock),
    .reset(intrGen_reset),
    .auto_in_awready(intrGen_auto_in_awready),
    .auto_in_awvalid(intrGen_auto_in_awvalid),
    .auto_in_awid(intrGen_auto_in_awid),
    .auto_in_awaddr(intrGen_auto_in_awaddr),
    .auto_in_awlen(intrGen_auto_in_awlen),
    .auto_in_awsize(intrGen_auto_in_awsize),
    .auto_in_awburst(intrGen_auto_in_awburst),
    .auto_in_awlock(intrGen_auto_in_awlock),
    .auto_in_awcache(intrGen_auto_in_awcache),
    .auto_in_awprot(intrGen_auto_in_awprot),
    .auto_in_awqos(intrGen_auto_in_awqos),
    .auto_in_wready(intrGen_auto_in_wready),
    .auto_in_wvalid(intrGen_auto_in_wvalid),
    .auto_in_wdata(intrGen_auto_in_wdata),
    .auto_in_wstrb(intrGen_auto_in_wstrb),
    .auto_in_wlast(intrGen_auto_in_wlast),
    .auto_in_bready(intrGen_auto_in_bready),
    .auto_in_bvalid(intrGen_auto_in_bvalid),
    .auto_in_bid(intrGen_auto_in_bid),
    .auto_in_bresp(intrGen_auto_in_bresp),
    .auto_in_arready(intrGen_auto_in_arready),
    .auto_in_arvalid(intrGen_auto_in_arvalid),
    .auto_in_arid(intrGen_auto_in_arid),
    .auto_in_araddr(intrGen_auto_in_araddr),
    .auto_in_arlen(intrGen_auto_in_arlen),
    .auto_in_arsize(intrGen_auto_in_arsize),
    .auto_in_arburst(intrGen_auto_in_arburst),
    .auto_in_arlock(intrGen_auto_in_arlock),
    .auto_in_arcache(intrGen_auto_in_arcache),
    .auto_in_arprot(intrGen_auto_in_arprot),
    .auto_in_arqos(intrGen_auto_in_arqos),
    .auto_in_rready(intrGen_auto_in_rready),
    .auto_in_rvalid(intrGen_auto_in_rvalid),
    .auto_in_rid(intrGen_auto_in_rid),
    .auto_in_rdata(intrGen_auto_in_rdata),
    .auto_in_rresp(intrGen_auto_in_rresp),
    .auto_in_rlast(intrGen_auto_in_rlast),
    .io_extra_intrVec(intrGen_io_extra_intrVec)
  );
  AXI4FakeDMA dmaGen ( // @[SimMMIO.scala 45:26]
    .clock(dmaGen_clock),
    .reset(dmaGen_reset),
    .auto_dma_out_awready(dmaGen_auto_dma_out_awready),
    .auto_dma_out_awvalid(dmaGen_auto_dma_out_awvalid),
    .auto_dma_out_awid(dmaGen_auto_dma_out_awid),
    .auto_dma_out_awaddr(dmaGen_auto_dma_out_awaddr),
    .auto_dma_out_wready(dmaGen_auto_dma_out_wready),
    .auto_dma_out_wvalid(dmaGen_auto_dma_out_wvalid),
    .auto_dma_out_wdata(dmaGen_auto_dma_out_wdata),
    .auto_dma_out_wstrb(dmaGen_auto_dma_out_wstrb),
    .auto_dma_out_wlast(dmaGen_auto_dma_out_wlast),
    .auto_dma_out_bvalid(dmaGen_auto_dma_out_bvalid),
    .auto_dma_out_bid(dmaGen_auto_dma_out_bid),
    .auto_dma_out_arready(dmaGen_auto_dma_out_arready),
    .auto_dma_out_arvalid(dmaGen_auto_dma_out_arvalid),
    .auto_dma_out_arid(dmaGen_auto_dma_out_arid),
    .auto_dma_out_araddr(dmaGen_auto_dma_out_araddr),
    .auto_dma_out_rvalid(dmaGen_auto_dma_out_rvalid),
    .auto_dma_out_rid(dmaGen_auto_dma_out_rid),
    .auto_dma_out_rdata(dmaGen_auto_dma_out_rdata),
    .auto_in_awready(dmaGen_auto_in_awready),
    .auto_in_awvalid(dmaGen_auto_in_awvalid),
    .auto_in_awid(dmaGen_auto_in_awid),
    .auto_in_awaddr(dmaGen_auto_in_awaddr),
    .auto_in_awlen(dmaGen_auto_in_awlen),
    .auto_in_awsize(dmaGen_auto_in_awsize),
    .auto_in_awburst(dmaGen_auto_in_awburst),
    .auto_in_awlock(dmaGen_auto_in_awlock),
    .auto_in_awcache(dmaGen_auto_in_awcache),
    .auto_in_awprot(dmaGen_auto_in_awprot),
    .auto_in_awqos(dmaGen_auto_in_awqos),
    .auto_in_wready(dmaGen_auto_in_wready),
    .auto_in_wvalid(dmaGen_auto_in_wvalid),
    .auto_in_wdata(dmaGen_auto_in_wdata),
    .auto_in_wstrb(dmaGen_auto_in_wstrb),
    .auto_in_wlast(dmaGen_auto_in_wlast),
    .auto_in_bready(dmaGen_auto_in_bready),
    .auto_in_bvalid(dmaGen_auto_in_bvalid),
    .auto_in_bid(dmaGen_auto_in_bid),
    .auto_in_bresp(dmaGen_auto_in_bresp),
    .auto_in_arready(dmaGen_auto_in_arready),
    .auto_in_arvalid(dmaGen_auto_in_arvalid),
    .auto_in_arid(dmaGen_auto_in_arid),
    .auto_in_araddr(dmaGen_auto_in_araddr),
    .auto_in_arlen(dmaGen_auto_in_arlen),
    .auto_in_arsize(dmaGen_auto_in_arsize),
    .auto_in_arburst(dmaGen_auto_in_arburst),
    .auto_in_arlock(dmaGen_auto_in_arlock),
    .auto_in_arcache(dmaGen_auto_in_arcache),
    .auto_in_arprot(dmaGen_auto_in_arprot),
    .auto_in_arqos(dmaGen_auto_in_arqos),
    .auto_in_rready(dmaGen_auto_in_rready),
    .auto_in_rvalid(dmaGen_auto_in_rvalid),
    .auto_in_rid(dmaGen_auto_in_rid),
    .auto_in_rdata(dmaGen_auto_in_rdata),
    .auto_in_rresp(dmaGen_auto_in_rresp),
    .auto_in_rlast(dmaGen_auto_in_rlast)
  );
  AXI4Xbar axi4xbar ( // @[Xbar.scala 218:30]
    .clock(axi4xbar_clock),
    .reset(axi4xbar_reset),
    .auto_in_awready(axi4xbar_auto_in_awready),
    .auto_in_awvalid(axi4xbar_auto_in_awvalid),
    .auto_in_awid(axi4xbar_auto_in_awid),
    .auto_in_awaddr(axi4xbar_auto_in_awaddr),
    .auto_in_awlen(axi4xbar_auto_in_awlen),
    .auto_in_awsize(axi4xbar_auto_in_awsize),
    .auto_in_awburst(axi4xbar_auto_in_awburst),
    .auto_in_awlock(axi4xbar_auto_in_awlock),
    .auto_in_awcache(axi4xbar_auto_in_awcache),
    .auto_in_awprot(axi4xbar_auto_in_awprot),
    .auto_in_awqos(axi4xbar_auto_in_awqos),
    .auto_in_wready(axi4xbar_auto_in_wready),
    .auto_in_wvalid(axi4xbar_auto_in_wvalid),
    .auto_in_wdata(axi4xbar_auto_in_wdata),
    .auto_in_wstrb(axi4xbar_auto_in_wstrb),
    .auto_in_wlast(axi4xbar_auto_in_wlast),
    .auto_in_bready(axi4xbar_auto_in_bready),
    .auto_in_bvalid(axi4xbar_auto_in_bvalid),
    .auto_in_bid(axi4xbar_auto_in_bid),
    .auto_in_bresp(axi4xbar_auto_in_bresp),
    .auto_in_arready(axi4xbar_auto_in_arready),
    .auto_in_arvalid(axi4xbar_auto_in_arvalid),
    .auto_in_arid(axi4xbar_auto_in_arid),
    .auto_in_araddr(axi4xbar_auto_in_araddr),
    .auto_in_arlen(axi4xbar_auto_in_arlen),
    .auto_in_arsize(axi4xbar_auto_in_arsize),
    .auto_in_arburst(axi4xbar_auto_in_arburst),
    .auto_in_arlock(axi4xbar_auto_in_arlock),
    .auto_in_arcache(axi4xbar_auto_in_arcache),
    .auto_in_arprot(axi4xbar_auto_in_arprot),
    .auto_in_arqos(axi4xbar_auto_in_arqos),
    .auto_in_rready(axi4xbar_auto_in_rready),
    .auto_in_rvalid(axi4xbar_auto_in_rvalid),
    .auto_in_rid(axi4xbar_auto_in_rid),
    .auto_in_rdata(axi4xbar_auto_in_rdata),
    .auto_in_rresp(axi4xbar_auto_in_rresp),
    .auto_in_rlast(axi4xbar_auto_in_rlast),
    .auto_out_8_awready(axi4xbar_auto_out_8_awready),
    .auto_out_8_awvalid(axi4xbar_auto_out_8_awvalid),
    .auto_out_8_awid(axi4xbar_auto_out_8_awid),
    .auto_out_8_awaddr(axi4xbar_auto_out_8_awaddr),
    .auto_out_8_awlen(axi4xbar_auto_out_8_awlen),
    .auto_out_8_awsize(axi4xbar_auto_out_8_awsize),
    .auto_out_8_awburst(axi4xbar_auto_out_8_awburst),
    .auto_out_8_awlock(axi4xbar_auto_out_8_awlock),
    .auto_out_8_awcache(axi4xbar_auto_out_8_awcache),
    .auto_out_8_awprot(axi4xbar_auto_out_8_awprot),
    .auto_out_8_awqos(axi4xbar_auto_out_8_awqos),
    .auto_out_8_wready(axi4xbar_auto_out_8_wready),
    .auto_out_8_wvalid(axi4xbar_auto_out_8_wvalid),
    .auto_out_8_wdata(axi4xbar_auto_out_8_wdata),
    .auto_out_8_wstrb(axi4xbar_auto_out_8_wstrb),
    .auto_out_8_wlast(axi4xbar_auto_out_8_wlast),
    .auto_out_8_bready(axi4xbar_auto_out_8_bready),
    .auto_out_8_bvalid(axi4xbar_auto_out_8_bvalid),
    .auto_out_8_bid(axi4xbar_auto_out_8_bid),
    .auto_out_8_bresp(axi4xbar_auto_out_8_bresp),
    .auto_out_8_arready(axi4xbar_auto_out_8_arready),
    .auto_out_8_arvalid(axi4xbar_auto_out_8_arvalid),
    .auto_out_8_arid(axi4xbar_auto_out_8_arid),
    .auto_out_8_araddr(axi4xbar_auto_out_8_araddr),
    .auto_out_8_arlen(axi4xbar_auto_out_8_arlen),
    .auto_out_8_arsize(axi4xbar_auto_out_8_arsize),
    .auto_out_8_arburst(axi4xbar_auto_out_8_arburst),
    .auto_out_8_arlock(axi4xbar_auto_out_8_arlock),
    .auto_out_8_arcache(axi4xbar_auto_out_8_arcache),
    .auto_out_8_arprot(axi4xbar_auto_out_8_arprot),
    .auto_out_8_arqos(axi4xbar_auto_out_8_arqos),
    .auto_out_8_rready(axi4xbar_auto_out_8_rready),
    .auto_out_8_rvalid(axi4xbar_auto_out_8_rvalid),
    .auto_out_8_rid(axi4xbar_auto_out_8_rid),
    .auto_out_8_rdata(axi4xbar_auto_out_8_rdata),
    .auto_out_8_rresp(axi4xbar_auto_out_8_rresp),
    .auto_out_8_rlast(axi4xbar_auto_out_8_rlast),
    .auto_out_7_awready(axi4xbar_auto_out_7_awready),
    .auto_out_7_awvalid(axi4xbar_auto_out_7_awvalid),
    .auto_out_7_awid(axi4xbar_auto_out_7_awid),
    .auto_out_7_awaddr(axi4xbar_auto_out_7_awaddr),
    .auto_out_7_awlen(axi4xbar_auto_out_7_awlen),
    .auto_out_7_awsize(axi4xbar_auto_out_7_awsize),
    .auto_out_7_awburst(axi4xbar_auto_out_7_awburst),
    .auto_out_7_awlock(axi4xbar_auto_out_7_awlock),
    .auto_out_7_awcache(axi4xbar_auto_out_7_awcache),
    .auto_out_7_awprot(axi4xbar_auto_out_7_awprot),
    .auto_out_7_awqos(axi4xbar_auto_out_7_awqos),
    .auto_out_7_wready(axi4xbar_auto_out_7_wready),
    .auto_out_7_wvalid(axi4xbar_auto_out_7_wvalid),
    .auto_out_7_wdata(axi4xbar_auto_out_7_wdata),
    .auto_out_7_wstrb(axi4xbar_auto_out_7_wstrb),
    .auto_out_7_wlast(axi4xbar_auto_out_7_wlast),
    .auto_out_7_bready(axi4xbar_auto_out_7_bready),
    .auto_out_7_bvalid(axi4xbar_auto_out_7_bvalid),
    .auto_out_7_bid(axi4xbar_auto_out_7_bid),
    .auto_out_7_bresp(axi4xbar_auto_out_7_bresp),
    .auto_out_7_arready(axi4xbar_auto_out_7_arready),
    .auto_out_7_arvalid(axi4xbar_auto_out_7_arvalid),
    .auto_out_7_arid(axi4xbar_auto_out_7_arid),
    .auto_out_7_araddr(axi4xbar_auto_out_7_araddr),
    .auto_out_7_arlen(axi4xbar_auto_out_7_arlen),
    .auto_out_7_arsize(axi4xbar_auto_out_7_arsize),
    .auto_out_7_arburst(axi4xbar_auto_out_7_arburst),
    .auto_out_7_arlock(axi4xbar_auto_out_7_arlock),
    .auto_out_7_arcache(axi4xbar_auto_out_7_arcache),
    .auto_out_7_arprot(axi4xbar_auto_out_7_arprot),
    .auto_out_7_arqos(axi4xbar_auto_out_7_arqos),
    .auto_out_7_rready(axi4xbar_auto_out_7_rready),
    .auto_out_7_rvalid(axi4xbar_auto_out_7_rvalid),
    .auto_out_7_rid(axi4xbar_auto_out_7_rid),
    .auto_out_7_rdata(axi4xbar_auto_out_7_rdata),
    .auto_out_7_rresp(axi4xbar_auto_out_7_rresp),
    .auto_out_7_rlast(axi4xbar_auto_out_7_rlast),
    .auto_out_6_awready(axi4xbar_auto_out_6_awready),
    .auto_out_6_awvalid(axi4xbar_auto_out_6_awvalid),
    .auto_out_6_awid(axi4xbar_auto_out_6_awid),
    .auto_out_6_awaddr(axi4xbar_auto_out_6_awaddr),
    .auto_out_6_awlen(axi4xbar_auto_out_6_awlen),
    .auto_out_6_awsize(axi4xbar_auto_out_6_awsize),
    .auto_out_6_awburst(axi4xbar_auto_out_6_awburst),
    .auto_out_6_awlock(axi4xbar_auto_out_6_awlock),
    .auto_out_6_awcache(axi4xbar_auto_out_6_awcache),
    .auto_out_6_awprot(axi4xbar_auto_out_6_awprot),
    .auto_out_6_awqos(axi4xbar_auto_out_6_awqos),
    .auto_out_6_wready(axi4xbar_auto_out_6_wready),
    .auto_out_6_wvalid(axi4xbar_auto_out_6_wvalid),
    .auto_out_6_wdata(axi4xbar_auto_out_6_wdata),
    .auto_out_6_wstrb(axi4xbar_auto_out_6_wstrb),
    .auto_out_6_wlast(axi4xbar_auto_out_6_wlast),
    .auto_out_6_bready(axi4xbar_auto_out_6_bready),
    .auto_out_6_bvalid(axi4xbar_auto_out_6_bvalid),
    .auto_out_6_bid(axi4xbar_auto_out_6_bid),
    .auto_out_6_bresp(axi4xbar_auto_out_6_bresp),
    .auto_out_6_arready(axi4xbar_auto_out_6_arready),
    .auto_out_6_arvalid(axi4xbar_auto_out_6_arvalid),
    .auto_out_6_arid(axi4xbar_auto_out_6_arid),
    .auto_out_6_araddr(axi4xbar_auto_out_6_araddr),
    .auto_out_6_arlen(axi4xbar_auto_out_6_arlen),
    .auto_out_6_arsize(axi4xbar_auto_out_6_arsize),
    .auto_out_6_arburst(axi4xbar_auto_out_6_arburst),
    .auto_out_6_arlock(axi4xbar_auto_out_6_arlock),
    .auto_out_6_arcache(axi4xbar_auto_out_6_arcache),
    .auto_out_6_arprot(axi4xbar_auto_out_6_arprot),
    .auto_out_6_arqos(axi4xbar_auto_out_6_arqos),
    .auto_out_6_rready(axi4xbar_auto_out_6_rready),
    .auto_out_6_rvalid(axi4xbar_auto_out_6_rvalid),
    .auto_out_6_rid(axi4xbar_auto_out_6_rid),
    .auto_out_6_rdata(axi4xbar_auto_out_6_rdata),
    .auto_out_6_rresp(axi4xbar_auto_out_6_rresp),
    .auto_out_6_rlast(axi4xbar_auto_out_6_rlast),
    .auto_out_5_awready(axi4xbar_auto_out_5_awready),
    .auto_out_5_awvalid(axi4xbar_auto_out_5_awvalid),
    .auto_out_5_awid(axi4xbar_auto_out_5_awid),
    .auto_out_5_awaddr(axi4xbar_auto_out_5_awaddr),
    .auto_out_5_awlen(axi4xbar_auto_out_5_awlen),
    .auto_out_5_awsize(axi4xbar_auto_out_5_awsize),
    .auto_out_5_awburst(axi4xbar_auto_out_5_awburst),
    .auto_out_5_awlock(axi4xbar_auto_out_5_awlock),
    .auto_out_5_awcache(axi4xbar_auto_out_5_awcache),
    .auto_out_5_awprot(axi4xbar_auto_out_5_awprot),
    .auto_out_5_awqos(axi4xbar_auto_out_5_awqos),
    .auto_out_5_wready(axi4xbar_auto_out_5_wready),
    .auto_out_5_wvalid(axi4xbar_auto_out_5_wvalid),
    .auto_out_5_wdata(axi4xbar_auto_out_5_wdata),
    .auto_out_5_wstrb(axi4xbar_auto_out_5_wstrb),
    .auto_out_5_wlast(axi4xbar_auto_out_5_wlast),
    .auto_out_5_bready(axi4xbar_auto_out_5_bready),
    .auto_out_5_bvalid(axi4xbar_auto_out_5_bvalid),
    .auto_out_5_bid(axi4xbar_auto_out_5_bid),
    .auto_out_5_bresp(axi4xbar_auto_out_5_bresp),
    .auto_out_5_arready(axi4xbar_auto_out_5_arready),
    .auto_out_5_arvalid(axi4xbar_auto_out_5_arvalid),
    .auto_out_5_arid(axi4xbar_auto_out_5_arid),
    .auto_out_5_araddr(axi4xbar_auto_out_5_araddr),
    .auto_out_5_arlen(axi4xbar_auto_out_5_arlen),
    .auto_out_5_arsize(axi4xbar_auto_out_5_arsize),
    .auto_out_5_arburst(axi4xbar_auto_out_5_arburst),
    .auto_out_5_arlock(axi4xbar_auto_out_5_arlock),
    .auto_out_5_arcache(axi4xbar_auto_out_5_arcache),
    .auto_out_5_arprot(axi4xbar_auto_out_5_arprot),
    .auto_out_5_arqos(axi4xbar_auto_out_5_arqos),
    .auto_out_5_rready(axi4xbar_auto_out_5_rready),
    .auto_out_5_rvalid(axi4xbar_auto_out_5_rvalid),
    .auto_out_5_rid(axi4xbar_auto_out_5_rid),
    .auto_out_5_rdata(axi4xbar_auto_out_5_rdata),
    .auto_out_5_rresp(axi4xbar_auto_out_5_rresp),
    .auto_out_5_rlast(axi4xbar_auto_out_5_rlast),
    .auto_out_4_awready(axi4xbar_auto_out_4_awready),
    .auto_out_4_awvalid(axi4xbar_auto_out_4_awvalid),
    .auto_out_4_awid(axi4xbar_auto_out_4_awid),
    .auto_out_4_awaddr(axi4xbar_auto_out_4_awaddr),
    .auto_out_4_awlen(axi4xbar_auto_out_4_awlen),
    .auto_out_4_awsize(axi4xbar_auto_out_4_awsize),
    .auto_out_4_awburst(axi4xbar_auto_out_4_awburst),
    .auto_out_4_awlock(axi4xbar_auto_out_4_awlock),
    .auto_out_4_awcache(axi4xbar_auto_out_4_awcache),
    .auto_out_4_awprot(axi4xbar_auto_out_4_awprot),
    .auto_out_4_awqos(axi4xbar_auto_out_4_awqos),
    .auto_out_4_wready(axi4xbar_auto_out_4_wready),
    .auto_out_4_wvalid(axi4xbar_auto_out_4_wvalid),
    .auto_out_4_wdata(axi4xbar_auto_out_4_wdata),
    .auto_out_4_wstrb(axi4xbar_auto_out_4_wstrb),
    .auto_out_4_wlast(axi4xbar_auto_out_4_wlast),
    .auto_out_4_bready(axi4xbar_auto_out_4_bready),
    .auto_out_4_bvalid(axi4xbar_auto_out_4_bvalid),
    .auto_out_4_bid(axi4xbar_auto_out_4_bid),
    .auto_out_4_bresp(axi4xbar_auto_out_4_bresp),
    .auto_out_4_arready(axi4xbar_auto_out_4_arready),
    .auto_out_4_arvalid(axi4xbar_auto_out_4_arvalid),
    .auto_out_4_arid(axi4xbar_auto_out_4_arid),
    .auto_out_4_araddr(axi4xbar_auto_out_4_araddr),
    .auto_out_4_arlen(axi4xbar_auto_out_4_arlen),
    .auto_out_4_arsize(axi4xbar_auto_out_4_arsize),
    .auto_out_4_arburst(axi4xbar_auto_out_4_arburst),
    .auto_out_4_arlock(axi4xbar_auto_out_4_arlock),
    .auto_out_4_arcache(axi4xbar_auto_out_4_arcache),
    .auto_out_4_arprot(axi4xbar_auto_out_4_arprot),
    .auto_out_4_arqos(axi4xbar_auto_out_4_arqos),
    .auto_out_4_rready(axi4xbar_auto_out_4_rready),
    .auto_out_4_rvalid(axi4xbar_auto_out_4_rvalid),
    .auto_out_4_rid(axi4xbar_auto_out_4_rid),
    .auto_out_4_rdata(axi4xbar_auto_out_4_rdata),
    .auto_out_4_rresp(axi4xbar_auto_out_4_rresp),
    .auto_out_4_rlast(axi4xbar_auto_out_4_rlast),
    .auto_out_3_awready(axi4xbar_auto_out_3_awready),
    .auto_out_3_awvalid(axi4xbar_auto_out_3_awvalid),
    .auto_out_3_awid(axi4xbar_auto_out_3_awid),
    .auto_out_3_awaddr(axi4xbar_auto_out_3_awaddr),
    .auto_out_3_awlen(axi4xbar_auto_out_3_awlen),
    .auto_out_3_awsize(axi4xbar_auto_out_3_awsize),
    .auto_out_3_awburst(axi4xbar_auto_out_3_awburst),
    .auto_out_3_awlock(axi4xbar_auto_out_3_awlock),
    .auto_out_3_awcache(axi4xbar_auto_out_3_awcache),
    .auto_out_3_awprot(axi4xbar_auto_out_3_awprot),
    .auto_out_3_awqos(axi4xbar_auto_out_3_awqos),
    .auto_out_3_wready(axi4xbar_auto_out_3_wready),
    .auto_out_3_wvalid(axi4xbar_auto_out_3_wvalid),
    .auto_out_3_wdata(axi4xbar_auto_out_3_wdata),
    .auto_out_3_wstrb(axi4xbar_auto_out_3_wstrb),
    .auto_out_3_wlast(axi4xbar_auto_out_3_wlast),
    .auto_out_3_bready(axi4xbar_auto_out_3_bready),
    .auto_out_3_bvalid(axi4xbar_auto_out_3_bvalid),
    .auto_out_3_bid(axi4xbar_auto_out_3_bid),
    .auto_out_3_bresp(axi4xbar_auto_out_3_bresp),
    .auto_out_3_arvalid(axi4xbar_auto_out_3_arvalid),
    .auto_out_3_arid(axi4xbar_auto_out_3_arid),
    .auto_out_3_arlock(axi4xbar_auto_out_3_arlock),
    .auto_out_3_arcache(axi4xbar_auto_out_3_arcache),
    .auto_out_3_arqos(axi4xbar_auto_out_3_arqos),
    .auto_out_3_rready(axi4xbar_auto_out_3_rready),
    .auto_out_3_rvalid(axi4xbar_auto_out_3_rvalid),
    .auto_out_3_rid(axi4xbar_auto_out_3_rid),
    .auto_out_3_rlast(axi4xbar_auto_out_3_rlast),
    .auto_out_2_awready(axi4xbar_auto_out_2_awready),
    .auto_out_2_awvalid(axi4xbar_auto_out_2_awvalid),
    .auto_out_2_awid(axi4xbar_auto_out_2_awid),
    .auto_out_2_awaddr(axi4xbar_auto_out_2_awaddr),
    .auto_out_2_awlen(axi4xbar_auto_out_2_awlen),
    .auto_out_2_awsize(axi4xbar_auto_out_2_awsize),
    .auto_out_2_awburst(axi4xbar_auto_out_2_awburst),
    .auto_out_2_awlock(axi4xbar_auto_out_2_awlock),
    .auto_out_2_awcache(axi4xbar_auto_out_2_awcache),
    .auto_out_2_awprot(axi4xbar_auto_out_2_awprot),
    .auto_out_2_awqos(axi4xbar_auto_out_2_awqos),
    .auto_out_2_wready(axi4xbar_auto_out_2_wready),
    .auto_out_2_wvalid(axi4xbar_auto_out_2_wvalid),
    .auto_out_2_wdata(axi4xbar_auto_out_2_wdata),
    .auto_out_2_wstrb(axi4xbar_auto_out_2_wstrb),
    .auto_out_2_wlast(axi4xbar_auto_out_2_wlast),
    .auto_out_2_bready(axi4xbar_auto_out_2_bready),
    .auto_out_2_bvalid(axi4xbar_auto_out_2_bvalid),
    .auto_out_2_bid(axi4xbar_auto_out_2_bid),
    .auto_out_2_bresp(axi4xbar_auto_out_2_bresp),
    .auto_out_2_arready(axi4xbar_auto_out_2_arready),
    .auto_out_2_arvalid(axi4xbar_auto_out_2_arvalid),
    .auto_out_2_arid(axi4xbar_auto_out_2_arid),
    .auto_out_2_araddr(axi4xbar_auto_out_2_araddr),
    .auto_out_2_arlen(axi4xbar_auto_out_2_arlen),
    .auto_out_2_arsize(axi4xbar_auto_out_2_arsize),
    .auto_out_2_arburst(axi4xbar_auto_out_2_arburst),
    .auto_out_2_arlock(axi4xbar_auto_out_2_arlock),
    .auto_out_2_arcache(axi4xbar_auto_out_2_arcache),
    .auto_out_2_arprot(axi4xbar_auto_out_2_arprot),
    .auto_out_2_arqos(axi4xbar_auto_out_2_arqos),
    .auto_out_2_rready(axi4xbar_auto_out_2_rready),
    .auto_out_2_rvalid(axi4xbar_auto_out_2_rvalid),
    .auto_out_2_rid(axi4xbar_auto_out_2_rid),
    .auto_out_2_rdata(axi4xbar_auto_out_2_rdata),
    .auto_out_2_rresp(axi4xbar_auto_out_2_rresp),
    .auto_out_2_rlast(axi4xbar_auto_out_2_rlast),
    .auto_out_1_awready(axi4xbar_auto_out_1_awready),
    .auto_out_1_awvalid(axi4xbar_auto_out_1_awvalid),
    .auto_out_1_awid(axi4xbar_auto_out_1_awid),
    .auto_out_1_awaddr(axi4xbar_auto_out_1_awaddr),
    .auto_out_1_awlen(axi4xbar_auto_out_1_awlen),
    .auto_out_1_awsize(axi4xbar_auto_out_1_awsize),
    .auto_out_1_awburst(axi4xbar_auto_out_1_awburst),
    .auto_out_1_awlock(axi4xbar_auto_out_1_awlock),
    .auto_out_1_awcache(axi4xbar_auto_out_1_awcache),
    .auto_out_1_awprot(axi4xbar_auto_out_1_awprot),
    .auto_out_1_awqos(axi4xbar_auto_out_1_awqos),
    .auto_out_1_wready(axi4xbar_auto_out_1_wready),
    .auto_out_1_wvalid(axi4xbar_auto_out_1_wvalid),
    .auto_out_1_wdata(axi4xbar_auto_out_1_wdata),
    .auto_out_1_wstrb(axi4xbar_auto_out_1_wstrb),
    .auto_out_1_wlast(axi4xbar_auto_out_1_wlast),
    .auto_out_1_bready(axi4xbar_auto_out_1_bready),
    .auto_out_1_bvalid(axi4xbar_auto_out_1_bvalid),
    .auto_out_1_bid(axi4xbar_auto_out_1_bid),
    .auto_out_1_bresp(axi4xbar_auto_out_1_bresp),
    .auto_out_1_arready(axi4xbar_auto_out_1_arready),
    .auto_out_1_arvalid(axi4xbar_auto_out_1_arvalid),
    .auto_out_1_arid(axi4xbar_auto_out_1_arid),
    .auto_out_1_araddr(axi4xbar_auto_out_1_araddr),
    .auto_out_1_arlen(axi4xbar_auto_out_1_arlen),
    .auto_out_1_arsize(axi4xbar_auto_out_1_arsize),
    .auto_out_1_arburst(axi4xbar_auto_out_1_arburst),
    .auto_out_1_arlock(axi4xbar_auto_out_1_arlock),
    .auto_out_1_arcache(axi4xbar_auto_out_1_arcache),
    .auto_out_1_arprot(axi4xbar_auto_out_1_arprot),
    .auto_out_1_arqos(axi4xbar_auto_out_1_arqos),
    .auto_out_1_rready(axi4xbar_auto_out_1_rready),
    .auto_out_1_rvalid(axi4xbar_auto_out_1_rvalid),
    .auto_out_1_rid(axi4xbar_auto_out_1_rid),
    .auto_out_1_rdata(axi4xbar_auto_out_1_rdata),
    .auto_out_1_rresp(axi4xbar_auto_out_1_rresp),
    .auto_out_1_rlast(axi4xbar_auto_out_1_rlast),
    .auto_out_0_awready(axi4xbar_auto_out_0_awready),
    .auto_out_0_awvalid(axi4xbar_auto_out_0_awvalid),
    .auto_out_0_awid(axi4xbar_auto_out_0_awid),
    .auto_out_0_awaddr(axi4xbar_auto_out_0_awaddr),
    .auto_out_0_awlen(axi4xbar_auto_out_0_awlen),
    .auto_out_0_awsize(axi4xbar_auto_out_0_awsize),
    .auto_out_0_awburst(axi4xbar_auto_out_0_awburst),
    .auto_out_0_awlock(axi4xbar_auto_out_0_awlock),
    .auto_out_0_awcache(axi4xbar_auto_out_0_awcache),
    .auto_out_0_awprot(axi4xbar_auto_out_0_awprot),
    .auto_out_0_awqos(axi4xbar_auto_out_0_awqos),
    .auto_out_0_wready(axi4xbar_auto_out_0_wready),
    .auto_out_0_wvalid(axi4xbar_auto_out_0_wvalid),
    .auto_out_0_wdata(axi4xbar_auto_out_0_wdata),
    .auto_out_0_wstrb(axi4xbar_auto_out_0_wstrb),
    .auto_out_0_wlast(axi4xbar_auto_out_0_wlast),
    .auto_out_0_bready(axi4xbar_auto_out_0_bready),
    .auto_out_0_bvalid(axi4xbar_auto_out_0_bvalid),
    .auto_out_0_bid(axi4xbar_auto_out_0_bid),
    .auto_out_0_bresp(axi4xbar_auto_out_0_bresp),
    .auto_out_0_arready(axi4xbar_auto_out_0_arready),
    .auto_out_0_arvalid(axi4xbar_auto_out_0_arvalid),
    .auto_out_0_arid(axi4xbar_auto_out_0_arid),
    .auto_out_0_araddr(axi4xbar_auto_out_0_araddr),
    .auto_out_0_arlen(axi4xbar_auto_out_0_arlen),
    .auto_out_0_arsize(axi4xbar_auto_out_0_arsize),
    .auto_out_0_arburst(axi4xbar_auto_out_0_arburst),
    .auto_out_0_arlock(axi4xbar_auto_out_0_arlock),
    .auto_out_0_arcache(axi4xbar_auto_out_0_arcache),
    .auto_out_0_arprot(axi4xbar_auto_out_0_arprot),
    .auto_out_0_arqos(axi4xbar_auto_out_0_arqos),
    .auto_out_0_rready(axi4xbar_auto_out_0_rready),
    .auto_out_0_rvalid(axi4xbar_auto_out_0_rvalid),
    .auto_out_0_rid(axi4xbar_auto_out_0_rid),
    .auto_out_0_rdata(axi4xbar_auto_out_0_rdata),
    .auto_out_0_rresp(axi4xbar_auto_out_0_rresp),
    .auto_out_0_rlast(axi4xbar_auto_out_0_rlast)
  );
  TLError_2 errorDev ( // @[SimMMIO.scala 51:28]
    .clock(errorDev_clock),
    .reset(errorDev_reset),
    .auto_in_a_ready(errorDev_auto_in_a_ready),
    .auto_in_a_valid(errorDev_auto_in_a_valid),
    .auto_in_a_bits_opcode(errorDev_auto_in_a_bits_opcode),
    .auto_in_a_bits_size(errorDev_auto_in_a_bits_size),
    .auto_in_a_bits_source(errorDev_auto_in_a_bits_source),
    .auto_in_d_ready(errorDev_auto_in_d_ready),
    .auto_in_d_valid(errorDev_auto_in_d_valid),
    .auto_in_d_bits_opcode(errorDev_auto_in_d_bits_opcode),
    .auto_in_d_bits_size(errorDev_auto_in_d_bits_size),
    .auto_in_d_bits_source(errorDev_auto_in_d_bits_source),
    .auto_in_d_bits_corrupt(errorDev_auto_in_d_bits_corrupt)
  );
  TLXbar_9 xbar ( // @[Xbar.scala 142:26]
    .clock(xbar_clock),
    .reset(xbar_reset),
    .auto_in_a_ready(xbar_auto_in_a_ready),
    .auto_in_a_valid(xbar_auto_in_a_valid),
    .auto_in_a_bits_opcode(xbar_auto_in_a_bits_opcode),
    .auto_in_a_bits_size(xbar_auto_in_a_bits_size),
    .auto_in_a_bits_source(xbar_auto_in_a_bits_source),
    .auto_in_a_bits_address(xbar_auto_in_a_bits_address),
    .auto_in_a_bits_user_amba_prot_bufferable(xbar_auto_in_a_bits_user_amba_prot_bufferable),
    .auto_in_a_bits_user_amba_prot_modifiable(xbar_auto_in_a_bits_user_amba_prot_modifiable),
    .auto_in_a_bits_user_amba_prot_readalloc(xbar_auto_in_a_bits_user_amba_prot_readalloc),
    .auto_in_a_bits_user_amba_prot_writealloc(xbar_auto_in_a_bits_user_amba_prot_writealloc),
    .auto_in_a_bits_user_amba_prot_privileged(xbar_auto_in_a_bits_user_amba_prot_privileged),
    .auto_in_a_bits_user_amba_prot_secure(xbar_auto_in_a_bits_user_amba_prot_secure),
    .auto_in_a_bits_user_amba_prot_fetch(xbar_auto_in_a_bits_user_amba_prot_fetch),
    .auto_in_a_bits_mask(xbar_auto_in_a_bits_mask),
    .auto_in_a_bits_data(xbar_auto_in_a_bits_data),
    .auto_in_d_ready(xbar_auto_in_d_ready),
    .auto_in_d_valid(xbar_auto_in_d_valid),
    .auto_in_d_bits_opcode(xbar_auto_in_d_bits_opcode),
    .auto_in_d_bits_size(xbar_auto_in_d_bits_size),
    .auto_in_d_bits_source(xbar_auto_in_d_bits_source),
    .auto_in_d_bits_denied(xbar_auto_in_d_bits_denied),
    .auto_in_d_bits_data(xbar_auto_in_d_bits_data),
    .auto_in_d_bits_corrupt(xbar_auto_in_d_bits_corrupt),
    .auto_out_1_a_ready(xbar_auto_out_1_a_ready),
    .auto_out_1_a_valid(xbar_auto_out_1_a_valid),
    .auto_out_1_a_bits_opcode(xbar_auto_out_1_a_bits_opcode),
    .auto_out_1_a_bits_size(xbar_auto_out_1_a_bits_size),
    .auto_out_1_a_bits_source(xbar_auto_out_1_a_bits_source),
    .auto_out_1_a_bits_address(xbar_auto_out_1_a_bits_address),
    .auto_out_1_a_bits_user_amba_prot_bufferable(xbar_auto_out_1_a_bits_user_amba_prot_bufferable),
    .auto_out_1_a_bits_user_amba_prot_modifiable(xbar_auto_out_1_a_bits_user_amba_prot_modifiable),
    .auto_out_1_a_bits_user_amba_prot_readalloc(xbar_auto_out_1_a_bits_user_amba_prot_readalloc),
    .auto_out_1_a_bits_user_amba_prot_writealloc(xbar_auto_out_1_a_bits_user_amba_prot_writealloc),
    .auto_out_1_a_bits_user_amba_prot_privileged(xbar_auto_out_1_a_bits_user_amba_prot_privileged),
    .auto_out_1_a_bits_user_amba_prot_secure(xbar_auto_out_1_a_bits_user_amba_prot_secure),
    .auto_out_1_a_bits_user_amba_prot_fetch(xbar_auto_out_1_a_bits_user_amba_prot_fetch),
    .auto_out_1_a_bits_mask(xbar_auto_out_1_a_bits_mask),
    .auto_out_1_a_bits_data(xbar_auto_out_1_a_bits_data),
    .auto_out_1_d_ready(xbar_auto_out_1_d_ready),
    .auto_out_1_d_valid(xbar_auto_out_1_d_valid),
    .auto_out_1_d_bits_opcode(xbar_auto_out_1_d_bits_opcode),
    .auto_out_1_d_bits_size(xbar_auto_out_1_d_bits_size),
    .auto_out_1_d_bits_source(xbar_auto_out_1_d_bits_source),
    .auto_out_1_d_bits_denied(xbar_auto_out_1_d_bits_denied),
    .auto_out_1_d_bits_data(xbar_auto_out_1_d_bits_data),
    .auto_out_1_d_bits_corrupt(xbar_auto_out_1_d_bits_corrupt),
    .auto_out_0_a_ready(xbar_auto_out_0_a_ready),
    .auto_out_0_a_valid(xbar_auto_out_0_a_valid),
    .auto_out_0_a_bits_opcode(xbar_auto_out_0_a_bits_opcode),
    .auto_out_0_a_bits_size(xbar_auto_out_0_a_bits_size),
    .auto_out_0_a_bits_source(xbar_auto_out_0_a_bits_source),
    .auto_out_0_d_ready(xbar_auto_out_0_d_ready),
    .auto_out_0_d_valid(xbar_auto_out_0_d_valid),
    .auto_out_0_d_bits_opcode(xbar_auto_out_0_d_bits_opcode),
    .auto_out_0_d_bits_size(xbar_auto_out_0_d_bits_size),
    .auto_out_0_d_bits_source(xbar_auto_out_0_d_bits_source),
    .auto_out_0_d_bits_corrupt(xbar_auto_out_0_d_bits_corrupt)
  );
  TLFIFOFixer_1 fixer ( // @[FIFOFixer.scala 144:27]
    .clock(fixer_clock),
    .reset(fixer_reset),
    .auto_in_a_ready(fixer_auto_in_a_ready),
    .auto_in_a_valid(fixer_auto_in_a_valid),
    .auto_in_a_bits_opcode(fixer_auto_in_a_bits_opcode),
    .auto_in_a_bits_size(fixer_auto_in_a_bits_size),
    .auto_in_a_bits_source(fixer_auto_in_a_bits_source),
    .auto_in_a_bits_address(fixer_auto_in_a_bits_address),
    .auto_in_a_bits_user_amba_prot_bufferable(fixer_auto_in_a_bits_user_amba_prot_bufferable),
    .auto_in_a_bits_user_amba_prot_modifiable(fixer_auto_in_a_bits_user_amba_prot_modifiable),
    .auto_in_a_bits_user_amba_prot_readalloc(fixer_auto_in_a_bits_user_amba_prot_readalloc),
    .auto_in_a_bits_user_amba_prot_writealloc(fixer_auto_in_a_bits_user_amba_prot_writealloc),
    .auto_in_a_bits_user_amba_prot_privileged(fixer_auto_in_a_bits_user_amba_prot_privileged),
    .auto_in_a_bits_user_amba_prot_secure(fixer_auto_in_a_bits_user_amba_prot_secure),
    .auto_in_a_bits_user_amba_prot_fetch(fixer_auto_in_a_bits_user_amba_prot_fetch),
    .auto_in_a_bits_mask(fixer_auto_in_a_bits_mask),
    .auto_in_a_bits_data(fixer_auto_in_a_bits_data),
    .auto_in_d_ready(fixer_auto_in_d_ready),
    .auto_in_d_valid(fixer_auto_in_d_valid),
    .auto_in_d_bits_opcode(fixer_auto_in_d_bits_opcode),
    .auto_in_d_bits_size(fixer_auto_in_d_bits_size),
    .auto_in_d_bits_source(fixer_auto_in_d_bits_source),
    .auto_in_d_bits_denied(fixer_auto_in_d_bits_denied),
    .auto_in_d_bits_data(fixer_auto_in_d_bits_data),
    .auto_in_d_bits_corrupt(fixer_auto_in_d_bits_corrupt),
    .auto_out_a_ready(fixer_auto_out_a_ready),
    .auto_out_a_valid(fixer_auto_out_a_valid),
    .auto_out_a_bits_opcode(fixer_auto_out_a_bits_opcode),
    .auto_out_a_bits_size(fixer_auto_out_a_bits_size),
    .auto_out_a_bits_source(fixer_auto_out_a_bits_source),
    .auto_out_a_bits_address(fixer_auto_out_a_bits_address),
    .auto_out_a_bits_user_amba_prot_bufferable(fixer_auto_out_a_bits_user_amba_prot_bufferable),
    .auto_out_a_bits_user_amba_prot_modifiable(fixer_auto_out_a_bits_user_amba_prot_modifiable),
    .auto_out_a_bits_user_amba_prot_readalloc(fixer_auto_out_a_bits_user_amba_prot_readalloc),
    .auto_out_a_bits_user_amba_prot_writealloc(fixer_auto_out_a_bits_user_amba_prot_writealloc),
    .auto_out_a_bits_user_amba_prot_privileged(fixer_auto_out_a_bits_user_amba_prot_privileged),
    .auto_out_a_bits_user_amba_prot_secure(fixer_auto_out_a_bits_user_amba_prot_secure),
    .auto_out_a_bits_user_amba_prot_fetch(fixer_auto_out_a_bits_user_amba_prot_fetch),
    .auto_out_a_bits_mask(fixer_auto_out_a_bits_mask),
    .auto_out_a_bits_data(fixer_auto_out_a_bits_data),
    .auto_out_d_ready(fixer_auto_out_d_ready),
    .auto_out_d_valid(fixer_auto_out_d_valid),
    .auto_out_d_bits_opcode(fixer_auto_out_d_bits_opcode),
    .auto_out_d_bits_size(fixer_auto_out_d_bits_size),
    .auto_out_d_bits_source(fixer_auto_out_d_bits_source),
    .auto_out_d_bits_denied(fixer_auto_out_d_bits_denied),
    .auto_out_d_bits_data(fixer_auto_out_d_bits_data),
    .auto_out_d_bits_corrupt(fixer_auto_out_d_bits_corrupt)
  );
  TLWidthWidget_4 widget ( // @[WidthWidget.scala 219:28]
    .clock(widget_clock),
    .reset(widget_reset),
    .auto_in_a_ready(widget_auto_in_a_ready),
    .auto_in_a_valid(widget_auto_in_a_valid),
    .auto_in_a_bits_opcode(widget_auto_in_a_bits_opcode),
    .auto_in_a_bits_size(widget_auto_in_a_bits_size),
    .auto_in_a_bits_source(widget_auto_in_a_bits_source),
    .auto_in_a_bits_address(widget_auto_in_a_bits_address),
    .auto_in_a_bits_user_amba_prot_bufferable(widget_auto_in_a_bits_user_amba_prot_bufferable),
    .auto_in_a_bits_user_amba_prot_modifiable(widget_auto_in_a_bits_user_amba_prot_modifiable),
    .auto_in_a_bits_user_amba_prot_readalloc(widget_auto_in_a_bits_user_amba_prot_readalloc),
    .auto_in_a_bits_user_amba_prot_writealloc(widget_auto_in_a_bits_user_amba_prot_writealloc),
    .auto_in_a_bits_user_amba_prot_privileged(widget_auto_in_a_bits_user_amba_prot_privileged),
    .auto_in_a_bits_user_amba_prot_secure(widget_auto_in_a_bits_user_amba_prot_secure),
    .auto_in_a_bits_user_amba_prot_fetch(widget_auto_in_a_bits_user_amba_prot_fetch),
    .auto_in_a_bits_mask(widget_auto_in_a_bits_mask),
    .auto_in_a_bits_data(widget_auto_in_a_bits_data),
    .auto_in_d_ready(widget_auto_in_d_ready),
    .auto_in_d_valid(widget_auto_in_d_valid),
    .auto_in_d_bits_opcode(widget_auto_in_d_bits_opcode),
    .auto_in_d_bits_size(widget_auto_in_d_bits_size),
    .auto_in_d_bits_source(widget_auto_in_d_bits_source),
    .auto_in_d_bits_denied(widget_auto_in_d_bits_denied),
    .auto_in_d_bits_data(widget_auto_in_d_bits_data),
    .auto_in_d_bits_corrupt(widget_auto_in_d_bits_corrupt),
    .auto_out_a_ready(widget_auto_out_a_ready),
    .auto_out_a_valid(widget_auto_out_a_valid),
    .auto_out_a_bits_opcode(widget_auto_out_a_bits_opcode),
    .auto_out_a_bits_size(widget_auto_out_a_bits_size),
    .auto_out_a_bits_source(widget_auto_out_a_bits_source),
    .auto_out_a_bits_address(widget_auto_out_a_bits_address),
    .auto_out_a_bits_user_amba_prot_bufferable(widget_auto_out_a_bits_user_amba_prot_bufferable),
    .auto_out_a_bits_user_amba_prot_modifiable(widget_auto_out_a_bits_user_amba_prot_modifiable),
    .auto_out_a_bits_user_amba_prot_readalloc(widget_auto_out_a_bits_user_amba_prot_readalloc),
    .auto_out_a_bits_user_amba_prot_writealloc(widget_auto_out_a_bits_user_amba_prot_writealloc),
    .auto_out_a_bits_user_amba_prot_privileged(widget_auto_out_a_bits_user_amba_prot_privileged),
    .auto_out_a_bits_user_amba_prot_secure(widget_auto_out_a_bits_user_amba_prot_secure),
    .auto_out_a_bits_user_amba_prot_fetch(widget_auto_out_a_bits_user_amba_prot_fetch),
    .auto_out_a_bits_mask(widget_auto_out_a_bits_mask),
    .auto_out_a_bits_data(widget_auto_out_a_bits_data),
    .auto_out_d_ready(widget_auto_out_d_ready),
    .auto_out_d_valid(widget_auto_out_d_valid),
    .auto_out_d_bits_opcode(widget_auto_out_d_bits_opcode),
    .auto_out_d_bits_size(widget_auto_out_d_bits_size),
    .auto_out_d_bits_source(widget_auto_out_d_bits_source),
    .auto_out_d_bits_denied(widget_auto_out_d_bits_denied),
    .auto_out_d_bits_data(widget_auto_out_d_bits_data),
    .auto_out_d_bits_corrupt(widget_auto_out_d_bits_corrupt)
  );
  AXI4ToTL_1 axi42tl ( // @[ToTL.scala 216:29]
    .clock(axi42tl_clock),
    .reset(axi42tl_reset),
    .auto_in_awready(axi42tl_auto_in_awready),
    .auto_in_awvalid(axi42tl_auto_in_awvalid),
    .auto_in_awid(axi42tl_auto_in_awid),
    .auto_in_awaddr(axi42tl_auto_in_awaddr),
    .auto_in_awlen(axi42tl_auto_in_awlen),
    .auto_in_awsize(axi42tl_auto_in_awsize),
    .auto_in_awcache(axi42tl_auto_in_awcache),
    .auto_in_awprot(axi42tl_auto_in_awprot),
    .auto_in_wready(axi42tl_auto_in_wready),
    .auto_in_wvalid(axi42tl_auto_in_wvalid),
    .auto_in_wdata(axi42tl_auto_in_wdata),
    .auto_in_wstrb(axi42tl_auto_in_wstrb),
    .auto_in_wlast(axi42tl_auto_in_wlast),
    .auto_in_bready(axi42tl_auto_in_bready),
    .auto_in_bvalid(axi42tl_auto_in_bvalid),
    .auto_in_bid(axi42tl_auto_in_bid),
    .auto_in_bresp(axi42tl_auto_in_bresp),
    .auto_in_arready(axi42tl_auto_in_arready),
    .auto_in_arvalid(axi42tl_auto_in_arvalid),
    .auto_in_arid(axi42tl_auto_in_arid),
    .auto_in_araddr(axi42tl_auto_in_araddr),
    .auto_in_arlen(axi42tl_auto_in_arlen),
    .auto_in_arsize(axi42tl_auto_in_arsize),
    .auto_in_arcache(axi42tl_auto_in_arcache),
    .auto_in_arprot(axi42tl_auto_in_arprot),
    .auto_in_rready(axi42tl_auto_in_rready),
    .auto_in_rvalid(axi42tl_auto_in_rvalid),
    .auto_in_rid(axi42tl_auto_in_rid),
    .auto_in_rdata(axi42tl_auto_in_rdata),
    .auto_in_rresp(axi42tl_auto_in_rresp),
    .auto_in_rlast(axi42tl_auto_in_rlast),
    .auto_out_a_ready(axi42tl_auto_out_a_ready),
    .auto_out_a_valid(axi42tl_auto_out_a_valid),
    .auto_out_a_bits_opcode(axi42tl_auto_out_a_bits_opcode),
    .auto_out_a_bits_size(axi42tl_auto_out_a_bits_size),
    .auto_out_a_bits_source(axi42tl_auto_out_a_bits_source),
    .auto_out_a_bits_address(axi42tl_auto_out_a_bits_address),
    .auto_out_a_bits_user_amba_prot_bufferable(axi42tl_auto_out_a_bits_user_amba_prot_bufferable),
    .auto_out_a_bits_user_amba_prot_modifiable(axi42tl_auto_out_a_bits_user_amba_prot_modifiable),
    .auto_out_a_bits_user_amba_prot_readalloc(axi42tl_auto_out_a_bits_user_amba_prot_readalloc),
    .auto_out_a_bits_user_amba_prot_writealloc(axi42tl_auto_out_a_bits_user_amba_prot_writealloc),
    .auto_out_a_bits_user_amba_prot_privileged(axi42tl_auto_out_a_bits_user_amba_prot_privileged),
    .auto_out_a_bits_user_amba_prot_secure(axi42tl_auto_out_a_bits_user_amba_prot_secure),
    .auto_out_a_bits_user_amba_prot_fetch(axi42tl_auto_out_a_bits_user_amba_prot_fetch),
    .auto_out_a_bits_mask(axi42tl_auto_out_a_bits_mask),
    .auto_out_a_bits_data(axi42tl_auto_out_a_bits_data),
    .auto_out_d_ready(axi42tl_auto_out_d_ready),
    .auto_out_d_valid(axi42tl_auto_out_d_valid),
    .auto_out_d_bits_opcode(axi42tl_auto_out_d_bits_opcode),
    .auto_out_d_bits_size(axi42tl_auto_out_d_bits_size),
    .auto_out_d_bits_source(axi42tl_auto_out_d_bits_source),
    .auto_out_d_bits_denied(axi42tl_auto_out_d_bits_denied),
    .auto_out_d_bits_data(axi42tl_auto_out_d_bits_data),
    .auto_out_d_bits_corrupt(axi42tl_auto_out_d_bits_corrupt)
  );
  AXI4UserYanker_3 axi4yank ( // @[UserYanker.scala 105:30]
    .clock(axi4yank_clock),
    .reset(axi4yank_reset),
    .auto_in_awready(axi4yank_auto_in_awready),
    .auto_in_awvalid(axi4yank_auto_in_awvalid),
    .auto_in_awid(axi4yank_auto_in_awid),
    .auto_in_awaddr(axi4yank_auto_in_awaddr),
    .auto_in_awlen(axi4yank_auto_in_awlen),
    .auto_in_awsize(axi4yank_auto_in_awsize),
    .auto_in_awcache(axi4yank_auto_in_awcache),
    .auto_in_awprot(axi4yank_auto_in_awprot),
    .auto_in_wready(axi4yank_auto_in_wready),
    .auto_in_wvalid(axi4yank_auto_in_wvalid),
    .auto_in_wdata(axi4yank_auto_in_wdata),
    .auto_in_wstrb(axi4yank_auto_in_wstrb),
    .auto_in_wlast(axi4yank_auto_in_wlast),
    .auto_in_bready(axi4yank_auto_in_bready),
    .auto_in_bvalid(axi4yank_auto_in_bvalid),
    .auto_in_bid(axi4yank_auto_in_bid),
    .auto_in_bresp(axi4yank_auto_in_bresp),
    .auto_in_arready(axi4yank_auto_in_arready),
    .auto_in_arvalid(axi4yank_auto_in_arvalid),
    .auto_in_arid(axi4yank_auto_in_arid),
    .auto_in_araddr(axi4yank_auto_in_araddr),
    .auto_in_arlen(axi4yank_auto_in_arlen),
    .auto_in_arsize(axi4yank_auto_in_arsize),
    .auto_in_arcache(axi4yank_auto_in_arcache),
    .auto_in_arprot(axi4yank_auto_in_arprot),
    .auto_in_rready(axi4yank_auto_in_rready),
    .auto_in_rvalid(axi4yank_auto_in_rvalid),
    .auto_in_rid(axi4yank_auto_in_rid),
    .auto_in_rdata(axi4yank_auto_in_rdata),
    .auto_in_rresp(axi4yank_auto_in_rresp),
    .auto_in_rlast(axi4yank_auto_in_rlast),
    .auto_out_awready(axi4yank_auto_out_awready),
    .auto_out_awvalid(axi4yank_auto_out_awvalid),
    .auto_out_awid(axi4yank_auto_out_awid),
    .auto_out_awaddr(axi4yank_auto_out_awaddr),
    .auto_out_awlen(axi4yank_auto_out_awlen),
    .auto_out_awsize(axi4yank_auto_out_awsize),
    .auto_out_awcache(axi4yank_auto_out_awcache),
    .auto_out_awprot(axi4yank_auto_out_awprot),
    .auto_out_wready(axi4yank_auto_out_wready),
    .auto_out_wvalid(axi4yank_auto_out_wvalid),
    .auto_out_wdata(axi4yank_auto_out_wdata),
    .auto_out_wstrb(axi4yank_auto_out_wstrb),
    .auto_out_wlast(axi4yank_auto_out_wlast),
    .auto_out_bready(axi4yank_auto_out_bready),
    .auto_out_bvalid(axi4yank_auto_out_bvalid),
    .auto_out_bid(axi4yank_auto_out_bid),
    .auto_out_bresp(axi4yank_auto_out_bresp),
    .auto_out_arready(axi4yank_auto_out_arready),
    .auto_out_arvalid(axi4yank_auto_out_arvalid),
    .auto_out_arid(axi4yank_auto_out_arid),
    .auto_out_araddr(axi4yank_auto_out_araddr),
    .auto_out_arlen(axi4yank_auto_out_arlen),
    .auto_out_arsize(axi4yank_auto_out_arsize),
    .auto_out_arcache(axi4yank_auto_out_arcache),
    .auto_out_arprot(axi4yank_auto_out_arprot),
    .auto_out_rready(axi4yank_auto_out_rready),
    .auto_out_rvalid(axi4yank_auto_out_rvalid),
    .auto_out_rid(axi4yank_auto_out_rid),
    .auto_out_rdata(axi4yank_auto_out_rdata),
    .auto_out_rresp(axi4yank_auto_out_rresp),
    .auto_out_rlast(axi4yank_auto_out_rlast)
  );
  AXI4UserYanker_4 axi4yank_1 ( // @[UserYanker.scala 105:30]
    .clock(axi4yank_1_clock),
    .reset(axi4yank_1_reset),
    .auto_in_awready(axi4yank_1_auto_in_awready),
    .auto_in_awvalid(axi4yank_1_auto_in_awvalid),
    .auto_in_awid(axi4yank_1_auto_in_awid),
    .auto_in_awaddr(axi4yank_1_auto_in_awaddr),
    .auto_in_awlen(axi4yank_1_auto_in_awlen),
    .auto_in_awsize(axi4yank_1_auto_in_awsize),
    .auto_in_awburst(axi4yank_1_auto_in_awburst),
    .auto_in_awlock(axi4yank_1_auto_in_awlock),
    .auto_in_awcache(axi4yank_1_auto_in_awcache),
    .auto_in_awprot(axi4yank_1_auto_in_awprot),
    .auto_in_awqos(axi4yank_1_auto_in_awqos),
    .auto_in_awecho_tl_state_size(axi4yank_1_auto_in_awecho_tl_state_size),
    .auto_in_awecho_tl_state_source(axi4yank_1_auto_in_awecho_tl_state_source),
    .auto_in_wready(axi4yank_1_auto_in_wready),
    .auto_in_wvalid(axi4yank_1_auto_in_wvalid),
    .auto_in_wdata(axi4yank_1_auto_in_wdata),
    .auto_in_wstrb(axi4yank_1_auto_in_wstrb),
    .auto_in_wlast(axi4yank_1_auto_in_wlast),
    .auto_in_bready(axi4yank_1_auto_in_bready),
    .auto_in_bvalid(axi4yank_1_auto_in_bvalid),
    .auto_in_bid(axi4yank_1_auto_in_bid),
    .auto_in_bresp(axi4yank_1_auto_in_bresp),
    .auto_in_becho_tl_state_size(axi4yank_1_auto_in_becho_tl_state_size),
    .auto_in_becho_tl_state_source(axi4yank_1_auto_in_becho_tl_state_source),
    .auto_in_arready(axi4yank_1_auto_in_arready),
    .auto_in_arvalid(axi4yank_1_auto_in_arvalid),
    .auto_in_arid(axi4yank_1_auto_in_arid),
    .auto_in_araddr(axi4yank_1_auto_in_araddr),
    .auto_in_arlen(axi4yank_1_auto_in_arlen),
    .auto_in_arsize(axi4yank_1_auto_in_arsize),
    .auto_in_arburst(axi4yank_1_auto_in_arburst),
    .auto_in_arlock(axi4yank_1_auto_in_arlock),
    .auto_in_arcache(axi4yank_1_auto_in_arcache),
    .auto_in_arprot(axi4yank_1_auto_in_arprot),
    .auto_in_arqos(axi4yank_1_auto_in_arqos),
    .auto_in_arecho_tl_state_size(axi4yank_1_auto_in_arecho_tl_state_size),
    .auto_in_arecho_tl_state_source(axi4yank_1_auto_in_arecho_tl_state_source),
    .auto_in_rready(axi4yank_1_auto_in_rready),
    .auto_in_rvalid(axi4yank_1_auto_in_rvalid),
    .auto_in_rid(axi4yank_1_auto_in_rid),
    .auto_in_rdata(axi4yank_1_auto_in_rdata),
    .auto_in_rresp(axi4yank_1_auto_in_rresp),
    .auto_in_recho_tl_state_size(axi4yank_1_auto_in_recho_tl_state_size),
    .auto_in_recho_tl_state_source(axi4yank_1_auto_in_recho_tl_state_source),
    .auto_in_rlast(axi4yank_1_auto_in_rlast),
    .auto_out_awready(axi4yank_1_auto_out_awready),
    .auto_out_awvalid(axi4yank_1_auto_out_awvalid),
    .auto_out_awid(axi4yank_1_auto_out_awid),
    .auto_out_awaddr(axi4yank_1_auto_out_awaddr),
    .auto_out_awlen(axi4yank_1_auto_out_awlen),
    .auto_out_awsize(axi4yank_1_auto_out_awsize),
    .auto_out_awburst(axi4yank_1_auto_out_awburst),
    .auto_out_awlock(axi4yank_1_auto_out_awlock),
    .auto_out_awcache(axi4yank_1_auto_out_awcache),
    .auto_out_awprot(axi4yank_1_auto_out_awprot),
    .auto_out_awqos(axi4yank_1_auto_out_awqos),
    .auto_out_wready(axi4yank_1_auto_out_wready),
    .auto_out_wvalid(axi4yank_1_auto_out_wvalid),
    .auto_out_wdata(axi4yank_1_auto_out_wdata),
    .auto_out_wstrb(axi4yank_1_auto_out_wstrb),
    .auto_out_wlast(axi4yank_1_auto_out_wlast),
    .auto_out_bready(axi4yank_1_auto_out_bready),
    .auto_out_bvalid(axi4yank_1_auto_out_bvalid),
    .auto_out_bid(axi4yank_1_auto_out_bid),
    .auto_out_bresp(axi4yank_1_auto_out_bresp),
    .auto_out_arready(axi4yank_1_auto_out_arready),
    .auto_out_arvalid(axi4yank_1_auto_out_arvalid),
    .auto_out_arid(axi4yank_1_auto_out_arid),
    .auto_out_araddr(axi4yank_1_auto_out_araddr),
    .auto_out_arlen(axi4yank_1_auto_out_arlen),
    .auto_out_arsize(axi4yank_1_auto_out_arsize),
    .auto_out_arburst(axi4yank_1_auto_out_arburst),
    .auto_out_arlock(axi4yank_1_auto_out_arlock),
    .auto_out_arcache(axi4yank_1_auto_out_arcache),
    .auto_out_arprot(axi4yank_1_auto_out_arprot),
    .auto_out_arqos(axi4yank_1_auto_out_arqos),
    .auto_out_rready(axi4yank_1_auto_out_rready),
    .auto_out_rvalid(axi4yank_1_auto_out_rvalid),
    .auto_out_rid(axi4yank_1_auto_out_rid),
    .auto_out_rdata(axi4yank_1_auto_out_rdata),
    .auto_out_rresp(axi4yank_1_auto_out_rresp),
    .auto_out_rlast(axi4yank_1_auto_out_rlast)
  );
  TLToAXI4_2 tl2axi4 ( // @[ToAXI4.scala 283:29]
    .clock(tl2axi4_clock),
    .reset(tl2axi4_reset),
    .auto_in_a_ready(tl2axi4_auto_in_a_ready),
    .auto_in_a_valid(tl2axi4_auto_in_a_valid),
    .auto_in_a_bits_opcode(tl2axi4_auto_in_a_bits_opcode),
    .auto_in_a_bits_size(tl2axi4_auto_in_a_bits_size),
    .auto_in_a_bits_source(tl2axi4_auto_in_a_bits_source),
    .auto_in_a_bits_address(tl2axi4_auto_in_a_bits_address),
    .auto_in_a_bits_user_amba_prot_bufferable(tl2axi4_auto_in_a_bits_user_amba_prot_bufferable),
    .auto_in_a_bits_user_amba_prot_modifiable(tl2axi4_auto_in_a_bits_user_amba_prot_modifiable),
    .auto_in_a_bits_user_amba_prot_readalloc(tl2axi4_auto_in_a_bits_user_amba_prot_readalloc),
    .auto_in_a_bits_user_amba_prot_writealloc(tl2axi4_auto_in_a_bits_user_amba_prot_writealloc),
    .auto_in_a_bits_user_amba_prot_privileged(tl2axi4_auto_in_a_bits_user_amba_prot_privileged),
    .auto_in_a_bits_user_amba_prot_secure(tl2axi4_auto_in_a_bits_user_amba_prot_secure),
    .auto_in_a_bits_user_amba_prot_fetch(tl2axi4_auto_in_a_bits_user_amba_prot_fetch),
    .auto_in_a_bits_mask(tl2axi4_auto_in_a_bits_mask),
    .auto_in_a_bits_data(tl2axi4_auto_in_a_bits_data),
    .auto_in_d_ready(tl2axi4_auto_in_d_ready),
    .auto_in_d_valid(tl2axi4_auto_in_d_valid),
    .auto_in_d_bits_opcode(tl2axi4_auto_in_d_bits_opcode),
    .auto_in_d_bits_size(tl2axi4_auto_in_d_bits_size),
    .auto_in_d_bits_source(tl2axi4_auto_in_d_bits_source),
    .auto_in_d_bits_denied(tl2axi4_auto_in_d_bits_denied),
    .auto_in_d_bits_data(tl2axi4_auto_in_d_bits_data),
    .auto_in_d_bits_corrupt(tl2axi4_auto_in_d_bits_corrupt),
    .auto_out_awready(tl2axi4_auto_out_awready),
    .auto_out_awvalid(tl2axi4_auto_out_awvalid),
    .auto_out_awid(tl2axi4_auto_out_awid),
    .auto_out_awaddr(tl2axi4_auto_out_awaddr),
    .auto_out_awlen(tl2axi4_auto_out_awlen),
    .auto_out_awsize(tl2axi4_auto_out_awsize),
    .auto_out_awburst(tl2axi4_auto_out_awburst),
    .auto_out_awlock(tl2axi4_auto_out_awlock),
    .auto_out_awcache(tl2axi4_auto_out_awcache),
    .auto_out_awprot(tl2axi4_auto_out_awprot),
    .auto_out_awqos(tl2axi4_auto_out_awqos),
    .auto_out_awecho_tl_state_size(tl2axi4_auto_out_awecho_tl_state_size),
    .auto_out_awecho_tl_state_source(tl2axi4_auto_out_awecho_tl_state_source),
    .auto_out_wready(tl2axi4_auto_out_wready),
    .auto_out_wvalid(tl2axi4_auto_out_wvalid),
    .auto_out_wdata(tl2axi4_auto_out_wdata),
    .auto_out_wstrb(tl2axi4_auto_out_wstrb),
    .auto_out_wlast(tl2axi4_auto_out_wlast),
    .auto_out_bready(tl2axi4_auto_out_bready),
    .auto_out_bvalid(tl2axi4_auto_out_bvalid),
    .auto_out_bid(tl2axi4_auto_out_bid),
    .auto_out_bresp(tl2axi4_auto_out_bresp),
    .auto_out_becho_tl_state_size(tl2axi4_auto_out_becho_tl_state_size),
    .auto_out_becho_tl_state_source(tl2axi4_auto_out_becho_tl_state_source),
    .auto_out_arready(tl2axi4_auto_out_arready),
    .auto_out_arvalid(tl2axi4_auto_out_arvalid),
    .auto_out_arid(tl2axi4_auto_out_arid),
    .auto_out_araddr(tl2axi4_auto_out_araddr),
    .auto_out_arlen(tl2axi4_auto_out_arlen),
    .auto_out_arsize(tl2axi4_auto_out_arsize),
    .auto_out_arburst(tl2axi4_auto_out_arburst),
    .auto_out_arlock(tl2axi4_auto_out_arlock),
    .auto_out_arcache(tl2axi4_auto_out_arcache),
    .auto_out_arprot(tl2axi4_auto_out_arprot),
    .auto_out_arqos(tl2axi4_auto_out_arqos),
    .auto_out_arecho_tl_state_size(tl2axi4_auto_out_arecho_tl_state_size),
    .auto_out_arecho_tl_state_source(tl2axi4_auto_out_arecho_tl_state_source),
    .auto_out_rready(tl2axi4_auto_out_rready),
    .auto_out_rvalid(tl2axi4_auto_out_rvalid),
    .auto_out_rid(tl2axi4_auto_out_rid),
    .auto_out_rdata(tl2axi4_auto_out_rdata),
    .auto_out_rresp(tl2axi4_auto_out_rresp),
    .auto_out_recho_tl_state_size(tl2axi4_auto_out_recho_tl_state_size),
    .auto_out_recho_tl_state_source(tl2axi4_auto_out_recho_tl_state_source),
    .auto_out_rlast(tl2axi4_auto_out_rlast)
  );
  assign io_axi4_0_awready = axi4yank_auto_in_awready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_wready = axi4yank_auto_in_wready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_bvalid = axi4yank_auto_in_bvalid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_bid = axi4yank_auto_in_bid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_bresp = axi4yank_auto_in_bresp; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_arready = axi4yank_auto_in_arready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rvalid = axi4yank_auto_in_rvalid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rid = axi4yank_auto_in_rid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rdata = axi4yank_auto_in_rdata; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rresp = axi4yank_auto_in_rresp; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rlast = axi4yank_auto_in_rlast; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_dma_0_awvalid = dmaGen_auto_dma_out_awvalid; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_awid = dmaGen_auto_dma_out_awid; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_awaddr = dmaGen_auto_dma_out_awaddr; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_wvalid = dmaGen_auto_dma_out_wvalid; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_wdata = dmaGen_auto_dma_out_wdata; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_wstrb = dmaGen_auto_dma_out_wstrb; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_wlast = dmaGen_auto_dma_out_wlast; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_arvalid = dmaGen_auto_dma_out_arvalid; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_arid = dmaGen_auto_dma_out_arid; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_dma_0_araddr = dmaGen_auto_dma_out_araddr; // @[Nodes.scala 1210:84 LazyModule.scala 296:16]
  assign io_uart_out_valid = uart_io_extra_out_valid; // @[SimMMIO.scala 93:13]
  assign io_uart_out_ch = uart_io_extra_out_ch; // @[SimMMIO.scala 93:13]
  assign io_uart_in_valid = uart_io_extra_in_valid; // @[SimMMIO.scala 93:13]
  assign io_interrupt_intrVec = intrGen_io_extra_intrVec; // @[SimMMIO.scala 94:18]
  assign bootrom0_clock = clock;
  assign bootrom0_reset = reset;
  assign bootrom0_auto_in_awvalid = axi4xbar_auto_out_0_awvalid; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awid = axi4xbar_auto_out_0_awid; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awaddr = axi4xbar_auto_out_0_awaddr; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awlen = axi4xbar_auto_out_0_awlen; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awsize = axi4xbar_auto_out_0_awsize; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awburst = axi4xbar_auto_out_0_awburst; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awlock = axi4xbar_auto_out_0_awlock; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awcache = axi4xbar_auto_out_0_awcache; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awprot = axi4xbar_auto_out_0_awprot; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_awqos = axi4xbar_auto_out_0_awqos; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_wvalid = axi4xbar_auto_out_0_wvalid; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_wdata = axi4xbar_auto_out_0_wdata; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_wstrb = axi4xbar_auto_out_0_wstrb; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_wlast = axi4xbar_auto_out_0_wlast; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_bready = axi4xbar_auto_out_0_bready; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arvalid = axi4xbar_auto_out_0_arvalid; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arid = axi4xbar_auto_out_0_arid; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_araddr = axi4xbar_auto_out_0_araddr; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arlen = axi4xbar_auto_out_0_arlen; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arsize = axi4xbar_auto_out_0_arsize; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arburst = axi4xbar_auto_out_0_arburst; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arlock = axi4xbar_auto_out_0_arlock; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arcache = axi4xbar_auto_out_0_arcache; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arprot = axi4xbar_auto_out_0_arprot; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_arqos = axi4xbar_auto_out_0_arqos; // @[LazyModule.scala 296:16]
  assign bootrom0_auto_in_rready = axi4xbar_auto_out_0_rready; // @[LazyModule.scala 296:16]
  assign bootrom1_clock = clock;
  assign bootrom1_reset = reset;
  assign bootrom1_auto_in_awvalid = axi4xbar_auto_out_1_awvalid; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awid = axi4xbar_auto_out_1_awid; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awaddr = axi4xbar_auto_out_1_awaddr; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awlen = axi4xbar_auto_out_1_awlen; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awsize = axi4xbar_auto_out_1_awsize; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awburst = axi4xbar_auto_out_1_awburst; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awlock = axi4xbar_auto_out_1_awlock; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awcache = axi4xbar_auto_out_1_awcache; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awprot = axi4xbar_auto_out_1_awprot; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_awqos = axi4xbar_auto_out_1_awqos; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_wvalid = axi4xbar_auto_out_1_wvalid; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_wdata = axi4xbar_auto_out_1_wdata; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_wstrb = axi4xbar_auto_out_1_wstrb; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_wlast = axi4xbar_auto_out_1_wlast; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_bready = axi4xbar_auto_out_1_bready; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arvalid = axi4xbar_auto_out_1_arvalid; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arid = axi4xbar_auto_out_1_arid; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_araddr = axi4xbar_auto_out_1_araddr; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arlen = axi4xbar_auto_out_1_arlen; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arsize = axi4xbar_auto_out_1_arsize; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arburst = axi4xbar_auto_out_1_arburst; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arlock = axi4xbar_auto_out_1_arlock; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arcache = axi4xbar_auto_out_1_arcache; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arprot = axi4xbar_auto_out_1_arprot; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_arqos = axi4xbar_auto_out_1_arqos; // @[LazyModule.scala 296:16]
  assign bootrom1_auto_in_rready = axi4xbar_auto_out_1_rready; // @[LazyModule.scala 296:16]
  assign flash_clock = clock;
  assign flash_reset = reset;
  assign flash_auto_in_awvalid = axi4xbar_auto_out_5_awvalid; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awid = axi4xbar_auto_out_5_awid; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awaddr = axi4xbar_auto_out_5_awaddr; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awlen = axi4xbar_auto_out_5_awlen; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awsize = axi4xbar_auto_out_5_awsize; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awburst = axi4xbar_auto_out_5_awburst; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awlock = axi4xbar_auto_out_5_awlock; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awcache = axi4xbar_auto_out_5_awcache; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awprot = axi4xbar_auto_out_5_awprot; // @[LazyModule.scala 296:16]
  assign flash_auto_in_awqos = axi4xbar_auto_out_5_awqos; // @[LazyModule.scala 296:16]
  assign flash_auto_in_wvalid = axi4xbar_auto_out_5_wvalid; // @[LazyModule.scala 296:16]
  assign flash_auto_in_wdata = axi4xbar_auto_out_5_wdata; // @[LazyModule.scala 296:16]
  assign flash_auto_in_wstrb = axi4xbar_auto_out_5_wstrb; // @[LazyModule.scala 296:16]
  assign flash_auto_in_wlast = axi4xbar_auto_out_5_wlast; // @[LazyModule.scala 296:16]
  assign flash_auto_in_bready = axi4xbar_auto_out_5_bready; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arvalid = axi4xbar_auto_out_5_arvalid; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arid = axi4xbar_auto_out_5_arid; // @[LazyModule.scala 296:16]
  assign flash_auto_in_araddr = axi4xbar_auto_out_5_araddr; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arlen = axi4xbar_auto_out_5_arlen; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arsize = axi4xbar_auto_out_5_arsize; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arburst = axi4xbar_auto_out_5_arburst; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arlock = axi4xbar_auto_out_5_arlock; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arcache = axi4xbar_auto_out_5_arcache; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arprot = axi4xbar_auto_out_5_arprot; // @[LazyModule.scala 296:16]
  assign flash_auto_in_arqos = axi4xbar_auto_out_5_arqos; // @[LazyModule.scala 296:16]
  assign flash_auto_in_rready = axi4xbar_auto_out_5_rready; // @[LazyModule.scala 296:16]
  assign uart_clock = clock;
  assign uart_reset = reset;
  assign uart_auto_in_awvalid = axi4xbar_auto_out_2_awvalid; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awid = axi4xbar_auto_out_2_awid; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awaddr = axi4xbar_auto_out_2_awaddr; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awlen = axi4xbar_auto_out_2_awlen; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awsize = axi4xbar_auto_out_2_awsize; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awburst = axi4xbar_auto_out_2_awburst; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awlock = axi4xbar_auto_out_2_awlock; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awcache = axi4xbar_auto_out_2_awcache; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awprot = axi4xbar_auto_out_2_awprot; // @[LazyModule.scala 296:16]
  assign uart_auto_in_awqos = axi4xbar_auto_out_2_awqos; // @[LazyModule.scala 296:16]
  assign uart_auto_in_wvalid = axi4xbar_auto_out_2_wvalid; // @[LazyModule.scala 296:16]
  assign uart_auto_in_wdata = axi4xbar_auto_out_2_wdata; // @[LazyModule.scala 296:16]
  assign uart_auto_in_wstrb = axi4xbar_auto_out_2_wstrb; // @[LazyModule.scala 296:16]
  assign uart_auto_in_wlast = axi4xbar_auto_out_2_wlast; // @[LazyModule.scala 296:16]
  assign uart_auto_in_bready = axi4xbar_auto_out_2_bready; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arvalid = axi4xbar_auto_out_2_arvalid; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arid = axi4xbar_auto_out_2_arid; // @[LazyModule.scala 296:16]
  assign uart_auto_in_araddr = axi4xbar_auto_out_2_araddr; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arlen = axi4xbar_auto_out_2_arlen; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arsize = axi4xbar_auto_out_2_arsize; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arburst = axi4xbar_auto_out_2_arburst; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arlock = axi4xbar_auto_out_2_arlock; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arcache = axi4xbar_auto_out_2_arcache; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arprot = axi4xbar_auto_out_2_arprot; // @[LazyModule.scala 296:16]
  assign uart_auto_in_arqos = axi4xbar_auto_out_2_arqos; // @[LazyModule.scala 296:16]
  assign uart_auto_in_rready = axi4xbar_auto_out_2_rready; // @[LazyModule.scala 296:16]
  assign uart_io_extra_in_ch = io_uart_in_ch; // @[SimMMIO.scala 93:13]
  assign vga_clock = clock;
  assign vga_reset = reset;
  assign vga_auto_in_1_awvalid = axi4xbar_auto_out_4_awvalid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awid = axi4xbar_auto_out_4_awid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awaddr = axi4xbar_auto_out_4_awaddr; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awlen = axi4xbar_auto_out_4_awlen; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awsize = axi4xbar_auto_out_4_awsize; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awburst = axi4xbar_auto_out_4_awburst; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awlock = axi4xbar_auto_out_4_awlock; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awcache = axi4xbar_auto_out_4_awcache; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awprot = axi4xbar_auto_out_4_awprot; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_awqos = axi4xbar_auto_out_4_awqos; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_wvalid = axi4xbar_auto_out_4_wvalid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_wdata = axi4xbar_auto_out_4_wdata; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_wstrb = axi4xbar_auto_out_4_wstrb; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_wlast = axi4xbar_auto_out_4_wlast; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_bready = axi4xbar_auto_out_4_bready; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arvalid = axi4xbar_auto_out_4_arvalid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arid = axi4xbar_auto_out_4_arid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_araddr = axi4xbar_auto_out_4_araddr; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arlen = axi4xbar_auto_out_4_arlen; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arsize = axi4xbar_auto_out_4_arsize; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arburst = axi4xbar_auto_out_4_arburst; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arlock = axi4xbar_auto_out_4_arlock; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arcache = axi4xbar_auto_out_4_arcache; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arprot = axi4xbar_auto_out_4_arprot; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_arqos = axi4xbar_auto_out_4_arqos; // @[LazyModule.scala 296:16]
  assign vga_auto_in_1_rready = axi4xbar_auto_out_4_rready; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awvalid = axi4xbar_auto_out_3_awvalid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awid = axi4xbar_auto_out_3_awid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awaddr = axi4xbar_auto_out_3_awaddr; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awlen = axi4xbar_auto_out_3_awlen; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awsize = axi4xbar_auto_out_3_awsize; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awburst = axi4xbar_auto_out_3_awburst; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awlock = axi4xbar_auto_out_3_awlock; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awcache = axi4xbar_auto_out_3_awcache; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awprot = axi4xbar_auto_out_3_awprot; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_awqos = axi4xbar_auto_out_3_awqos; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_wvalid = axi4xbar_auto_out_3_wvalid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_wdata = axi4xbar_auto_out_3_wdata; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_wstrb = axi4xbar_auto_out_3_wstrb; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_wlast = axi4xbar_auto_out_3_wlast; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_bready = axi4xbar_auto_out_3_bready; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_arvalid = axi4xbar_auto_out_3_arvalid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_arid = axi4xbar_auto_out_3_arid; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_arlock = axi4xbar_auto_out_3_arlock; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_arcache = axi4xbar_auto_out_3_arcache; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_arqos = axi4xbar_auto_out_3_arqos; // @[LazyModule.scala 296:16]
  assign vga_auto_in_0_rready = axi4xbar_auto_out_3_rready; // @[LazyModule.scala 296:16]
  assign sd_clock = clock;
  assign sd_reset = reset;
  assign sd_auto_in_awvalid = axi4xbar_auto_out_6_awvalid; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awid = axi4xbar_auto_out_6_awid; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awaddr = axi4xbar_auto_out_6_awaddr; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awlen = axi4xbar_auto_out_6_awlen; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awsize = axi4xbar_auto_out_6_awsize; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awburst = axi4xbar_auto_out_6_awburst; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awlock = axi4xbar_auto_out_6_awlock; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awcache = axi4xbar_auto_out_6_awcache; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awprot = axi4xbar_auto_out_6_awprot; // @[LazyModule.scala 296:16]
  assign sd_auto_in_awqos = axi4xbar_auto_out_6_awqos; // @[LazyModule.scala 296:16]
  assign sd_auto_in_wvalid = axi4xbar_auto_out_6_wvalid; // @[LazyModule.scala 296:16]
  assign sd_auto_in_wdata = axi4xbar_auto_out_6_wdata; // @[LazyModule.scala 296:16]
  assign sd_auto_in_wstrb = axi4xbar_auto_out_6_wstrb; // @[LazyModule.scala 296:16]
  assign sd_auto_in_wlast = axi4xbar_auto_out_6_wlast; // @[LazyModule.scala 296:16]
  assign sd_auto_in_bready = axi4xbar_auto_out_6_bready; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arvalid = axi4xbar_auto_out_6_arvalid; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arid = axi4xbar_auto_out_6_arid; // @[LazyModule.scala 296:16]
  assign sd_auto_in_araddr = axi4xbar_auto_out_6_araddr; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arlen = axi4xbar_auto_out_6_arlen; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arsize = axi4xbar_auto_out_6_arsize; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arburst = axi4xbar_auto_out_6_arburst; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arlock = axi4xbar_auto_out_6_arlock; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arcache = axi4xbar_auto_out_6_arcache; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arprot = axi4xbar_auto_out_6_arprot; // @[LazyModule.scala 296:16]
  assign sd_auto_in_arqos = axi4xbar_auto_out_6_arqos; // @[LazyModule.scala 296:16]
  assign sd_auto_in_rready = axi4xbar_auto_out_6_rready; // @[LazyModule.scala 296:16]
  assign intrGen_clock = clock;
  assign intrGen_reset = reset;
  assign intrGen_auto_in_awvalid = axi4xbar_auto_out_7_awvalid; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awid = axi4xbar_auto_out_7_awid; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awaddr = axi4xbar_auto_out_7_awaddr; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awlen = axi4xbar_auto_out_7_awlen; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awsize = axi4xbar_auto_out_7_awsize; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awburst = axi4xbar_auto_out_7_awburst; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awlock = axi4xbar_auto_out_7_awlock; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awcache = axi4xbar_auto_out_7_awcache; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awprot = axi4xbar_auto_out_7_awprot; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_awqos = axi4xbar_auto_out_7_awqos; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_wvalid = axi4xbar_auto_out_7_wvalid; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_wdata = axi4xbar_auto_out_7_wdata; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_wstrb = axi4xbar_auto_out_7_wstrb; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_wlast = axi4xbar_auto_out_7_wlast; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_bready = axi4xbar_auto_out_7_bready; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arvalid = axi4xbar_auto_out_7_arvalid; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arid = axi4xbar_auto_out_7_arid; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_araddr = axi4xbar_auto_out_7_araddr; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arlen = axi4xbar_auto_out_7_arlen; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arsize = axi4xbar_auto_out_7_arsize; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arburst = axi4xbar_auto_out_7_arburst; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arlock = axi4xbar_auto_out_7_arlock; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arcache = axi4xbar_auto_out_7_arcache; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arprot = axi4xbar_auto_out_7_arprot; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_arqos = axi4xbar_auto_out_7_arqos; // @[LazyModule.scala 296:16]
  assign intrGen_auto_in_rready = axi4xbar_auto_out_7_rready; // @[LazyModule.scala 296:16]
  assign dmaGen_clock = clock;
  assign dmaGen_reset = reset;
  assign dmaGen_auto_dma_out_awready = io_dma_0_awready; // @[Nodes.scala 1210:84 1694:56]
  assign dmaGen_auto_dma_out_wready = io_dma_0_wready; // @[Nodes.scala 1210:84 1694:56]
  assign dmaGen_auto_dma_out_bvalid = io_dma_0_bvalid; // @[Nodes.scala 1210:84 1694:56]
  assign dmaGen_auto_dma_out_bid = io_dma_0_bid; // @[Nodes.scala 1210:84 1694:56]
  assign dmaGen_auto_dma_out_arready = io_dma_0_arready; // @[Nodes.scala 1210:84 1694:56]
  assign dmaGen_auto_dma_out_rvalid = io_dma_0_rvalid; // @[Nodes.scala 1210:84 1694:56]
  assign dmaGen_auto_dma_out_rid = io_dma_0_rid; // @[Nodes.scala 1210:84 1694:56]
  assign dmaGen_auto_dma_out_rdata = io_dma_0_rdata; // @[Nodes.scala 1210:84 1694:56]
  assign dmaGen_auto_in_awvalid = axi4xbar_auto_out_8_awvalid; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awid = axi4xbar_auto_out_8_awid; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awaddr = axi4xbar_auto_out_8_awaddr; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awlen = axi4xbar_auto_out_8_awlen; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awsize = axi4xbar_auto_out_8_awsize; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awburst = axi4xbar_auto_out_8_awburst; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awlock = axi4xbar_auto_out_8_awlock; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awcache = axi4xbar_auto_out_8_awcache; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awprot = axi4xbar_auto_out_8_awprot; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_awqos = axi4xbar_auto_out_8_awqos; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_wvalid = axi4xbar_auto_out_8_wvalid; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_wdata = axi4xbar_auto_out_8_wdata; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_wstrb = axi4xbar_auto_out_8_wstrb; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_wlast = axi4xbar_auto_out_8_wlast; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_bready = axi4xbar_auto_out_8_bready; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arvalid = axi4xbar_auto_out_8_arvalid; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arid = axi4xbar_auto_out_8_arid; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_araddr = axi4xbar_auto_out_8_araddr; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arlen = axi4xbar_auto_out_8_arlen; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arsize = axi4xbar_auto_out_8_arsize; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arburst = axi4xbar_auto_out_8_arburst; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arlock = axi4xbar_auto_out_8_arlock; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arcache = axi4xbar_auto_out_8_arcache; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arprot = axi4xbar_auto_out_8_arprot; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_arqos = axi4xbar_auto_out_8_arqos; // @[LazyModule.scala 296:16]
  assign dmaGen_auto_in_rready = axi4xbar_auto_out_8_rready; // @[LazyModule.scala 296:16]
  assign axi4xbar_clock = clock;
  assign axi4xbar_reset = reset;
  assign axi4xbar_auto_in_awvalid = axi4yank_1_auto_out_awvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awid = axi4yank_1_auto_out_awid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awaddr = axi4yank_1_auto_out_awaddr; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awlen = axi4yank_1_auto_out_awlen; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awsize = axi4yank_1_auto_out_awsize; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awburst = axi4yank_1_auto_out_awburst; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awlock = axi4yank_1_auto_out_awlock; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awcache = axi4yank_1_auto_out_awcache; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awprot = axi4yank_1_auto_out_awprot; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_awqos = axi4yank_1_auto_out_awqos; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_wvalid = axi4yank_1_auto_out_wvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_wdata = axi4yank_1_auto_out_wdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_wstrb = axi4yank_1_auto_out_wstrb; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_wlast = axi4yank_1_auto_out_wlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_bready = axi4yank_1_auto_out_bready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arvalid = axi4yank_1_auto_out_arvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arid = axi4yank_1_auto_out_arid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_araddr = axi4yank_1_auto_out_araddr; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arlen = axi4yank_1_auto_out_arlen; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arsize = axi4yank_1_auto_out_arsize; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arburst = axi4yank_1_auto_out_arburst; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arlock = axi4yank_1_auto_out_arlock; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arcache = axi4yank_1_auto_out_arcache; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arprot = axi4yank_1_auto_out_arprot; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_arqos = axi4yank_1_auto_out_arqos; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_in_rready = axi4yank_1_auto_out_rready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_awready = dmaGen_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_wready = dmaGen_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_bvalid = dmaGen_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_bid = dmaGen_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_bresp = dmaGen_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_arready = dmaGen_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_rvalid = dmaGen_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_rid = dmaGen_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_rdata = dmaGen_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_rresp = dmaGen_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_8_rlast = dmaGen_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_awready = intrGen_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_wready = intrGen_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_bvalid = intrGen_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_bid = intrGen_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_bresp = intrGen_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_arready = intrGen_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_rvalid = intrGen_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_rid = intrGen_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_rdata = intrGen_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_rresp = intrGen_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_7_rlast = intrGen_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_awready = sd_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_wready = sd_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_bvalid = sd_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_bid = sd_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_bresp = sd_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_arready = sd_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_rvalid = sd_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_rid = sd_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_rdata = sd_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_rresp = sd_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_6_rlast = sd_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_awready = flash_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_wready = flash_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_bvalid = flash_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_bid = flash_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_bresp = flash_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_arready = flash_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_rvalid = flash_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_rid = flash_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_rdata = flash_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_rresp = flash_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_5_rlast = flash_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_awready = vga_auto_in_1_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_wready = vga_auto_in_1_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_bvalid = vga_auto_in_1_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_bid = vga_auto_in_1_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_bresp = vga_auto_in_1_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_arready = vga_auto_in_1_arready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_rvalid = vga_auto_in_1_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_rid = vga_auto_in_1_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_rdata = vga_auto_in_1_rdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_rresp = vga_auto_in_1_rresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_4_rlast = vga_auto_in_1_rlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_3_awready = vga_auto_in_0_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_3_wready = vga_auto_in_0_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_3_bvalid = vga_auto_in_0_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_3_bid = vga_auto_in_0_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_3_bresp = vga_auto_in_0_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_3_rvalid = vga_auto_in_0_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_3_rid = vga_auto_in_0_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_3_rlast = vga_auto_in_0_rlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_awready = uart_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_wready = uart_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_bvalid = uart_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_bid = uart_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_bresp = uart_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_arready = uart_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_rvalid = uart_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_rid = uart_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_rdata = uart_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_rresp = uart_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_2_rlast = uart_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_awready = bootrom1_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_wready = bootrom1_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_bvalid = bootrom1_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_bid = bootrom1_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_bresp = bootrom1_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_arready = bootrom1_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_rvalid = bootrom1_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_rid = bootrom1_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_rdata = bootrom1_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_rresp = bootrom1_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_1_rlast = bootrom1_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_awready = bootrom0_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_wready = bootrom0_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_bvalid = bootrom0_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_bid = bootrom0_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_bresp = bootrom0_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_arready = bootrom0_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_rvalid = bootrom0_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_rid = bootrom0_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_rdata = bootrom0_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_rresp = bootrom0_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4xbar_auto_out_0_rlast = bootrom0_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign errorDev_clock = clock;
  assign errorDev_reset = reset;
  assign errorDev_auto_in_a_valid = xbar_auto_out_0_a_valid; // @[LazyModule.scala 296:16]
  assign errorDev_auto_in_a_bits_opcode = xbar_auto_out_0_a_bits_opcode; // @[LazyModule.scala 296:16]
  assign errorDev_auto_in_a_bits_size = xbar_auto_out_0_a_bits_size; // @[LazyModule.scala 296:16]
  assign errorDev_auto_in_a_bits_source = xbar_auto_out_0_a_bits_source; // @[LazyModule.scala 296:16]
  assign errorDev_auto_in_d_ready = xbar_auto_out_0_d_ready; // @[LazyModule.scala 296:16]
  assign xbar_clock = clock;
  assign xbar_reset = reset;
  assign xbar_auto_in_a_valid = fixer_auto_out_a_valid; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_opcode = fixer_auto_out_a_bits_opcode; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_size = fixer_auto_out_a_bits_size; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_source = fixer_auto_out_a_bits_source; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_address = fixer_auto_out_a_bits_address; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_user_amba_prot_bufferable = fixer_auto_out_a_bits_user_amba_prot_bufferable; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_user_amba_prot_modifiable = fixer_auto_out_a_bits_user_amba_prot_modifiable; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_user_amba_prot_readalloc = fixer_auto_out_a_bits_user_amba_prot_readalloc; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_user_amba_prot_writealloc = fixer_auto_out_a_bits_user_amba_prot_writealloc; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_user_amba_prot_privileged = fixer_auto_out_a_bits_user_amba_prot_privileged; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_user_amba_prot_secure = fixer_auto_out_a_bits_user_amba_prot_secure; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_user_amba_prot_fetch = fixer_auto_out_a_bits_user_amba_prot_fetch; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_mask = fixer_auto_out_a_bits_mask; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_a_bits_data = fixer_auto_out_a_bits_data; // @[LazyModule.scala 296:16]
  assign xbar_auto_in_d_ready = fixer_auto_out_d_ready; // @[LazyModule.scala 296:16]
  assign xbar_auto_out_1_a_ready = tl2axi4_auto_in_a_ready; // @[LazyModule.scala 298:16]
  assign xbar_auto_out_1_d_valid = tl2axi4_auto_in_d_valid; // @[LazyModule.scala 298:16]
  assign xbar_auto_out_1_d_bits_opcode = tl2axi4_auto_in_d_bits_opcode; // @[LazyModule.scala 298:16]
  assign xbar_auto_out_1_d_bits_size = tl2axi4_auto_in_d_bits_size; // @[LazyModule.scala 298:16]
  assign xbar_auto_out_1_d_bits_source = tl2axi4_auto_in_d_bits_source; // @[LazyModule.scala 298:16]
  assign xbar_auto_out_1_d_bits_denied = tl2axi4_auto_in_d_bits_denied; // @[LazyModule.scala 298:16]
  assign xbar_auto_out_1_d_bits_data = tl2axi4_auto_in_d_bits_data; // @[LazyModule.scala 298:16]
  assign xbar_auto_out_1_d_bits_corrupt = tl2axi4_auto_in_d_bits_corrupt; // @[LazyModule.scala 298:16]
  assign xbar_auto_out_0_a_ready = errorDev_auto_in_a_ready; // @[LazyModule.scala 296:16]
  assign xbar_auto_out_0_d_valid = errorDev_auto_in_d_valid; // @[LazyModule.scala 296:16]
  assign xbar_auto_out_0_d_bits_opcode = errorDev_auto_in_d_bits_opcode; // @[LazyModule.scala 296:16]
  assign xbar_auto_out_0_d_bits_size = errorDev_auto_in_d_bits_size; // @[LazyModule.scala 296:16]
  assign xbar_auto_out_0_d_bits_source = errorDev_auto_in_d_bits_source; // @[LazyModule.scala 296:16]
  assign xbar_auto_out_0_d_bits_corrupt = errorDev_auto_in_d_bits_corrupt; // @[LazyModule.scala 296:16]
  assign fixer_clock = clock;
  assign fixer_reset = reset;
  assign fixer_auto_in_a_valid = widget_auto_out_a_valid; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_opcode = widget_auto_out_a_bits_opcode; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_size = widget_auto_out_a_bits_size; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_source = widget_auto_out_a_bits_source; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_address = widget_auto_out_a_bits_address; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_user_amba_prot_bufferable = widget_auto_out_a_bits_user_amba_prot_bufferable; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_user_amba_prot_modifiable = widget_auto_out_a_bits_user_amba_prot_modifiable; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_user_amba_prot_readalloc = widget_auto_out_a_bits_user_amba_prot_readalloc; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_user_amba_prot_writealloc = widget_auto_out_a_bits_user_amba_prot_writealloc; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_user_amba_prot_privileged = widget_auto_out_a_bits_user_amba_prot_privileged; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_user_amba_prot_secure = widget_auto_out_a_bits_user_amba_prot_secure; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_user_amba_prot_fetch = widget_auto_out_a_bits_user_amba_prot_fetch; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_mask = widget_auto_out_a_bits_mask; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_a_bits_data = widget_auto_out_a_bits_data; // @[LazyModule.scala 296:16]
  assign fixer_auto_in_d_ready = widget_auto_out_d_ready; // @[LazyModule.scala 296:16]
  assign fixer_auto_out_a_ready = xbar_auto_in_a_ready; // @[LazyModule.scala 296:16]
  assign fixer_auto_out_d_valid = xbar_auto_in_d_valid; // @[LazyModule.scala 296:16]
  assign fixer_auto_out_d_bits_opcode = xbar_auto_in_d_bits_opcode; // @[LazyModule.scala 296:16]
  assign fixer_auto_out_d_bits_size = xbar_auto_in_d_bits_size; // @[LazyModule.scala 296:16]
  assign fixer_auto_out_d_bits_source = xbar_auto_in_d_bits_source; // @[LazyModule.scala 296:16]
  assign fixer_auto_out_d_bits_denied = xbar_auto_in_d_bits_denied; // @[LazyModule.scala 296:16]
  assign fixer_auto_out_d_bits_data = xbar_auto_in_d_bits_data; // @[LazyModule.scala 296:16]
  assign fixer_auto_out_d_bits_corrupt = xbar_auto_in_d_bits_corrupt; // @[LazyModule.scala 296:16]
  assign widget_clock = clock;
  assign widget_reset = reset;
  assign widget_auto_in_a_valid = axi42tl_auto_out_a_valid; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_opcode = axi42tl_auto_out_a_bits_opcode; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_size = axi42tl_auto_out_a_bits_size; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_source = axi42tl_auto_out_a_bits_source; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_address = axi42tl_auto_out_a_bits_address; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_user_amba_prot_bufferable = axi42tl_auto_out_a_bits_user_amba_prot_bufferable; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_user_amba_prot_modifiable = axi42tl_auto_out_a_bits_user_amba_prot_modifiable; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_user_amba_prot_readalloc = axi42tl_auto_out_a_bits_user_amba_prot_readalloc; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_user_amba_prot_writealloc = axi42tl_auto_out_a_bits_user_amba_prot_writealloc; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_user_amba_prot_privileged = axi42tl_auto_out_a_bits_user_amba_prot_privileged; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_user_amba_prot_secure = axi42tl_auto_out_a_bits_user_amba_prot_secure; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_user_amba_prot_fetch = axi42tl_auto_out_a_bits_user_amba_prot_fetch; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_mask = axi42tl_auto_out_a_bits_mask; // @[LazyModule.scala 296:16]
  assign widget_auto_in_a_bits_data = axi42tl_auto_out_a_bits_data; // @[LazyModule.scala 296:16]
  assign widget_auto_in_d_ready = axi42tl_auto_out_d_ready; // @[LazyModule.scala 296:16]
  assign widget_auto_out_a_ready = fixer_auto_in_a_ready; // @[LazyModule.scala 296:16]
  assign widget_auto_out_d_valid = fixer_auto_in_d_valid; // @[LazyModule.scala 296:16]
  assign widget_auto_out_d_bits_opcode = fixer_auto_in_d_bits_opcode; // @[LazyModule.scala 296:16]
  assign widget_auto_out_d_bits_size = fixer_auto_in_d_bits_size; // @[LazyModule.scala 296:16]
  assign widget_auto_out_d_bits_source = fixer_auto_in_d_bits_source; // @[LazyModule.scala 296:16]
  assign widget_auto_out_d_bits_denied = fixer_auto_in_d_bits_denied; // @[LazyModule.scala 296:16]
  assign widget_auto_out_d_bits_data = fixer_auto_in_d_bits_data; // @[LazyModule.scala 296:16]
  assign widget_auto_out_d_bits_corrupt = fixer_auto_in_d_bits_corrupt; // @[LazyModule.scala 296:16]
  assign axi42tl_clock = clock;
  assign axi42tl_reset = reset;
  assign axi42tl_auto_in_awvalid = axi4yank_auto_out_awvalid; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_awid = axi4yank_auto_out_awid; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_awaddr = axi4yank_auto_out_awaddr; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_awlen = axi4yank_auto_out_awlen; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_awsize = axi4yank_auto_out_awsize; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_awcache = axi4yank_auto_out_awcache; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_awprot = axi4yank_auto_out_awprot; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_wvalid = axi4yank_auto_out_wvalid; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_wdata = axi4yank_auto_out_wdata; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_wstrb = axi4yank_auto_out_wstrb; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_wlast = axi4yank_auto_out_wlast; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_bready = axi4yank_auto_out_bready; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_arvalid = axi4yank_auto_out_arvalid; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_arid = axi4yank_auto_out_arid; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_araddr = axi4yank_auto_out_araddr; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_arlen = axi4yank_auto_out_arlen; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_arsize = axi4yank_auto_out_arsize; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_arcache = axi4yank_auto_out_arcache; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_arprot = axi4yank_auto_out_arprot; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_in_rready = axi4yank_auto_out_rready; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_out_a_ready = widget_auto_in_a_ready; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_out_d_valid = widget_auto_in_d_valid; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_out_d_bits_opcode = widget_auto_in_d_bits_opcode; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_out_d_bits_size = widget_auto_in_d_bits_size; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_out_d_bits_source = widget_auto_in_d_bits_source; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_out_d_bits_denied = widget_auto_in_d_bits_denied; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_out_d_bits_data = widget_auto_in_d_bits_data; // @[LazyModule.scala 296:16]
  assign axi42tl_auto_out_d_bits_corrupt = widget_auto_in_d_bits_corrupt; // @[LazyModule.scala 296:16]
  assign axi4yank_clock = clock;
  assign axi4yank_reset = reset;
  assign axi4yank_auto_in_awvalid = io_axi4_0_awvalid; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_awid = io_axi4_0_awid; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_awaddr = io_axi4_0_awaddr; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_awlen = io_axi4_0_awlen; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_awsize = io_axi4_0_awsize; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_awcache = io_axi4_0_awcache; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_awprot = io_axi4_0_awprot; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_wvalid = io_axi4_0_wvalid; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_wdata = io_axi4_0_wdata; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_wstrb = io_axi4_0_wstrb; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_wlast = io_axi4_0_wlast; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_bready = io_axi4_0_bready; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_arvalid = io_axi4_0_arvalid; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_arid = io_axi4_0_arid; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_araddr = io_axi4_0_araddr; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_arlen = io_axi4_0_arlen; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_arsize = io_axi4_0_arsize; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_arcache = io_axi4_0_arcache; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_arprot = io_axi4_0_arprot; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_in_rready = io_axi4_0_rready; // @[Nodes.scala 1207:84 1630:60]
  assign axi4yank_auto_out_awready = axi42tl_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_wready = axi42tl_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_bvalid = axi42tl_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_bid = axi42tl_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_bresp = axi42tl_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_arready = axi42tl_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_rvalid = axi42tl_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_rid = axi42tl_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_rdata = axi42tl_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_rresp = axi42tl_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4yank_auto_out_rlast = axi42tl_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign axi4yank_1_clock = clock;
  assign axi4yank_1_reset = reset;
  assign axi4yank_1_auto_in_awvalid = tl2axi4_auto_out_awvalid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awid = tl2axi4_auto_out_awid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awaddr = tl2axi4_auto_out_awaddr; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awlen = tl2axi4_auto_out_awlen; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awsize = tl2axi4_auto_out_awsize; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awburst = tl2axi4_auto_out_awburst; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awlock = tl2axi4_auto_out_awlock; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awcache = tl2axi4_auto_out_awcache; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awprot = tl2axi4_auto_out_awprot; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awqos = tl2axi4_auto_out_awqos; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awecho_tl_state_size = tl2axi4_auto_out_awecho_tl_state_size; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_awecho_tl_state_source = tl2axi4_auto_out_awecho_tl_state_source; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_wvalid = tl2axi4_auto_out_wvalid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_wdata = tl2axi4_auto_out_wdata; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_wstrb = tl2axi4_auto_out_wstrb; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_wlast = tl2axi4_auto_out_wlast; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_bready = tl2axi4_auto_out_bready; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arvalid = tl2axi4_auto_out_arvalid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arid = tl2axi4_auto_out_arid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_araddr = tl2axi4_auto_out_araddr; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arlen = tl2axi4_auto_out_arlen; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arsize = tl2axi4_auto_out_arsize; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arburst = tl2axi4_auto_out_arburst; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arlock = tl2axi4_auto_out_arlock; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arcache = tl2axi4_auto_out_arcache; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arprot = tl2axi4_auto_out_arprot; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arqos = tl2axi4_auto_out_arqos; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arecho_tl_state_size = tl2axi4_auto_out_arecho_tl_state_size; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_arecho_tl_state_source = tl2axi4_auto_out_arecho_tl_state_source; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_in_rready = tl2axi4_auto_out_rready; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_awready = axi4xbar_auto_in_awready; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_wready = axi4xbar_auto_in_wready; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_bvalid = axi4xbar_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_bid = axi4xbar_auto_in_bid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_bresp = axi4xbar_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_arready = axi4xbar_auto_in_arready; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_rvalid = axi4xbar_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_rid = axi4xbar_auto_in_rid; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_rdata = axi4xbar_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_rresp = axi4xbar_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign axi4yank_1_auto_out_rlast = axi4xbar_auto_in_rlast; // @[LazyModule.scala 296:16]
  assign tl2axi4_clock = clock;
  assign tl2axi4_reset = reset;
  assign tl2axi4_auto_in_a_valid = xbar_auto_out_1_a_valid; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_opcode = xbar_auto_out_1_a_bits_opcode; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_size = xbar_auto_out_1_a_bits_size; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_source = xbar_auto_out_1_a_bits_source; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_address = xbar_auto_out_1_a_bits_address; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_user_amba_prot_bufferable = xbar_auto_out_1_a_bits_user_amba_prot_bufferable; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_user_amba_prot_modifiable = xbar_auto_out_1_a_bits_user_amba_prot_modifiable; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_user_amba_prot_readalloc = xbar_auto_out_1_a_bits_user_amba_prot_readalloc; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_user_amba_prot_writealloc = xbar_auto_out_1_a_bits_user_amba_prot_writealloc; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_user_amba_prot_privileged = xbar_auto_out_1_a_bits_user_amba_prot_privileged; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_user_amba_prot_secure = xbar_auto_out_1_a_bits_user_amba_prot_secure; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_user_amba_prot_fetch = xbar_auto_out_1_a_bits_user_amba_prot_fetch; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_mask = xbar_auto_out_1_a_bits_mask; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_a_bits_data = xbar_auto_out_1_a_bits_data; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_in_d_ready = xbar_auto_out_1_d_ready; // @[LazyModule.scala 298:16]
  assign tl2axi4_auto_out_awready = axi4yank_1_auto_in_awready; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_wready = axi4yank_1_auto_in_wready; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_bvalid = axi4yank_1_auto_in_bvalid; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_bid = axi4yank_1_auto_in_bid; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_bresp = axi4yank_1_auto_in_bresp; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_becho_tl_state_size = axi4yank_1_auto_in_becho_tl_state_size; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_becho_tl_state_source = axi4yank_1_auto_in_becho_tl_state_source; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_arready = axi4yank_1_auto_in_arready; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_rvalid = axi4yank_1_auto_in_rvalid; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_rid = axi4yank_1_auto_in_rid; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_rdata = axi4yank_1_auto_in_rdata; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_rresp = axi4yank_1_auto_in_rresp; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_recho_tl_state_size = axi4yank_1_auto_in_recho_tl_state_size; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_recho_tl_state_source = axi4yank_1_auto_in_recho_tl_state_source; // @[LazyModule.scala 296:16]
  assign tl2axi4_auto_out_rlast = axi4yank_1_auto_in_rlast; // @[LazyModule.scala 296:16]
endmodule

