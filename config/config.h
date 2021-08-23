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

// emulated memory size (Byte)
// #define EMU_RAM_SIZE (256 * 1024 * 1024UL) // 256 MB
#define EMU_RAM_SIZE (8 * 1024 * 1024 * 1024UL) // 8 GB

// first valid instruction's address, difftest starts from this instruction
#define FIRST_INST_ADDRESS 0x80000000

// sdcard image to be used in simulation
// #define SDCARD_IMAGE "/home/xyn/workloads/debian/riscv-debian.img"

// max commit width
#define DIFFTEST_COMMIT_WIDTH 6

// max store width
#define DIFFTEST_STORE_WIDTH 2

// commit inst history length 
#define DEBUG_INST_TRACE_SIZE 16

// commit inst group history length 
#define DEBUG_GROUP_TRACE_SIZE 16

// time to fork a new checkpoint process
#define FORK_INTERVAL 1 // unit: second

// max number of checkpoint process at a time 
#define SLOT_SIZE 2

// exit when error when fork 
#define FAIT_EXIT    exit(EXIT_FAILURE);

// process sleep time  
#define WAIT_INTERVAL 5

//time to save a snapshot
#define SNAPSHOT_INTERVAL 60 // unit: second

#endif
