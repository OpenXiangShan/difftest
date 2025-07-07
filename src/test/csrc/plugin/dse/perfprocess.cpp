#include "perfprocess.h"

Perfprocess::Perfprocess(VSimTop *dut_ptr, int commit_width) {
  this->dut_ptr = dut_ptr;
  this->commit_width = commit_width;
  this->perfNames = getIOPerfNames();
}

Perfprocess::~Perfprocess() {
}

uint64_t Perfprocess::find_perfCnt(std::string perfName) {
  std::vector<uint64_t> perfCnts = getIOPerfCnts(dut_ptr);
  // Find the index of the perfName in perfNames
  auto it = std::find(perfNames.begin(), perfNames.end(), perfName);
  if (it != perfNames.end()) {
    // Calculate the index
    size_t index = std::distance(perfNames.begin(), it);
    // Return the corresponding value from perfCnts
    return perfCnts[index];
  } else {
    throw std::runtime_error("perfName not found in perfNames");
  }
  return perfCnts[1];
}

double Perfprocess::get_ipc() {
  auto clockCnt = find_perfCnt("clock_cycle");
  auto instrCnt = find_perfCnt("commitInstr");
  return (double)instrCnt / (double)clockCnt;
}

double Perfprocess::get_cpi() {
  auto clockCnt = find_perfCnt("clock_cycle");
  auto instrCnt = find_perfCnt("commitInstr");
  return (double)clockCnt / (double)instrCnt;
}

void Perfprocess::update_deg() {
  for (int i = 0; i < commit_width; i++) {
    auto do_update = find_perfCnt("isCommit_" + std::to_string(i));
    if (do_update != 0) {
      printf("%ld: system.switch_cpus: T0 : 0x%lx : 0x%lx"
          " : %ld_%ld_%ld : _"
          " : FetchCacheLine=%ld : ProcessCacheCompletion=%ld"

          " : Fetch=%ld"
          " : Decode=%ld : Rename=%ld"

          " : BlockFromROB=%ld"
          " : BlockFromDPQ=%ld"
          " : BlockFromSerial=%ld"
          " : BlockFromRF=%ld"
          " : BlockFromLQ=%ld : BlockFromSQ=%ld"

          " : EliminatedMove=%ld"
          " : Dispatch=%ld : EnqRS=%ld"
          " : InsertReadyList=%ld"
          " : Select=%ld : Issue=%ld"
          " : Complete=%ld : Commit=%ld"

          " : ROB=%ld : LQ=%ld : SQ=%ld"
          " : RS=%ld : FU=%ld"
          " : SRC=%ld,%ld,%ld : DST=%ld"
          " : SRCTYPE=%ld,%ld,%ld"
          "\n", 
          find_perfCnt("cf_" + std::to_string(i)),
          find_perfCnt("pc_" + std::to_string(i)),
          find_perfCnt("instr_" + std::to_string(i)),
          find_perfCnt("fuType_" + std::to_string(i)),
          find_perfCnt("fuOpType_" + std::to_string(i)),
          find_perfCnt("fpu_" + std::to_string(i)),
          find_perfCnt("FetchCacheLine_" + std::to_string(i)),
          find_perfCnt("ProcessCacheCompletion_" + std::to_string(i)),
          find_perfCnt("Fetch_" + std::to_string(i)),
          find_perfCnt("Decode_" + std::to_string(i)),
          find_perfCnt("Rename_" + std::to_string(i)),
          find_perfCnt("BlockFromROB_" + std::to_string(i)),
          find_perfCnt("BlockFromDPQ_" + std::to_string(i)),
          find_perfCnt("BlockFromSerial_" + std::to_string(i)),
          find_perfCnt("BlockFromRF_" + std::to_string(i)),
          find_perfCnt("BlockFromLQ_" + std::to_string(i)),
          find_perfCnt("BlockFromSQ_" + std::to_string(i)),
          find_perfCnt("EliminatedMove_" + std::to_string(i)),
          find_perfCnt("Dispatch_" + std::to_string(i)),
          find_perfCnt("EnqRS_" + std::to_string(i)),
          find_perfCnt("InsertReadyList_" + std::to_string(i)),
          find_perfCnt("Select_" + std::to_string(i)),
          find_perfCnt("Issue_" + std::to_string(i)),
          find_perfCnt("Complete_" + std::to_string(i)),
          find_perfCnt("Commit_" + std::to_string(i)),
          find_perfCnt("ROB_" + std::to_string(i)),
          find_perfCnt("LQ_" + std::to_string(i)),
          find_perfCnt("SQ_" + std::to_string(i)),
          find_perfCnt("RS_" + std::to_string(i)),
          find_perfCnt("FU_" + std::to_string(i)),
          find_perfCnt("SRC0_" + std::to_string(i)),
          find_perfCnt("SRC1_" + std::to_string(i)),
          find_perfCnt("SRC2_" + std::to_string(i)),
          find_perfCnt("DST_" + std::to_string(i)),
          find_perfCnt("SRCTYPE0_" + std::to_string(i)),
          find_perfCnt("SRCTYPE1_" + std::to_string(i)),
          find_perfCnt("SRCTYPE2_" + std::to_string(i))
        );
    }
  }
}

std::string exec(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;

  // 使用 popen 执行 shell 命令
  FILE* proc_pipe = popen(cmd, "r");
  if (!proc_pipe) {
      throw std::runtime_error("popen() failed!");
  }
  
  // 读取子进程输出
  while (fgets(buffer.data(), buffer.size(), proc_pipe) != nullptr) {
      result += buffer.data();
  }

  // 关闭进程
  pclose(proc_pipe);
  return result;
}

int Perfprocess::update_deg_v2() {
  int commit_count = 0;
  clear_traces();
  for (int i = 0; i < commit_width; i++) {
    // printf("[debug] iter=%d\n", i);
    auto do_update = find_perfCnt("isCommit_" + std::to_string(i));
    // printf("[debug] iter=%d do_update=%lu\n", i, (unsigned long)do_update);
    if (do_update != 0) {
      commit_count++;
      uint64_t cycle = find_perfCnt("cf_" + std::to_string(i));
      uint64_t pc = find_perfCnt("pc_" + std::to_string(i));
      uint64_t instr = find_perfCnt("instr_" + std::to_string(i));
      uint64_t fuType = find_perfCnt("fuType_" + std::to_string(i));
      uint64_t fuOpType = find_perfCnt("fuOpType_" + std::to_string(i));
      uint64_t fpu = find_perfCnt("fpu_" + std::to_string(i));
      uint64_t decode = find_perfCnt("Decode_" + std::to_string(i));
      uint64_t rename = find_perfCnt("Rename_" + std::to_string(i));
      uint64_t issue = find_perfCnt("Issue_" + std::to_string(i));
      uint64_t complete = find_perfCnt("Complete_" + std::to_string(i));
      uint64_t commit = find_perfCnt("Commit_" + std::to_string(i));
      uint64_t src_0 = find_perfCnt("SRC0_" + std::to_string(i));
      uint64_t src_1 = find_perfCnt("SRC1_" + std::to_string(i));
      uint64_t src_2 = find_perfCnt("SRC2_" + std::to_string(i));
      int src_valid_0 = find_perfCnt("SRCTYPE0_" + std::to_string(i));
      int src_valid_1 = find_perfCnt("SRCTYPE1_" + std::to_string(i));
      int src_valid_2 = find_perfCnt("SRCTYPE2_" + std::to_string(i));
      uint64_t dest = find_perfCnt("DST_" + std::to_string(i));

      // printf("[debug] iter=%d cycle=%lu pc=0x%lx instr=0x%lx\n",
      //              i, (unsigned long)cycle, (unsigned long)pc, (unsigned long)instr);

      // 调用 spike-dasm 解析指令
      std::stringstream command;
      command << "echo \"DASM(" << std::setw(8) << std::setfill('0') << std::hex << instr << ")\" | spike-dasm";

      // printf("[debug] iter=%d spike-dasm cmd=\"%s\"\n", i, command.str().c_str());
      std::string insn_decoded = exec(command.str().c_str());
      insn_decoded.erase(insn_decoded.find_last_not_of(" \n") + 1);
      // printf("[debug] iter=%d insn_decoded=\"%s\"\n", i, insn_decoded.c_str());
      
      // 解析指令类型
      std::string type_str = "unknown";
      if (fuType < 8) {
          type_str = (fuOpType == 4) ? "IntMult" : (fuOpType == 5) ? "IntDiv" : "IntAlu";
      } else if (fuType < 12) {
          type_str = "Fp";
      } else {
          type_str = (fuType == 12 || fuType == 15) ? "MemRead" : (fuType == 13) ? "MemWrite" : "unknown";
      }

      // 生成 SRC 字符串
      std::string src_str = "";
      if (src_valid_0 && src_0 != 0) src_str += std::to_string(src_0);
      if (src_valid_1 && src_1 != 0) {
        if (!src_str.empty()) src_str += ",";
        src_str += std::to_string(src_1);
      }
      if (src_valid_2 && src_2 != 0) {
        if (!src_str.empty()) src_str += ",";
        src_str += std::to_string(src_2);
      }

      // 生成 trace 记录
      std::ostringstream trace;
      trace << cycle * 1000 << " : system.cpu : T0 : "
            << std::hex << "0x" << pc << " : " << insn_decoded << " : " << std::dec << type_str
            << " : _ : FetchCacheLine=" << find_perfCnt("FetchCacheLine_" + std::to_string(i)) * 1000
            << " : ProcessCacheCompletion=" << find_perfCnt("ProcessCacheCompletion_" + std::to_string(i)) * 1000
            << " : Fetch=" << find_perfCnt("Fetch_" + std::to_string(i)) * 1000
            << " : DecodeSortInsts=" << decode * 1000
            << " : Decode=" << decode * 1000
            << " : RenameSortInsts=" << rename * 1000
            << " : BlockFromROB=" << find_perfCnt("BlockFromROB_" + std::to_string(i))
            << " : BlockFromRF=" << find_perfCnt("BlockFromRF_" + std::to_string(i))
            << " : BlockFromIQ=" << (find_perfCnt("BlockFromDPQ_" + std::to_string(i)) | find_perfCnt("BlockFromSerial_" + std::to_string(i)))
            << " : BlockFromLQ=" << find_perfCnt("BlockFromLQ_" + std::to_string(i))
            << " : BlockFromSQ=" << find_perfCnt("BlockFromSQ_" + std::to_string(i))
            << " : Rename=" << rename * 1000
            << " : Dispatch=" << find_perfCnt("Dispatch_" + std::to_string(i)) * 1000
            << " : InsertReadyList=" << find_perfCnt("InsertReadyList_" + std::to_string(i)) * 1000
            << " : Issue=" << find_perfCnt("Issue_" + std::to_string(i)) * 1000
            << " : Memory=" << (issue == 0 ? 0 : (issue + 1) * 1000)
            << " : Complete=" << complete * 1000
            << " : CompleteMemory=" << complete * 1000
            << " : CommitHead=" << (commit - 1) * 1000
            << " : Commit=" << commit * 1000
            << " : ROB=" << find_perfCnt("ROB_" + std::to_string(i))
            << " : LQ=" << find_perfCnt("LQ_" + std::to_string(i))
            << " : SQ=" << find_perfCnt("SQ_" + std::to_string(i))
            << " : IQ=" << find_perfCnt("RS_" + std::to_string(i))
            << " : FU=" << find_perfCnt("FU_" + std::to_string(i))
            << " : SRC=" << src_str
            << " : DST=" << (dest == 0 ? "" : std::to_string(dest))
            << " : BlockFromDPQ=" << find_perfCnt("BlockFromDPQ_" + std::to_string(i))
            << " : BlockFromSerial=" << find_perfCnt("BlockFromSerial_" + std::to_string(i));
        
        traces.push_back(trace.str());
        // if (true) {
        //     std::cerr << trace.str() << std::endl;
        // }
    }
  }
  return commit_count;
}

bool Perfprocess::get_simulation_stats(long int dse_epoch) {
  // get perf counters
  uint64_t BPWrong = find_perfCnt("BPWrong");
  uint64_t commitUop = find_perfCnt("commitUop");
  uint64_t rob_reads = find_perfCnt("rob_reads");
  uint64_t rob_writes = find_perfCnt("rob_writes");
  uint64_t rename_reads = find_perfCnt("rename_reads");
  uint64_t rename_writes = find_perfCnt("rename_writes");
  uint64_t fp_rename_reads = find_perfCnt("fp_rename_reads");
  uint64_t fp_rename_writes = find_perfCnt("fp_rename_writes");
  uint64_t intdqreads = find_perfCnt("intdqreads");
  uint64_t intdqwrites = find_perfCnt("intdqwrites");
  uint64_t issue_num = find_perfCnt("issue_num");
  uint64_t clock_cycle = find_perfCnt("clock_cycle");
  uint64_t commitInstr = find_perfCnt("commitInstr");
  uint64_t commitInstrBranch = find_perfCnt("commitInstrBranch");
  uint64_t jmp_instr_cnt = find_perfCnt("jmp_instr_cnt");
  uint64_t div_instr_cnt = find_perfCnt("div_instr_cnt");
  uint64_t fdiv_fsqrt_instr_cnt = find_perfCnt("fdiv/fsqrt_instr_cnt");
  uint64_t int_to_float_instr_cnt = find_perfCnt("int_to_float_instr_cnt");
  uint64_t alu_instr_cnt = find_perfCnt("alu_instr_cnt");
  uint64_t store_instr_cnt = find_perfCnt("store_instr_cnt");
  uint64_t csr_instr_cnt = find_perfCnt("csr_instr_cnt");
  uint64_t load_instr_cnt = find_perfCnt("load_instr_cnt");
  uint64_t bku_instr_cnt = find_perfCnt("bku_instr_cnt");
  uint64_t fence_instr_cnt = find_perfCnt("fence_instr_cnt");
  uint64_t fmisc_instr_cnt = find_perfCnt("fmisc_instr_cnt");
  uint64_t fmac_instr_cnt = find_perfCnt("fmac_instr_cnt");
  uint64_t fmac_instr_cnt_fma = find_perfCnt("fmac_instr_cnt_fma");
  uint64_t mul_instr_cnt = find_perfCnt("mul_instr_cnt");
  uint64_t mou_instr_cnt = find_perfCnt("mou_instr_cnt");
  uint64_t intRegfileReads = find_perfCnt("intRegfileReads");
  uint64_t intRegfileWrites = find_perfCnt("intRegfileWrites");
  uint64_t fpRegfileReads = find_perfCnt("fpRegfileReads");
  uint64_t fpRegfileWrites = find_perfCnt("fpRegfileWrites");
  uint64_t intAluAccess = find_perfCnt("intAluAccess");
  uint64_t fpuAccess = find_perfCnt("fpuAccess");
  uint64_t mulAccess = find_perfCnt("mulAccess");
  uint64_t access_itlb = find_perfCnt("access_itlb");
  uint64_t miss_itlb = find_perfCnt("miss_itlb");
  uint64_t access_ldtlb = find_perfCnt("access_ldtlb");
  uint64_t miss_ldtlb = find_perfCnt("miss_ldtlb");
  uint64_t access_sttlb = find_perfCnt("access_sttlb");
  uint64_t miss_sttlb = find_perfCnt("miss_sttlb");
  uint64_t replace_itlb = find_perfCnt("replace_itlb");
  uint64_t replace_ldtlb = find_perfCnt("replace_ldtlb");
  uint64_t replace_sttlb = find_perfCnt("replace_sttlb");
  uint64_t icache_read_access = find_perfCnt("ICache_read_access");
  uint64_t icache_read_miss = find_perfCnt("ICache_read_miss");
  uint64_t icache_conflit = find_perfCnt("ICache_conflit");
  uint64_t dcache_read_access = find_perfCnt("dcache_read_access");
  uint64_t dcache_read_miss = find_perfCnt("dcache_read_miss");
  uint64_t dcache_write_access = find_perfCnt("dcache_write_access");
  uint64_t dcache_write_miss = find_perfCnt("dcache_write_miss");
  uint64_t dcache_conflit = find_perfCnt("dcache_conflit");
  uint64_t btb_read_access = find_perfCnt("btb_read_access");
  uint64_t btb_write_access = find_perfCnt("btb_write_access");
  uint64_t l2_read_access = find_perfCnt("l2_read_access");
  uint64_t l2_read_miss = find_perfCnt("l2_read_miss");
  uint64_t l2_write_access = find_perfCnt("l2_write_access");
  uint64_t l2_write_miss = find_perfCnt("l2_write_miss");
  uint64_t l2_conflit = find_perfCnt("l2_conflit");
  // uint64_t memory_access = find_perfCnt("memory_access");
  // uint64_t memory_write = find_perfCnt("memory_write");
  
  // do some calculations
  uint64_t int_instructions = alu_instr_cnt + div_instr_cnt + mul_instr_cnt + jmp_instr_cnt;
  uint64_t fp_instructions = fdiv_fsqrt_instr_cnt + fmisc_instr_cnt + fmac_instr_cnt + fmac_instr_cnt_fma;
  uint64_t access_dtlb = access_ldtlb + access_sttlb;
  uint64_t miss_dtlb = miss_ldtlb + miss_sttlb;
  uint64_t replace_dtlb = replace_ldtlb + replace_sttlb;

  // output
  std::ostringstream stats;
  stats << "dse_epoch: " << dse_epoch << std::endl
        << "total_cycles: " << clock_cycle << std::endl
        << "total_instructions: " << issue_num << std::endl
        << "int_instructions: " << int_instructions << std::endl
        << "fp_instructions: " << fp_instructions << std::endl
        << "branch_instructions: " << commitInstrBranch << std::endl
        << "load_instructions: " << load_instr_cnt << std::endl
        << "store_instructions: " << store_instr_cnt << std::endl
        << "committed_instructions: " << commitUop << std::endl
        << "committed_int_instructions: " << int_instructions << std::endl
        << "committed_fp_instructions: " << fp_instructions << std::endl
        << "ROB_reads: " << rob_reads << std::endl
        << "ROB_writes: " << rob_writes << std::endl
        << "rename_reads: " << rename_reads << std::endl
        << "rename_writes: " << rename_writes << std::endl
        << "fp_rename_reads: " << fp_rename_reads << std::endl
        << "fp_rename_writes: " << fp_rename_writes << std::endl
        << "intRegfileReads: " << intRegfileReads << std::endl
        << "intRegfileWrites: " << intRegfileWrites << std::endl
        << "fpRegfileReads: " << fpRegfileReads << std::endl
        << "fpRegfileWrites: " << fpRegfileWrites << std::endl
        << "intAluAccess: " << intAluAccess << std::endl
        << "fpuAccess: " << fpuAccess << std::endl
        << "mulAccess: " << mulAccess << std::endl
        << "access_itlb: " << access_itlb << std::endl
        << "miss_itlb: " << miss_itlb << std::endl
        << "access_dtlb: " << access_dtlb << std::endl
        << "miss_dtlb: " << miss_dtlb << std::endl
        << "replace_itlb: " << replace_itlb << std::endl
        << "replace_dtlb: " << replace_dtlb << std::endl
        << "icache_read_access: " << icache_read_access << std::endl
        << "icache_read_miss: " << icache_read_miss << std::endl
        << "icache_conflit: " << icache_conflit << std::endl
        << "dcache_read_access: " << dcache_read_access << std::endl
        << "dcache_read_miss: " << dcache_read_miss << std::endl
        << "dcache_write_access: " << dcache_write_access << std::endl
        << "dcache_write_miss: " << dcache_write_miss << std::endl
        << "dcache_conflit: " << dcache_conflit << std::endl
        << "btb_read_access: " << btb_read_access << std::endl
        << "btb_write_access: " << btb_write_access << std::endl
        << "l2_read_access: " << l2_read_access << std::endl
        << "l2_read_misses: " << l2_read_miss << std::endl
        << "l2_write_access: " << l2_write_access << std::endl
        << "l2_write_misses: " << l2_write_miss << std::endl
        << "l2_conflit: " << l2_conflit << std::endl
        << "branch_mispredictions: " << BPWrong << std::endl
        << "intdqreads: " << intdqreads << std::endl
        << "intdqwrites: " << intdqwrites << std::endl
        << "memory_access: " << 0 << std::endl
        << "memory_write: " << 0 << std::endl;

  std::ofstream stats_file("stats.txt", std::ios::trunc);
  if (stats_file.is_open()) {
    stats_file << stats.str();
    stats_file.close();
    return true;
  } else {
    std::cerr << "Failed to open stats.txt for writing." << std::endl;
    return false;
  }
}