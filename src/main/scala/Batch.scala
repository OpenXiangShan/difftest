/***************************************************************************************
 * Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest.batch

import chisel3._
import chisel3.util._
import chisel3.util.experimental.BoringUtils
import difftest._

import scala.collection.mutable.ListBuffer

object Batch {
  private val instances = ListBuffer.empty[DifftestBundle]

  def apply[T <: DifftestBundle](gen: T): T = {
    register(WireInit(0.U.asTypeOf(gen)))
  }

  def register[T <: DifftestBundle](gen: T): T = {
    BoringUtils.addSource(gen, s"batch_${instances.length}")
    instances += gen
    gen
  }

  def collect(): Seq[String] = {
    Module(new BatchEndpoint(instances.toSeq))
    Seq("CONFIG_DIFFTEST_BATCH")
  }
}

class BatchEndpoint(signals: Seq[DifftestBundle]) extends Module {
  val in = WireInit(0.U.asTypeOf(MixedVec(signals.map(_.cloneType))))
  for ((data, batch_id) <- in.zipWithIndex) {
    BoringUtils.addSink(data, s"batch_$batch_id")
  }

  val batch_size = 32
  val batch_data = Mem(batch_size, in.cloneType)
  val batch_ptr = RegInit(0.U(log2Ceil(batch_size).W))
  batch_ptr := batch_ptr + 1.U
  batch_data(batch_ptr) := in

  // Sync the data when batch is completed
  val do_batch_sync = batch_ptr === (batch_size - 1).U
  // TODO: implement the sync logic for the batch data
  dontTouch(do_batch_sync)
  dontTouch(WireInit(batch_data(0)))
}
