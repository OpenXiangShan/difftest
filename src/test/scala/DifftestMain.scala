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

package difftest

import chisel3._
import chisel3.stage._

// Main class to generate difftest modules when design is not written in chisel.
class DifftestTop extends Module {
  val difftest_arch_event = DifftestModule(new DiffArchEvent, dontCare = true)
  val difftest_instr_commit = DifftestModule(new DiffInstrCommit, dontCare = true)
  val difftest_trap_event = DifftestModule(new DiffTrapEvent, dontCare = true)
  val difftest_csr_state = DifftestModule(new DiffCSRState, dontCare = true)
  val difftest_hcsr_state = DifftestModule(new DiffHCSRState, dontCare = true)
  val difftest_debug_mode = DifftestModule(new DiffDebugMode, dontCare = true)
  val difftest_vector_state = DifftestModule(new DiffArchVecRegState, dontCare = true)
  val difftest_vector_csr_state = DifftestModule(new DiffVecCSRState, dontCare = true)
  val difftest_int_writeback = DifftestModule(new DiffIntWriteback, dontCare = true)
  val difftest_fp_writeback = DifftestModule(new DiffFpWriteback, dontCare = true)
  val difftest_arch_int_reg_state = DifftestModule(new DiffArchIntRegState, dontCare = true)
  val difftest_arch_fp_reg_state = DifftestModule(new DiffArchFpRegState, dontCare = true)
  val difftest_sbuffer_event = DifftestModule(new DiffSbufferEvent, dontCare = true)
  val difftest_store_event = DifftestModule(new DiffStoreEvent, dontCare = true)
  val difftest_load_event = DifftestModule(new DiffLoadEvent, dontCare = true)
  val difftest_atomic_event = DifftestModule(new DiffAtomicEvent, dontCare = true)
  val difftest_itlb_event = DifftestModule(new DiffL1TLBEvent, dontCare = true)
  val difftest_ldtlb_event = DifftestModule(new DiffL1TLBEvent, dontCare = true)
  val difftest_sttlb_event = DifftestModule(new DiffL1TLBEvent, dontCare = true)
  val difftest_l2tlb_event = DifftestModule(new DiffL2TLBEvent, dontCare = true)
  val difftest_irefill_event = DifftestModule(new DiffRefillEvent, dontCare = true)
  val difftest_drefill_event = DifftestModule(new DiffRefillEvent, dontCare = true)
  val difftest_ptwrefill_event = DifftestModule(new DiffRefillEvent, dontCare = true);
  val difftest_lr_sc_event = DifftestModule(new DiffLrScEvent, dontCare = true)
  val difftest_runahead_event = DifftestModule(new DiffRunaheadEvent, dontCare = true)
  val difftest_runahead_commit_event = DifftestModule(new DiffRunaheadCommitEvent, dontCare = true)
  val difftest_runahead_redirect_event = DifftestModule(new DiffRunaheadRedirectEvent, dontCare = true)

  DifftestModule.finish("demo")
}

object DifftestMain extends App {
  (new ChiselStage).execute(args, Seq(ChiselGeneratorAnnotation(() => new DifftestTop)))
}
