/* Author: baichen.bai@alibaba-inc.com */


#include "deg/riscv_instruction.h"


RiscvInstructionStream::RiscvInstructionStream(const std::string& file_name):
    InstructionStream(file_name) {

}


RiscvInstruction* RiscvInstructionStream::next() {
    std::string line;

    if (!getline(trace, line)) {
        return nullptr;
    }
    inst_seq++;

    return new RiscvInstruction(line, inst_seq);
}

RiscvInstruction* RiscvInstructionStream::next_v2(std::string line) {
    if (line == "") {
        return nullptr;
    }
    inst_seq++;

    return new RiscvInstruction(line, inst_seq);
}
