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

package difftest.dpic

import chisel3._
import chisel3.util._
import chisel3.experimental.{ChiselAnnotation, DataMirror, ExtModule}
import chisel3.util.experimental.BoringUtils
import difftest._
import difftest.gateway.{GatewayConfig, GatewayBundle}

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths}
import scala.collection.mutable.ListBuffer

class DPIC[T <: DifftestBundle](gen: T, config: GatewayConfig) extends ExtModule
  with HasExtModuleInline
  with DifftestModule[T] {
  val clock = IO(Input(Clock()))
  val enable = IO(Input(Bool()))
  val io = IO(Input(gen))
  val dut_pos = Option.when(config.hasDutPos)(IO(Input(UInt(config.dutPosWidth.W))))

  def getDirectionString(data: Data): String = {
    if (DataMirror.directionOf(data) == ActualDirection.Input) "input " else "output"
  }

  def getDPICArgString(argName: String, data: Data, isC: Boolean): String = {
    val typeString = data.getWidth match {
      case 1                                  => if (isC) "uint8_t"  else "bit"
      case width if width > 1  && width <= 8  => if (isC) "uint8_t"  else "byte"
      case width if width > 8  && width <= 32 => if (isC) "uint32_t" else "int"
      case width if width > 32 && width <= 64 => if (isC) "uint64_t" else "longint"
      case _ => s"unsupported io type of width ${data.getWidth}!!\n"
    }
    if (isC) {
      f"$typeString%-8s $argName"
    }
    else {
      val directionString = getDirectionString(data)
      f"$directionString $typeString%8s $argName"
    }
  }

  def getModArgString(argName: String, data: Data): String = {
    val widthString = if (data.getWidth == 1) "      " else f"[${data.getWidth - 1}%2d:0]"
    val argString = Seq(getDirectionString(data), widthString, s"$argName")
    argString.mkString(" ")
  }

  override def desiredName: String = gen.desiredModuleName
  val dpicFuncName: String = s"v_difftest_${desiredName.replace("Difftest", "")}"
  val modPorts: Seq[Seq[(String, Data)]] = {
    val common = ListBuffer(Seq(("clock", clock)), Seq(("enable", enable)))
    if (config.hasDutPos) {
      common += Seq(("dut_pos", dut_pos.get))
    }
    // ExtModule implicitly adds io_* prefix to the IOs (because the IO val is named as io).
    // This is different from BlackBoxes.
    common.toSeq ++ io.elements.toSeq.reverse.map{ case (name, data) =>
      data match {
        case vec: Vec[_] => vec.zipWithIndex.map { case (v, i) => (s"io_${name}_$i", v) }
        case _ => Seq((s"io_$name", data))
      }
    }
  }
  val dpicFuncArgsWithClock = if (gen.bits.hasValid) {
    modPorts.filterNot(p => p.length == 1 && p.head._1 == "io_valid")
  } else modPorts
  val dpicDropNum = 2
  val dpicFuncArgs: Seq[Seq[(String, Data)]] = dpicFuncArgsWithClock.drop(2)
  val dpicFuncAssigns: Seq[String] = {
    val filters: Seq[(DifftestBundle => Boolean, Seq[String])] = Seq(
      ((_: DifftestBundle) => true, Seq("io_coreid")),
      ((_: DifftestBundle) => config.hasDutPos, Seq("dut_pos")),
      ((x: DifftestBundle) => x.isIndexed, Seq("io_index")),
      ((x: DifftestBundle) => x.isFlatten, Seq("io_address")),
    )
    val rhs = dpicFuncArgs.map(_.map(_._1).filterNot(s => filters.exists(f => f._1(gen) && f._2.contains(s))))
    val lhs = rhs.map(_.map(_.replace("io_", ""))).flatMap(r =>
      if (r.length == 1) r
      else r.map(x => x.slice(0, x.lastIndexOf('_')) + s"[${x.split('_').last}]")
    )
    val body = lhs.zip(rhs.flatten).map{ case (l, r) => s"packet->$l = $r;" }
    val validAssign = if (!gen.bits.hasValid || gen.isFlatten) Seq() else Seq("packet->valid = true;")
    validAssign ++ body
  }
  val dpicFuncProto: String =
    s"""
       |extern "C" void $dpicFuncName (
       |  ${dpicFuncArgs.flatten.map(arg => getDPICArgString(arg._1, arg._2, true)).mkString(",\n  ")}
       |)""".stripMargin
  val dpicFunc: String = {
    val dut_pos = if (config.hasDutPos) "dut_pos" else "0"
    val packet = s"DUT_BUF(io_coreid,$dut_pos)->${gen.desiredCppName}"
    val index = if (gen.isIndexed) "[io_index]" else if (gen.isFlatten) "[io_address]" else ""
    s"""
       |$dpicFuncProto {
       |  if (diffstate_buffer[io_coreid] == NULL) return;
       |  auto packet = &($packet$index);
       |  ${dpicFuncAssigns.mkString("\n  ")}
       |}
       |""".stripMargin
  }

  val moduleBody: String = {
    val dpicDecl =
    // (1) DPI-C function prototype
      s"""
         |import "DPI-C" function void $dpicFuncName (
         |  ${dpicFuncArgs.flatten.map(arg => getDPICArgString(arg._1, arg._2, false)).mkString(",\n  ")}
         |);
         |""".stripMargin
    // (2) module definition
    val modPortsString = modPorts.flatten.map(i => getModArgString(i._1, i._2)).mkString(",\n  ")
    // Initial for Palladium GFIFO
    val gfifoInitial =
      s"""
         |`ifdef PALLADIUM
         |initial $$ixc_ctrl("gfifo", "$dpicFuncName");
         |`endif
         |""".stripMargin
    val modDef =
      s"""
         |module $desiredName(
         |  $modPortsString
         |);
         |`ifndef SYNTHESIS
         |`ifdef DIFFTEST
         |$dpicDecl
         |$gfifoInitial
         |  always @(posedge clock) begin
         |    if (enable)
         |    $dpicFuncName (${dpicFuncArgs.flatten.map(_._1).mkString(", ")});
         |  end
         |`endif
         |`endif
         |endmodule
         |""".stripMargin
    modDef
  }

  setInline(s"$desiredName.v", moduleBody)
}

private class DummyDPICWrapper[T <: DifftestBundle](gen: T, config: GatewayConfig) extends Module {
  val io = IO(Input(UInt(gen.getWidth.W)))
  val enable = IO(Input(Bool()))
  val dut_pos = Option.when(config.hasDutPos)(IO(Input(UInt(config.dutPosWidth.W))))

  val unpack = io.asTypeOf(gen)
  val dpic = Module(new DPIC(gen, config))
  dpic.clock := clock
  dpic.enable := unpack.bits.getValid && enable
  if (config.hasDutPos) dpic.dut_pos.get := dut_pos.get
  dpic.io := unpack
}

object DPIC {
  val interfaces = ListBuffer.empty[(String, String, String)]

  def apply[T <: DifftestBundle](gen: T, config: GatewayConfig, port: GatewayBundle): UInt = {
    val module = Module(new DummyDPICWrapper(gen, config))
    module.enable := port.enable
    if (config.hasDutPos) module.dut_pos.get := port.dut_pos.get

    val dpic = module.dpic
    if (!interfaces.map(_._1).contains(dpic.dpicFuncName)) {
      val interface = (dpic.dpicFuncName, dpic.dpicFuncProto, dpic.dpicFunc)
      interfaces += interface
    }
    module.io
  }

  def collect(config: GatewayConfig): Seq[String] = {
    if (interfaces.isEmpty) {
      return Seq()
    }
    val buf_len = config.dutBufLen
    val class_def =
      s"""
         |class DPICBuffer : public DiffStateBuffer {
         |private:
         |  DiffTestState buffer[$buf_len];
         |  int read_ptr = 0;
         |public:
         |  DPICBuffer() {
         |    memset(buffer, 0, sizeof(buffer));
         |  }
         |  inline DiffTestState* get(int pos) {
         |    return buffer+pos;
         |  }
         |  inline DiffTestState* next() {
         |    DiffTestState* ret = buffer+read_ptr;
         |    read_ptr = (read_ptr + 1) % $buf_len;
         |    return ret;
         |  }
         |};
         |""".stripMargin
    val interfaceCpp = ListBuffer.empty[String]
    interfaceCpp += "#ifndef __DIFFTEST_DPIC_H__"
    interfaceCpp += "#define __DIFFTEST_DPIC_H__"
    interfaceCpp += ""
    interfaceCpp += "#include <cstdint>"
    interfaceCpp += "#include \"diffstate.h\""
    interfaceCpp += ""
    interfaceCpp += class_def
    interfaceCpp += interfaces.map(_._2 + ";").mkString("\n")
    interfaceCpp += ""
    interfaceCpp += "#endif // __DIFFTEST_DPIC_H__"
    interfaceCpp += ""
    val outputDir = sys.env("NOOP_HOME") + "/build/generated-src"
    Files.createDirectories(Paths.get(outputDir))
    val outputHeaderFile = outputDir + "/difftest-dpic.h"
    Files.write(Paths.get(outputHeaderFile), interfaceCpp.mkString("\n").getBytes(StandardCharsets.UTF_8))

    val diff_func =
      s"""
         |void diffstate_buffer_init() {
         |  for (int i = 0; i < NUM_CORES; i++) {
         |    diffstate_buffer[i] = new DPICBuffer;
         |  }
         |}
         |void diffstate_buffer_free() {
         |  for (int i = 0; i < NUM_CORES; i++) {
         |    delete diffstate_buffer[i];
         |  }
         |}
      """.stripMargin
    interfaceCpp.clear()
    interfaceCpp += "#ifndef CONFIG_NO_DIFFTEST"
    interfaceCpp += ""
    interfaceCpp += "#include \"difftest.h\""
    interfaceCpp += "#include \"difftest-dpic.h\""
    interfaceCpp += ""
    interfaceCpp += "DiffStateBuffer* diffstate_buffer[NUM_CORES];"
    interfaceCpp += "#define DUT_BUF(core_id,pos) (diffstate_buffer[core_id]->get(pos))"
    interfaceCpp += diff_func;
    interfaceCpp += interfaces.map(_._3).mkString("")
    interfaceCpp += ""
    interfaceCpp += "#endif // CONFIG_NO_DIFFTEST"
    interfaceCpp += ""
    val outputFile = outputDir + "/difftest-dpic.cpp"
    Files.write(Paths.get(outputFile), interfaceCpp.mkString("\n").getBytes(StandardCharsets.UTF_8))

    Seq("CONFIG_DIFFTEST_DPIC")
  }
}
