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
import chisel3.experimental.ExtModule
import chisel3.reflect.DataMirror
import chisel3.util._
import difftest.DifftestModule.streamToFile
import difftest._
import difftest.gateway.{GatewayBatchBundle, GatewayBundle, GatewayConfig}

import scala.collection.mutable.ListBuffer

class DPIC[T <: DifftestBundle](gen: T, config: GatewayConfig)
  extends ExtModule
  with HasExtModuleInline
  with DifftestModule[T] {
  val clock = IO(Input(Clock()))
  val enable = IO(Input(Bool()))
  val io = IO(Input(gen))
  val dut_zone = Option.when(config.hasDutZone)(IO(Input(UInt(config.dutZoneWidth.W))))
  val dut_index = Option.when(config.isBatch)(IO(Input(UInt(log2Ceil(config.batchSize).W))))

  def getDirectionString(data: Data): String = {
    if (DataMirror.directionOf(data) == ActualDirection.Input) "input " else "output"
  }

  def getDPICArgString(argName: String, data: Data, isC: Boolean): String = {
    val typeString = data.getWidth match {
      case 1                                  => if (isC) "uint8_t" else "bit"
      case width if width > 1 && width <= 8   => if (isC) "uint8_t" else "byte"
      case width if width > 8 && width <= 32  => if (isC) "uint32_t" else "int"
      case width if width > 32 && width <= 64 => if (isC) "uint64_t" else "longint"
      case _                                  => s"unsupported io type of width ${data.getWidth}!!\n"
    }
    if (isC) {
      f"$typeString%-8s $argName"
    } else {
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
    if (config.hasDutZone) common += Seq(("dut_zone", dut_zone.get))
    if (config.isBatch) common += Seq(("dut_index", dut_index.get))
    // ExtModule implicitly adds io_* prefix to the IOs (because the IO val is named as io).
    // This is different from BlackBoxes.
    common.toSeq ++ io.elements.toSeq.reverse.map { case (name, data) =>
      data match {
        case vec: Vec[_] => vec.zipWithIndex.map { case (v, i) => (s"io_${name}_$i", v) }
        case _           => Seq((s"io_$name", data))
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
      ((_: DifftestBundle) => config.hasDutZone, Seq("dut_zone")),
      ((_: DifftestBundle) => config.isBatch, Seq("dut_index")),
      ((x: DifftestBundle) => x.isIndexed, Seq("io_index")),
      ((x: DifftestBundle) => x.isFlatten, Seq("io_address"))
    )
    val rhs = dpicFuncArgs.map(_.map(_._1).filterNot(s => filters.exists(f => f._1(gen) && f._2.contains(s))))
    val lhs = rhs
      .map(_.map(_.replace("io_", "")))
      .flatMap(r =>
        if (r.length == 1) r
        else r.map(x => x.slice(0, x.lastIndexOf('_')) + s"[${x.split('_').last}]")
      )
    val body = lhs.zip(rhs.flatten).map { case (l, r) => s"packet->$l = $r;" }
    val validAssign = if (!gen.bits.hasValid || gen.isFlatten) Seq() else Seq("packet->valid = true;")
    validAssign ++ body
  }
  val dpicFuncProto: String =
    s"""
       |extern "C" void $dpicFuncName (
       |  ${dpicFuncArgs.flatten.map(arg => getDPICArgString(arg._1, arg._2, true)).mkString(",\n  ")}
       |)""".stripMargin
  val dpicFunc: String = {
    val dut_zone = if (config.hasDutZone) "dut_zone" else "0"
    val dut_index = if (config.isBatch) "dut_index" else "0"
    val packet = s"DUT_BUF(io_coreid, $dut_zone, $dut_index)->${gen.desiredCppName}"
    val index = if (gen.isIndexed) "[io_index]" else if (gen.isFlatten) "[io_address]" else ""
    s"""
       |$dpicFuncProto {
       |  if (!diffstate_buffer) return;
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
         |`ifdef CONFIG_DIFFTEST_NONBLOCK
         |`ifdef PALLADIUM
         |initial $$ixc_ctrl("gfifo", "$dpicFuncName");
         |`endif
         |`endif // CONFIG_DIFFTEST_NONBLOCK
         |""".stripMargin
    val modDef =
      s"""
         |`include "DifftestMacros.v"
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
  val io = IO(Input(new GatewayBundle(gen, config)))
  val dpic = Module(new DPIC(gen, config))
  dpic.clock := clock
  dpic.enable := io.data.bits.getValid && io.enable
  dpic.io := io.data
  if (config.hasDutZone) dpic.dut_zone.get := io.dut_zone.get
}

private class DummyDPICBatchWrapper[T <: Seq[DifftestBundle]](bundles: T, config: GatewayConfig) extends Module {
  val io = IO(Input(new GatewayBatchBundle(bundles, config)))
  val interfaces = ListBuffer.empty[(String, String, String)]
  for ((group, ifo) <- io.data.zip(io.info)) {
    for (gen <- group) {
      val dpic = Module(new DPIC(gen.cloneType, config))
      dpic.clock := clock
      dpic.enable := gen.bits.getValid && io.enable
      dpic.io := gen
      if (config.hasDutZone) dpic.dut_zone.get := io.dut_zone.get
      dpic.dut_index.get := ifo
      if (!interfaces.map(_._1).contains(dpic.dpicFuncName)) {
        val interface = (dpic.dpicFuncName, dpic.dpicFuncProto, dpic.dpicFunc)
        interfaces += interface
      }
    }
  }
}

object DPIC {
  val interfaces = ListBuffer.empty[(String, String, String)]

  def apply(bundle: GatewayBundle, config: GatewayConfig): Unit = {
    val module = Module(new DummyDPICWrapper(bundle.data.cloneType, config))
    module.io := bundle
    val dpic = module.dpic
    if (!interfaces.map(_._1).contains(dpic.dpicFuncName)) {
      val interface = (dpic.dpicFuncName, dpic.dpicFuncProto, dpic.dpicFunc)
      interfaces += interface
    }
  }

  def batch(template: MixedVec[DifftestBundle], bundle: GatewayBatchBundle, config: GatewayConfig): Unit = {
    val module = Module(new DummyDPICBatchWrapper(template.toSeq.map(_.cloneType), config))
    module.io := bundle
    interfaces ++= module.interfaces.toSeq
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
    interfaceCpp += "#include \"diffstate.h\""
    interfaceCpp += ""
    interfaceCpp +=
      """
        |class DPICBuffer : public DiffStateBuffer {
        |private:
        |  DiffTestState buffer[CONFIG_DIFFTEST_ZONESIZE][CONFIG_DIFFTEST_BUFLEN];
        |  int read_ptr = 0;
        |  int zone_ptr = 0;
        |  bool init = true;
        |public:
        |  DPICBuffer() {
        |    memset(buffer, 0, sizeof(buffer));
        |  }
        |  inline DiffTestState* get(int zone, int index) {
        |    return buffer[zone] + index;
        |  }
        |  inline DiffTestState* next() {
        |    DiffTestState* ret = buffer[zone_ptr] + read_ptr;
        |    read_ptr = read_ptr + 1;
        |    return ret;
        |  }
        |  inline void switch_zone() {
        |    if (init) {
        |      init = false;
        |      return;
        |    }
        |    zone_ptr = (zone_ptr + 1) % CONFIG_DIFFTEST_ZONESIZE;
        |    read_ptr = 0;
        |  }
        |};
        |""".stripMargin
    interfaceCpp += interfaces.map(_._2 + ";").mkString("\n")
    interfaceCpp += ""
    interfaceCpp += "#endif // __DIFFTEST_DPIC_H__"
    interfaceCpp += ""
    streamToFile(interfaceCpp, "difftest-dpic.h")

    interfaceCpp.clear()
    interfaceCpp += "#ifndef CONFIG_NO_DIFFTEST"
    interfaceCpp += ""
    interfaceCpp += "#include \"difftest.h\""
    interfaceCpp += "#include \"difftest-dpic.h\""
    interfaceCpp += ""
    interfaceCpp +=
      s"""
         |DiffStateBuffer** diffstate_buffer = nullptr;
         |#define DUT_BUF(core_id, zone, index) (diffstate_buffer[core_id]->get(zone, index))
         |
         |void diffstate_buffer_init() {
         |  diffstate_buffer = new DiffStateBuffer*[NUM_CORES];
         |  for (int i = 0; i < NUM_CORES; i++) {
         |    diffstate_buffer[i] = new DPICBuffer;
         |  }
         |}
         |void diffstate_buffer_free() {
         |  for (int i = 0; i < NUM_CORES; i++) {
         |    delete diffstate_buffer[i];
         |  }
         |  delete[] diffstate_buffer;
         |}
      """.stripMargin
    interfaceCpp += interfaces.map(_._3).mkString("")
    interfaceCpp += ""
    interfaceCpp += "#endif // CONFIG_NO_DIFFTEST"
    interfaceCpp += ""
    streamToFile(interfaceCpp, "difftest-dpic.cpp")
  }
}
