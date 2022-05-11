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

#include "emu.h"
#include "device.h"
#include "sdcard.h"
#include "difftest.h"
#include "runahead.h"
#include "refproxy.h"
#include "goldenmem.h"
#include "flash.h"
#include "runahead.h"
#include <getopt.h>
#include <signal.h>
#ifdef  DEBUG_TILELINK
#include "tllogger.h"
#endif
#include "ram.h"
#include "zlib.h"
#include "compress.h"
#include "lightsss.h"
#include "remote_bitbang.h"

extern remote_bitbang_t * jtag;


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
  printf("  -b, --log-begin=NUM        display log from NUM th cycle\n");
  printf("  -e, --log-end=NUM          stop display log at NUM th cycle\n");
#ifdef DEBUG_REFILL
  printf("  -T, --track-instr=ADDR     track refill action concerning ADDR\n");
#endif
  printf("      --force-dump-result    force dump performance counter result in the end\n");
  printf("      --load-snapshot=PATH   load snapshot from PATH\n");
  printf("      --no-snapshot          disable saving snapshots\n");
  printf("      --dump-wave            dump waveform when log is enabled\n");
#ifdef DEBUG_TILELINK
  printf("      --dump-tl              dump tilelink transactions\n");
  printf("      --flash                the flash bin file for simulation\n");
#endif
  printf("      --sim-run-ahead        let a fork of simulator run ahead of commit for perf analysis\n");
  printf("      --wave-path=FILE       dump waveform to a specified PATH\n");
  printf("      --ram-size=SIZE        simulation memory size, for example 8GB / 128MB\n");
  printf("      --enable-fork          enable folking child processes to debug\n");
  printf("      --no-diff              disable differential testing\n");
  printf("      --diff=PATH            set the path of REF for differential testing\n");
  printf("      --enable-jtag          enable remote bitbang server\n");
  printf("  -h, --help                 print program help info\n");
  printf("\n");
}

inline EmuArgs parse_args(int argc, const char *argv[]) {
  EmuArgs args;
  int long_index = 0;
  extern const char *difftest_ref_so;
  const struct option long_options[] = {
    { "load-snapshot",     1, NULL,  0  },
    { "dump-wave",         0, NULL,  0  },
    { "no-snapshot",       0, NULL,  0  },
    { "force-dump-result", 0, NULL,  0  },
    { "diff",              1, NULL,  0  },
    { "no-diff",           0, NULL,  0  },
    { "enable-fork",       0, NULL,  0  },
    { "enable-jtag",       0, NULL,  0  },
    { "wave-path",         1, NULL,  0  },
    { "ram-size",          1, NULL,  0  },
    { "sim-run-ahead",     0, NULL,  0  },
#ifdef DEBUG_TILELINK
    { "dump-tl",           0, NULL,  0  },
#endif
    { "seed",              1, NULL, 's' },
    { "max-cycles",        1, NULL, 'C' },
    { "max-instr",         1, NULL, 'I' },
#ifdef DEBUG_REFILL
    { "track-instr",       1, NULL, 'T' },
#endif
    { "warmup-instr",      1, NULL, 'W' },
    { "stat-cycles",       1, NULL, 'D' },
    { "image",             1, NULL, 'i' },
    { "log-begin",         1, NULL, 'b' },
    { "log-end",           1, NULL, 'e' },
    { "flash",             1, NULL, 'F' },
    { "help",              0, NULL, 'h' },
    { 0,                   0, NULL,  0  }
  };

  int o;
  while ( (o = getopt_long(argc, const_cast<char *const*>(argv),
          "-s:C:I:T:W:hi:m:b:e:F:", long_options, &long_index)) != -1) {
    switch (o) {
      case 0:
        switch (long_index) {
          case 0: args.snapshot_path = optarg; continue;
          case 1: args.enable_waveform = true; continue;
          case 2: args.enable_snapshot = false; continue;
          case 3: args.force_dump_result = true; continue;
          case 4: difftest_ref_so = optarg; continue;
          case 5: args.enable_diff = false; continue;
          case 6: args.enable_fork = true; continue;
          case 7: args.enable_jtag = true; continue;
          case 8: args.wave_path = optarg; continue;
          case 9: args.ram_size = optarg; continue;
          case 10:
#ifdef ENABLE_RUNHEAD
            args.enable_runahead = true;
#else
            printf("[WARN] runahead is not enabled at compile time, ignore --sim-run-ahead\n");
#endif
            continue;
          case 11:
#ifdef DEBUG_TILELINK
            args.dump_tl = true;
#else
            printf("[WARN] debug tilelink is not enabled at compile time, ignore --dump-tl\n");
#endif
            continue;
        }
        // fall through
      default:
        print_help(argv[0]);
        exit(0);
      case 's':
        if(std::string(optarg) != "NO_SEED") {
          args.seed = atoll_strict(optarg, "seed");
          printf("Using seed = %d\n", args.seed);
        }
        break;
      case 'C': args.max_cycles = atoll_strict(optarg, "max-cycles");  break;
      case 'I': args.max_instr = atoll_strict(optarg, "max-instr");  break;
#ifdef DEBUG_REFILL
      case 'T':
        args.track_instr = std::strtoll(optarg, NULL, 0);
        printf("Tracking addr 0x%lx\n", args.track_instr);
        if(args.track_instr == 0) {
          printf("Invalid track addr\n");
          exit(1);
        }
        break;
#endif
      case 'W': args.warmup_instr = atoll_strict(optarg, "warmup-instr");  break;
      case 'D': args.stat_cycles = atoll_strict(optarg, "stat-cycles");  break;
      case 'i': args.image = optarg; break;
      case 'b': args.log_begin = atoll_strict(optarg, "log-begin");  break;
      case 'e': args.log_end = atoll_strict(optarg, "log-end"); break;
      case 'F': args.flash_bin = optarg; break;
    }
  }

  if(args.image == NULL) {
    print_help(argv[0]);
    printf("Hint: --image=IMAGE_FILE must be given\n");
    exit(0);
  }

  if(args.ram_size){
    parse_and_update_ramsize(args.ram_size);
  }

  Verilated::commandArgs(argc, argv); // Prepare extra args for TLMonitor
  return args;
}


Emulator::Emulator(int argc, const char *argv[]):
  dut_ptr(new VSimTop),
  cycles(0), trapCode(STATE_RUNNING)
{
  args = parse_args(argc, argv);

  // srand
  srand(args.seed);
  srand48(args.seed);
  Verilated::randReset(2);
  assert_init();

  // init remote-bitbang
  enable_simjtag = args.enable_jtag;
  if (args.enable_jtag) {
    jtag = new remote_bitbang_t(23334);
  }
  // init flash
  init_flash(args.flash_bin);

  // init core
  reset_ncycles(10);

  // init ram
  init_ram(args.image);

#ifdef DEBUG_TILELINK
  // init logger
  init_logger(args.dump_tl);
#endif

#if VM_TRACE == 1
  enable_waveform = args.enable_waveform && !args.enable_fork;
  if (enable_waveform ) {
    Verilated::traceEverOn(true);	// Verilator must compute traced signals
    tfp = new VerilatedVcdC;
    dut_ptr->trace(tfp, 99);	// Trace 99 levels of hierarchy
    if (args.wave_path != NULL) {
      tfp->open(args.wave_path);
    }
    else {
      time_t now = time(NULL);
      tfp->open(waveform_filename(now));	// Open the dump file
    }
  }
#endif

#ifdef VM_SAVABLE
  if (args.snapshot_path != NULL) {
    printf("loading from snapshot `%s`...\n", args.snapshot_path);
    snapshot_load(args.snapshot_path);
    auto cycleCnt = difftest[0]->get_trap_event()->cycleCnt;
    printf("model cycleCnt = %" PRIu64 "\n", cycleCnt);
  }
#endif

  // set log time range and log level
  dut_ptr->io_logCtrl_log_begin = args.log_begin;
  dut_ptr->io_logCtrl_log_end = args.log_end;
}

Emulator::~Emulator() {
  ram_finish();
  assert_finish();

#ifdef VM_SAVABLE
  if (args.enable_snapshot && trapCode != STATE_GOODTRAP && trapCode != STATE_LIMIT_EXCEEDED) {
    printf("Saving snapshots to file system. Please wait.\n");
    snapshot_slot[0].save();
    snapshot_slot[1].save();
    printf("Please remove unused snapshots manually\n");
  }
#endif

#ifdef DEBUG_TILELINK
  if(args.dump_tl){
    time_t now = time(NULL);
    save_db(logdb_filename(now));
  }
#endif
}

inline void Emulator::reset_ncycles(size_t cycles) {
  for(int i = 0; i < cycles; i++) {
    dut_ptr->reset = 1;
    dut_ptr->clock = 0;
    dut_ptr->eval();
    dut_ptr->clock = 1;
    dut_ptr->eval();
    dut_ptr->reset = 0;
  }
}

inline void Emulator::single_cycle() {
  dut_ptr->clock = 0;
  dut_ptr->eval();

#ifdef WITH_DRAMSIM3
  axi_channel axi;
  axi_copy_from_dut_ptr(dut_ptr, axi);
  axi.aw.addr -= PMEM_BASE;
  axi.ar.addr -= PMEM_BASE;
  dramsim3_helper_rising(axi);
#endif

#if VM_TRACE == 1
  if (enable_waveform) {
    auto trap = difftest[0]->get_trap_event();
    uint64_t cycle = trap->cycleCnt;
    uint64_t begin = dut_ptr->io_logCtrl_log_begin;
    uint64_t end   = dut_ptr->io_logCtrl_log_end;
    bool in_range  = (begin <= cycle) && (cycle <= end);
    if (in_range || force_dump_wave) { tfp->dump(cycle); }
  }
#endif

  dut_ptr->clock = 1;
  dut_ptr->eval();

#ifdef WITH_DRAMSIM3
  axi_copy_from_dut_ptr(dut_ptr, axi);
  axi.aw.addr -= PMEM_BASE;
  axi.ar.addr -= PMEM_BASE;
  dramsim3_helper_falling(axi);
  axi_set_dut_ptr(dut_ptr, axi);
#endif

  if (dut_ptr->io_uart_out_valid) {
    printf("%c", dut_ptr->io_uart_out_ch);
    fflush(stdout);
  }
  if (dut_ptr->io_uart_in_valid) {
    extern uint8_t uart_getc();
    dut_ptr->io_uart_in_ch = uart_getc();
  }
  cycles ++;
}

uint64_t Emulator::execute(uint64_t max_cycle, uint64_t max_instr) {

  difftest_init();
  init_device();
  if (args.enable_diff) {
    init_goldenmem();
    size_t ref_ramsize = args.ram_size ? EMU_RAM_SIZE : 0;
    init_nemuproxy(ref_ramsize);
  }
  if(args.enable_runahead){
    runahead_init();
  }

#ifdef DEBUG_REFILL
  difftest[0]->save_track_instr(args.track_instr);
#endif

  uint32_t lasttime_poll = 0;
  uint32_t lasttime_snapshot = 0;
  uint64_t core_max_instr[NUM_CORES];
  for (int i = 0; i < NUM_CORES; i++) {
    core_max_instr[i] = max_instr;
  }

  uint32_t t = uptime();
  if (t - lasttime_poll > 100) {
    poll_event();
    lasttime_poll = t;
  }

  //check compiling options for lightSSS
  if (args.enable_fork) {
    // Currently, runahead does not work well with fork based snapshot
    assert(!args.enable_runahead);
    FORK_PRINTF("enable fork debugging...\n")
  }

#if VM_COVERAGE == 1
  // we dump coverage into files at the end
  // since we are not sure when an emu will stop
  // we distinguish multiple dat files by emu start time
  time_t coverage_start_time = time(NULL);
#endif

  while (!Verilated::gotFinish() && trapCode == STATE_RUNNING) {
    if (is_fork_child() && cycles != 0 && cycles == lightsss.get_end_cycles()) {
      FORK_PRINTF("checkpoint has reached the main process abort point: %lu\n", cycles)
    }
    if (is_fork_child() && cycles != 0 && cycles == lightsss.get_end_cycles() + STEP_FORWARD_CYCLES) {
      trapCode = STATE_ABORT;
      break;
    }

    if (!max_cycle) {
      trapCode = STATE_LIMIT_EXCEEDED;
      break;
    }
    // instruction limitation
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      if (trap->instrCnt >= core_max_instr[i]) {
        trapCode = STATE_LIMIT_EXCEEDED;
        break;
      }
    }
    // assertions
    if (assert_count > 0) {
      eprintf("The simulation stopped. There might be some assertion failed.\n");
      trapCode = STATE_ABORT;
      break;
    }
    // signals
    if (signal_num != 0) {
      trapCode = STATE_SIG;
    }

    if (trapCode != STATE_RUNNING) {
      break;
    }
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      if (trap->instrCnt >= args.warmup_instr) {
        printf("Warmup finished. The performance counters will be dumped and then reset.\n");
        dut_ptr->io_perfInfo_clean = 1;
        dut_ptr->io_perfInfo_dump = 1;
        args.warmup_instr = -1;
      }
      if (trap->cycleCnt % args.stat_cycles == args.stat_cycles - 1) {
        dut_ptr->io_perfInfo_clean = 1;
        dut_ptr->io_perfInfo_dump = 1;
      }
    }

    single_cycle();

    max_cycle --;
    dut_ptr->io_perfInfo_clean = 0;
    dut_ptr->io_perfInfo_dump = 0;

    trapCode = difftest_state();
    if (trapCode != STATE_RUNNING) {
      break;
    } 

    if (args.enable_diff) {
      if (difftest_step()) {
        trapCode = STATE_ABORT;
        break;
      }
    }

    if (args.enable_runahead) {
      runahead_step();
    }

#ifdef VM_SAVABLE
    static int snapshot_count = 0;
    if (args.enable_snapshot && trapCode != STATE_GOODTRAP && t - lasttime_snapshot > 1000 * SNAPSHOT_INTERVAL) {
      // save snapshot every 60s
      time_t now = time(NULL);
      snapshot_save(snapshot_filename(now));
      lasttime_snapshot = t;
      // dump one snapshot to file every 60 snapshots
      snapshot_count++;
      if (snapshot_count == 60) {
        snapshot_slot[0].save();
        snapshot_count = 0;
      }
    }
#endif

    if (args.enable_fork) {
      static bool have_initial_fork = false;
      uint32_t timer = uptime();
      //check if it's time to fork a checkpoint process
      if (((timer - lasttime_snapshot > 1000 * FORK_INTERVAL) || !have_initial_fork) && !is_fork_child()) {
        have_initial_fork = true;
        lasttime_snapshot = timer;
        switch (lightsss.do_fork()) {
          case FORK_ERROR: return -1;
          case WAIT_EXIT: exit(0);
          case WAIT_LAST: fork_child_init();
          default: break;
        }
      }
    }
  }
  // Simulation ends here, do clean up & display jobs
#if VM_TRACE == 1
  if (enable_waveform) tfp->close();
#endif

#if VM_COVERAGE == 1
  save_coverage(coverage_start_time);
#endif

  if(args.enable_runahead){
    runahead_cleanup(); // remove all checkpoints
  }

  if (args.enable_fork) {
    bool need_wakeup = trapCode != STATE_GOODTRAP && trapCode != STATE_LIMIT_EXCEEDED && trapCode != STATE_SIG;
    if (need_wakeup) {
      lightsss.wakeup_child(cycles);
    }
    printf("*************** ");
    printf("%s", is_fork_child() ? "CHECHPOINT" : "MAIN");
    printf(" INFO START (PID %d) ***************\n", getpid());
    //when reach maximum instruction, clear the checkpoint process
    if (!is_fork_child()) {
      lightsss.do_clear();
    }
  }

  display_trapinfo();

  return cycles;
}

void parse_and_update_ramsize(const char* arg_ramsize_str) {
  unsigned long ram_size_value = 0;
  char ram_size_unit[64];
  sscanf(arg_ramsize_str, "%ld%s", &ram_size_value, (char*) &ram_size_unit);
  assert(ram_size_value > 0);
  
  if(!strcmp(ram_size_unit, "GB") || !strcmp(ram_size_unit, "gb")){
    EMU_RAM_SIZE = ram_size_value * 1024 * 1024 * 1024;
    return;
  }
  if(!strcmp(ram_size_unit, "MB") || !strcmp(ram_size_unit, "mb")){
    EMU_RAM_SIZE = ram_size_value * 1024 * 1024;
    return;
  }
  printf("Invalid ram size %s\n", ram_size_unit);
  assert(0);
}

inline char* Emulator::timestamp_filename(time_t t, char *buf) {
  char buf_time[64];
  strftime(buf_time, sizeof(buf_time), "%F@%T", localtime(&t));
  char *noop_home = getenv("NOOP_HOME");
  assert(noop_home != NULL);
  int len = snprintf(buf, 1024, "%s/build/%s", noop_home, buf_time);
  return buf + len;
}

#ifdef VM_SAVABLE
inline char* Emulator::snapshot_filename(time_t t) {
  static char buf[1024];
  char *p = timestamp_filename(t, buf);
  strcpy(p, ".snapshot");
  return buf;
}
#endif

inline char* Emulator::logdb_filename(time_t t) {
  static char buf[1024];
  char *p = timestamp_filename(t, buf);
  strcpy(p, ".db");
  return buf;
}

inline char* Emulator::waveform_filename(time_t t) {
  static char buf[1024];
  char *p = timestamp_filename(t, buf);
  strcpy(p, ".vcd");
  printf("dump wave to %s...\n", buf);
  return buf;
}

inline char* Emulator::cycle_wavefile(uint64_t cycles, time_t t) {
  static char buf[1024];
  char buf_time[64];
  strftime(buf_time, sizeof(buf_time), "%F@%T", localtime(&t));
  char *noop_home = getenv("NOOP_HOME");
  assert(noop_home != NULL);
  int len = snprintf(buf, 1024, "%s/build/%s_%ld", noop_home, buf_time, cycles);
  strcpy(buf + len, ".vcd");
  FORK_PRINTF("dump wave to %s...\n", buf);
  return buf;
}


#if VM_COVERAGE == 1
inline char* Emulator::coverage_filename(time_t t) {
  static char buf[1024];
  char *p = timestamp_filename(t, buf);
  strcpy(p, ".coverage.dat");
  return buf;
}

inline void Emulator::save_coverage(time_t t) {
  char *p = coverage_filename(t);
  VerilatedCov::write(p);
}
#endif

void Emulator::trigger_stat_dump() {
  dut_ptr->io_perfInfo_dump = 1;
  if(get_args().force_dump_result) {
    dut_ptr->io_logCtrl_log_end = -1;
  }
  single_cycle();
}

void Emulator::display_trapinfo() {
  for (int i = 0; i < NUM_CORES; i++) {
    printf("Core %d: ", i);
    auto trap = difftest[i]->get_trap_event();
    uint64_t pc = trap->pc;
    uint64_t instrCnt = trap->instrCnt;
    uint64_t cycleCnt = trap->cycleCnt;

    switch (trapCode) {
      case STATE_GOODTRAP:
        eprintf(ANSI_COLOR_GREEN "HIT GOOD TRAP at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case STATE_BADTRAP:
        eprintf(ANSI_COLOR_RED "HIT BAD TRAP at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case STATE_ABORT:
        eprintf(ANSI_COLOR_RED "ABORT at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case STATE_LIMIT_EXCEEDED:
        eprintf(ANSI_COLOR_YELLOW "EXCEEDING CYCLE/INSTR LIMIT at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case STATE_SIG:
        eprintf(ANSI_COLOR_YELLOW "SOME SIGNAL STOPS THE PROGRAM at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      default:
        eprintf(ANSI_COLOR_RED "Unknown trap code: %d\n", trapCode);
    }

    double ipc = (double)instrCnt / cycleCnt;
    eprintf(ANSI_COLOR_MAGENTA "total guest instructions = %'" PRIu64 "\n" ANSI_COLOR_RESET, instrCnt);
    eprintf(ANSI_COLOR_MAGENTA "instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64 ", IPC = %lf\n" ANSI_COLOR_RESET,
        instrCnt, cycleCnt, ipc);
  #ifdef TRACE_INFLIGHT_MEM_INST
    runahead[i]->memdep_watcher->print_pred_matrix();
  #endif

  }

  if (trapCode != STATE_ABORT) {
    trigger_stat_dump();
  }
}

#ifdef VM_SAVABLE
void Emulator::snapshot_save(const char *filename) {
  static int last_slot = 0;
  VerilatedSaveMem &stream = snapshot_slot[last_slot];
  last_slot = !last_slot;

  stream.init(filename);
  stream << *dut_ptr;
  stream.flush();

  long size = get_ram_size();
  stream.unbuf_write(&size, sizeof(size));
  stream.unbuf_write(get_ram_start(), size);

  auto diff = difftest[0];
  uint64_t cycleCnt = diff->get_trap_event()->cycleCnt;
  stream.unbuf_write(&cycleCnt, sizeof(cycleCnt));

  auto proxy = diff->proxy;

  uint64_t ref_r[DIFFTEST_NR_REG];
  proxy->regcpy(&ref_r, REF_TO_DUT);
  stream.unbuf_write(ref_r, sizeof(ref_r));

  char *buf = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  proxy->memcpy(PMEM_BASE, buf, size, DIFFTEST_TO_DUT);
  stream.unbuf_write(buf, size);
  munmap(buf, size);

  struct SyncState sync_mastate;
  proxy->uarchstatus_cpy(&sync_mastate, REF_TO_DUT);
  stream.unbuf_write(&sync_mastate, sizeof(struct SyncState));

  uint64_t csr_buf[4096];
  proxy->csrcpy(csr_buf, REF_TO_DIFFTEST);
  stream.unbuf_write(&csr_buf, sizeof(csr_buf));

  long sdcard_offset;
  if(fp)
    sdcard_offset = ftell(fp);
  else
    sdcard_offset = 0;
  stream.unbuf_write(&sdcard_offset, sizeof(sdcard_offset));

  // actually write to file in snapshot_finalize()
}

void Emulator::snapshot_load(const char *filename) {
  VerilatedRestoreMem stream;
  stream.open(filename);
  stream >> *dut_ptr;

  long size;
  stream.read(&size, sizeof(size));
  assert(size == get_ram_size());
  stream.read(get_ram_start(), size);

  auto diff = difftest[0];
  uint64_t *cycleCnt = &(diff->get_trap_event()->cycleCnt);
  stream.read(cycleCnt, sizeof(*cycleCnt));

  auto proxy = diff->proxy;

  uint64_t ref_r[DIFFTEST_NR_REG];
  stream.read(ref_r, sizeof(ref_r));
  proxy->regcpy(&ref_r, DUT_TO_REF);

  char *buf = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  stream.read(buf, size);
  proxy->memcpy(PMEM_BASE, buf, size, DIFFTEST_TO_REF);
  munmap(buf, size);

  struct SyncState sync_mastate;
  stream.read(&sync_mastate, sizeof(struct SyncState));
  proxy->uarchstatus_cpy(&sync_mastate, DUT_TO_REF);

  uint64_t csr_buf[4096];
  stream.read(&csr_buf, sizeof(csr_buf));
  proxy->csrcpy(csr_buf, DIFFTEST_TO_REF);

  long sdcard_offset = 0;
  stream.read(&sdcard_offset, sizeof(sdcard_offset));

  if(fp)
    fseek(fp, sdcard_offset, SEEK_SET);

  // No one uses snapshot when !has_commit, isn't it?
  diff->has_commit = 1;
}
#endif

void Emulator::fork_child_init() {
#if EMU_THREAD > 1
#ifdef VERILATOR_4_210
  dut_ptr->vlSymsp->__Vm_threadPoolp = new VlThreadPool(dut_ptr->contextp(), EMU_THREAD - 1, 0);
#else
  dut_ptr->__Vm_threadPoolp = new VlThreadPool(dut_ptr->contextp(), EMU_THREAD - 1, 0);
#endif
#endif
  FORK_PRINTF("the oldest checkpoint start to dump wave and dump nemu log...\n")
#if VM_TRACE == 1
  //dump wave
  Verilated::traceEverOn(true);
  tfp = new VerilatedVcdC;
  dut_ptr->trace(tfp, 99);
  time_t now = time(NULL);
  tfp->open(cycle_wavefile(cycles, now));
  // override output range config, force dump wave
  force_dump_wave = true;
  enable_waveform = true;
#endif
#ifdef ENABLE_SIMULATOR_DEBUG_INFO
  // let simulator print debug info
  DynamicSimulatorConfig nemu_config;
  nemu_config.debug_difftest = true;
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->proxy->update_config(&nemu_config);
  }
#endif
}
