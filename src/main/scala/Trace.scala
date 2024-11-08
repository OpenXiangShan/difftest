/***************************************************************************************
 * Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.trace

import chisel3._
import chisel3.experimental.ExtModule
import chisel3.util._
import difftest._
import difftest.common.FileControl

import scala.collection.mutable.ListBuffer

object Trace {
  def apply(bundles: MixedVec[DifftestBundle]): Unit = {
    val bundleType = chiselTypeOf(bundles).toSeq
    generateCppFile(bundleType, true)
    val module = Module(new TraceDumper(bundleType))
    module.io := bundles
  }

  def load(bundles: Seq[DifftestBundle]): MixedVec[DifftestBundle] = {
    generateCppFile(bundles, false)
    val module = Module(new TraceLoader(bundles))
    module.io
  }

  def generateCppFile(bundles: Seq[DifftestBundle], isDump: Boolean): Unit = {
    val difftestCpp = ListBuffer.empty[String]
    difftestCpp += "#ifndef __DIFFTEST_IOTRACE_H__"
    difftestCpp += "#define __DIFFTEST_IOTRACE_H__"
    difftestCpp += ""
    difftestCpp += "#include <cstdint>"
    difftestCpp += "#include \"svdpi.h\""
    difftestCpp += ""
    val uniqBundles = bundles.groupBy(_.desiredModuleName).toSeq.sortBy(_._2.head.order)
    val traceDecl = ListBuffer.empty[String]
    uniqBundles.foreach { case (name, gens) =>
      difftestCpp += gens.head.toTraceDeclaration
      difftestCpp += ""
      val suffix = if (gens.length == 1) "" else s"[${gens.length}]"
      val typeName = name.replace("Difftest", "DiffTrace")
      traceDecl += f"  ${typeName}%-30s ${gens.head.desiredCppName}${suffix};"
    }
    difftestCpp += "typedef struct __attribute__((packed)) {"
    difftestCpp ++= traceDecl
    difftestCpp += "} DiffTestIOTrace;"
    difftestCpp += "extern \"C\" void set_iotrace_name(char *s);"
    difftestCpp += "extern void difftest_iotrace_init();"
    difftestCpp += "extern void difftest_iotrace_free();"
    val dpicArgPrefix = if (isDump) "const" else ""
    val dpicArgLen = (bundles.map(_.getByteAlignWidth(true)).sum + 31) / 32
    val dpicFuncProto =
      s"""
         |extern "C" void v_difftest_trace (
         |  $dpicArgPrefix svBitVecVal io[$dpicArgLen]
         |)""".stripMargin
    difftestCpp += dpicFuncProto + ";"
    difftestCpp += "#endif // __DIFFTEST_TRACE_H__"
    difftestCpp += ""
    FileControl.write(difftestCpp, "difftest-iotrace.h")

    difftestCpp.clear()
    difftestCpp += "#include \"difftest-iotrace.h\""
    difftestCpp += "#include \"difftrace.h\""
    difftestCpp += "#include <cassert>"
    val (isRead, traceFunc) = if (isDump) {
      ("false", "append")
    } else {
      ("true", "read_next")
    }
    // set iotrace to zero before loading
    val traceInit = if (isDump) "" else "memset(io, 0, sizeof(DiffTestIOTrace));"
    // padding N trace to file, allow difftest to finish comparision within N cycles after loading last trace
    val traceFinish =
      if (isDump)
        s"""
           |  DiffTestIOTrace* padding = (DiffTestIOTrace*)calloc(1, sizeof(DiffTestIOTrace));
           |  for (int i = 0; i < 100; i++) {
           |    difftest_iotrace->append(padding);
           |  }
           |""".stripMargin
      else ""
    difftestCpp +=
      s"""
         |static char *iotrace_name = nullptr;
         |DiffTrace<DiffTestIOTrace> *difftest_iotrace = nullptr;
         |extern \"C\" void set_iotrace_name(char *s) {
         |  printf(\"difftest iotrace: %s\\n\", s);
         |  iotrace_name = (char *)malloc(256);
         |  strcpy(iotrace_name, s);
         |}
         |void difftest_iotrace_init() {
         |  assert(iotrace_name);
         |  difftest_iotrace = new DiffTrace<DiffTestIOTrace>(iotrace_name, $isRead);
         |}
         |void difftest_iotrace_free() {
         |$traceFinish
         |  delete difftest_iotrace;
         |  difftest_iotrace = nullptr;
         |}
         |$dpicFuncProto {
         |  if (!difftest_iotrace) {
         |    $traceInit
         |    return;
         |  }
         |  difftest_iotrace->${traceFunc}((DiffTestIOTrace *)io);
         |}
         |""".stripMargin
    FileControl.write(difftestCpp, "difftest-iotrace.cpp")
  }
}

class TraceDumper(bundles: Seq[DifftestBundle]) extends Module {
  val io = IO(Input(MixedVec(bundles)))
  val aligned = MixedVecInit(io.sortBy(_.order).toSeq.map(_.getByteAlign(true)))
  val trace = Module(new DifftestTrace(aligned.getWidth, true))
  trace.clock := clock
  trace.io := aligned.asUInt
  trace.enable := VecInit(io.flatMap(_.bits.needUpdate).toSeq).asUInt.orR
}

class TraceLoader(bundles: Seq[DifftestBundle]) extends Module {
  val io = IO(Output(MixedVec(bundles)))
  val io_sort = io.sortBy(_.order).toSeq
  val aligned = WireInit(
    0.U.asTypeOf(MixedVec(io_sort.map { b => UInt(b.getByteAlignWidth(true).W) }))
  )
  val trace = Module(new DifftestTrace(aligned.getWidth, false))
  trace.clock := clock
  trace.enable := !reset.asBool
  aligned := trace.io.asTypeOf(aligned)
  io_sort.zip(aligned).foreach { case (o, a) => o := o.reverseByteAlign(a, true) }
}

class DifftestTrace(width: Int, isDump: Boolean) extends ExtModule with HasExtModuleInline {
  def do_flip[T <: Data](dt: T): T = if (isDump) dt else Flipped(dt)
  val clock = IO(Input(Clock()))
  val enable = IO(Input(Bool()))
  val io = IO(do_flip(Input(UInt(width.W))))

  val io_direction = if (isDump) "input" else "output"
  val io_assign = if (isDump) "assign io_dummy = io;" else "assign io = io_dummy;"
  setInline(
    "DifftestTrace.v",
    s"""
       |module DifftestTrace(
       |  input clock,
       |  input enable,
       |  $io_direction [${width - 1}:0] io
       |);
       |
       |import "DPI-C" function void v_difftest_trace (
       |  $io_direction bit[${width - 1}:0] io
       |);
       |  bit[${width - 1}:0] io_dummy;
       |  $io_assign
       |  always @(posedge clock) begin
       |    if (enable) v_difftest_trace(io_dummy);
       |  end
       |endmodule
       |""".stripMargin,
  )
}
