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

object ParallelCRC {

  /** Zero-seed CRC and the number of fixed-size blocks absorbed. */
  case class Summary(count: UInt, crc: UInt)
}

/** MSB-first, unreflected CRC over fixed-size blocks, without a final XOR.
  * Each block is at least width bits. The polynomial omits its leading x^width
  * term. Matrices are calculated at
  * elaboration time; update, advance and combine emit only fixed XORs and muxes.
  *
  *   C' = rem(C * x^blockWidth xor data * x^width, G)
  */
class ParallelCRC(val width: Int, polynomial: BigInt, blockWidth: Int, maxBlocks: Int) {
  require(width > 0 && blockWidth >= width && maxBlocks > 0)
  require(polynomial > 0 && polynomial.bitLength <= width && polynomial.testBit(0))
  import ParallelCRC.Summary

  private val generator = (BigInt(1) << width) | polynomial
  private val payloadWidth = blockWidth - width
  val seed: UInt = 0.U(width.W)

  private def remainder(value: BigInt): BigInt = {
    var rem = value
    while (rem.bitLength > width) {
      rem ^= generator << (rem.bitLength - 1 - width)
    }
    rem
  }

  // The high width bits of data have the same coefficient as the old state.
  // XOR them first to reduce the input matrix from width+blockWidth to blockWidth.
  private val updateColumns = (0 until width).map { bit =>
    remainder(BigInt(1) << (blockWidth + bit))
  } ++ (0 until payloadWidth).map { bit =>
    remainder(BigInt(1) << (width + bit))
  }

  private def transform(inputs: UInt, columns: Seq[BigInt]): UInt = {
    Cat((0 until width).reverse.map { outputBit =>
      val mask = columns.zipWithIndex.foldLeft(BigInt(0)) { case (acc, (column, inputBit)) =>
        if (column.testBit(outputBit)) acc | (BigInt(1) << inputBit) else acc
      }
      (inputs & mask.U(columns.size.W)).xorR
    })
  }

  def update(state: UInt, data: UInt): UInt = {
    require(state.getWidth == width && data.getWidth == blockWidth)
    val feedback = state ^ data(blockWidth - 1, payloadWidth)
    val inputs = if (payloadWidth == 0) feedback else Cat(data(payloadWidth - 1, 0), feedback)
    transform(inputs, updateColumns)
  }

  // A^(2^bit): advance by that many zero blocks without adding data.
  private val advanceColumns = (0 until log2Ceil(maxBlocks + 1)).map { bit =>
    (0 until width).map { inputBit =>
      remainder(BigInt(1) << (blockWidth * (1 << bit) + inputBit))
    }
  }

  /** Advance a CRC by count zero blocks using conditional fixed transforms. */
  def advance(state: UInt, count: UInt, maxCount: Int): UInt = {
    require(maxCount >= 0 && maxCount <= maxBlocks)
    (0 until log2Ceil(maxCount + 1)).foldLeft(state) { (value, bit) =>
      Mux(count(bit), transform(value, advanceColumns(bit)), value)
    }
  }

  /** Concatenate summaries in order: A^right.count(left.crc) xor right.crc. */
  def combine(left: Summary, right: Summary, maxLeft: Int, maxRight: Int): Summary = {
    val countWidth = log2Ceil(maxLeft + maxRight + 1)
    val count = (left.count +& right.count).pad(countWidth)(countWidth - 1, 0)
    Summary(count, advance(left.crc, right.count, maxRight) ^ right.crc)
  }
}
