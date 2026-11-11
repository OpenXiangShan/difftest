/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef __MMA_WORKER_H__
#define __MMA_WORKER_H__

#include "common.h"
#include <sys/types.h>

int mma_worker_main(unsigned threads, pid_t owner);

#endif // __MMA_WORKER_H__
