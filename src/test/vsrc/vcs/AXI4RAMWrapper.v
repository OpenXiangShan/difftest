module AXI4RAMWrapper(
  input          clock,
  input          reset,
  output         io_axi4_0_awready,
  input          io_axi4_0_awvalid,
  input  [5:0]   io_axi4_0_awid,
  input  [37:0]  io_axi4_0_awaddr,
  input  [7:0]   io_axi4_0_awlen,
  input  [2:0]   io_axi4_0_awsize,
  input  [1:0]   io_axi4_0_awburst,
  input          io_axi4_0_awlock,
  input  [3:0]   io_axi4_0_awcache,
  input  [2:0]   io_axi4_0_awprot,
  input  [3:0]   io_axi4_0_awqos,
  output         io_axi4_0_wready,
  input          io_axi4_0_wvalid,
  input  [255:0] io_axi4_0_wdata,
  input  [31:0]  io_axi4_0_wstrb,
  input          io_axi4_0_wlast,
  input          io_axi4_0_bready,
  output         io_axi4_0_bvalid,
  output [5:0]   io_axi4_0_bid,
  output [1:0]   io_axi4_0_bresp,
  output         io_axi4_0_arready,
  input          io_axi4_0_arvalid,
  input  [5:0]   io_axi4_0_arid,
  input  [37:0]  io_axi4_0_araddr,
  input  [7:0]   io_axi4_0_arlen,
  input  [2:0]   io_axi4_0_arsize,
  input  [1:0]   io_axi4_0_arburst,
  input          io_axi4_0_arlock,
  input  [3:0]   io_axi4_0_arcache,
  input  [2:0]   io_axi4_0_arprot,
  input  [3:0]   io_axi4_0_arqos,
  input          io_axi4_0_rready,
  output         io_axi4_0_rvalid,
  output [5:0]   io_axi4_0_rid,
  output [255:0] io_axi4_0_rdata,
  output [1:0]   io_axi4_0_rresp,
  output         io_axi4_0_rlast
);
  wire  ram_clock; // @[AXI4RAM.scala 106:23]
  wire  ram_reset; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_awready; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_awvalid; // @[AXI4RAM.scala 106:23]
  wire [5:0] ram_auto_in_awid; // @[AXI4RAM.scala 106:23]
  wire [37:0] ram_auto_in_awaddr; // @[AXI4RAM.scala 106:23]
  wire [7:0] ram_auto_in_awlen; // @[AXI4RAM.scala 106:23]
  wire [2:0] ram_auto_in_awsize; // @[AXI4RAM.scala 106:23]
  wire [1:0] ram_auto_in_awburst; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_awlock; // @[AXI4RAM.scala 106:23]
  wire [3:0] ram_auto_in_awcache; // @[AXI4RAM.scala 106:23]
  wire [2:0] ram_auto_in_awprot; // @[AXI4RAM.scala 106:23]
  wire [3:0] ram_auto_in_awqos; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_wready; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_wvalid; // @[AXI4RAM.scala 106:23]
  wire [255:0] ram_auto_in_wdata; // @[AXI4RAM.scala 106:23]
  wire [31:0] ram_auto_in_wstrb; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_wlast; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_bready; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_bvalid; // @[AXI4RAM.scala 106:23]
  wire [5:0] ram_auto_in_bid; // @[AXI4RAM.scala 106:23]
  wire [1:0] ram_auto_in_bresp; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_arready; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_arvalid; // @[AXI4RAM.scala 106:23]
  wire [5:0] ram_auto_in_arid; // @[AXI4RAM.scala 106:23]
  wire [37:0] ram_auto_in_araddr; // @[AXI4RAM.scala 106:23]
  wire [7:0] ram_auto_in_arlen; // @[AXI4RAM.scala 106:23]
  wire [2:0] ram_auto_in_arsize; // @[AXI4RAM.scala 106:23]
  wire [1:0] ram_auto_in_arburst; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_arlock; // @[AXI4RAM.scala 106:23]
  wire [3:0] ram_auto_in_arcache; // @[AXI4RAM.scala 106:23]
  wire [2:0] ram_auto_in_arprot; // @[AXI4RAM.scala 106:23]
  wire [3:0] ram_auto_in_arqos; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_rready; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_rvalid; // @[AXI4RAM.scala 106:23]
  wire [5:0] ram_auto_in_rid; // @[AXI4RAM.scala 106:23]
  wire [255:0] ram_auto_in_rdata; // @[AXI4RAM.scala 106:23]
  wire [1:0] ram_auto_in_rresp; // @[AXI4RAM.scala 106:23]
  wire  ram_auto_in_rlast; // @[AXI4RAM.scala 106:23]
  AXI4RAM_1 ram ( // @[AXI4RAM.scala 106:23]
    .clock(ram_clock),
    .reset(ram_reset),
    .auto_in_awready(ram_auto_in_awready),
    .auto_in_awvalid(ram_auto_in_awvalid),
    .auto_in_awid(ram_auto_in_awid),
    .auto_in_awaddr(ram_auto_in_awaddr),
    .auto_in_awlen(ram_auto_in_awlen),
    .auto_in_awsize(ram_auto_in_awsize),
    .auto_in_awburst(ram_auto_in_awburst),
    .auto_in_awlock(ram_auto_in_awlock),
    .auto_in_awcache(ram_auto_in_awcache),
    .auto_in_awprot(ram_auto_in_awprot),
    .auto_in_awqos(ram_auto_in_awqos),
    .auto_in_wready(ram_auto_in_wready),
    .auto_in_wvalid(ram_auto_in_wvalid),
    .auto_in_wdata(ram_auto_in_wdata),
    .auto_in_wstrb(ram_auto_in_wstrb),
    .auto_in_wlast(ram_auto_in_wlast),
    .auto_in_bready(ram_auto_in_bready),
    .auto_in_bvalid(ram_auto_in_bvalid),
    .auto_in_bid(ram_auto_in_bid),
    .auto_in_bresp(ram_auto_in_bresp),
    .auto_in_arready(ram_auto_in_arready),
    .auto_in_arvalid(ram_auto_in_arvalid),
    .auto_in_arid(ram_auto_in_arid),
    .auto_in_araddr(ram_auto_in_araddr),
    .auto_in_arlen(ram_auto_in_arlen),
    .auto_in_arsize(ram_auto_in_arsize),
    .auto_in_arburst(ram_auto_in_arburst),
    .auto_in_arlock(ram_auto_in_arlock),
    .auto_in_arcache(ram_auto_in_arcache),
    .auto_in_arprot(ram_auto_in_arprot),
    .auto_in_arqos(ram_auto_in_arqos),
    .auto_in_rready(ram_auto_in_rready),
    .auto_in_rvalid(ram_auto_in_rvalid),
    .auto_in_rid(ram_auto_in_rid),
    .auto_in_rdata(ram_auto_in_rdata),
    .auto_in_rresp(ram_auto_in_rresp),
    .auto_in_rlast(ram_auto_in_rlast)
  );
  assign io_axi4_0_awready = ram_auto_in_awready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_wready = ram_auto_in_wready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_bvalid = ram_auto_in_bvalid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_bid = ram_auto_in_bid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_bresp = ram_auto_in_bresp; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_arready = ram_auto_in_arready; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rvalid = ram_auto_in_rvalid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rid = ram_auto_in_rid; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rdata = ram_auto_in_rdata; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rresp = ram_auto_in_rresp; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign io_axi4_0_rlast = ram_auto_in_rlast; // @[Nodes.scala 1207:84 LazyModule.scala 298:16]
  assign ram_clock = clock;
  assign ram_reset = reset;
  assign ram_auto_in_awvalid = io_axi4_0_awvalid; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awid = io_axi4_0_awid; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awaddr = io_axi4_0_awaddr; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awlen = io_axi4_0_awlen; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awsize = io_axi4_0_awsize; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awburst = io_axi4_0_awburst; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awlock = io_axi4_0_awlock; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awcache = io_axi4_0_awcache; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awprot = io_axi4_0_awprot; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_awqos = io_axi4_0_awqos; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_wvalid = io_axi4_0_wvalid; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_wdata = io_axi4_0_wdata; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_wstrb = io_axi4_0_wstrb; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_wlast = io_axi4_0_wlast; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_bready = io_axi4_0_bready; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arvalid = io_axi4_0_arvalid; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arid = io_axi4_0_arid; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_araddr = io_axi4_0_araddr; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arlen = io_axi4_0_arlen; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arsize = io_axi4_0_arsize; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arburst = io_axi4_0_arburst; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arlock = io_axi4_0_arlock; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arcache = io_axi4_0_arcache; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arprot = io_axi4_0_arprot; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_arqos = io_axi4_0_arqos; // @[Nodes.scala 1207:84 1630:60]
  assign ram_auto_in_rready = io_axi4_0_rready; // @[Nodes.scala 1207:84 1630:60]
endmodule

