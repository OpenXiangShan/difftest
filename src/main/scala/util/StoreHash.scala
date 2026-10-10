/***************************************************************************************
 * Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
 *
 * DiffTest is licensed under Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 ***************************************************************************************/

package difftest.util

import chisel3._
import chisel3.util._
import difftest.{DiffStoreEvent, DiffStoreEventQueue, DiffStoreHashEventQueue, DifftestBundle}

/** Canonical records and a zero-seed CRC summary for one accepted StoreEvent. */
object StoreHash {
  val MaxRecords = 16
  val MaxLanes = 3

  // A merged summary must wait for every store, including non-monotonic preCommit stamps.
  // Stamp ordering assumes a span below half the ring, as in the software checker.
  def latestStamp(left: UInt, right: UInt): UInt = {
    val distance = (right - left)(11, 0) // Same 12-bit stamp ring as Stamper and the C++ checker.
    Mux(distance.orR && !distance(11), right, left)
  }

  // CRC-64/6sub8x: normal polynomial 0xf1 with the implicit x^64 term.
  // Source: https://users.ece.cmu.edu/~koopman/crc/crc64.html
  // One record is addr64 || maskedData64 || mask8, MSB first, with seed zero.
  val crc =
    new ParallelCRC(
      width = 64,
      polynomial = BigInt("f1", 16),
      blockWidth = 64 + 64 + 8,
      maxBlocks = MaxLanes * MaxRecords,
    )
  private class Record extends Bundle {
    val valid = Bool()
    val addr = UInt(64.W)
    val data = UInt(64.W)
    val mask = UInt(8.W)
  }

  private def maskExpand(mask: UInt): UInt = {
    Cat((0 until 8).reverse.map(byte => Fill(8, mask(byte))))
  }

  private def records(store: DiffStoreEvent, valid: Bool): Seq[Record] = {
    // StoreEvent.eew is the element width in bytes: 1, 2, 4 or 8.
    // Enumerate these geometries at elaboration time, dropping only windows
    // that are always empty. Preserve element/low/high order.
    val geometries = Seq(1, 2, 4, 8).flatMap { eew =>
      (0 until eew).map { offset =>
        val fragments = (-1 until (16 / eew)).flatMap { element =>
          val start = element * eew + offset
          val end = start + eew
          (0 until 2).flatMap { half =>
            val mask = (0 until 8).foldLeft(0) { (bits, byte) =>
              val position = half * 8 + byte
              if (start <= position && position < end) bits | (1 << byte) else bits
            }
            if (mask != 0) Seq((half, mask)) else Seq.empty
          }
        }
        require(fragments.size <= MaxRecords)
        val selectedEew = if (eew == 1) store.eew === 1.U || store.eew === 0.U else store.eew === eew.U
        val selectedOffset = if (eew == 1) true.B else store.offset(log2Ceil(eew) - 1, 0) === offset.U
        (selectedEew && selectedOffset, fragments)
      }
    }

    when(valid && store.vecNeedSplit) {
      assert(
        Seq(0, 1, 2, 4, 8).map(eew => store.eew === eew.U).reduce(_ || _),
        "StoreEvent vector EEW must be 1/2/4/8 bytes (zero retains the one-byte fallback)",
      )
    }

    val highAddr = store.addr +% 8.U
    (0 until MaxRecords).map { slot =>
      val entries = geometries.collect {
        case (selected, fragments) if slot < fragments.size =>
          (selected, fragments(slot))
      }
      val high = entries.collect { case (selected, (1, _)) => selected }.foldLeft(false.B)(_ || _)
      val windowMask = entries.map { case (selected, (_, mask)) =>
        Mux(selected, mask.U(8.W), 0.U(8.W))
      }.reduce(_ | _)
      val vectorMask = windowMask & Mux(high, store.mask(15, 8), store.mask(7, 0))
      val vectorRecord = WireInit(0.U.asTypeOf(new Record))
      vectorRecord.valid := valid && vectorMask.orR
      vectorRecord.addr := Mux(high, highAddr, store.addr)
      vectorRecord.data := Mux(high, store.highData, store.data) & maskExpand(vectorMask)
      vectorRecord.mask := vectorMask

      val lineRecord = WireInit(0.U.asTypeOf(new Record))
      if (slot < 8) {
        lineRecord.valid := valid
        lineRecord.addr := Cat(store.addr(63, 6), 0.U(6.W)) +% (slot * 8).U
        lineRecord.mask := "hff".U
      }
      val scalarRecord = WireInit(0.U.asTypeOf(new Record))
      if (slot < 2) {
        val mask = if (slot == 0) store.mask(7, 0) else store.mask(15, 8)
        scalarRecord.valid := valid && mask.orR
        scalarRecord.addr := (if (slot == 0) store.addr else highAddr)
        scalarRecord.data := (if (slot == 0) store.data else store.highData) & maskExpand(mask)
        scalarRecord.mask := mask
      }
      val record = Wire(new Record)
      record := Mux(store.vecNeedSplit, vectorRecord, Mux(store.wLine, lineRecord, scalarRecord))
      record
    }
  }

  /** Each lane computes one zero-seed summary independently of the old CRC. */
  def summarize(store: DiffStoreEvent, valid: Bool): ParallelCRC.Summary = {
    val storeRecords = records(store, valid)
    val hash = storeRecords.foldLeft(crc.seed) { (state, record) =>
      val next = crc.update(state, Cat(record.addr, record.data, record.mask))
      Mux(record.valid, next, state)
    }
    ParallelCRC.Summary(PopCount(VecInit(storeRecords.map(_.valid))), hash)
  }

  /** Combine stamped store lanes into one summary per core before cross-cycle squash. */
  def apply(bundles: MixedVec[Valid[DifftestBundle]], accepted: Bool): MixedVec[Valid[DifftestBundle]] = {
    val stores = bundles.filter(_.bits.isInstanceOf[DiffStoreEventQueue])
    if (stores.isEmpty) return bundles
    val numCores = bundles.count(_.bits.isUniqueIdentifier)
    require(stores.length % numCores == 0, "Store lanes must be evenly distributed across cores")
    val lanesPerCore = stores.length / numCores
    require(lanesPerCore > 0 && lanesPerCore <= MaxLanes)
    val sequence = RegInit(VecInit.fill(numCores)(0.U(64.W)))

    // The gateway orders lanes by core, then by event index, as for commit lanes.
    val hashes = stores
      .grouped(lanesPerCore)
      .zipWithIndex
      .map { case (lanes, core) =>
        val hash = WireInit(0.U.asTypeOf(Valid(new DiffStoreHashEventQueue)))
        var summary = ParallelCRC.Summary(0.U(6.W), crc.seed)
        var nextSequence = sequence(core)
        var hasRecords = false.B
        var stamp = 0.U(16.W)
        lanes.zipWithIndex.foreach { case (lane, index) =>
          val store = lane.bits.asInstanceOf[DiffStoreEventQueue]
          val incoming = summarize(store, lane.valid)
          val nonempty = incoming.count.orR
          summary = crc.combine(summary, incoming, index * MaxRecords, MaxRecords)
          when(nonempty && !hasRecords) { hash.bits.instr_begin := nextSequence }
          when(nonempty) {
            hash.bits.instr_end := nextSequence
          }
          stamp = Mux(nonempty, Mux(hasRecords, latestStamp(stamp, store.stamp), store.stamp), stamp)
          hasRecords = hasRecords || nonempty
          nextSequence = Mux(lane.valid, nextSequence + 1.U, nextSequence)
        }
        when(accepted) { sequence(core) := nextSequence }
        hash.valid := hasRecords
        hash.bits.valid := hasRecords
        hash.bits.coreid := core.U
        hash.bits.hash_lo := summary.crc
        hash.bits.record_count := summary.count
        hash.bits.stamp := stamp
        hash
      }
      .toSeq
    MixedVecInit(bundles.filterNot(_.bits.isInstanceOf[DiffStoreEventQueue]).toSeq ++ hashes)
  }
}
