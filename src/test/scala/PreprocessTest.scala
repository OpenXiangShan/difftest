/***************************************************************************************
 * Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

package difftest

import chisel3._
import chisel3.simulator.scalatest.ChiselSim
import difftest.preprocess.Preprocess
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatest.matchers.should.Matchers

private class DirectIntPreprocessProbe(includeDirectXrf: Boolean, includeDirectCommitData: Boolean) extends Module {
  val io = IO(new Bundle {
    val directX1 = Input(UInt(64.W))
    val physX1 = Input(UInt(64.W))
    val directCommitData = Input(UInt(64.W))
    val physCommitData = Input(UInt(64.W))
    val fpCommitData = Input(UInt(64.W))
    val outX1 = Output(UInt(64.W))
    val outXrfCount = Output(UInt(4.W))
    val outCommit0Data = Output(UInt(64.W))
    val outCommit1Data = Output(UInt(64.W))
    val outCommitDataCount = Output(UInt(4.W))
    val outHasPregsXrf = Output(Bool())
    val outHasRatXrf = Output(Bool())
  })

  val event = WireInit(0.U.asTypeOf(new DiffArchEvent))
  event.coreid := 0.U

  val commit0 = WireInit(0.U.asTypeOf(new DiffInstrCommit(64)))
  commit0.coreid := 0.U
  commit0.index := 0.U
  commit0.valid := true.B
  commit0.rfwen := true.B
  commit0.wdest := 1.U
  commit0.wpdest := 33.U

  val commit1 = WireInit(0.U.asTypeOf(new DiffInstrCommit(64)))
  commit1.coreid := 0.U
  commit1.index := 1.U
  commit1.valid := true.B
  commit1.fpwen := true.B
  commit1.wdest := 2.U
  commit1.wpdest := 34.U

  val pregsXrf = WireInit(0.U.asTypeOf(new DiffPhyIntRegState(64)))
  pregsXrf.coreid := 0.U
  pregsXrf.value(33) := io.physX1

  val ratXrf = WireInit(0.U.asTypeOf(new DiffArchIntRenameTable(64)))
  ratXrf.coreid := 0.U
  ratXrf.value(1) := 33.U

  val pregsFrf = WireInit(0.U.asTypeOf(new DiffPhyFpRegState(64)))
  pregsFrf.coreid := 0.U
  pregsFrf.value(34) := io.fpCommitData

  val ratFrf = WireInit(0.U.asTypeOf(new DiffArchFpRenameTable(64)))
  ratFrf.coreid := 0.U
  ratFrf.value(2) := 34.U

  val directBundles = Seq.newBuilder[DifftestBundle]
  if (includeDirectXrf) {
    val directXrf = WireInit(0.U.asTypeOf(new DiffArchIntRegState))
    directXrf.coreid := 0.U
    directXrf.value(1) := io.directX1
    directBundles += directXrf
  }
  if (includeDirectCommitData) {
    val directCd0 = WireInit(0.U.asTypeOf(new DiffCommitData))
    directCd0.coreid := 0.U
    directCd0.index := 0.U
    directCd0.valid := true.B
    directCd0.data := io.directCommitData

    val directCd1 = WireInit(0.U.asTypeOf(new DiffCommitData))
    directCd1.coreid := 0.U
    directCd1.index := 1.U
    directCd1.valid := true.B
    directCd1.data := 0.U

    directBundles += directCd0
    directBundles += directCd1
  }

  val replaced = Preprocess.replaceRegs(
    Seq(event, commit0, commit1, pregsXrf, ratXrf, pregsFrf, ratFrf) ++ directBundles.result()
  )

  val xrf = replaced.collectFirst { case x: DiffArchIntRegState if x.desiredCppName == "xrf" => x }.get
  val xrfCount = replaced.count(_.desiredCppName == "xrf")
  val commitData = replaced.collect { case cd: DiffCommitData => cd }

  io.outX1 := xrf.value(1)
  io.outXrfCount := xrfCount.U
  io.outCommit0Data := commitData.head.data
  io.outCommit1Data := commitData(1).data
  io.outCommitDataCount := commitData.length.U
  io.outHasPregsXrf := replaced.exists(_.desiredCppName == "pregs_xrf").B
  io.outHasRatXrf := replaced.exists(_.desiredCppName == "rat_xrf").B
}

class PreprocessTest extends AnyFlatSpec with Matchers with ChiselSim {
  behavior of "Difftest Preprocess"

  private def checkDirectIntReusePattern(): Unit = {
    simulate(new DirectIntPreprocessProbe(includeDirectXrf = true, includeDirectCommitData = true)) { dut =>
      val valueFromA = BigInt("a001", 16)
      val valueFromC = BigInt("c003", 16)
      val fpValue = BigInt("f00d", 16)

      dut.io.directX1.poke(valueFromA.U)
      dut.io.physX1.poke(valueFromC.U)
      dut.io.directCommitData.poke(valueFromA.U)
      dut.io.physCommitData.poke(valueFromC.U)
      dut.io.fpCommitData.poke(fpValue.U)

      dut.io.outX1.expect(valueFromA.U)
      dut.io.outCommit0Data.expect(valueFromA.U)
      dut.io.outCommit1Data.expect(fpValue.U)
      dut.io.outXrfCount.expect(1.U)
      dut.io.outCommitDataCount.expect(2.U)
      dut.io.outHasPregsXrf.expect(false.B)
      dut.io.outHasRatXrf.expect(false.B)
    }
  }

  it should "prefer direct integer xrf over physical integer synthesis" in {
    simulate(new DirectIntPreprocessProbe(includeDirectXrf = true, includeDirectCommitData = false)) { dut =>
      dut.io.directX1.poke("h1111".U)
      dut.io.physX1.poke("h2222".U)
      dut.io.directCommitData.poke("h3333".U)
      dut.io.physCommitData.poke("h4444".U)
      dut.io.fpCommitData.poke("h5555".U)

      dut.io.outX1.expect("h1111".U)
      dut.io.outXrfCount.expect(1.U)
      dut.io.outCommit0Data.expect("h2222".U)
      dut.io.outCommit1Data.expect("h5555".U)
      dut.io.outCommitDataCount.expect(2.U)
      dut.io.outHasPregsXrf.expect(false.B)
      dut.io.outHasRatXrf.expect(false.B)
    }
  }

  it should "prefer direct integer commit data while keeping physical FP commit data" in {
    simulate(new DirectIntPreprocessProbe(includeDirectXrf = true, includeDirectCommitData = true)) { dut =>
      dut.io.directX1.poke("h1111".U)
      dut.io.physX1.poke("h2222".U)
      dut.io.directCommitData.poke("h3333".U)
      dut.io.physCommitData.poke("h4444".U)
      dut.io.fpCommitData.poke("h5555".U)

      dut.io.outX1.expect("h1111".U)
      dut.io.outXrfCount.expect(1.U)
      dut.io.outCommit0Data.expect("h3333".U)
      dut.io.outCommit1Data.expect("h5555".U)
      dut.io.outCommitDataCount.expect(2.U)
      dut.io.outHasPregsXrf.expect(false.B)
      dut.io.outHasRatXrf.expect(false.B)
    }
  }

  it should "prefer direct integer commit data independently from direct xrf" in {
    simulate(new DirectIntPreprocessProbe(includeDirectXrf = false, includeDirectCommitData = true)) { dut =>
      dut.io.directX1.poke("h1111".U)
      dut.io.physX1.poke("h2222".U)
      dut.io.directCommitData.poke("h3333".U)
      dut.io.physCommitData.poke("h4444".U)
      dut.io.fpCommitData.poke("h5555".U)

      dut.io.outX1.expect("h2222".U)
      dut.io.outXrfCount.expect(1.U)
      dut.io.outCommit0Data.expect("h3333".U)
      dut.io.outCommit1Data.expect("h5555".U)
      dut.io.outCommitDataCount.expect(2.U)
      dut.io.outHasPregsXrf.expect(false.B)
      dut.io.outHasRatXrf.expect(false.B)
    }
  }

  it should "keep legacy physical integer synthesis when no direct integer bundle exists" in {
    simulate(new DirectIntPreprocessProbe(includeDirectXrf = false, includeDirectCommitData = false)) { dut =>
      dut.io.directX1.poke("h1111".U)
      dut.io.physX1.poke("h2222".U)
      dut.io.directCommitData.poke("h3333".U)
      dut.io.physCommitData.poke("h4444".U)
      dut.io.fpCommitData.poke("h5555".U)

      dut.io.outX1.expect("h2222".U)
      dut.io.outXrfCount.expect(1.U)
      dut.io.outCommit0Data.expect("h2222".U)
      dut.io.outCommit1Data.expect("h5555".U)
      dut.io.outCommitDataCount.expect(2.U)
      dut.io.outHasPregsXrf.expect(false.B)
      dut.io.outHasRatXrf.expect(false.B)
    }
  }

  it should "keep direct integer data ahead of recycled physical values in an A B C reuse pattern" in {
    checkDirectIntReusePattern()
  }
}
