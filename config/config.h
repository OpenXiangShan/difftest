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

#ifndef __CONFIG_H
#define __CONFIG_H

#ifndef NUM_CORES
#define NUM_CORES 1
#endif

// -----------------------------------------------------------------------
// Memory and device config
// -----------------------------------------------------------------------

// emulated memory size (Byte)
#define DEFAULT_EMU_RAM_SIZE (8 * 1024 * 1024 * 1024UL) // 8 GB

// physical memory base address
#define PMEM_BASE 0x80000000UL

// first valid instruction's address, difftest starts from this instruction
#ifndef FIRST_INST_ADDRESS
#define FIRST_INST_ADDRESS 0x10000000
#endif

// sdcard image to be used in simulation
// uncomment the following line to enable this feature
// #define SDCARD_IMAGE "/home/xyn/workloads/debian/riscv-debian.img"

// flash image to be used in simulation
// flash access address align mask
#define FLASH_ALIGH_MASK 0xfffffff8

#define DEFAULT_EMU_FLASH_SIZE (32 * 1024UL) // 4 MB
extern unsigned long EMU_FLASH_SIZE;

// Use sdl to show screen
// Note: It does not work well with clang, to use that, switch to gcc
// uncomment the following line to enable this feature
// #define SHOW_SCREEN

// -----------------------------------------------------------------------
// Difftest interface config
// -----------------------------------------------------------------------
#ifndef DIFF_PROXY
#define DIFF_PROXY NemuProxy
#endif

// max physical register file size
#define DIFFTEST_MAX_PRF_SIZE 256

// max commit width
#define DIFFTEST_COMMIT_WIDTH 6

// max store width
//
// max num of stores turn from predicted to non-predicted in 1 cycle
#define DIFFTEST_STORE_WIDTH 2

// max store buffer resp width, for golden mem check
#define DIFFTEST_SBUFFER_RESP_WIDTH 3

// commit inst history length 
#define DEBUG_INST_TRACE_SIZE 32

// commit inst group history length 
#define DEBUG_GROUP_TRACE_SIZE 16

// -----------------------------------------------------------------------
// Checkpoint config
// -----------------------------------------------------------------------

// time to fork a new checkpoint process
#define FORK_INTERVAL 1 // unit: second

// max number of checkpoint process at a time 
#define SLOT_SIZE 2

// exit when error when fork 
#define FAIT_EXIT    exit(EXIT_FAILURE);

// process sleep time  
#define WAIT_INTERVAL 5

// time to save a snapshot
#define SNAPSHOT_INTERVAL 60 // unit: second

// if error, let simulator print debug info
#define ENABLE_SIMULATOR_DEBUG_INFO

// how many cycles child processes step forward when reaching error point
#define STEP_FORWARD_CYCLES 100

// -----------------------------------------------------------------------
// Memory difftest config
// -----------------------------------------------------------------------

// whether to enable smp difftest
// #define DEBUG_SMP

// whether to check memory coherence during refilling
#define DEBUG_REFILL

// dump all tilelink trace to a database
// uncomment the following line to enable this feature
#define DEBUG_TILELINK


// -----------------------------------------------------------------------
// Simulator run ahead config
// -----------------------------------------------------------------------

// Let a fork of simulator run ahead of commit for perf analysis
// uncomment the following line to enable this feature
//#define ENABLE_RUNHEAD

// max run ahead width
#define DIFFTEST_RUNAHEAD_WIDTH 6

// print simulator runahead debug info
// uncomment the following line to enable this feature
// #define RUNAHEAD_DEBUG

// mem dependency check config
#define MEMDEP_WINDOW_SIZE 64

// upper bound of runahead checkpoints
// will be ignored if AUTO_RUNAHEAD_CHECKPOINT_GC is defined
#define RUN_AHEAD_CHECKPOINT_SIZE 64

// automatically free the oldest checkpoint for runahead
// so that we do not need to implement RunaheadCommitEvent in RTL design
// uncomment the following line to enable this feature
#define AUTO_RUNAHEAD_CHECKPOINT_GC
#define AUTO_RUNAHEAD_CHECKPOINT_GC_THRESHOLD 192

// -----------------------------------------------------------------------
// Debug mode and trigger
// -----------------------------------------------------------------------
// make diff in debug mode available by copying debug mode mmio
// currently only usable on spike
//#define DEBUG_MODE_DIFF

#ifndef DEBUG_MEM_BASE
#define DEBUG_MEM_BASE 0x38020000
#endif 

// -----------------------------------------------------------------------
// Do not touch
// -----------------------------------------------------------------------

// whether to maintain goldenmem
#if NUM_CORES>1
    #define DEBUG_GOLDENMEM
#endif

#ifdef DEBUG_REFILL
    #define DEBUG_GOLDENMEM
#endif

#define RUNAHEAD_UNIT_TEST

#ifdef ENABLE_RUNAHEAD
#define TRACE_INFLIGHT_MEM_INST
#define QUERY_MEM_ACCESS
#endif

extern unsigned long EMU_RAM_SIZE;

#endif
