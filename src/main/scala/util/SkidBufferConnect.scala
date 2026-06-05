/***************************************************************************************
 * Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.util

import chisel3._
import chisel3.util._

object SkidBufferConnect {
  def apply[T <: Data](left: DecoupledIO[T], right: DecoupledIO[T], splitDepth: Int = 0): T = {
    require(splitDepth >= 0)
    val full = RegInit(false.B)
    val hold = !full && left.valid && !right.ready

    left.ready := !full
    right.valid := left.valid || full
    connectBits(left.bits, right.bits, hold, full, splitDepth)

    when(full) {
      when(right.ready) {
        full := false.B
      }
    }.otherwise {
      when(left.valid && !right.ready) {
        full := true.B
      }
    }

    right.bits
  }

  def apply[T <: Data](left: DecoupledIO[T], splitDepth: Int): DecoupledIO[T] = {
    val right = Wire(Decoupled(chiselTypeOf(left.bits)))
    apply(left, right, splitDepth)
    right
  }

  def apply[T <: Data](left: DecoupledIO[T]): DecoupledIO[T] = apply(left, splitDepth = 0)

  private def connectBits(left: Data, right: Data, hold: Bool, useHeld: Bool, depth: Int): Unit = {
    (left, right) match {
      case (leftVec: Vec[_], rightVec: Vec[_]) if depth > 0 =>
        require(leftVec.length == rightVec.length)
        leftVec.zip(rightVec).foreach { case (leftElem, rightElem) =>
          connectBits(leftElem, rightElem, hold, useHeld, depth - 1)
        }
      case (leftRecord: Record, rightRecord: Record) if depth > 0 =>
        leftRecord.elements.foreach { case (name, leftElem) =>
          connectBits(leftElem, rightRecord.elements(name), hold, useHeld, depth - 1)
        }
      case _ =>
        connectLeaf(left, right, hold, useHeld)
    }
  }

  private def connectLeaf(left: Data, right: Data, hold: Bool, useHeld: Bool): Unit = {
    val held = Reg(chiselTypeOf(left))
    when(hold) {
      held := left
    }
    right := Mux(useHeld, held, left)
  }
}
