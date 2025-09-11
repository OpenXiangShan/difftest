/***************************************************************************************
 * Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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
import chisel3.util.experimental.BoringUtils
import chisel3.reflect.DataMirror

// Wrapper for the Chisel wiring utils.
private object WiringControl {
  def addSource(data: Data, name: String): Unit = BoringUtils.addSource(data, s"difftest_$name")

  def addSink(data: Data, name: String): Unit = BoringUtils.addSink(data, s"difftest_$name")

  def tapAndRead[T <: Data](source: T): T = BoringUtils.tapAndRead(source)

  def bore[T <: Data](source: T): T = BoringUtils.bore(source)
}

private class WiringInfo(val data: Data, val name: String, val isHierarchical: Boolean) {
  private var nSources: Int = 0
  private var nSinks: Int = 0

  def setSource(): WiringInfo = {
    require(nSources == 0, s"$name already declared as a source")
    nSources += 1
    this
  }

  def addSink(): WiringInfo = {
    nSinks += 1
    this
  }

  def isPending: Boolean = nSources == 0 || nSinks == 0
  // (isSource, dataType, name)
  def toPendingTuple: Option[(Boolean, Data, String, Boolean)] = {
    if (isPending) {
      Some((nSources == 0, chiselTypeOf(data), name, isHierarchical))
    } else {
      None
    }
  }
}

object DifftestWiring {
  private val wires = scala.collection.mutable.ListBuffer.empty[WiringInfo]
  private def getWire(data: Data, name: String, isHierarchical: Boolean): WiringInfo = {
    wires.find(_.name == name).getOrElse {
      val info = new WiringInfo(data, name, isHierarchical)
      wires.addOne(info)
      info
    }
  }

  def addSource[T <: Data](data: T, name: String, isHierarchical: Boolean): T = {
    getWire(data, name, isHierarchical).setSource()
    data
  }

  def addSource[T <: Data](data: T, name: String): T = {
    addSource[T](data, name, false)
  }

  def addSink[T <: Data](data: T, name: String, isHierarchical: Boolean): T = {
    val info = getWire(data, name, isHierarchical).addSink()
    require(!info.isPending, s"[${info.name}]: Wiring requires addSource before addSink")
    if (DataMirror.isVisible(info.data)) {
      data := info.data
    } else if (isHierarchical) {
      data := WiringControl.tapAndRead(info.data)
    } else {
      data := WiringControl.bore(info.data)
    }
    data
  }

  def addSink[T <: Data](data: T, name: String): T = {
    addSink[T](data, name, false)
  }

  def isEmpty: Boolean = !hasPending
  def hasPending: Boolean = wires.exists(_.isPending)
  def getPending: Seq[(Boolean, Data, String, Boolean)] = wires.flatMap(_.toPendingTuple).toSeq

  def createPendingWires(): Seq[Data] = {
    getPending.map { case (isSource, dataType, name, isHierarchical) =>
      val target = WireInit(0.U.asTypeOf(dataType)).suggestName(name)
      if (isSource) {
        addSource(target, name, isHierarchical)
      } else {
        addSink(target, name, isHierarchical)
      }
    }
  }

  def createExtraIOs(flipped: Boolean = false): Seq[Data] = {
    getPending.map { case (isSource, dataType, name, _) =>
      def do_direction(dt: Data): Data = if (isSource) Input(dt) else Output(dt)
      def do_flip(dt: Data): Data = if (flipped) Flipped(dt) else dt
      IO(do_flip(do_direction(dataType))).suggestName(name)
    }
  }

  def createAndConnectExtraIOs(): Seq[Data] = {
    createExtraIOs().zip(createPendingWires()).map { case (io, wire) =>
      io <> wire
      io
    }
  }
}
