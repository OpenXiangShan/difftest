package difftest.fpga.xdma
import chisel3._
import chisel3.util._
import util.Decoupled

class AXI4StreamBundle(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val last = Bool()
}

class AXI4Stream(val dataWidth: Int) extends DecoupledIO(new AXI4StreamBundle(dataWidth))

// AXI4-Lite channel bundles (standard five channels) in the same style as AXI4 below
class AXI4LiteABundle(val addrWidth: Int) extends Bundle {
  val addr = UInt(addrWidth.W)
  val prot = UInt(3.W)
}

class AXI4LiteWBundle(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val strb = UInt((dataWidth/8).W)
}

class AXI4LiteBBundle extends Bundle {
  val resp = UInt(2.W) // OKAY=0, SLVERR=2, DECERR=3
}

class AXI4LiteRBundle(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val resp = UInt(2.W)
}

class AXI4Lite(val addrWidth: Int = 32, val dataWidth: Int = 32) extends Bundle {
  val aw = Decoupled(new AXI4LiteABundle(addrWidth))
  val w  = Decoupled(new AXI4LiteWBundle(dataWidth))
  val b  = Flipped(Decoupled(new AXI4LiteBBundle))
  val ar = Decoupled(new AXI4LiteABundle(addrWidth))
  val r  = Flipped(Decoupled(new AXI4LiteRBundle(dataWidth)))
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
    val axi = Flipped(new AXI4Lite(addrWidth, dataWidth))
    val host_reset = Output(Bool())
    val host_ddraxi_addr_reset = Output(Bool())
  })

  // Only regfile[0] is used
  val regfile = RegInit(VecInit(Seq.fill(8)(0.U(dataWidth.W))))
  io.host_reset := regfile(0)(0)
  io.host_ddraxi_addr_reset := regfile(1)(0)

  // Common parameters
  val wordShift    = log2Ceil(dataWidth/8)
  val idxBits      = log2Ceil(regfile.length)

  // -----------------
  // Write Address (AW)
  // -----------------
  val awaddr_r   = Reg(UInt(addrWidth.W))
  val aw_captured = RegInit(false.B)
  io.axi.aw.ready := !aw_captured
  when (io.axi.aw.fire) {
    awaddr_r    := io.axi.aw.bits.addr
    aw_captured := true.B
  }

  // ---------
  // Write (W)
  // ---------
  val wdata_r     = Reg(UInt(dataWidth.W))
  val wstrb_r     = Reg(UInt((dataWidth/8).W))
  val w_captured  = RegInit(false.B)
  io.axi.w.ready := !w_captured
  when (io.axi.w.fire) {
    wdata_r    := io.axi.w.bits.data
    wstrb_r    := io.axi.w.bits.strb
    w_captured := true.B
  }

  // ------
  // Resp B
  // ------
  val bvalid_r = RegInit(false.B)
  val bresp_r  = RegInit(0.U(2.W)) // OKAY
  io.axi.b.valid := bvalid_r
  io.axi.b.bits.resp := bresp_r

  // Do the write once we have both AW and W captured and no outstanding B
  val haveWrite = aw_captured && w_captured && !bvalid_r
  when (haveWrite) {
    val aindex = (awaddr_r >> wordShift)(idxBits-1, 0)
    val inRange = (awaddr_r >> wordShift) < regfile.length.U

    when (inRange) {
      // Byte write mask from strobe
      val wmask = Cat((0 until dataWidth/8).map(i => Fill(8, wstrb_r(i))).reverse)
      val old   = regfile(aindex)
      val next  = (old & ~wmask) | (wdata_r & wmask)
      regfile(aindex) := next
      bresp_r := 0.U // OKAY
    } .otherwise {
      bresp_r := 2.U // SLVERR for out-of-range
    }
    bvalid_r   := true.B
    aw_captured := false.B
    w_captured  := false.B
  }

  when (io.axi.b.fire) { bvalid_r := false.B }

  // ---------------
  // Read Address AR
  // ---------------
  val araddr_r   = Reg(UInt(addrWidth.W))
  val ar_captured = RegInit(false.B)
  io.axi.ar.ready := !ar_captured
  when (io.axi.ar.fire) {
    araddr_r    := io.axi.ar.bits.addr
    ar_captured := true.B
  }

  // ----------
  // Read Data R
  // ----------
  val rvalid_r = RegInit(false.B)
  val rdata_r  = Reg(UInt(dataWidth.W))
  val rresp_r  = RegInit(0.U(2.W))
  io.axi.r.valid := rvalid_r
  io.axi.r.bits.data := rdata_r
  io.axi.r.bits.resp := rresp_r

  when (ar_captured && !rvalid_r) {
    val aindex = (araddr_r >> wordShift)(idxBits-1, 0)
    val inRange = (araddr_r >> wordShift) < regfile.length.U
    rdata_r := Mux(inRange, regfile(aindex), 0.U)
    rresp_r := Mux(inRange, 0.U, 2.U) // OKAY or SLVERR
    rvalid_r := true.B
    ar_captured := false.B
  }
  when (io.axi.r.fire) { rvalid_r := false.B }
}

class XDMA_AxisToAxi4(addrWidth: Int = 64, dataWidth: Int = 512) extends Module {
  val io = IO(new Bundle {
    val axis = Flipped(new AXI4Stream(dataWidth))
    val aw   = Decoupled(new AXI4AWBundle(addrWidth, 4))
    val w    = Decoupled(new AXI4WBundle(dataWidth))
    val b    = Flipped(Decoupled(new AXI4BBundle(4)))
  })

  // bytes per beat
  val bytesPerBeat = (dataWidth/8).U

  // Address pointer increases per accepted AXIS beat
  val addr_ptr = RegInit(0.U(addrWidth.W))

  // One-beat buffer to decouple AXIS from AXI4
  val buf_valid = RegInit(false.B)
  val buf_data  = Reg(UInt(dataWidth.W))
  val buf_addr  = Reg(UInt(addrWidth.W))

  // Track progress of current AXI write and outstanding B
  val aw_sent   = RegInit(false.B)
  val w_sent    = RegInit(false.B)
  val b_pending = RegInit(false.B)

  // Busy when a transaction is in-flight (from accept until B response completes)
  val busy = buf_valid || aw_sent || w_sent || b_pending

  // Accept AXIS only when AXI is idle (no outstanding write)
  io.axis.ready := !busy

  // Capture incoming AXIS beat and start a new AXI write
  when (io.axis.valid && io.axis.ready) {
    buf_valid := true.B
    buf_data  := io.axis.bits.data
    buf_addr  := addr_ptr
    addr_ptr  := addr_ptr + bytesPerBeat
    aw_sent   := false.B
    w_sent    := false.B
    b_pending := false.B
  }

  // Drive AW when we have buffered data and AW not yet sent
  io.aw.valid := buf_valid && !aw_sent
  io.aw.bits.addr  := buf_addr
  io.aw.bits.id    := 0.U
  io.aw.bits.len   := 0.U // single beat
  io.aw.bits.size  := log2Ceil(dataWidth/8).U
  io.aw.bits.burst := 1.U // INCR
  io.aw.bits.lock  := false.B
  io.aw.bits.cache := 0.U
  io.aw.bits.prot  := 0.U
  io.aw.bits.qos   := 0.U
  io.aw.bits.user  := 0.U
  when (io.aw.fire) { aw_sent := true.B }

  // Drive W when we have buffered data and W not yet sent
  io.w.valid := buf_valid && !w_sent
  io.w.bits.data := buf_data
  io.w.bits.strb := Fill(dataWidth/8, 1.U(1.W))
  io.w.bits.last := true.B
  when (io.w.fire) { w_sent := true.B }

  // Once both AW and W have been sent, clear buffer and wait for B
  when (buf_valid && aw_sent && w_sent) {
    buf_valid := false.B
    b_pending := true.B
  }

  // Ready for B only when waiting for it
  io.b.ready := b_pending
  when (io.b.valid && io.b.ready) {
    b_pending := false.B
  }
}

// Convert 512-bit AXIS stream to 64-bit AXI write-only transactions using xdma_axi bundles.
class Axis512ToAxi64Write(addrWidth: Int = 64) extends Module {
  val axisWidth = 512
  val axiDataWidth = 64
  val bytesPerBeat = axiDataWidth / 8
  val subBeatsPerAxis = axisWidth / axiDataWidth // 8

  val io = IO(new Bundle {
    val axis = Flipped(new AXI4Stream(axisWidth))
    val axi  = new AXI4(addrWidth = addrWidth, dataWidth = axiDataWidth, idWidth = 1)
  })

  // Default tie-offs for unused read channels
  io.axi.ar.valid := false.B
  io.axi.ar.bits.addr  := 0.U
  io.axi.ar.bits.prot  := 0.U
  io.axi.ar.bits.id    := 0.U
  io.axi.ar.bits.len   := 0.U
  io.axi.ar.bits.size  := log2Ceil(bytesPerBeat).U
  io.axi.ar.bits.burst := 1.U
  io.axi.ar.bits.lock  := false.B
  io.axi.ar.bits.cache := 0.U
  io.axi.ar.bits.qos   := 0.U
  io.axi.ar.bits.user  := 0.U
  io.axi.r.ready := true.B

  // State for 8-beat burst write
  val addrPtr     = RegInit(0.U(addrWidth.W))
  val axisReg     = Reg(UInt(axisWidth.W))
  val beatCnt     = RegInit(0.U(log2Ceil(subBeatsPerAxis).W)) // 0..7

  // Burst-level sequencing: Idle -> AW(len=7) -> send 8 W beats -> wait for B -> Idle
  val sIdle :: sAw :: sW :: sB :: Nil = Enum(4)
  val state = RegInit(sIdle)

  // Accept one 512-bit AXIS word only when idle; each word maps to one 8-beat AXI INCR burst
  io.axis.ready := (state === sIdle)
  val takeAxis = io.axis.valid && io.axis.ready


  // Default values for AW/W/B to avoid latches
  io.axi.aw.valid := false.B
  io.axi.aw.bits.addr  := addrPtr
  io.axi.aw.bits.prot  := 0.U
  io.axi.aw.bits.id    := 0.U
  io.axi.aw.bits.len   := (subBeatsPerAxis - 1).U // 8-beat burst -> len=7
  io.axi.aw.bits.size  := log2Ceil(bytesPerBeat).U // 64-bit
  io.axi.aw.bits.burst := 1.U // INCR
  io.axi.aw.bits.lock  := false.B
  io.axi.aw.bits.cache := 0.U
  io.axi.aw.bits.qos   := 0.U
  io.axi.aw.bits.user  := 0.U

  io.axi.w.valid := false.B
  io.axi.w.bits.data := 0.U
  io.axi.w.bits.strb := ((BigInt(1) << bytesPerBeat) - 1).U
  io.axi.w.bits.last := false.B

  io.axi.b.ready := true.B

  switch(state) {
    is(sIdle) {
      when(takeAxis) {
        axisReg := io.axis.bits.data
        beatCnt := 0.U
        state   := sAw
      }
    }
    is(sAw) {
      io.axi.aw.valid := true.B
      when(io.axi.aw.fire) { state := sW }
    }
    is(sW) {
      io.axi.w.valid := true.B
      // Send least-significant 64b first, shift right by 64b per beat
      io.axi.w.bits.data := axisReg(axiDataWidth-1, 0)
      io.axi.w.bits.last := (beatCnt === (subBeatsPerAxis-1).U) // assert last on 8th beat
      when(io.axi.w.fire) {
        when(beatCnt === (subBeatsPerAxis-1).U) {
          state := sB
        }.otherwise {
          beatCnt := beatCnt + 1.U
          axisReg := axisReg >> axiDataWidth
        }
      }
    }
    is(sB) {
      when(io.axi.b.valid && io.axi.b.ready) {
        // One 8-beat burst finished; advance write address by 64B (next 512b window)
        addrPtr := addrPtr + (axisWidth/8).U
        // Reset and return to idle
        state   := sIdle
      }
    }
  }
}
