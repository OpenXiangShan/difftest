/***************************************************************************************
 * Copyright (c) 2026 Beijing Institute of Open Source Chip
 *
 * DiffTest is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

package difftest.fpga

import chisel3._
import chisel3.util._
import difftest.DifftestMemIO
import difftest.common.{AXI4Bundle, AXI4Stream, AXI4StreamBundle}

object MemInitState extends ChiselEnum {
  val sIdle, sSetup, sAddr, sData, sResp, sDone = Value
}

object H2CAXIs2MemState extends ChiselEnum {
  val sIdle, sReadPayload, sAddr, sData, sResp, sDone = Value
}

class AsyncClockFIFO[T <: Data](gen: T, depth: Int) extends Module {
  require(isPow2(depth), s"AsyncClockFIFO depth must be power-of-two, got $depth")
  require(depth >= 4, s"AsyncClockFIFO depth must be at least 4, got $depth")

  val io = IO(new Bundle {
    val enqClock = Input(Clock())
    val enq = Flipped(Decoupled(gen))
    val deq = Decoupled(gen)
  })

  private val idxWidth = log2Ceil(depth)
  private val ptrWidth = idxWidth + 1
  private val mem = Mem(depth, gen)

  private def binToGray(x: UInt): UInt = (x >> 1) ^ x

  private val rdPtrBin = RegInit(0.U(ptrWidth.W))
  private val rdPtrGray = RegInit(0.U(ptrWidth.W))

  private val (wrPtrBin, wrPtrGray) = withClockAndReset(io.enqClock, reset) {
    val bin = RegInit(0.U(ptrWidth.W))
    val gray = RegInit(0.U(ptrWidth.W))
    (bin, gray)
  }

  private val wrPtrGraySync = RegNext(RegNext(wrPtrGray, 0.U), 0.U)
  private val empty = wrPtrGraySync === rdPtrGray

  io.deq.valid := !empty
  io.deq.bits := mem(rdPtrBin(idxWidth - 1, 0))
  when(io.deq.fire) {
    val next = rdPtrBin + 1.U
    rdPtrBin := next
    rdPtrGray := binToGray(next)
  }

  withClockAndReset(io.enqClock, reset) {
    val rdPtrGraySync = RegNext(RegNext(rdPtrGray, 0.U), 0.U)
    val wrPtrBinNext = wrPtrBin + 1.U
    val wrPtrGrayNext = binToGray(wrPtrBinNext)
    val full = wrPtrGrayNext === Cat(
      ~rdPtrGraySync(ptrWidth - 1, ptrWidth - 2),
      rdPtrGraySync(ptrWidth - 3, 0),
    )

    io.enq.ready := !full
    when(io.enq.fire) {
      mem.write(wrPtrBin(idxWidth - 1, 0), io.enq.bits)
      wrPtrBin := wrPtrBinNext
      wrPtrGray := wrPtrGrayNext
    }
  }
}

class H2CAXIs2Mem(
  val axisWidth: Int,
  val addrWidth: Int,
  val dataWidth: Int,
  val idWidth: Int,
  val userWidth: Int,
  val baseAddr: BigInt,
) extends Module {
  require(axisWidth % 8 == 0, s"AXIS width $axisWidth must be byte-aligned")
  require(dataWidth % 8 == 0, s"AXI data width $dataWidth must be byte-aligned")
  require(axisWidth % dataWidth == 0, s"AXIS width $axisWidth must be a multiple of AXI data width $dataWidth")
  require(isPow2(dataWidth / 8), s"AXI data width must be power-of-two bytes, got $dataWidth bits")
  require(
    baseAddr >= 0 && baseAddr < (BigInt(1) << addrWidth),
    s"H2C base address 0x${baseAddr.toString(16)} exceeds $addrWidth-bit AXI address",
  )

  val io = IO(new Bundle {
    val pcie_clock = Input(Clock())
    val enable = Input(Bool())
    val axis = Flipped(new AXI4Stream(axisWidth))
    val axi = new AXI4Bundle(addrWidth, dataWidth, idWidth, userWidth)
    val done = Output(Bool())
  })

  import H2CAXIs2MemState._

  private val fifoDepth = 32
  private val chunksPerAxis = axisWidth / dataWidth
  private val chunkWidth = log2Ceil(chunksPerAxis).max(1)
  private val burstWidth = log2Ceil(chunksPerAxis + 1).max(1)
  private val beatBytes = dataWidth / 8
  private val axisBytes = axisWidth / 8

  private val fifo = Module(new AsyncClockFIFO(new AXI4StreamBundle(axisWidth), fifoDepth))
  private val enableSync = withClockAndReset(io.pcie_clock, reset) {
    RegNext(RegNext(io.enable, false.B), false.B)
  }

  fifo.io.enqClock := io.pcie_clock
  fifo.io.enq.valid := io.axis.valid && enableSync
  fifo.io.enq.bits := io.axis.bits
  io.axis.ready := fifo.io.enq.ready && enableSync

  private val state = RegInit(sIdle)
  private val addr = RegInit(baseAddr.U(addrWidth.W))
  private val payload = Reg(UInt(axisWidth.W))
  private val payloadKeep = Reg(UInt(axisBytes.W))
  private val payloadLast = RegInit(false.B)
  private val chunk = RegInit(0.U(chunkWidth.W))
  private val burstBeats = RegInit(0.U(burstWidth.W))
  private val burstBeat = RegInit(0.U(burstWidth.W))
  private val payloadWords = payload.asTypeOf(Vec(chunksPerAxis, UInt(dataWidth.W)))
  private val payloadKeeps = payloadKeep.asTypeOf(Vec(chunksPerAxis, UInt(beatBytes.W)))
  private val validBytes = PopCount(fifo.io.deq.bits.keep.asBools)
  private val beatsFromKeep = (validBytes + (beatBytes - 1).U) >> log2Ceil(beatBytes)
  private val beatsInPayload = beatsFromKeep(burstWidth - 1, 0)

  io.axi.aw.valid := state === sAddr
  io.axi.aw.bits := 0.U.asTypeOf(io.axi.aw.bits)
  io.axi.aw.bits.addr := addr
  io.axi.aw.bits.len := burstBeats - 1.U
  io.axi.aw.bits.size := log2Ceil(beatBytes).U
  io.axi.aw.bits.burst := 1.U

  io.axi.w.valid := state === sData
  io.axi.w.bits.data := payloadWords(chunk)
  io.axi.w.bits.strb := payloadKeeps(chunk)
  io.axi.w.bits.last := burstBeat === burstBeats - 1.U

  io.axi.b.ready := state === sResp
  io.axi.ar.valid := false.B
  io.axi.ar.bits := 0.U.asTypeOf(io.axi.ar.bits)
  io.axi.r.ready := false.B

  io.done := state === sDone
  fifo.io.deq.ready := state === sReadPayload && io.enable

  when(!io.enable) {
    state := sIdle
    addr := baseAddr.U(addrWidth.W)
  }.otherwise {
    switch(state) {
      is(sIdle) {
        when(fifo.io.deq.valid) {
          state := sReadPayload
        }
      }
      is(sReadPayload) {
        when(fifo.io.deq.fire) {
          payload := fifo.io.deq.bits.data
          payloadKeep := fifo.io.deq.bits.keep
          payloadLast := fifo.io.deq.bits.last
          chunk := 0.U
          burstBeat := 0.U
          burstBeats := beatsInPayload
          state := Mux(beatsInPayload === 0.U, Mux(fifo.io.deq.bits.last, sDone, sReadPayload), sAddr)
        }
      }
      is(sAddr) {
        when(io.axi.aw.fire) {
          state := sData
        }
      }
      is(sData) {
        when(io.axi.w.fire) {
          addr := addr + beatBytes.U
          when(io.axi.w.bits.last) {
            state := sResp
          }.otherwise {
            chunk := chunk + 1.U
            burstBeat := burstBeat + 1.U
          }
        }
      }
      is(sResp) {
        when(io.axi.b.fire) {
          state := Mux(payloadLast, sDone, sReadPayload)
        }
      }
      is(sDone) {
        when(!io.enable) {
          state := sIdle
        }
      }
    }
  }
}

class DifftestMemCtrl(
  axiType: AXI4Bundle,
  baseAddr: BigInt,
) extends Module {
  private val addrWidth = axiType.addrWidth
  private val dataWidth = axiType.dataWidth
  private val idWidth = axiType.idWidth
  private val userWidth = axiType.userWidth
  private val beatBytes = dataWidth / 8
  require(isPow2(beatBytes), s"AXI data width must be power-of-two bytes, got $dataWidth bits")
  require(dataWidth % 64 == 0, s"AXI data width must be 64-bit aligned, got $dataWidth bits")
  private val wordsPerBeat = dataWidth / 64

  val io = IO(new Bundle {
    val ctrl = Flipped(new XDMAMemCtrlIO)
    val pcie_clock = Input(Clock())
    val h2c = Flipped(new AXI4Stream(512))
    val cpu = Flipped(new AXI4Bundle(addrWidth, dataWidth, idWidth, userWidth))
    val mem = new AXI4Bundle(addrWidth, dataWidth, idWidth, userWidth)
  })

  import MemInitState._

  private val state = RegInit(sIdle)
  private val addr = RegInit(0.U(addrWidth.W))
  private val beatsLeft = RegInit(0.U((addrWidth + 1).W))
  private val burstBeats = Reg(UInt(9.W))
  private val beat = Reg(UInt(9.W))
  private val lfsr = RegInit(0.U(64.W))
  private val initBytesWidth = (addrWidth + 21).max(64)
  private val initSizeMB = Wire(UInt(initBytesWidth.W))
  initSizeMB := io.ctrl.ramSizeMB
  private val initBytes = (initSizeMB << 20)(initBytesWidth - 1, 0)
  private val maxInitBytes =
    if (baseAddr >= (BigInt(1) << addrWidth)) BigInt(0) else (BigInt(1) << addrWidth) - baseAddr
  private val initRangeError = io.ctrl.ramSizeMB =/= 0.U && initBytes > maxInitBytes.U(initBytesWidth.W)
  private val start = io.ctrl.memInit && io.ctrl.ramSizeMB =/= 0.U && !initRangeError
  private val randomData = VecInit(lfsrWords(lfsr, wordsPerBeat)).asUInt
  private val nextState = stepLFSR(lfsr, wordsPerBeat)
  private val memDone = state === sDone
  private val memSrcInit = io.ctrl.memInit
  private val memSrcH2C = io.ctrl.memH2C && !memSrcInit
  private val memSrcCPU = io.ctrl.memCPU && !memSrcInit && !memSrcH2C
  private val memInitAxi = Wire(new AXI4Bundle(addrWidth, dataWidth, idWidth, userWidth))
  private val h2c = Module(new H2CAXIs2Mem(512, addrWidth, dataWidth, idWidth, userWidth, baseAddr))

  h2c.io.pcie_clock := io.pcie_clock
  h2c.io.enable := io.ctrl.memH2C
  h2c.io.axis <> io.h2c

  io.ctrl.memStatus := MuxCase(
    0.U(2.W),
    Seq(
      (io.ctrl.memInit && initRangeError) -> 3.U(2.W),
      (io.ctrl.memInit && memDone) -> 2.U(2.W),
      (io.ctrl.memH2C && h2c.io.done) -> 2.U(2.W),
    ),
  )

  memInitAxi.aw.valid := state === sAddr
  memInitAxi.aw.bits := 0.U.asTypeOf(memInitAxi.aw.bits)
  memInitAxi.aw.bits.addr := addr
  memInitAxi.aw.bits.len := burstBeats(7, 0) - 1.U
  memInitAxi.aw.bits.size := log2Ceil(beatBytes).U
  memInitAxi.aw.bits.burst := 1.U

  memInitAxi.w.valid := state === sData
  memInitAxi.w.bits.data := Mux(io.ctrl.seed === 0.U, 0.U, randomData)
  memInitAxi.w.bits.strb := Fill(beatBytes, 1.U(1.W))
  memInitAxi.w.bits.last := beat === burstBeats - 1.U

  memInitAxi.b.ready := state === sResp
  memInitAxi.ar.valid := false.B
  memInitAxi.ar.bits := 0.U.asTypeOf(memInitAxi.ar.bits)
  memInitAxi.r.ready := false.B

  connectMux(Seq(io.cpu -> memSrcCPU, h2c.io.axi -> memSrcH2C, memInitAxi -> memSrcInit))

  switch(state) {
    is(sIdle) {
      when(start) {
        val seedState = nextLFSR(Cat(0.U(32.W), io.ctrl.seed))
        val initFirstWord = Mux(io.ctrl.seed === 0.U, 0.U, seedState)
        addr := baseAddr.U
        beatsLeft := (io.ctrl.ramSizeMB << (20 - log2Ceil(beatBytes))).asUInt
        lfsr := seedState
        printf(
          p"[FPGA_SIM] Memory random init with seed = ${io.ctrl.seed}, first word = 0x${Hexadecimal(initFirstWord)}\n"
        )
        state := sSetup
      }
    }
    is(sSetup) {
      burstBeats := Mux(beatsLeft > 256.U, 256.U, beatsLeft(8, 0))
      beat := 0.U
      state := Mux(beatsLeft === 0.U, sDone, sAddr)
    }
    is(sAddr) {
      when(memInitAxi.aw.fire) {
        state := sData
      }
    }
    is(sData) {
      when(memInitAxi.w.fire) {
        beat := beat + 1.U
        lfsr := nextState
        when(memInitAxi.w.bits.last) {
          state := sResp
        }
      }
    }
    is(sResp) {
      when(memInitAxi.b.fire) {
        val clearedBeats = burstBeats.asTypeOf(beatsLeft)
        beatsLeft := beatsLeft - clearedBeats
        addr := addr + (burstBeats << log2Ceil(beatBytes)).asUInt
        state := Mux(beatsLeft === clearedBeats, sDone, sSetup)
      }
    }
    is(sDone) {
      when(!io.ctrl.memInit) {
        state := sIdle
      }
    }
  }

  private def nextLFSR(state: UInt): UInt = {
    val bit = state(0) ^ state(1) ^ state(3) ^ state(4)
    val next = Cat(bit, state(state.getWidth - 1, 1))
    Mux(next === 0.U, 1.U, next)
  }

  private def lfsrWords(state: UInt, n: Int): Seq[UInt] =
    Seq.iterate(state, n)(nextLFSR)

  private def stepLFSR(state: UInt, n: Int): UInt =
    (0 until n).foldLeft(state) { case (s, _) => nextLFSR(s) }

  private def connectMux(vAxis: Seq[(AXI4Bundle, Bool)]): Unit = {
    require(vAxis.nonEmpty)
    Seq("aw", "w", "ar").foreach { name =>
      val out = io.mem.elements(name).asInstanceOf[ReadyValidIO[Data]]
      val in = vAxis.map { case (axis, valid) =>
        axis.elements(name).asInstanceOf[ReadyValidIO[Data]] -> valid
      }
      out.valid := in.map { case (port, valid) => port.valid && valid }.reduce(_ || _)
      out.bits := Mux1H(in.map { case (port, valid) => valid -> port.bits.asUInt }).asTypeOf(out.bits)
      in.foreach { case (port, valid) =>
        port.ready := out.ready && valid
      }
    }
    Seq("b", "r").foreach { name =>
      val in = io.mem.elements(name).asInstanceOf[ReadyValidIO[Data]]
      val out = vAxis.map { case (axis, valid) =>
        axis.elements(name).asInstanceOf[ReadyValidIO[Data]] -> valid
      }
      out.foreach { case (port, valid) =>
        port.valid := in.valid && valid
        port.bits := in.bits.asUInt.asTypeOf(port.bits)
      }
      in.ready := Mux1H(out.map { case (port, valid) => valid -> port.ready })
    }
  }
}

object DifftestMemCtrl {
  def exposeIO(cpu: Record, mem: Record, name: String = "bore_"): DifftestMemIO = {
    val cpuPort = IO(AXI4Bundle.typeOf(cpu)).suggestName(s"${name}CpuAXI")
    AXI4Bundle.connectRecord(cpuPort, cpu)
    val memPort = IO(Flipped(AXI4Bundle.typeOf(mem))).suggestName(s"${name}MemAXI")
    AXI4Bundle.connectRecord(mem, memPort)
    DifftestMemIO(cpuPort, Some(memPort))
  }
}
