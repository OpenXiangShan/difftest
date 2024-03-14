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

private trait HasMemInitializer { this: ExtModule =>
  def mem_decl: String
  def mem_target: String

  private val initializer =
    """
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
      |    foreach (`MEM_TARGET[i]) begin
      |      if (byte_read == 0) break;
      |      for (integer j = 0; j < 8; j++) begin
      |        byte_read = $fread(data, memory_image);
      |        if (byte_read == 0) break;
      |        n_read += 1;
      |        `MEM_TARGET[i][j * 8 +: 8] = data;
      |      end
      |    end
      |    $fclose(memory_image);
      |    $display("%m: load %d bytes from %s.", n_read, bin_file);
      |  end
      |end
      |""".stripMargin
  val mem_init =
    s"""
       |`ifdef DISABLE_DIFFTEST_RAM_DPIC
       |`ifdef PALLADIUM
       |  initial $$ixc_ctrl("tb_import", "$$display");
       |`endif // PALLADIUM
       |$mem_decl
       |`define MEM_TARGET $mem_target
       |$initializer
       |`endif // DISABLE_DIFFTEST_RAM_DPIC
       |""".stripMargin
}

private trait HasReadPort { this: ExtModule =>
  val r = IO(new Bundle {
    val enable = Input(Bool())
    val index = Input(UInt(64.W))
    val data = Output(UInt(64.W))
  })

  val r_dpic =
    """
      |`ifndef DISABLE_DIFFTEST_RAM_DPIC
      |import "DPI-C" function longint difftest_ram_read(input longint rIdx);
      |`endif // DISABLE_DIFFTEST_RAM_DPIC
      |""".stripMargin

  val r_if =
    """
      |input             r_enable,
      |input      [63:0] r_index,
      |output reg [63:0] r_data,
      |""".stripMargin

  val r_func =
    """
      |`ifndef DISABLE_DIFFTEST_RAM_DPIC
      |if (r_enable) begin
      |  r_data <= difftest_ram_read(r_index);
      |end
      |`else
      |if (r_enable) begin
      |  r_data <= `MEM_TARGET[r_index];
      |end
      |`endif // DISABLE_DIFFTEST_RAM_DPIC
      |""".stripMargin

  def read(enable: Bool, index: UInt): UInt = {
    r.enable := enable
    r.index := index
    r.data
  }
}

private trait HasWritePort { this: ExtModule =>
  val w = IO(new Bundle {
    val enable = Input(Bool())
    val index = Input(UInt(64.W))
    val data = Input(UInt(64.W))
    val mask = Input(UInt(64.W))
  })

  val w_dpic =
    """
      |`ifndef DISABLE_DIFFTEST_RAM_DPIC
      |import "DPI-C" function void difftest_ram_write
      |(
      |  input  longint index,
      |  input  longint data,
      |  input  longint mask
      |);
      |`endif // DISABLE_DIFFTEST_RAM_DPIC
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
      |`ifndef DISABLE_DIFFTEST_RAM_DPIC
      |if (w_enable) begin
      |  difftest_ram_write(w_index, w_data, w_mask);
      |end
      |`else
      |if (w_enable) begin
      |  `MEM_TARGET[w_index] <= (w_data & w_mask) | (`MEM_TARGET[w_index] & ~w_mask);
      |end
      |`endif // DISABLE_DIFFTEST_RAM_DPIC
      |""".stripMargin

  def write(enable: Bool, index: UInt, data: UInt, mask: UInt): HasWritePort = {
    w.enable := enable
    w.index := index
    w.data := data
    w.mask := mask
    this
  }
}

abstract private class MemHelper extends ExtModule with HasExtModuleInline with HasMemInitializer

private class MemRWHelper extends MemHelper with HasReadPort with HasWritePort {
  val clock = IO(Input(Clock()))

  def mem_decl: String =
    """
      |// 1536MB memory
      |`define RAM_SIZE (1536 * 1024 * 1024)
      |reg [63:0] memory [0 : `RAM_SIZE / 8 - 1];
      |""".stripMargin

  def mem_target: String = "memory"

  setInline(
    "MemRWHelper.v",
    s"""
       |`ifdef SYNTHESIS
       |  `define DISABLE_DIFFTEST_RAM_DPIC
       |`endif
       |$r_dpic
       |$w_dpic
       |module MemRWHelper(
       |  $r_if
       |  $w_if
       |  input clock
       |);
       |  $mem_init
       |  always @(posedge clock) begin
       |    $r_func
       |    $w_func
       |  end
       |endmodule
     """.stripMargin,
  )
}

abstract class DifftestMem(size: BigInt, lanes: Int, bits: Int) extends Module {
  require(bits == 8 && lanes % 8 == 0, "supports 64-bits aligned byte access only")
  protected val n_helper = lanes / 8
  private val helper = Seq.fill(n_helper)(Module(new MemRWHelper))

  val read = IO(new Bundle {
    val valid = Input(Bool())
    val index = Input(UInt(64.W))
    val data = Output(Vec(n_helper, UInt(64.W)))
  })
  val write = IO(Input(new Bundle {
    val valid = Bool()
    val index = UInt(64.W)
    val data = Vec(n_helper, UInt(64.W))
    val mask = Vec(n_helper, UInt(64.W))
  }))

  read.data := helper.zipWithIndex.map { case (h, i) =>
    h.clock := clock
    h.read(
      enable = !reset.asBool && read.valid,
      index = read.index * n_helper.U + i.U,
    )
  }

  helper.zipWithIndex.foreach { case (h, i) =>
    h.clock := clock
    h.write(
      enable = !reset.asBool && write.valid,
      index = write.index * n_helper.U + i.U,
      data = write.data(i),
      mask = write.mask(i),
    )
  }

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
    write.data := VecInit(data).asTypeOf(write.data)
    require(data.length == lanes, s"data Vec[UInt] should have the length of $lanes")
    require(mask.length == lanes, s"mask Vec[Bool] should have the length of $lanes")
    require(data.head.getWidth == bits, s"data should have the width of $bits")
    write.mask := VecInit(mask.map(m => Fill(bits, m))).asTypeOf(write.mask)
  }
}

private class DifftestMemReadOnly(size: BigInt, lanes: Int, bits: Int) extends DifftestMem(size, lanes, bits) {
  assert(!write.valid, "no write allowed in read-only mem")
}

private class DifftestMemWriteOnly(size: BigInt, lanes: Int, bits: Int) extends DifftestMem(size, lanes, bits) {
  assert(!read.valid, "no read allowed in write-only mem")
}

private class DifftestMem2P(size: BigInt, lanes: Int, bits: Int) extends DifftestMem(size, lanes, bits)

private class DifftestMem1P(size: BigInt, lanes: Int, bits: Int) extends DifftestMem2P(size, lanes, bits) {
  assert(!read.valid || !write.valid, "read and write come at the same cycle")
}

private class SynthesizableDifftestMem(size: BigInt, lanes: Int, bits: Int) extends DifftestMem(size, lanes, bits) {
  val mem = Mem(size / 8, UInt(64.W))

  for (i <- 0 until n_helper) {
    val r_index = read.index * n_helper.U + i.U
    read.data(i) := RegEnable(mem(r_index), read.valid)

    val w_index = write.index * n_helper.U + i.U
    when(write.valid) {
      mem(w_index) := (write.data(i) & write.mask(i)) | (mem(w_index) & (~write.mask(i)).asUInt)
    }
  }

  Module(new MemInitializer(s"$desiredName.mem"))
}

private class MemInitializer(mem: String) extends MemHelper {
  override def mem_decl: String = ""
  override def mem_target: String = mem

  setInline(
    s"$desiredName.v",
    s"""
       |module $desiredName();
       |$mem_init
       |endmodule
      """.stripMargin,
  )
}

object DifftestMem {
  private def setDefaultIOs(mod: DifftestMem): DifftestMem = {
    mod.read := DontCare
    mod.read.valid := false.B
    mod.write := DontCare
    mod.write.valid := false.B
    mod
  }

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
    setDefaultIOs((synthesizable, singlePort) match {
      case (true, _)      => Module(new SynthesizableDifftestMem(size, lanes, bits))
      case (false, true)  => Module(new DifftestMem1P(size, lanes, bits))
      case (false, false) => Module(new DifftestMem2P(size, lanes, bits))
    })
  }

  def readOnly(size: BigInt, beatBytes: Int): DifftestMem = {
    setDefaultIOs(Module(new DifftestMemReadOnly(size, beatBytes, 8)))
  }

  def writeOnly(size: BigInt, beatBytes: Int): DifftestMem = {
    setDefaultIOs(Module(new DifftestMemWriteOnly(size, beatBytes, 8)))
  }
}
