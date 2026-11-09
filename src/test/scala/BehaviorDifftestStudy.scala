package difftest

import chisel3._
import chisel3.util._
import _root_.circt.stage.ChiselStage
import difftest.util.Delayer
import utility.SimulationModels
import java.nio.file.{Files, Paths}

private[difftest] object BehaviorStudyConfig {
  val physical = Seq(
    BehaviorWriteSpec("int", 40, 64, 6, 4, 2),
    BehaviorWriteSpec("fp", 40, 64, 6, 4, 2),
    BehaviorWriteSpec("vf", 128, 128, 7, 4, 2),
    BehaviorWriteSpec("v0", 22, 128, 5, 4, 2))
  val rats = Seq(
    BehaviorWriteSpec("int-rat", 32, 8, 5, 4, 2),
    BehaviorWriteSpec("fp-rat", 32, 8, 5, 4, 2),
    BehaviorWriteSpec("vec-rat", 32, 8, 5, 4, 2),
    BehaviorWriteSpec("v0-rat", 1, 8, 0, 4, 2))
  val commits = (0 until 4).map(i => BehaviorCommitSource(300, s"study_commit_$i", 3))
}

/** Reference uses the original physical/RAT snapshots, Delayer and Preprocess. */
private[difftest] class CommitRatHardwareReference extends Module {
  import BehaviorStudyConfig._
  val io = IO(new CommitRatBehaviorAdapterIO(physical, rats, commits.size, 300))
  val physicalState = physical.zipWithIndex.map { case (s, bank) =>
    val regs = Reg(Vec(s.rows, UInt(s.dataWidth.W)))
    for (lane <- 0 until s.count) {
      when(io.physical(bank).enable(lane)) {
        regs(io.physical(bank).address(lane)) := io.physical(bank).data(lane)
      }
    }
    if (bank == 0) regs(0) := 0.U
    regs
  }
  val ratState = rats.zipWithIndex.map { case (s, bank) =>
    val regs = RegInit(VecInit((0 until s.rows).map(i => (if (bank == 0) 0 else i).U(s.dataWidth.W))))
    for (lane <- 0 until s.count) {
      when(io.rats(bank).enable(lane)) { regs(io.rats(bank).address(lane)) := io.rats(bank).data(lane) }
    }
    regs
  }
  val physicalVec = VecInit((physicalState(3).toSeq ++ physicalState(2).toSeq)
    .flatMap(v => Seq(v(63, 0), v(127, 64))))
  val ratVec = VecInit((Seq(ratState(3)(0).pad(9)) ++ ratState(2).drop(1).map(_.pad(9) + 22.U))
    .flatMap(p => Seq((p << 1)(8, 0), ((p << 1) + 1.U)(8, 0))))
  val pi = Wire(new DiffPhyIntRegState(40)); pi.coreid := io.commit.head.coreid; pi.value := physicalState(0)
  val pf = Wire(new DiffPhyFpRegState(40)); pf.coreid := pi.coreid; pf.value := physicalState(1)
  val pv = Wire(new DiffPhyVecRegState(300)); pv.coreid := pi.coreid; pv.value := physicalVec
  val ri = Wire(new DiffArchIntRenameTable(40)); ri.coreid := pi.coreid; ri.value := ratState(0)
  val rf = Wire(new DiffArchFpRenameTable(40)); rf.coreid := pi.coreid; rf.value := ratState(1)
  val rv = Wire(new DiffArchVecRenameTable(300)); rv.coreid := pi.coreid; rv.value := ratVec
  val event = WireInit(0.U.asTypeOf(new DiffArchEvent))
  val delayedPi = Delayer(pi, 2); val delayedPf = Delayer(pf, 2); val delayedPv = Delayer(pv, 2)
  val delayedCommits = io.commit.map(c => Delayer(c, 3))
  val original = preprocess.Preprocess.replaceRegs(Seq(event, delayedPi, delayedPf, delayedPv,
    Delayer(ri, 2), Delayer(rf, 2), Delayer(rv, 2)) ++ delayedCommits)
  io.xrf := original.find(_.desiredCppName == "xrf").get.asInstanceOf[DiffArchIntRegState]
  io.frf := original.find(_.desiredCppName == "frf").get.asInstanceOf[DiffArchFpRegState]
  io.vrf := original.find(_.desiredCppName == "vrf").get.asInstanceOf[DiffArchVecRegState]
  io.commitData := VecInit(original.filter(_.desiredCppName == "commit_data").map(_.asInstanceOf[DiffCommitData]))
  io.vecCommitData.zip(delayedCommits).foreach { case (v, c) =>
    v.coreid := c.coreid; v.index := c.index; v.valid := c.valid && (c.vecwen || c.v0wen)
    v.data := c.otherwpdest.map(p => delayedPv.value(p))
  }
  io.ratInt := ratState(0); io.ratFp := ratState(1); io.ratVec := ratVec
}

class BehaviorDifftestStudy extends Module {
  import BehaviorStudyConfig._
  val io = IO(new Bundle {
    val physical = Input(MixedVec(BehaviorStudyConfig.physical.map(s => new BehaviorWritePort(s.count, s.addrWidth, s.dataWidth))))
    val rats = Input(MixedVec(BehaviorStudyConfig.rats.map(s => new BehaviorWritePort(s.count, s.addrWidth, s.dataWidth))))
    val commit = Input(Vec(commits.size, new DiffInstrCommit(300)))
    val archEqual = Output(Bool())
    val ratEqual = Output(Bool())
    val commitEqual = Output(Bool())
    val vectorEqual = Output(Bool())
  })
  val grouped = SimulationModels.lowering == "grouped"
  val dut = Module(new CommitRatBehaviorAdapter(physical, if (grouped) rats else rats.map(s => s.copy(count = s.rows)), commits))
  SimulationModels.observerModule(dut, "difftest.commit-rat")
  private val ref = Module(new CommitRatHardwareReference)
  dut.io.physical := io.physical; dut.io.commit := io.commit
  ref.io.physical := io.physical; ref.io.rats := io.rats; ref.io.commit := io.commit
  if (grouped) { dut.io.rats := io.rats } else rats.zipWithIndex.foreach { case (spec, bank) =>
    val input = io.rats(bank)
    val compact = BehaviorDifftest.compactRatWrites(spec.rows, input.enable, input.address, input.data)
    compact.zipWithIndex.foreach { case ((enable, value), row) =>
      dut.io.rats(bank).enable(row) := enable
      dut.io.rats(bank).address(row) := row.U
      dut.io.rats(bank).data(row) := value
    }
  }
  io.archEqual := dut.io.xrf.value.asUInt === ref.io.xrf.value.asUInt &&
    dut.io.frf.value.asUInt === ref.io.frf.value.asUInt && dut.io.vrf.value.asUInt === ref.io.vrf.value.asUInt
  io.ratEqual := dut.io.ratInt.asUInt === ref.io.ratInt.asUInt &&
    dut.io.ratFp.asUInt === ref.io.ratFp.asUInt && dut.io.ratVec.asUInt === ref.io.ratVec.asUInt
  io.commitEqual := dut.io.commitData.zip(ref.io.commitData).map { case (a,b) =>
    a.valid === b.valid && (!b.valid || a.asUInt === b.asUInt)
  }.reduce(_ && _)
  io.vectorEqual := dut.io.vecCommitData.zip(ref.io.vecCommitData).map { case (a,b) =>
    a.valid === b.valid && (!b.valid || a.asUInt === b.asUInt)
  }.reduce(_ && _)
}

object BehaviorDifftestStudyMain {
  def main(args: Array[String]): Unit = {
    require(args.length == 2 || args.length == 3, "<output> <firtool> [ordinary|grouped]")
    val output = Paths.get(args(0)); Files.createDirectories(output)
    SimulationModels.withMode("commit-rat", Some(output.resolve("simulation_models.json")),
      if (args.length == 3) args(2) else "ordinary") {
      val sv = ChiselStage.emitSystemVerilog(new BehaviorDifftestStudy,
        args = Array("--firtool-binary-path", args(1)),
        firtoolOpts = Array("-O=release", "--disable-all-randomization",
          "--default-layer-specialization=enable",
          "--lowering-options=disallowLocalVariables,disallowPackedArrays,disallowPortDeclSharing"))
      Files.writeString(output.resolve("BehaviorDifftestStudy.sv"), sv)
    }
  }
}
