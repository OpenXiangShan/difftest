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
import chisel3.util._
import chisel3.reflect.DataMirror

class LogPerfControl extends Bundle {
  val timer = UInt(64.W)
  val logEnable = Bool()
  val clean = Bool()
  val dump = Bool()
}

private class LogPerfHelper extends BlackBox with HasBlackBoxInline {
  val io = IO(Output(new LogPerfControl))

  val cppExtModule =
    """
      |void LogPerfHelper (
      |  uint64_t& timer,
      |  uint8_t&  logEnable,
      |  uint8_t&  clean,
      |  uint8_t&  dump
      |) {
      |  timer     = difftest$$timer;
      |  logEnable = difftest$$log_enable;
      |  clean     = difftest$$perfCtrl$$clean;
      |  dump      = difftest$$perfCtrl$$dump;
      |}
      |""".stripMargin
  difftest.DifftestModule.createCppExtModule("LogPerfHelper", cppExtModule)

  val verilog =
    """`ifndef SIM_TOP_MODULE_NAME
      |  `define SIM_TOP_MODULE_NAME SimTop
      |`endif
      |
      |/*verilator tracing_off*/
      |/*verilator coverage_off*/
      |
      |module LogPerfHelper(
      |  output [63:0] timer,
      |  output        logEnable,
      |  output        clean,
      |  output        dump
      |);
      |
      |assign timer     = `SIM_TOP_MODULE_NAME.difftest_timer;
      |assign logEnable = `SIM_TOP_MODULE_NAME.difftest_log_enable;
      |assign clean     = `SIM_TOP_MODULE_NAME.difftest_perfCtrl_clean;
      |assign dump      = `SIM_TOP_MODULE_NAME.difftest_perfCtrl_dump;
      |
      |endmodule
      |
      |""".stripMargin
  setInline("LogPerfHelper.v", verilog)
}

object LogPerfControl {
  private val instances = scala.collection.mutable.ListBuffer.empty[LogPerfControl]
  private def instantiate(): LogPerfControl = instances.addOne(WireInit(Module(new LogPerfHelper).io)).last

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
