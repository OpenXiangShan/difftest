/***************************************************************************************
 * Copyright (c) 2025-2026 Beijing Institute of Open Source Chip
 * Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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
import difftest.common.AXI4LiteBundle
import difftest.gateway.{ReplayTraceRequest, ReplayTraceStatus}

/** XDMA Config BAR for both FPGA/FPGA_SIM
  *
  * Register Map:
  *   - 0x00: HOST_IO_CFG_RESET
  *   - 0x04: HOST_IO_RESET
  *   - 0x08: HOST_IO_DIFF_ENABLE
  *   - 0x0c: HOST_IO_ILA_TRIGGER
  *   - 0x10: HOST_IO_SQUASH_ENABLE
  *   - 0x14: HOST_IO_SQUASH_MAX_FUSED
  *   - 0x18: HOST_IO_SEED
  *   - 0x1c: HOST_IO_RAM_SIZE_MB
  *   - 0x20: HOST_IO_MEM_INIT
  *   - 0x24: HOST_IO_MEM_CPU
  *   - 0x28: HOST_IO_MEM_H2C
  *   - 0x2c: HOST_IO_H2C_SIZE_MB
  *   - 0x30: HOST_IO_REPLAY_TRACE_FREEZE
  *   - 0x34: HOST_IO_REPLAY_TRACE_HEAD
  *   - 0x38: HOST_IO_REPLAY_TRACE_SIZE
  *   - 0x3c: HOST_IO_REPLAY_TRACE_DUMP
  *   - 0x40: HOST_IO_REPLAY_TRACE_REARM
  *   - 0x44: HOST_IO_REPLAY_TRACE_STATUS (read-only)
  *   - 0x48: HOST_IO_REPLAY_TRACE_WRITE_PTR (read-only)
  *   - 0x4c: HOST_IO_REPLAY_TRACE_WRITE_SEQ (read-only)
  *   - 0x50: HOST_IO_REPLAY_TRACE_DUMP_START (read-only)
  *   - 0x54: HOST_IO_REPLAY_TRACE_DUMP_BEATS (read-only)
  */
class XDMAHostCtrlIO extends Bundle {
  val reset = Bool()
  val diffEnable = Bool()
  val ilaTrigger = Bool()
  val enableSquash = Bool()
  val squashMaxFused = UInt(8.W)
}

class XDMAMemCtrlIO extends Bundle {
  val memInit = Output(Bool())
  val memH2C = Output(Bool())
  val memCPU = Output(Bool())
  val seed = Output(UInt(32.W))
  val ramSizeMB = Output(UInt(32.W))
  val h2cSizeMB = Output(UInt(32.W))
  val memStatus = Input(UInt(2.W))
}

private object XDMAConfigReg extends Enumeration {
  val CfgReset, HostReset, DiffEnable, IlaTrigger, EnableSquash, SquashMaxFused, Seed, RamSizeMB, MemInit, MemCPU, MemH2C, H2CSizeMB,
    ReplayTraceFreeze, ReplayTraceHead, ReplayTraceSize, ReplayTraceDump, ReplayTraceRearm, ReplayTraceStatus,
    ReplayTraceWritePtr, ReplayTraceWriteSeq, ReplayTraceDumpStart, ReplayTraceDumpBeats = Value
}

class XDMAConfigBar(val addrWidth: Int = 32, val dataWidth: Int = 32) extends Module {
  require(dataWidth == 32, "XDMAConfigBar currently models a 32-bit AXI-Lite BAR")

  val io = IO(new Bundle {
    val axilite = Flipped(new AXI4LiteBundle(addrWidth, dataWidth))
    val cfgReset = Output(Bool())
    val hostCtrl = Output(new XDMAHostCtrlIO)
    val memCtrl = new XDMAMemCtrlIO
    val replayTraceRequest = Output(new ReplayTraceRequest)
    val replayTraceStatus = Input(new ReplayTraceStatus)
  })

  private val numRegs = XDMAConfigReg.maxId
  private val idxBits = log2Ceil(numRegs)
  private val regfile = RegInit(VecInit(Seq.tabulate(numRegs) { idx =>
    val init = if (idx == XDMAConfigReg.SquashMaxFused.id) 255 else 0
    init.U(dataWidth.W)
  }))

  io.hostCtrl.reset := regfile(XDMAConfigReg.HostReset.id)(0)
  io.hostCtrl.diffEnable := regfile(XDMAConfigReg.DiffEnable.id)(0)
  io.hostCtrl.ilaTrigger := regfile(XDMAConfigReg.IlaTrigger.id)(0)
  io.hostCtrl.enableSquash := regfile(XDMAConfigReg.EnableSquash.id)(0)
  io.hostCtrl.squashMaxFused := regfile(XDMAConfigReg.SquashMaxFused.id)(7, 0)
  io.memCtrl.memInit := regfile(XDMAConfigReg.MemInit.id)(0)
  io.memCtrl.memH2C := regfile(XDMAConfigReg.MemH2C.id)(0)
  io.memCtrl.memCPU := regfile(XDMAConfigReg.MemCPU.id)(0)
  io.memCtrl.seed := regfile(XDMAConfigReg.Seed.id)
  io.memCtrl.ramSizeMB := regfile(XDMAConfigReg.RamSizeMB.id)
  io.memCtrl.h2cSizeMB := regfile(XDMAConfigReg.H2CSizeMB.id)
  io.cfgReset := regfile(XDMAConfigReg.CfgReset.id)(0)
  io.replayTraceRequest.freeze := regfile(XDMAConfigReg.ReplayTraceFreeze.id)(0)
  io.replayTraceRequest.traceHead := regfile(XDMAConfigReg.ReplayTraceHead.id)(15, 0)
  io.replayTraceRequest.traceSize := regfile(XDMAConfigReg.ReplayTraceSize.id)(15, 0)
  io.replayTraceRequest.dump := regfile(XDMAConfigReg.ReplayTraceDump.id)(0)
  io.replayTraceRequest.rearm := regfile(XDMAConfigReg.ReplayTraceRearm.id)(0)

  private def mergeByByte(oldData: UInt, newData: UInt, strb: UInt): UInt = {
    VecInit((0 until dataWidth / 8).map { i =>
      Mux(strb(i), newData(8 * i + 7, 8 * i), oldData(8 * i + 7, 8 * i))
    }).asUInt
  }

  val awaddr = Reg(UInt(addrWidth.W))
  val awValid = RegInit(false.B)
  val wdata = Reg(UInt(dataWidth.W))
  val wstrb = Reg(UInt((dataWidth / 8).W))
  val wValid = RegInit(false.B)
  val bValid = RegInit(false.B)

  io.axilite.aw.ready := !awValid && !bValid
  io.axilite.w.ready := !wValid && !bValid
  io.axilite.b.valid := bValid
  io.axilite.b.bits.resp := 0.U

  val awFire = io.axilite.aw.fire
  val wFire = io.axilite.w.fire
  val nextAwAddr = Mux(awFire, io.axilite.aw.bits.addr, awaddr)
  val nextWData = Mux(wFire, io.axilite.w.bits.data, wdata)
  val nextWStrb = Mux(wFire, io.axilite.w.bits.strb, wstrb)
  val doWrite = !bValid && (awValid || awFire) && (wValid || wFire)
  val writeWord = nextAwAddr(addrWidth - 1, 2)
  val writeIdx = writeWord(idxBits - 1, 0)

  when(awFire) {
    awaddr := io.axilite.aw.bits.addr
    awValid := true.B
  }
  when(wFire) {
    wdata := io.axilite.w.bits.data
    wstrb := io.axilite.w.bits.strb
    wValid := true.B
  }
  when(doWrite) {
    when(
      writeWord < XDMAConfigReg.ReplayTraceStatus.id.U ||
        writeWord === XDMAConfigReg.ReplayTraceRearm.id.U
    ) {
      regfile(writeIdx) := mergeByByte(regfile(writeIdx), nextWData, nextWStrb)
    }
    awValid := false.B
    wValid := false.B
    bValid := true.B
  }.elsewhen(bValid && io.axilite.b.ready) {
    bValid := false.B
  }

  val arReady = RegInit(true.B)
  val rValid = RegInit(false.B)
  val rData = Reg(UInt(dataWidth.W))

  io.axilite.ar.ready := arReady
  io.axilite.r.valid := rValid
  io.axilite.r.bits.data := rData
  io.axilite.r.bits.resp := 0.U

  val replayTraceStatus = Cat(
    0.U(28.W),
    io.replayTraceStatus.rangeLost,
    io.replayTraceStatus.dumpDone,
    io.replayTraceStatus.dumpActive,
    io.replayTraceStatus.frozen,
  )
  when(io.axilite.ar.valid && arReady) {
    val readWord = io.axilite.ar.bits.addr(addrWidth - 1, 2)
    val readIdx = readWord(idxBits - 1, 0)
    rData := MuxLookup(readWord, Mux(readWord < numRegs.U, regfile(readIdx), 0.U))(
      Seq(
        XDMAConfigReg.ReplayTraceStatus.id.U -> replayTraceStatus,
        XDMAConfigReg.ReplayTraceWritePtr.id.U -> io.replayTraceStatus.writePtr,
        XDMAConfigReg.ReplayTraceWriteSeq.id.U -> io.replayTraceStatus.writeSeq,
        XDMAConfigReg.ReplayTraceDumpStart.id.U -> io.replayTraceStatus.dumpStart,
        XDMAConfigReg.ReplayTraceDumpBeats.id.U -> io.replayTraceStatus.dumpBeats,
      )
    )
    arReady := false.B
    rValid := true.B
  }.elsewhen(rValid && io.axilite.r.ready) {
    arReady := true.B
    rValid := false.B
  }

  when(io.memCtrl.memInit && io.memCtrl.memStatus =/= 0.U) {
    regfile(XDMAConfigReg.MemInit.id) := io.memCtrl.memStatus
  }
  when(io.memCtrl.memH2C && io.memCtrl.memStatus =/= 0.U) {
    regfile(XDMAConfigReg.MemH2C.id) := io.memCtrl.memStatus
  }
}
