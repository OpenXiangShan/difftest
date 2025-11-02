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
import chisel3.experimental.ExtModule
import chisel3.util.HasBlackBoxInline
import difftest.gateway.FpgaDiffIO

class PacketBuffer(val data_width: Int, val pkt_num: Int) extends Module {
  val addr_width = log2Ceil(pkt_num)
  val wr = IO(Input(new Bundle {
    val en = Bool()
    val addr = UInt(addr_width.W)
    val data = UInt(data_width.W)
  }))
  val rd = IO(new Bundle {
    val addr = Input(UInt(addr_width.W))
    val data = Output(UInt(data_width.W))
  })

  def write(en: Bool, addr: UInt, data: UInt): Unit = {
    wr.en := en
    wr.addr := addr
    wr.data := data
  }
  def read(addr: UInt): UInt = {
    rd.addr := addr
    rd.data
  }
  val block_width = 4000
  val block_num = data_width / block_width
  val rd_data_vec = Seq.tabulate(block_num) { idx =>
    val (hi, lo) = ((idx + 1) * block_width - 1, idx * block_width)
    val ram = SyncReadMem(pkt_num, UInt(block_width.W))
    when(wr.en) {
      ram.write(wr.addr, wr.data(hi, lo))
    }
    ram.read(rd.addr)
  }
  rd.data := Cat(rd_data_vec.reverse)
}

class Difftest2AXIs(val difftest_width: Int, val axis_width: Int) extends Module {
  val io = IO(new Bundle {
    val clock = Input(Clock())
    val reset = Input(Bool())
    val difftest = Input(new FpgaDiffIO(difftest_width))
    val clock_enable = Output(Bool())
    val axis = new AXI4Stream(axis_width)
  })
  val packet_num = 8
  val axis_send_len = (difftest_width + 8 + axis_width - 1) / axis_width
  val pkt_id_w = 8 // pkt = (difftest_data, pkt_id)

  val inTransfer = RegInit(false.B)

  val pktID = RegInit(0.U(pkt_id_w.W))

  val wrBuf = RegInit(0.U(1.W))
  val rdBuf = RegInit(0.U(1.W))
  val wrPkt = RegInit(0.U(8.W))
  val rdPkt = RegInit(0.U(8.W))
  val lastPkt = rdPkt === (packet_num - 1).U
  val nextRdPkt = Mux(lastPkt || !inTransfer, 0.U, rdPkt + 1.U) // Sync Read, addr for next read

  val buf_v = RegInit(VecInit.fill(2)(false.B))
  val buf_wen = io.difftest.enable & io.clock_enable
  val buf_clear = io.axis.valid && io.axis.bits.last
  val buf_rdata_vec = VecInit.tabulate(2) { idx =>
    val buf = Module(new PacketBuffer(difftest_width, packet_num))
    buf.clock := clock
    buf.reset := reset
    buf.write(buf_wen && wrBuf === idx.U, wrPkt, io.difftest.data)
    buf.read(nextRdPkt)
  }
  val buf_rdata = buf_rdata_vec(rdBuf)

  // Backpressure
  io.clock_enable := !buf_v.asUInt.andR

  // Write
  when(buf_wen) {
    wrPkt := wrPkt + 1.U
    when(wrPkt === (packet_num - 1).U) {
      buf_v(wrBuf) := true.B
      wrPkt := 0.U
      wrBuf := ~wrBuf
    }
  }
  when(buf_clear) {
    buf_v(rdBuf) := false.B
  }

  // Read
  val mix_data = RegInit(0.U((difftest_width + pkt_id_w).W))
  io.axis.valid := inTransfer
  io.axis.bits.data := mix_data(axis_width - 1, 0)
  val sendCnt = RegInit(0.U(8.W))
  val lastSend = WireInit(sendCnt === (axis_send_len - 1).U)
  io.axis.bits.last := lastPkt && lastSend

  when(inTransfer) {
    when(io.axis.fire) {
      when(lastSend) {
        sendCnt := 0.U
        mix_data := Cat(buf_rdata, 0.U(pkt_id_w.W))
        rdPkt := nextRdPkt
        when(lastPkt) {
          inTransfer := false.B
          rdBuf := ~rdBuf
          rdPkt := 0.U
        }
      }.otherwise {
        sendCnt := sendCnt + 1.U
        mix_data := mix_data >> axis_width
      }
    }
  }.otherwise { // Idle
    when(buf_v(rdBuf)) {
      mix_data := Cat(buf_rdata, pktID) // first pkt in buffer
      pktID := pktID + 1.U
      inTransfer := true.B
    }
  }
}

class HostEndpoint(
  val diffWidth: Int,
  val axisWidth: Int = 512,
) extends Module {
  val io = IO(new Bundle {
    val difftest = Input(new FpgaDiffIO(diffWidth))
    val toHost_axis = new AXI4Stream(axisWidth)
    val clock_enable = Output(Bool())
  })
  val Difftest2AXI = Module(new Difftest2AXIs(diffWidth, axisWidth))
  Difftest2AXI.io.clock := clock
  Difftest2AXI.io.reset := reset
  Difftest2AXI.io.difftest := io.difftest
  io.toHost_axis <> Difftest2AXI.io.axis
  io.clock_enable := Difftest2AXI.io.clock_enable
}
