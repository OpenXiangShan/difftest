/***************************************************************************************
 * Copyright (c) 2025-2026 Beijing Institute of Open Source Chip
 * Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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

class AXI4LiteBundleA(val addrWidth: Int) extends Bundle {
  val addr = UInt(addrWidth.W)
}

class AXI4LiteBundleAR(addrWidth: Int) extends AXI4LiteBundleA(addrWidth)

class AXI4LiteBundleAW(addrWidth: Int) extends AXI4LiteBundleA(addrWidth)

class AXI4LiteBundleW(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val strb = UInt((dataWidth / 8).W)
}

class AXI4LiteBundleB extends Bundle {
  val resp = UInt(2.W)
}

class AXI4LiteBundleR(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val resp = UInt(2.W)
}

class AXI4LiteBundle(val addrWidth: Int, val dataWidth: Int) extends Bundle {
  val aw = Decoupled(new AXI4LiteBundleAW(addrWidth))
  val w = Decoupled(new AXI4LiteBundleW(dataWidth))
  val b = Flipped(Decoupled(new AXI4LiteBundleB))
  val ar = Decoupled(new AXI4LiteBundleAR(addrWidth))
  val r = Flipped(Decoupled(new AXI4LiteBundleR(dataWidth)))
}

class VerilogAXI4LiteRecord(val addrWidth: Int, val dataWidth: Int) extends Record {
  private val axilite = new AXI4LiteBundle(addrWidth, dataWidth)
  private def traverseAndMap(data: (String, Data)): SeqMap[String, Data] = {
    data match {
      case (name, node: Bundle) =>
        SeqMap.from(
          node.elements.toSeq.flatMap(traverseAndMap).map { case (nodeName, nodeData) =>
            s"${name.replace("bits", "")}$nodeName" -> nodeData
          }
        )
      case (name, node) => SeqMap(name -> node)
    }
  }
  private val outputPattern = "^(aw|w|ar).*".r
  private val elems = traverseAndMap("" -> axilite).map { case (name, node) =>
    val directed = name match {
      case outputPattern(_) => Output(node)
      case _                => Input(node)
    }
    name match {
      case s"${_}ready" => name -> Flipped(directed)
      case _            => name -> directed
    }
  }

  override def elements: SeqMap[String, Data] = elems
}

object VerilogAXI4LiteRecord {
  private val elementsMap: Seq[(VerilogAXI4LiteRecord, AXI4LiteBundle) => (Data, Data)] = {
    val names = new VerilogAXI4LiteRecord(1, 8).elements.keys
    val pattern = "^(aw|w|b|ar|r)(.*)".r
    names.map { name => (verilog: VerilogAXI4LiteRecord, chisel: AXI4LiteBundle) =>
      val (channel, signal) = name match {
        case pattern(prefix, suffix) =>
          chisel.elements(prefix).asInstanceOf[Record] -> suffix
        case _ => throw new IllegalArgumentException(s"unexpected AXI4-Lite port name: $name")
      }
      val chiselData = channel.elements.applyOrElse(
        signal,
        channel.elements("bits").asInstanceOf[Record].elements,
      )
      verilog.elements(name) -> chiselData
    }.toSeq
  }

  implicit val recordToBundle: DataView[VerilogAXI4LiteRecord, AXI4LiteBundle] =
    DataView[VerilogAXI4LiteRecord, AXI4LiteBundle](
      record => new AXI4LiteBundle(record.addrWidth, record.dataWidth),
      elementsMap: _*
    )

  implicit val bundleToRecord: DataView[AXI4LiteBundle, VerilogAXI4LiteRecord] =
    recordToBundle.invert(bundle => new VerilogAXI4LiteRecord(bundle.addrWidth, bundle.dataWidth))
}
