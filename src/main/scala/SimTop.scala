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

package difftest

import chisel3._
import chisel3.experimental.prefix
import difftest.common.DifftestWiring
import difftest.fpga.HostEndpoint

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
  val level = Input(UInt(64.W))

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

  def connectTopIOs(difftest: DifftestTopIO): Seq[Data] = Seq.empty
  def connectTopIOsWithName(difftest: DifftestTopIO): Seq[(Data, String)] = connectTopIOs(difftest).map { gen =>
    (gen, gen.instanceName)
  }
}

// Top-level module for DiffTest simulation. Will be created by DifftestModule.top
class SimTop[T <: RawModule with HasDiffTestInterfaces](cpuGen: => T) extends Module {
  val cpu = Module(cpuGen)

  val cpuName = cpu.cpuName.getOrElse(cpu.getClass.getName.split("\\.").last)
  val gateway = DifftestModule.collect(cpuName)

  // IO: difftest_*
  val difftest = IO(new DifftestTopIO)

  difftest.exit := gateway.exit.getOrElse(0.U)
  difftest.step := gateway.step.getOrElse(0.U)
  difftest.uart := DontCare

  prefix("difftest") {
    // Required signals for LogPerfControl
    val timer = RegInit(0.U(64.W))
    timer := timer + 1.U
    dontTouch(timer)

    val log_enable = difftest.logCtrl.enable(timer)
    dontTouch(log_enable)

    // IO: difftest_fpga_*
    gateway.fpgaIO.map { fpgaIO =>
      val ref_clock = IO(Input(Clock()))
      val host = withClock(ref_clock) { Module(new HostEndpoint(fpgaIO.data.getWidth)) }
      host.io.difftest := fpgaIO

      val toHost = host.io.to_host_axis
      val to_host_axis = IO(chiselTypeOf(toHost))
      to_host_axis <> toHost

      val clock_enable = IO(Output(Bool()))
      clock_enable := host.io.clock_enable
    }
  }

  val cpuIO = cpu.connectTopIOsWithName(difftest)
  cpuIO.foreach { case (gen, name) =>
    val io = IO(chiselTypeOf(gen)).suggestName(name)
    io <> gen
  }

  // There should not be anymore IOs
  require(DifftestWiring.isEmpty, s"pending wires left: ${DifftestWiring.getPending}")
}
