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
import chisel3.experimental.ExtModule
import chisel3.util._

trait HasReadPort { this: ExtModule =>
  val r = IO(new Bundle {
    val enable = Input(Bool())
    val index  = Input(UInt(64.W))
    val data   = Output(UInt(64.W))
  })

  val r_dpic =
    """
      |import "DPI-C" function longint difftest_ram_read(input longint rIdx);
      |""".stripMargin

  val r_if =
    """
      |input             r_enable,
      |input      [63:0] r_index,
      |output reg [63:0] r_data,
      |""".stripMargin

  val r_func =
    """
      |if (r_enable) begin
      |  r_data <= difftest_ram_read(r_index);
      |end
      |""".stripMargin

  def read(enable: Bool, index: UInt): UInt = {
    r.enable := enable
    r.index  := index
    r.data
  }
}


trait HasWritePort { this: ExtModule =>
  val w = IO(new Bundle {
    val enable = Input(Bool())
    val index  = Input(UInt(64.W))
    val data   = Input(UInt(64.W))
    val mask   = Input(UInt(64.W))
  })

  val w_dpic =
    """
      |import "DPI-C" function void difftest_ram_write
      |(
      |  input  longint index,
      |  input  longint data,
      |  input  longint mask
      |);
      |""".stripMargin

  val w_if =
    """
      |input         w_enable,
      |input  [63:0] w_index,
      |input  [63:0] w_data,
      |input  [63:0] w_mask,
      |""".stripMargin

  val w_func =
    """
      |if (w_enable) begin
      |  difftest_ram_write(w_index, w_data, w_mask);
      |end
      |""".stripMargin

  def write(enable: Bool, index: UInt, data: UInt, mask: UInt): HasWritePort = {
    w.enable := enable
    w.index  := index
    w.data   := data
    w.mask   := mask
    this
  }
}

class MemRHelper extends ExtModule with HasExtModuleInline with HasReadPort {
  val clock  = IO(Input(Clock()))

  setInline("MemRHelper.v",
    s"""
       |$r_dpic
       |module MemRHelper(
       |  $r_if
       |  input clock
       |);
       |  always @(posedge clock) begin
       |    $r_func
       |  end
       |endmodule
     """.stripMargin)
}

class MemWHelper extends ExtModule with HasExtModuleInline with HasWritePort {
  val clock  = IO(Input(Clock()))

  setInline("MemWHelper.v",
    s"""
       |$w_dpic
       |module MemWHelper(
       |  $w_if
       |  input clock
       |);
       |  always @(posedge clock) begin
       |   $w_func
       |  end
       |endmodule
     """.stripMargin)
}

class MemRWHelper extends ExtModule with HasExtModuleInline with HasReadPort with HasWritePort {
  val clock  = IO(Input(Clock()))
  val enable = IO(Input(Bool()))

  setInline("MemRWHelper.v",
    s"""
       |$r_dpic
       |$w_dpic
       |module MemRWHelper(
       |  $r_if
       |  $w_if
       |  input enable,
       |  input clock
       |);
       |  always @(posedge clock) begin
       |    if (enable) begin
       |      $r_func
       |      $w_func
       |    end
       |  end
       |endmodule
     """.stripMargin)
}

abstract class DifftestMem(size: BigInt, lanes: Int, bits: Int) extends Module {
  require(bits == 8 && lanes % 8 == 0, "supports 64-bits aligned byte access only")
  val n_helper = lanes / 8

  val read = IO(new Bundle {
    val valid = Input(Bool())
    val index = Input(UInt(64.W))
    val data  = Output(Vec(n_helper, UInt(64.W)))
  })
  val write = IO(Input(new Bundle {
    val valid = Bool()
    val index = UInt(64.W)
    val data  = Vec(n_helper, UInt(64.W))
    val mask  = Vec(n_helper, UInt(64.W))
  }))

  def read(addr: UInt): Vec[UInt] = {
    read.valid := !write.valid
    read.index := addr
    read.data
  }

  def readAndHold(addr: UInt, en: Bool): Vec[UInt] = {
    read.valid := en
    read.index := addr
    Mux(RegNext(en), read.data, RegEnable(read.data, RegNext(en))).asTypeOf(Vec(lanes, UInt(bits.W)))
  }

  def write(addr: UInt, data: Seq[UInt], mask: Seq[Bool]): Unit = {
    write.valid := true.B
    write.index := addr
    write.data  := VecInit(data).asTypeOf(write.data)
    require(data.length == lanes, s"data Vec[UInt] should have the length of $lanes")
    require(mask.length == lanes, s"mask Vec[Bool] should have the length of $lanes")
    require(data.head.getWidth == bits, s"data should have the width of $bits")
    write.mask  := VecInit(mask.map(m => Fill(bits, m))).asTypeOf(write.mask)
  }
}

class DifftestMem1P(size: BigInt, lanes: Int, bits: Int) extends DifftestMem(size, lanes, bits) {
  assert(!read.valid || !write.valid, "read and write come at the same cycle")

  val helper = Seq.fill(n_helper)(Module(new MemRWHelper))
  read.data := helper.zipWithIndex.map{ case (h, i) =>
    h.clock := clock
    h.enable := !reset.asBool
    h.write(
      enable = write.valid,
      index  = write.index * n_helper.U + i.U,
      data   = write.data(i),
      mask   = write.mask(i)
    )
    h.read(
      enable = read.valid,
      index  = read.index * n_helper.U + i.U
    )
  }
}

class DifftestMem2P(size: BigInt, lanes: Int, bits: Int) extends DifftestMem(size, lanes, bits) {
  val r_helper = Seq.fill(n_helper)(Module(new MemRHelper))
  read.data := r_helper.zipWithIndex.map{ case (h, i) =>
    h.clock  := clock
    h.read(
      enable = !reset.asBool && read.valid,
      index  = read.index * n_helper.U + i.U
    )
  }

  val w_helper = Seq.fill(n_helper)(Module(new MemWHelper))
  w_helper.zipWithIndex.foreach { case (h, i) =>
    h.clock  := clock
    h.write(
      enable = !reset.asBool && write.valid,
      index  = write.index * n_helper.U + i.U,
      data   = write.data(i),
      mask   = write.mask(i)
    )
  }
}

object DifftestMem {
  def apply(size: BigInt, beatBytes: Int): DifftestMem = {
    apply(size, beatBytes, 8)
  }

  def apply(size: BigInt, lanes: Int, bits: Int, singlePort: Boolean = true): DifftestMem = {
    val mod = if (singlePort) {
      Module(new DifftestMem1P(size, lanes, bits))
    } else {
      Module(new DifftestMem2P(size, lanes, bits))
    }
    mod.read        := DontCare
    mod.read.valid  := false.B
    mod.write       := DontCare
    mod.write.valid := false.B
    mod
  }
}
