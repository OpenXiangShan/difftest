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
import difftest.common.{AXI4Bundle, AXI4BundleA}

private class AXI4DynamicAddressDelayer(
  addrWidth: Int,
  idWidth: Int,
  userWidth: Int,
  depth: Int,
  orderById: Boolean,
) extends Module {
  require(depth > 0, s"address delay depth must be positive, got $depth")

  private val indexWidth = log2Ceil(depth).max(1)

  val io = IO(new Bundle {
    val enable = Input(Bool())
    val delayCycles = Input(UInt(32.W))
    val in = Flipped(Decoupled(new AXI4BundleA(addrWidth, idWidth, userWidth)))
    val out = Decoupled(new AXI4BundleA(addrWidth, idWidth, userWidth))
  })

  private val cycle = RegInit(0.U(64.W))
  private val nextSequence = RegInit(0.U(64.W))
  private val valid = RegInit(VecInit(Seq.fill(depth)(false.B)))
  private val entries = Reg(Vec(depth, new AXI4BundleA(addrWidth, idWidth, userWidth)))
  private val releaseCycles = Reg(Vec(depth, UInt(64.W)))
  private val sequences = Reg(Vec(depth, UInt(64.W)))

  cycle := cycle + 1.U

  private val eligible = Wire(Vec(depth, Bool()))
  for (i <- 0 until depth) {
    val olderEntry = (0 until depth).map { j =>
      val sameOrderDomain = if (orderById) entries(j).id === entries(i).id else true.B
      valid(j) && sameOrderDomain && sequences(j) < sequences(i)
    }
      .reduce(_ || _)
    eligible(i) := valid(i) && cycle >= releaseCycles(i) && !olderEntry
  }

  private val (selectedValid, selectedIndex, _) =
    (0 until depth).foldLeft((false.B, 0.U(indexWidth.W), 0.U(64.W))) { case ((found, index, sequence), i) =>
      val select = eligible(i) && (!found || sequences(i) < sequence)
      (
        found || eligible(i),
        Mux(select, i.U(indexWidth.W), index),
        Mux(select, sequences(i), sequence),
      )
    }

  private val locked = RegInit(false.B)
  private val lockedIndex = Reg(UInt(indexWidth.W))
  private val activeIndex = Mux(locked, lockedIndex, selectedIndex)
  private val activeValid = Mux(locked, valid(lockedIndex), selectedValid)
  private val empty = !valid.asUInt.orR
  private val bypass = io.enable && empty && !locked && io.delayCycles === 0.U

  io.out.valid := io.enable && Mux(bypass, io.in.valid, activeValid)
  io.out.bits := Mux(
    bypass,
    io.in.bits.asUInt,
    Mux(activeValid, entries(activeIndex).asUInt, 0.U(io.out.bits.getWidth.W)),
  ).asTypeOf(io.out.bits)

  private val freeMask = ~valid.asUInt
  private val hasFree = freeMask.orR
  private val freeIndex = PriorityEncoder(freeMask)
  io.in.ready := io.enable && Mux(bypass, io.out.ready, hasFree)

  private val store = io.in.fire && !bypass
  private val remove = io.out.fire && !bypass
  private val nextValid = WireDefault(valid)

  when(store) {
    nextValid(freeIndex) := true.B
    entries(freeIndex) := io.in.bits
    releaseCycles(freeIndex) := cycle + io.delayCycles
    sequences(freeIndex) := nextSequence
    nextSequence := nextSequence + 1.U
  }
  when(remove) {
    nextValid(activeIndex) := false.B
  }
  valid := nextValid

  when(io.enable && !locked && selectedValid && !io.out.ready) {
    locked := true.B
    lockedIndex := selectedIndex
  }
  when(remove) {
    locked := false.B
  }

  private val stalled = RegNext(io.enable && io.out.valid && !io.out.ready, false.B)
  private val stalledBits = RegEnable(io.out.bits.asUInt, io.enable && io.out.valid && !io.out.ready)
  when(stalled && io.enable) {
    assert(io.out.valid)
    assert(io.out.bits.asUInt === stalledBits)
  }
  when(locked) {
    assert(valid(lockedIndex))
  }
  when(remove) {
    assert(activeValid)
    assert(cycle >= releaseCycles(activeIndex))
    for (i <- 0 until depth) {
      val sameOrderDomain = if (orderById) entries(i).id === entries(activeIndex).id else true.B
      assert(!(valid(i) && sameOrderDomain && sequences(i) < sequences(activeIndex)))
    }
  }
}

class AXI4DynamicDelayer(
  axiType: AXI4Bundle,
  readDepth: Int = 64,
  writeDepth: Int = 64,
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

  private val readDelay = Module(
    new AXI4DynamicAddressDelayer(addrWidth, idWidth, userWidth, readDepth, orderById = true)
  )
  private val writeDelay = Module(
    new AXI4DynamicAddressDelayer(addrWidth, idWidth, userWidth, writeDepth, orderById = false)
  )

  readDelay.io.enable := io.enable
  readDelay.io.delayCycles := io.delayCycles
  readDelay.io.in <> io.in.ar
  io.out.ar <> readDelay.io.out

  writeDelay.io.enable := io.enable
  writeDelay.io.delayCycles := io.delayCycles
  writeDelay.io.in <> io.in.aw
  io.out.aw <> writeDelay.io.out

  io.out.w <> io.in.w
  io.in.b <> io.out.b
  io.in.r <> io.out.r
}
