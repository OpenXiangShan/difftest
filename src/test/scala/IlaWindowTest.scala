package difftest

import chisel3._
import chisel3.stage.ChiselGeneratorAnnotation
import circt.stage.ChiselStage

class IlaWindowTestCpu extends Module with HasDiffTestInterfaces {
  val startCycle = IO(Input(UInt(64.W)))
  val commitGap = IO(Input(UInt(8.W)))
  val finishCycle = IO(Input(UInt(64.W)))
  val corrupt = IO(Input(Bool()))
  val cycles = IO(Output(UInt(64.W)))
  val commits = IO(Output(UInt(64.W)))
  val cycle = Reg(UInt(64.W))
  val retired = RegInit(0.U(64.W))
  val commitValid = cycle =/= startCycle && cycle % commitGap === 0.U
  cycle := cycle + 1.U
  when(reset.asBool) { cycle := startCycle }
  when(commitValid) { retired := retired + 1.U }
  cycles := cycle
  commits := retired

  val commit = DifftestModule(new DiffInstrCommit, dontCare = true)
  commit := 0.U.asTypeOf(commit)
  commit.valid := commitValid
  commit.rfwen := retired =/= 0.U
  commit.wdest := 1.U
  commit.pc := ("h80000000".U + (retired << 2))(63, 0)
  commit.instr := Mux(retired === 0.U, "h00000013".U, "h00108093".U)

  val trap = DifftestModule(new DiffTrapEvent, dontCare = true)
  trap := 0.U.asTypeOf(trap)
  trap.cycleCnt := cycle
  trap.instrCnt := retired + commitValid
  trap.hasTrap := cycle === finishCycle

  val event = DifftestModule(new DiffArchEvent, dontCare = true)
  event := 0.U.asTypeOf(event)
  val registers = DifftestModule(new DiffArchIntRegState, dontCare = true)
  registers := 0.U.asTypeOf(registers)
  val committed = retired + commitValid
  registers.value(1) := Mux(committed === 0.U, 0.U, committed - 1.U) ^
    Mux(corrupt && cycle >= finishCycle / 2.U, 1.U, 0.U)
  val csr = DifftestModule(new DiffCSRState, dontCare = true)
  csr := 0.U.asTypeOf(csr)
  csr.privilegeMode := 3.U
}

object IlaWindowTest extends App {
  DifftestModule.parseArgs(Array("--difftest-config", args(0)))
  (new ChiselStage).execute(
    Array("--target", "systemverilog", "--split-verilog", "--target-dir", args(1)),
    Seq(ChiselGeneratorAnnotation(() => DifftestModule.top(new IlaWindowTestCpu))),
  )
}
