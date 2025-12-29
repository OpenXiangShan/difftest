/***************************************************************************************
 * Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
 * Copyright (c) 2025 Beijing Institute of Open Source Chip
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
import difftest.gateway.FpgaDiffIO

class Difftest2AXIs(val difftest_width: Int, val axis_width: Int) extends Module {
  val io = IO(new Bundle {
    val rd_clock = Input(Clock()) // Read clock
    val reset = Input(Bool())
    val difftest = Input(new FpgaDiffIO(difftest_width))
    val clock_enable = Output(Bool())
    val axis = new AXI4Stream(axis_width)
  })

  val numPacketPerRange = 8 // packet in each range
  val pkt_id_w = 8 // pkt = (difftest_data, pkt_id)
  val axis_send_len = (difftest_width + pkt_id_w + axis_width - 1) / axis_width
  val fifo_depth = 16
  val fifo_addr_width = log2Ceil(fifo_depth)

  // FIFO implementation using SyncReadMem
  val fifo_ram = SyncReadMem(fifo_depth, UInt(difftest_width.W))

  // Define counters that are accessible in both clock domains
  val wr_cnt = RegInit(0.U(64.W))
  val rd_cnt = withClock(io.rd_clock) { RegInit(0.U(64.W)) }

  // Write clock domain (using default clock)
  val wr_ptr = RegInit(0.U(fifo_addr_width.W))
  val rd_cnt_sync1 = RegInit(0.U(64.W))
  val rd_cnt_sync2 = RegInit(0.U(64.W))
  val wr_occupancy = Reg(UInt(64.W)) // Occupancy in write clock domain

  // Synchronize read counter to write clock domain
  rd_cnt_sync1 := RegNext(rd_cnt, 0.U)
  rd_cnt_sync2 := RegNext(rd_cnt_sync1, 0.U)

  // Calculate occupancy (in write clock domain)
  wr_occupancy := wr_cnt - rd_cnt_sync2

  // Write side (difftest side)
  val pktID = RegInit(0.U(pkt_id_w.W))
  val wrRangeCounter = RegInit(0.U(3.W)) // 0 to 7
  val fifo_not_full = wr_occupancy < (fifo_depth - 1).U
  val wr_en = io.difftest.enable && fifo_not_full

  // Store incoming data
  when(wr_en) {
    fifo_ram.write(wr_ptr, io.difftest.data)
    wrRangeCounter := Mux(wrRangeCounter === (numPacketPerRange - 1).U, 0.U, wrRangeCounter + 1.U)
    wr_cnt := wr_cnt + 1.U // Increment write counter
  }

  // Increment write pointer
  when(wr_en) {
    wr_ptr := wr_ptr + 1.U
  }

  // Increment packet ID when starting a new range
  when(wr_en && wrRangeCounter === (numPacketPerRange - 1).U) {
    pktID := pktID + 1.U
  }

  // Backpressure based on FIFO space - only consider FIFO occupancy
  io.clock_enable := fifo_not_full

  // Read clock domain
  withClock(io.rd_clock) {
    val rd_ptr = RegInit(0.U(fifo_addr_width.W))
    val wr_cnt_sync1 = RegInit(0.U(64.W))
    val wr_cnt_sync2 = RegInit(0.U(64.W))
    val rd_occupancy = Reg(UInt(64.W)) // Occupancy in read clock domain

    // Synchronize write counter to read clock domain
    wr_cnt_sync1 := RegNext(wr_cnt, 0.U)
    wr_cnt_sync2 := RegNext(wr_cnt_sync1, 0.U)

    // Calculate occupancy (in read clock domain)
    rd_occupancy := wr_cnt_sync2 - rd_cnt

    val inTransfer = RegInit(false.B)
    val mix_data = RegInit(0.U((difftest_width + pkt_id_w).W))
    val sendCnt = RegInit(0.U(log2Ceil(axis_send_len).W))
    val sendLast = sendCnt === (axis_send_len - 1).U
    val counter = RegInit(0.U(3.W)) // 0 to 7
    val currentPktID = RegInit(0.U(pkt_id_w.W))
    val sendPacketEnd = counter === (numPacketPerRange - 1).U
    // Read from FIFO
    val fifo_out = fifo_ram.read(rd_ptr)

    // Counter for throttling debug prints
    // Start transfer when we have data available
    when(!inTransfer) {
      when(rd_occupancy >= numPacketPerRange.U) {
        mix_data := Cat(fifo_out, currentPktID) // First data in range
        rd_ptr := rd_ptr + 1.U
        rd_cnt := rd_cnt + 1.U // Increment read counter
        // counter := 0.U  // 0~7
        inTransfer := true.B
        sendCnt := 0.U
      }
    }.elsewhen(inTransfer) {
      when(io.axis.fire) {
        when(sendLast) {
          sendCnt := 0.U
          when(sendPacketEnd) {
            // Last data in range, finish transfer
            inTransfer := false.B
            counter := 0.U
            currentPktID := currentPktID + 1.U // Increment ID for next range
          }.otherwise {
            // Read next data in range
            mix_data := Cat(fifo_out, currentPktID)
            rd_ptr := rd_ptr + 1.U
            rd_cnt := rd_cnt + 1.U // Increment read counter
            counter := counter + 1.U
          }
        }.otherwise {
          // Still sending beats of current data
          sendCnt := sendCnt + 1.U
          mix_data := mix_data >> axis_width
        }
      }
    }

    // AXI output
    io.axis.valid := inTransfer
    io.axis.bits.data := mix_data(axis_width - 1, 0)
    io.axis.bits.last := inTransfer && sendLast && sendPacketEnd
  }
}

class HostEndpoint(
  val diffWidth: Int,
  val axisWidth: Int = 512,
) extends Module {
  val io = IO(new Bundle {
    val difftest = Input(new FpgaDiffIO(diffWidth))
    val to_host_axis = new AXI4Stream(axisWidth)
    val clock_enable = Output(Bool())
    val pcie_clock = Input(Clock())
  })

  // Instantiate the converter module
  val diff2axis = Module(new Difftest2AXIs(diffWidth, axisWidth))

  // Connect clock and reset signals
  diff2axis.io.rd_clock := io.pcie_clock
  diff2axis.io.reset := reset

  // Handle cross-clock domain data transfer
  // Difftest input domain (local clock)
  withClock(clock) {
    diff2axis.io.difftest := io.difftest
  }

  // AXI-Stream output domain (PCIe clock)
  withClock(io.pcie_clock) {
    io.to_host_axis <> diff2axis.io.axis
  }

  // Clock enable output (can be used in either domain, but connecting in current module's domain)
  io.clock_enable := diff2axis.io.clock_enable
}
