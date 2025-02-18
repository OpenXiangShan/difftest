/* Author: baichen.bai@alibaba-inc.com */


#ifndef __O3__H_
#define __O3__H_


#include "graph_utils.h"
#include "riscv_instruction.h"


struct UnknownEdge {
    latency from_decode_to_rename;
    latency from_rename_to_dispatch;
    latency from_dispatch_to_issue;
    latency from_issue_to_complete;
    latency from_issue_to_memory;
    latency from_memory_to_complete;
    latency from_complete_to_commit;

    UnknownEdge() = default;
    ~UnknownEdge() = default;

public:
    void reset() {
        from_decode_to_rename = 0;
        from_rename_to_dispatch = 0;
        from_dispatch_to_issue = 0;
        from_issue_to_complete = 0;
        from_issue_to_memory = 0;
        from_memory_to_complete = 0;
        from_complete_to_commit = 0;
    }

    latency length() const {
        return from_decode_to_rename + from_rename_to_dispatch + \
            from_dispatch_to_issue + from_issue_to_complete + \
            from_issue_to_memory + from_memory_to_complete + \
            from_complete_to_commit;
    }
};


class PipelineDelay {
public:
    PipelineDelay() = default;
    ~PipelineDelay() = default;

public:
    /*
     * pre-defined cache hit latency
     */
    static latency icache_hit_latency;
    static latency dcache_int_latency;
};


class O3Graph {
public:
    O3Graph() = default;
    // O3Graph(std::string&, std::string&, std::string&, RiscvInstructionStream*);
    O3Graph(std::unordered_map<std::string, std::string>&, RiscvInstructionStream*);
    ~O3Graph() = default;

public:
    void run();
    void step(std::string line);
    void finalize();
    static const std::vector<std::string>& get_bottlenecks() {
        return bottlenecks;
    }

protected:
    struct VertexHash {
        std::size_t operator()(const Vertex& vertex) const {
            return (vertex.inst_seq % WINDOW) * \
                (VertexType::NumOfVertexType + 1) + vertex.type;
        }
    };

    struct VertexCompare {
        bool operator()(const Vertex& lhs, const Vertex& rhs) const {
            return (lhs.type < rhs.type) || \
                (lhs.type == rhs.type && lhs.inst_seq < rhs.inst_seq);
        }
    };

    struct VertexEqual {
        bool operator()(const Vertex& lhs, const Vertex& rhs) const {
            return (lhs.type == rhs.type) && \
                (lhs.inst_seq % WINDOW == rhs.inst_seq % WINDOW);
        }
    };

protected:
    struct EdgeTimingHash {
        std::size_t operator()(const Vertex& vertex) const {
            return (vertex.inst_seq % WINDOW) * \
                (VertexType::NumOfVertexType + 1) + vertex.type;
        }
    };

    struct EdgeTimingCompare {
        bool operator()(const Vertex& lhs, const Vertex& rhs) const {
            return (lhs.timing < rhs.timing) || \
                (lhs.timing == rhs.timing && lhs.inst_seq < rhs.inst_seq);
        }
    };

    struct EdgeTimingEqual {
        bool operator()(const Vertex& lhs, const Vertex& rhs) const {
            return (lhs.timing == rhs.timing) && \
                (lhs.inst_seq % WINDOW == rhs.inst_seq % WINDOW);
        }
    };

protected:
    struct EdgeInstSeqHash {
        std::size_t operator()(const Vertex& vertex) const {
            return (vertex.inst_seq % WINDOW) * \
                (VertexType::NumOfVertexType + 1) + vertex.type;
        }
    };

    struct EdgeInstSeqCompare {
        bool operator()(const Vertex& lhs, const Vertex& rhs) const {
            return (lhs.inst_seq < rhs.inst_seq) || \
                (lhs.inst_seq == rhs.inst_seq && lhs.timing < rhs.timing);
        }
    };

    struct EdgeInstSeqEqual {
        bool operator()(const Vertex& lhs, const Vertex& rhs) const {
            return (lhs.inst_seq == rhs.inst_seq) || \
                (lhs.inst_seq % WINDOW == rhs.inst_seq % WINDOW);
        }
    };

private:
    void construct_new_deg(int);
    void model(RiscvInstruction*);
    void model_interaction(RiscvInstruction*);
    void model_interaction_impl(RiscvInstruction*);
    bool raw_dep(RiscvInstruction*, RiscvInstruction*);
    bool war_dep(RiscvInstruction*, RiscvInstruction*);
    bool wrw_dep(RiscvInstruction*, RiscvInstruction*);
    void model_miss_bp_interaction_impl(RiscvInstruction*);
    void model_serial_interaction_impl(RiscvInstruction*);
    void model_rob_interaction_impl(RiscvInstruction*);
    void model_lq_interaction_impl(RiscvInstruction*);
    void model_sq_interaction_impl(RiscvInstruction*);
    void model_rf_interaction_impl(RiscvInstruction*);
    void model_iq_interaction_impl(RiscvInstruction*);
    void model_fu_interaction_impl(RiscvInstruction*);
    void model_raw_interaction_impl(RiscvInstruction*, int CONST=50);
    void analyze_window();
    void construct_induced_graph();
    void add_virtual_edge(Vertex&, Vertex&);
    void add_virtual_edge_via_timing();
    void add_virtual_edge_via_timing_impl(
        std::map<Vertex, std::vector<OutgoingEdge>, EdgeTimingCompare>::iterator,
        std::map<Vertex, std::vector<OutgoingEdge>, EdgeTimingCompare>&
    );
    void add_virtual_edge_via_inst_seq();
    void add_virtual_edge_via_inst_seq_impl(
        std::map<Vertex, std::vector<OutgoingEdge>, EdgeInstSeqCompare>::iterator,
        std::map<Vertex, std::vector<OutgoingEdge>, EdgeInstSeqCompare>&
    );
    std::vector<Vertex> longest_path();
    std::vector<Vertex> longest_path_impl();
    Vertex& get_source_vertex();
    Vertex& get_sink_vertex();
    std::vector<Vertex> topological_sort();
    void profiling_critical_path(std::vector<Vertex>&);
    void generate_report(BottleneckReport&, std::vector<Vertex>&);
    bool if_exist_edge(Vertex&, Vertex&);
    bool if_exist_path(Vertex&, Vertex&);
    bool find_path(Vertex&, Vertex&, std::vector<Vertex>&);
    OutgoingEdge& get_outgoing_edge(Vertex&, Vertex&);
    void add_edge(Vertex&, OutgoingEdge&);
    void add_edge(Vertex&, OutgoingEdge&&);
    void add_edge(Vertex&, IngoingEdge&);
    void add_edge(Vertex&, IngoingEdge&&);
    void register_vertex(Vertex&);
    std::string hash_inst(RiscvInstruction*, VertexType&);
    std::string hash_inst(RiscvInstruction*, VertexType&&);
    Vertex get_vertex_from_inst(RiscvInstruction*, VertexType&);
    Vertex get_vertex_from_inst(RiscvInstruction*, VertexType&&);
    void from_f1_to_f2f(RiscvInstruction*, Vertex&, Vertex&);
    void from_f2f_to_d(RiscvInstruction*, Vertex&, Vertex&);
    void from_f1_to_f2(RiscvInstruction*, Vertex&, Vertex&);
    void from_f2_to_f(RiscvInstruction*, Vertex&, Vertex&);
    void from_f_to_d(RiscvInstruction*, Vertex&, Vertex&);
    void from_d_to_r(RiscvInstruction*, Vertex&, Vertex&);
    void from_r_to_di(RiscvInstruction*, Vertex&, Vertex&);
    void from_r_to_ds(RiscvInstruction*, Vertex&, Vertex&);
    void from_ds_to_i(RiscvInstruction*, Vertex&, Vertex&);
    void from_di_to_mp(RiscvInstruction*, Vertex&, Vertex&);
    void from_i_to_mp(RiscvInstruction*, Vertex&, Vertex&);
    void from_di_to_m(RiscvInstruction*, Vertex&, Vertex&);
    void from_m_to_p(RiscvInstruction*, Vertex&, Vertex&);
    void from_i_to_m(RiscvInstruction*, Vertex&, Vertex&);
    void from_di_to_p(RiscvInstruction*, Vertex&, Vertex&);
    void from_i_to_p(RiscvInstruction*, Vertex&, Vertex&);
    void from_mp_to_c(RiscvInstruction*, Vertex&, Vertex&);
    void from_p_to_c(RiscvInstruction*, Vertex&, Vertex&);
    void from_p_to_f1(Vertex&, Vertex&);
    void from_r_to_r_rob(Vertex&, Vertex&);
    void from_r_to_r_lq(Vertex&, Vertex&);
    void from_r_to_r_sq(Vertex&, Vertex&);
    void from_r_to_r_rf(Vertex&, Vertex&, Bottleneck&);
    void from_r_to_r_iq(Vertex&, Vertex&);
    void from_i_to_i_fu(Vertex&, Vertex&, Bottleneck&);
    void from_i_to_i_dcache(Vertex&, Vertex&);
    void from_i_to_i_raw(Vertex&, Vertex&);
    bool from_c_to_sink(Vertex&, Vertex&, std::vector<Vertex>&);
    void from_c_to_ds_serial(Vertex&, Vertex&);
    void visualize();
    void dump_time_stats();
    void clear_time_stats();

public:
    typedef std::map<Vertex, std::vector<OutgoingEdge>, VertexCompare> deg_t;
    typedef std::map<Vertex, std::vector<IngoingEdge>, VertexCompare> pred_t;
    typedef std::map<Vertex, std::vector<OutgoingEdge>, EdgeTimingCompare> timing_skewed_t;
    typedef std::map<Vertex, std::vector<OutgoingEdge>, EdgeInstSeqCompare> inst_seq_skewed_t;

    typedef std::unordered_map<std::string, Vertex> inst_to_vertex_t;

private:
    std::string output_file;
    bool view;
    count_t start;
    count_t end;
    RiscvInstructionStream* inst_stream;
    count_t prev_inst_seq;
    int pipeline_width;
    struct TimeStats {
        runtime_t inst_stream_runtime;
        runtime_t new_deg_construction_runtime;
        runtime_t induced_deg_construction_runtime;
        runtime_t longest_path_runtime;
        runtime_t analysis_runtime;
        runtime_t visualization_runtime;
    } time_stats;
    deg_t graph;
    pred_t pred;
    inst_to_vertex_t inst_to_vertex;
    std::deque<RiscvInstruction*> inst_window;
    static std::vector<std::string> bottlenecks;
};


#endif
