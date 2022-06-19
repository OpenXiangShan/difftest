/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
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
import chisel3.stage._

// Main class to generat difftest modules when design is not written in chisel.
class DifftestTop extends Module {
    var difftest_arch_event = Module(new DifftestArchEvent);
    var difftest_basic_instr_commit = Module(new DifftestBasicInstrCommit);
    var difftest_instr_commit = Module(new DifftestInstrCommit);
    var difftest_basic_trap_event = Module(new DifftestBasicTrapEvent);
    var difftest_trap_event = Module(new DifftestTrapEvent);
    var difftest_csr_state = Module(new DifftestCSRState);
    var difftest_debug_mode = Module(new DifftestDebugMode);
    var difftest_int_writeback = Module(new DifftestIntWriteback);
    var difftest_fp_writeback = Module(new DifftestFpWriteback);
    var difftest_arch_int_reg_state = Module(new DifftestArchIntRegState);
    var difftest_arch_fp_reg_state = Module(new DifftestArchFpRegState);
    var difftest_sbuffer_event = Module(new DifftestSbufferEvent);
    var difftest_store_event = Module(new DifftestStoreEvent);
    var difftest_load_event = Module(new DifftestLoadEvent);
    var difftest_atomic_event = Module(new DifftestAtomicEvent);
    var difftest_ptw_event = Module(new DifftestPtwEvent);
    var difftest_irefill_event = Module(new DifftestRefillEvent);
    var difftest_drefill_event = Module(new DifftestRefillEvent);
    var difftest_lr_sc_event = Module(new DifftestLrScEvent);
    var difftest_runahead_event = Module(new DifftestRunaheadEvent);
    var difftest_runahead_commit_event = Module(new DifftestRunaheadCommitEvent);
    var difftest_runahead_redirect_event = Module(new DifftestRunaheadRedirectEvent);
    var difftest_runahead_memdep_pred = Module(new DifftestRunaheadMemdepPred);
}

object DifftestMain extends App {
  (new ChiselStage).execute(args, Seq(
      ChiselGeneratorAnnotation(() => new DifftestTop))
  )
}
