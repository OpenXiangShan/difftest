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

private trait HasMemReadHelper { this: ExtModule =>
  private def r(i: Int): String = s"r_$i"

  def r_dpic: String =
    """
      |`ifndef DISABLE_DIFFTEST_RAM_DPIC
      |import "DPI-C" function longint difftest_ram_read(input longint rIdx);
      |`endif // DISABLE_DIFFTEST_RAM_DPIC
      |""".stripMargin

  private def sv_interface(i: Int): String =
    s"""
       |input             ${r(i)}_enable,
       |input      [63:0] ${r(i)}_index,
       |output reg [63:0] ${r(i)}_data,
       |output            ${r(i)}_async
       |""".stripMargin
  def r_sv_interface(n: Int): String = (0 until n).map(sv_interface).mkString(",\n")

  private def sv_body(i: Int): String =
    s"""
       |`ifdef GSIM
       |  assign ${r(i)}_async = 1'b1;
       |always @(*) begin
       |  ${r(i)}_data = 0;
       |`ifndef DISABLE_DIFFTEST_RAM_DPIC
       |  if (${r(i)}_enable) begin
       |    ${r(i)}_data = difftest_ram_read(${r(i)}_index);
       |  end
       |`else
       |  if (${r(i)}_enable) begin
       |    ${r(i)}_data = `MEM_TARGET[${r(i)}_index];
       |  end
       |`endif // DISABLE_DIFFTEST_RAM_DPIC
       |end
       |`else // GSIM
       |  assign ${r(i)}_async = 1'b0;
       |always @(posedge clock) begin
       |`ifndef DISABLE_DIFFTEST_RAM_DPIC
       |  if (${r(i)}_enable) begin
       |    ${r(i)}_data <= difftest_ram_read(${r(i)}_index);
       |  end
       |`else
       |  if (${r(i)}_enable) begin
       |    ${r(i)}_data <= `MEM_TARGET[${r(i)}_index];
       |  end
       |`endif // DISABLE_DIFFTEST_RAM_DPIC
       |end
       |`endif // GSIM
       |""".stripMargin
  def r_sv_body(n: Int): String = (0 until n).map(sv_body).mkString

  private def cpp_arg(i: Int): String =
    s"""
       |uint8_t   ${r(i)}_enable,
       |uint64_t  ${r(i)}_index,
       |uint64_t& ${r(i)}_data,
       |uint8_t&  ${r(i)}_async
       |""".stripMargin
  def r_cpp_arg(n: Int): String = (0 until n).map(cpp_arg).mkString(",\n")

  private def cpp_body(i: Int): String =
    s"""
       |  ${r(i)}_async = 1;
       |  if (${r(i)}_enable) ${r(i)}_data = difftest_ram_read(${r(i)}_index);
       |""".stripMargin
  def r_cpp_body(n: Int): String = (0 until n).map(cpp_body).mkString
}

private trait HasMemWriteHelper {
  private def w(i: Int): String = s"w_$i"

  def w_dpic: String =
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

  private def sv_interface(i: Int): String =
    s"""
       |input         ${w(i)}_enable,
       |input  [63:0] ${w(i)}_index,
       |input  [63:0] ${w(i)}_data,
       |input  [63:0] ${w(i)}_mask
       |""".stripMargin
  def w_sv_interface(n: Int): String = (0 until n).map(sv_interface).mkString(",\n")

  private def sv_body(i: Int): String =
    s"""
       |`ifndef DISABLE_DIFFTEST_RAM_DPIC
       |if (${w(i)}_enable) begin
       |  difftest_ram_write(${w(i)}_index, ${w(i)}_data, ${w(i)}_mask);
       |end
       |`else
       |if (${w(i)}_enable) begin
       |  `MEM_TARGET[${w(i)}_index] <= (${w(i)}_data & ${w(i)}_mask) | (`MEM_TARGET[${w(i)}_index] & ~${w(i)}_mask);
       |end
       |`endif // DISABLE_DIFFTEST_RAM_DPIC
       |""".stripMargin
  def w_sv_body(n: Int): String =
    s"""
       |always @(posedge clock) begin
       |${(0 until n).map(sv_body).mkString}
       |end
       |""".stripMargin

  private def cpp_arg(i: Int): String =
    s"""
       |uint8_t  ${w(i)}_enable,
       |uint64_t ${w(i)}_index,
       |uint64_t ${w(i)}_data,
       |uint64_t ${w(i)}_mask""".stripMargin
  def w_cpp_arg(n: Int): String = (0 until n).map(cpp_arg).mkString(",\n")

  private def cpp_body(i: Int): String =
    s"""
       |  if (${w(i)}_enable) {
       |    difftest_ram_write(${w(i)}_index, ${w(i)}_data, ${w(i)}_mask);
       |  }
       |""".stripMargin
  def w_cpp_body(n: Int): String = (0 until n).map(cpp_body).mkString
}

private class CppMemReadIO extends Bundle {
  val enable = Input(Bool())
  val index = Input(UInt(64.W))
  val data = Output(UInt(64.W))
  val async = Output(Bool())
}

private class CppMemWriteIO extends Bundle {
  val enable = Input(Bool())
  val index = Input(UInt(64.W))
  val data = Input(UInt(64.W))
  val mask = Input(UInt(64.W))
}

private class MemRWHelper(size: BigInt, val nr: Int, val nw: Int)
  extends ExtModule(Map("RAM_SIZE" -> size))
  with HasExtModuleInline
  with HasMemInitializer
  with HasMemReadHelper
  with HasMemWriteHelper {

  val r = IO(Vec(nr, new CppMemReadIO))
  val w = IO(Vec(nw, new CppMemWriteIO))
  val clock = IO(Input(Clock()))

  def read(i: Int, enable: Bool, index: UInt): UInt = {
    r(i).enable := enable
    r(i).index := index
    Mux(r(i).async, RegEnable(r(i).data, r(i).enable), r(i).data)
  }
  def write(i: Int, enable: Bool, index: UInt, data: UInt, mask: UInt): Unit = {
    w(i).enable := enable
    w(i).index := index
    w(i).data := data
    w(i).mask := mask
  }

  def mem_decl: String =
    """
      |reg [63:0] memory [0 : RAM_SIZE / 8 - 1];
      |""".stripMargin

  def mem_target: String = "memory"

  override def desiredName: String = s"Mem${nr}R${nw}WHelper"

  val cppExtModule =
    s"""
       |void $desiredName(
       |int ram_size,
       |${r_cpp_arg(nr)},
       |${w_cpp_arg(nw)}
       |) {
       |  ${r_cpp_body(nr)}
       |  ${w_cpp_body(nw)}
       |}
       |""".stripMargin
  difftest.DifftestModule.createCppExtModule(desiredName, cppExtModule, Some("\"ram.h\""))

  setInline(
    s"$desiredName.v",
    s"""
       |`ifdef SYNTHESIS
       |  `define DISABLE_DIFFTEST_RAM_DPIC
       |`endif
       |module $desiredName #(
       |  parameter RAM_SIZE
       |)(
       |  input clock,
       |  ${r_sv_interface(nr)},
       |  ${w_sv_interface(nw)}
       |);
       |  $mem_init
       |  $r_dpic
       |  $w_dpic
       |  ${r_sv_body(nr)}
       |  ${w_sv_body(nw)}
       |endmodule
     """.stripMargin,
  )
}

class DifftestMemReadIO(nWord: Int) extends Bundle {
  val valid = Input(Bool())
  val index = Input(UInt(64.W))
  val data = Output(Vec(nWord, UInt(64.W)))
}

class DifftestMemWriteIO(nWord: Int) extends Bundle {
  val valid = Input(Bool())
  val index = Input(UInt(64.W))
  val data = Input(Vec(nWord, UInt(64.W)))
  val mask = Input(Vec(nWord, UInt(64.W)))
}

class DifftestMem(size: BigInt, lanes: Int, bits: Int, nr: Int, nw: Int) extends Module {
  override def desiredName: String = s"DifftestMem${nr}R${nw}W"

  require(bits == 8 && lanes % 8 == 0, "supports 64-bits aligned byte access only")
  private val n_helper = lanes / 8

  val read = IO(Vec(nr, new DifftestMemReadIO(n_helper)))
  val write = IO(Vec(nw, new DifftestMemWriteIO(n_helper)))

  private val helper = Seq.fill(n_helper)(Module(new MemRWHelper(size, nr, nw)))
  read.zipWithIndex.foreach { case (r, i) =>
    r.data := helper.zipWithIndex.map { case (h, j) =>
      h.clock := clock
      h.read(i, enable = !reset.asBool && r.valid, index = r.index * n_helper.U + j.U)
    }
  }
  write.zipWithIndex.foreach { case (w, i) =>
    helper.zipWithIndex.foreach { case (h, j) =>
      h.clock := clock
      h.write(
        i,
        enable = !reset.asBool && w.valid,
        index = w.index * n_helper.U + j.U,
        data = w.data(j),
        mask = w.mask(j),
      )
    }
  }

  private var r_index = 0
  def read(addr: UInt, en: Bool): Vec[UInt] = {
    val port = read(r_index)
    r_index += 1
    port.valid := en
    port.index := addr
    port.data
  }

  def readAndHold(addr: UInt, en: Bool): Vec[UInt] = {
    val port = read(r_index)
    r_index += 1
    port.valid := en
    port.index := addr
    Mux(RegNext(en), port.data, RegEnable(port.data, RegNext(en))).asTypeOf(Vec(lanes, UInt(bits.W)))
  }

  private var w_index = 0
  def write(addr: UInt, data: Seq[UInt], mask: Seq[Bool]): Unit = {
    val port = write(w_index)
    w_index += 1
    port.valid := true.B
    port.index := addr
    port.data := VecInit(data).asTypeOf(port.data)
    require(data.length == lanes, s"data Vec[UInt] should have the length of $lanes")
    require(mask.length == lanes, s"mask Vec[Bool] should have the length of $lanes")
    require(data.head.getWidth == bits, s"data should have the width of $bits")
    port.mask := VecInit(mask.map(m => Fill(bits, m))).asTypeOf(port.mask)
  }
}

private class DifftestMem1P(size: BigInt, lanes: Int, bits: Int) extends DifftestMem(size, lanes, bits, 1, 1) {
  assert(!read.head.valid || !write.head.valid, "read and write come at the same cycle")
}

private class DifftestMemMP(size: BigInt, lanes: Int, bits: Int, nr: Int, nw: Int)
  extends DifftestMem(size, lanes, bits, nr, nw) {
  override def desiredName: String = s"DifftestMem${nr}R${nw}W"
}

object DifftestMem {
  private def setDefaultIOs(mod: DifftestMem): DifftestMem = {
    mod.read := DontCare
    mod.read.foreach(_.valid := false.B)
    mod.write := DontCare
    mod.write.foreach(_.valid := false.B)
    mod
  }

  def apply(size: BigInt, beatBytes: Int): DifftestMem = apply(size, beatBytes, 8)

  // only for compatibility
  def apply(
    size: BigInt,
    lanes: Int,
    bits: Int,
    synthesizable: Boolean = false,
    singlePort: Boolean = true,
  ): DifftestMem = apply(size, lanes, bits, 1, 1)

  def apply(
    size: BigInt,
    lanes: Int,
    bits: Int,
    nr: Int,
    nw: Int,
  ): DifftestMem = setDefaultIOs(Module(new DifftestMemMP(size, lanes, bits, nr, nw)))
}
