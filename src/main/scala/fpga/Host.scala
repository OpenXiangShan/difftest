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
import difftest.util.{Delayer, PipelineConnect}

class Difftest2AXIs(val difftest_width: Int, val axis_width: Int) extends Module {
  val io = IO(new Bundle {
    val pcie_clock = Input(Clock()) // Read clock
    val reset = Input(Bool())
    val difftest = Flipped(new FpgaDiffIO(difftest_width))
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
  val rd_cnt = withClock(io.pcie_clock) { RegInit(0.U(64.W)) }

  // Write clock domain (using default clock)
  val wr_ptr = RegInit(0.U(fifo_addr_width.W))
  val rd_cnt_sync = Delayer(rd_cnt, 2) // Synchronize read counter to write clock domain
  val wr_occupancy = wr_cnt - rd_cnt_sync // Calculate occupancy in write clock domain

  // Write side (difftest side)
  val pktID = RegInit(0.U(pkt_id_w.W))
  val wrRangeCounter = RegInit(0.U(3.W)) // 0 to 7
  val fifo_not_full = wr_occupancy < (fifo_depth - 1).U
  io.difftest.ready := fifo_not_full // Backpressure based on FIFO space - only consider FIFO occupancy
  val wr_en = io.difftest.fire

  when(wr_en) {
    fifo_ram.write(wr_ptr, io.difftest.bits)
    wr_ptr := wr_ptr + 1.U
    wrRangeCounter := Mux(wrRangeCounter === (numPacketPerRange - 1).U, 0.U, wrRangeCounter + 1.U)
    // Increment packet ID when starting a new range
    when(wrRangeCounter === (numPacketPerRange - 1).U) {
      pktID := pktID + 1.U
    }
    wr_cnt := wr_cnt + 1.U // Increment write counter
  }

  // Read clock domain
  withClock(io.pcie_clock) {
    val rd_ptr = RegInit(0.U(fifo_addr_width.W))
    val wr_cnt_sync = Delayer(wr_cnt, 2) // Synchronize write counter to read clock domain
    val rd_occupancy = wr_cnt_sync - rd_cnt // Calculate occupancy in read clock domain

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
    }.otherwise {
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
    val difftest = Flipped(new FpgaDiffIO(diffWidth))
    val to_host_axis = new AXI4Stream(axisWidth)
    val pcie_clock = Input(Clock())
  })

  // Instantiate the converter module
  val diff2axis = Module(new Difftest2AXIs(diffWidth, axisWidth))

  // Connect clock and reset signals
  diff2axis.io.pcie_clock := io.pcie_clock
  diff2axis.io.reset := reset
  PipelineConnect(io.difftest, diff2axis.io.difftest, true.B)

  // AXI-Stream output domain (PCIe clock)
  io.to_host_axis <> diff2axis.io.axis
}
