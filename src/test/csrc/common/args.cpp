/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "args.h"
#include "ram.h"
#include "remote_bitbang.h"
#include <getopt.h>
#ifdef CONFIG_DIFFTEST_IOTRACE
#include "difftest-iotrace.h"
#endif // CONFIG_DIFFTEST_IOTRACE

static inline long long int atoll_strict(const char *str, const char *arg) {
  if (strspn(str, " +-0123456789") != strlen(str)) {
    printf("[ERROR] --%s=NUM only accept numeric argument\n", arg);
    exit(EINVAL);
  }
  return atoll(str);
}

static inline void print_help(const char *file) {
  printf("Usage: %s [OPTION...]\n", file);
  printf("\n");
  printf("  -s, --seed=NUM             use this seed\n");
  printf("  -C, --max-cycles=NUM       execute at most NUM cycles\n");
  printf("  -I, --max-instr=NUM        execute at most NUM instructions\n");
  printf("  -W, --warmup-instr=NUM     the number of warmup instructions\n");
  printf("  -D, --stat-cycles=NUM      the interval cycles of dumping statistics\n");
  printf("  -i, --image=FILE           run with this image file\n");
  printf("  -r, --gcpt-restore=FILE    overwrite gcptrestore img with this image file\n");
  printf("  -b, --log-begin=NUM        display log from NUM th cycle\n");
  printf("  -e, --log-end=NUM          stop display log at NUM th cycle\n");
#ifdef DEBUG_REFILL
  printf("  -T, --track-instr=ADDR     track refill action concerning ADDR\n");
#endif
#ifdef ENABLE_IPC
  printf("  -R, --ipc-interval=NUM     the interval insts of drawing IPC curve\n");
#endif
  printf("  -X, --fork-interval=NUM    LightSSS snapshot interval (in seconds), default: 10\n");
  printf("      --overwrite-nbytes=N   set valid bytes, but less than 0xf00, default: 0xe00\n");
  printf("      --overwrite-auto       overwrite size is automatically set of the new gcpt\n");
#ifdef PLUGIN_SIMFRONTEND
  printf("      --instr-trace          Setting the trace of instructions for SimFrontEnd\n");
#endif // PLUGIN_SIMFRONTEND
  printf("      --force-dump-result    force dump performance counter result in the end\n");
  printf("      --load-snapshot=PATH   load snapshot from PATH\n");
  printf("      --enable-snapshot      enable simulation snapshots\n");
  printf("      --dump-wave            dump waveform when log is enabled\n");
  printf("      --dump-wave-full       dump full waveform when log is enabled\n");
  printf("      --dump-ref-trace       dump REF trace when log is enabled\n");
  printf("      --dump-commit-trace    dump commit trace when log is enabled\n");
#ifdef ENABLE_CHISEL_DB
  printf("      --dump-db              enable database dump\n");
  printf("      --dump-select-db       select database's table to dump\n");
#endif
  printf("  -F, --flash                the flash bin file for simulation\n");
  printf("      --sim-run-ahead        let a fork of simulator run ahead of commit for perf analysis\n");
  printf("      --wave-path=FILE       dump waveform to a specified PATH\n");
  printf("      --ram-size=SIZE        simulation memory size, for example 8GB / 128MB\n");
  printf("      --enable-fork          enable folking child processes to debug\n");
  printf("      --no-diff              disable differential testing\n");
  printf("      --diff=PATH            set the path of REF for differential testing\n");
  printf("      --enable-jtag          enable remote bitbang server\n");
  printf("      --remote-jtag-port     specify remote bitbang port\n");
#ifdef WITH_DRAMSIM3
  printf("      --dramsim3-ini         specify the ini file for DRAMSim3\n");
  printf("      --dramsim3-outdir      specify the output dir for DRAMSim3\n");
#endif
#if VM_COVERAGE == 1
  printf("      --dump-coverage        enable coverage dump\n");
#endif // VM_COVERAGE
  printf("      --load-difftrace=NAME  load from trace NAME\n");
  printf("      --dump-difftrace=NAME  dump to trace NAME\n");
  printf("      --iotrace-name=NAME    load from/dump to iotrace NAME\n");
  printf("      --dump-footprints=NAME dump memory access footprints to NAME\n");
  printf("      --as-footprints        load the image as memory access footprints\n");
  printf("      --dump-linearized=NAME dump the linearized footprints to NAME\n");
  printf("      --copy-ram=OFFSET      duplicate the memory at OFFSET\n");
  printf("  -h, --help                 print program help info\n");
  printf("\n");
}

CommonArgs parse_args(int argc, const char *argv[]) {
  CommonArgs args;
  int long_index = 0;
#ifndef CONFIG_NO_DIFFTEST
  extern const char *difftest_ref_so;
#endif // CONFIG_NO_DIFFTEST

  /* clang-format off */
  const struct option long_options[] = {
    { "load-snapshot",     1, NULL,  0  },
    { "dump-wave",         0, NULL,  0  },
    { "enable-snapshot",   0, NULL,  0  },
    { "force-dump-result", 0, NULL,  0  },
    { "diff",              1, NULL,  0  },
    { "no-diff",           0, NULL,  0  },
    { "enable-fork",       0, NULL,  0  },
    { "enable-jtag",       0, NULL,  0  },
    { "wave-path",         1, NULL,  0  },
    { "ram-size",          1, NULL,  0  },
    { "sim-run-ahead",     0, NULL,  0  },
    { "dump-db",           0, NULL,  0  },
    { "dump-select-db",    1, NULL,  0  },
    { "dump-coverage",     0, NULL,  0  },
    { "dump-ref-trace",    0, NULL,  0  },
    { "dump-commit-trace", 0, NULL,  0  },
    { "load-difftrace",    1, NULL,  0  },
    { "dump-difftrace",    1, NULL,  0  },
    { "dump-footprints",   1, NULL,  0  },
    { "as-footprints",     0, NULL,  0  },
    { "dump-linearized",   1, NULL,  0  },
    { "dump-wave-full",    0, NULL,  0  },
    { "overwrite-nbytes",  1, NULL,  0  },
    { "remote-jtag-port",  1, NULL,  0  },
    { "iotrace-name",      1, NULL,  0  },
    { "dramsim3-ini",      1, NULL,  0  },
    { "dramsim3-outdir",   1, NULL,  0  },
    { "overwrite-auto",    1, NULL,  0  },
    { "instr-trace",       1, NULL,  0  },
    { "copy-ram",          1, NULL,  0  },
    { "seed",              1, NULL, 's' },
    { "max-cycles",        1, NULL, 'C' },
    { "fork-interval",     1, NULL, 'X' },
    { "max-instr",         1, NULL, 'I' },
#ifdef DEBUG_REFILL
    { "track-instr",       1, NULL, 'T' },
#endif
    { "ipc-interval",      1, NULL, 'R' },
    { "warmup-instr",      1, NULL, 'W' },
    { "stat-cycles",       1, NULL, 'D' },
    { "image",             1, NULL, 'i' },
    { "gcpt-restore",      1, NULL, 'r' },
    { "log-begin",         1, NULL, 'b' },
    { "log-end",           1, NULL, 'e' },
    { "flash",             1, NULL, 'F' },
    { "help",              0, NULL, 'h' },
    { 0,                   0, NULL,  0  }
  };
  /* clang-format on */

  int o;
  while ((o = getopt_long(argc, const_cast<char *const *>(argv), "-s:C:X:I:T:R:W:D:hi:r:m:b:e:F:", long_options,
                          &long_index)) != -1) {
    switch (o) {
      case 0:
        switch (long_index) {
          case 0: args.snapshot_path = optarg; continue;
          case 1: args.enable_waveform = true; continue;
          case 2: args.enable_snapshot = true; continue;
          case 3: args.force_dump_result = true; continue;
#ifndef CONFIG_NO_DIFFTEST
          case 4: difftest_ref_so = optarg; continue;
#endif // CONFIG_NO_DIFFTEST
          case 5: args.enable_diff = false; continue;
          case 6: args.enable_fork = true; continue;
          case 7: enable_simjtag = true; continue;
          case 8: args.wave_path = optarg; continue;
          case 9: args.ram_size = optarg; continue;
          case 10:
#ifdef ENABLE_RUNHEAD
            args.enable_runahead = true;
#else
            printf("[WARN] runahead is not enabled at compile time, ignore --sim-run-ahead\n");
#endif
            continue;
#ifdef ENABLE_CHISEL_DB
          case 11: args.dump_db = true; continue;
          case 12:
            args.dump_db = true;
            args.select_db = optarg;
            continue;
#else
          case 11:
          case 12: printf("[WARN] chisel db is not enabled at compile time, ignore --dump-db\n"); continue;
#endif
          case 13:
#if VM_COVERAGE == 1
            args.dump_coverage = true;
#else
            printf("[WARN] coverage is not enabled at compile time, ignore --dump-coverage\n");
#endif // VM_COVERAGE
            continue;
          case 14: args.enable_ref_trace = true; continue;
          case 15: args.enable_commit_trace = true; continue;
          case 16:
            args.trace_name = optarg;
            args.trace_is_read = true;
            continue;
          case 17:
            args.trace_name = optarg;
            args.trace_is_read = false;
            continue;
          case 18: args.footprints_name = optarg; continue;
          case 19: args.image_as_footprints = true; continue;
          case 20: args.linearized_name = optarg; continue;
          case 21:
            args.enable_waveform = true;
            args.enable_waveform_full = true;
            continue;
          case 22: args.overwrite_nbytes = atoll_strict(optarg, "overwrite_nbytes"); continue;
          case 23: remote_jtag_port = atoll_strict(optarg, "remote-jtag-port"); continue;
          case 24:
#ifdef CONFIG_DIFFTEST_IOTRACE
            extern void set_iotrace_name(char *s);
            set_iotrace_name(optarg);
#else
            printf("[WARN] iotrace is not enabled at compile time, ignore --iotrace-name");
#endif // CONFIG_DIFFTEST_IOTRACE
            continue;
          case 25:
#ifdef WITH_DRAMSIM3
            args.dramsim3_ini = optarg;
            continue;
#else
            printf("Dramsim3 is not enabled, but --dramsim3-ini is specified\n");
            exit(1);
            break;
#endif
          case 26:
#ifdef WITH_DRAMSIM3
            args.dramsim3_outdir = optarg;
            continue;
#else
            printf("Dramsim3 is not enabled, but --dramsim3-outdir is specified\n");
            exit(1);
            break;
#endif
          case 27: args.overwrite_nbytes_autoset = true; continue;
          case 28: args.instr_trace = optarg; continue;
          case 29: args.copy_ram_offset = parse_ramsize(optarg); continue;
        }
        // fall through
      default: print_help(argv[0]); exit(0);
      case 's':
        if (std::string(optarg) != "NO_SEED") {
          args.seed = atoll_strict(optarg, "seed");
          Info("Using seed = %d\n", args.seed);
        }
        break;
      case 'C': args.max_cycles = atoll_strict(optarg, "max-cycles"); break;
      case 'X': args.fork_interval = 1000 * atoll_strict(optarg, "fork-interval"); break;
      case 'I': args.max_instr = atoll_strict(optarg, "max-instr"); break;
#ifdef DEBUG_REFILL
      case 'T':
        args.track_instr = std::strtoll(optarg, NULL, 0);
        Info("Tracking addr 0x%lx\n", args.track_instr);
        if (args.track_instr == 0) {
          printf("Invalid track addr\n");
          exit(1);
        }
        break;
#endif
      case 'R':
#ifdef ENABLE_IPC
        args.ipc_interval = atoll_strict(optarg, "ipc-interval");
        printf("Drawing IPC curve each %d cycles\n", args.ipc_interval);
        if (args.ipc_interval == 0) {
          printf("Invalid ipc interval\n");
          exit(1);
        }
#else
        printf("[WARN] drawing ipc curve is not enabled at compile time, ignore --ipc-interval\n");
#endif
        break;
      case 'W': args.warmup_instr = atoll_strict(optarg, "warmup-instr"); break;
      case 'D': args.stat_cycles = atoll_strict(optarg, "stat-cycles"); break;
      case 'i': args.image = optarg; break;
      case 'r': args.gcpt_restore = optarg; break;
      case 'b': args.log_begin = atoll_strict(optarg, "log-begin"); break;
      case 'e': args.log_end = atoll_strict(optarg, "log-end"); break;
      case 'F': args.flash_bin = optarg; break;
    }
  }

  args.enable_waveform = args.enable_waveform && !args.enable_fork;

#ifdef ENABLE_IPC
  char *ipc_image = (char *)malloc(255);
  char *ipc_file = (char *)malloc(255);
  strcpy(ipc_image, args.image);
  char *c = strchr(ipc_image, '/');
  while (c) {
    *c = '_';
    c = strchr(c, '/');
  }
  printf("%s\n", ipc_image);
  strcpy(ipc_file, "ipc/");
  strcat(ipc_file, ipc_image);
  strcat(ipc_file, ".txt");
  mkdir("ipc", 0755);
  args.ipc_file = fopen(ipc_file, "w");
#endif

  return args;
}
