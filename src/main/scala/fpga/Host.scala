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
import difftest.common.AXI4Stream
import difftest.gateway.FpgaDiffIO
import difftest.util.PipelineConnect

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
  val payload_width = axis_send_len * axis_width
  val payload_pad_width = payload_width - difftest_width - pkt_id_w
  val fifo_depth = 16

  // Read clock domain
  withClock(io.pcie_clock) {
    val fifo = Module(new AsyncClockFIFO(UInt(difftest_width.W), fifo_depth, axis_width))
    fifo.io.enqClock := clock
    fifo.io.enq <> io.difftest

    val rangeActive = RegInit(false.B)
    val packetValid = RegInit(false.B)
    val packetBeats = RegInit(VecInit(Seq.fill(axis_send_len)(0.U(axis_width.W))))
    val sendCnt = RegInit(0.U(log2Ceil(axis_send_len).W))
    val sendLast = sendCnt === (axis_send_len - 1).U
    val counter = RegInit(0.U(3.W)) // 0 to 7
    val currentPktID = RegInit(0.U(pkt_id_w.W))
    val sendPacketEnd = counter === (numPacketPerRange - 1).U

    val startTransfer = !rangeActive && fifo.io.deq.valid
    val loadPacket = fifo.io.deq.valid && (!rangeActive || !packetValid)

    val packetPayload =
      if (payload_pad_width > 0) {
        Cat(0.U(payload_pad_width.W), fifo.io.deq.bits, currentPktID)
      } else {
        Cat(fifo.io.deq.bits, currentPktID)
      }
    val packetPayloadBeats = packetPayload.asTypeOf(Vec(axis_send_len, UInt(axis_width.W)))

    fifo.io.deq.ready := loadPacket

    when(startTransfer) {
      rangeActive := true.B
      counter := 0.U
    }

    when(loadPacket) {
      packetBeats := packetPayloadBeats
      packetValid := true.B
      sendCnt := 0.U
    }.elsewhen(packetValid && io.axis.fire) {
      when(sendLast) {
        packetValid := false.B
        sendCnt := 0.U
        when(sendPacketEnd) {
          // Last packet in range, finish transfer
          rangeActive := false.B
          counter := 0.U
          currentPktID := currentPktID + 1.U // Increment ID for next range
        }.otherwise {
          counter := counter + 1.U
        }
      }.otherwise {
        sendCnt := sendCnt + 1.U
      }
    }

    // AXI output
    io.axis.valid := packetValid
    io.axis.bits.data := packetBeats(sendCnt)
    io.axis.bits.keep := Fill(axis_width / 8, 1.U(1.W))
    io.axis.bits.last := packetValid && sendLast && sendPacketEnd
  }
}

class HostEndpoint(
  val diffWidth: Int,
  val axisWidth: Int,
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
  PipelineConnect(io.difftest, diff2axis.io.difftest, diff2axis.io.difftest.fire)

  // AXI-Stream output domain (PCIe clock)
  io.to_host_axis <> diff2axis.io.axis
}
