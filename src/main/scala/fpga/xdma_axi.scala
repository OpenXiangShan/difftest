package xdma
import chisel3._
import chisel3.util._

// AXIS interface Bundle
class AxisBundle(val dataWidth: Int) extends Bundle {
  val valid = Input(Bool())
  val data  = Input(UInt(dataWidth.W))
  val ready = Output(Bool())
}


// AXI4Lite read interface for master
class Axi4LiteReadMasterBundle(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val araddr  = Output(UInt(addrWidth.W))
  val arvalid = Output(Bool())
  val arready = Input(Bool())
  val rdata   = Input(UInt(dataWidth.W))
  val rresp   = Input(UInt(2.W))
  val rvalid  = Input(Bool())
  val rready  = Output(Bool())
}

// AXI4Lite read interface for slave
class Axi4LiteReadSlaveBundle(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val araddr  = Input(UInt(addrWidth.W))
  val arvalid = Input(Bool())
  val arready = Output(Bool())
  val rdata   = Output(UInt(dataWidth.W))
  val rresp   = Output(UInt(2.W))
  val rvalid  = Output(Bool())
  val rready  = Input(Bool())
}


class Axi4LiteWriteMasterBundle(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val awaddr  = Output(UInt(addrWidth.W))
  val awvalid = Output(Bool())
  val awready = Input(Bool())
  val wdata   = Output(UInt(dataWidth.W))
  val wstrb   = Output(UInt((dataWidth/8).W))
  val wvalid  = Output(Bool())
  val wready  = Input(Bool())
  val bresp   = Input(UInt(2.W))
  val bvalid  = Input(Bool())
  val bready  = Output(Bool())
}

// AXI4Lite write interface for slave
class Axi4LiteWriteSlaveBundle(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val awaddr  = Input(UInt(addrWidth.W))
  val awvalid = Input(Bool())
  val awready = Output(Bool())
  val wdata   = Input(UInt(dataWidth.W))
  val wstrb   = Input(UInt((dataWidth/8).W))
  val wvalid  = Input(Bool())
  val wready  = Output(Bool())
  val bresp   = Output(UInt(2.W))
  val bvalid  = Output(Bool())
  val bready  = Input(Bool())
}

object XDMA_AXIFactory {
  def genAxi4LiteBar(addrWidth: Int = 32, dataWidth: Int = 32): XDMA_AXI4LiteBar = {
    Module(new XDMA_AXI4LiteBar(addrWidth, dataWidth))
  }

  def genAxisToAxi4Lite(addrWidth: Int = 64, dataWidth: Int = 512): XDMA_AxisToAxi4Lite = {
    Module(new XDMA_AxisToAxi4Lite(addrWidth, dataWidth))
  }
}


class XDMA_AXI4LiteBar(addrWidth: Int = 32, dataWidth: Int = 32) extends Module {
  val io = IO(new Bundle {
    val axi_write = new Axi4LiteWriteSlaveBundle(addrWidth, dataWidth)
    val axi_read  = new Axi4LiteReadSlaveBundle(addrWidth, dataWidth)
    val host_reset = Output(Bool())
    val host_ddraxi_addr_reset = Output(Bool())
  })

  // Only regfile[0] is used
  val regfile = RegInit(VecInit(Seq.fill(8)(0.U(dataWidth.W))))
  io.host_reset := regfile(0)(0)
  io.host_ddraxi_addr_reset := regfile(1)(0)

  // Handshake signals
  val awready_r = RegInit(true.B)
  val wready_r  = RegInit(true.B)
  val bvalid_r  = RegInit(false.B)
  val awaddr_r  = Reg(UInt(addrWidth.W))
  io.axi_write.awready := awready_r
  io.axi_write.wready  := wready_r
  io.axi_write.bvalid  := bvalid_r
  io.axi_write.bresp   := 0.U // OKAY

  val arready_r = RegInit(true.B)
  val rvalid_r  = RegInit(false.B)
  val araddr_r  = Reg(UInt(addrWidth.W))
  io.axi_read.arready := arready_r
  io.axi_read.rvalid  := rvalid_r
  io.axi_read.rresp   := 0.U // OKAY

  // register index computation (divide byte addr by 4)
  val bytesPerReg = 4
  val idxBits = log2Ceil(regfile.length)
  val write_idx = (awaddr_r >> 2)(idxBits-1, 0)
  val read_idx  = (araddr_r  >> 2)(idxBits-1, 0)

  // Write FSM: capture AW, accept W, update regfile only if index < regfile.length
  when (io.axi_write.awvalid && awready_r) {
    awaddr_r := io.axi_write.awaddr
    awready_r := false.B
  } .elsewhen (io.axi_write.wvalid && wready_r) {
    // compute index and write only if in range; otherwise ignore data but complete handshake
    when ((awaddr_r >> 2) < regfile.length.U) {
      regfile((awaddr_r >> 2)(idxBits-1,0)) := io.axi_write.wdata
    }
    wready_r := false.B
    bvalid_r := true.B
  } .elsewhen (bvalid_r && io.axi_write.bready) {
    bvalid_r := false.B
    awready_r := true.B
    wready_r := true.B
  }

  // Read FSM
  when (io.axi_read.arvalid && arready_r) {
    araddr_r := io.axi_read.araddr
    arready_r := false.B
    rvalid_r := true.B
  } .elsewhen (rvalid_r && io.axi_read.rready) {
    rvalid_r := false.B
    arready_r := true.B
  }

  // Read mapping: return regfile[index] if in range, else zero
  io.axi_read.rdata := Mux((araddr_r >> 2) < regfile.length.U, regfile((araddr_r >> 2)(idxBits-1,0)), 0.U)
}

class XDMA_AxisToAxi4Lite(addrWidth: Int = 64, dataWidth: Int = 512) extends Module {
  val io = IO(new Bundle {
    val axis = new AxisBundle(dataWidth)
    val axi  = new Axi4LiteWriteMasterBundle(addrWidth, dataWidth)
    val addr_reset = Input(Bool())
  })

  val addr = RegInit(0.U(addrWidth.W))
  when(io.addr_reset) {
    addr := 0.U
  }.elsewhen(io.axis.valid && io.axis.ready) {
    addr := addr + (dataWidth / 8).U
  }

  io.axis.ready := io.axi.awready && io.axi.wready

  io.axi.awaddr  := addr
  io.axi.awvalid := io.axis.valid && io.axis.ready
  io.axi.wdata   := io.axis.data
  io.axi.wstrb   := Fill(dataWidth/8, 1.U(1.W)) // All bytes valid
  io.axi.wvalid  := io.axis.valid && io.axis.ready
  io.axi.bready  := true.B
}
