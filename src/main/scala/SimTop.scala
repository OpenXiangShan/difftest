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
import chisel3.experimental.dataview._
import chisel3.reflect.DataMirror
import difftest.common.{
  AXI4Bundle,
  AXI4LiteBundle,
  AXI4Stream,
  DifftestWiring,
  VerilogAXI4LiteRecord,
  VerilogAXI4Record,
  VerilogAXI4StreamRecord,
}
import difftest.fpga.{DifftestMemCtrl, HostEndpoint, XDMAConfigBar, XDMAHostCtrlIO}

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

case class DifftestMemIO(cpu: Record, mem: Option[Record] = None)

trait HasDiffTestInterfaces extends ImplicitClock with ImplicitReset { this: RawModule =>
  def cpuName: Option[String] = None

  private[difftest] def dutClock = implicitClock
  private[difftest] def dutReset = implicitReset
  private[difftest] def dutIOs: Seq[(String, Data)] = {
    val memPorts = difftestMemIO.toSeq.flatMap { memIO =>
      memIO.cpu +: memIO.mem.toSeq
    }
    def memPortFilter(data: Data): Boolean = memPorts.contains(data) && DifftestModule.isFPGA
    def portFilter(port: (String, Data)): Boolean = {
      memPortFilter(port._2) ||
      port._1.contains("bore") || // Internal port created by BoringUtils, connected to difftest only
      (port._2 match {
        case u: UARTIO => true // UART port for simulation framework, connected to difftest only
        case b: Bundle => b.elements.toSeq.exists(portFilter)
        case other     => Seq(dutClock, dutReset).contains(other)
      })
    }
    def portCollect(ports: Seq[(String, Data)]): Seq[(String, Data)] = {
      ports.filterNot(portFilter) ++
        ports.filter(portFilter).flatMap { case (root, data) =>
          if (memPortFilter(data)) {
            Seq.empty
          } else
            data match {
              case u: UARTIO => Seq.empty
              case b: Bundle => portCollect(b.elements.toSeq.map { case (n, d) => (s"${root}_${n}", d) })
              case _         => Seq.empty
            }
        }
    }
    portCollect(DataMirror.modulePorts(this))
  }

  def connectTopIOs(difftest: DifftestTopIO): Unit = {}
  def difftestMemIO: Option[DifftestMemIO] = None
}

// Top-level module for DiffTest simulation. Will be created by DifftestModule.top
class SimTop[T <: RawModule with HasDiffTestInterfaces](cpuGen: => T, modPrefix: Option[String]) extends Module {
  override def desiredName: String = modPrefix.getOrElse("") + super.desiredName
  val cpu = Module(cpuGen)
  cpu.dutClock := clock

  val cpuName = cpu.cpuName.getOrElse(cpu.getClass.getName.split("\\.").last)
  val fpgaHostReset = WireInit(false.B)
  val fpgaDiffEnable = WireInit(true.B)
  val gateway = withReset(reset.asBool || fpgaHostReset) {
    DifftestModule.collect(cpuName)
  }

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

    val ref_clock = Option.when(gateway.refClock.isDefined)(IO(Input(Clock())))
    gateway.refClock.foreach(_ := ref_clock.get)

    // IO: difftest_fpga_*
    gateway.fpgaIO.foreach { fpgaIO =>
      val ref_reset = IO(Input(Bool()))
      val cfgResetReq = WireInit(false.B)
      val cfgReset = withClockAndReset(ref_clock.get, ref_reset) {
        val cnt = RegInit(0.U(4.W))
        when(cfgResetReq && cnt === 0.U) {
          cnt := 10.U
        }.elsewhen(cnt =/= 0.U) {
          cnt := cnt - 1.U
        }
        cnt =/= 0.U
      }
      withClockAndReset(ref_clock.get, ref_reset || cfgReset) {
        val cfg = Module(new XDMAConfigBar)
        cfgResetReq := cfg.io.cfgReset
        val ctrl = cfg.io.hostCtrl
        val host = Module(new HostEndpoint(fpgaIO.bits.getWidth))

        host.io.difftest.valid := fpgaIO.valid && ctrl.diffEnable
        host.io.difftest.bits := fpgaIO.bits
        fpgaIO.ready := Mux(ctrl.diffEnable, host.io.difftest.ready, true.B)
        val pcie_clock = IO(Input(Clock()))
        host.io.pcie_clock := pcie_clock

        val toHost = host.io.to_host_axis
        val to_host_axis = IO(VerilogAXI4StreamRecord.typeOf(toHost))
        to_host_axis.viewAs[AXI4Stream] <> toHost

        val from_host_axis = IO(Flipped(new VerilogAXI4StreamRecord(512)))

        val cfg_axilite = IO(Flipped(new VerilogAXI4LiteRecord(32, 32)))
        cfg.io.axilite <> cfg_axilite.viewAs[AXI4LiteBundle]

        val hostCtrl = IO(Output(new XDMAHostCtrlIO))
        hostCtrl := ctrl
        fpgaHostReset := ctrl.reset
        fpgaDiffEnable := ctrl.diffEnable
        gateway.fpgaSquashEnable.foreach(_ := ctrl.enableSquash)

        cfg.io.memCtrl.memStatus := 0.U
        from_host_axis.viewAs[AXI4Stream].ready := false.B
        cpu.difftestMemIO.foreach { case DifftestMemIO(cpuRecord, memRecord) =>
          val cpuAxi = Wire(AXI4Bundle.typeOf(cpuRecord))
          AXI4Bundle.connectRecord(cpuAxi, cpuRecord)
          val memCtrl = Module(new DifftestMemCtrl(cpuAxi.cloneType, baseAddr = 0x80000000L))
          memCtrl.io.ctrl <> cfg.io.memCtrl
          memCtrl.io.pcie_clock := pcie_clock
          memCtrl.io.h2c <> from_host_axis.viewAs[AXI4Stream]
          memCtrl.io.cpu <> cpuAxi
          memRecord match {
            case Some(record) =>
              AXI4Bundle.connectRecord(record, memCtrl.io.mem)
            case None =>
              val mem = IO(VerilogAXI4Record.typeOf(cpuAxi))
              mem.viewAs[AXI4Bundle] <> memCtrl.io.mem
          }
        }
      }
    }

    gateway.clockEnable.foreach { clockEnable =>
      val clock_enable = IO(Output(Bool()))
      clock_enable := clockEnable || fpgaHostReset || !fpgaDiffEnable
    }
  }

  cpu.dutReset := (reset.asBool || fpgaHostReset).asTypeOf(cpu.dutReset)
  cpu.connectTopIOs(difftest)
  cpu.dutIOs.foreach { case (name, gen) =>
    val io = IO(chiselTypeOf(gen)).suggestName(name)
    dontTouch(gen)
    io <> gen
  }

  // There should not be anymore IOs
  require(DifftestWiring.isEmpty, s"pending wires left: ${DifftestWiring.getPending}")
}
