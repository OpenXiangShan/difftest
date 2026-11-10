/***************************************************************************************
 * Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.common

import chisel3._
import chisel3.reflect.DataMirror
import chisel3.util.experimental.BoringUtils

class LogPerfControl extends Bundle {
  val timer = UInt(64.W)
  val logEnable = Bool()
  val clean = Bool()
  val dump = Bool()
}

object LogPerfControl {
  private val instances = scala.collection.mutable.ListBuffer.empty[LogPerfControl]
  private var source: LogPerfControl = _

  def setSource(control: LogPerfControl): Unit = source = control

  private def instantiate(): LogPerfControl = instances.addOne(WireInit(BoringUtils.bore(source))).last

  def apply(): LogPerfControl = instances.find(DataMirror.isVisible).getOrElse(instantiate())
}

object DifftestPerf {
  def apply(perfName: String, perfCnt: UInt) = {
    val helper = LogPerfControl.apply()
    val counter = RegInit(0.U(64.W))
    val next_counter = WireInit(counter + perfCnt)
    counter := Mux(helper.clean, 0.U, next_counter)
    when(helper.dump) {
      printf(p"[DIFFTEST_PERF][time=${helper.timer}] $perfName, $next_counter\n")
    }
  }
}
