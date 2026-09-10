/*
 * Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
 * DiffTest is licensed under Mulan PSL v2.
 */

package difftest.preprocess

import chisel3._
import chisel3.util._
import difftest._

// Reconstruct complete maps at ordered commit-group boundaries. Group IDs are
// opaque: their allocation, generation encoding, and lifetime belong to the DUT.
private class RenameTableState(
  renameWidth: Int,
  retireWidth: Int,
  groupWidth: Int,
  slotsPerGroup: Int,
  phyRegWidth: Int,
  numRegs: Int,
  targets: String,
  commitWidth: Int,
  numPhyRegs: Int,
) extends Module {
  private val slotCount = (BigInt(1) << groupWidth) * slotsPerGroup
  require(slotCount > 0 && slotCount.isValidInt, "rename snapshot storage size is out of range")
  private val slots = slotCount.toInt
  private val slotWidth = log2Ceil(slots).max(1)
  private val bankWidth = log2Ceil(renameWidth).max(1)
  private val mapEntries = numRegs * targets.split(',').length

  val io = IO(new Bundle {
    val enable = Input(Bool())
    val info =
      Input(new DiffRenameEvent(renameWidth, retireWidth, groupWidth, slotsPerGroup, phyRegWidth, numRegs, targets))
    val commits = Input(Vec(commitWidth, new DiffInstrCommit(numPhyRegs)))
    val haveState = Output(Bool())
    val state = Output(Vec(mapEntries, UInt(phyRegWidth.W)))
  })

  private def slotOf(group: UInt, member: UInt): UInt = {
    val slot = group * slotsPerGroup.U +& member
    slot.pad(slotWidth)(slotWidth - 1, 0)
  }

  val info = io.info
  val laneState = Wire(Vec(renameWidth + 1, Vec(mapEntries, UInt(phyRegWidth.W))))
  laneState.head := info.base
  for {
    lane <- 0 until renameWidth
    reg <- 0 until mapEntries
  } {
    val hit = info.renameValid(lane) && info.writeEnable(lane)(reg / numRegs) &&
      info.ldest(lane) === (reg % numRegs).U
    laneState(lane + 1)(reg) := Mux(hit, info.pdest(lane), laneState(lane)(reg))
  }

  val writeSlots = (0 until renameWidth).map { lane =>
    slotOf(info.groupId(lane), info.member(lane))
  }
  val writeValid = (0 until renameWidth).map { lane =>
    val overwritten = (lane + 1 until renameWidth).map { younger =>
      info.renameValid(younger) && writeSlots(younger) === writeSlots(lane)
    }.foldLeft(false.B)(_ || _)
    info.renameValid(lane) && !overwritten
  }
  val banks = Seq.fill(renameWidth)(Mem(slots, UInt((mapEntries * phyRegWidth).W)))
  val bankTags = Mem(slots, UInt(bankWidth.W))
  val occupied = RegInit(VecInit.fill(slots)(false.B))
  private def occupiedAt(slot: UInt): Bool = if (slots == 1) occupied.head else occupied(slot)

  for (lane <- 0 until renameWidth) {
    when(io.enable && info.renameValid(lane)) {
      assert(info.member(lane) < slotsPerGroup.U, "rename snapshot member is out of range")
      when(info.writeEnable(lane).orR) {
        assert(info.ldest(lane) < numRegs.U, "rename logical destination is out of range")
      }
    }
    when(io.enable && writeValid(lane)) {
      banks(lane).write(writeSlots(lane), laneState(lane + 1).asUInt)
      bankTags.write(writeSlots(lane), lane.U)
      occupiedAt(writeSlots(lane)) := true.B
    }
  }

  val anyCommit = io.commits.map(_.valid).reduce(_ || _)
  val lastCommit = PriorityMux(io.commits.reverse.map(c => c.valid -> c))
  val commitMember = Mux(anyCommit, lastCommit.index % slotsPerGroup.U, info.fallbackMember)
  val readSlot = slotOf(info.commitGroupId, commitMember)
  val readState = Mux1H(UIntToOH(bankTags(readSlot), renameWidth), VecInit(banks.map(_.read(readSlot))))
    .asTypeOf(Vec(mapEntries, UInt(phyRegWidth.W)))
  val committed = Reg(Vec(mapEntries, UInt(phyRegWidth.W)))
  val haveCommitted = RegInit(false.B)

  // A combinational read keeps the selected map in the same transaction as
  // commits, physical registers, and architectural events. All state changes
  // are gated by this transaction's fire, including rename-only transactions.
  io.haveState := info.commitValid || haveCommitted
  io.state := Mux(info.commitValid, readState, committed)
  when(io.enable) {
    assert(!anyCommit || info.commitValid, "rename observation and commit epochs do not match")
    when(anyCommit) {
      val retireLane = lastCommit.index / slotsPerGroup.U
      val matchingGroup = (0 until retireWidth).map { lane =>
        retireLane === lane.U && info.retireValid(lane) &&
        info.retireGroupId(lane) === info.commitGroupId
      }.reduce(_ || _)
      assert(matchingGroup, "rename observation selects a different retiring group")
    }
    when(info.commitValid) {
      assert(commitMember < slotsPerGroup.U, "rename selected member is out of range")
      assert(occupiedAt(readSlot), "rename selection has no captured state")
      for (lane <- 0 until renameWidth) {
        assert(
          !info.renameValid(lane) || writeSlots(lane) =/= readSlot,
          "rename snapshot is selected and captured in the same transaction",
        )
      }
      committed := readState
      haveCommitted := true.B
    }
  }

  for (lane <- 0 until retireWidth) {
    when(io.enable && info.retireValid(lane)) {
      for (writer <- 0 until renameWidth) {
        assert(
          !info.renameValid(writer) || info.groupId(writer) =/= info.retireGroupId(lane),
          "rename group is retired and captured in the same transaction",
        )
      }
      for (member <- 0 until slotsPerGroup) {
        occupiedAt(slotOf(info.retireGroupId(lane), member.U)) := false.B
      }
    }
  }
}

object Rename {
  def replaceRenameTables(bundles: Seq[DifftestBundle], enable: Bool): Seq[DifftestBundle] = {
    val observations = bundles.collect { case observation: DiffRenameEvent => observation }
    if (observations.isEmpty) return bundles

    val numCores = bundles.count(_.isUniqueIdentifier)
    require(
      numCores > 0 && observations.length == numCores,
      "rename observations require exactly one complete observation per core",
    )
    val commits = bundles.collect { case commit: DiffInstrCommit => commit }
    require(
      commits.nonEmpty && commits.length % numCores == 0,
      "rename observations require symmetric commit interfaces",
    )
    val coreCommits = commits.grouped(commits.length / numCores).toSeq
    val rats = bundles.collect { case rat: DiffArchRenameTable => rat }.groupBy(_.desiredCppName)

    val replacements = observations.zipWithIndex.flatMap { case (info, core) =>
      val cs = coreCommits(core)
      require(
        cs.length == info.retireWidth * info.slotsPerGroup,
        "commit interfaces must describe every member of each retiring group",
      )
      require(
        info.targetNames.nonEmpty && info.targetNames.distinct.length == info.targetNames.length,
        "rename observation targets must be nonempty and unique",
      )
      val originals = info.targetNames.map { target =>
        val perCore =
          rats.getOrElse(target, throw new IllegalArgumentException(s"rename observation target $target is missing"))
        require(perCore.length == numCores, s"rename target $target must be present once per core")
        val original = perCore(core)
        require(
          original.value.nonEmpty && original.value.length % info.numRegs == 0,
          s"rename target $target cannot be expanded from ${info.numRegs} logical registers",
        )
        original
      }
      val state = Module(
        new RenameTableState(
          info.renameWidth,
          info.retireWidth,
          info.groupWidth,
          info.slotsPerGroup,
          info.phyRegWidth,
          info.numRegs,
          info.targetNames.mkString(","),
          cs.length,
          cs.head.numPhyRegs,
        )
      )
      state.io.enable := enable
      state.io.info := info
      state.io.commits := VecInit(cs)
      when(enable) {
        for (commit <- cs) {
          when(commit.valid) {
            assert(commit.coreid === info.coreid, "rename observation and commit core identifiers do not match")
          }
        }
        when(state.io.haveState) {
          for (rat <- originals) {
            assert(rat.coreid === info.coreid, "rename observation and RAT core identifiers do not match")
          }
        }
      }
      originals.zipWithIndex.map { case (original, bank) =>
        val replacement = Wire(chiselTypeOf(original))
        replacement := original
        val ratio = original.value.length / info.numRegs
        when(state.io.haveState) {
          replacement.value := VecInit((0 until info.numRegs).flatMap { reg =>
            (0 until ratio).map { slice =>
              state.io.state(bank * info.numRegs + reg) * ratio.U + slice.U
            }
          })
        }
        original -> replacement
      }
    }

    bundles.filterNot(_.isInstanceOf[DiffRenameEvent]).map { bundle =>
      replacements.find { case (original, _) => original eq bundle }.map(_._2).getOrElse(bundle)
    }
  }
}
