/***************************************************************************************
 * Copyright (c) 2026 Beijing Institute of Open Source Chip
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
import difftest.common.AXI4Bundle

private class AXI4DynamicChannelDelayer(
  dataWidth: Int,
  depth: Int,
) extends Module {
  require(dataWidth > 0, s"channel width must be positive, got $dataWidth")
  require(depth > 0, s"channel delay depth must be positive, got $depth")

  private val cycleWidth = 64
  private val entryWidth = dataWidth + cycleWidth

  val io = IO(new Bundle {
    val enable = Input(Bool())
    val currentCycle = Input(UInt(cycleWidth.W))
    val releaseCycle = Input(UInt(cycleWidth.W))
    val in = Flipped(Decoupled(UInt(dataWidth.W)))
    val out = Decoupled(UInt(dataWidth.W))
  })

  private val inputEntry = Cat(io.in.bits, io.releaseCycle)
  private val headEntry = Reg(UInt(entryWidth.W))
  private val headValid = RegInit(false.B)
  private val headReleaseCycle = headEntry(cycleWidth - 1, 0)
  private val released = headValid && io.currentCycle >= headReleaseCycle
  private val headFire = io.enable && released && io.out.ready

  io.out.valid := io.enable && released
  io.out.bits := headEntry(entryWidth - 1, cycleWidth)

  if (depth == 1) {
    io.in.ready := io.enable && (!headValid || headFire)

    when(io.enable) {
      when(headFire) {
        headValid := false.B
      }
      when(io.in.fire) {
        headEntry := inputEntry
        headValid := true.B
      }
    }
  } else {
    val tail = Module(new Queue(UInt(entryWidth.W), depth - 1, pipe = true, flow = false))
    val inputToHead = !headValid || (headFire && !tail.io.deq.valid)

    tail.io.enq.valid := io.enable && io.in.valid && !inputToHead
    tail.io.enq.bits := inputEntry
    tail.io.deq.ready := headFire
    io.in.ready := io.enable && (inputToHead || tail.io.enq.ready)

    when(io.enable) {
      when(headFire) {
        when(tail.io.deq.valid) {
          headEntry := tail.io.deq.bits
          headValid := true.B
        }.elsewhen(io.in.fire) {
          headEntry := inputEntry
          headValid := true.B
        }.otherwise {
          headValid := false.B
        }
      }.elsewhen(!headValid && io.in.fire) {
        headEntry := inputEntry
        headValid := true.B
      }
    }
  }

  private val stalled = RegNext(io.enable && io.out.valid && !io.out.ready, false.B)
  private val stalledBits = RegEnable(io.out.bits, io.enable && io.out.valid && !io.out.ready)
  when(stalled && io.enable) {
    assert(io.out.valid)
    assert(io.out.bits === stalledBits)
  }
  when(io.out.fire) {
    assert(io.currentCycle >= headReleaseCycle)
  }
}

class AXI4DynamicDelayer(
  axiType: AXI4Bundle,
  readAddressDepth: Int = 64,
  readDataDepth: Int = 64,
  writeAddressDepth: Int = 64,
  writeDataDepth: Int = 64,
  writeResponseDepth: Int = 64,
) extends Module {
  private val addrWidth = axiType.addrWidth
  private val dataWidth = axiType.dataWidth
  private val idWidth = axiType.idWidth
  private val userWidth = axiType.userWidth

  val io = IO(new Bundle {
    val enable = Input(Bool())
    val delayCycles = Input(UInt(32.W))
    val in = Flipped(new AXI4Bundle(addrWidth, dataWidth, idWidth, userWidth))
    val out = new AXI4Bundle(addrWidth, dataWidth, idWidth, userWidth)
  })

  private val cycle = RegInit(0.U(64.W))
  cycle := cycle + 1.U
  private val releaseCycle = cycle + io.delayCycles

  private def delayChannel[T <: Data](
    producer: DecoupledIO[T],
    consumer: DecoupledIO[T],
    depth: Int,
    name: String,
  ): Unit = {
    val delay = Module(new AXI4DynamicChannelDelayer(producer.bits.getWidth, depth)).suggestName(name)
    delay.io.enable := io.enable
    delay.io.currentCycle := cycle
    delay.io.releaseCycle := releaseCycle
    delay.io.in.valid := producer.valid
    delay.io.in.bits := producer.bits.asUInt
    producer.ready := delay.io.in.ready
    consumer.valid := delay.io.out.valid
    consumer.bits := delay.io.out.bits.asTypeOf(consumer.bits)
    delay.io.out.ready := consumer.ready
  }

  delayChannel(io.in.aw, io.out.aw, writeAddressDepth, "awDelay")
  delayChannel(io.in.w, io.out.w, writeDataDepth, "wDelay")
  delayChannel(io.out.b, io.in.b, writeResponseDepth, "bDelay")
  delayChannel(io.in.ar, io.out.ar, readAddressDepth, "arDelay")
  delayChannel(io.out.r, io.in.r, readDataDepth, "rDelay")
}
