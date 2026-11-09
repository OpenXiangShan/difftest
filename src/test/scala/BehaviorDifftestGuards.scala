package difftest

import chisel3._
import _root_.circt.stage.ChiselStage
import utility.SimulationModels

/** Reject a partially enabled observer before any Gateway or RTL is emitted. */
private[difftest] class BehaviorDifftestMissingCommit(excluded: Boolean) extends Module {
  if (excluded) DifftestModule.parseArgs(Array("--difftest-exclude", "InstrCommit"))
  val commit = DifftestModule(new DiffInstrCommit(64))
  commit := 0.U.asTypeOf(commit)
}

object BehaviorDifftestGuardsMain {
  def main(args: Array[String]): Unit = {
    require(args.length == 2, "<excluded> <firtool>")
    require(args(0) == "excluded")
    val expected = "commit-rat requires enabled, non-excluded commit events"
    val rejected = try {
      SimulationModels.withMode("commit-rat") {
        ChiselStage.emitSystemVerilog(new BehaviorDifftestMissingCommit(args(0) == "excluded"),
          args = Array("--firtool-binary-path", args(1)))
      }
      false
    } catch {
      case error: Exception =>
        Iterator.iterate[Throwable](error)(_.getCause).takeWhile(_ != null)
          .exists(e => Option(e.getMessage).exists(_.contains(expected)))
    }
    require(rejected, s"${args(0)} commit source was not rejected with the capability error")
    println(s"PASS: ${args(0)} commit source rejected")
  }
}
