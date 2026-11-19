/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

package app

import chisel3._
import difftest._
import difftest.util.DifftestProfile

import scala.annotation.tailrec

// Main class to generate difftest modules when design is not written in chisel.
class DifftestInterfaces extends Module {
  val arch_event = DifftestModule(new DiffArchEvent, dontCare = true)
  val instr_commit = DifftestModule(new DiffInstrCommit(192), dontCare = true)
  val trap_event = DifftestModule(new DiffTrapEvent, dontCare = true)
  val csr_state = DifftestModule(new DiffCSRState, dontCare = true)
  val hcsr_state = DifftestModule(new DiffHCSRState, dontCare = true)
  val debug_mode = DifftestModule(new DiffDebugMode, dontCare = true)
  val trigger_csr_state = DifftestModule(new DiffTriggerCSRState, dontCare = true)
  val arch_int_delayed_update = DifftestModule(new DiffArchIntDelayedUpdate, dontCare = true)
  val arch_fp_delayed_update = DifftestModule(new DiffArchFpDelayedUpdate, dontCare = true)
  val arch_int_rename_table = DifftestModule(new DiffArchIntRenameTable(224), dontCare = true)
  val arch_fp_rename_table = DifftestModule(new DiffArchFpRenameTable(192), dontCare = true)
  val arch_vec_rename_table = DifftestModule(new DiffArchVecRenameTable(150), dontCare = true)
  val phy_int_reg_state = DifftestModule(new DiffPhyIntRegState(224), dontCare = true)
  val phy_fp_reg_state = DifftestModule(new DiffPhyFpRegState(192), dontCare = true)
  val phy_vec_reg_state = DifftestModule(new DiffPhyVecRegState(150), dontCare = true)
  val vector_csr_state = DifftestModule(new DiffVecCSRState, dontCare = true)
  val fp_csr_state = DifftestModule(new DiffFpCSRState, dontCare = true)
  val sbuffer_event = DifftestModule(new DiffSbufferEvent, dontCare = true)
  val uncache_mm_store = DifftestModule(new DiffUncacheMMStoreEvent, dontCare = true)
  val store_event = DifftestModule(new DiffStoreEvent, dontCare = true)
  val load_event = DifftestModule(new DiffLoadEvent, dontCare = true)
  val atomic_event = DifftestModule(new DiffAtomicEvent, dontCare = true)
  val cmo_inval_event = DifftestModule(new DiffCMOInvalEvent, dontCare = true)
  val l1tlb_event = DifftestModule(new DiffL1TLBEvent, dontCare = true)
  val l2tlb_event = DifftestModule(new DiffL2TLBEvent, dontCare = true)
  val refill_event = DifftestModule(new DiffRefillEvent, dontCare = true);
  val lr_sc_event = DifftestModule(new DiffLrScEvent, dontCare = true)
  val runahead_event = DifftestModule(new DiffRunaheadEvent, dontCare = true)
  val runahead_commit_event = DifftestModule(new DiffRunaheadCommitEvent, dontCare = true)
  val runahead_redirect_event = DifftestModule(new DiffRunaheadRedirectEvent, dontCare = true)
  val non_reg_interrupt_pending_event = DifftestModule(new DiffNonRegInterruptPendingEvent, dontCare = true)
  val mhpmevent_overflow_event = DifftestModule(new DiffMhpmeventOverflowEvent, dontCare = true)
  val critical_error_event = DifftestModule(new DiffCriticalErrorEvent, dontCare = true)
  val sync_aia_event = DifftestModule(new DiffSyncAIAEvent, dontCare = true)
  val sync_custom_mflushpwr_event = DifftestModule(new DiffSyncCustomMflushpwrEvent, dontCare = true)

  DifftestModule.collect("demo")
}

// Generate simulation interface based on Profile describing the instantiated information of design
class DifftestTop(profileName: String, numCoresOption: Option[Int]) extends Module with HasDiffTestInterfaces {
  val profile = DifftestProfile.fromJson(profileName)
  val numCores = numCoresOption.getOrElse(profile.numCores)
  val bundles = (0 until numCores).flatMap(coreid =>
    profile.bundles.zipWithIndex.map { case (p, i) =>
      val io = DifftestModule(p.toBundle, true, p.delay).suggestName(s"gateway_${coreid}_$i")
      dontTouch(io)
    }
  )
  DifftestModule.generateSvhInterface(bundles, numCores)

  override def cpuName: Option[String] = Some(profile.cpu)
}

abstract class DifftestApp extends App {
  case class GenParams(
    profile: Option[String] = None,
    numCores: Option[Int] = None,
  )
  def parseArgs(args: Array[String]): (GenParams, Array[String]) = {
    val default = new GenParams()
    var firrtlOpts = Array[String]()
    @tailrec
    def nextOption(param: GenParams, list: List[String]): GenParams = {
      list match {
        case Nil                            => param
        case "--profile" :: str :: tail     => nextOption(param.copy(profile = Some(str)), tail)
        case "--num-cores" :: value :: tail => nextOption(param.copy(numCores = Some(value.toInt)), tail)
        case option :: tail =>
          firrtlOpts :+= option
          nextOption(param, tail)
      }
    }
    (nextOption(default, args.toList), firrtlOpts)
  }
  val (newArgs, firtoolOptions) = DifftestModule.parseArgs(args)
  val (param, firrtlOpts) = parseArgs(newArgs)
  val gen = if (param.profile.isDefined) { () =>
    DifftestModule.top(new DifftestTop(param.profile.get, param.numCores))
  } else { () =>
    new DifftestInterfaces
  }
}
