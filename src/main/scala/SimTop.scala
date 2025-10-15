/***************************************************************************************
 * Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest

import chisel3._
import chisel3.util._
import difftest.DifftestModule
import difftest.common.DifftestWiring
import difftest.fpga.HostEndpoint
import difftest.fpga.xdma._

class DifftestTopIO extends Bundle {
  val exit = Output(UInt(64.W))
  val step = Output(UInt(64.W))
  val perfCtrl = new PerfCtrlIO
  val logCtrl = new LogCtrlIO
  val uart = new UARTIO
}

class PerfCtrlIO extends Bundle {
  val clean = Input(Bool())
  val dump = Input(Bool())
}

class LogCtrlIO extends Bundle {
  val begin = Input(UInt(64.W))
  val end = Input(UInt(64.W))
  val level = Input(UInt(64.W)) // a cpp uint

  def enable(timer: UInt): Bool = {
    val en = WireInit(false.B)
    en := timer >= begin && timer < end
    en
  }
}

// UART IO: input/output through the simulation framework
class UARTIO extends Bundle {
  val out = new Bundle {
    val valid = Output(Bool())
    val ch = Output(UInt(8.W))
  }
  val in = new Bundle {
    val valid = Output(Bool())
    val ch = Input(UInt(8.W))
  }
}

trait HasDiffTestInterfaces {
  def cpuName: Option[String] = None

  def connectTopIOs(difftest: DifftestTopIO): Unit
}

// Top-level module for DiffTest simulation. Will be created by DifftestModule.finish
class SimTop[T <: Module with HasDiffTestInterfaces](cpuGen: => T) extends Module {
  val cpu = Module(cpuGen)

  val cpuName = cpu.cpuName.getOrElse(cpu.getClass.getName.split("\\.").last)
  val gateway = DifftestModule.collect(cpuName)

  // IO: difftest_*
  val difftest = IO(new DifftestTopIO)
  
  // IO: ref_clock (only when FPGA IO is available)
  // In FPGA DiffTest mode, drive CPU clock from an external reference clock
  private val ref_clock = Option.when(gateway.fpgaIO.isDefined)(IO(Input(Clock())))
  // IO: core_clock_enable (mirror from HostEndpoint when FPGA IO is available)
  private val core_clock_enable = Option.when(gateway.fpgaIO.isDefined)(IO(Output(Bool())))

  difftest.exit := gateway.exit.getOrElse(0.U)
  difftest.step := gateway.step.getOrElse(0.U)

  val timer = RegInit(0.U(64.W)).suggestName("timer")
  timer := timer + 1.U
  dontTouch(timer)

  val log_enable = difftest.logCtrl.enable(timer).suggestName("log_enable")
  dontTouch(log_enable)

  difftest.uart := DontCare

  cpu.connectTopIOs(difftest)

  // IO: hostendpoint_* (only when FPGA batch IO is available)
  Option.when(gateway.fpgaIO.isDefined) {
    val fpgaIO = gateway.fpgaIO.get

    // Instantiate HostEndpoint with matching data width
    val host = withClock(ref_clock.get) { Module(new HostEndpoint(
      dataWidth = fpgaIO.data.getWidth,
      axisDataWidth = 512,
    )) }
    // Connect difftest batch data and enable
    host.io.difftest_data := fpgaIO.data
    host.io.difftest_enable := fpgaIO.enable

    // Expose AXIS to top-level
    val host_c2h_axis = IO(new AxisMasterBundle(512))
    host_c2h_axis.valid := host.io.host_c2h_axis.valid
    host_c2h_axis.data  := host.io.host_c2h_axis.data
    host_c2h_axis.last  := host.io.host_c2h_axis.last
    host.io.host_c2h_axis.ready := host_c2h_axis.ready

    // Expose additional control signals to top-level
    core_clock_enable.get := host.io.core_clock_enable
  }

  // There should not be anymore IOs
  require(DifftestWiring.isEmpty, s"pending wires left: ${DifftestWiring.getPending}")
}
