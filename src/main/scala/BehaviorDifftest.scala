/***************************************************************************************
 * Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
 *
 * DiffTest is licensed under Mulan PSL v2.
 ***************************************************************************************/

package difftest

import chisel3._
import chisel3.util._
import difftest.common.DifftestWiring
import difftest.util.Delayer
import utility.SimulationModels

import scala.collection.mutable.ListBuffer

private[difftest] case class BehaviorWriteSpec(
  name: String,
  rows: Int,
  dataWidth: Int,
  addrWidth: Int,
  count: Int,
  delay: Int,
)

private[difftest] case class BehaviorWriteSource(
  spec: BehaviorWriteSpec,
  enableNames: Seq[String],
  addressNames: Seq[String],
  dataNames: Seq[String],
)

private[difftest] case class BehaviorCommitSource(numPhyRegs: Int, sourceName: String, delay: Int)

private[difftest] class BehaviorWritePort(val count: Int, val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val enable = Vec(count, Bool())
  val address = Vec(count, UInt(addrWidth.W))
  val data = Vec(count, UInt(dataWidth.W))
}

private[difftest] class CommitRatBehaviorAdapterIO(
  physicalSpecs: Seq[BehaviorWriteSpec],
  ratSpecs: Seq[BehaviorWriteSpec],
  commits: Int,
  maxPhyRegs: Int,
) extends Bundle {
  val physical = Input(MixedVec(physicalSpecs.map(s => new BehaviorWritePort(s.count, s.addrWidth, s.dataWidth))))
  val rats = Input(MixedVec(ratSpecs.map(s => new BehaviorWritePort(s.count, s.addrWidth, s.dataWidth))))
  val commit = Input(Vec(commits, new DiffInstrCommit(maxPhyRegs)))

  val xrf = Output(new DiffArchIntRegState)
  val frf = Output(new DiffArchFpRegState)
  val vrf = Output(new DiffArchVecRegState)
  val commitData = Output(Vec(commits, new DiffCommitData))
  val vecCommitData = Output(Vec(commits, new DiffVecCommitData))
  val ratInt = Output(Vec(32, UInt(ratSpecs.find(_.name == "int-rat").get.dataWidth.W)))
  val ratFp = Output(Vec(32, UInt(ratSpecs.find(_.name == "fp-rat").get.dataWidth.W)))
  val ratVec = Output(Vec(64, UInt(log2Ceil(2 * (physicalSpecs.find(_.name == "v0").get.rows +
    physicalSpecs.find(_.name == "vf").get.rows)).W)))
}

/**
  * Semantic DiffTest observer for the single-core XiangShan simulation target.
  *
  * The adapter consumes commit-only RAT writes and physical-register writeback
  * events.  It reconstructs the canonical xrf/frf/vrf and commit_data bundles
  * without creating the original full physical-register Gateway interfaces.
  */
private[difftest] class CommitRatBehaviorAdapter(
  physicalSpecs: Seq[BehaviorWriteSpec],
  ratSpecs: Seq[BehaviorWriteSpec],
  commitSources: Seq[BehaviorCommitSource],
) extends Module {
  require(commitSources.nonEmpty, "commit-rat behavior model requires at least one commit source")
  require(commitSources.map(_.delay).distinct.size == 1, "commit sources must use one delay")
  require(physicalSpecs.map(_.name).distinct.size == physicalSpecs.size, "duplicate physical behavior source")
  require(ratSpecs.map(_.name).distinct.size == ratSpecs.size, "duplicate RAT behavior source")

  private val maxPhyRegs = commitSources.map(_.numPhyRegs).max
  val io = IO(new CommitRatBehaviorAdapterIO(
    physicalSpecs,
    ratSpecs,
    commitSources.size,
    maxPhyRegs,
  ))

  private def onePhysical(name: String): (BehaviorWriteSpec, Int) = {
    val matches = physicalSpecs.zipWithIndex.filter(_._1.name == name)
    require(matches.size == 1, s"commit-rat behavior model requires one physical source '$name'")
    matches.head
  }

  private def oneRat(name: String): (BehaviorWriteSpec, Int) = {
    val matches = ratSpecs.zipWithIndex.filter(_._1.name == name)
    require(matches.size == 1, s"commit-rat behavior model requires one RAT source '$name'")
    matches.head
  }

  private val (intPhysicalSpec, intPhysicalIndex) = onePhysical("int")
  private val (fpPhysicalSpec, fpPhysicalIndex) = onePhysical("fp")
  private val (vfPhysicalSpec, vfPhysicalIndex) = onePhysical("vf")
  private val (v0PhysicalSpec, v0PhysicalIndex) = onePhysical("v0")
  private val (intRatSpec, intRatIndex) = oneRat("int-rat")
  private val (fpRatSpec, fpRatIndex) = oneRat("fp-rat")
  private val (vecRatSpec, vecRatIndex) = oneRat("vec-rat")
  private val (v0RatSpec, v0RatIndex) = oneRat("v0-rat")

  private def delayed[T <: Data](data: T, cycles: Int): T = Delayer(data, cycles)

  private def rowIndex(address: UInt, rows: Int): UInt = {
    if (rows <= 1) 0.U
    else {
      val width = log2Ceil(rows)
      address.pad(width)(width - 1, 0)
    }
  }

  private val observationDelay = physicalSpecs.head.delay
  require(observationDelay >= 1 && physicalSpecs.forall(_.delay == observationDelay))
  require(ratSpecs.forall(_.delay == observationDelay), "physical/RAT snapshot delays must agree")
  require(commitSources.head.delay >= observationDelay, "commit delay precedes register observation")
  require(intPhysicalSpec.dataWidth == 64 && fpPhysicalSpec.dataWidth == 64)
  require(vfPhysicalSpec.dataWidth == 128 && v0PhysicalSpec.dataWidth == 128,
    "commit-rat currently requires 128-bit vector registers")
  require(intRatSpec.rows == 32 && fpRatSpec.rows == 32 && vecRatSpec.rows == 32 && v0RatSpec.rows == 1)
  private val vecIndexWidth = log2Ceil(2 * (v0PhysicalSpec.rows + vfPhysicalSpec.rows))
  private val alignedCommits = io.commit.map(c => delayed(c, commitSources.head.delay))
  private val readCommits = io.commit.map(c => delayed(c, commitSources.head.delay - observationDelay))

  SimulationModels.model(
    "commit-rat", clock, reset, observerOnly = true,
    commitBoundary = "ROB commit (RAT updates include all retiring micro-ops)",
    updateGroup = "commit-rat",
    lowering = Some("ordinary"),
  ) { ctx =>
    Seq("commit", "rat", "architectural-state", "physical-writeback", "integer", "fp", "vector")
      .foreach(ctx.capability)
    ctx.input("commit", io.commit.asUInt)
    physicalSpecs.zipWithIndex.foreach { case (spec, index) =>
      ctx.input(s"physical.${spec.name}", io.physical(index).asUInt)
    }
    ratSpecs.zipWithIndex.foreach { case (spec, index) =>
      ctx.input(s"rat.${spec.name}", io.rats(index).asUInt)
    }

    // The actual RF has no reset. Ordinary memories preserve that behavior;
    // the native simulator's zero initialization also applies to these shadows.
    def physical(spec: BehaviorWriteSpec, index: Int, zero: Boolean = false) = {
      val state = ctx.memory(s"physical.${spec.name}", spec.rows, UInt(spec.dataWidth.W))
      val port = io.physical(index)
      port.enable.indices.foreach { lane =>
        state.update(port.enable(lane) && (if (zero) port.address(lane) =/= 0.U else true.B),
          port.address(lane), port.data(lane))
      }
      state
    }
    val intPhysical = physical(intPhysicalSpec, intPhysicalIndex, zero = true)
    val fpPhysical = physical(fpPhysicalSpec, fpPhysicalIndex)
    val vfPhysical = physical(vfPhysicalSpec, vfPhysicalIndex)
    val v0Physical = physical(v0PhysicalSpec, v0PhysicalIndex)
    def intRead(index: UInt): UInt = Mux(index === 0.U, 0.U, intPhysical.read(index))

    val (intRat, fpRat, vecRat) = if (SimulationModels.lowering == "grouped") {
      SimulationModels.model("commit-rat", clock, reset, observerOnly = true,
        commitBoundary = "all ROB retiring micro-ops in source order", updateGroup = "rat-ordered") { rat =>
        Seq("rat", "integer", "fp", "vector", "raw-retirement-order").foreach(rat.capability)
        val int = rat.array("rat.int", 32, 0.U(intRatSpec.dataWidth.W))
        val fp = rat.array("rat.fp", 32, (i: Int) => i.U(fpRatSpec.dataWidth.W))
        val vec = rat.array("rat.vec", 32, (i: Int) => i.U(vecRatSpec.dataWidth.W))
        val v0 = rat.array("rat.v0", 1, 0.U(v0RatSpec.dataWidth.W))
        for ((state, index) <- Seq((int, intRatIndex), (fp, fpRatIndex),
          (vec, vecRatIndex), (v0, v0RatIndex))) {
          val port = io.rats(index)
          rat.input(ratSpecs(index).name, port.asUInt)
          port.enable.indices.foreach { lane =>
            val inRange = if (index == v0RatIndex) port.address(lane) === 0.U
              else port.address(lane) < 32.U && (if (index == vecRatIndex) port.address(lane) =/= 0.U else true.B)
            state.update(port.enable(lane) && inRange, port.address(lane), port.data(lane))
          }
        }
        val integer = int.snapshot; val floating = fp.snapshot; val vector = vec.snapshot
        val mapping = VecInit((0 until 32).flatMap { row =>
          val physical = if (row == 0) v0.read(0.U).pad(vecIndexWidth)
            else vector(row).pad(vecIndexWidth) + v0PhysicalSpec.rows.U
          Seq((physical << 1)(vecIndexWidth - 1, 0), ((physical << 1) + 1.U)(vecIndexWidth - 1, 0))
        })
        rat.output("rat.int", integer.asUInt); rat.output("rat.fp", floating.asUInt)
        rat.output("rat.vec", mapping.asUInt)
        (integer, floating, mapping)
      }
    } else {
    val intRat = ctx.array("rat.int", 32, 0.U(intRatSpec.dataWidth.W))
    val fpRat = ctx.array("rat.fp", 32, (i: Int) => i.U(fpRatSpec.dataWidth.W))
    val vecRat = ctx.array("rat.vec", 64, (i: Int) => {
      val physical = if (i < 2) i else 2 * (v0PhysicalSpec.rows + i / 2) + i % 2
      physical.U(vecIndexWidth.W)
    })
    for ((state, index) <- Seq((intRat, intRatIndex), (fpRat, fpRatIndex))) {
      val port = io.rats(index)
      port.enable.indices.foreach { lane =>
        state.update(port.enable(lane), port.address(lane), port.data(lane))
      }
    }
    for ((index, isV0) <- Seq((v0RatIndex, true), (vecRatIndex, false))) {
      val port = io.rats(index)
      port.enable.indices.foreach { lane =>
        val valid = port.enable(lane) && (if (isV0) port.address(lane) === 0.U
          else port.address(lane) =/= 0.U && port.address(lane) < 32.U)
        val logical = if (isV0) 0.U else port.address(lane)
        val physical = if (isV0) port.data(lane).pad(vecIndexWidth)
          else port.data(lane).pad(vecIndexWidth) + v0PhysicalSpec.rows.U
        vecRat.update(valid, logical << 1, physical << 1)
        vecRat.update(valid, (logical << 1) + 1.U, (physical << 1) + 1.U)
      }
    }
    (intRat.snapshot, fpRat.snapshot, vecRat.snapshot)
    }
    def vectorChunk(index: UInt): UInt = {
      val slot = index >> 1
      val value = Mux(index < (2 * v0PhysicalSpec.rows).U,
        v0Physical.read(rowIndex(slot, v0PhysicalSpec.rows)),
        vfPhysical.read(rowIndex(slot - v0PhysicalSpec.rows.U, vfPhysicalSpec.rows)))
      Mux(index(0), value(127, 64), value(63, 0))
    }

    // Cache the mapped architectural snapshot as the first observation-delay
    // stage. map(delay(P), delay(R)) == delay(map(P, R)); this moves the delay
    // after the gather, avoiding delay storage for the entire physical RF.
    val archInt = ctx.array("arch.int", 32, 0.U(64.W))
    val archFp = ctx.array("arch.fp", 32, 0.U(64.W))
    val archVec = ctx.array("arch.vec", 64, 0.U(64.W))
    (0 until 32).foreach { row =>
      archInt.update(true.B, row.U, intRead(intRat(row)))
      archFp.update(true.B, row.U, fpPhysical.read(fpRat(row)))
    }
    (0 until 64).foreach { row =>
      archVec.update(true.B, row.U, vectorChunk(vecRat(row)))
    }
    io.xrf.coreid := alignedCommits.head.coreid
    io.frf.coreid := alignedCommits.head.coreid
    io.vrf.coreid := alignedCommits.head.coreid
    io.xrf.value := delayed(archInt.snapshot, observationDelay - 1)
    io.frf.value := delayed(archFp.snapshot, observationDelay - 1)
    io.vrf.value := delayed(archVec.snapshot, observationDelay - 1)
    io.ratInt := intRat
    io.ratFp := fpRat
    io.ratVec := vecRat
    Seq("arch.int" -> io.xrf.value, "arch.fp" -> io.frf.value, "arch.vec" -> io.vrf.value,
      "rat.int" -> io.ratInt, "rat.fp" -> io.ratFp, "rat.vec" -> io.ratVec).foreach {
      case (name, data) => ctx.output(name, data.asUInt); ctx.check(name, data.asUInt)
    }

    // Delay the gathered values rather than the full physical snapshot. The
    // read address is delayed by (commitDelay - observationDelay), so the final
    // data and all commit metadata refer to the same cycle and lane.
    readCommits.zip(alignedCommits).zipWithIndex.foreach { case ((read, commit), lane) =>
      val cd = io.commitData(lane)
      cd.coreid := commit.coreid
      cd.index := commit.index
      cd.valid := commit.valid && (commit.rfwen || commit.fpwen)
      cd.data := delayed(Mux(read.fpwen, fpPhysical.read(rowIndex(read.wpdest, fpPhysicalSpec.rows)),
        intRead(rowIndex(read.wpdest, intPhysicalSpec.rows))), observationDelay)
      val vd = io.vecCommitData(lane)
      vd.coreid := commit.coreid
      vd.index := commit.index
      vd.valid := commit.valid && (commit.v0wen || commit.vecwen)
      vd.data := delayed(VecInit(read.otherwpdest.map(vectorChunk)), observationDelay)
    }
    ctx.output("commit.data", io.commitData.asUInt)
    ctx.output("commit.vector-data", io.vecCommitData.asUInt)
  }

}

private[difftest] object BehaviorDifftest {
  private val physicalSources = ListBuffer.empty[BehaviorWriteSource]
  private val ratSources = ListBuffer.empty[BehaviorWriteSource]
  private val commitSources = ListBuffer.empty[BehaviorCommitSource]
  private var built = false

  def registerPhysicalWrites(
    name: String,
    rows: Int,
    dataWidth: Int,
    addrWidth: Int,
    enables: Seq[Bool],
    addresses: Seq[UInt],
    data: Seq[UInt],
    delay: Int,
  ): Unit = {
    require(SimulationModels.enabled("commit-rat"))
    require(enables.nonEmpty && enables.length == addresses.length && addresses.length == data.length)
    val prefix = s"behavior_physical_$name"
    val enableNames = enables.indices.map(i => s"${prefix}_${i}_enable")
    val addressNames = addresses.indices.map(i => s"${prefix}_${i}_address")
    val dataNames = data.indices.map(i => s"${prefix}_${i}_data")
    enables.zip(enableNames).foreach { case (value, source) => DifftestWiring.addSource(value, source) }
    addresses.zip(addressNames).foreach { case (value, source) => DifftestWiring.addSource(value, source) }
    data.zip(dataNames).foreach { case (value, source) => DifftestWiring.addSource(value, source) }
    physicalSources += BehaviorWriteSource(
      BehaviorWriteSpec(name, rows, dataWidth, addrWidth, enables.length, delay),
      enableNames,
      addressNames,
      dataNames,
    )
  }

  private[difftest] def compactRatWrites(rows: Int, enables: Seq[Bool],
      addresses: Seq[UInt], data: Seq[UInt]): Seq[(Bool, UInt)] = {
    // Keep the last writer to each architectural row before crossing module
    // boundaries. The ROB's 520 possible micro-op updates are not interchangeable
    // with the later 8-lane RAB retirement stream. A balanced selection preserves
    // their order while transporting only one update per observable row.
    def last(items: Seq[(Bool, UInt)]): (Bool, UInt) = {
      if (items.size == 1) items.head
      else {
        val (left, right) = items.splitAt(items.size / 2)
        val a = last(left); val b = last(right)
        (a._1 || b._1, Mux(b._1, b._2, a._2))
      }
    }
    (0 until rows).map { row =>
      last(enables.zip(addresses).zip(data).map { case ((valid, address), value) =>
        (valid && address === row.U, value)
      })
    }
  }

  def registerRatWrites(
    name: String,
    rows: Int,
    dataWidth: Int,
    addrWidth: Int,
    enables: Seq[Bool],
    addresses: Seq[UInt],
    data: Seq[UInt],
    delay: Int,
  ): Unit = {
    require(SimulationModels.enabled("commit-rat"))
    require(enables.nonEmpty && enables.length == addresses.length && addresses.length == data.length)
    val updates = if (SimulationModels.lowering == "grouped")
      enables.indices.map(i => (enables(i), addresses(i), data(i)))
    else compactRatWrites(rows, enables, addresses, data).zipWithIndex.map { case ((e, d), row) =>
      (e, WireInit(row.U(addrWidth.W)), d)
    }
    val prefix = s"behavior_rat_$name"
    val enableNames = updates.indices.map(i => s"${prefix}_${i}_enable")
    val addressNames = updates.indices.map(i => s"${prefix}_${i}_address")
    val dataNames = updates.indices.map(i => s"${prefix}_${i}_data")
    updates.zipWithIndex.foreach { case ((enable, address, value), lane) =>
      DifftestWiring.addSource(enable, enableNames(lane))
      DifftestWiring.addSource(address, addressNames(lane))
      DifftestWiring.addSource(value, dataNames(lane))
    }
    ratSources += BehaviorWriteSource(
      BehaviorWriteSpec(name, rows, dataWidth, addrWidth, updates.size, delay),
      enableNames, addressNames, dataNames,
    )
  }

  def registerCommit(bundle: DiffInstrCommit, delay: Int): Unit = {
    require(SimulationModels.enabled("commit-rat"))
    // A core can have several commit lanes.  BoringUtils names identify one
    // source/sink pair, so keep each lane distinct while the adapter presents
    // them as one ordered Vec.
    val sourceName = s"behavior_commit_${commitSources.size}"
    DifftestWiring.addSource(bundle, sourceName)
    commitSources += BehaviorCommitSource(bundle.numPhyRegs, sourceName, delay)
  }

  def build(): Unit = {
    if (!SimulationModels.enabled("commit-rat") || built) return
    require(DifftestModule.get_current_interfaces().count(_._1.isUniqueIdentifier) == 1,
      "commit-rat requires exactly one core with full DiffTest events")
    built = true
    require(commitSources.nonEmpty, "commit-rat behavior model did not receive DiffInstrCommit")
    require(commitSources.map(_.numPhyRegs).distinct.size == 1)
    val physical = physicalSources.toSeq
    val rats = ratSources.toSeq
    val module = Module(new CommitRatBehaviorAdapter(
      physical.map(_.spec),
      rats.map(_.spec),
      commitSources.toSeq,
    ))
    SimulationModels.observerModule(module, "difftest.commit-rat")
    physical.zipWithIndex.foreach { case (source, index) =>
      source.enableNames.zipWithIndex.foreach { case (name, lane) =>
        DifftestWiring.addSink(module.io.physical(index).enable(lane), name)
      }
      source.addressNames.zipWithIndex.foreach { case (name, lane) =>
        DifftestWiring.addSink(module.io.physical(index).address(lane), name)
      }
      source.dataNames.zipWithIndex.foreach { case (name, lane) =>
        DifftestWiring.addSink(module.io.physical(index).data(lane), name)
      }
    }
    rats.zipWithIndex.foreach { case (source, index) =>
      source.enableNames.zipWithIndex.foreach { case (name, lane) =>
        DifftestWiring.addSink(module.io.rats(index).enable(lane), name)
      }
      source.addressNames.zipWithIndex.foreach { case (name, lane) =>
        DifftestWiring.addSink(module.io.rats(index).address(lane), name)
      }
      source.dataNames.zipWithIndex.foreach { case (name, lane) =>
        DifftestWiring.addSink(module.io.rats(index).data(lane), name)
      }
    }
    commitSources.zipWithIndex.foreach { case (source, index) =>
      DifftestWiring.addSink(module.io.commit(index), source.sourceName)
    }
    DifftestModule.attachBehavior(module.io.xrf, 0)
    DifftestModule.attachBehavior(module.io.frf, 0)
    DifftestModule.attachBehavior(module.io.vrf, 0)
    module.io.commitData.foreach(DifftestModule.attachBehavior(_, 0))
    commitSources.zipWithIndex.foreach { case (source, index) =>
      DifftestModule.attachBehavior(module.io.commit(index), source.delay)
    }
    // XiangShan's normal single-core profile intentionally omits vec_commit_data.
    // Keep the generated vector data available for a future multi-core adapter.
  }
}
