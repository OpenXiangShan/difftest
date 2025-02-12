/* Author: baichen.bai@alibaba-inc.com */


#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__


#include <fstream>
#include "utils.h"


struct Instruction {
    const std::string line;

    Instruction() = default;
    ~Instruction() = default;
    explicit Instruction(const std::string& line): line(line) {}
};


class InstructionStream {
public:
    InstructionStream() = delete;
    InstructionStream(const std::string&);
    ~InstructionStream() = default;

public:
    virtual Instruction* next() = 0;
    std::string get_name() const noexcept {
        return name;
    }

protected:
    std::string name;
    std::ifstream trace;
    Instruction* inst;
};


#endif