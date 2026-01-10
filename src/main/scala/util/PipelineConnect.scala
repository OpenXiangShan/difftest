/***************************************************************************************
 * Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
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
import difftest.DifftestBundle

object PipelineConnect {
  private def connect[T <: Data](left: DecoupledIO[T], right: DecoupledIO[T], rightOutFire: Bool): T = {
    val valid = RegInit(false.B)
    when(rightOutFire) { valid := false.B }
    when(left.valid && right.ready) { valid := true.B }

    val data = RegEnable(left.bits, left.valid && right.ready)
    left.ready := right.ready
    right.bits := data
    right.valid := valid
    data
  }

  def apply[T <: Data](left: DecoupledIO[T], right: DecoupledIO[T], rightOutFire: Bool): T =
    connect(left, right, rightOutFire)

  def apply(
    left: DecoupledIO[MixedVec[Valid[DifftestBundle]]],
    right: DecoupledIO[MixedVec[Valid[DifftestBundle]]],
    rightOutFire: Bool,
  ): MixedVec[Valid[DifftestBundle]] = {
    val data = connect(left, right, rightOutFire)
    right.bits.zip(data).foreach { case (r, d) =>
      val valid = d.valid && right.fire
      r.valid := valid
      r.bits.bits.getValidOption.foreach(_ := valid)
    }
    data
  }
}
