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
 * @param memDataWidth AXI4 DDR write width (same as CPU mem path)
 * @param axisDataWidth AXI4-Stream width from XDMA (default follows memDataWidth)
 */
class H2CAxisToAxi4(
  val addrWidth: Int = 64,
  val memDataWidth: Int = 64,
  val axisDataWidth: Int = 0,
) extends Module {
  private val h2cDataWidth = if (axisDataWidth == 0) memDataWidth else axisDataWidth
  require(h2cDataWidth == memDataWidth, s"axisDataWidth($h2cDataWidth) must equal memDataWidth($memDataWidth)")
  require(memDataWidth % 8 == 0, s"memDataWidth($memDataWidth) must be byte aligned")

  val io = IO(new Bundle {
    // Control signals
    val enable = Input(Bool()) // H2C enable from Config BAR
    val max_beats = Input(UInt(32.W)) // Max transfer length (0=unlimited)

    // Stream input (from xdma_axi_h2c)
    val axis = Flipped(new AXI4Stream(h2cDataWidth))

    // AXI4 output (to DDR)
    val mem = new AXI4(addrWidth, memDataWidth, 1)

    // Status outputs (to Config BAR)
    val active = Output(Bool())
    val done = Output(Bool())
    val beat_count = Output(UInt(32.W))
  })

  // Fixed base address: 0x80000000 (2GB)
  val BASE_ADDR = 0x80000000L.U(addrWidth.W)

  val memBytesPerBeat = (memDataWidth / 8).U(addrWidth.W)

  // ===== State Machine =====
  val sIdle :: sTransfer :: sWaitResp :: sDone :: Nil = Enum(4)
  val state = RegInit(sIdle)

  // H2C beat counter keeps stream-beat semantics (64-bit beat on current board wiring).
  val beatCounter = RegInit(0.U(32.W))
  io.beat_count := beatCounter

  // Address pointer for DDR writes.
  val addrPtr = RegInit(BASE_ADDR)
  // Track whether the last transfer beat has been fully drained to DDR.
  val transferDoneSeen = RegInit(false.B)
  // Number of write responses still expected from DDR (counts mem-width writes).
  val outstandingWrites = RegInit(0.U(33.W))

  // One-beat buffer for AW/W decoupling.
  val haveBufferedBeat = RegInit(false.B)
  val bufferedBeatData = Reg(UInt(h2cDataWidth.W))
  val bufferedBeatLast = RegInit(false.B)
  val awDoneForBeat = RegInit(false.B)
  val wDoneForBeat = RegInit(false.B)

  // ===== FSM Logic =====
  val autoStart = io.enable && (state === sIdle)
  val wFire = io.mem.w.fire
  val bFire = io.mem.b.fire
  val responsesDrained = outstandingWrites === 0.U

  // Track outstanding responses in transfer/wait states.
  when(state === sTransfer || state === sWaitResp) {
    when(wFire && !bFire) {
      outstandingWrites := outstandingWrites + 1.U
    }.elsewhen(!wFire && bFire) {
      // A response must correspond to a previously accepted write.
      assert(outstandingWrites =/= 0.U)
      outstandingWrites := outstandingWrites - 1.U
    }
  }

  // Update state based on AXI4 handshakes
  switch(state) {
    is(sIdle) {
      // Wait for enable.
      beatCounter := 0.U
      addrPtr := BASE_ADDR
      transferDoneSeen := false.B
      outstandingWrites := 0.U
      haveBufferedBeat := false.B
      bufferedBeatLast := false.B
      awDoneForBeat := false.B
      wDoneForBeat := false.B
      when(autoStart) {
        state := sTransfer
      }
    }

    is(sTransfer) {
      // Latch one stream beat.
      when(io.axis.fire) {
        bufferedBeatData := io.axis.bits.data
        bufferedBeatLast := io.axis.bits.last || ((io.max_beats =/= 0.U) && ((beatCounter + 1.U) >= io.max_beats))
        beatCounter := beatCounter + 1.U
        haveBufferedBeat := true.B
        awDoneForBeat := false.B
        wDoneForBeat := false.B
      }

      val awDoneNext = awDoneForBeat || io.mem.aw.fire
      val wDoneNext = wDoneForBeat || io.mem.w.fire
      val beatDone = haveBufferedBeat && awDoneNext && wDoneNext
      when(io.mem.aw.fire) {
        awDoneForBeat := true.B
      }
      when(io.mem.w.fire) {
        wDoneForBeat := true.B
      }

      when(beatDone) {
        addrPtr := addrPtr + memBytesPerBeat
        awDoneForBeat := false.B
        wDoneForBeat := false.B
        haveBufferedBeat := false.B
        when(bufferedBeatLast) {
          transferDoneSeen := true.B
        }
      }

      when(transferDoneSeen && !haveBufferedBeat) {
        // Stop accepting stream beats, then wait for all write responses to drain.
        state := sWaitResp
      }
    }

    is(sWaitResp) {
      // Wait until all responses are drained.
      when(responsesDrained) {
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
  // Accept one stream beat only when buffer is empty.
  io.axis.ready := (state === sTransfer) && !haveBufferedBeat

  // ===== AXI4 Write Address Channel (AW) =====
  io.mem.aw.valid := (state === sTransfer) && haveBufferedBeat && !awDoneForBeat
  io.mem.aw.bits.addr := addrPtr
  io.mem.aw.bits.prot := 0.U
  io.mem.aw.bits.id := 0.U
  io.mem.aw.bits.len := 0.U // Single beat per transaction
  io.mem.aw.bits.size := log2Ceil(memDataWidth / 8).U
  io.mem.aw.bits.burst := 1.U // INCR
  io.mem.aw.bits.lock := false.B
  io.mem.aw.bits.cache := 0.U
  io.mem.aw.bits.qos := 0.U
  io.mem.aw.bits.user := 0.U

  // ===== AXI4 Write Data Channel (W) =====
  io.mem.w.valid := (state === sTransfer) && haveBufferedBeat && !wDoneForBeat
  io.mem.w.bits.data := bufferedBeatData
  io.mem.w.bits.strb := Fill(memDataWidth / 8, 1.U(1.W))
  io.mem.w.bits.last := true.B // len=0 (single-beat write)

  // ===== AXI4 Write Response Channel (B) =====
  io.mem.b.ready := (state === sWaitResp) || (state === sTransfer)

  // ===== AXI4 Read Channels (not used, tie off) =====
  io.mem.ar.valid := false.B
  io.mem.ar.bits := 0.U.asTypeOf(new AXI4ARBundle(addrWidth, 1))
  io.mem.r.ready := true.B
}
