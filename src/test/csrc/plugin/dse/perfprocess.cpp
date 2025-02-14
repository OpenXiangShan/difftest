#include "perfprocess.h"

Perfprocess::Perfprocess(VSimTop *dut_ptr, int commit_width) {
  this->dut_ptr = dut_ptr;
  this->commit_width = commit_width;
  this->perfNames = getIOPerfNames();
  auto args = deg_parse_args(0, nullptr);
  args["output"] = "output";
  args["view"] = "1";
  this->o3graph = new O3Graph(
    args, new RiscvInstructionStream("")
  );
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
    auto do_update = find_perfCnt("isCommit_" + std::to_string(i));
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

      // 调用 spike-dasm 解析指令
      std::stringstream command;
      command << "echo \"DASM(" << std::setw(8) << std::setfill('0') << std::hex << instr << ")\" | spike-dasm";
      std::string insn_decoded = exec(command.str().c_str());
      insn_decoded.erase(insn_decoded.find_last_not_of(" \n") + 1);
      
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
            << " : DST=" << (dest == 0 ? "" : std::to_string(dest));
        
        traces.push_back(trace.str());
        if (true) {
            std::cerr << trace.str() << std::endl;
        }
        this->o3graph->step(trace.str());
    }
  }
  return commit_count;
}