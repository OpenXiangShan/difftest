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

package difftest.fpga
import chisel3._
import chisel3.util._

// ===== AXI4 Stream (用于 C2H 和 H2C 数据流) =====
class AXI4StreamBundle(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val last = Bool()
}

class AXI4Stream(val dataWidth: Int) extends DecoupledIO(new AXI4StreamBundle(dataWidth))

// ===== AXI4-Lite Bundles (用于 Config BAR) =====
// Using undirected UInt, directions determined by Decoupled/Flipped context
class AXI4LiteBundleA(val addrWidth: Int) extends Bundle {
  val addr = UInt(addrWidth.W)
  val prot = UInt(3.W)
}

class AXI4LiteBundleW(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val strb = UInt((dataWidth / 8).W)
}

class AXI4LiteBundleB extends Bundle {
  val resp = UInt(2.W)
}

class AXI4LiteBundleR(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val resp = UInt(2.W)
}

// ===== AXI4-Lite Slave Interface =====
class AXI4LiteSlaveIO(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val aw = Flipped(Decoupled(new AXI4LiteBundleA(addrWidth)))
  val w = Flipped(Decoupled(new AXI4LiteBundleW(dataWidth)))
  val b = Decoupled(new AXI4LiteBundleB)
  val ar = Flipped(Decoupled(new AXI4LiteBundleA(addrWidth)))
  val r = Decoupled(new AXI4LiteBundleR(dataWidth))
}

// ===== Full AXI4 Bundles (用于 H2C 内存写入) =====
class AXI4AWBundle(val addrWidth: Int, val idWidth: Int) extends Bundle {
  val addr = Output(UInt(addrWidth.W))
  val id = Output(UInt(idWidth.W))
  val len = Output(UInt(8.W))
  val size = Output(UInt(3.W))
  val burst = Output(UInt(2.W))
  val lock = Output(UInt(1.W))
  val cache = Output(UInt(4.W))
  val prot = Output(UInt(3.W))
  val qos = Output(UInt(4.W))
  val user = Output(UInt(1.W))
}

class AXI4WBundle(val dataWidth: Int) extends Bundle {
  val data = Output(UInt(dataWidth.W))
  val strb = Output(UInt((dataWidth / 8).W))
  val last = Output(Bool())
}

class AXI4BBundle(val idWidth: Int) extends Bundle {
  val id = Output(UInt(idWidth.W))
  val resp = Output(UInt(2.W))
  val user = Output(UInt(1.W))
}

class AXI4ARBundle(val addrWidth: Int, val idWidth: Int) extends Bundle {
  val addr = Output(UInt(addrWidth.W))
  val id = Output(UInt(idWidth.W))
  val len = Output(UInt(8.W))
  val size = Output(UInt(3.W))
  val burst = Output(UInt(2.W))
  val lock = Output(UInt(1.W))
  val cache = Output(UInt(4.W))
  val prot = Output(UInt(3.W))
  val qos = Output(UInt(4.W))
  val user = Output(UInt(1.W))
}

class AXI4RBundle(val dataWidth: Int, val idWidth: Int) extends Bundle {
  val id = Output(UInt(idWidth.W))
  val data = Output(UInt(dataWidth.W))
  val resp = Output(UInt(2.W))
  val last = Output(Bool())
  val user = Output(UInt(1.W))
}

// Full AXI4 Master Interface (for H2C to write to DDR)
class AXI4(val addrWidth: Int, val dataWidth: Int, val idWidth: Int) extends Bundle {
  val aw = Decoupled(new AXI4AWBundle(addrWidth, idWidth))
  val w = Decoupled(new AXI4WBundle(dataWidth))
  val b = Flipped(Decoupled(new AXI4BBundle(idWidth)))
  val ar = Decoupled(new AXI4ARBundle(addrWidth, idWidth))
  val r = Flipped(Decoupled(new AXI4RBundle(dataWidth, idWidth)))
}

// ===== BusMemAXI4 (兼容 bus.axi4.AXI4 接口) =====
// 这是 bus.axi4.AXI4 的复制版本，用于 difftest.fpga 包
// 使 H2C 模块能够与 CPU 内存接口兼容

class BusMemAXI4BundleA(val idBits: Int) extends Bundle {
  val addr = Output(UInt(64.W))
  val prot = Output(UInt(3.W))
  val id = Output(UInt(idBits.W))
  val len = Output(UInt(8.W))
  val size = Output(UInt(3.W))
  val burst = Output(UInt(2.W))
  val lock = Output(UInt(1.W))
  val cache = Output(UInt(4.W))
  val qos = Output(UInt(4.W))
  val user = Output(UInt(1.W))
}

class BusMemAXI4BundleW(val dataBits: Int) extends Bundle {
  val data = Output(UInt(dataBits.W))
  val strb = Output(UInt((dataBits / 8).W))
  val last = Output(Bool())
}

class BusMemAXI4BundleB(val idBits: Int) extends Bundle {
  val id = Output(UInt(idBits.W))
  val resp = Output(UInt(2.W))
  val user = Output(UInt(1.W))
}

class BusMemAXI4BundleAR(val idBits: Int) extends Bundle {
  val addr = Output(UInt(64.W))
  val prot = Output(UInt(3.W))
  val id = Output(UInt(idBits.W))
  val len = Output(UInt(8.W))
  val size = Output(UInt(3.W))
  val burst = Output(UInt(2.W))
  val lock = Output(UInt(1.W))
  val cache = Output(UInt(4.W))
  val qos = Output(UInt(4.W))
  val user = Output(UInt(1.W))
}

class BusMemAXI4BundleR(val dataBits: Int, val idBits: Int) extends Bundle {
  val id = Output(UInt(idBits.W))
  val data = Output(UInt(dataBits.W))
  val resp = Output(UInt(2.W))
  val last = Output(Bool())
  val user = Output(UInt(1.W))
}

// BusMemAXI4 主接口（兼容 bus.axi4.AXI4）
class BusMemAXI4(val dataBits: Int, val idBits: Int) extends Bundle {
  val aw = Decoupled(new BusMemAXI4BundleA(idBits))
  val w = Decoupled(new BusMemAXI4BundleW(dataBits))
  val b = Flipped(Decoupled(new BusMemAXI4BundleB(idBits)))
  val ar = Decoupled(new BusMemAXI4BundleAR(idBits))
  val r = Flipped(Decoupled(new BusMemAXI4BundleR(dataBits, idBits)))
}
