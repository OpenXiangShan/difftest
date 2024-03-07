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
  def getValid: Bool = {
    this match {
      case b: HasValid => b.valid
      case _ => true.B
    }
  }

  def hasAddress: Boolean = this.isInstanceOf[HasAddress]
  def getNumElements: Int = {
    this match {
      case b: HasAddress => b.numElements
      case _ => 0
    }
  }
}

class ArchEvent extends DifftestBaseBundle with HasValid {
  val interrupt = UInt(32.W)
  val exception = UInt(32.W)
  val exceptionPC = UInt(64.W)
  val exceptionInst = UInt(32.W)
}

class InstrCommit(val numPhyRegs: Int) extends DifftestBaseBundle with HasValid {
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

class TrapEvent extends DifftestBaseBundle {
  val hasTrap = Bool()
  val cycleCnt = UInt(64.W)
  val instrCnt = UInt(64.W)
  val hasWFI = Bool()

  val code = UInt(3.W)
  val pc = UInt(64.W)
}

class CSRState extends DifftestBaseBundle {
  val priviledgeMode = UInt(64.W)
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
  val fdiMainCfg = UInt(64.W)
  val fdiUMBoundLo = UInt(64.W)
  val fdiUMBoundHi = UInt(64.W)
  val fdiLibCfg = UInt(64.W)
  val fdiLibBound = Vec(8, UInt(64.W))
  val fdiMainCall = UInt(64.W)
  val fdiReturnPC = UInt(64.W)
  val fdiJumpCfg = UInt(64.W)
  val fdiJumpBound = Vec(2, UInt(64.W))
}

class DebugModeCSRState extends DifftestBaseBundle {
  val debugMode = Bool()
  val dcsr = UInt(64.W)
  val dpc = UInt(64.W)
  val dscratch0 = UInt(64.W)
  val dscratch1 = UInt(64.W)
}

class DataWriteback(val numElements: Int) extends DifftestBaseBundle with HasValid with HasAddress {
  val data  = UInt(64.W)
}

class ArchIntRegState extends DifftestBaseBundle {
  val value = Vec(32, UInt(64.W))
}

class ArchFpRegState extends DifftestBaseBundle {
  val value = Vec(32, UInt(64.W))
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
  val fuType = UInt(8.W)
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
}

class L2TLBEvent extends DifftestBaseBundle with HasValid {
  val valididx = Vec(8, Bool())
  val satp = UInt(64.W)
  val vpn = UInt(64.W)
  val ppn = Vec(8, UInt(64.W))
  val perm = UInt(8.W)
  val level = UInt(8.W)
  val pf = Bool()
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
