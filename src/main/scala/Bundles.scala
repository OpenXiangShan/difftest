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

package difftest

import chisel3._
import chisel3.util._

sealed trait HasValid {
  val valid = Bool()
}

sealed trait HasAddress { this: HasValid =>
  val numElements: Int

  val address = UInt(log2Ceil(numElements).W)
}

sealed trait DifftestBaseBundle extends Bundle {
  def hasValid: Boolean = this.isInstanceOf[HasValid]
  def getValid: Bool = getValidOption.getOrElse(true.B)
  def getValidOption: Option[Bool] = {
    this match {
      case b: HasValid => Some(b.valid)
      case _           => None
    }
  }

  def needUpdate: Option[Bool] = if (hasValid) Some(getValid) else None
  def hasAddress: Boolean = this.isInstanceOf[HasAddress]
  def getNumElements: Int = {
    this match {
      case b: HasAddress => b.numElements
      case _             => 0
    }
  }
}

class ArchEvent extends DifftestBaseBundle with HasValid {
  val interrupt = UInt(32.W)
  val exception = UInt(32.W)
  val exceptionPC = UInt(64.W)
  val exceptionInst = UInt(32.W)
  val hasNMI = Bool()
  val virtualInterruptIsHvictlInject = Bool()
}

class InstrCommit(val numPhyRegs: Int = 32) extends DifftestBaseBundle with HasValid {
  val skip = Bool()
  val isRVC = Bool()
  val rfwen = Bool()
  val fpwen = Bool()
  val vecwen = Bool()
  val wpdest = UInt(log2Ceil(numPhyRegs).W)
  val wdest = UInt(8.W)

  val pc = UInt(64.W)
  val instr = UInt(32.W)
  val robIdx = UInt(10.W)
  val lqIdx = UInt(7.W)
  val sqIdx = UInt(7.W)
  val isLoad = Bool()
  val isStore = Bool()
  val nFused = UInt(8.W)
  val special = UInt(8.W)

  def setSpecial(
    isDelayedWb: Bool = false.B,
    isExit: Bool = false.B,
  ): Unit = {
    special := Cat(isExit, isDelayedWb)
  }
}

// Instantiate inside DiffTest, work for get_commit_data specially
class CommitData extends DifftestBaseBundle with HasValid {
  val data = UInt(64.W)
}

class TrapEvent extends DifftestBaseBundle {
  val hasTrap = Bool()
  val cycleCnt = UInt(64.W)
  val instrCnt = UInt(64.W)
  val hasWFI = Bool()

  val code = UInt(32.W)
  val pc = UInt(64.W)

  override def needUpdate: Option[Bool] = Some(hasTrap || hasWFI)
}

class CSRState extends DifftestBaseBundle {
  val privilegeMode = UInt(64.W)
  val mstatus = UInt(64.W)
  val sstatus = UInt(64.W)
  val mepc = UInt(64.W)
  val sepc = UInt(64.W)
  val mtval = UInt(64.W)
  val stval = UInt(64.W)
  val mtvec = UInt(64.W)
  val stvec = UInt(64.W)
  val mcause = UInt(64.W)
  val scause = UInt(64.W)
  val satp = UInt(64.W)
  val mip = UInt(64.W)
  val mie = UInt(64.W)
  val mscratch = UInt(64.W)
  val sscratch = UInt(64.W)
  val mideleg = UInt(64.W)
  val medeleg = UInt(64.W)

  def toSeq: Seq[UInt] = getElements.map(_.asUInt)
  def names: Seq[String] = elements.keys.toSeq

  def ===(that: CSRState): Bool = VecInit(toSeq.zip(that.toSeq).map(v => v._1 === v._2)).asUInt.andR
  def =/=(that: CSRState): Bool = VecInit(toSeq.zip(that.toSeq).map(v => v._1 =/= v._2)).asUInt.orR
}

class HCSRState extends DifftestBaseBundle {
  val virtMode = UInt(64.W)
  val mtval2 = UInt(64.W)
  val mtinst = UInt(64.W)
  val hstatus = UInt(64.W)
  val hideleg = UInt(64.W)
  val hedeleg = UInt(64.W)
  val hcounteren = UInt(64.W)
  val htval = UInt(64.W)
  val htinst = UInt(64.W)
  val hgatp = UInt(64.W)
  val vsstatus = UInt(64.W)
  val vstvec = UInt(64.W)
  val vsepc = UInt(64.W)
  val vscause = UInt(64.W)
  val vstval = UInt(64.W)
  val vsatp = UInt(64.W)
  val vsscratch = UInt(64.W)
}

class DebugModeCSRState extends DifftestBaseBundle {
  val debugMode = Bool()
  val dcsr = UInt(64.W)
  val dpc = UInt(64.W)
  val dscratch0 = UInt(64.W)
  val dscratch1 = UInt(64.W)
}

class TriggerCSRState extends DifftestBaseBundle {
  val tselect = UInt(64.W)
  val tdata1 = UInt(64.W)
  val tinfo = UInt(64.W)
  val tcontrol = UInt(64.W)
}

class DataWriteback(val numElements: Int) extends DifftestBaseBundle with HasValid with HasAddress {
  val data = UInt(64.W)
}

class ArchIntRegState extends DifftestBaseBundle {
  val value = Vec(32, UInt(64.W))

  def apply(i: UInt): UInt = value(i(4, 0))
  def apply(i: Int): UInt = value(i)

  def toSeq: Seq[UInt] = value
  def names: Seq[String] = Seq(
    "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2",
    "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
  )

  def ===(that: ArchIntRegState): Bool = {
    VecInit(value.zip(that.value).map(v => v._1 === v._2)).asUInt.andR
  }
  def =/=(that: ArchIntRegState): Bool = {
    VecInit(value.zip(that.value).map(v => v._1 =/= v._2)).asUInt.orR
  }
}

class ArchFpRegState extends ArchIntRegState {
  override def names: Seq[String] = Seq(
    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
    "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
  )
}

class ArchVecRegState extends DifftestBaseBundle {
  val value = Vec(64, UInt(64.W))
}

class ArchDelayedUpdate(val numElements: Int) extends DifftestBaseBundle with HasValid with HasAddress {
  val data = UInt(64.W)
  val nack = Bool()
}

class VecCSRState extends DifftestBaseBundle {
  val vstart = UInt(64.W)
  val vxsat = UInt(64.W)
  val vxrm = UInt(64.W)
  val vcsr = UInt(64.W)
  val vl = UInt(64.W)
  val vtype = UInt(64.W)
  val vlenb = UInt(64.W)
}

class FpCSRState extends DifftestBaseBundle {
  val fcsr = UInt(64.W)
}

class SbufferEvent extends DifftestBaseBundle with HasValid {
  val addr = UInt(64.W)
  val data = Vec(64, UInt(8.W))
  val mask = UInt(64.W)
}

class StoreEvent extends DifftestBaseBundle with HasValid {
  val addr = UInt(64.W)
  val data = UInt(64.W)
  val mask = UInt(8.W)
}

class LoadEvent extends DifftestBaseBundle with HasValid {
  val paddr = UInt(64.W)
  val opType = UInt(8.W)
  val isAtomic = Bool()
  val isLoad = Bool() // Todo: support vector load
}

class AtomicEvent extends DifftestBaseBundle with HasValid {
  val addr = UInt(64.W)
  val data = UInt(64.W)
  val mask = UInt(8.W)
  val fuop = UInt(8.W)
  val out = UInt(64.W)
}

class L1TLBEvent extends DifftestBaseBundle with HasValid {
  val satp = UInt(64.W)
  val vpn = UInt(64.W)
  val ppn = UInt(64.W)
  val vsatp = UInt(64.W)
  val hgatp = UInt(64.W)
  val s2xlate = UInt(2.W)
}

class L2TLBEvent extends DifftestBaseBundle with HasValid {
  val valididx = Vec(8, Bool())
  val satp = UInt(64.W)
  val vpn = UInt(64.W)
  val pbmt = UInt(2.W)
  val g_pbmt = UInt(2.W)
  val ppn = Vec(8, UInt(64.W))
  val perm = UInt(8.W)
  val level = UInt(8.W)
  val pf = Bool()
  val pteidx = Vec(8, Bool())
  val vsatp = UInt(64.W)
  val hgatp = UInt(64.W)
  val gvpn = UInt(64.W)
  val g_perm = UInt(8.W)
  val g_level = UInt(8.W)
  val s2ppn = UInt(64.W)
  val gpf = Bool()
  val s2xlate = UInt(2.W)
}

class RefillEvent extends DifftestBaseBundle with HasValid {
  val addr = UInt(64.W)
  val data = Vec(8, UInt(64.W))
  val idtfr = UInt(8.W) // identifier for flexible usage
}

class ScEvent extends DifftestBaseBundle with HasValid {
  val success = Bool()
}

class RunaheadEvent extends DifftestBaseBundle with HasValid {
  val branch = Bool()
  val may_replay = Bool()
  val pc = UInt(64.W)
  val checkpoint_id = UInt(64.W)
}

class RunaheadCommitEvent extends DifftestBaseBundle with HasValid {
  val pc = UInt(64.W)
}

class RunaheadRedirectEvent extends DifftestBaseBundle with HasValid {
  val pc = UInt(64.W) // for debug only
  val target_pc = UInt(64.W) // for debug only
  val checkpoint_id = UInt(64.W)
}

class TraceInfo extends DifftestBaseBundle with HasValid {
  val in_replay = Bool()
  val trace_head = UInt(16.W)
  val trace_size = UInt(16.W)
}
