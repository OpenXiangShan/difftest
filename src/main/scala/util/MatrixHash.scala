package difftest.util

import chisel3._
import chisel3.util._
import difftest.{DiffAmuHashEvent, DiffAmuWritebackEvent, DifftestModule}

/** CRC-32/ISCSI (CRC32C) of the canonical accepted matrix-register image.
  *
  * For logical register byte i, encode a written byte as s(i) = 0x100 | data(i), or zero if unwritten.
  * Canonical stream: descending logical byte offsets, symbol bits 8 down to 0,
  * packed into byte bits 0 up to 7 for the reflected input convention.
  * The ninth bit distinguishes a written zero from an unwritten byte.
  * Polynomial: 0x1edc6f41; init/xorout all ones; refin/refout true.
  *
  * NEMU uses the same canonical stream, including unwritten register capacity.
  * Hardware hashes each bank fragment, multiplies by its position coefficient, and XORs contributions.
  * Each defined byte must be written once per instruction; disjoint masks and arbitrary order are supported.
  */
object MatrixHash {
  private val hashWidth = 32
  private val symbolWidth = 9
  private val polynomial = (BigInt(1) << hashWidth) | BigInt("1edc6f41", 16)

  private def xorTree(values: Seq[UInt]): UInt = {
    require(values.nonEmpty)
    if (values.size == 1) values.head
    else {
      val (left, right) = values.splitAt(values.size / 2)
      xorTree(left) ^ xorTree(right)
    }
  }

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
  def apply(banks: Int, bankBytes: Int, regBytes: Int, rowBytes: Int): DiffAmuWritebackEvent =
    apply(banks, bankBytes, regBytes, rowBytes, ports = 1).head

  // Port zero owns instruction metadata and finish. Other ports only contribute
  // accepted writes to that same instruction; they never complete independently.
  def apply(banks: Int, bankBytes: Int, regBytes: Int, rowBytes: Int, ports: Int): Vec[DiffAmuWritebackEvent] = {
    val module = Module(new MatrixHash(banks, bankBytes, regBytes, rowBytes, ports))
    val writes = Wire(Vec(ports, new DiffAmuWritebackEvent(banks, bankBytes / 8)))
    module.io.in := writes
    DifftestModule(new DiffAmuHashEvent) := module.io.out
    writes
  }
}

/** One instruction's write stream per instance, with concurrent write beats accepted per cycle and no backpressure.
  * A beat may write several banks. `valid && finish` closes the instruction, including that beat's writes.
  * Instructions may be back-to-back, but their write streams must not interleave within this instance.
  *
  * Pipeline: fragment/address -> partial products -> shared contribution -> accumulation/completion.
  * Output valid and metadata are delayed by four registers from the input finish beat.
  */
class MatrixHash(banks: Int, bankBytes: Int, regBytes: Int, rowBytes: Int, ports: Int = 1) extends Module {
  import MatrixHash._

  require(ports > 0)
  require(bankBytes % 8 == 0 && rowBytes % bankBytes == 0)
  require(regBytes % (banks * rowBytes) == 0)
  private val entriesPerBank = regBytes / (banks * bankBytes)
  private val chunksPerRow = rowBytes / bankBytes
  private val contributionLatency = 3
  private val completionLatency = contributionLatency + 1
  require(entriesPerBank > 1 && entriesPerBank <= 256 && isPow2(entriesPerBank))

  val io = IO(new Bundle {
    val in = Input(Vec(ports, new DiffAmuWritebackEvent(banks, bankBytes / 8)))
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

  // Standard CRC initialization is an affine correction over this fixed-size image.
  private val allOnes = (BigInt(1) << hashWidth) - 1
  private val initialRemainder = (0 until symbolWidth * regBytes)
    .foldLeft(allOnes)((value, _) => constantRemainder(value << 1))

  val products = (0 until ports).flatMap { port =>
    val writes = io.in(port)
    (0 until banks).map { bank =>
      val active = writes.valid && writes.bankValid(bank)

      // Stage 1: encode this beat's bytes and register their logical bank address.
      // Reverse before Cat so word 0, then byte 0, occupies the least-significant bits.
      val data = writes.data
        .slice(bank * bankBytes / 8, (bank + 1) * bankBytes / 8)
        .reverse
        .reduce(Cat(_, _))
      val bytes = (0 until bankBytes).map { byte =>
        Mux(
          active && writes.bankMask.get(bank)(byte),
          Cat(1.U(1.W), data(byte * 8 + 7, byte * 8)),
          0.U(symbolWidth.W),
        )
      }
      // CRC input bits enter through the feedback polynomial: include the x^32 factor.
      val fragment = RegNext(
        reducePolynomial(
          bytes.reverse.reduce(Cat(_, _)) << hashWidth,
          symbolWidth * bankBytes + hashWidth,
        )
      )
      val address = RegNext(writes.bankAddr(bank)(log2Ceil(entriesPerBank) - 1, 0))

      // Banks interleave logical rows; consecutive chunks of a row stay in the same bank.
      // Lookup after the address register avoids registering the full coefficient.
      // Address arithmetic uses Scala Ints to build constants, not hardware dividers.
      val coefficients = VecInit((0 until entriesPerBank).map { addr =>
        val row = (addr / chunksPerRow) * banks + bank
        val chunkInRow = addr % chunksPerRow
        val offset = row * rowBytes + chunkInRow * bankBytes
        positions(offset / bankBytes).U(hashWidth.W)
      })
      val coefficient = coefficients(address)
      when(active) {
        assert(writes.bankAddr(bank) < entriesPerBank.U, "Matrix hash address exceeds register capacity")
      }

      // Stage 2: three 16x16 carry-less products form a 32x32 Karatsuba multiplication.
      val lo = RegNext(carrylessMultiply(fragment(15, 0), coefficient(15, 0)))
      val hi = RegNext(carrylessMultiply(fragment(31, 16), coefficient(31, 16)))
      val mid =
        RegNext(carrylessMultiply(fragment(15, 0) ^ fragment(31, 16), coefficient(15, 0) ^ coefficient(31, 16)))

      lo.pad(64) ^ ((mid ^ lo ^ hi) << 16).pad(64) ^ (hi << 32)
    }
  }

  // Stage 3: reduction is linear, so all products share one reduction and register.
  // Balance the cross-port/bank XOR to keep its depth logarithmic in the number of products.
  val contribution = RegNext(reducePolynomial(xorTree(products), 64))

  // Count accepted byte writes (not unique addresses) and align with the stage-3 contribution.
  // This catches extra writes even when identical polynomial contributions cancel under XOR.
  for (port <- 1 until ports) {
    assert(!io.in(port).finish, "Matrix hash finish belongs to port zero")
  }
  for {
    left <- 0 until ports
    right <- left + 1 until ports
    bank <- 0 until banks
  } {
    val a = io.in(left)
    val b = io.in(right)
    when(
      a.valid && b.valid && a.bankValid(bank) && b.bankValid(bank) &&
        a.bankAddr(bank) === b.bankAddr(bank)
    ) {
      assert(!(a.bankMask.get(bank) & b.bankMask.get(bank)).orR, "Concurrent matrix hash writes must not overlap bytes")
    }
  }
  val count = PopCount(VecInit((0 until ports).flatMap { port =>
    (0 until banks).flatMap { bank =>
      (0 until bankBytes).map { byte =>
        io.in(port).valid && io.in(port).bankValid(bank) && io.in(port).bankMask.get(bank)(byte)
      }
    }
  }))
  val beatCount = ShiftRegister(count, contributionLatency)
  val valid = ShiftRegister(io.in.map(_.valid).reduce(_ || _), contributionLatency, false.B, true.B)
  val finish = ShiftRegister(io.in(0).valid && io.in(0).finish, contributionLatency, false.B, true.B)

  // Stage 4: accumulate this instruction's contributions. Resetting valid/finish flushes the
  // pipeline; fragment/product registers need no reset because invalid beats cannot update state.
  val hash = RegInit(0.U(hashWidth.W))
  val byteCount = RegInit(0.U(32.W))
  val nextHash = hash ^ contribution
  val nextCount = byteCount + beatCount

  // Capture nextHash/nextCount on finish, so final-beat writes are included in the completion.
  // Clear the accumulator on that same edge to accept the next instruction without a bubble.
  when(valid) {
    hash := Mux(finish, 0.U, nextHash)
    byteCount := Mux(finish, 0.U, nextCount)
  }
  io.out.valid := RegNext(finish, false.B)
  io.out.pc := ShiftRegister(io.in(0).pc, completionLatency)
  io.out.coreid := ShiftRegister(io.in(0).coreid, completionLatency)
  io.out.index := ShiftRegister(io.in(0).index, completionLatency)
  val normalState = (nextHash ^ initialRemainder.U(hashWidth.W))(hashWidth - 1, 0)
  val checksum = Reverse(normalState) ^ allOnes.U(hashWidth.W)
  io.out.hashLo := RegEnable(checksum, finish)
  io.out.byteCount := RegEnable(nextCount, finish)
}
