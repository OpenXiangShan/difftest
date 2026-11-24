/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

package difftest.matrix

import chisel3._
import chisel3.util.log2Ceil
import difftest.{DiffAmuWritebackBlock, DiffAmuWritebackEvent}

object MatrixWritebackBatch {
  // The producer writes every bank at one common address and signals completion separately.
  def apply(in: DiffAmuWritebackEvent): (DiffAmuWritebackBlock, DiffAmuWritebackEvent) = {
    val block = Wire(new DiffAmuWritebackBlock(in.data.length))
    val size = block.bankAddr.length
    val pendingAddr = Reg(Vec(size - 1, chiselTypeOf(in.bankAddr.head)))
    val pendingData = Reg(Vec(size - 1, chiselTypeOf(in.data)))
    val count = RegInit(0.U(log2Ceil(size).W))
    val emit = in.valid && (in.finish || count === (size - 1).U)

    when(in.valid) {
      when(in.finish) {
        assert(!in.bankValid.orR)
      }.otherwise {
        assert(in.bankValid.andR && in.bankAddr.map(_ === in.bankAddr.head).reduce(_ && _))
      }
      when(emit) {
        count := 0.U
      }.otherwise {
        pendingAddr(count) := in.bankAddr.head
        pendingData(count) := in.data
        count := count + 1.U
      }
    }

    block.valid := emit && (!in.finish || count =/= 0.U)
    block.coreid := in.coreid
    block.pc := in.pc
    block.count := count +& !in.finish
    block.finish := in.finish
    for (i <- 0 until size) {
      val current = !in.finish && count === i.U
      val addr = if (i == size - 1) in.bankAddr.head else Mux(current, in.bankAddr.head, pendingAddr(i))
      val data = if (i == size - 1) in.data else Mux(current, in.data, pendingData(i))
      val active = i.U < block.count
      block.bankAddr(i) := Mux(active, addr, 0.U)
      for (j <- in.data.indices) {
        block.data(i * in.data.length + j) := Mux(active, data(j), 0.U)
      }
    }

    val finish = WireInit(0.U.asTypeOf(chiselTypeOf(in)))
    finish.valid := emit && in.finish && count === 0.U
    finish.finish := true.B
    finish.pc := in.pc
    finish.coreid := in.coreid
    (block, finish)
  }
}
