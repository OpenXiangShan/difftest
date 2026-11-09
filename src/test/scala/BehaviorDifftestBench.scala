package difftest

import chisel3._
import chisel3.util._
import _root_.circt.stage.ChiselStage
import utility.SimulationModels
import java.nio.file.{Files, Paths}

/** Same public inputs/outputs for the hardware and ordinary-GRH model benchmark. */
class BehaviorDifftestBench(behavior: Boolean) extends Module {
  import BehaviorStudyConfig._
  val io = IO(new CommitRatBehaviorAdapterIO(physical, rats, commits.size, 300))
  if (behavior) {
    val m = Module(new CommitRatBehaviorAdapter(physical, rats.map(s => s.copy(count=s.rows)), commits))
    m.io.physical := io.physical; m.io.commit := io.commit
    rats.zipWithIndex.foreach { case (spec, bank) =>
      val in=io.rats(bank)
      BehaviorDifftest.compactRatWrites(spec.rows, in.enable, in.address, in.data).zipWithIndex.foreach {
        case ((enable, value), row) =>
          m.io.rats(bank).enable(row) := enable
          m.io.rats(bank).address(row) := row.U
          m.io.rats(bank).data(row) := value
      }
    }
    io.xrf := m.io.xrf; io.frf := m.io.frf; io.vrf := m.io.vrf
    io.commitData := m.io.commitData; io.vecCommitData := m.io.vecCommitData
    io.ratInt := m.io.ratInt; io.ratFp := m.io.ratFp; io.ratVec := m.io.ratVec
  } else {
    val m = Module(new CommitRatHardwareReference)
    io <> m.io
  }
}
object BehaviorDifftestBenchMain {
  def main(args: Array[String]): Unit = {
    val out=Paths.get(args(0)); Files.createDirectories(out)
    val behavior=args(2)=="B"
    SimulationModels.withMode(if(behavior) "commit-rat" else "hardware", Some(out.resolve("simulation_models.json"))) {
      val sv=ChiselStage.emitSystemVerilog(new BehaviorDifftestBench(behavior),
        args=Array("--firtool-binary-path", args(1)),
        firtoolOpts=Array("-O=release", "--disable-all-randomization", "--default-layer-specialization=enable",
          "--lowering-options=disallowLocalVariables,disallowPackedArrays,disallowPortDeclSharing"))
      Files.writeString(out.resolve("BehaviorDifftestBench.sv"),sv)
    }
  }
}
