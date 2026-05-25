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
import difftest.common.AXI4Bundle

object MemInitState extends ChiselEnum {
  val sIdle, sSetup, sAddr, sData, sResp, sDone = Value
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
  private val memSrcCPU = io.ctrl.memCPU && !memSrcInit
  private val memInitAxi = Wire(new AXI4Bundle(addrWidth, dataWidth, idWidth, userWidth))

  io.ctrl.memStatus := MuxCase(
    0.U(2.W),
    Seq(
      (io.ctrl.memInit && initRangeError) -> 3.U(2.W),
      (io.ctrl.memInit && memDone) -> 2.U(2.W),
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

  connectMux(Seq(io.cpu -> memSrcCPU, memInitAxi -> memSrcInit))

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
