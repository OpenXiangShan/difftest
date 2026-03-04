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
 * H2C AXI4 Stream to AXI4 Memory Write Converter
 *
 * Converts incoming AXI4 Stream (from XDMA H2C) to AXI4 memory write transactions.
 * Auto-starts when enable is asserted and stream data is available.
 *
 * Features:
 * - Auto-start: detects valid stream data and begins transfer
 * - Address increment: increments DDR address for each beat
 * - Transfer termination: stops on tlast OR when beat_count reaches max_beats
 * - Fixed base address: 0x80000000 (2GB)
 *
 * @param addrWidth AXI4 address width (default 64 bits)
 * @param dataWidth AXI4 data width (default 512 bits for XDMA)
 */
class H2CAxisToAxi4(
  val addrWidth: Int = 64,
  val dataWidth: Int = 512,
) extends Module {
  val io = IO(new Bundle {
    // Control signals
    val enable = Input(Bool()) // H2C enable from Config BAR
    val max_beats = Input(UInt(32.W)) // Max transfer length (0=unlimited)

    // Stream input (from xdma_axi_h2c)
    val axis = Flipped(new AXI4Stream(dataWidth))

    // AXI4 output (to DDR)
    val mem = new AXI4(addrWidth, dataWidth, 1)

    // Status outputs (to Config BAR)
    val active = Output(Bool())
    val done = Output(Bool())
    val beat_count = Output(UInt(32.W))
  })

  // Fixed base address: 0x80000000 (2GB)
  val BASE_ADDR = 0x80000000L.U(addrWidth.W)

  // Bytes per beat
  val bytesPerBeat = (dataWidth / 8).U(addrWidth.W)

  // ===== State Machine =====
  val sIdle :: sTransfer :: sWaitResp :: sDone :: Nil = Enum(4)
  val state = RegInit(sIdle)

  // Beat counter
  val beatCounter = RegInit(0.U(32.W))
  io.beat_count := beatCounter

  // Address pointer
  val addrPtr = RegInit(BASE_ADDR)

  // ===== FSM Logic =====
  // Auto-start condition: enable && axis.valid
  val autoStart = io.enable && io.axis.valid && (state === sIdle)

  when(autoStart) {
    state := sTransfer
  }

  // Update state based on AXI4 handshakes
  switch(state) {
    is(sIdle) {
      // Wait for auto-start
      beatCounter := 0.U
      addrPtr := BASE_ADDR
    }

    is(sTransfer) {
      // Check termination conditions
      val beatWillCommit = io.mem.w.fire
      val beatCountNext = beatCounter + beatWillCommit.asUInt
      val tlastReceived = io.axis.fire && io.axis.bits.last
      val lengthReached = (io.max_beats =/= 0.U) && beatWillCommit && (beatCountNext >= io.max_beats)

      when(tlastReceived || lengthReached) {
        // Transfer complete
        state := sWaitResp
      }
    }

    is(sWaitResp) {
      // Wait for final write response
      when(io.mem.b.fire) {
        state := sDone
      }
    }

    is(sDone) {
      // Transfer done, wait for enable deassert
      when(!io.enable) {
        state := sIdle
      }
    }
  }

  // ===== Status Outputs =====
  io.active := (state === sTransfer) || (state === sWaitResp)
  io.done := (state === sDone)

  // ===== AXI4 Stream Ready Logic =====
  // Accept new data only when in transfer state and both AW/W channels are ready
  val awReady = !io.mem.aw.valid || io.mem.aw.ready
  val wReady = !io.mem.w.valid || io.mem.w.ready
  io.axis.ready := (state === sTransfer) && awReady && wReady

  // ===== AXI4 Write Address Channel (AW) =====
  // Use register for valid to avoid combinational cycle
  val awValidReg = RegInit(false.B)
  val wValidReg = RegInit(false.B)

  // AW channel: set valid when in transfer and we have new data, clear on handshake
  when(state === sTransfer && io.axis.valid && !awValidReg) {
    awValidReg := true.B
  }.elsewhen(awValidReg && io.mem.aw.ready) {
    awValidReg := false.B
  }
  io.mem.aw.valid := awValidReg
  io.mem.aw.bits.addr := addrPtr
  io.mem.aw.bits.prot := 0.U
  io.mem.aw.bits.id := 0.U
  io.mem.aw.bits.len := 0.U // Single beat per transaction
  io.mem.aw.bits.size := log2Ceil(dataWidth / 8).U
  io.mem.aw.bits.burst := 1.U // INCR
  io.mem.aw.bits.lock := false.B
  io.mem.aw.bits.cache := 0.U
  io.mem.aw.bits.qos := 0.U
  io.mem.aw.bits.user := 0.U

  // ===== AXI4 Write Data Channel (W) =====
  // W channel: set valid when in transfer and we have new data, clear on handshake
  when(state === sTransfer && io.axis.valid && !wValidReg) {
    wValidReg := true.B
  }.elsewhen(wValidReg && io.mem.w.ready) {
    wValidReg := false.B
  }
  io.mem.w.valid := wValidReg

  // Data and control signals
  val wDataReg = Reg(UInt(dataWidth.W))
  val wLastReg = Reg(Bool())

  when(state === sTransfer && io.axis.valid && !wValidReg) {
    wDataReg := io.axis.bits.data
    wLastReg := io.axis.bits.last || ((io.max_beats =/= 0.U) && ((beatCounter + 1.U) >= io.max_beats))
  }

  io.mem.w.bits.data := wDataReg
  io.mem.w.bits.strb := Fill(dataWidth / 8, 1.U(1.W))
  io.mem.w.bits.last := wLastReg

  // ===== AXI4 Write Response Channel (B) =====
  io.mem.b.ready := (state === sWaitResp) || (state === sTransfer)

  // ===== AXI4 Read Channels (not used, tie off) =====
  io.mem.ar.valid := false.B
  io.mem.ar.bits := 0.U.asTypeOf(new AXI4ARBundle(addrWidth, 1))
  io.mem.r.ready := true.B

  // ===== Beat Counter & Address Increment =====
  when(io.mem.w.fire) {
    beatCounter := beatCounter + 1.U
    addrPtr := addrPtr + bytesPerBeat
  }
}
