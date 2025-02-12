/* Author: baichen.bai@alibaba-inc.com */


#ifndef __GRAPH__UTILS_H__
#define __GRAPH__UTILS_H__


#include "utils.h"
#include "bottleneck.h"
#include "riscv_instruction.h"


enum VertexType {
    // request to fetch cache line
    F1,             // 0
    // receive from cache
    F2,             // 1
    // fetch
    F,              // 2
    // merge F2 & F
    F2F,            // 3
    // decode
    D,              // 4
    // rename
    R,              // 5
    // dispatch
    DS,             // 6
    // issue
    I,              // 7
    // merge dispatch & issue
    DI,             // 8
    // memory
    M,              // 9
    // complete
    P,              // 10
    // merge memory & complete
    MP,             // 11
    // commit
    C,              // 12
    NumOfVertexType // 13
};


const std::unordered_map<int, std::string> name_of_vertex_type = {
    std::pair<int, std::string>(int(VertexType::F1), "F1"),
    std::pair<int, std::string>(int(VertexType::F2), "F2"),
    std::pair<int, std::string>(int(VertexType::F), "F"),
    std::pair<int, std::string>(int(VertexType::F2F), "F2F"),
    std::pair<int, std::string>(int(VertexType::D), "D"),
    std::pair<int, std::string>(int(VertexType::R), "R"),
    std::pair<int, std::string>(int(VertexType::DS), "DS"),
    std::pair<int, std::string>(int(VertexType::I), "I"),
    std::pair<int, std::string>(int(VertexType::DI), "DI"),
    std::pair<int, std::string>(int(VertexType::M), "M"),
    std::pair<int, std::string>(int(VertexType::P), "P"),
    std::pair<int, std::string>(int(VertexType::MP), "MP"),
    std::pair<int, std::string>(int(VertexType::C), "C")
};


typedef struct VERTEX {
    VertexType type;
    count_t inst_seq;
    tick timing;
    RiscvInstruction* inst;

    VERTEX() = default;
    ~VERTEX() = default;
    VERTEX(VertexType type, count_t& inst_seq, RiscvInstruction* inst):
        type(type), inst_seq(inst_seq), inst(inst) {
            set_timing(type, inst);
        }
    VERTEX(VertexType type, count_t&& inst_seq, RiscvInstruction* inst):
        type(type), inst_seq(inst_seq), inst(inst) {
            set_timing(type, inst);
        }
    VERTEX(const VERTEX& vertex):
        type(vertex.type),
        inst_seq(vertex.inst_seq),
        timing(vertex.timing) {
            inst = new RiscvInstruction(*(vertex.inst));
        }
    VERTEX& operator=(const VERTEX& vertex) {
        this->type = vertex.type;
        this->inst_seq = vertex.inst_seq;
        this->inst = vertex.inst;
        set_timing(this->type, this->inst);
        return *this;
    }

private:
    void set_timing(VertexType type, RiscvInstruction* inst) {
        if (type == VertexType::F1) {
            timing = inst->fetch_cache_line();
        } else if (type == VertexType::F2) {
            timing = inst->process_cache_completion();
        } else if (type == VertexType::F) {
            timing = inst->fetch();
        } else if (type == VertexType::F2F) {
            PPK_ASSERT(
                inst->process_cache_completion() == inst->fetch(),
                "inst seq: %lu, inst: %s, " \
                "F2F timing is not correct: F2 = %lu, F = %lu\n",
                inst->inst_seq, inst->inst.c_str(),
                inst->process_cache_completion(),
                inst->fetch()
            );
            timing = inst->fetch();
        } else if (type == VertexType::D) {
            timing = inst->decode();
        } else if (type == VertexType::R) {
            timing = inst->rename();
        } else if (type == VertexType::DS) {
            timing = inst->dispatch();
        } else if (type == VertexType::I) {
            timing = inst->issue();
        } else if (type == VertexType::DI) {
            PPK_ASSERT(
                inst->dispatch() == inst->issue(),
                "inst seq: %lu, inst: %s, " \
                "DI timing is not correct: D = %lu, I = %lu\n",
                inst->inst_seq, inst->inst.c_str(),
                inst->dispatch(),
                inst->issue()
            );
            timing = inst->issue();
        } else if (type == VertexType::M) {
            timing = inst->memory();
        } else if (type == VertexType::P) {
            if (inst->is_load()) {
                /*
                 * `complete_memory` is valid for load instructions.
                 */
                timing = inst->complete_memory();
            } else {
                timing = inst->complete();
            }
        } else if (type == VertexType::MP) {
            /*
             * if `inst` is a load, we need to compare
             * `memory` and `complete_memory`. If `inst`
             * is a store, we do not compare.
             */
            if (inst->is_load()) {
                PPK_ASSERT(
                    inst->memory() == inst->complete_memory(),
                    "inst seq: %lu, inst: %s, " \
                    "MP timing is not correct: M = %lu, P = %lu\n",
                    inst->inst_seq, inst->inst.c_str(),
                    inst->memory(),
                    inst->complete_memory()
                );
            }
            timing = inst->memory();
        } else if (type == VertexType::C) {
            timing = inst->commit();
        }
    }

public:
    bool operator==(const VERTEX& vertex) const noexcept {
        if (this->type == vertex.type && \
            this->inst_seq == vertex.inst_seq && \
            this->timing == vertex.timing && \
            *(this->inst) == *(vertex.inst)
        ) {
            return true;
        }
        return false;
    }

    std::string __repr__() const noexcept {
        return boost::lexical_cast<std::string>(this->inst_seq) + \
            '-' + name_of_vertex_type.at(this->type);
    }

} Vertex;


template<>
struct std::hash<Vertex> {
    std::size_t operator()(const Vertex& vertex) const {
        return (vertex.inst_seq % WINDOW) * \
            (VertexType::NumOfVertexType + 1) + vertex.type;
    }
};


typedef struct OUTGOING_EDGE {
    Vertex child;
    // weight is for true delay
    latency weight;
    // cost is for critical path computation
    latency cost;
    Bottleneck bottleneck;
    bool critical;
    bool _virtual;

    OUTGOING_EDGE() = delete;
    ~OUTGOING_EDGE() = default;
    OUTGOING_EDGE(
        Vertex child,
        latency weight,
        latency cost,
        Bottleneck& bottleneck,
        bool critical=false
    ):
        child(child), weight(weight),
        cost(cost), bottleneck(bottleneck),
        critical(critical),
        _virtual(false)
        {}
    OUTGOING_EDGE(
        Vertex child,
        latency weight,
        latency cost,
        Bottleneck&& bottleneck,
        bool critical=false
    ):
        child(child), weight(weight),
        cost(cost), bottleneck(bottleneck),
        critical(critical),
        _virtual(false)
        {}

public:
    void mask_critical() {
        critical = true;
    }
    void mask_virtual() {
        _virtual = true;
    }

} OutgoingEdge;


/*
 * DEPRECATED.
// vectorized DEG
typedef struct OUTGOING_EDGE {
    Vertex child;
    std::vector<latency> weight;

    OUTGOING_EDGE() = delete;
    ~OUTGOING_EDGE() = default;
    OUTGOING_EDGE(Vertex child, std::vector<latency> weight):
        child(child), weight(weight) {}
} OutgoingEdge;
*/


typedef struct INGOING_EDGE {
    Vertex parent;

    INGOING_EDGE() = delete;
    ~INGOING_EDGE() = default;
    INGOING_EDGE(
        Vertex& parent
    ):
        parent(parent)
        {}

} IngoingEdge;

#endif
