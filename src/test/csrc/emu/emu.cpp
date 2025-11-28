/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "emu.h"
#include "compress.h"
#include "device.h"
#include "flash.h"
#include "lightsss.h"
#include "ram.h"
#include "remote_bitbang.h"
#include "sdcard.h"
#include <getopt.h>
#include <signal.h>
#include <sys/resource.h>
#ifndef CONFIG_NO_DIFFTEST
#include "difftest.h"
#include "goldenmem.h"
#include "refproxy.h"
#endif // CONFIG_NO_DIFFTEST
#ifdef ENABLE_RUNHEAD
#include "runahead.h"
#endif
#ifdef ENABLE_CHISEL_DB
#include "chisel_db.h"
#endif
#ifdef ENABLE_IPC
#include <sys/stat.h>
#endif
#ifdef PLUGIN_SIMFRONTEND
#include "simfrontend.h"
#endif // PLUGIN_SIMFRONTEND

extern remote_bitbang_t *jtag;

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

inline EmuArgs parse_args(int argc, const char *argv[]) {
  EmuArgs args;
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
  while ((o = getopt_long(argc, const_cast<char *const *>(argv), "-s:C:X:I:T:R:W:hi:r:m:b:e:F:", long_options,
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

  if (args.image == NULL) {
    Info("Hint: --image=IMAGE_FILE is not specified. Use /dev/zero instead.\n");
    args.image = "/dev/zero";
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

#ifdef VERILATOR
  Verilated::commandArgs(argc, argv); // Prepare extra args for TLMonitor
#endif
  return args;
}

Emulator::Emulator(int argc, const char *argv[])
    : dut_ptr(new SIMULATOR), cycles(0), trapCode(STATE_RUNNING), elapsed_time(uptime()) {

#ifdef VERILATOR
#if !defined(VERILATOR_VERSION_INTEGER) || VERILATOR_VERSION_INTEGER < 5026000
  // Large designs may cause segmentation fault due to stack overflow.
  // Legacy versions of Verilator do not automatically set the stack size.
  // Therefore, we set it manually here with a default value.
  const size_t EMU_STACK_SIZE = 32 * 1024 * 1024;
  struct rlimit rlim;
  getrlimit(RLIMIT_STACK, &rlim);
  rlim.rlim_cur = EMU_STACK_SIZE;
  if (setrlimit(RLIMIT_STACK, &rlim)) {
    printf("[warning] cannot set stack size. Large designs may cause SIGSEGV.\n");
  }
#endif
#endif // VERILATOR

  args = parse_args(argc, argv);
#ifdef ENABLE_CONSTANTIN
  void constantinLoad();
  constantinLoad();
#endif // CONSTANTIN
#ifdef VERILATOR
  // srand
  srand(args.seed);
  srand48(args.seed);
  Verilated::randSeed(args.seed);
  Verilated::randReset(2);
#endif // VERILATOR

  // init remote-bitbang
  if (enable_simjtag) {
    jtag_init();
  }
  // init flash
  init_flash(args.flash_bin);

  if (args.enable_waveform) {
    uint64_t waveform_clock = args.enable_waveform_full ? 2 * args.log_begin : args.log_begin;
    if (args.wave_path != NULL) {
      dut_ptr->waveform_init(waveform_clock, args.wave_path);
    } else {
      dut_ptr->waveform_init(waveform_clock);
    }
  }

#ifdef PLUGIN_SIMFRONTEND
  init_sim_frontend(args.instr_trace);
#endif // PLUGIN_SIMFRONTEND

  // init core
  reset_ncycles(args.reset_cycles);

  // init ram
  uint64_t ram_size = DEFAULT_EMU_RAM_SIZE;
  if (args.ram_size) {
    ram_size = parse_ramsize(args.ram_size);
  }
  // footprints
  if (args.image_as_footprints) {
    if (args.linearized_name) {
      simMemory = new LinearizedFootprintsMemory(args.image, ram_size, args.linearized_name);
    } else {
      simMemory = new FootprintsMemory(args.image, ram_size);
    }
  }
  // normal linear memory
  else {
    if (args.footprints_name) {
      simMemory = new MmapMemoryWithFootprints(args.image, ram_size, args.footprints_name);
    } else {
      init_ram(args.image, ram_size);
#ifdef WITH_DRAMSIM3
      dramsim3_init(args.dramsim3_ini, args.dramsim3_outdir);
#endif
    }
  }

  if (args.gcpt_restore) {
    if (args.overwrite_nbytes_autoset) {
      FILE *fp = fopen(args.gcpt_restore, "rb");
      fseek(fp, 4, SEEK_SET);
      fread(&args.overwrite_nbytes, sizeof(uint32_t), 1, fp);
      fclose(fp);
    }
    overwrite_ram(args.gcpt_restore, args.overwrite_nbytes);
  }

  if (args.copy_ram_offset) {
    copy_ram(args.copy_ram_offset);
  }

#ifdef ENABLE_CHISEL_DB
  init_db(args.dump_db, (args.select_db != NULL), args.select_db);
#endif

  if (args.enable_snapshot || args.snapshot_path) {
    dut_ptr->snapshot_init();

    if (args.snapshot_path) {
      Info("loading from snapshot `%s`...\n", args.snapshot_path);
      snapshot_load(args.snapshot_path);
#ifndef CONFIG_NO_DIFFTEST
      auto cycleCnt = difftest[0]->get_trap_event()->cycleCnt;
      Info("model cycleCnt = %" PRIu64 "\n", cycleCnt);
#endif // CONFIG_NO_DIFFTEST
    }
  }

  // set log time range and log level
  dut_ptr->set_log_begin(args.log_begin);
  dut_ptr->set_log_end(args.log_end);

#ifndef CONFIG_NO_DIFFTEST
  // init difftest
  difftest_init();

  // init difftest traces
  if (args.trace_name) {
    for (int i = 0; i < NUM_CORES; i++) {
      difftest[i]->set_trace(args.trace_name, args.trace_is_read);
    }
  }
#endif // CONFIG_NO_DIFFTEST

  init_device();

#ifndef CONFIG_NO_DIFFTEST
  if (args.enable_diff) {
    init_goldenmem();
    size_t ref_ramsize = args.ram_size ? simMemory->get_size() : 0;
    init_nemuproxy(ref_ramsize);
  }
#endif // CONFIG_NO_DIFFTEST
#ifdef ENABLE_RUNAHEAD
  if (args.enable_runahead) {
    runahead_init();
  }
#endif // ENABLE_RUNAHEAD

#ifndef CONFIG_NO_DIFFTEST
#ifdef DEBUG_REFILL
  difftest[0]->save_track_instr(args.track_instr);
#endif
#endif // CONFIG_NO_DIFFTEST

  for (int i = 0; i < NUM_CORES; i++) {
    core_max_instr[i] = args.max_instr;
  }

  //check compiling options for lightSSS
  if (args.enable_fork) {
#ifdef ENABLE_RUNAHEAD
    // Currently, runahead does not work well with fork based snapshot
    assert(!args.enable_runahead);
#endif // ENABLE_RUNAHEAD
    lightsss = new LightSSS;
    FORK_PRINTF("enable fork debugging...\n")
  }

#if VM_COVERAGE == 1
  if (args.dump_coverage) {
    coverage = Verilated::threadContextp()->coveragep();
  }
#endif
}

Emulator::~Emulator() {
  // Simulation ends here, do clean up & display jobs

#if VM_COVERAGE == 1
  // we dump coverage into files at the end
  // since we are not sure when an emu will stop
  // we distinguish multiple dat files by emu start time
  if (args.dump_coverage) {
    save_coverage();
  }
#endif

#ifdef ENABLE_RUNAHEAD
  if (args.enable_runahead) {
    runahead_cleanup(); // remove all checkpoints
  }
#endif // ENABLE_RUNAHEAD

  if (args.enable_fork && !is_fork_child()) {
    bool need_wakeup = trapCode != STATE_GOODTRAP && trapCode != STATE_LIMIT_EXCEEDED && trapCode != STATE_SIG;
    if (need_wakeup) {
      lightsss->wakeup_child(cycles);
    } else {
      lightsss->do_clear();
    }
    delete lightsss;
  }

  // warning: this function may still simulate the circuit
  // simulator resources must be released after this function
  display_stats();

#ifndef CONFIG_NO_DIFFTEST
  stats.update(difftest[0]->dut);
#endif // CONFIG_NO_DIFFTEST

  simMemory->display_stats();
  delete simMemory;
  simMemory = nullptr;

#ifndef CONFIG_NO_DIFFTEST
  if (args.enable_diff) {
    goldenmem_finish();
  }
#endif // CONFIG_NO_DIFFTEST
  flash_finish();
#ifndef CONFIG_NO_DIFFTEST
  difftest_finish();
#endif // CONFIG_NO_DIFFTEST

  if (args.enable_snapshot && trapCode != STATE_GOODTRAP && trapCode != STATE_LIMIT_EXCEEDED) {
    dut_ptr->snapshot_save(-1); // save all snapshots
  }

#ifdef ENABLE_CHISEL_DB
  if (args.dump_db) {
    save_db(logdb_filename());
  }
#endif

  elapsed_time = uptime() - elapsed_time;

  Info(ANSI_COLOR_BLUE "Seed=%d Guest cycle spent: %'" PRIu64
                       " (this will be different from cycleCnt if emu loads a snapshot)\n" ANSI_COLOR_RESET,
       args.seed, cycles);
  Info(ANSI_COLOR_BLUE "Host time spent: %'dms\n" ANSI_COLOR_RESET, elapsed_time);

  if (enable_simjtag) {
    delete jtag;
  }

  delete dut_ptr;
}

inline void Emulator::reset_ncycles(size_t cycles) {
  if (args.trace_name && args.trace_is_read) {
    return;
  }
  for (int i = 0; i < cycles; i++) {
    dut_ptr->set_reset(1);

#ifdef VERILATOR
    dut_ptr->set_clock(1);
    dut_ptr->step();
#endif // VERILATOR

    if (args.enable_waveform && args.enable_waveform_full && args.log_begin == 0) {
      dut_ptr->waveform_tick();
    }

#ifdef VERILATOR
    dut_ptr->set_clock(0);
    dut_ptr->step();
#endif // VERILATOR

    if (args.enable_waveform && args.enable_waveform_full && args.log_begin == 0) {
      dut_ptr->waveform_tick();
    }

#ifdef GSIM
    dut_ptr->step();
#endif

    dut_ptr->set_reset(0);

#ifdef GSIM
    dut_ptr->step();
#endif // GSIM
  }
}

inline void Emulator::single_cycle() {
  if (args.trace_name && args.trace_is_read) {
    goto end_single_cycle;
  }

#ifdef VERILATOR
  dut_ptr->set_clock(1);
  dut_ptr->step();
#endif // VERILATOR

  if (args.enable_waveform) {
#if !defined(CONFIG_NO_DIFFTEST) && !defined(CONFIG_DIFFTEST_SQUASH)
    uint64_t cycle = difftest[0]->get_trap_event()->cycleCnt;
#else
    static uint64_t cycle = -1UL;
    cycle++;
#endif
    bool in_range = (args.log_begin <= cycle) && (cycle <= args.log_end);
    if (in_range || force_dump_wave) {
      dut_ptr->waveform_tick();
    }
  }

#ifdef WITH_DRAMSIM3
  dramsim3_step();
#endif

#ifdef GSIM
  dut_ptr->step();
#endif // GSIM

  dut_ptr->step_uart();

#ifdef VERILATOR
  dut_ptr->set_clock(0);
  dut_ptr->step();
#endif // VERILATOR

  if (args.enable_waveform && args.enable_waveform_full) {
#if !defined(CONFIG_NO_DIFFTEST) && !defined(CONFIG_DIFFTEST_MERGE)
    uint64_t cycle = difftest[0]->get_trap_event()->cycleCnt;
#else
    static uint64_t cycle = -1UL;
    cycle++;
#endif
    bool in_range = (args.log_begin <= cycle) && (cycle <= args.log_end);
    if (in_range || force_dump_wave) {
      dut_ptr->waveform_tick();
    }
  }

end_single_cycle:
  cycles++;
}

int Emulator::tick() {

#ifdef SHOW_SCREEN
  uint32_t t = uptime();
  if (t - lasttime_poll > 100) {
    poll_event();
    lasttime_poll = t;
  }
#endif

  if (args.enable_fork && is_fork_child() && cycles != 0) {
    if (cycles == lightsss->get_end_cycles()) {
      FORK_PRINTF("checkpoint has reached the main process abort point: %lu\n", cycles)
    }
    if (cycles == lightsss->get_end_cycles() + STEP_FORWARD_CYCLES) {
      trapCode = STATE_ABORT;
      return trapCode;
    }
  }

  // cycle limitation
  bool exceed_cycle_limit = false;
#ifdef CONFIG_NO_DIFFTEST
  exceed_cycle_limit = !args.max_cycles;
#else
  for (int i = 0; i < NUM_CORES; i++) {
    auto trap = difftest[i]->get_trap_event();
    if (trap->cycleCnt >= args.max_cycles) {
      exceed_cycle_limit = true;
    }
  }
#endif // CONFIG_NO_DIFFTEST

  if (exceed_cycle_limit) {
    trapCode = STATE_LIMIT_EXCEEDED;
#ifdef FUZZER_LIB
    stats.exit_code = SimExitCode::exceed_limit;
#endif // FUZZER_LIB
    return trapCode;
  }

  // instruction limitation
#ifndef CONFIG_NO_DIFFTEST
  for (int i = 0; i < NUM_CORES; i++) {
    auto trap = difftest[i]->get_trap_event();
    if (trap->instrCnt >= core_max_instr[i]) {
      trapCode = STATE_LIMIT_EXCEEDED;
#ifdef FUZZER_LIB
      stats.exit_code = SimExitCode::exceed_limit;
#endif // FUZZER_LIB
      return trapCode;
    }
  }
#endif // CONFIG_NO_DIFFTEST
  // assertions
  if (assert_count > 0) {
    Info("The simulation stopped. There might be some assertion failed.\n");
    trapCode = STATE_ABORT;
    return trapCode;
  }
  // signals
  if (signal_num != 0) {
    trapCode = STATE_SIG;
  }

  // exit signal: non-zero exit exits the simulation. exit all 1's indicates good.
  uint64_t difftest_exit = dut_ptr->get_difftest_exit();
  if (difftest_exit) {
    if (difftest_exit == -1UL) {
      trapCode = STATE_SIM_EXIT;
    } else {
      Info("The simulation aborted via the top-level exit of 0x%lx.\n", difftest_exit);
      trapCode = STATE_ABORT;
    }
  }

  if (trapCode != STATE_RUNNING) {
    return trapCode;
  }
#ifndef CONFIG_NO_DIFFTEST
  for (int i = 0; i < NUM_CORES; i++) {
    auto trap = difftest[i]->get_trap_event();
    if (trap->instrCnt >= args.warmup_instr) {
      Info("Warmup finished. The performance counters will be dumped and then reset.\n");
      dut_ptr->set_perf_clean(1);
      dut_ptr->set_perf_dump(1);
      args.warmup_instr = -1;
    }
    if (trap->cycleCnt % args.stat_cycles == args.stat_cycles - 1) {
      dut_ptr->set_perf_clean(1);
      dut_ptr->set_perf_dump(1);
    }
#ifdef ENABLE_IPC
    if (trap->instrCnt >= args.ipc_times * args.ipc_interval &&
        args.ipc_last_instr < args.ipc_times * args.ipc_interval) {
      fprintf(args.ipc_file, "%d %f\n", args.ipc_times * args.ipc_interval,
              (float)args.ipc_interval / (cycles - args.ipc_last_cycle));
      args.ipc_times++;
      args.ipc_last_instr = trap->instrCnt;
      args.ipc_last_cycle = cycles;
    }
#endif
    if (args.enable_ref_trace) {
      bool is_debug = difftest[i]->proxy->get_debug();
      if (trap->cycleCnt >= args.log_begin && !is_debug) {
        difftest[i]->proxy->set_debug(true);
      }
      if (trap->cycleCnt >= args.log_end && is_debug) {
        difftest[i]->proxy->set_debug(false);
      }
    }
    if (args.enable_commit_trace) {
      bool is_commit_trace = difftest[i]->get_commit_trace();
      if (trap->cycleCnt >= args.log_begin && !is_commit_trace) {
        difftest[i]->set_commit_trace(true);
      }
      if (trap->cycleCnt >= args.log_end && is_commit_trace) {
        difftest[i]->set_commit_trace(false);
      }
    }
  }
#endif // CONFIG_NO_DIFFTEST

  single_cycle();
#ifdef CONFIG_NO_DIFFTEST
  args.max_cycles--;
#endif // CONFIG_NO_DIFFTEST

  dut_ptr->set_perf_clean(0);
  dut_ptr->set_perf_dump(0);

#ifndef CONFIG_NO_DIFFTEST
  int step = 0;
  if (args.trace_name && args.trace_is_read) {
    step = 1;
    difftest_trace_read();
  } else {
    step = dut_ptr->get_difftest_step();
  }

  static uint64_t stuck_timer = 0;
  if (step) {
    stuck_timer = 0;
  } else {
    stuck_timer++;
    if (stuck_timer >= Difftest::stuck_limit) {
      Info("No difftest check for more than %lu cycles, maybe get stuck.", Difftest::stuck_limit);
      return STATE_ABORT;
    }
  }

  if (args.trace_name && !args.trace_is_read) {
    difftest_trace_write(step);
  }

  trapCode = difftest_nstep(step, args.enable_diff);

  if (trapCode != STATE_RUNNING) {
#ifdef FUZZER_LIB
    if (trapCode == STATE_GOODTRAP) {
      stats.exit_code = SimExitCode::good_trap;
    } else if (trapCode != STATE_FUZZ_COND && trapCode != STATE_SIM_EXIT) {
      stats.exit_code = SimExitCode::bad_trap;
    } else if (stats.exit_code == SimExitCode::unknown) {
      stats.exit_code = SimExitCode::bad_trap;
    }
#endif
    return trapCode;
  }
#endif // CONFIG_NO_DIFFTEST

#ifdef ENABLE_RUNAHEAD
  if (args.enable_runahead) {
    runahead_step();
  }
#endif // ENABLE_RUNAHEAD

  if (args.enable_snapshot) {
    static int snapshot_count = 0;
    uint32_t t = uptime();
    if (trapCode != STATE_GOODTRAP && t - lasttime_snapshot > 1000 * SNAPSHOT_INTERVAL) {
      // save snapshot every 60s
      snapshot_save();
      lasttime_snapshot = t;
      // dump one snapshot to file every 60 snapshots
      snapshot_count++;
      if (snapshot_count == 60) {
        dut_ptr->snapshot_save(0);
        snapshot_count = 0;
      }
    }
  }

#ifdef DEBUG_TILELINK
  if (args.dump_tl_interval != 0) {
    if ((cycles != 0) && (cycles % args.dump_tl_interval == 0)) {
      checkpoint_db(logdb_filename());
    }
  }
#endif

#ifdef ENABLE_IPC
  fclose(args.ipc_file);
#endif

  if (args.enable_fork) {
    static bool have_initial_fork = false;
    uint32_t timer = uptime();
    // check if it's time to fork a checkpoint process
    if (((timer - lasttime_snapshot > args.fork_interval) || !have_initial_fork) && !is_fork_child()) {
      have_initial_fork = true;
      lasttime_snapshot = timer;
      switch (lightsss->do_fork()) {
        case FORK_ERROR: return -1;
        case FORK_CHILD: fork_child_init();
        default: break;
      }
    }
  }
  return 0;
}

int Emulator::is_finished() {
  return
#ifdef VERILATOR
      Verilated::gotFinish() ||
#endif // VERILATOR
      trapCode != STATE_RUNNING;
}

int Emulator::is_good() {
  return is_good_trap();
}

#if VM_COVERAGE == 1
void Emulator::save_coverage() {
  const char *p = create_noop_filename(".coverage.dat");
  Info("dump coverage data to %s...\n", p);
  coverage->write(p);
}
#endif

void Emulator::trigger_stat_dump() {
  dut_ptr->set_perf_dump(1);
  if (get_args().force_dump_result) {
    dut_ptr->set_log_end(-1);
  }
  single_cycle();
}

void Emulator::display_stats() {
#ifndef CONFIG_NO_DIFFTEST
  for (int i = 0; i < NUM_CORES; i++) {
    printf("Core %d: ", i);
    uint64_t pc = difftest[i]->get_trap_event()->pc;
    switch (trapCode) {
      case STATE_GOODTRAP:
        eprintf(ANSI_COLOR_GREEN "HIT GOOD TRAP at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case STATE_BADTRAP: eprintf(ANSI_COLOR_RED "HIT BAD TRAP at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc); break;
      case STATE_ABORT: eprintf(ANSI_COLOR_RED "ABORT at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc); break;
      case STATE_LIMIT_EXCEEDED:
        eprintf(ANSI_COLOR_YELLOW "EXCEEDING CYCLE/INSTR LIMIT at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case STATE_SIG:
        eprintf(ANSI_COLOR_YELLOW "SOME SIGNAL STOPS THE PROGRAM at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case STATE_SIM_EXIT: eprintf(ANSI_COLOR_YELLOW "EXIT at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc); break;
      default: eprintf(ANSI_COLOR_RED "Unknown trap code: %d\n", trapCode);
    }

    difftest[i]->display_stats();

#ifdef TRACE_INFLIGHT_MEM_INST
    runahead[i]->memdep_watcher->print_pred_matrix();
#endif
  }
#endif // CONFIG_NO_DIFFTEST

  if (trapCode != STATE_ABORT) {
    trigger_stat_dump();
  }
}

void Emulator::snapshot_save() {
  auto snapshot_write = dut_ptr->snapshot_take();

  long size = simMemory->get_size();
  snapshot_write(&size, sizeof(size));
  if (!simMemory->as_ptr()) {
    printf("simMemory does not support as_ptr\n");
    assert(0);
  }
  snapshot_write(simMemory->as_ptr(), size);

#ifndef CONFIG_NO_DIFFTEST
  auto diff = difftest[0];
  uint64_t cycleCnt = diff->get_trap_event()->cycleCnt;
  snapshot_write(&cycleCnt, sizeof(cycleCnt));

  auto proxy = diff->proxy;
  snapshot_write(&proxy->state, sizeof(proxy->state));

  char *buf = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  proxy->mem_init(PMEM_BASE, buf, size, REF_TO_DUT);
  snapshot_write(buf, size);
  munmap(buf, size);

  uint64_t csr_buf[4096];
  proxy->ref_csrcpy(csr_buf, REF_TO_DUT);
  snapshot_write(&csr_buf, sizeof(csr_buf));
#endif // CONFIG_NO_DIFFTEST

  long sdcard_offset;
  if (fp)
    sdcard_offset = ftell(fp);
  else
    sdcard_offset = 0;
  snapshot_write(&sdcard_offset, sizeof(sdcard_offset));
}

void Emulator::snapshot_load(const char *filename) {
  auto snapshot_read = dut_ptr->snapshot_load(filename);

  long size;
  snapshot_read(&size, sizeof(size));
  assert(size == simMemory->get_size());
  if (!simMemory->as_ptr()) {
    printf("simMemory does not support as_ptr\n");
    assert(0);
  }
  snapshot_read(simMemory->as_ptr(), size);

#ifndef CONFIG_NO_DIFFTEST
  auto diff = difftest[0];
  uint64_t *cycleCnt = &(diff->get_trap_event()->cycleCnt);
  snapshot_read(cycleCnt, sizeof(*cycleCnt));

  auto proxy = diff->proxy;
  snapshot_read(&proxy->state, sizeof(proxy->state));
  proxy->ref_regcpy(&proxy->state, DUT_TO_REF, false);

  char *buf = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  snapshot_read(buf, size);
  proxy->mem_init(PMEM_BASE, buf, size, DUT_TO_REF);
  munmap(buf, size);

  uint64_t csr_buf[4096];
  snapshot_read(&csr_buf, sizeof(csr_buf));
  proxy->ref_csrcpy(csr_buf, DUT_TO_REF);

  // No one uses snapshot when !has_commit, isn't it?
  diff->has_commit = 1;
#endif // CONFIG_NO_DIFFTEST

  long sdcard_offset = 0;
  snapshot_read(&sdcard_offset, sizeof(sdcard_offset));

  if (fp)
    fseek(fp, sdcard_offset, SEEK_SET);
}

void Emulator::fork_child_init() {
  dut_ptr->atClone();

  FORK_PRINTF("the oldest checkpoint start to dump wave and dump nemu log...\n")

  dut_ptr->waveform_init(args.enable_waveform_full ? 2 * cycles : cycles);
  // override output range config, force dump wave
  force_dump_wave = true;
  args.enable_waveform = true;

#ifndef CONFIG_NO_DIFFTEST
#ifdef ENABLE_SIMULATOR_DEBUG_INFO
  // let simulator print debug info
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->proxy->set_debug(true);
  }
#endif
#endif // CONFIG_NO_DIFFTEST
}
