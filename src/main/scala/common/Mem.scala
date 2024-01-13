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

trait HasMemInit { this: ExtModule =>
  val mem_init =
    """
      |`ifdef SYNTHESIS
      |  // 1536MB memory
      |  `define RAM_SIZE (1536 * 1024 * 1024)
      |
      |  // memory array
      |  reg [63:0] memory [0 : `RAM_SIZE/8 - 1];
      |
      |  string bin_file;
      |  integer memory_image = 0, n_read = 0, byte_read = 1;
      |  byte data;
      |  initial begin
      |    if ($test$plusargs("workload")) begin
      |      $value$plusargs("workload=%s", bin_file);
      |      memory_image = $fopen(bin_file, "rb");
      |    if (memory_image == 0) begin
      |      $display("Error: failed to open %s", bin_file);
      |      $finish;
      |    end
      |    foreach (memory[i]) begin
      |      if (byte_read == 0) break;
      |      for (integer j = 0; j < 8; j++) begin
      |        byte_read = $fread(data, memory_image);
      |        if (byte_read == 0) break;
      |        n_read += 1;
      |        memory[i][j * 8 +: 8] = data;
      |      end
      |    end
      |    $fclose(memory_image);
      |    $display("%m: load %d bytes from %s.", n_read, bin_file);
      |  end
      |end
      |`endif // SYNTHESIS
      |""".stripMargin
}

trait HasReadPort { this: ExtModule =>
  val r = IO(new Bundle {
    val enable = Input(Bool())
    val index  = Input(UInt(64.W))
    val data   = Output(UInt(64.W))
  })

  val r_dpic =
    """
      |`ifndef SYNTHESIS
      |import "DPI-C" function longint difftest_ram_read(input longint rIdx);
      |`endif // SYNTHESIS
      |""".stripMargin

  val r_if =
    """
      |input             r_enable,
      |input      [63:0] r_index,
      |output reg [63:0] r_data,
      |""".stripMargin

  val r_func =
    """
      |`ifndef SYNTHESIS
      |if (r_enable) begin
      |  r_data <= difftest_ram_read(r_index);
      |end
      |`else
      |if (r_enable) begin
      |  r_data <= memory[r_index];
      |end
      |`endif // SYNTHESIS
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
      |`ifndef SYNTHESIS
      |import "DPI-C" function void difftest_ram_write
      |(
      |  input  longint index,
      |  input  longint data,
      |  input  longint mask
      |);
      |`endif // SYNTHESIS
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
      |`ifndef SYNTHESIS
      |if (w_enable) begin
      |  difftest_ram_write(w_index, w_data, w_mask);
      |end
      |`else
      |if (w_enable) begin
      |  memory[w_index] <= (w_data & w_mask) | (memory[w_index] & ~w_mask);
      |end
      |`endif // SYNTHESIS
      |""".stripMargin

  def write(enable: Bool, index: UInt, data: UInt, mask: UInt): HasWritePort = {
    w.enable := enable
    w.index  := index
    w.data   := data
    w.mask   := mask
    this
  }
}

class MemRHelper extends ExtModule with HasExtModuleInline with HasReadPort with HasMemInit {
  val clock  = IO(Input(Clock()))

  setInline("MemRHelper.v",
    s"""
       |$r_dpic
       |module MemRHelper(
       |  $r_if
       |  input clock
       |);
       |  $mem_init
       |  always @(posedge clock) begin
       |    $r_func
       |  end
       |endmodule
     """.stripMargin)
}

class MemWHelper extends ExtModule with HasExtModuleInline with HasWritePort with HasMemInit {
  val clock  = IO(Input(Clock()))

  setInline("MemWHelper.v",
    s"""
       |$w_dpic
       |module MemWHelper(
       |  $w_if
       |  input clock
       |);
       |  $mem_init
       |  always @(posedge clock) begin
       |   $w_func
       |  end
       |endmodule
     """.stripMargin)
}

class MemRWHelper extends ExtModule with HasExtModuleInline with HasReadPort with HasWritePort with HasMemInit {
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
       |  $mem_init
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

class DifftestMemInitializer extends ExtModule with HasExtModuleInline {
  setInline("DifftestMemInitializer.v",
    """
       |module DifftestMemInitializer();
       |`ifndef SYNTHESIS
       |`define TARGET SynthesizableDifftestMem.mem
       |string bin_file;
       |integer memory_image = 0, n_read = 0, byte_read = 1;
       |byte data;
       |initial begin
       |  if ($test$plusargs("workload")) begin
       |    $value$plusargs("workload=%s", bin_file);
       |    memory_image = $fopen(bin_file, "rb");
       |    if (memory_image == 0) begin
       |      $display("Error: failed to open %s", bin_file);
       |      $finish;
       |    end
       |    foreach (`TARGET[i]) begin
       |      if (byte_read == 0) break;
       |      for (integer j = 0; j < 8; j++) begin
       |        byte_read = $fread(data, memory_image);
       |        if (byte_read == 0) break;
       |        n_read += 1;
       |        `TARGET[i][j * 8 +: 8] = data;
       |      end
       |    end
       |    $fclose(memory_image);
       |    $display("%m: load %d bytes from %s.", n_read, bin_file);
       |  end
       |end
       |`endif
       |endmodule
       |""".stripMargin)
}

class SynthesizableDifftestMem(size: BigInt, lanes: Int, bits: Int) extends DifftestMem(size, lanes, bits) {
  val mem = Mem(size / 8, UInt(64.W))

  for (i <- 0 until n_helper) {
    val r_index = read.index * n_helper.U + i.U
    read.data(i) := RegEnable(mem(r_index), read.valid)

    val w_index = write.index * n_helper.U + i.U
    when (write.valid) {
      mem(w_index) := (write.data(i) & write.mask(i)) | (mem(w_index) & (~write.mask(i)).asUInt)
    }
  }

  val initializer = Module(new DifftestMemInitializer)
}

object DifftestMem {
  def apply(size: BigInt, beatBytes: Int): DifftestMem = {
    apply(size, beatBytes, 8)
  }

  def apply(size: BigInt, beatBytes: Int, synthesizable: Boolean): DifftestMem = {
    apply(size, beatBytes, 8, synthesizable = synthesizable)
  }

  def apply(
    size: BigInt,
    lanes: Int,
    bits: Int,
    synthesizable: Boolean = false,
    singlePort: Boolean = true,
  ): DifftestMem = {
    val mod = (synthesizable, singlePort) match {
      case (true, _) => Module(new SynthesizableDifftestMem(size, lanes, bits))
      case (false, true) => Module(new DifftestMem1P(size, lanes, bits))
      case (false, false) => Module(new DifftestMem2P(size, lanes, bits))
    }
    mod.read        := DontCare
    mod.read.valid  := false.B
    mod.write       := DontCare
    mod.write.valid := false.B
    mod
  }
}
