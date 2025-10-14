package difftest.fpga.xdma
import chisel3._
import chisel3.util._

class AxisMasterBundle(val dataWidth: Int) extends Bundle {
  val valid = Output(Bool())
  val data = Output(UInt(dataWidth.W))
  val ready = Input(Bool())
  val last = Output(Bool())
}

class AXI4LiteReadMasterBundle(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val addr    = Output(UInt(addrWidth.W))
  val prot    = Output(UInt(3.W))
  val valid   = Output(Bool())
  val ready   = Input(Bool())
  val data    = Input(UInt(dataWidth.W))
}

class AXI4LiteReadSlaveBundle(addrWidth: Int, dataWidth: Int) extends Bundle {
  val master = Flipped(new AXI4LiteReadMasterBundle(addrWidth, dataWidth))
}


class AXI4LiteWriteMasterBundle(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val addr    = Output(UInt(addrWidth.W))
  val prot    = Output(UInt(3.W))
  val valid   = Output(Bool())
  val ready   = Input(Bool())
  val data    = Output(UInt(dataWidth.W))
  val strb    = Output(UInt((dataWidth/8).W))
}

class AXI4LiteWriteSlaveBundle(addrWidth: Int, dataWidth: Int) extends Bundle {
  val master = Flipped(new AXI4LiteWriteMasterBundle(addrWidth, dataWidth))
}

// AW channel: reuse AXI4Lite and extend AXI4 fields
class AXI4AWBundle(addrWidth: Int, idWidth: Int) extends Bundle {
  // AXI4 write address channel signals
  val addr  = Output(UInt(addrWidth.W))
  val prot  = Output(UInt(3.W))
  // AXI4-specific fields
  val id    = UInt(idWidth.W)
  val len   = UInt(8.W)
  val size  = UInt(3.W)
  val burst = UInt(2.W)
  val lock  = Bool()
  val cache = UInt(4.W)
  val qos   = UInt(4.W)
  val user  = UInt(1.W)
}

// W channel: reuse AXI4Lite and extend AXI4 fields
class AXI4WBundle(dataWidth: Int) extends Bundle {
  val data  = UInt(dataWidth.W)
  val strb  = UInt((dataWidth/8).W)
  val last  = Bool()
}

// B channel: reuse AXI4Lite and extend AXI4 fields
class AXI4BBundle(idWidth: Int) extends Bundle {
  val id   = UInt(idWidth.W)
  val resp = UInt(2.W)
  val user = UInt(1.W)
}

// AR channel: reuse AXI4Lite and extend AXI4 fields
class AXI4ARBundle(addrWidth: Int, idWidth: Int) extends Bundle {
  // AXI4 read address channel signals
  val addr  = Output(UInt(addrWidth.W))
  val prot  = Output(UInt(3.W))
  // AXI4-specific fields
  val id    = UInt(idWidth.W)
  val len   = UInt(8.W)
  val size  = UInt(3.W)
  val burst = UInt(2.W)
  val lock  = Bool()
  val cache = UInt(4.W)
  val qos   = UInt(4.W)
  // AXI4 user signal
  val user  = UInt(1.W)
}

class AXI4RBundle(dataWidth: Int, idWidth: Int) extends Bundle {
  val id   = UInt(idWidth.W)
  val data = UInt(dataWidth.W)
  val resp = UInt(2.W)
  val last = Bool()
  val user = UInt(1.W)
}

class AXI4(val addrWidth: Int = 32, val dataWidth: Int = 64, val idWidth: Int = 4) extends Bundle {
  val aw = Decoupled(new AXI4AWBundle(addrWidth, idWidth))
  val w  = Decoupled(new AXI4WBundle(dataWidth))
  val b  = Flipped(Decoupled(new AXI4BBundle(idWidth)))
  val ar = Decoupled(new AXI4ARBundle(addrWidth, idWidth))
  val r  = Flipped(Decoupled(new AXI4RBundle(dataWidth, idWidth)))
}

class AXI4Arbiter(addrWidth: Int = 32, dataWidth: Int = 64, idWidth: Int = 4) extends Module {
  val io = IO(new Bundle {
    val in  = Vec(2, Flipped(new AXI4(addrWidth, dataWidth, idWidth)))
    val out = new AXI4(addrWidth, dataWidth, idWidth)
  })

  // Simple, safe arbitration without interleaving across transactions.
  // Write path: lock from AW handshake until B response completes.
  val writeActive = RegInit(false.B)
  val writeOwner  = RegInit(0.U(1.W))

  val aw_sel_comb = Mux(io.in(0).aw.valid, 0.U, 1.U)
  val aw_any      = io.in(0).aw.valid || io.in(1).aw.valid
  val aw_sel_eff  = Mux(writeActive, writeOwner, aw_sel_comb)

  // AW
  io.out.aw.valid := Mux(writeActive, io.in(writeOwner).aw.valid, aw_any)
  io.out.aw.bits  := io.in(aw_sel_eff).aw.bits
  io.in(0).aw.ready := (aw_sel_eff === 0.U) && io.out.aw.ready
  io.in(1).aw.ready := (aw_sel_eff === 1.U) && io.out.aw.ready
  when(io.out.aw.fire) {
    writeActive := true.B
    writeOwner  := aw_sel_eff
  }

  // W: only accept from current writeOwner until W.last; still keep writeActive until B completes
  io.out.w.valid := writeActive && io.in(writeOwner).w.valid
  io.out.w.bits  := io.in(writeOwner).w.bits
  io.in(0).w.ready := (writeOwner === 0.U) && io.out.w.ready
  io.in(1).w.ready := (writeOwner === 1.U) && io.out.w.ready

  // B routed to current writeOwner; release when B handshake completes
  io.in(0).b.valid := io.out.b.valid && writeActive && (writeOwner === 0.U)
  io.in(1).b.valid := io.out.b.valid && writeActive && (writeOwner === 1.U)
  io.in(0).b.bits  := io.out.b.bits
  io.in(1).b.bits  := io.out.b.bits
  io.out.b.ready   := Mux(writeOwner === 0.U, io.in(0).b.ready, io.in(1).b.ready)
  when(io.out.b.fire) { writeActive := false.B }

  // Read path: lock from AR handshake until R.last
  val readActive = RegInit(false.B)
  val readOwner  = RegInit(0.U(1.W))

  val ar_sel_comb = Mux(io.in(0).ar.valid, 0.U, 1.U)
  val ar_any      = io.in(0).ar.valid || io.in(1).ar.valid
  val ar_sel_eff  = Mux(readActive, readOwner, ar_sel_comb)

  // AR
  io.out.ar.valid := Mux(readActive, io.in(readOwner).ar.valid, ar_any)
  io.out.ar.bits  := io.in(ar_sel_eff).ar.bits
  io.in(0).ar.ready := (ar_sel_eff === 0.U) && io.out.ar.ready
  io.in(1).ar.ready := (ar_sel_eff === 1.U) && io.out.ar.ready
  when(io.out.ar.fire) {
    readActive := true.B
    readOwner  := ar_sel_eff
  }

  // R
  io.in(0).r.valid := io.out.r.valid && readActive && (readOwner === 0.U)
  io.in(1).r.valid := io.out.r.valid && readActive && (readOwner === 1.U)
  io.in(0).r.bits  := io.out.r.bits
  io.in(1).r.bits  := io.out.r.bits
  io.out.r.ready   := Mux(readOwner === 0.U, io.in(0).r.ready, io.in(1).r.ready)
  when(io.out.r.fire && io.out.r.bits.last) { readActive := false.B }
}

class XDMA_AXI4LiteBar(addrWidth: Int = 32, dataWidth: Int = 32) extends Module {
  val io = IO(new Bundle {
  val axi_write = new AXI4LiteWriteSlaveBundle(addrWidth, dataWidth)
  val axi_read  = new AXI4LiteReadSlaveBundle(addrWidth, dataWidth)
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
  io.axi_write.master.ready := awready_r && wready_r

  val arready_r = RegInit(true.B)
  val rvalid_r  = RegInit(false.B)
  val araddr_r  = Reg(UInt(addrWidth.W))
  io.axi_read.master.ready := arready_r

  // register index computation (divide byte addr by 4)
  val bytesPerReg = 4
  val idxBits = log2Ceil(regfile.length)
  val write_idx = (awaddr_r >> 2)(idxBits-1, 0)
  val read_idx  = (araddr_r  >> 2)(idxBits-1, 0)

  // Write FSM: capture AW, accept W, update regfile only if index < regfile.length
  when (io.axi_write.master.valid && awready_r) {
    awaddr_r := io.axi_write.master.addr
    awready_r := false.B
  } .elsewhen (io.axi_write.master.valid && wready_r) {
    // compute index and write only if in range; otherwise ignore data but complete handshake
    when ((awaddr_r >> 2) < regfile.length.U) {
      regfile((awaddr_r >> 2)(idxBits-1,0)) := io.axi_write.master.data
    }
    wready_r := false.B
    bvalid_r := true.B
  } .elsewhen (bvalid_r && io.axi_write.master.ready) {
    bvalid_r := false.B
    awready_r := true.B
    wready_r := true.B
  }

  // Read FSM
  when (io.axi_read.master.valid && arready_r) {
    araddr_r := io.axi_read.master.addr
    arready_r := false.B
    rvalid_r := true.B
  } .elsewhen (rvalid_r && io.axi_read.master.ready) {
    rvalid_r := false.B
    arready_r := true.B
  }

  // Read mapping: return regfile[index] if in range, else zero
  io.axi_read.master.data := Mux((araddr_r >> 2) < regfile.length.U, regfile((araddr_r >> 2)(idxBits-1,0)), 0.U)
}

class XDMA_AxisToAxi4(addrWidth: Int = 64, dataWidth: Int = 512) extends Module {
  val io = IO(new Bundle {
    val axis = Flipped(new AxisMasterBundle(dataWidth))
    val aw   = Decoupled(new AXI4AWBundle(addrWidth, 4))
    val w    = Decoupled(new AXI4WBundle(dataWidth))
    val b    = Flipped(Decoupled(new AXI4BBundle(4)))
  })

  // bytes per beat
  val bytesPerBeat = (dataWidth/8).U

  val addr_ptr = RegInit(0.U(addrWidth.W))
  val axis_data_reg = Reg(UInt(dataWidth.W))

  // AW channel
  val awaddr_r  = Reg(UInt(addrWidth.W))
  val awvalid_r = RegInit(false.B)
  // W channel
  val wdata_r   = Reg(UInt(dataWidth.W))
  val wstrb_r   = Reg(UInt((dataWidth/8).W))
  val wvalid_r  = RegInit(false.B)
  // B channel
  val bready_r  = RegInit(false.B)

  // AXIS ready: only accept new data when both AW/W channels are idle
  io.axis.ready := !awvalid_r && !wvalid_r

  // AXIS data sampling
  when (io.axis.valid && io.axis.ready) {
    awaddr_r := addr_ptr
    wdata_r  := io.axis.data
    wstrb_r  := Fill(dataWidth/8, 1.U(1.W))
    addr_ptr := addr_ptr + bytesPerBeat
    awvalid_r := true.B
    wvalid_r  := true.B
    bready_r  := false.B
  }

  // AW channel handshake
  when (awvalid_r && io.aw.ready) {
    awvalid_r := false.B
  }

  // W channel handshake
  when (wvalid_r && io.w.ready) {
    wvalid_r := false.B
    bready_r := true.B
  }

  // B channel handshake
  when (bready_r && io.b.valid) {
    bready_r := false.B
  }

  // Port connections
  io.aw.valid := awvalid_r
  io.aw.bits.addr := awaddr_r
  io.aw.bits.id := 0.U
  io.aw.bits.len := 0.U
  io.aw.bits.size := log2Ceil(dataWidth/8).U
  io.aw.bits.burst := 1.U
  io.aw.bits.lock := false.B
  io.aw.bits.cache := 0.U
  io.aw.bits.prot := 0.U
  io.aw.bits.qos := 0.U
  io.aw.bits.user := 0.U

  io.w.valid := wvalid_r
  io.w.bits.data  := wdata_r
  io.w.bits.strb  := wstrb_r
  io.w.bits.last := true.B

  io.b.ready := bready_r
}
