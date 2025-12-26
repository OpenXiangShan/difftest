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

#include "checkers.h"

#ifdef CONFIG_DIFFTEST_LRSCEVENT
bool LrScChecker::get_valid(const DifftestLrScEvent &probe) {
  return probe.valid;
}

void LrScChecker::clear_valid(DifftestLrScEvent &probe) {
  probe.valid = 0;
}

int LrScChecker::check(const DifftestLrScEvent &probe) {
  // sync lr/sc reg microarchitectural status to the REF
  struct SyncState sync;
  sync.sc_fail = !probe.success;
  proxy->uarchstatus_sync((uint64_t *)&sync);
  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_LRSCEVENT

#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
bool NonRegInterruptPendingChecker::get_valid(const DifftestNonRegInterruptPendingEvent &probe) {
  return probe.valid;
}

void NonRegInterruptPendingChecker::clear_valid(DifftestNonRegInterruptPendingEvent &probe) {
  probe.valid = 0;
}

int NonRegInterruptPendingChecker::check(const DifftestNonRegInterruptPendingEvent &probe) {
  struct NonRegInterruptPending ip;
  ip.platformIRPMeip = probe.platformIRPMeip;
  ip.platformIRPMtip = probe.platformIRPMtip;
  ip.platformIRPMsip = probe.platformIRPMsip;
  ip.platformIRPSeip = probe.platformIRPSeip;
  ip.platformIRPStip = probe.platformIRPStip;
  ip.platformIRPVseip = probe.platformIRPVseip;
  ip.platformIRPVstip = probe.platformIRPVstip;
  ip.fromAIAMeip = probe.fromAIAMeip;
  ip.fromAIASeip = probe.fromAIASeip;
  ip.localCounterOverflowInterruptReq = probe.localCounterOverflowInterruptReq;
  proxy->non_reg_interrupt_pending(ip);
  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT

#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
bool AiaChecker::get_valid(const DifftestSyncAIAEvent &probe) {
  return probe.valid;
}
void AiaChecker::clear_valid(DifftestSyncAIAEvent &probe) {
  probe.valid = 0;
}

int AiaChecker::check(const DifftestSyncAIAEvent &probe) {
  struct FromAIA aia;
  aia.mtopei = probe.mtopei;
  aia.stopei = probe.stopei;
  aia.vstopei = probe.vstopei;
  aia.hgeip = probe.hgeip;
  proxy->sync_aia(aia);
  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_SYNCAIAEVENT

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
bool MhpmeventOverflowChecker::get_valid(const DifftestMhpmeventOverflowEvent &probe) {
  return probe.valid;
}

void MhpmeventOverflowChecker::clear_valid(DifftestMhpmeventOverflowEvent &probe) {
  probe.valid = 0;
}

int MhpmeventOverflowChecker::check(const DifftestMhpmeventOverflowEvent &probe) {
  proxy->mhpmevent_overflow(probe.mhpmeventOverflow);
  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT

#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
bool CustomMflushpwrChecker::get_valid(const DifftestSyncCustomMflushpwrEvent &probe) {
  return probe.valid;
}

void CustomMflushpwrChecker::clear_valid(DifftestSyncCustomMflushpwrEvent &probe) {
  probe.valid = 0;
}

int CustomMflushpwrChecker::check(const DifftestSyncCustomMflushpwrEvent &probe) {
  proxy->sync_custom_mflushpwr(probe.l2FlushDone);
  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
