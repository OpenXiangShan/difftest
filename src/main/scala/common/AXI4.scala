/***************************************************************************************
 * Copyright (c) 2026 Beijing Institute of Open Source Chip
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

class AXI4BundleA(val addrWidth: Int, val idWidth: Int, val userWidth: Int) extends Bundle {
  val addr = UInt(addrWidth.W)
  val id = UInt(idWidth.W)
  val len = UInt(8.W)
  val size = UInt(3.W)
  val burst = UInt(2.W)
  val lock = Bool()
  val cache = UInt(4.W)
  val prot = UInt(3.W)
  val qos = UInt(4.W)
  val user = UInt(userWidth.W)
}

class AXI4BundleW(val dataWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val strb = UInt((dataWidth / 8).W)
  val last = Bool()
}

class AXI4BundleB(val idWidth: Int, val userWidth: Int) extends Bundle {
  val resp = UInt(2.W)
  val id = UInt(idWidth.W)
  val user = UInt(userWidth.W)
}

class AXI4BundleR(val dataWidth: Int, val idWidth: Int, val userWidth: Int) extends Bundle {
  val data = UInt(dataWidth.W)
  val resp = UInt(2.W)
  val last = Bool()
  val id = UInt(idWidth.W)
  val user = UInt(userWidth.W)
}

class AXI4Bundle(val addrWidth: Int, val dataWidth: Int, val idWidth: Int = 1, val userWidth: Int = 1) extends Bundle {
  val aw = Decoupled(new AXI4BundleA(addrWidth, idWidth, userWidth))
  val w = Decoupled(new AXI4BundleW(dataWidth))
  val b = Flipped(Decoupled(new AXI4BundleB(idWidth, userWidth)))
  val ar = Decoupled(new AXI4BundleA(addrWidth, idWidth, userWidth))
  val r = Flipped(Decoupled(new AXI4BundleR(dataWidth, idWidth, userWidth)))
}

object AXI4Bundle {
  def typeOf(record: Record): AXI4Bundle =
    AXI4RecordView(record).difftestType

  def fromRecord(record: Record, name: String = "axi"): AXI4Bundle = {
    val view = AXI4RecordView(record)
    val axi = Wire(view.difftestType).suggestName(name)
    connectRecord(axi, record)
    axi
  }

  def connectRecord(dst: Record, src: Record): Unit =
    AXI4RecordView(dst).connectFrom(AXI4RecordView(src))

  def connectFields(dst: Data, src: Data): Unit = {
    (dst, src) match {
      case (dstRecord: Record, srcRecord: Record) =>
        dstRecord.elements.foreach { case (name, dstField) =>
          srcRecord.elements.get(name) match {
            case Some(srcField) => connectFields(dstField, srcField)
            case None           => dstField := 0.U.asTypeOf(dstField)
          }
        }
      case _ =>
        dst := src.asUInt.asTypeOf(dst)
    }
  }

  private case class AXI4RecordView(axi: Record) {
    val aw: AXI4ChannelView = channel("aw")
    val w: AXI4ChannelView = channel("w")
    val b: AXI4ChannelView = channel("b")
    val ar: AXI4ChannelView = channel("ar")
    val r: AXI4ChannelView = channel("r")

    def difftestType: AXI4Bundle =
      new AXI4Bundle(
        fieldWidth(aw, "addr"),
        fieldWidth(w, "data"),
        fieldWidth(aw, "id"),
        fieldWidth(aw, "user"),
      )

    def connectFrom(src: AXI4RecordView): Unit = {
      connectChannel(aw, src.aw)
      connectChannel(w, src.w)
      connectChannel(src.b, b)
      connectChannel(ar, src.ar)
      connectChannel(src.r, r)
    }

    private def channel(name: String): AXI4ChannelView =
      axi.elements.get(name) match {
        case Some(channel: ReadyValidIO[_]) =>
          AXI4NestedChannelView(channel.asInstanceOf[ReadyValidIO[Data]])
        case Some(_) =>
          throw new IllegalArgumentException(s"AXI channel $name is not ReadyValidIO")
        case _ =>
          AXI4FlatChannelView(axi, name)
      }
  }

  private trait AXI4ChannelView {
    def valid: Bool
    def ready: Bool
    def bits: SeqMap[String, Data]
  }

  private case class AXI4NestedChannelView(channel: ReadyValidIO[Data]) extends AXI4ChannelView {
    def valid: Bool = channel.valid
    def ready: Bool = channel.ready
    def bits: SeqMap[String, Data] = channel.bits.asInstanceOf[Record].elements
  }

  private case class AXI4FlatChannelView(record: Record, name: String) extends AXI4ChannelView {
    def valid: Bool = boolField("valid")
    def ready: Bool = boolField("ready")
    def bits: SeqMap[String, Data] =
      SeqMap.from(record.elements.toSeq.flatMap { case (fieldName, field) =>
        val bitName = fieldName.drop(name.length)
        Option.when(fieldName.startsWith(name) && bitName != "valid" && bitName != "ready")(bitName -> field)
      })

    private def boolField(suffix: String): Bool =
      record.elements
        .getOrElse(
          s"$name$suffix",
          throw new IllegalArgumentException(s"AXI flat record is missing field $name$suffix"),
        )
        .asInstanceOf[Bool]
  }

  private def fieldWidth(channel: AXI4ChannelView, fieldName: String): Int =
    channel.bits.get(fieldName).map(_.getWidth.max(1)).getOrElse(1)

  private def connectChannel(dst: ReadyValidIO[Data], src: ReadyValidIO[Data]): Unit = {
    dst.valid := src.valid
    connectFields(dst.bits, src.bits)
    src.ready := dst.ready
  }

  private def connectChannel(dst: AXI4ChannelView, src: AXI4ChannelView): Unit = {
    dst.valid := src.valid
    dst.bits.foreach { case (name, dstField) =>
      src.bits.get(name) match {
        case Some(srcField) => connectFields(dstField, srcField)
        case None           => dstField := 0.U.asTypeOf(dstField)
      }
    }
    src.ready := dst.ready
  }
}

class VerilogAXI4Record(
  val addrWidth: Int,
  val dataWidth: Int,
  val idWidth: Int = 1,
  val userWidth: Int = 1,
) extends Record {
  private val axi = new AXI4Bundle(addrWidth, dataWidth, idWidth, userWidth)
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
  private val elems = traverseAndMap("" -> axi).map { case (name, node) =>
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

object VerilogAXI4Record {
  def typeOf(axi: AXI4Bundle): VerilogAXI4Record =
    new VerilogAXI4Record(axi.addrWidth, axi.dataWidth, axi.idWidth, axi.userWidth)

  private val elementsMap: Seq[(VerilogAXI4Record, AXI4Bundle) => (Data, Data)] = {
    val names = new VerilogAXI4Record(1, 8).elements.keys
    val pattern = "^(aw|w|b|ar|r)(.*)".r
    names.map { name => (verilog: VerilogAXI4Record, chisel: AXI4Bundle) =>
      val (channel, signal) = name match {
        case pattern(prefix, suffix) =>
          chisel.elements(prefix).asInstanceOf[Record] -> suffix
        case _ => throw new IllegalArgumentException(s"unexpected AXI4 port name: $name")
      }
      val chiselData = channel.elements.applyOrElse(
        signal,
        channel.elements("bits").asInstanceOf[Record].elements,
      )
      verilog.elements(name) -> chiselData
    }.toSeq
  }

  implicit val recordToBundle: DataView[VerilogAXI4Record, AXI4Bundle] =
    DataView[VerilogAXI4Record, AXI4Bundle](
      record => new AXI4Bundle(record.addrWidth, record.dataWidth, record.idWidth, record.userWidth),
      elementsMap: _*
    )

  implicit val bundleToRecord: DataView[AXI4Bundle, VerilogAXI4Record] =
    recordToBundle.invert(bundle =>
      new VerilogAXI4Record(bundle.addrWidth, bundle.dataWidth, bundle.idWidth, bundle.userWidth)
    )
}
