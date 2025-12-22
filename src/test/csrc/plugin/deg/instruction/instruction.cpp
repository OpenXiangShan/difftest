/* Author: baichen.bai@alibaba-inc.com */


#include "deg/instruction.h"


InstructionStream::InstructionStream(const std::string& file_name):
    name(file_name) {
    if (file_name != "") {
        trace.open(file_name);
        if (!trace.is_open())
            ERROR("cannot open trace: %s\n", file_name);
    }
}