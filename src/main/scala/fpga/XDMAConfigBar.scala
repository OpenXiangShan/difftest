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

/** XDMA Config BAR for both FPGA/FPGA_SIM
  *
  * Register Map:
  *   - 0x00: HOST_IO_CFG_RESET
  *   - 0x04: HOST_IO_RESET
  *   - 0x08: HOST_IO_DIFF_ENABLE
  *   - 0x0c: HOST_IO_ILA_TRIGGER
  *   - 0x10: HOST_IO_SQUASH_ENABLE
  *   - 0x14: HOST_IO_SEED
  *   - 0x18: HOST_IO_RAM_SIZE_MB
  *   - 0x1c: HOST_IO_MEM_INIT
  *   - 0x20: HOST_IO_MEM_CPU
  *   - 0x24: HOST_IO_MEM_H2C
  *   - 0x28: HOST_IO_H2C_SIZE_MB
  */
class XDMAHostCtrlIO extends Bundle {
  val reset = Bool()
  val diffEnable = Bool()
  val ilaTrigger = Bool()
  val enableSquash = Bool()
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
  val CfgReset, HostReset, DiffEnable, IlaTrigger, EnableSquash, Seed, RamSizeMB, MemInit, MemCPU, MemH2C, H2CSizeMB =
    Value
}

class XDMAConfigBar(val addrWidth: Int = 32, val dataWidth: Int = 32) extends Module {
  require(dataWidth == 32, "XDMAConfigBar currently models a 32-bit AXI-Lite BAR")

  val io = IO(new Bundle {
    val axilite = Flipped(new AXI4LiteBundle(addrWidth, dataWidth))
    val cfgReset = Output(Bool())
    val hostCtrl = Output(new XDMAHostCtrlIO)
    val memCtrl = new XDMAMemCtrlIO
  })

  private val numRegs = XDMAConfigReg.maxId
  private val idxBits = log2Ceil(numRegs)
  private val regfile = RegInit(VecInit(Seq.fill(numRegs)(0.U(dataWidth.W))))

  io.hostCtrl.reset := regfile(XDMAConfigReg.HostReset.id)(0)
  io.hostCtrl.diffEnable := regfile(XDMAConfigReg.DiffEnable.id)(0)
  io.hostCtrl.ilaTrigger := regfile(XDMAConfigReg.IlaTrigger.id)(0)
  io.hostCtrl.enableSquash := regfile(XDMAConfigReg.EnableSquash.id)(0)
  io.memCtrl.memInit := regfile(XDMAConfigReg.MemInit.id)(0)
  io.memCtrl.memH2C := regfile(XDMAConfigReg.MemH2C.id)(0)
  io.memCtrl.memCPU := regfile(XDMAConfigReg.MemCPU.id)(0)
  io.memCtrl.seed := regfile(XDMAConfigReg.Seed.id)
  io.memCtrl.ramSizeMB := regfile(XDMAConfigReg.RamSizeMB.id)
  io.memCtrl.h2cSizeMB := regfile(XDMAConfigReg.H2CSizeMB.id)
  io.cfgReset := regfile(XDMAConfigReg.CfgReset.id)(0)

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
    when(writeWord < numRegs.U) {
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

  when(io.axilite.ar.valid && arReady) {
    val readWord = io.axilite.ar.bits.addr(addrWidth - 1, 2)
    val readIdx = readWord(idxBits - 1, 0)
    rData := Mux(readWord < numRegs.U, regfile(readIdx), 0.U)
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
