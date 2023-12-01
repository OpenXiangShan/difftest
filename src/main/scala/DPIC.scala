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
import chisel3.experimental.{ChiselAnnotation, ExtModule}
import chisel3.util.experimental.BoringUtils
import difftest._
import difftest.gateway.{GatewayConfig, GatewayBundle}

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths}
import scala.collection.mutable.ListBuffer

abstract class DPICBase(port: GatewayBundle, config: GatewayConfig) extends ExtModule
  with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val enable = IO(Input(port.enable))
  val select = Option.when(config.diffStateSelect)(IO(Input(port.select.get)))
  val batch_idx = Option.when(config.isBatch)(IO(Input(port.batch_idx.get)))

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
      f"input $typeString%8s $argName"
    }
  }

  def getModArgString(argName: String, data: Data): String = {
    val widthString = if (data.getWidth == 1) "      " else f"[${data.getWidth - 1}%2d:0]"
    val argString = Seq("input ", widthString, s"$argName")
    argString.mkString(" ")
  }

  def desiredName: String
  def dpicFuncName: String
  def modPorts: Seq[Seq[(String, Data)]] = {
    val common = ListBuffer(Seq(("clock", clock)), Seq(("enable", port.enable)))
    if (config.diffStateSelect) {
      common += Seq(("select", port.select.get))
    }
    if (config.isBatch) {
      common += Seq(("batch_idx", port.batch_idx.get))
    }
    common.toSeq
  }
  def dpicFuncArgsWithClock: Seq[Seq[(String, Data)]] = modPorts
  def dpicFuncArgs: Seq[Seq[(String, Data)]] = dpicFuncArgsWithClock.drop(2)

  def dutPos: String = {
    if (config.diffStateSelect) "select"
    else if (config.isBatch) "batch_idx"
    else "0"
  }
  def dpicFuncAssigns: Seq[String]
  def dpicFuncProto: String =
    s"""
       |extern "C" void $dpicFuncName (
       |  ${dpicFuncArgs.flatten.map(arg => getDPICArgString(arg._1, arg._2, true)).mkString(",\n  ")}
       |)""".stripMargin
  def dpicFunc: String = {
    s"""
       |$dpicFuncProto {
       |  if (diffstate_buffer == NULL) return;
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

class DPIC[T <: DifftestBundle](gen: T, port: GatewayBundle, config: GatewayConfig) extends DPICBase(port, config)
  with DifftestModule[T] {
  val io = IO(Input(gen))
  override def desiredName: String = gen.desiredModuleName
  override def dpicFuncName: String = s"v_difftest_${desiredName.replace("Difftest", "")}"

  // ExtModule implicitly adds io_* prefix to the IOs (because the IO val is named as io).
  // This is different from BlackBoxes.
  override def modPorts: Seq[Seq[(String, Data)]] = super.modPorts ++ gen.elements.toSeq.reverse.map { case (name, data) =>
      data match {
        case vec: Vec[_] => vec.zipWithIndex.map { case (v, i) => (s"io_${name}_$i", v) }
        case _ => Seq((s"io_$name", data))
      }
  }

  override def dpicFuncArgsWithClock: Seq[Seq[(String, Data)]] = {
    if (gen.bits.hasValid) {
      modPorts.filterNot(p => p.length == 1 && p.head._1 == "io_valid")
    } else modPorts
  }

  override def dpicFuncAssigns: Seq[String] = {
    val filters: Seq[(DifftestBundle => Boolean, Seq[String])] = Seq(
      ((_: DifftestBundle) => true, Seq("io_coreid")),
      ((_: DifftestBundle) => config.diffStateSelect, Seq("select")),
      ((_: DifftestBundle) => config.isBatch, Seq("batch_idx")),
      ((x: DifftestBundle) => x.isIndexed, Seq("io_index")),
      ((x: DifftestBundle) => x.isFlatten, Seq("io_address")),
    )
    val rhs = dpicFuncArgs.map(_.map(_._1).filterNot(s => filters.exists(f => f._1(gen) && f._2.contains(s))))
    val lhs = rhs.map(_.map(_.replace("io_", ""))).flatMap(r =>
      if (r.length == 1) r
      else r.map(x => x.slice(0, x.lastIndexOf('_')) + s"[${x.split('_').last}]")
    )
    val body = lhs.zip(rhs.flatten).map { case (l, r) => s"packet->$l = $r;" }
    val validAssign = if (!gen.bits.hasValid || gen.isFlatten) Seq() else Seq("packet->valid = true;")

    val packet = s"DUT_BUF(io_coreid,$dutPos)->${gen.desiredCppName}"
    val index = if (gen.isIndexed) "[io_index]" else if (gen.isFlatten) "[io_address]" else ""
    val head = Seq(s"auto packet = &($packet$index);")

    head ++ validAssign ++ body
  }
}

class DPICControl(coreid: UInt, port: GatewayBundle, config: GatewayConfig) extends DPICBase(port, config) {
  val squash_idx = Option.when(config.squashReplay)(IO(Input(port.squash_idx.get)))
  val io_coreid = IO(Input(coreid))

  override def desiredName: String = "DifftestControl"
  override def dpicFuncName: String = "v_difftest_Control"
  override def modPorts: Seq[Seq[(String, Data)]] = {
    val ports = ListBuffer.empty[Seq[(String, Data)]]
    ports ++= super.modPorts
    if (config.squashReplay) {
      ports += Seq(("squash_idx", port.squash_idx.get))
    }
    ports += Seq(("io_coreid", coreid))
    ports.toSeq
  }

  override def dpicFuncAssigns: Seq[String] = {
    val assigns = ListBuffer.empty[String]
    if (config.squashReplay) {
      assigns += s"*(diffstate_buffer[io_coreid].get_idx($dutPos)) = squash_idx;"
    }
    assigns.toSeq
  }
}

private class DummyDPICWrapper[T <: DifftestBundle](gen: T, port: GatewayBundle, config: GatewayConfig) extends Module {
  val interfaces = ListBuffer.empty[(String, String, String)]

  val io = IO(Input(UInt(gen.getWidth.W)))
  val enable = IO(Input(port.enable))
  val select = Option.when(config.diffStateSelect)(IO(Input(port.select.get)))
  val batch_idx = Option.when(config.isBatch)(IO(Input(port.batch_idx.get)))

  val unpack = io.asTypeOf(gen)
  val dpic = Module(new DPIC(gen, port, config))
  dpic.clock := clock
  dpic.enable := unpack.bits.getValid && enable
  if (config.diffStateSelect) {
    dpic.select.get := select.get
  }
  if (config.isBatch) {
    dpic.batch_idx.get := batch_idx.get
  }
  dpic.io := unpack
  interfaces += ((dpic.dpicFuncName, dpic.dpicFuncProto, dpic.dpicFunc))

  val hasControl = gen.isUniqueIdentifier && config.needControl
  val squash_idx = Option.when(hasControl)(IO(Input(port.squash_idx.get)))
  if (hasControl) {
    val control = Module(new DPICControl(gen.coreid, port, config))
    control.clock := clock
    control.enable := enable
    control.io_coreid := unpack.coreid
    if (config.diffStateSelect) {
      control.select.get := select.get
    }
    if (config.isBatch) {
      control.batch_idx.get := batch_idx.get
    }
    if (config.squashReplay) {
      control.squash_idx.get := squash_idx.get
    }
    interfaces += ((control.dpicFuncName, control.dpicFuncProto, control.dpicFunc))
  }
}

object DPIC {
  val interfaces = ListBuffer.empty[(String, String, String)]

  def apply[T <: DifftestBundle](gen: T, config: GatewayConfig, port: GatewayBundle): UInt = {
    val module = Module(new DummyDPICWrapper(gen, port.cloneType, config))
    module.enable := port.enable
    if (config.diffStateSelect) {
      module.select.get := port.select.get
    }
    if (config.isBatch) {
      module.batch_idx.get := port.batch_idx.get
    }
    if (module.hasControl) {
      if (config.squashReplay) {
        module.squash_idx.get := port.squash_idx.get
      }
    }

    for(ifs <- module.interfaces) {
      if (!interfaces.map(_._1).contains(ifs._1)) {
        interfaces += ifs
      }
    }
    module.io
  }

  def collect(config: GatewayConfig, port: GatewayBundle): Seq[String] = {
    if (interfaces.isEmpty) {
      return Seq()
    }

    val buf_len = {
      if (config.diffStateSelect) 2
      else if (config.isBatch) config.batchSize
      else 1
    }
    val class_def =
      s"""
         |class DPICBuffer : public DiffStateBuffer {
         |private:
         |  int squash_idx[$buf_len];
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
         |  inline int* get_idx(int pos) {
         |    return squash_idx+pos;
         |  }
         |  inline int* next_idx() {
         |    return squash_idx+read_ptr;
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
         |  diffstate_buffer = new DPICBuffer[NUM_CORES];
         |}
         |void diffstate_buffer_free() {
         |  delete[] diffstate_buffer;
         |}
      """.stripMargin
    interfaceCpp.clear()
    interfaceCpp += "#ifndef CONFIG_NO_DIFFTEST"
    interfaceCpp += ""
    interfaceCpp += "#include \"difftest.h\""
    interfaceCpp += "#include \"difftest-dpic.h\""
    interfaceCpp += ""
    interfaceCpp += "DiffStateBuffer* diffstate_buffer;"
    interfaceCpp += "#define DUT_BUF(core_id,pos) (diffstate_buffer[core_id].get(pos))"
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
