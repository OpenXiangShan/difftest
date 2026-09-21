/***************************************************************************************
 * Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
 *
 * DiffTest is licensed under Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 ***************************************************************************************/

package difftest

import chisel3._
import chisel3.util._

/** Small, deterministic hash primitives shared by the hardware store compressor. */
object StoreHash {
  val GroupSize = 64
  val HashWidth = 64

  private val ArxSeed0 = BigInt("243f6a8885a308d3", 16).U(HashWidth.W)
  private val ArxSeed1 = BigInt("13198a2e03707344", 16).U(HashWidth.W)
  private val Golden = BigInt("9e3779b97f4a7c15", 16).U(HashWidth.W)
  private val MixMul0 = BigInt("bf58476d1ce4e5b9", 16).U(HashWidth.W)
  private val MixMul1 = BigInt("94d049bb133111eb", 16).U(HashWidth.W)
  private val H0Add = BigInt("a4093822299f31d0", 16).U(HashWidth.W)
  private val H1Xor = BigInt("082efa98ec4e6c89", 16).U(HashWidth.W)
  private val H0Mul = BigInt("9e3779b185ebca87", 16).U(HashWidth.W)
  private val H1Mul = BigInt("c2b2ae3d27d4eb4f", 16).U(HashWidth.W)
  private val CrcPolynomial = BigInt("42f0e1eba9ea3693", 16).U(HashWidth.W)

  val arxSeed0: UInt = ArxSeed0
  val arxSeed1: UInt = ArxSeed1

  private def lowMul(lhs: UInt, rhs: UInt): UInt = (lhs * rhs)(HashWidth - 1, 0)

  def rol(value: UInt, amount: Int): UInt = {
    require(amount > 0 && amount < HashWidth)
    Cat(value(HashWidth - amount - 1, 0), value(HashWidth - 1, HashWidth - amount))
  }

  /** One ARX update. It matches difftest_store_hash_update in NEMU. */
  def arxUpdate(h0: UInt, h1: UInt, count: UInt, addr: UInt, data: UInt, mask: UInt): (UInt, UInt) = {
    val value0 = addr ^ rol(data, 23) ^ Cat(mask, 0.U(56.W))
    val value1 = value0 +% lowMul(count, Golden)
    val mixed0 = value1 ^ (value1 >> 30)
    val mixed1 = lowMul(mixed0, MixMul0)
    val mixed2 = mixed1 ^ (mixed1 >> 27)
    val mixed3 = lowMul(mixed2, MixMul1)
    val value = mixed3 ^ (mixed3 >> 31)
    val next0 = lowMul(rol(h0 ^ (value +% H0Add), 17), H0Mul)
    val next1 = lowMul(rol(h1 +% (value ^ H1Xor), 29), H1Mul)
    (next0, next1)
  }

  /** MSB-first CRC64/ECMA update over one 64-bit word. */
  def crcUpdate(state: UInt, word: UInt): UInt = {
    (0 until HashWidth).foldLeft(state) { case (crc, bit) =>
      val shifted = Cat(crc(HashWidth - 2, 0), 0.U(1.W))
      Mux(crc(HashWidth - 1) ^ word(HashWidth - 1 - bit), shifted ^ CrcPolynomial, shifted)
    }
  }

  private class Record extends Bundle {
    val valid = Bool()
    val addr = UInt(64.W)
    val data = UInt(64.W)
    val mask = UInt(8.W)
  }

  private def maskExpand(mask: UInt): UInt = {
    Cat((0 until 8).reverse.map(byte => Fill(8, mask(byte))))
  }

  private def vectorMask(store: DiffStoreEvent, element: Int): UInt = {
    // Generate the same element windows as the software checker. The fixed
    // byte loop keeps the implementation independent of the runtime EEW.
    val eew = Mux(store.eew === 0.U, 1.U, store.eew)
    val eewOffset = store.offset % eew
    val start = eew.asSInt * element.S(8.W) + eewOffset.asSInt
    val end = start + eew.asSInt
    val bytes = (0 until 16).map(byte => start <= byte.S && byte.S < end)
    store.mask & Cat(bytes.reverse)
  }

  private def records(store: DiffStoreEvent): Seq[Record] = {
    val vectorRecords = (-1 to 15).flatMap { element =>
      val flowMask = vectorMask(store, element)
      Seq(
        (flowMask(7, 0), store.addr, store.data),
        (flowMask(15, 8), store.addr +% 8.U, store.highData),
      ).map { case (mask, addr, data) =>
        val record = WireInit(0.U.asTypeOf(new Record))
        record.valid := store.valid && mask.orR
        record.addr := addr
        record.data := data & maskExpand(mask)
        record.mask := mask
        record
      }
    }

    val scalarRecords = Seq(store.mask(7, 0), store.mask(15, 8)).zipWithIndex.map { case (mask, index) =>
      val record = WireInit(0.U.asTypeOf(new Record))
      record.valid := store.valid && mask.orR
      record.addr := store.addr +% (index * 8).U
      record.data := (if (index == 0) store.data else store.highData) & maskExpand(mask)
      record.mask := mask
      record
    }

    val lineRecords = (0 until 8).map { index =>
      val record = WireInit(0.U.asTypeOf(new Record))
      record.valid := store.valid
      record.addr := Cat(store.addr(63, 6), 0.U(6.W)) +% (index * 8).U
      record.data := 0.U
      record.mask := "hff".U
      record
    }

    vectorRecords.zipWithIndex.map { case (record, index) =>
      val selected = Wire(new Record)
      selected := Mux(store.vecNeedSplit, record, 0.U.asTypeOf(new Record))
      selected
    } ++ lineRecords.zipWithIndex.map { case (record, index) =>
      val selected = Wire(new Record)
      selected := Mux(store.wLine && !store.vecNeedSplit, record, 0.U.asTypeOf(new Record))
      selected
    } ++ scalarRecords.map { record =>
      val selected = Wire(new Record)
      selected := Mux(!store.wLine && !store.vecNeedSplit, record, 0.U.asTypeOf(new Record))
      selected
    }
  }
}

/**
  * Compresses all store lanes of one core. A hash event is emitted only after
  * the final canonical record of the 64th store instruction in a group.
  */
class StoreHash(val laneCount: Int) extends Module {
  require(laneCount > 0)

  val in = IO(Input(Vec(laneCount, Valid(new DiffStoreEvent))))
  val out = IO(Output(Valid(new DiffStoreHashEvent)))

  val h0Reg = RegInit(StoreHash.arxSeed0)
  val h1Reg = RegInit(StoreHash.arxSeed1)
  val recordCountReg = RegInit(0.U(16.W))
  val instrSeqReg = RegInit(0.U(64.W))
  val instrBeginReg = RegInit(0.U(64.W))
  val instrEndReg = RegInit(0.U(64.W))
  val groupIdReg = RegInit(0.U(64.W))

  var h0 = h0Reg
  var h1 = h1Reg
  var recordCount = recordCountReg
  var instrSeq = instrSeqReg
  var instrBegin = instrBeginReg
  var instrEnd = instrEndReg
  var groupId = groupIdReg
  var flushed = false.B
  var flushH0 = 0.U(64.W)
  var flushH1 = 0.U(64.W)
  var flushRecordCount = 0.U(16.W)
  var flushInstrBegin = 0.U(64.W)
  var flushInstrEnd = 0.U(64.W)
  var flushGroupId = 0.U(64.W)
  val coreid = in.map(_.bits.coreid).head

  in.foreach { store =>
    val storeRecords = StoreHash.records(store.bits)
    val storeSeq = instrSeq
    instrSeq = Mux(store.bits.valid, instrSeq + 1.U, instrSeq)

    storeRecords.zipWithIndex.foreach { case (record, recordIndex) =>
      val nextH = StoreHash.arxUpdate(h0, h1, recordCount, record.addr, record.data, record.mask)
      val recordH0 = Mux(record.valid, nextH._1, h0)
      val recordH1 = Mux(record.valid, nextH._2, h1)
      val recordCountNext = Mux(record.valid, recordCount + 1.U, recordCount)
      val beginNext = Mux(record.valid && recordCount === 0.U, storeSeq, instrBegin)
      val endNext = Mux(record.valid, storeSeq, instrEnd)
      val laterRecordValid = storeRecords.drop(recordIndex + 1).map(_.valid).foldLeft(false.B)(_ || _)
      val isLastRecord = record.valid && !laterRecordValid
      val complete = isLastRecord && endNext - beginNext + 1.U >= StoreHash.GroupSize.U

      val emit = complete && !flushed
      flushH0 = Mux(emit, recordH0, flushH0)
      flushH1 = Mux(emit, recordH1, flushH1)
      flushRecordCount = Mux(emit, recordCountNext, flushRecordCount)
      flushInstrBegin = Mux(emit, beginNext, flushInstrBegin)
      flushInstrEnd = Mux(emit, endNext, flushInstrEnd)
      flushGroupId = Mux(emit, groupId, flushGroupId)
      flushed = flushed || complete

      h0 = Mux(complete, StoreHash.arxSeed0, recordH0)
      h1 = Mux(complete, StoreHash.arxSeed1, recordH1)
      recordCount = Mux(complete, 0.U, recordCountNext)
      instrBegin = Mux(complete, 0.U, beginNext)
      instrEnd = Mux(complete, 0.U, endNext)
      groupId = Mux(complete, groupId + 1.U, groupId)
    }
  }

  h0Reg := h0
  h1Reg := h1
  recordCountReg := recordCount
  instrSeqReg := instrSeq
  instrBeginReg := instrBegin
  instrEndReg := instrEnd
  groupIdReg := groupId

  out.valid := flushed
  out.bits.valid := flushed
  out.bits.coreid := coreid
  out.bits.index := 0.U
  out.bits.hash_lo := flushH0
  out.bits.hash_hi := flushH1
  out.bits.record_count := flushRecordCount
  out.bits.group_id := flushGroupId
  out.bits.instr_begin := flushInstrBegin
  out.bits.instr_end := flushInstrEnd
}
