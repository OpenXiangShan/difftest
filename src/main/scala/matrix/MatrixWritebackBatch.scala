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
import difftest.DiffAmuWritebackEvent

object MatrixWritebackBatch {
  def apply(in: DiffAmuWritebackEvent): Vec[DiffAmuWritebackEvent] = {
    val size = 8
    val pending = Reg(Vec(size - 1, chiselTypeOf(in)))
    val count = RegInit(0.U(log2Ceil(size).W))
    val emit = in.valid && (in.finish || count === (size - 1).U)

    when(in.valid) {
      when(emit) {
        count := 0.U
      }.otherwise {
        pending(count) := in
        count := count + 1.U
      }
    }

    // The current record follows the buffered records, including on completion.
    val out = Wire(Vec(size, chiselTypeOf(in)))
    for ((slot, i) <- out.zipWithIndex) {
      slot := (if (i < size - 1) Mux(i.U < count, pending(i), in) else in)
      slot.valid := emit && i.U <= count
      slot.index := i.U
    }
    out
  }
}
