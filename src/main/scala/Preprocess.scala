/***************************************************************************************
 * Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.preprocess

import chisel3._
import chisel3.util._
import difftest._
import difftest.gateway.GatewayConfig
import difftest.util.PipelineConnect

object Preprocess {
  def apply(
    bundles: DecoupledIO[MixedVec[DifftestBundle]],
    config: GatewayConfig,
  ): DecoupledIO[MixedVec[DifftestBundle]] = {
    val module = Module(new PreprocessEndpoint(chiselTypeOf(bundles.bits).toSeq, config))
    module.in <> bundles
    module.out
  }

  def getArchRegs(bundles: Seq[DifftestBundle], isHardware: Boolean): Seq[ArchRegState with DifftestBundle] = {
    bundles.collect { case p: DiffPhyRegState => p }
      .groupBy(_.desiredCppName)
      .flatMap { case (name, pregs) =>
        val archTarget = pregs.head.archTarget
        val ratTarget = pregs.head.ratTarget
        require(!bundles.exists(_.isInstanceOf[archTarget.type]))
        if (isHardware) {
          val needRat = pregs.head.needRat
          val rats = bundles.collect {
            case rat: DiffArchRenameTable if rat.desiredCppName == ratTarget.desiredCppName => rat
          }
          require((needRat && rats.length == pregs.length) || (!needRat && rats.isEmpty))
          pregs.zipWithIndex.map { case (preg, idx) =>
            val archReg = Wire(archTarget)
            archReg.coreid := preg.coreid
            if (needRat) {
              val rat = rats(idx)
              require(rat.numPhyRegs == preg.numPhyRegs)
              archReg.value.zipWithIndex.foreach { case (data, vid) =>
                data := preg.value(rat.value(vid))
              }
            } else {
              archReg.value := preg.value
            }
            archReg
          }
        } else {
          Seq.fill(pregs.length)(archTarget)
        }
      }
      .toSeq
  }
  // Replace PhyReg + Rename with ArchReg + CommitData/VecCommitData
  def replaceRegs(bundles: Seq[DifftestBundle]): Seq[DifftestBundle] = {
    def getBundle[T <: DifftestBundle](name: String): Seq[T] =
      bundles.filter(_.desiredCppName == name).asInstanceOf[Seq[T]]

    val numCores = bundles.count(_.isUniqueIdentifier)
    val archRegs = getArchRegs(bundles, true)

    val commits = getBundle[DiffInstrCommit]("commit")
    val phyInts = getBundle[DiffPhyIntRegState]("pregs_xrf")
    val phyFps = getBundle[DiffPhyFpRegState]("pregs_frf")
    val phyVecs = getBundle[DiffPhyVecRegState]("pregs_vrf")
    val commitDatas = commits.zipWithIndex.flatMap { case (c, idx) =>
      val coreID = idx / (commits.length / numCores)
      val intData = phyInts(coreID).value(c.wpdest)
      val fpData = if (phyFps.nonEmpty) phyFps(coreID).value(c.wpdest) else 0.U
      val cd = Wire(new DiffCommitData)
      cd.coreid := c.coreid
      cd.index := c.index
      cd.valid := c.valid && (c.rfwen || c.fpwen)
      cd.data := Mux(c.fpwen, fpData, intData)
      // Also skip vec_commit_data (used in vec_load check) for single core
      val vcd = Option.when(phyVecs.nonEmpty && numCores > 1) {
        val gen = Wire(new DiffVecCommitData)
        gen.coreid := c.coreid
        gen.index := c.index
        gen.valid := c.valid && (c.v0wen || c.vecwen)
        gen.data := c.otherwpdest.map { wpdest =>
          phyVecs(coreID).value(wpdest)
        }
        gen
      }
      Seq(cd) ++ vcd.toSeq
    }

    bundles.filterNot(b => Seq("pregs_", "rat_").exists(s => b.desiredCppName.contains(s))) ++ archRegs ++ commitDatas
  }

  def addUArchInfo(bundles: Seq[DifftestBundle], valid: Bool): Seq[DifftestBundle] = {
    val probes = bundles.collect { case probe: DiffUArchProbe[_] => probe }
    if (probes.isEmpty) {
      bundles
    } else {
      val numCores = bundles.count(_.isUniqueIdentifier)
      val traps = bundles.collect { case trap: DiffTrapEvent => trap }
      require(numCores > 0, "DiffUArchProbe requires DiffArchEvent to identify cores")
      require(traps.length == numCores, "DiffUArchProbe requires one DiffTrapEvent per core")
      require(probes.length % numCores == 0, "DiffUArchProbe instances must be symmetric across cores")

      val probesPerCore = probes.length / numCores
      require(probesPerCore <= (1 << 16), "DiffUArchProbe uarchId exceeds 16 bits")
      val layouts = probes.grouped(probesPerCore).map(_.map(_.desiredCppName)).toSeq
      require(layouts.tail.forall(_ == layouts.head), "DiffUArchProbe layout must be symmetric across cores")
      layouts.head.groupBy(identity).foreach { case (name, instances) =>
        require(instances.length <= (1 << 8), s"$name index exceeds 8 bits")
      }

      val processed = probes.zipWithIndex.map { case (probe, globalId) =>
        val localId = globalId % probesPerCore
        val coreBase = globalId - localId
        val typeIndex = probes.slice(coreBase, globalId).count(_.desiredCppName == probe.desiredCppName)
        val coreMatches = traps.map(_.coreid === probe.coreid)
        val result = WireInit(probe)
        result.uarchId := localId.U
        result.index := typeIndex.U
        result.cycleCnt := Mux1H(coreMatches, traps.map(_.cycleCnt))
        when(valid && probe.valid) {
          assert(PopCount(coreMatches) === 1.U, "DiffUArchProbe coreid must match exactly one DiffTrapEvent")
        }
        result
      }

      val processedIterator = processed.iterator
      bundles.map {
        case _: DiffUArchProbe[_] => processedIterator.next()
        case bundle               => bundle
      }
    }
  }
}

class PreprocessEndpoint(bundles: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Flipped(Decoupled(MixedVec(bundles))))
  val pipelined = Wire(Decoupled(MixedVec(bundles)))
  PipelineConnect(in, pipelined, pipelined.fire)

  val replaceReg = if (!config.softArchUpdate && pipelined.bits.exists(_.desiredCppName == "pregs_xrf")) {
    // extract ArchReg in Hardware
    Preprocess.replaceRegs(pipelined.bits)
  } else {
    pipelined.bits
  }

  // LoadEvent will not be checked when single-core
  val skipLoad = if (replaceReg.count(_.isUniqueIdentifier) == 1) {
    replaceReg.filterNot(_.desiredCppName == "load")
  } else {
    replaceReg
  }

  val preprocessed = MixedVecInit(Preprocess.addUArchInfo(skipLoad, pipelined.valid).toSeq)
  val out = IO(Decoupled(chiselTypeOf(preprocessed)))
  pipelined.ready := out.ready
  out.valid := pipelined.valid
  out.bits := preprocessed
}
