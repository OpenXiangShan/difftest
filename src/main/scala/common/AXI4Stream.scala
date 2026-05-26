/***************************************************************************************
 * Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
 * Copyright (c) 2025 Beijing Institute of Open Source Chip
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

package difftest.common
import chisel3._
import chisel3.experimental.dataview._
import chisel3.util._
import scala.collection.immutable.SeqMap

class AXI4StreamBundle(val dataWidth: Int) extends Bundle {
  require(dataWidth % 8 == 0, s"AXI-Stream data width must be byte-aligned, got $dataWidth")

  val data = UInt(dataWidth.W)
  val keep = UInt((dataWidth / 8).W)
  val last = Bool()
}

class AXI4Stream(val dataWidth: Int) extends DecoupledIO(new AXI4StreamBundle(dataWidth))

class VerilogAXI4StreamRecord(val dataWidth: Int) extends Record {
  require(dataWidth % 8 == 0, s"AXI-Stream data width must be byte-aligned, got $dataWidth")

  private val elems = SeqMap[String, Data](
    "tvalid" -> Output(Bool()),
    "tready" -> Input(Bool()),
    "tdata" -> Output(UInt(dataWidth.W)),
    "tkeep" -> Output(UInt((dataWidth / 8).W)),
    "tlast" -> Output(Bool()),
  )

  override def elements: SeqMap[String, Data] = elems
}

object VerilogAXI4StreamRecord {
  def typeOf(axis: AXI4Stream): VerilogAXI4StreamRecord =
    new VerilogAXI4StreamRecord(axis.dataWidth)

  implicit val recordToBundle: DataView[VerilogAXI4StreamRecord, AXI4Stream] =
    DataView[VerilogAXI4StreamRecord, AXI4Stream](
      record => new AXI4Stream(record.dataWidth),
      (record, axis) => record.elements("tvalid") -> axis.valid,
      (record, axis) => record.elements("tready") -> axis.ready,
      (record, axis) => record.elements("tdata") -> axis.bits.data,
      (record, axis) => record.elements("tkeep") -> axis.bits.keep,
      (record, axis) => record.elements("tlast") -> axis.bits.last,
    )

  implicit val bundleToRecord: DataView[AXI4Stream, VerilogAXI4StreamRecord] =
    recordToBundle.invert(axis => new VerilogAXI4StreamRecord(axis.dataWidth))
}
