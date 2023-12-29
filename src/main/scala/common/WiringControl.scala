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

// Wrapper for the Chisel wiring utils.
private object WiringControl {
  def addSource(data: Data, name: String): Unit = BoringUtils.addSource(data, s"difftest_$name")

  def addSink(data: Data, name: String): Unit = BoringUtils.addSink(data, s"difftest_$name")
}

private class WiringInfo(val dataType: Data, val name: String) {
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
  def toPendingTuple: Option[(Boolean, Data, String)] = {
    if (isPending) {
      Some((nSources == 0, dataType, name))
    }
    else {
      None
    }
  }
}

object DifftestWiring {
  private val wires = scala.collection.mutable.ListBuffer.empty[WiringInfo]
  private def getWire(data: Data, name: String): WiringInfo = {
    wires.find(_.name == name).getOrElse({
      val info = new WiringInfo(chiselTypeOf(data), name)
      wires.addOne(info)
      info
    })
  }

  def addSource(data: Data, name: String): Data = {
    getWire(data, name).setSource()
    WiringControl.addSource(data, name)
    data
  }

  def addSink(data: Data, name: String): Data = {
    getWire(data, name).addSink()
    WiringControl.addSink(data, name)
    data
  }

  def isEmpty: Boolean = !hasPending
  def hasPending: Boolean = wires.exists(_.isPending)
  def getPending: Seq[(Boolean, Data, String)] = wires.flatMap(_.toPendingTuple).toSeq

  def createPendingWires(): Seq[Data] = {
    getPending.map{ case (isSource, dataType, name) =>
      val target = WireInit(0.U.asTypeOf(dataType)).suggestName(name)
      if (isSource) {
        addSource(target, name)
      }
      else {
        addSink(target, name)
      }
    }
  }

  def createExtraIOs(flipped: Boolean = false): Seq[Data] = {
    getPending.map{ case (isSource, dataType, name) =>
      def do_direction(dt: Data): Data = if (isSource) Input(dt) else Output(dt)
      def do_flip(dt: Data): Data = if (flipped) Flipped(dt) else dt
      IO(do_flip(do_direction(dataType))).suggestName(name)
    }
  }

  def createAndConnectExtraIOs(): Seq[Data] = {
    createExtraIOs().zip(createPendingWires()).map{ case (io, wire) =>
      io <> wire
      io
    }
  }
}
