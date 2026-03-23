/***************************************************************************************
 * Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

/**
 * AXI4-Lite Configuration BAR for H2C DDR Initialization
 *
 * Register Map:
 * 0x00: HOST_IO_RESET (RW)
 * 0x04: HOST_IO_DIFFTEST_ENABLE (RW)
 * 0x08: DDR_ARB_SEL (RW) - Bit 0: 0=CPU owns DDR, 1=H2C owns DDR
 * 0x0C: H2C_LENGTH (RW)  - Transfer length in beats (0=unlimited/tlast-based)
 * 0x10: H2C_STATUS (RO)  - Bit 1: H2C active, Bit 2: H2C done
 * 0x14: H2C_BEAT_CNT (RO) - Current beat counter
 * 0x18: H2C_BEAT_BYTES (RO) - Bytes per H2C beat
 * 0x1C: Reserved for future expansion
 */
class XDMAConfigBar(val addrWidth: Int = 32, val dataWidth: Int = 32, val h2cBeatBytes: Int = 8) extends Module {
  require(h2cBeatBytes > 0 && h2cBeatBytes <= 255, s"h2cBeatBytes($h2cBeatBytes) out of range")

  val io = IO(new Bundle {
    val axilite = new AXI4LiteSlaveIO(addrWidth, dataWidth)

    // Host control outputs
    val HOST_IO_RESET = Output(Bool())
    val HOST_IO_DIFFTEST_ENABLE = Output(Bool())

    // Control outputs
    val ddr_arb_sel = Output(Bool()) // 0=CPU, 1=H2C
    val h2c_length = Output(UInt(32.W)) // Transfer length in beats

    // Status inputs (from H2C module)
    val h2c_active = Input(Bool())
    val h2c_done = Input(Bool())
    val h2c_beat_count = Input(UInt(32.W))
  })

  // Number of registers
  val numRegs = 8
  val idxBits = log2Ceil(numRegs)

  // Register file (8 x 32-bit)
  val regfile = RegInit(VecInit(Seq.fill(numRegs)(0.U(dataWidth.W))))
  val regHostReset = 0
  val regDiffEnable = 1
  val regDdrArbSel = 2
  val regH2cLength = 3
  val regH2cStatus = 4
  val regH2cBeatCnt = 5
  val regH2cBeatBytes = 6

  // Register outputs
  io.HOST_IO_RESET := regfile(regHostReset)(0)
  io.HOST_IO_DIFFTEST_ENABLE := regfile(regDiffEnable)(0)
  io.ddr_arb_sel := regfile(regDdrArbSel)(0)
  io.h2c_length := regfile(regH2cLength)

  // Status register (read-only, updated from logic)
  // Bit 0: reserved, Bit 1: H2C active, Bit 2: H2C done, Bits 31-3: beat_count[31:5]
  regfile(regH2cStatus) := Cat(
    io.h2c_beat_count(31, 3),
    io.h2c_done,
    io.h2c_active,
    0.U(1.W),
  )

  // Beat counter register (read-only)
  regfile(regH2cBeatCnt) := io.h2c_beat_count

  // Beat bytes register (read-only)
  regfile(regH2cBeatBytes) := h2cBeatBytes.U(dataWidth.W)

  // Reserved register (read-only, return 0)
  regfile(7) := 0.U

  // ===== AXI4-Lite Write Channel FSM =====
  val awready_r = RegInit(true.B)
  val wready_r = RegInit(true.B)
  val bvalid_r = RegInit(false.B)
  val awaddr_r = Reg(UInt(addrWidth.W))

  // Write address channel
  when(io.axilite.aw.valid && awready_r) {
    awaddr_r := io.axilite.aw.bits.addr
    awready_r := false.B
  }

  // Write data channel
  when(io.axilite.w.valid && wready_r) {
    // AW/W may handshake in the same cycle. Use the in-flight AW addr if available.
    val write_addr = Mux(io.axilite.aw.valid && awready_r, io.axilite.aw.bits.addr, awaddr_r)
    // Only write to writable registers (offsets 0x00/0x04/0x08/0x0C)
    val write_idx = (write_addr >> 2)(idxBits - 1, 0)
    when(write_idx <= regH2cLength.U) {
      // Apply byte strobe
      val wdata = io.axilite.w.bits.data
      val wstrb = io.axilite.w.bits.strb
      val byte0 = Mux(wstrb(0), wdata(7, 0), regfile(write_idx)(7, 0))
      val byte1 = Mux(wstrb(1), wdata(15, 8), regfile(write_idx)(15, 8))
      val byte2 = Mux(wstrb(2), wdata(23, 16), regfile(write_idx)(23, 16))
      val byte3 = Mux(wstrb(3), wdata(31, 24), regfile(write_idx)(31, 24))
      val mask_data = Cat(byte3, byte2, byte1, byte0)
      regfile(write_idx) := mask_data
    }
    wready_r := false.B
    bvalid_r := true.B
  }

  // Write response channel
  when(bvalid_r && io.axilite.b.ready) {
    bvalid_r := false.B
    awready_r := true.B
    wready_r := true.B
  }

  // Connect write channel signals
  io.axilite.aw.ready := awready_r
  io.axilite.w.ready := wready_r
  io.axilite.b.valid := bvalid_r
  io.axilite.b.bits.resp := 0.U // RESP_OKAY

  // ===== AXI4-Lite Read Channel FSM =====
  val arready_r = RegInit(true.B)
  val rvalid_r = RegInit(false.B)
  val araddr_r = Reg(UInt(addrWidth.W))
  val rdata_r = Reg(UInt(dataWidth.W))

  // Read address channel
  when(io.axilite.ar.valid && arready_r) {
    araddr_r := io.axilite.ar.bits.addr
    arready_r := false.B
    rvalid_r := true.B
    // Pre-fetch read data
    val read_idx = (io.axilite.ar.bits.addr >> 2)(idxBits - 1, 0)
    rdata_r := Mux(read_idx < numRegs.U, regfile(read_idx), 0.U)
  }

  // Read data channel
  when(rvalid_r && io.axilite.r.ready) {
    rvalid_r := false.B
    arready_r := true.B
  }

  // Connect read channel signals
  io.axilite.ar.ready := arready_r
  io.axilite.r.valid := rvalid_r
  io.axilite.r.bits.data := rdata_r
  io.axilite.r.bits.resp := 0.U // RESP_OKAY
}
