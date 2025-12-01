/***************************************************************************************
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifndef __CONFIG_H
#define __CONFIG_H

#include "difftest-state.h"

#if defined(CPU_NUTSHELL)
#elif defined(CPU_XIANGSHAN)
#elif defined(CPU_ROCKET_CHIP)
#else
// This is the default CPU
#define CPU_NUTSHELL
#endif

// -----------------------------------------------------------------------
// Memory and device config
// -----------------------------------------------------------------------

// emulated memory size (Byte)
#if defined(CPU_XIANGSHAN)
#define DEFAULT_EMU_RAM_SIZE 0x7ff80000000UL // from 0x8000_0000 to 0x800_0000_0000, (8192-2)GB memory
#else
#define DEFAULT_EMU_RAM_SIZE (8 * 1024 * 1024 * 1024UL) // 8 GB
#endif

// physical memory base address
#define _PMEM_BASE 0x80000000UL
extern uint64_t PMEM_BASE;
extern uint64_t FIRST_INST_ADDRESS;

// first valid instruction's address, difftest starts from this instruction
#if defined(CPU_NUTSHELL)
#define _FIRST_INST_ADDRESS 0x80000000UL
#elif defined(CPU_XIANGSHAN) || defined(CPU_ROCKET_CHIP)
#define _FIRST_INST_ADDRESS 0x10000000UL
#endif

// sdcard image to be used in simulation
// uncomment the following line to enable this feature
// #define SDCARD_IMAGE "/home/xyn/workloads/debian/riscv-debian.img"

// flash image to be used in simulation
// flash access address align mask
#define FLASH_ALIGH_MASK 0xfffffff8

#if defined(CPU_ROCKET_CHIP)
#define DEFAULT_EMU_FLASH_SIZE 0x10000UL
#else
#define DEFAULT_EMU_FLASH_SIZE (32 * 1024UL) // 4 MB
#endif
extern unsigned long EMU_FLASH_SIZE;

#define DIFFTEST_VLEN 128
#define VLENE_64 DIFFTEST_VLEN/64

// Use sdl to show screen
// Note: It does not work well with clang, to use that, switch to gcc
// uncomment the following line to enable this feature
// #define SHOW_SCREEN

// -----------------------------------------------------------------------
// Checkpoint config
// -----------------------------------------------------------------------

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

// whether to check l1tlb response
#define DEBUG_L1TLB

// whether to check l2tlb response
// #define DEBUG_L2TLB

// whether to enable REF/GoldenMemory record origin data of memory and restore
#ifdef CONFIG_DIFFTEST_REPLAY
#define ENABLE_STORE_LOG
#endif // CONFIG_DIFFTEST_REPLAY

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

// dump chisel-db to a database file
// set WITH_CHISELDB=1 when make to enable chisel db in verilator simulation
// #define ENABLE_CHISEL_DB

// draw ipc curve to a database file
// set WITH_IPC=1 when make to enable drawing ipc curve in verilator simulation
// #define ENABLE_IPC

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

#endif
