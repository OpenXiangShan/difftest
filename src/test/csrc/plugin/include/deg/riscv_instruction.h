/* Author: baichen.bai@alibaba-inc.com */


#ifndef __RISCV_INSTRUCTION_H__
#define __RISCV_INSTRUCTION_H__


#include "instruction.h"


enum InstType {
    MemRead,        // 0
    FloatMemRead,   // 1
    MemWrite,       // 2
    FloatMemWrite,  // 3
    IntAlu,         // 4
    IntMult,        // 5
    IntDiv,         // 6
    FloatAdd,       // 7
    FloatCmp,       // 8
    FloatCvt,       // 9
    FloatMult,      // 10
    FloatMultAcc,   // 11
    FloatMisc,      // 12
    FloatDiv,       // 13
    FloatSqrt,      // 14
    NoOpClass,      // 15
    NumOfInstType   // 16
};


static std::unordered_map<InstType, std::string> inst_type = {
    {InstType::MemRead, "MemRead"},
    {InstType::FloatMemRead, "FloatMemRead"},
    {InstType::MemWrite, "MemWrite"},
    {InstType::FloatMemWrite, "FloatMemWrite"},
    {InstType::IntAlu, "IntAlu"},
    {InstType::IntMult, "IntMult"},
    {InstType::IntDiv, "IntDiv"},
    {InstType::FloatAdd, "FloatAdd"},
    {InstType::FloatCmp, "FloatCmp"},
    {InstType::FloatCvt, "FloatCvt"},
    {InstType::FloatMult, "FloatMult"},
    {InstType::FloatMultAcc, "FloatMultAcc"},
    {InstType::FloatMisc, "FloatMisc"},
    {InstType::FloatDiv, "FloatDiv"},
    {InstType::FloatSqrt, "FloatSqrt"},
    {InstType::NoOpClass, "NoOpClass"}
};


struct Tick {
    tick fetch_cache_line;
    tick process_cache_completion;
    tick fetch;
    tick decode_sort_insts;
    tick decode;
    tick rename_sort_insts;
    tick block_from_rob;
    tick block_from_rf;
    tick block_from_iq;
    tick block_from_lq;
    tick block_from_sq;
    tick rename;
    tick dispatch;
    tick insert_ready_list;
    tick issue;
    tick memory;
    tick complete;
    tick complete_memory;
    tick commit_head;
    tick commit;
    tick block_from_serial;

    Tick() = default;
    ~Tick() = default;
    Tick(
        tick fetch_cache_line,
        tick process_cache_completion,
        tick fetch,
        tick decode_sort_insts,
        tick decode,
        tick rename_sort_insts,
        tick block_from_rob,
        tick block_from_rf,
        tick block_from_iq,
        tick block_from_lq,
        tick block_from_sq,
        tick rename,
        tick dispatch,
        tick insert_ready_list,
        tick issue,
        tick memory,
        tick complete,
        tick complete_memory,
        tick commit_head,
        tick commit,
        tick block_from_serial
    ):
        fetch_cache_line(fetch_cache_line),
        process_cache_completion(process_cache_completion),
        fetch(fetch),
        decode_sort_insts(decode_sort_insts),
        decode(decode),
        rename_sort_insts(rename_sort_insts),
        block_from_rob(block_from_rob),
        block_from_rf(block_from_rf),
        block_from_iq(block_from_iq),
        block_from_lq(block_from_lq),
        block_from_sq(block_from_sq),
        rename(rename),
        dispatch(dispatch),
        insert_ready_list(insert_ready_list),
        issue(issue),
        memory(memory),
        complete(complete),
        complete_memory(complete_memory),
        commit_head(commit_head),
        commit(commit),
        block_from_serial(block_from_serial)
     {}
     Tick(const Tick& tick):
        fetch_cache_line(tick.fetch_cache_line),
        process_cache_completion(tick.process_cache_completion),
        fetch(tick.fetch),
        decode_sort_insts(tick.decode_sort_insts),
        decode(tick.decode),
        rename_sort_insts(tick.rename_sort_insts),
        block_from_rob(tick.block_from_rob),
        block_from_rf(tick.block_from_rf),
        block_from_iq(tick.block_from_iq),
        block_from_lq(tick.block_from_lq),
        block_from_sq(tick.block_from_sq),
        rename(tick.rename),
        dispatch(tick.dispatch),
        insert_ready_list(tick.insert_ready_list),
        issue(tick.issue),
        memory(tick.memory),
        complete(tick.complete),
        complete_memory(tick.complete_memory),
        commit_head(tick.commit_head),
        commit(tick.commit),
        block_from_serial(tick.block_from_serial)
    {}

public:
    bool operator==(const Tick& tick) const noexcept {
        if (this->fetch_cache_line == tick.fetch_cache_line && \
            this->process_cache_completion == tick.process_cache_completion && \
            this->fetch == tick.fetch && \
            this->decode_sort_insts == tick.decode_sort_insts && \
            this->decode == tick.decode && \
            this->rename_sort_insts == tick.rename_sort_insts && \
            this->block_from_rob == tick.block_from_rob && \
            this->block_from_rf == tick.block_from_rf && \
            this->block_from_iq == tick.block_from_iq && \
            this->block_from_lq == tick.block_from_lq && \
            this->block_from_sq == tick.block_from_sq && \
            this->rename == tick.rename && \
            this->dispatch == tick.dispatch && \
            this->insert_ready_list == tick.insert_ready_list && \
            this->issue == tick.issue && \
            this->memory == tick.memory && \
            this->complete == tick.complete && \
            this->complete_memory == tick.complete_memory && \
            this->commit_head == tick.commit_head && \
            this->commit == tick.commit && \
            this->block_from_serial == tick.block_from_serial
        ) {
            return true;
        }
        return false;
    }
};


struct RiscvInstruction : public Instruction {
    std::string inst;
    InstType inst_type;
    addr pc;
    Tick* event_tick;
    count_t inst_seq;
    std::vector<int> src;
    int dst;
    int rob;
    int lq;
    int sq;
    int iq;
    int fu;

    RiscvInstruction() {
        event_tick = new Tick;
    };
    ~RiscvInstruction() {
        delete event_tick;
    };
    RiscvInstruction(const std::string& line, count_t inst_seq):
        Instruction(line),
        inst_seq(inst_seq) {
        std::vector<std::string> segments;
        split(segments, line, ":");
        set_inst(segments[4]);
        set_inst_type(segments[5]);
        set_pc(segments[3]);
        set_event_tick(segments);
        set_rob(segments[27]);
        set_lq(segments[28]);
        set_sq(segments[29]);
        set_iq(segments[30]);
        set_fu(segments[31]);
        set_src_reg(segments[32]);
        set_dst_reg(segments[33]);
    }
    RiscvInstruction(const RiscvInstruction& inst):
        inst(inst.inst),
        inst_type(inst.inst_type),
        pc(inst.pc),
        inst_seq(inst.inst_seq),
        src(inst.src),
        dst(inst.dst),
        rob(inst.rob),
        lq(inst.lq),
        sq(inst.sq),
        iq(inst.iq),
        fu(inst.fu) {
        event_tick = new Tick(*(inst.event_tick));
    }

public:
    void set_inst(std::string& line) {
        inst = strip(line);
    }

    void set_pc(const std::string& line) {
        std::stringstream ss;
        ss << std::hex << line.substr(0, line.find("@") - 1);
        ss >> pc;
    }

    void set_inst_type(std::string& line) {
        auto type = strip(line);
        if (type == "MemRead")
            inst_type = InstType::MemRead;
        else if (type == "FloatMemRead")
            inst_type = InstType::FloatMemRead;
        else if (type == "MemWrite")
            inst_type = InstType::MemWrite;
        else if (type == "FloatMemWrite")
            inst_type = InstType::FloatMemWrite;
        else if (type == "IntAlu")
            inst_type = InstType::IntAlu;
        else if (type == "IntMult")
            inst_type = InstType::IntMult;
        else if (type == "IntDiv")
            inst_type = InstType::IntDiv;
        else if (type == "FloatAdd")
            inst_type = InstType::FloatAdd;
        else if (type == "FloatCmp")
            inst_type = InstType::FloatCmp;
        else if (type == "FloatCvt")
            inst_type = InstType::FloatCvt;
        else if (type == "FloatMult")
            inst_type = InstType::FloatMult;
        else if (type == "FloatMultAcc")
            inst_type = InstType::FloatMultAcc;
        else if (type == "FloatMisc")
            inst_type = InstType::FloatMisc;
        else if (type == "FloatDiv")
            inst_type = InstType::FloatDiv;
        else if (type == "FloatSqrt")
            inst_type = InstType::FloatSqrt;
        else {
            PPK_ASSERT(
                type == "No_OpClass",
                "inst: %s, FU type: %s", this->inst.c_str(), type.c_str()
            );
            inst_type = InstType::NoOpClass;
        }
    }

    void set_event_tick(const std::vector<std::string>& segments) {
        auto f = [&segments] (int idx) -> tick {
            return std::stoull(
                segments[idx].substr(segments[idx].find('=') + 1)
            ) / CYCLE_PER_TICK;
        };

        auto f_raw = [&segments] (int idx) -> tick {
            return std::stoull(
                segments[idx].substr(segments[idx].find('=') + 1)
            );
        };

        /*
        #7 tick fetch_cache_line,
        #8 tick process_cache_completion,
        #9 tick fetch,
        #10 tick decode_sort_insts,
        #11 tick decode,
        #12 tick rename_sort_insts,
        #13 tick block_from_rob,
        #14 tick block_from_rf,
        #15 tick block_from_iq,
        #16 tick block_from_lq,
        #17 tick block_from_sq,
        #18 tick rename,
        #19 tick dispatch,
        #20 tick insert_ready_list,
        #21 tick issue,
        #22 tick memory,
        #23 tick complete,
        #24 tick complete_memory,
        #25 tick commit_head,
        #26 tick commit
        #35 tick block_from_serial
        */
       if (f(19) != 0 && f(20) == 0 && f(26) != 0) {  // move elimination
        event_tick = new Tick(
            f(7), f(8), f(9), f(10),
            f(11), f(12), f_raw(13), f_raw(14),
            f_raw(15), f_raw(16), f_raw(17), f(18),
            f(19), f(19), f(19), f(19),
            f(19), f(19), f(25), f(26),
            f_raw(35)
        );
        } else {
            event_tick = new Tick(
                f(7), f(8), f(9), f(10),
                f(11), f(12), f_raw(13), f_raw(14),
                f_raw(15), f_raw(16), f_raw(17), f(18),
                f(19), f(20), f(21), f(22),
                f(23), f(24), f(25), f(26),
                f_raw(35)
            );
        }
    }

    void set_rob(const std::string& line) {
        rob = std::stoi(line.substr(line.find('=') + 1));
    }

    void set_lq(const std::string& line) {
        lq = std::stoi(line.substr(line.find('=') + 1));
    }

    void set_sq(const std::string& line) {
        sq = std::stoi(line.substr(line.find('=') + 1));
    }

    void set_iq(const std::string& line) {
        iq = std::stoi(line.substr(line.find('=') + 1));
    }

    void set_fu(const std::string& line) {
        fu = std::stoi(line.substr(line.find('=') + 1));
    }

    void set_src_reg(const std::string& line) {
        auto src_line = line.substr(line.find('=') + 1);
        std::vector<std::string> _line;
        if (src_line.find(',') != std::string::npos) {
            split(_line, strip(src_line), ",");
            for (auto s : _line) {
                if (s.length() > 0)
                    src.emplace_back(std::stoi(s));
            }
        } else {
            src_line = strip(src_line);
            if (src_line.length() > 0) {
                src.emplace_back(std::stoi(src_line));
            }
        }
    }

    void set_dst_reg(const std::string& line) {
        auto pos = line.find('=') + 1;
        if (pos == line.length() || line.substr(pos) == " ") {
            dst = -1;
        } else {
            dst = std::stoi(line.substr(pos));
        }
    }

public:
    tick fetch_cache_line() const {
        return this->event_tick->fetch_cache_line;
    }

    tick process_cache_completion() const {
        return this->event_tick->process_cache_completion;
    }

    tick fetch() const {
        return this->event_tick->fetch;
    }

    tick decode_sort_insts() const {
        return this->event_tick->decode_sort_insts;
    }

    tick decode() const {
        return this->event_tick->decode;
    }

    tick rename_sort_insts() const {
        return this->event_tick->rename_sort_insts;
    }

    tick block_from_rob() const {
        return this->event_tick->block_from_rob;
    }

    tick block_from_rf() const {
        return this->event_tick->block_from_rf;
    }

    tick block_from_iq() const {
        return this->event_tick->block_from_iq;
    }

    tick block_from_lq() const {
        return this->event_tick->block_from_lq;
    }

    tick block_from_sq() const {
        return this->event_tick->block_from_sq;
    }

    tick rename() const {
        return this->event_tick->rename;
    }

    tick dispatch() const {
        return this->event_tick->dispatch;
    }

    tick insert_ready_list() const {
        return this->event_tick->insert_ready_list;
    }

    tick issue() const {
        return this->event_tick->issue;
    }

    tick memory() const {
        return this->event_tick->memory;
    }

    tick complete() const {
        return this->event_tick->complete;
    }

    tick complete_memory() const {
        return this->event_tick->complete_memory;
    }

    tick commit_head() const {
        return this->event_tick->commit_head;
    }

    tick commit() const {
        return this->event_tick->commit;
    }

    tick block_from_serial() const {
        return this->event_tick->block_from_serial;
    }

public:
    std::string get_rs1_from_jalr() const {
        std::vector<std::string> segments;
        split(segments, this->inst, " ");
        return segments[1].substr(0, segments[1].size() - 1);
    }

    std::string get_rd_from_jalr() const {
        std::vector<std::string> v1, v2;
        split(v1, this->inst, "(");
        split(v2, v1[1], ")");
        return v2[0];
    }

    bool use_bpu() const {
        return starts_with(this->inst, "beq") || starts_with(this->inst, "bne") || \
            starts_with(this->inst, "bltu") || starts_with(this->inst, "blt") || \
            starts_with(this->inst, "bgeu") || starts_with(this->inst, "bge");
    }

    bool use_btb() const {
        if(use_bpu() || \
            (starts_with(this->inst, "jal") && !starts_with(this->inst, "jalr"))
        )
            return true;
        return false;
    }

    bool use_ras() const {
        if (starts_with(this->inst, "jalr")) {
            auto rs1 = this->get_rs1_from_jalr(), rd = this->get_rd_from_jalr();
            if (rs1 != "ra" && rs1 != "t0" && rd != "ra" && rd != "t0")
                return false;
            return true;
        }
        if (starts_with(this->inst, "jal ra") || starts_with(this->inst, "jal t0"))
            return true;
        return false;
    }

    bool use_ipred() const {
        if (starts_with(this->inst, "jalr")) {
            auto rs1 = this->get_rs1_from_jalr(), rd = this->get_rd_from_jalr();
            if (rs1 != "ra" && rs1 != "t0")
                return true;
            if ((rs1 == "ra" || rs1 == "t0") && (rd == "ra" || rd == "t0") && (rs1 == rd))
                return true;
        }
        return false;
    }

    bool is_mem() const {
        if (inst_type == InstType::MemRead || \
            inst_type == InstType::FloatMemRead || \
            inst_type == InstType::MemWrite || \
            inst_type == InstType::FloatMemWrite
        ) {
            return true;
        }
        return false;
    }

    bool is_load() const {
        if (inst_type == InstType::MemRead || \
            inst_type == InstType::FloatMemRead
        ) {
            return true;
        }
        return false;
    }

    bool is_store() const {
        if (inst_type == InstType::MemWrite || \
            inst_type == InstType::FloatMemWrite
        ) {
            return true;
        }
        return false;
    }

    bool is_control() const {
        if (
            starts_with(this->inst, "beq") || starts_with(this->inst, "bne") || \
            starts_with(this->inst, "bltu") || starts_with(this->inst, "blt") || \
            starts_with(this->inst, "bgeu") || starts_with(this->inst, "bge") || \
            starts_with(this->inst, "jalr") || starts_with(this->inst, "jal") || \
            starts_with(this->inst, "c_beqz")
        ) {
            return true;
        }
        return false;
    }

    bool use_int_alu() const {
        return inst_type == InstType::IntAlu;
    }

    bool use_int_mult_div() const {
        if (inst_type == InstType::IntMult || \
            inst_type == InstType::IntDiv
        ) {
            return true;
        }
        return false;
    }

    bool use_fp_alu() const {
        if (inst_type == InstType::FloatAdd || \
            inst_type == InstType::FloatCmp || \
            inst_type == InstType::FloatCvt
        ) {
            return true;
        }
        return false;
    }

    bool use_fp_mult_div() const {
        if (inst_type == InstType::FloatMult || \
            inst_type == InstType::FloatMultAcc || \
            inst_type == InstType::FloatMisc || \
            inst_type == InstType::FloatDiv || \
            inst_type == InstType::FloatSqrt
        ) {
            return true;
        }
        return false;
    }

    bool use_rd_wr_port() const {
        if (inst_type == InstType::MemRead || \
            inst_type == InstType::MemWrite || \
            inst_type == InstType::FloatMemRead || \
            inst_type == InstType::FloatMemWrite
        ) {
            return true;
        }
        return false;
    }

    bool use_int_rf() const {
        if (inst_type == InstType::IntAlu || \
            inst_type == InstType::IntMult || \
            inst_type == InstType::IntDiv || \
            inst_type == InstType::MemRead || \
            inst_type == InstType::MemWrite || \
            inst_type == InstType::FloatMemRead || \
            inst_type == InstType::FloatMemWrite
        ) {
            return true;
        }
        return false;
    }

    bool use_fp_rf() const {
        if (inst_type == InstType::FloatAdd || \
            inst_type == InstType::FloatCmp || \
            inst_type == InstType::FloatCvt
        ) {
            return true;
        }
        return false;
    }

public:
    bool operator==(const RiscvInstruction& inst) const noexcept {
        if (this->inst == inst.inst && \
            this->inst_type == inst.inst_type && \
            this->pc == inst.pc && \
            *(this->event_tick) == *(inst.event_tick) && \
            this->inst_seq == inst.inst_seq && \
            this->src == inst.src && \
            this->dst == inst.dst && \
            this->rob == inst.rob && \
            this->lq == inst.lq && \
            this->sq == inst.sq && \
            this->iq == inst.iq && \
            this->fu == inst.fu
        ) {
            return true;
        }
        return false;
    }
};


class RiscvInstructionStream : public InstructionStream {
public:
    RiscvInstructionStream() = delete;
    RiscvInstructionStream(const std::string&);
    ~RiscvInstructionStream() = default;

public:
    RiscvInstruction* next() override;
    RiscvInstruction* next_v2(std::string line);
    count_t get_inst_seq() const noexcept {
        return inst_seq;
    }

private:
    count_t inst_seq = 0;
};


#endif
