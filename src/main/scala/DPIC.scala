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
import difftest._

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths}
import scala.collection.mutable.ListBuffer

class DPIC[T <: DifftestBundle](gen: T) extends ExtModule
  with HasExtModuleInline
  with DifftestModule[T] {
  val io = IO(Input(gen))

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
  val dpicFuncArgsWithClock: Seq[Seq[(String, Data)]] = {
    // ExtModule implicitly adds io_* prefix to the IOs (because the IO val is named as io).
    // This is different from BlackBoxes.
    io.elements.toSeq.reverse.map{ case (name, data) =>
      data match {
        case vec: Vec[_] => vec.zipWithIndex.map { case (v, i) => (s"io_${name}_$i", v) }
        case _ => Seq((s"io_$name", data))
      }
    }
  }
  val dpicFuncArgs: Seq[Seq[(String, Data)]] = dpicFuncArgsWithClock.map(_.filterNot(_._1 == "io_clock"))
  val dpicFuncAssigns: Seq[String] = {
    val filters: Seq[(DifftestBundle => Boolean, Seq[String])] = Seq(
      ((_: DifftestBundle) => true, Seq("io_coreid")),
      ((x: DifftestBundle) => x.withValid, Seq("io_valid")),
      ((x: DifftestBundle) => x.isIndexed, Seq("io_index")),
      ((x: DifftestBundle) => x.isFlatten, Seq("io_valid", "io_address")),
    )
    val rhs = dpicFuncArgs.map(_.map(_._1).filterNot(s => filters.exists(f => f._1(gen) && f._2.contains(s))))
    val lhs = rhs.map(_.map(_.replace("io_", ""))).flatMap(r =>
      if (r.length == 1) r
      else r.map(x => x.slice(0, x.lastIndexOf('_')) + s"[${x.split('_').last}]")
    )
    val indent = if (gen.isFlatten || gen.withValid) "  " else ""
    val body = lhs.zip(rhs.flatten).map{ case (l, r) => s"${indent}packet->$l = $r;" }
    if (gen.withValid) {
      val validAssign = if (gen.isFlatten) Seq() else Seq("packet->valid = io_valid;")
      validAssign ++ Seq("if (io_valid) {") ++ body ++ Seq("}")
    } else body
  }
  val dpicFuncProto: String =
    s"""
       |extern "C" void $dpicFuncName (
       |  ${dpicFuncArgs.flatten.map(arg => getDPICArgString(arg._1, arg._2, true)).mkString(",\n  ")}
       |)""".stripMargin
  val dpicFunc: String = {
    val packet = s"difftest[io_coreid]->dut.${gen.desiredCppName}"
    val index = if (gen.isIndexed) "[io_index]" else if (gen.isFlatten) "[io_address]" else ""
    s"""
       |$dpicFuncProto {
       |  if (difftest == NULL) return;
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
    val modPorts = dpicFuncArgsWithClock.flatten.map(i => getModArgString(i._1, i._2)).mkString(",\n  ")
    val modDef =
      s"""
         |module $desiredName(
         |  $modPorts
         |);
         |`ifndef SYNTHESIS
         |`ifdef DIFFTEST
         |$dpicDecl
         |  always @(posedge io_clock) begin
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

class Delayer[T <: Data](gen: T, n_cycles: Int) extends Module {
  val i = IO(Input(gen.cloneType))
  val o = IO(Output(gen.cloneType))

  var r = WireInit(i)
  for (_ <- 0 until n_cycles) {
    r = RegNext(r)
  }
  o := r
}

object Delayer {
  def apply[T <: Data](gen: T, n_cycles: Int): T = {
    if (n_cycles > 0) {
      val delayer = Module(new Delayer(gen, n_cycles))
      delayer.i := gen
      delayer.o
    }
    else {
      gen
    }
  }
}

object DPIC {
  val interfaces = ListBuffer.empty[(String, String, String)]

  def apply[T <: DifftestBundle](gen: T, delay: Int): T = {
    val module = Module(new DPIC(gen))
    if (!interfaces.map(_._1).contains(module.dpicFuncName)) {
      val interface = (module.dpicFuncName, module.dpicFuncProto, module.dpicFunc)
      interfaces += interface
    }
    if (delay > 0) {
      val difftest: T = Wire(module.io.cloneType)
      module.io := Delayer(difftest, delay)
      module.io.clock  := difftest.clock
      module.io.coreid := difftest.coreid
      difftest
    }
    else {
      module.io
    }
  }

  def collect(): Unit = {
    if (interfaces.isEmpty) {
      return
    }
    val interfaceCpp = ListBuffer.empty[String]
    interfaceCpp += "#ifndef __DIFFTEST_DPIC_H__"
    interfaceCpp += "#define __DIFFTEST_DPIC_H__"
    interfaceCpp += ""
    interfaceCpp += "#include <cstdint>"
    interfaceCpp += ""
    interfaceCpp += interfaces.map(_._2 + ";").mkString("\n")
    interfaceCpp += ""
    interfaceCpp += "#endif // __DIFFTEST_DPIC_H__"
    interfaceCpp += ""
    val outputDir = sys.env("NOOP_HOME") + "/build/generated-src"
    Files.createDirectories(Paths.get(outputDir))
    val outputHeaderFile = outputDir + "/difftest-dpic.h"
    Files.write(Paths.get(outputHeaderFile), interfaceCpp.mkString("\n").getBytes(StandardCharsets.UTF_8))

    interfaceCpp.clear()
    interfaceCpp += "#ifndef CONFIG_NO_DIFFTEST"
    interfaceCpp += ""
    interfaceCpp += "#include \"difftest.h\""
    interfaceCpp += "#include \"difftest-dpic.h\""
    interfaceCpp += interfaces.map(_._3).mkString("")
    interfaceCpp += ""
    interfaceCpp += "#endif // CONFIG_NO_DIFFTEST"
    interfaceCpp += ""
    val outputFile = outputDir + "/difftest-dpic.cpp"
    Files.write(Paths.get(outputFile), interfaceCpp.mkString("\n").getBytes(StandardCharsets.UTF_8))
  }
}
