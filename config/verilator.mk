#***************************************************************************************
# Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2020-2021 Peng Cheng Laboratory
#
# XiangShan is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#***************************************************************************************

# Verilator based simulation config

# Enable waveform in Verilator simulation
# It will enable `emu --dump-wave`
# EMU_TRACE ?= 1

# Enable Verilator multi-thread support
# EMU_THREADS  ?= 0

# Enable Verilator Savable based checkpoint
# EMU_SNAPSHOT ?= 1

# Enable Fork-wait based checkpoint 
# EMU_FORKWAIT ?= 1

# Enable Verilator coverage
# EMU_COVERAGE ?= 1

# Enable co-simulation with DRAMsim3
# WITH_DRAMSIM3 ?= 1

# Default output period control
# output will be generated when (B<=GTimer<=E) && (L < loglevel)
# use 'emu -h' to see more details
# B ?= 0
# E ?= 0

# default simulation image for `make emu-run`
IMAGE ?= "../ready-to-run/microbench.bin"