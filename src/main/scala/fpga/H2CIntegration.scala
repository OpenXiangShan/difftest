/***************************************************************************************
 * Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

/**
 * H2C Integration Module
 *
 * Integrates Config BAR and H2C Stream-to-AXI4 converter.
 * Provides arbitration between H2C and CPU mem interface.
 * Controls CPU clock gating during H2C transfer.
 *
 * Uses BusMemAXI4 interface which is compatible with bus.axi4.AXI4.
 *
 * @param memDataWidth AXI4 DDR width (aligned with CPU path)
 * @param axisDataWidth AXI4-Stream width from XDMA (default follows memDataWidth)
 */
class H2CIntegration(
  val memDataWidth: Int = 64,
  val axisDataWidth: Int = 0,
) extends Module {
  private val h2cDataWidth = if (axisDataWidth == 0) memDataWidth else axisDataWidth

  val io = IO(new Bundle {
    // ===== AXI4-Lite Config BAR interface =====
    val cfg_aw = Flipped(Decoupled(new AXI4LiteBundleA(32)))
    val cfg_w = Flipped(Decoupled(new AXI4LiteBundleW(32)))
    val cfg_b = Decoupled(new AXI4LiteBundleB)
    val cfg_ar = Flipped(Decoupled(new AXI4LiteBundleA(32)))
    val cfg_r = Decoupled(new AXI4LiteBundleR(32))

    // ===== H2C Stream input (from xdma_axi_h2c) =====
    val h2c_axis = Flipped(new AXI4Stream(h2cDataWidth))

    // ===== CPU AXI4 memory interface (BusMemAXI4, compatible with bus.axi4.AXI4) =====
    val cpu_mem = Flipped(new BusMemAXI4(dataBits = memDataWidth, idBits = 1))

    // ===== DDR AXI4 memory interface (to external DDR) =====
    val ddr_mem = new BusMemAXI4(dataBits = memDataWidth, idBits = 1)

    // ===== Control/Status outputs =====
    val HOST_IO_RESET = Output(Bool())
    val HOST_IO_DIFFTEST_ENABLE = Output(Bool())
    val ddr_arb_sel = Output(Bool()) // 0=CPU, 1=H2C
    val h2c_active = Output(Bool())
    val h2c_done = Output(Bool())
    val h2c_beat_count = Output(UInt(32.W))

    // ===== CPU Clock Enable (for clock gating) =====
    // When H2C owns DDR (ddr_arb_sel=1), CPU clock is disabled
    val cpu_clock_enable = Output(Bool())
  })

  // ===== Instantiate Config BAR =====
  val configBar = Module(new XDMAConfigBar(addrWidth = 32, dataWidth = 32, h2cBeatBytes = memDataWidth / 8))
  configBar.io.axilite.aw <> io.cfg_aw
  configBar.io.axilite.w <> io.cfg_w
  configBar.io.axilite.b <> io.cfg_b
  configBar.io.axilite.ar <> io.cfg_ar
  configBar.io.axilite.r <> io.cfg_r

  // ===== Instantiate H2C Stream-to-AXI4 Converter =====
  val h2cConverter = Module(new H2CAxisToAxi4(addrWidth = 64, axisDataWidth = h2cDataWidth, memDataWidth = memDataWidth))

  // Connect control signals from Config BAR
  h2cConverter.io.enable := configBar.io.ddr_arb_sel
  h2cConverter.io.max_beats := configBar.io.h2c_length

  // Connect H2C stream input
  h2cConverter.io.axis <> io.h2c_axis

  // Connect status back to Config BAR
  configBar.io.h2c_active := h2cConverter.io.active
  configBar.io.h2c_done := h2cConverter.io.done
  configBar.io.h2c_beat_count := h2cConverter.io.beat_count

  // ===== DDR Arbitration (Full Switch) =====
  // Enforce H2C ownership while transfer is active to avoid any CPU/DDR contention window.
  val h2cOwnsDDR = configBar.io.ddr_arb_sel || h2cConverter.io.active

  // Export arbitration/status
  io.HOST_IO_RESET := configBar.io.HOST_IO_RESET
  io.HOST_IO_DIFFTEST_ENABLE := configBar.io.HOST_IO_DIFFTEST_ENABLE
  io.ddr_arb_sel := h2cOwnsDDR
  io.h2c_active := configBar.io.h2c_active
  io.h2c_done := configBar.io.h2c_done
  io.h2c_beat_count := configBar.io.h2c_beat_count

  // ===== CPU Clock Enable Control =====
  // CPU must remain paused until H2C fully releases DDR ownership.
  io.cpu_clock_enable := !h2cOwnsDDR

  // Connect DDR outputs based on arbitration
  io.ddr_mem.aw.valid := Mux(h2cOwnsDDR, h2cConverter.io.mem.aw.valid, io.cpu_mem.aw.valid)
  io.ddr_mem.aw.bits := Mux(h2cOwnsDDR, convertAXI4AW(h2cConverter.io.mem.aw.bits), io.cpu_mem.aw.bits)

  io.ddr_mem.w.valid := Mux(h2cOwnsDDR, h2cConverter.io.mem.w.valid, io.cpu_mem.w.valid)
  io.ddr_mem.w.bits := Mux(h2cOwnsDDR, convertAXI4W(h2cConverter.io.mem.w.bits), io.cpu_mem.w.bits)

  io.ddr_mem.ar.valid := Mux(h2cOwnsDDR, h2cConverter.io.mem.ar.valid, io.cpu_mem.ar.valid)
  io.ddr_mem.ar.bits := Mux(h2cOwnsDDR, convertAXI4AR(h2cConverter.io.mem.ar.bits), io.cpu_mem.ar.bits)

  // CPU side ready signals (stall when H2C owns DDR)
  io.cpu_mem.aw.ready := Mux(h2cOwnsDDR, false.B, io.ddr_mem.aw.ready)
  io.cpu_mem.w.ready := Mux(h2cOwnsDDR, false.B, io.ddr_mem.w.ready)
  io.cpu_mem.ar.ready := Mux(h2cOwnsDDR, false.B, io.ddr_mem.ar.ready)

  // H2C side ready signals (stall when CPU owns DDR)
  h2cConverter.io.mem.aw.ready := Mux(h2cOwnsDDR, io.ddr_mem.aw.ready, false.B)
  h2cConverter.io.mem.w.ready := Mux(h2cOwnsDDR, io.ddr_mem.w.ready, false.B)
  h2cConverter.io.mem.ar.ready := Mux(h2cOwnsDDR, io.ddr_mem.ar.ready, false.B)

  // Response channels (B and R) - route back to appropriate master
  // Since only one master is active at a time, forward responses to the active master
  // When H2C owns DDR: route responses to H2C converter
  // When CPU owns DDR: route responses to CPU
  io.cpu_mem.b.valid := Mux(h2cOwnsDDR, false.B, io.ddr_mem.b.valid)
  io.cpu_mem.b.bits := io.ddr_mem.b.bits

  io.cpu_mem.r.valid := Mux(h2cOwnsDDR, false.B, io.ddr_mem.r.valid)
  io.cpu_mem.r.bits := io.ddr_mem.r.bits

  h2cConverter.io.mem.b.valid := Mux(h2cOwnsDDR, io.ddr_mem.b.valid, false.B)
  h2cConverter.io.mem.b.bits := convertAXI4BToInternal(io.ddr_mem.b.bits)

  h2cConverter.io.mem.r.valid := Mux(h2cOwnsDDR, io.ddr_mem.r.valid, false.B)
  h2cConverter.io.mem.r.bits := convertAXI4RToInternal(io.ddr_mem.r.bits)

  // Ready signals for response channels
  io.ddr_mem.b.ready := Mux(h2cOwnsDDR, h2cConverter.io.mem.b.ready, io.cpu_mem.b.ready)
  io.ddr_mem.r.ready := Mux(h2cOwnsDDR, h2cConverter.io.mem.r.ready, io.cpu_mem.r.ready)

  // ===== Conversion functions: difftest.fpga.AXI4 -> BusMemAXI4 =====
  def convertAXI4AW(in: AXI4AWBundle): BusMemAXI4BundleA = {
    val out = Wire(new BusMemAXI4BundleA(idBits = 1))
    out.addr := in.addr
    out.prot := in.prot
    out.id := in.id
    out.len := in.len
    out.size := in.size
    out.burst := in.burst
    out.lock := in.lock
    out.cache := in.cache
    out.qos := in.qos
    out.user := in.user
    out
  }

  def convertAXI4W(in: AXI4WBundle): BusMemAXI4BundleW = {
    val out = Wire(new BusMemAXI4BundleW(dataBits = memDataWidth))
    out.data := in.data
    out.strb := in.strb
    out.last := in.last
    out
  }

  def convertAXI4AR(in: AXI4ARBundle): BusMemAXI4BundleAR = {
    val out = Wire(new BusMemAXI4BundleAR(idBits = 1))
    out.addr := in.addr
    out.prot := in.prot
    out.id := in.id
    out.len := in.len
    out.size := in.size
    out.burst := in.burst
    out.lock := in.lock
    out.cache := in.cache
    out.qos := in.qos
    out.user := in.user
    out
  }

  def convertAXI4BToInternal(in: BusMemAXI4BundleB): AXI4BBundle = {
    val out = Wire(new AXI4BBundle(idWidth = 1))
    out.id := in.id
    out.resp := in.resp
    out.user := in.user
    out
  }

  def convertAXI4RToInternal(in: BusMemAXI4BundleR): AXI4RBundle = {
    val out = Wire(new AXI4RBundle(dataWidth = memDataWidth, idWidth = 1))
    out.id := in.id
    out.data := in.data
    out.resp := in.resp
    out.last := in.last
    out.user := in.user
    out
  }
}
