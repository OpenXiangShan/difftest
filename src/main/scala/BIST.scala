/***************************************************************************************
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

package difftest.bist

import chisel3._
import chisel3.experimental.ExtModule
import chisel3.util._
import difftest._
import difftest.common._
import difftest.gateway.GatewayResult

import scala.collection.mutable.ListBuffer

object BIST {
  var isDUT: Boolean = true
  private val instances = ListBuffer.empty[DifftestBundle]

  def apply(gen: DifftestBundle): Unit = {
    if (isDUT) {
      register(gen)
    }
  }

  def register(gen: DifftestBundle): Unit = {
    DifftestWiring.addSource(WireInit(gen.asUInt), s"bist_${instances.length}")
    instances += gen
  }

  def tester(mod: Module): Unit = {
    require(mod.isInstanceOf[HasTopMemoryMasterPort], "design top must implement HasTopMemoryMasterPort for BIST")
    val top = mod.asInstanceOf[Module with HasTopMemoryMasterPort]
    val bundleTypes = instances.toSeq
    val sinks = WireInit(0.U.asTypeOf(MixedVec(bundleTypes.map(b => UInt(b.getWidth.W)))))
    for ((data, bist_id) <- sinks.zipWithIndex) {
      DifftestWiring.addSink(data, s"bist_$bist_id")
    }
    val (read, write) = (top.getTopMemoryMasterRead, top.getTopMemoryMasterWrite)
    require(read.data.length == write.data.length, "read and write must have the same data bus width")
    val memBeatSize = read.data.length
    val p = BISTParams(bundleTypes, memBeatSize)

    val endpoint = Module(new BISTEndpointWrapper(p))
    endpoint.io.dut.mem.read := read
    endpoint.io.dut.mem.write := write
    endpoint.io.dut.interfaces := sinks
    when(endpoint.io.dut.reset) {
      top.reset := true.B
    }

    DifftestWiring.addSource(endpoint.io.dut.reset, "difftest_bist_reset", isHierarchical = true)
    DifftestWiring.addSource(endpoint.io.ref_.state, "difftest_bist_state", isHierarchical = true)
    DifftestWiring.addSource(endpoint.io.ref_.uart, "difftest_bist_uart", isHierarchical = true)

    instances.clear()
  }

  def collect(): GatewayResult = {
    require(instances.isEmpty, "must call BIST.tester before DifftestModule.finish")
    val state = DifftestWiring.getSink(UInt(64.W), "difftest_bist_state", isHierarchical = true)
    GatewayResult(
      cppMacros = Seq("CONFIG_DIFFTEST_BIST"),
      exit = state,
    )
  }
}

case class BISTParams(
  bundleTypes: Seq[DifftestBundle],
  memBeatSize: Int,
) {
  val nCores = bundleTypes.count(_.isUniqueIdentifier)
  val (nCommits, nPhyRegs) = {
    val commits = bundleTypes.filter(_.desiredCppName == "commit")
    (commits.length / nCores, commits.head.asInstanceOf[InstrCommit].numPhyRegs)
  }
  val hasFpExtension = bundleTypes.exists(_.desiredCppName == "regs_fp")
  require(bundleTypes.exists(_.desiredCppName == "wb_int"), "DiffIntWriteback must be defined for BIST")
  if (hasFpExtension) {
    require(bundleTypes.exists(_.desiredCppName == "wb_fp"), "DiffFpWriteback must be defined for BIST")
  }
}

class BISTEndpointIO(p: BISTParams) extends Bundle {
  val dut = new Bundle {
    val reset = Output(Bool())
    val mem = Input(new Bundle {
      val read = new DifftestMemReadIO(p.memBeatSize)
      val write = new DifftestMemWriteIO(p.memBeatSize)
    })
    val interfaces = Input(MixedVec(p.bundleTypes.map(tpe => UInt(tpe.getWidth.W))))
  }
  // named with ref_ to avoid conflicts with Chisel internal variables
  val ref_ = Output(new Bundle {
    val state = UInt(64.W)
    val uart = Valid(UInt(8.W))
  })
}

class BISTEndpoint(p: BISTParams) extends ExtModule {
  val clock = IO(Input(Bool()))
  val reset = IO(Input(Bool()))
  val io = IO(new BISTEndpointIO(p))
}

// to extract clock and reset implicitly
class BISTEndpointWrapper(p: BISTParams) extends Module {
  val io = IO(new BISTEndpointIO(p))
  dontTouch(io)

  val endpoint = Module(new BISTEndpoint(p))
  endpoint.clock := clock.asBool
  endpoint.reset := reset.asBool
  endpoint.io <> io
}
