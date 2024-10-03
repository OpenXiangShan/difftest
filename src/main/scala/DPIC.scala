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
import difftest.batch.{BatchInfo, BatchIO}
import difftest.gateway.{GatewayConfig, GatewayResult, GatewaySinkControl}

import scala.collection.mutable.ListBuffer

abstract class DPICBase(config: GatewayConfig) extends ExtModule with HasExtModuleInline {
  val clock = IO(Input(Clock()))
  val enable = IO(Input(Bool()))
  val dut_zone = Option.when(config.hasDutZone)(IO(Input(UInt(config.dutZoneWidth.W))))

  def getDirectionString(data: Data): String = {
    if (DataMirror.directionOf(data) == ActualDirection.Input) "input " else "output"
  }

  def getDPICArgString(argName: String, data: Data, isC: Boolean, isDPIC: Boolean = true): String = {
    val typeString = data.getWidth match {
      case 1                                  => if (isC) "uint8_t" else "bit"
      case width if width > 1 && width <= 8   => if (isC) "uint8_t" else "byte"
      case width if width > 8 && width <= 32  => if (isC) "uint32_t" else "int"
      case width if width > 32 && width <= 64 => if (isC) "uint64_t" else "longint"
      case width if width > 64 =>
        if (isC)
          if (isDPIC) "const svBitVecVal" else "uint8_t"
        else s"bit[${width - 1}:0]"
    }
    if (isC) {
      val width = data.getWidth
      val suffix = if (width > 64) {
        if (isDPIC) s"[${(width + 31) / 32}]" else s"[${(width + 7) / 8}]"
      } else ""
      f"$typeString%-8s $argName$suffix"
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

  protected val commonPorts = Seq(("clock", clock), ("enable", enable))
  def modPorts: Seq[Seq[(String, Data)]] = {
    var ports = commonPorts
    if (config.hasDutZone) ports ++= Seq(("dut_zone", dut_zone.get))
    ports.map(Seq(_))
  }

  def desiredName: String
  def dpicFuncName: String = s"v_difftest_${desiredName.replace("Difftest", "")}"
  def dpicFuncArgs: Seq[Seq[(String, Data)]] =
    modPorts.filterNot(p => p.length == 1 && commonPorts.exists(_._1 == p.head._1))
  def dpicFuncProto: String =
    s"""
       |extern "C" void $dpicFuncName (
       |  ${dpicFuncArgs.flatten.map(arg => getDPICArgString(arg._1, arg._2, true, !config.isFPGA)).mkString(",\n  ")}
       |)""".stripMargin
  def getPacketDecl(gen: DifftestBundle, prefix: String, config: GatewayConfig): String = {
    val dut_zone = if (config.hasDutZone) "dut_zone" else "0"
    val dut_index = if (config.isBatch) "dut_index" else "0"
    val packet = s"DUT_BUF(${prefix}coreid, $dut_zone, $dut_index)->${gen.desiredCppName}"
    val index = if (gen.isIndexed) s"[${prefix}index]" else if (gen.isFlatten) s"[${prefix}address]" else ""
    s"auto packet = &($packet$index);"
  }
  def dpicFuncAssigns: Seq[String]
  def perfCnt: String = {
    val name = "perf_" + dpicFuncName
    s"""
       |#ifdef CONFIG_DIFFTEST_PERFCNT
       |  dpic_calls[$name] ++;
       |  dpic_bytes[$name] += ${dpicFuncArgs.flatten.map(_._2.getWidth / 8).sum};
       |#endif // CONFIG_DIFFTEST_PERFCNT
       |""".stripMargin
  }

  def dpicFunc: String =
    s"""
       |$dpicFuncProto {
       |  if (!diffstate_buffer) return;
       |$perfCnt
       |  ${dpicFuncAssigns.mkString("\n  ")}
       |}
       |""".stripMargin

  def moduleBody: String = {
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
      if (config.isNonBlock)
        s"""
           |`ifdef PALLADIUM
           |initial $$ixc_ctrl("gfifo", "$dpicFuncName");
           |`endif
           |""".stripMargin
      else ""
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
         |      $dpicFuncName (${dpicFuncArgs.flatten.map(_._1).mkString(", ")});
         |  end
         |`endif
         |`endif
         |endmodule
         |""".stripMargin
    modDef
  }
}

class DPIC[T <: DifftestBundle](gen: T, config: GatewayConfig) extends DPICBase(config) with DifftestModule[T] {
  val io = IO(Input(gen))

  override def desiredName: String = gen.desiredModuleName
  override def modPorts: Seq[Seq[(String, Data)]] = {
    super.modPorts ++ io.elements.toSeq.reverse.map { case (name, data) =>
      data match {
        case vec: Vec[_] => vec.zipWithIndex.map { case (v, i) => (s"io_${name}_$i", v) }
        case _           => Seq((s"io_$name", data))
      }
    }
  }
  override def dpicFuncArgs: Seq[Seq[(String, Data)]] = if (gen.bits.hasValid) {
    super.dpicFuncArgs.filterNot(p => p.length == 1 && p.head._1 == "io_valid")
  } else {
    super.dpicFuncArgs
  }

  override def dpicFuncAssigns: Seq[String] = {
    val filters: Seq[(DifftestBundle => Boolean, Seq[String])] = Seq(
      ((_: DifftestBundle) => true, Seq("io_coreid", "dut_zone")),
      ((x: DifftestBundle) => x.isIndexed, Seq("io_index")),
      ((x: DifftestBundle) => x.isFlatten, Seq("io_address")),
    )
    val rhs = dpicFuncArgs.map(_.map(_._1).filterNot(s => filters.exists(f => f._1(gen) && f._2.contains(s))))
    val lhs = rhs
      .map(_.map(_.replace("io_", "")))
      .flatMap(r =>
        if (r.length == 1) r
        else r.map(x => x.slice(0, x.lastIndexOf('_')) + s"[${x.split('_').last}]")
      )
    val body = lhs.zip(rhs.flatten).map { case (l, r) => s"packet->$l = $r;" }
    val packetDecl = Seq(getPacketDecl(gen, "io_", config))
    val validAssign = if (!gen.bits.hasValid || gen.isFlatten) Seq() else Seq("packet->valid = true;")
    packetDecl ++ validAssign ++ body
  }

  setInline(s"$desiredName.v", moduleBody)
}

class DPICBatch(template: Seq[DifftestBundle], batchIO: BatchIO, config: GatewayConfig) extends DPICBase(config) {
  val io = IO(Input(UInt(batchIO.getWidth.W)))

  def getDPICBundleUnpack(gen: DifftestBundle): String = {
    val unpack = ListBuffer.empty[String]
    // Note: locating elems will not in struct defined, but at the end of reordered Bundle
    val (elem_names, elem_bytes) = gen.getByteAlignElems.map { case (name, data) => (name, data.getWidth / 8) }.unzip
    elem_names.zipWithIndex.foreach { case (name, idx) =>
      if (Seq("coreid", "index", "address").contains(name)) {
        val offset = elem_bytes.take(idx).sum
        unpack += s"$name = data[$offset];"
      }
    }
    unpack += getPacketDecl(gen, "", config)
    unpack += s"memcpy(packet, data, sizeof(${gen.desiredModuleName}));"
    unpack += s"data += ${elem_bytes.sum};"
    unpack.toSeq.mkString("\n        ")
  }

  override def modPorts = super.modPorts ++ Seq(Seq(("io", io)))

  override def desiredName: String = "DifftestBatch"
  override def dpicFuncAssigns: Seq[String] = {
    val bundleEnum = template.map(_.desiredModuleName.replace("Difftest", "")) ++ Seq("BatchInterval", "BatchFinish")
    val bundleAssign = template.zipWithIndex.map { case (t, idx) =>
      s"""
         |    else if (id == ${bundleEnum(idx)}) {
         |      for (int j = 0; j < num; j++) {
         |        ${getDPICBundleUnpack(t)}
         |      }
         |    }
        """.stripMargin
    }.mkString("")

    def parse(gen: BatchIO): (String, Int) = {
      val info = new BatchInfo
      val infoLen = gen.info.getWidth / info.getWidth
      val structDecl =
        s"""
           |  typedef struct {
           |    ${info.elements.toSeq.map { case (name, data) => getDPICArgString(name, data, true, false) }
            .mkString(";\n    ")};
           |  } BatchInfo;
           |  typedef struct {
           |    ${gen.elements.toSeq.map { case (name, data) =>
            if (name == "info") s"BatchInfo info[$infoLen]" else getDPICArgString(name, data, true, false)
          }.mkString(";\n    ")};
           |  } BatchPack;
           |  BatchPack* batch = (BatchPack*)io;
           |  BatchInfo* info = batch->info;
           |  uint8_t* data = batch->data;
           |""".stripMargin
      (structDecl, infoLen)
    }
    val (batchDecl, infoLen) = parse(batchIO)
    Seq(s"""
           |  enum DifftestBundleType {
           |  ${bundleEnum.mkString(",\n  ")}
           |  };
           |  extern void simv_nstep(uint8_t step);
           |  uint32_t dut_index = 0;
           |  $batchDecl
           |  for (int i = 0; i < $infoLen; i++) {
           |    uint8_t id = info[i].id;
           |    uint8_t num = info[i].num;
           |    uint32_t coreid, index, address;
           |    if (id == BatchFinish) {
           |#ifdef CONFIG_DIFFTEST_INTERNAL_STEP
           |      simv_nstep(num);
           |#endif // CONFIG_DIFFTEST_INTERNAL_STEP
           |      break;
           |    }
           |    else if (id == BatchInterval && i != 0) {
           |      dut_index ++;
           |      continue;
           |    }
           |    $bundleAssign
           |  }
           |""".stripMargin)
  }

  setInline(s"$desiredName.v", moduleBody)
}

private class DummyDPICWrapper(gen: Valid[DifftestBundle], config: GatewayConfig) extends Module {
  val control = IO(Input(new GatewaySinkControl(config)))
  val io = IO(Input(gen))
  val dpic = Module(new DPIC(gen.bits, config))
  dpic.clock := clock
  dpic.enable := io.valid && control.enable
  if (config.hasDutZone) dpic.dut_zone.get := control.dut_zone.get
  dpic.io := io.bits
}

private class DummyDPICBatchWrapper(
  template: Seq[DifftestBundle],
  batchIO: BatchIO,
  config: GatewayConfig,
) extends Module {
  val control = IO(Input(new GatewaySinkControl(config)))
  val io = IO(Input(batchIO))
  val dpic = Module(new DPICBatch(template, batchIO, config))
  dpic.clock := clock
  dpic.enable := control.enable
  if (config.hasDutZone) dpic.dut_zone.get := control.dut_zone.get
  dpic.io := io.asUInt
}

object DPIC {
  val interfaces = ListBuffer.empty[(String, String, String)]
  var defMacros = new StringBuilder()

  def apply(control: GatewaySinkControl, io: Valid[DifftestBundle], config: GatewayConfig): Unit = {
    val module = Module(new DummyDPICWrapper(chiselTypeOf(io), config))
    module.control := control
    module.io := io
    val dpic = module.dpic
    if (!interfaces.map(_._1).contains(dpic.dpicFuncName)) {
      val interface = (dpic.dpicFuncName, dpic.dpicFuncProto, dpic.dpicFunc)
      interfaces += interface
    }
  }

  def batch(template: Seq[DifftestBundle], control: GatewaySinkControl, io: BatchIO, config: GatewayConfig): Unit = {
    val module = Module(new DummyDPICBatchWrapper(template, chiselTypeOf(io), config))
    module.control := control
    module.io := io
    val dpic = module.dpic
    if (!config.isFPGA)
      defMacros ++=
        s"""
           |#ifdef CONFIG_DIFFTEST_BATCH
           |#include "svdpi.h"
           |#endif // CONFIG_DIFFTEST_BATCH""".stripMargin
    interfaces += ((dpic.dpicFuncName, dpic.dpicFuncProto, dpic.dpicFunc))
  }

  def collect(): GatewayResult = {
    if (interfaces.isEmpty) {
      return GatewayResult()
    }

    val interfaceCpp = ListBuffer.empty[String]
    interfaceCpp += "#ifndef __DIFFTEST_DPIC_H__"
    interfaceCpp += "#define __DIFFTEST_DPIC_H__"
    interfaceCpp += ""
    interfaceCpp += "#include <cstdint>"
    interfaceCpp += "#include \"diffstate.h\""
    interfaceCpp += "#ifdef CONFIG_DIFFTEST_PERFCNT"
    interfaceCpp += "#include \"perf.h\""
    interfaceCpp += "#endif // CONFIG_DIFFTEST_PERFCNT"
    interfaceCpp += defMacros.toString()
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
    interfaceCpp +=
      s"""
         |#ifdef CONFIG_DIFFTEST_PERFCNT
         |enum DIFFSTATE_PERF {
         |  ${(interfaces.map("perf_" + _._1) ++ Seq("DIFFSTATE_PERF_NUM")).mkString(",\n  ")}
         |};
         |long long dpic_calls[DIFFSTATE_PERF_NUM] = {0}, dpic_bytes[DIFFSTATE_PERF_NUM] = {0};
         |#endif // CONFIG_DIFFTEST_PERFCNT
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
         |
         |void diffstate_buffer_free() {
         |  for (int i = 0; i < NUM_CORES; i++) {
         |    delete diffstate_buffer[i];
         |  }
         |  delete[] diffstate_buffer;
         |  diffstate_buffer = nullptr;
         |}
      """.stripMargin
    interfaceCpp +=
      s"""
         |#ifdef CONFIG_DIFFTEST_PERFCNT
         |void diffstate_perfcnt_init() {
         |  for (int i = 0; i < DIFFSTATE_PERF_NUM; i++) {
         |    dpic_calls[i] = 0;
         |    dpic_bytes[i] = 0;
         |  }
         |}
         |void diffstate_perfcnt_finish(long long msec) {
         |  long long calls_sum = 0, bytes_sum = 0;
         |  const char *dpic_name[DIFFSTATE_PERF_NUM] = {
         |    ${interfaces.map("\"" + _._1 + "\"").mkString(",\n    ")}
         |  };
         |  for (int i = 0; i < DIFFSTATE_PERF_NUM; i++) {
         |    calls_sum += dpic_calls[i];
         |    bytes_sum += dpic_bytes[i];
         |    difftest_perfcnt_print(dpic_name[i], dpic_calls[i], dpic_bytes[i], msec);
         |  }
         |  difftest_perfcnt_print(\"DIFFSTATE_SUM\", calls_sum, bytes_sum, msec);
         |}
         |#endif // CONFIG_DIFFTEST_PERFCNT
         |""".stripMargin
    interfaceCpp += interfaces.map(_._3).mkString("")
    interfaceCpp += ""
    interfaceCpp += "#endif // CONFIG_NO_DIFFTEST"
    interfaceCpp += ""
    streamToFile(interfaceCpp, "difftest-dpic.cpp")

    GatewayResult(
      cppMacros = Seq("CONFIG_DIFFTEST_DPIC"),
      step = Some(1.U),
    )
  }
}
