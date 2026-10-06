package difftest.common

import chisel3._
import chisel3.util._
import difftest.{DiffAmuFinishEvent, DiffAmuHashEvent, DifftestModule}

/** A position-bound fingerprint of accepted matrix-register writes, before Gateway/FPGA transport.
  *
  * For logical register byte i, encode a written byte as s(i) = 0x100 | data(i), or zero if unwritten.
  * The hash is XOR_i(s(i) * x^(9*i)) modulo P(x) = x^128 + x^7 + x^2 + x + 1, using carry-less
  * polynomial arithmetic. The ninth bit distinguishes a written zero from an unwritten byte.
  * This is a linear error-detection fingerprint, not a cryptographic digest.
  *
  * NEMU computes the same remainder by visiting logical bytes from high to low. The hardware instead
  * hashes each bank fragment, multiplies by its position coefficient, and XORs the contributions.
  * Each defined byte must be written once per instruction; disjoint masks and arbitrary order are supported.
  */
object MatrixHash {
  private val hashWidth = 128
  private val symbolWidth = 9
  private val polynomial = (BigInt(1) << hashWidth) | 0x87

  // Elaboration-time polynomial division: BigInt calculations only determine constants and wiring.
  private def constantRemainder(value: BigInt): BigInt = {
    var result = value
    while (result.bitLength > hashWidth) {
      result ^= polynomial << (result.bitLength - hashWidth - 1)
    }
    result
  }

  // Runtime polynomial reduction is a fixed XOR network. Input bit i contributes x^i mod P;
  // each output bit XORs the input bits whose precomputed remainders contain that bit.
  private def reducePolynomial(value: UInt, width: Int): UInt = {
    val powers = (0 until width).map(i => constantRemainder(BigInt(1) << i))
    VecInit((0 until hashWidth).map { bit =>
      powers.indices
        .filter(i => powers(i).testBit(bit))
        .map(value(_))
        .reduce(_ ^ _)
    }).asUInt
  }

  // Full, unreduced carry-less product. Recursion expands at elaboration time into XOR/AND logic;
  // this helper adds no registers and does not infer ordinary integer/DSP multiplication.
  private def carrylessMultiply(a: UInt, b: UInt): UInt = {
    val width = a.getWidth
    require(width == b.getWidth && isPow2(width))
    if (width <= 8) {
      // Each set bit of b selects a shifted copy of a; summation in GF(2) is XOR.
      (0 until width)
        .map(i => Mux(b(i), (a << i).pad(2 * width), 0.U((2 * width).W)))
        .reduce(_ ^ _)
    } else {
      // Karatsuba: a = aLo + x^half*aHi, b = bLo + x^half*bHi.
      // The cross term aLo*bHi + aHi*bLo is mid XOR lo XOR hi.
      val half = width / 2
      val lo = carrylessMultiply(a(half - 1, 0), b(half - 1, 0))
      val hi = carrylessMultiply(a(width - 1, half), b(width - 1, half))
      val mid = carrylessMultiply(a(half - 1, 0) ^ a(width - 1, half), b(half - 1, 0) ^ b(width - 1, half))
      lo.pad(2 * width) ^ ((mid ^ lo ^ hi) << half).pad(2 * width) ^ (hi << width)
    }
  }

  // Callers drive the returned accepted-write bundle; only the compact hash enters the gateway.
  def apply(banks: Int, bankBytes: Int, regBytes: Int, rowBytes: Int): DiffAmuFinishEvent = {
    val module = Module(new MatrixHash(banks, bankBytes, regBytes, rowBytes))
    val writes = Wire(new DiffAmuFinishEvent(banks, bankBytes / 8))
    module.io.in := writes
    DifftestModule(new DiffAmuHashEvent) := module.io.out
    writes
  }
}

/** One instruction's write stream per instance, with one beat accepted per cycle and no backpressure.
  * A beat may write several banks. `valid && finish` closes the instruction, including that beat's writes.
  * Instructions may be back-to-back, but their write streams must not interleave within this instance.
  *
  * Pipeline: fragment/coefficient -> partial products -> bank contribution -> accumulation/completion.
  * Output valid and metadata are delayed by four registers from the input finish beat.
  */
class MatrixHash(banks: Int, bankBytes: Int, regBytes: Int, rowBytes: Int) extends Module {
  import MatrixHash._

  require(bankBytes % 8 == 0 && rowBytes % bankBytes == 0)
  require(regBytes % (banks * rowBytes) == 0)
  private val entriesPerBank = regBytes / (banks * bankBytes)
  private val chunksPerRow = rowBytes / bankBytes
  private val contributionLatency = 3
  private val completionLatency = contributionLatency + 1
  require(entriesPerBank > 1 && entriesPerBank <= 256 && isPow2(entriesPerBank))

  val io = IO(new Bundle {
    val in = Input(new DiffAmuFinishEvent(banks, bankBytes / 8))
    val out = Output(new DiffAmuHashEvent)
  })

  // Constant table: positions(chunk) = x^(9 * bankBytes * chunk) mod P.
  // Advancing by one bank-sized chunk preserves gaps for masked/unwritten logical bytes.
  val positions = Iterator
    .iterate(BigInt(1)) { value =>
      (0 until symbolWidth * bankBytes).foldLeft(value)((v, _) => constantRemainder(v << 1))
    }
    .take(regBytes / bankBytes)
    .toVector

  val contributions = (0 until banks).map { bank =>
    val active = io.in.valid && io.in.bankValid(bank)

    // Stage 1: encode this beat's bytes and select their logical position coefficient.
    // Reverse before Cat so word 0, then byte 0, occupies the least-significant bits.
    val data = io.in.data
      .slice(bank * bankBytes / 8, (bank + 1) * bankBytes / 8)
      .reverse
      .reduce(Cat(_, _))
    val bytes = (0 until bankBytes).map { byte =>
      Mux(
        active && io.in.bankMask(bank)(byte),
        Cat(1.U(1.W), data(byte * 8 + 7, byte * 8)),
        0.U(symbolWidth.W),
      )
    }
    val fragment = RegNext(reducePolynomial(bytes.reverse.reduce(Cat(_, _)), symbolWidth * bankBytes))

    // Banks interleave logical rows; consecutive chunks of a row stay in the same bank.
    // All address arithmetic here uses Scala Ints to build constants, not hardware dividers.
    val coefficients = VecInit((0 until entriesPerBank).map { addr =>
      val row = (addr / chunksPerRow) * banks + bank
      val chunkInRow = addr % chunksPerRow
      val offset = row * rowBytes + chunkInRow * bankBytes
      positions(offset / bankBytes).U(hashWidth.W)
    })
    val coefficient = RegNext(coefficients(io.in.bankAddr(bank)(log2Ceil(entriesPerBank) - 1, 0)))
    when(active) {
      assert(io.in.bankAddr(bank) < entriesPerBank.U, "Matrix hash address exceeds register capacity")
    }

    // Stage 2: three 64x64 carry-less products form a 128x128 Karatsuba multiplication.
    val lo = RegNext(carrylessMultiply(fragment(63, 0), coefficient(63, 0)))
    val hi = RegNext(carrylessMultiply(fragment(127, 64), coefficient(127, 64)))
    val mid = RegNext(carrylessMultiply(fragment(63, 0) ^ fragment(127, 64), coefficient(63, 0) ^ coefficient(127, 64)))

    // Stage 3: recombine the 256-bit product and reduce modulo P.
    // (fragment * x^(9*offset)) mod P binds the local bytes to their register positions.
    RegNext(reducePolynomial(lo.pad(256) ^ ((mid ^ lo ^ hi) << 64).pad(256) ^ (hi << 128), 256))
  }

  // Count accepted byte writes (not unique addresses) and align with the stage-3 contributions.
  // This catches extra writes even when identical polynomial contributions cancel under XOR.
  val count = PopCount(VecInit((0 until banks).flatMap { bank =>
    (0 until bankBytes).map(byte => io.in.valid && io.in.bankValid(bank) && io.in.bankMask(bank)(byte))
  }))
  val beatCount = ShiftRegister(count, contributionLatency)
  val valid = ShiftRegister(io.in.valid, contributionLatency, false.B, true.B)
  val finish = ShiftRegister(io.in.valid && io.in.finish, contributionLatency, false.B, true.B)

  // Stage 4: accumulate this instruction's contributions. Resetting valid/finish flushes the
  // pipeline; fragment/product registers need no reset because invalid beats cannot update state.
  val hash = RegInit(0.U(hashWidth.W))
  val byteCount = RegInit(0.U(32.W))
  val nextHash = hash ^ contributions.reduce(_ ^ _)
  val nextCount = byteCount + beatCount

  // Capture nextHash/nextCount on finish, so final-beat writes are included in the completion.
  // Clear the accumulator on that same edge to accept the next instruction without a bubble.
  when(valid) {
    hash := Mux(finish, 0.U, nextHash)
    byteCount := Mux(finish, 0.U, nextCount)
  }
  io.out.valid := RegNext(finish, false.B)
  io.out.pc := ShiftRegister(io.in.pc, completionLatency)
  io.out.coreid := ShiftRegister(io.in.coreid, completionLatency)
  io.out.index := ShiftRegister(io.in.index, completionLatency)
  io.out.hashLo := RegEnable(nextHash(63, 0), finish)
  io.out.hashHi := RegEnable(nextHash(127, 64), finish)
  io.out.byteCount := RegEnable(nextCount, finish)
}
