/* Author: baichen.bai@alibaba-inc.com */


#include "deg/o3.h"
#include "deg/visualizer.h"


/*
 * pre-defined cache hit latency
 */
latency PipelineDelay::icache_hit_latency = 2;
latency PipelineDelay::dcache_int_latency = 2;

std::vector<std::string> O3Graph::bottlenecks;

O3Graph::O3Graph(
    std::unordered_map<std::string, std::string>& args,
    RiscvInstructionStream* inst_stream
):
    output_file(args["output"]),
    inst_stream(inst_stream),
    prev_inst_seq(0) {
    // `view` flag
    if (args.find("view") != args.end())
        view = true;
    else
        view = false;
    // `start` flag
    if (args.find("start") != args.end())
        start = std::stoull(args["start"]);
    else
        start = 0;
    // `end` flag
    if (args.find("end") != args.end())
        end = std::stoull(args["end"]);
    else
        end = 0;
    clear_time_stats();
    bottlenecks = register_bottlenecks();
}


void O3Graph::run() {
    INFO("construct new DEG: %s\n", inst_stream->get_name());

    sys_nanoseconds runtime;

    while (true) {
        runtime = timestamp();
        RiscvInstruction* inst = inst_stream->next();

        time_stats.inst_stream_runtime += \
            (timestamp() - runtime).count();

        #ifdef DEBUG_INST_STREAM
            if (inst) {
                TEST("\tinst: %s\n", inst->inst);
                TEST("\tpc: %s\n", inst->pc);
                TEST("\tinst type: %s\n", inst_type.at(inst->inst_type));
                TEST("\tprocess cache completion: %lld\n",
                    inst->process_cache_completion()
                );
                TEST("\tinsert ready list: %lld\n",
                    inst->insert_ready_list()
                );
                TEST("\tcommit: %lld\n", inst->commit());
                TEST("\trob: %d\n", inst->rob);
                TEST("\tiq: %d\n", inst->iq);
                TEST("\tsrc: ");
                for (int i = 0; i < inst->src.size(); i++) {
                    std::cout << inst->src[i];
                    if (i < inst->src.size() - 1)
                        std::cout << ',';
                }
                std::cout << std::endl;
                TEST("\tdst: %d\n", inst->dst);
            }
        #endif

        /* 当建模到了一定的规模，按照 WINDOWS 为粒度进行指令的分析 */
        if (inst_stream->get_inst_seq() > 0 && \
            (inst_stream->get_inst_seq() % WINDOW == 0)) {
            analyze_window();
        }

        if (inst == nullptr) {
            if (inst_stream->get_inst_seq() % WINDOW != 0)
                analyze_window();
            break;
        }

        /* 对下一条指令进行自身流水线的建模 */
        model(inst);
        /* 建立这条指令与其他指令的依赖关联 */
        model_interaction(inst);
    }
    visualize();
    dump_time_stats();
    INFO("run DEG done.\n");
}

void O3Graph::step(std::string line) {
    RiscvInstruction* inst = inst_stream->next_v2(line);
    /* 当建模到了一定的规模，按照 WINDOWS 为粒度进行指令的分析 */
    if (inst_stream->get_inst_seq() > 0 && \
        (inst_stream->get_inst_seq() % WINDOW == 0)) {
        analyze_window();
    }

    if (inst == nullptr) {
        if (inst_stream->get_inst_seq() % WINDOW != 0)
            analyze_window();
        return;
    }

    /* 对下一条指令进行自身流水线的建模 */
    model(inst);
    /* 建立这条指令与其他指令的依赖关联 */
    model_interaction(inst);
}

void O3Graph::finalize() {
    analyze_window();
    visualize();
    dump_time_stats();
    INFO("run DEG done.\n");
}

void O3Graph::model(RiscvInstruction* inst) {
    sys_nanoseconds runtime = timestamp();

    /*
     * 1. F1 -> F2 -> F -> D
     * 2. F1 -> F2/F -> D
     */
    if (inst->process_cache_completion() == inst->fetch()) {
        // F1 -> F2/F -> D
        Vertex F1(VertexType::F1, inst_stream->get_inst_seq(), inst), \
            F2F(VertexType::F2F, inst_stream->get_inst_seq(), inst), \
            D(VertexType::D, inst_stream->get_inst_seq(), inst);

        from_f1_to_f2f(inst, F1, F2F);
        from_f2f_to_d(inst, F2F, D);
    } else {
        // F1 -> F2 -> F -> D
        Vertex F1(VertexType::F1, inst_stream->get_inst_seq(), inst), \
            F2(VertexType::F2, inst_stream->get_inst_seq(), inst), \
            F(VertexType::F, inst_stream->get_inst_seq(), inst), \
            D(VertexType::D, inst_stream->get_inst_seq(), inst);

        from_f1_to_f2(inst, F1, F2);
        from_f2_to_f(inst, F2, F);
        from_f_to_d(inst, F, D);
    }

    // D -> R
    Vertex D(VertexType::D, inst_stream->get_inst_seq(), inst), \
        R(VertexType::R, inst_stream->get_inst_seq(), inst);
    from_d_to_r(inst, D, R);

    /*
     * 1. R -> D -> I
     * 2. R -> D/I
     */
    if (inst->dispatch() == inst->issue()) {
        // R -> D/I
        Vertex R(VertexType::R, inst_stream->get_inst_seq(), inst), \
            DI(VertexType::DI, inst_stream->get_inst_seq(), inst);

        from_r_to_di(inst, R, DI);
    } else {
        // R -> D -> I

        Vertex R(VertexType::R, inst_stream->get_inst_seq(), inst), \
            DS(VertexType::DS, inst_stream->get_inst_seq(), inst), \
            I(VertexType::I, inst_stream->get_inst_seq(), inst);

        from_r_to_ds(inst, R, DS);
        from_ds_to_i(inst, DS, I);
    }

    /*
     * 1. I -> P (normal),
     * 2. I -> MP -> C (store/load),
     * 3. I -> M -> P (load),
     * 4. DI -> P (normal),
     * 5. DI -> MP -> C (store/load),
     * 6. DI -> M -> P (load)
     */
    if (inst->is_mem()) {
        if (inst->is_store()) {
            if (inst->dispatch() == inst->issue()) {
                // DI -> MP -> C
                Vertex DI(VertexType::DI, inst_stream->get_inst_seq(), inst), \
                    MP(VertexType::MP, inst_stream->get_inst_seq(), inst), \
                    C(VertexType::C, inst_stream->get_inst_seq(), inst);

                from_di_to_mp(inst, DI, MP);
                from_mp_to_c(inst, MP, C);
            } else {
                // I -> MP -> C
                Vertex I(VertexType::I, inst_stream->get_inst_seq(), inst), \
                    MP(VertexType::MP, inst_stream->get_inst_seq(), inst), \
                    C(VertexType::C, inst_stream->get_inst_seq(), inst);

                from_i_to_mp(inst, I, MP);
                from_mp_to_c(inst, MP, C);
            }
        } else {
            if (inst->dispatch() == inst->issue()) {
                if (inst->memory() == inst->complete_memory()) {
                    // DI -> MP -> C
                    Vertex DI(VertexType::DI, inst_stream->get_inst_seq(), inst), \
                        MP(VertexType::MP, inst_stream->get_inst_seq(), inst), \
                        C(VertexType::C, inst_stream->get_inst_seq(), inst);

                    from_di_to_mp(inst, DI, MP);
                    from_mp_to_c(inst, MP, C);
                } else {
                    // DI -> M -> P -> C
                    Vertex DI(VertexType::DI, inst_stream->get_inst_seq(), inst), \
                        M(VertexType::M, inst_stream->get_inst_seq(), inst), \
                        P(VertexType::P, inst_stream->get_inst_seq(), inst), \
                        C(VertexType::C, inst_stream->get_inst_seq(), inst);

                    from_di_to_m(inst, DI, M);
                    from_m_to_p(inst, M, P);
                    from_p_to_c(inst, P, C);
                }
            } else {
                if (inst->memory() == inst->complete_memory()) {
                    // I -> MP -> C
                    Vertex I(VertexType::I, inst_stream->get_inst_seq(), inst), \
                        MP(VertexType::MP, inst_stream->get_inst_seq(), inst), \
                        C(VertexType::C, inst_stream->get_inst_seq(), inst);

                    from_i_to_mp(inst, I, MP);
                    from_mp_to_c(inst, MP, C);
                } else {
                    // I -> M -> P -> C
                    Vertex I(VertexType::I, inst_stream->get_inst_seq(), inst), \
                        M(VertexType::M, inst_stream->get_inst_seq(), inst),
                        P(VertexType::P, inst_stream->get_inst_seq(), inst), \
                        C(VertexType::C, inst_stream->get_inst_seq(), inst);

                    from_i_to_m(inst, I, M);
                    from_m_to_p(inst, M, P);
                    from_p_to_c(inst, P, C);
                }
            }
        }
    } else {
        if (inst->dispatch() == inst->issue()) {
            // DI -> P -> C
            Vertex DI(VertexType::DI, inst_stream->get_inst_seq(), inst), \
                P(VertexType::P, inst_stream->get_inst_seq(), inst), \
                C(VertexType::C, inst_stream->get_inst_seq(), inst);

            from_di_to_p(inst, DI, P);
            from_p_to_c(inst, P, C);
        } else {
            // I -> P -> C
            Vertex I(VertexType::I, inst_stream->get_inst_seq(), inst), \
                P(VertexType::P, inst_stream->get_inst_seq(), inst), \
                C(VertexType::C, inst_stream->get_inst_seq(), inst);

            from_i_to_p(inst, I, P);
            from_p_to_c(inst, P, C);
        }
    }

    time_stats.new_deg_construction_runtime += (timestamp() - runtime).count();
}


void O3Graph::model_interaction(RiscvInstruction* inst) {
    model_interaction_impl(inst);
    inst_window.emplace_front(inst);
}


void O3Graph::model_interaction_impl(RiscvInstruction* inst) {
    if (inst_window.empty())
        return;

    // 控制依赖
    model_miss_bp_interaction_impl(inst);
    model_serial_interaction_impl(inst);
    // 资源依赖
    model_rob_interaction_impl(inst);
    model_lq_interaction_impl(inst);
    model_sq_interaction_impl(inst);
    model_rf_interaction_impl(inst);
    model_iq_interaction_impl(inst);
    model_fu_interaction_impl(inst);
    // 数据依赖
    model_raw_interaction_impl(inst);
}


bool O3Graph::raw_dep(RiscvInstruction *tgt, RiscvInstruction* src) {
    PPK_ASSERT(tgt->inst_seq > src->inst_seq,
        "target inst. seq: %lu should be greater than source inst. seq: %lu",
        tgt->inst_seq, src->inst_seq
    );
    auto ret = std::find(tgt->src.begin(), tgt->src.end(), src->dst);
    if (ret != tgt->src.end())
        return true;
    return false;
}


bool O3Graph::war_dep(RiscvInstruction *tgt, RiscvInstruction* src) {
    PPK_ASSERT(tgt->inst_seq > src->inst_seq,
        "target inst. seq: %lu should be greater than source inst. seq: %lu",
        tgt->inst_seq, src->inst_seq
    );
    auto ret = std::find(src->src.begin(), src->src.end(), tgt->dst);
    if (ret != src->src.end())
        return true;
    return false;
}


bool O3Graph::wrw_dep(RiscvInstruction *tgt, RiscvInstruction* src) {
    return tgt->dst == src->dst;
}


void O3Graph::model_miss_bp_interaction_impl(RiscvInstruction* inst) {
    auto prev = inst_window.front();

    // 前一条是分支，且本条指令顺序执行
    if (prev->is_control()) {
        if (prev->complete() < inst->fetch_cache_line()) {
            // miss branch prediction
            auto P = get_vertex_from_inst(prev, VertexType::P), \
                F1 = get_vertex_from_inst(inst, VertexType::F1);
            from_p_to_f1(P, F1);
        }
    }
}

void O3Graph::model_serial_interaction_impl(RiscvInstruction* inst) {
    auto prev = inst_window.front();

    if (inst->block_from_serial() != 0) {
        // serial dependence
        assert(prev->complete() < inst->dispatch());
        auto C = get_vertex_from_inst(prev, VertexType::C), \
            DS = get_vertex_from_inst(inst, VertexType::DS);
        from_c_to_ds_serial(C, DS);
    }
}

void O3Graph::model_rob_interaction_impl(RiscvInstruction* inst) {
    if (inst->block_from_rob() != 0) {
        // an ROB dependence
        // 根据 ROB 阻塞的信号，将同一个 rob entry 的指令依赖关系建立起来
        for (auto iter = inst_window.cbegin(); iter != inst_window.cend(); iter++) {
            if ((*iter)->rob == inst->rob) {
                auto _R = get_vertex_from_inst(*iter, VertexType::R), \
                    R = get_vertex_from_inst(inst, VertexType::R);
                from_r_to_r_rob(_R, R);
                break;
            }
        }
        // consider a serial
    }
}


void O3Graph::model_lq_interaction_impl(RiscvInstruction* inst) {
    if (inst->block_from_lq() != 0 && inst->is_load()) {
        // LQ dependence
        for (auto iter = inst_window.cbegin(); iter != inst_window.cend(); iter++) {
            if ((*iter)->lq == inst->lq) {
                auto _R = get_vertex_from_inst(*iter, VertexType::R), \
                    R = get_vertex_from_inst(inst, VertexType::R);
                from_r_to_r_lq(_R, R);
                break;
            }
        }
    }
}


void O3Graph::model_sq_interaction_impl(RiscvInstruction* inst) {
    if (inst->block_from_sq() != 0 && inst->is_store()) {
        // SQ dependence
        for (auto iter = inst_window.cbegin(); iter != inst_window.cend(); iter++) {
            if ((*iter)->sq == inst->sq) {
                auto _R = get_vertex_from_inst(*iter, VertexType::R), \
                    R = get_vertex_from_inst(inst, VertexType::R);
                from_r_to_r_sq(_R, R);
                break;
            }
        }
    }
}


void O3Graph::model_rf_interaction_impl(RiscvInstruction* inst) {
    if (inst->block_from_rf() != 0) {
        // RF dependence
        RiscvInstruction* dep = nullptr;

        for (auto iter = inst_window.cbegin(); iter != inst_window.cend(); iter++) {
            if (war_dep(inst, *iter) || wrw_dep(inst, *iter)) {
                dep = *iter;
                break;
            }
        }

        if (dep) {
            /*
             * `dep` could be `nullptr` since the trace could be imcomplete.
             */
            auto _R = get_vertex_from_inst(dep, VertexType::R), \
                R = get_vertex_from_inst(inst, VertexType::R);

            Bottleneck bottleneck;
            if (inst->use_int_rf()) {
                bottleneck = IntRF();
            } else if (inst->use_fp_rf()) {
                bottleneck = FpRF();
            } else {
                // No_OpClass could reach here
                return;
            }
            from_r_to_r_rf(_R, R, bottleneck);
        }
    }
}


void O3Graph::model_iq_interaction_impl(RiscvInstruction* inst) {
    if (inst->block_from_iq() != 0) {
        for (auto iter = inst_window.cbegin(); iter != inst_window.cend(); iter++) {
            if ((*iter)->iq == inst->iq) {
                auto _R = get_vertex_from_inst(*iter, VertexType::R), \
                    R = get_vertex_from_inst(inst, VertexType::R);
                from_r_to_r_iq(_R, R);
                break;
            }
        }
    }
}


void O3Graph::model_fu_interaction_impl(RiscvInstruction* inst) {
    if (inst->issue() > inst->insert_ready_list()) {
        for (auto iter = inst_window.cbegin(); iter != inst_window.cend(); iter++) {
            /*
             * `*iter` sould be completed before `inst`?
             */
            if ((*iter)->fu == inst->fu && (*iter)->issue() < inst->issue() && !raw_dep(inst, *iter)) {
                bool dep = false;
                if (inst->is_load() && (*iter)->complete_memory() >= inst->issue())
                    dep = true;
                else if ((*iter)->complete() >= inst->issue())
                    dep = true;
                if (dep) {
                    auto _I = get_vertex_from_inst(*iter, VertexType::I), \
                        I = get_vertex_from_inst(inst, VertexType::I);
                    Bottleneck bottleneck;
                    if (inst->use_int_alu())
                        bottleneck = IntALU();
                    else if (inst->use_int_mult_div())
                        bottleneck = IntMultDiv();
                    else if (inst->use_fp_alu())
                        bottleneck = FpAlu();
                    else if (inst->use_fp_mult_div())
                        bottleneck = FpMultDiv();
                    else if (inst->use_rd_wr_port())
                        bottleneck = RdWrPort();
                    else {
                        // No_OpClass could reach here
                        continue;
                    }
                    from_i_to_i_fu(_I, I, bottleneck);
                    break;
                }
            }
        }
    }
}


void O3Graph::model_raw_interaction_impl(RiscvInstruction* inst, int CONST) {
    /*
     * We set `CONST` here to accelerate the dependency search process.
     * We make an assumption that at most, the longest RAW dependence is
     * within `CONST` instructions in an instruction window.
     */
    if (inst->insert_ready_list() > inst->dispatch()) {
        int i = 0;
        tick t = 0;
        RiscvInstruction* dep = nullptr;
        while (i < inst_window.size()) {
            auto prev = inst_window.at(i);
            if (raw_dep(inst, prev)) {
                auto j = 0;
                while (j < CONST && i < inst_window.size()) {
                    /*
                     * We find the latest instruction that does
                     * has the RAW dependence.
                     */
                    auto prev = inst_window.at(j);
                    if (raw_dep(inst, prev)) {
                        if (prev->is_load()) {
                            if (t < prev->complete_memory()) {
                                t = prev->complete_memory();
                                dep = prev;
                            }
                        } else {
                            if (t < prev->complete()) {
                                t = prev->complete();
                                dep = prev;
                            }
                        }
                    }
                    j += 1;
                    i += 1;
                }
            }
            if (dep)
                break;
            i += 1;
        }
        if (dep) {
            /*
             * `dep` can be `nullptr`. So, we ignore if we have this case.
             */
            auto _I = get_vertex_from_inst(dep, VertexType::I), \
                I = get_vertex_from_inst(inst, VertexType::I);

            if (dep->is_load())
                from_i_to_i_dcache(_I, I);
            else
                from_i_to_i_raw(_I, I);
        }
    }
}


void O3Graph::analyze_window() {
    /*
     * We apply a dynamic programming method
     * to construct the graph. The idea is inspired from ICCAD'13.
     * "Methodology for standard cell compliance and detailed placement
     * for triple patterning lithography".
     * We apply the longest path algorithm to construct the critical path.
     * Several steps are shown below:
     * 1. Add virtual edges for cross-instruction dependencies.
     * The virtual edges are added following the timestamp.
     * We call this step as "contructing an induced DEG".
     * 2. Apply the longest path algorithm in the new graph.
     * And costs are assigned for each edge:
     * 1). horizontal edge: 0
     * 2). virual edge: 0
     * 3). cross-instruction dependence: `delay`
    */
    INFO("constuct induced DEG.\n");

    sys_nanoseconds runtime = timestamp();
    construct_induced_graph();
    time_stats.induced_deg_construction_runtime += \
        (timestamp() - runtime).count();

    INFO("analysis induced DEG.\n");
    runtime = timestamp();
    auto path = longest_path();
    time_stats.analysis_runtime += (timestamp() - runtime).count();

    profiling_critical_path(path);
}


void O3Graph::add_virtual_edge(Vertex& parent, Vertex& child) {
    /*
     * Connect between the vertex `parent` & `child`.
     * We directly connect `parent` & `child` with
     * a virtual edge.
     */
    if (parent == child || if_exist_edge(parent, child))
        return;
    else {
        latency weight;

        weight = child.timing - parent.timing;
        auto edge = OutgoingEdge(child, weight, 0, Virtual());
        edge.mask_virtual();
        add_edge(parent, edge);
        add_edge(child, IngoingEdge(parent));
    }
}


void O3Graph::construct_induced_graph() {
    /*
     * For each skewed edge, we add virtual edge
     * to connect them. The rule to add virtual
     * edge is: for each skewed edge's `v` node,
     * we connect the next skewed edge's `u` node.
     * The next skewed edge should satisfy some
     * rules as shown below:
     * 1. We select all next skewed edges whose
     * `seq` are the closest to the current skewed
     * edge.
     * 2. We select all next skewed edges whose
     * `u`'s timing are the closest to the current
     * skewed edge.
     * We can parallelize the virtual edge addition.
    */
    // std::thread t1([this]() {
    //         this->add_virtual_edge_via_timing();
    //     });
    // std::thread t2([this]() {
    //         this->add_virtual_edge_via_inst_seq();
    //     });

    // t1.join();
    // t2.join();
    add_virtual_edge_via_timing();
    add_virtual_edge_via_inst_seq();
}


void O3Graph::add_virtual_edge_via_timing() {
    /*
     * At first, we find out all skewed edges, and sort them
     * via timing.
     */
    timing_skewed_t skewed_edges;

    for (auto i = graph.begin(); i != graph.end(); i++) {
        for (auto j = i->second.begin(); j != i->second.end(); j++) {
            if (i->first.inst_seq != j->child.inst_seq) {
                skewed_edges[i->first].emplace_back(*j);
            }
        }
    }

    for (auto iter = skewed_edges.begin(); iter != skewed_edges.end(); iter++) {
        add_virtual_edge_via_timing_impl(iter, skewed_edges);
    }

    // /*
    //  * Add head to tail virtual edges.
    //  * Refer it to `add_virtual_edge_via_timing_impl`.
    //  */
    // // source
    // auto source = get_source_vertex();
    // tick early_timing;
    // for (auto iter = skewed_edges.begin(); iter != skewed_edges.end(); iter++) {
    //     if (iter->first.inst_seq > source.inst_seq) {
    //         add_virtual_edge(source, const_cast<Vertex&>(iter->first));
    //         early_timing = source.timing;

    //         for (auto i = ++iter; i != skewed_edges.end(); i++) {
    //             if (i->first.inst_seq > source.inst_seq && \
    //                 early_timing == i->first.timing
    //                 ) {
    //                 add_virtual_edge(source, const_cast<Vertex&>(i->first));
    //                 break;
    //             }
    //         }
    //         break;
    //     }
    // }

    // // tail
    // auto sink = get_sink_vertex();
    // auto iter = skewed_edges.rbegin();
    // tick late_timing = iter->second.rbegin()->child.timing;
    // for (auto i = iter->second.rbegin(); i != iter->second.rend(); i--) {
    //     if (i->child.timing >= late_timing) {
    //         /*
    //          * `i->child.timing` could be larger than `late_timing`.
    //          * We still connect them.
    //          */
    //         add_virtual_edge(const_cast<Vertex&>(i->child), sink);
    //     } else {
    //         break;
    //     }
    // }
}


void O3Graph::add_virtual_edge_via_timing_impl(
    timing_skewed_t::iterator iter,
    timing_skewed_t& skewed_edges
) {
    /*
     * We find vertices crossing instruction edges, whose
     * timing is the closest to `child`.
     */
    auto _add_virtual_edge_via_timing_impl = [iter, &skewed_edges, this](Vertex& vertex) mutable {
        tick early_timing;
        for (auto i = iter; i != skewed_edges.end(); i++) {
            /*
             * We iterate `skew_edges` until we find the next instruction
             * whose `inst_seq` is greater than `vertex`'s `inst_seq`.
             */
            if (i->first.timing >= vertex.timing && \
                (i->first.inst_seq > vertex.inst_seq)
            ) {
                /*
                 * We do not add the virtual edge if
                 * `i->first.inst_seq == vertex.inst_seq` since
                 * they are already connected.
                 */
                this->add_virtual_edge(vertex, const_cast<Vertex&>(i->first));
                early_timing = i->first.timing;
                /*
                 * We continue to find if there exist instructions whose
                 * `inst_seq` is equal to the one we have found just now.
                 */
                for (auto j = ++i; j != skewed_edges.end(); j++) {
                    if (j->first.inst_seq > vertex.inst_seq && \
                        early_timing == j->first.timing) {
                        this->add_virtual_edge(vertex, const_cast<Vertex&>(j->first));
                    } else {
                        break;
                    }
                }
                break;
            }
        }
    };

    _add_virtual_edge_via_timing_impl(const_cast<Vertex&>(iter->first));
    for (auto i = iter->second.begin(); i != iter->second.end(); i++) {
        _add_virtual_edge_via_timing_impl(i->child);
    }
}


void O3Graph::add_virtual_edge_via_inst_seq() {
    /*
     * At first, we find out all skewed edges, and sort them
     * via instruction sequence.
     */
    inst_seq_skewed_t skewed_edges;

    for (auto i = graph.begin(); i != graph.end(); i++) {
        for (auto j = i->second.begin(); j != i->second.end(); j++) {
            if (i->first.inst_seq != j->child.inst_seq) {
                skewed_edges[i->first].emplace_back(*j);
            }
        }
    }

    for (auto iter = skewed_edges.begin(); iter != skewed_edges.end(); iter++) {
        add_virtual_edge_via_inst_seq_impl(iter, skewed_edges);
    }

    // /*
    //  * Add head to tail virtual edges.
    //  * Refer it to `add_virtual_edge_via_inst_seq_impl`.
    //  */
    // // source
    // auto source = get_source_vertex();
    // for (auto iter = skewed_edges.begin(); iter != skewed_edges.end(); iter++) {
    //     if (iter->first.inst_seq > source.inst_seq) {
    //         add_virtual_edge(source, const_cast<Vertex&>(iter->first));
    //         break;
    //     }
    // }

    // // tail
    // auto sink = get_sink_vertex();
    // auto iter = skewed_edges.rbegin();
    // auto inst_seq = iter->second.rbegin()->child.inst_seq;
    // add_virtual_edge(const_cast<Vertex&>(iter->second.rbegin()->child), sink);
    // for (auto i = iter->second.rbegin(); i != iter->second.rend(); i--) {
    //     if (i->child.inst_seq >= inst_seq) {
    //         /*
    //          * `i->child.timing` could be larger than `late_timing`.
    //          * We still connect them.
    //          */
    //         add_virtual_edge(const_cast<Vertex&>(i->child), sink);
    //         // /*
    //         //  * We also connect `C` of `i->child.inst_seq` to `sink`.
    //         //  */
    //         // from_c_to_sink(i->child, sink);
    //     } else {
    //         break;
    //     }
    // }
}


void O3Graph::add_virtual_edge_via_inst_seq_impl(
    inst_seq_skewed_t::iterator iter,
    inst_seq_skewed_t& skewed_edges
) {
    /*
     * We find vertices crossing instruction edges, whose
     * instruction sequence is the closest to `child`.
     */
    auto _add_virtual_edge_via_inst_seq_impl = [iter, &skewed_edges, this](Vertex& vertex) mutable {
        for (auto i = iter; i != skewed_edges.end(); i++) {
            if (i->first.inst_seq > vertex.inst_seq && \
                (i->first.timing >= vertex.timing)
            ) {
                this->add_virtual_edge(vertex, const_cast<Vertex&>(i->first));
                return;
            }
        }
    };

    _add_virtual_edge_via_inst_seq_impl(const_cast<Vertex&>(iter->first));
    for (auto i = iter->second.begin(); i != iter->second.end(); i++) {
        _add_virtual_edge_via_inst_seq_impl(i->child);
    }
}


std::vector<Vertex> O3Graph::longest_path() {
    /*
     * We apply the longest path on the induced graph.
     */
    INFO("apply longest path.\n");
    auto runtime = timestamp();
    auto path = longest_path_impl();
    time_stats.longest_path_runtime += (timestamp() - runtime).count();

    // mask critical edges
    for (auto i = 0; i < path.size() - 1; i++) {
        get_outgoing_edge(path[i], path[i + 1]).mask_critical();
    }

    return path;
}


std::vector<Vertex> O3Graph::longest_path_impl() {
    auto longest_path_core = [this]() -> std::vector<Vertex> {
        std::vector<Vertex> path;
        std::map<Vertex, std::pair<latency, Vertex>, VertexCompare> dist;

        // topological sort
        auto topo = this->topological_sort();

        auto _update_dist = [this, &dist](Vertex& iter) {
            std::vector<std::pair<latency, Vertex>> pairs;
            for (auto i = this->pred[iter].begin(); i != this->pred[iter].end(); i++) {
                auto edge = this->get_outgoing_edge(i->parent, const_cast<Vertex&>(iter));
                pairs.emplace_back(std::pair<latency, Vertex>(
                        dist[i->parent].first + edge.cost, i->parent
                    )
                );
            }
            if (pairs.size()) {
                latency l = -1;
                std::pair<latency, Vertex> pair;
                for (auto j = pairs.begin(); j != pairs.end(); j++) {
                    if (j->first > l) {
                        pair = *j;
                        l = j->first;
                    }
                }
                dist[iter] = pair;
            } else {
                std::pair<latency, Vertex> pair(0, iter);
                dist[iter] = pair;
            }
        };

        for (auto iter = topo.begin(); iter != topo.end(); iter++) {
            _update_dist(const_cast<Vertex&>(*iter));
        }

        // extract the vertex whose latency is the largest
        latency l = -1;
        Vertex v;
        for (auto iter = dist.begin(); iter != dist.end(); iter++) {
            if (iter->second.first > l) {
                l = iter->second.first;
                v = iter->first;
            }
        }

        while (l) {
            path.emplace_back(v);
            l = dist[v].first;
            v = dist[v].second;
        }

        std::reverse(path.begin(), path.end());

        #ifdef DEBUG_CRITICAL_PATH
            TEST("longest_path_core:\n");
            for (auto i = path.begin(); i != path.end(); i++) {
                std::cout << i->__repr__().c_str();
                if ((i + 1) == path.end())
                    std::cout << std::endl;
                else
                    std::cout << " => ";
            }
        #endif

        return path;
    };

    auto path = longest_path_core();

    /*
     * We need to directly add head & tail.
     * If no path exists, we construct the virtual edge.
     */
    auto source = get_source_vertex();
    if (!if_exist_path(source, path[0])) {
        /*
         * If no path between `source` and `path[0]` exists,
         * we construct a virtual edge to connect them directly.
         */
        add_virtual_edge(source, path[0]);
        path.emplace(path.begin(), source);
    } else {
        std::vector<Vertex> _path;
        find_path(source, path[0], _path);
        path.insert(path.begin(), _path.begin(), _path.end());
    }

    auto sink = get_sink_vertex();
    if (!if_exist_path(path.back(), sink)) {
        /*
         * since no path exists, we connect `C` of
         * `path.back()` to `sink`.
         */
        std::vector<Vertex> _path;
        from_c_to_sink(path.back(), sink, _path);
        path.insert(path.end(), _path.begin() + 1, _path.end());
    } else {
        std::vector<Vertex> _path;
        find_path(path.back(), sink, _path);
        path.insert(path.end(), _path.begin() + 1, _path.end());
    }
    path.insert(path.end(), sink);

    #ifdef DEBUG_CRITICAL_PATH
        TEST("entire path:\n");
        for (auto i = path.begin(); i != path.end(); i++) {
            std::cout << i->__repr__().c_str();
            if ((i + 1) == path.end())
                std::cout << std::endl;
            else
                std::cout << " => ";
        }
    #endif

    return path;
}


std::vector<Vertex> O3Graph::topological_sort() {
    /*
     * Topological sorting, refer it to:
     * https://github.com/networkx/networkx/blob/main/networkx/algorithms/dag.py#L167
     */
    std::unordered_map<Vertex, int> indegree_map;
    std::vector<Vertex> zero_indegree, ret;

    for (auto iter = pred.begin(); iter != pred.end(); iter++) {
        indegree_map[iter->first] = iter->second.size();
    }

    for (auto iter = graph.begin(); iter != graph.end(); iter++) {
        if (indegree_map.find(iter->first) == indegree_map.end()) {
            zero_indegree.emplace_back(iter->first);
        }
    }

    while (zero_indegree.size()) {
        std::vector<Vertex> this_generation(zero_indegree);
        zero_indegree.clear();

        for (auto i = this_generation.begin(); i != this_generation.end(); i++) {
            for (auto j = graph[*i].begin(); j != graph[*i].end(); j++) {
                indegree_map[j->child] -= 1;
                if (indegree_map[j->child] == 0) {
                    zero_indegree.emplace_back(j->child);
                    indegree_map.erase(j->child);
                }
            }
        }
        if (ret.size() == 0) {
            ret = this_generation;
        } else {
            ret.insert(ret.end(), this_generation.begin(), this_generation.end());
        }
    }

    PPK_ASSERT(
        indegree_map.size() == 0,
        "new DEG contains a cycle or DEG " \
        "changed during iteration, while " \
        "indegree_map size: %lu\n",
        indegree_map.size()
    );

    PPK_ASSERT(
        ret.size() == graph.size(),
        "in topological sort, ret: %lu vs. graph: %lu.\n",
        ret.size(), graph.size()
    );

    return ret;
}


Vertex& O3Graph::get_source_vertex() {
    Vertex& vertex = const_cast<Vertex&>(graph.begin()->first);
    // for (auto iter = graph.begin(); iter != graph.end(); iter++) {
    //     if (iter->first.inst_seq == 1 && iter->first.type == VertexType::F1)
    //         return const_cast<Vertex&>(iter->first);
    // }
    PPK_ASSERT(
        vertex.inst_seq == 1 && vertex.type == VertexType::F1,
        "inst. seq = %lu, type: %s\n",
        vertex.inst_seq,
        name_of_vertex_type.at(vertex.type).c_str()
    );
    // DEG_UNREACHABLE;
    return vertex;
}


Vertex& O3Graph::get_sink_vertex() {
    for (auto i = graph.begin(); i != graph.end(); i++) {
        for (auto j = graph[i->first].begin(); j != graph[i->first].end(); j++) {
            if (j->child.inst_seq == inst_stream->get_inst_seq() && \
                j->child.type == VertexType::C
            )
                return const_cast<Vertex&>(j->child);
        }
    }
    ERROR("cannot find sink vertex: inst. seq = %llu, type: C.\n",
        inst_stream->get_inst_seq()
    );
}


void O3Graph::profiling_critical_path(std::vector<Vertex>& path) {
    BottleneckReport report;
    latency length = 0;

    for (int i = 0; i < path.size() - 1; i++) {
        auto edge = get_outgoing_edge(path[i], path[i + 1]);
        const std::string name = edge.bottleneck.__repr__();
        length += edge.weight;
        if (name == bottlenecks[BIdx::BTK_Base]) {
            report.incr_base(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_IcacheMiss]) {
            report.incr_icache_miss(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_DcacheMiss]) {
            report.incr_dcache_miss(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_BPMiss]) {
            report.incr_bp_miss(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_ROB]) {
            report.incr_rob(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_LQ]) {
            report.incr_lq(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_SQ]) {
            report.incr_sq(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_IntRF]) {
            report.incr_int_rf(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_FpRF]) {
            report.incr_fp_rf(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_IQ]) {
            report.incr_iq(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_IntAlu]) {
            report.incr_int_alu(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_IntMultDiv]) {
            report.incr_int_mult_div(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_FpAlu]) {
            report.incr_fp_alu(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_FpMultDiv]) {
            report.incr_fp_mult_div(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_RdWrPort]) {
            report.incr_rd_wr_port(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_RAW]) {
            report.incr_raw(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_Virtual]) {
            report.incr_virtual(edge.weight);
        } else if (name == bottlenecks[BIdx::BTK_Serial]) {
            report.incr_serial(edge.weight);
        } else {
            ERROR("no bottleneck: %s\n", name);
        }
    }

    report.set_length(length);

    latency golden_length = get_sink_vertex().timing - \
        get_source_vertex().timing;

    PPK_ASSERT(golden_length == length,
        "golden length: %f vs. length: %f\n",
        golden_length, length
    );

    generate_report(report, path);
}


void O3Graph::generate_report(BottleneckReport& report, std::vector<Vertex>& path) {
    std::ofstream os;
    os.open(output_file, std::ios::out);

    os << "critical path: " << report.get_length() << std::endl;

    for (int i = 0; i < path.size() - 1; i++) {
        auto edge = get_outgoing_edge(path[i], path[i + 1]);
        os << path[i].__repr__() << " => " << \
            path[i + 1].__repr__() << "\t: " << \
            edge.bottleneck.__repr__() << '\t' << \
            edge.weight << std::endl;
    }

    os << std::endl;
    os << "bottleneck: " << std::endl;
    for (auto iter = bottlenecks.begin(); iter != bottlenecks.end(); iter++) {
        latency weight = 0;
        auto name = *iter;
        if (name == bottlenecks[BIdx::BTK_Base]) {
            weight = report.get_base();
        } else if (name == bottlenecks[BIdx::BTK_IcacheMiss]) {
            weight = report.get_icache_miss();
        } else if (name == bottlenecks[BIdx::BTK_DcacheMiss]) {
            weight = report.get_dcache_miss();
        } else if (name == bottlenecks[BIdx::BTK_BPMiss]) {
            weight = report.get_bp_miss();
        } else if (name == bottlenecks[BIdx::BTK_ROB]) {
            weight = report.get_lack_rob();
        } else if (name == bottlenecks[BIdx::BTK_LQ]) {
            weight = report.get_lack_lq();
        } else if (name == bottlenecks[BIdx::BTK_SQ]) {
            weight = report.get_lack_sq();
        } else if (name == bottlenecks[BIdx::BTK_IntRF]) {
            weight = report.get_lack_int_rf();
        } else if (name == bottlenecks[BIdx::BTK_FpRF]) {
            weight = report.get_lack_fp_rf();
        } else if (name == bottlenecks[BIdx::BTK_IQ]) {
            weight = report.get_lack_iq();
        } else if (name == bottlenecks[BIdx::BTK_IntAlu]) {
            weight = report.get_lack_int_alu();
        } else if (name == bottlenecks[BIdx::BTK_IntMultDiv]) {
            weight = report.get_lack_int_mult_div();
        } else if (name == bottlenecks[BIdx::BTK_FpAlu]) {
            weight = report.get_lack_fp_alu();
        } else if (name == bottlenecks[BIdx::BTK_FpMultDiv]) {
            weight = report.get_lack_fp_mult_div();
        } else if (name == bottlenecks[BIdx::BTK_RdWrPort]) {
            weight = report.get_lack_rd_wr_port();
        } else if (name == bottlenecks[BIdx::BTK_RAW]) {
            weight = report.get_raw();
        } else if (name == bottlenecks[BIdx::BTK_Virtual]) {
            weight = report.get_virtual();
        } else if (name == bottlenecks[BIdx::BTK_Serial]) {
            weight = report.get_serial();
        } else {
            ERROR("no bottleneck: %s\n", name);
        }
        os << *iter << ": " << weight << std::endl;
    }

    os.close();

    INFO("generate bottleneck report: %s\n", output_file);
}


bool O3Graph::if_exist_edge(Vertex& parent, Vertex& child) {
    for (auto iter = graph[parent].begin(); iter != graph[parent].end(); iter++) {
        if (iter->child == child) {
            return true;
        }
    }
    return false;
}


bool O3Graph::if_exist_path(Vertex& source, Vertex& target) {
    /*
     * BFS search, refer it to:
     * https://github.com/networkx/networkx/blob/53be757de9a87a3413943737fbf8478bb23b17c7/networkx/algorithms/traversal/breadth_first_search.py#L19
     */
    std::vector<std::pair<Vertex, std::vector<OutgoingEdge>>> next_parents_children;
    std::vector<Vertex> seen;

    if (source.inst->inst_seq > target.inst->inst_seq)
        return false;

    next_parents_children.emplace_back(
        std::pair<Vertex, std::vector<OutgoingEdge>>(source, graph[source])
    );
    while (next_parents_children.size()) {
        auto this_generation(next_parents_children);
        next_parents_children.clear();
        for (auto i = this_generation.begin(); i != this_generation.end(); i++) {
            for (auto j = i->second.begin(); j != i->second.end(); j++) {
                if (j->child.inst->inst_seq > target.inst->inst_seq)
                    return false;
                if (if_exist_edge(j->child, target))
                    return true;
                if (std::find(seen.begin(), seen.end(), j->child) == seen.end()) {
                    seen.emplace_back(j->child);
                } else {
                    return false;
                }
                next_parents_children.emplace_back(
                    std::pair<Vertex, std::vector<OutgoingEdge>>(j->child, graph[j->child])
                );
            }
        }
    }
    return false;
}


bool O3Graph::find_path(Vertex& source, Vertex& target, std::vector<Vertex>& path) {
    if (if_exist_edge(source, target)) {
        path.insert(path.begin(), source);
        return true;
    }
    for (auto i = graph[source].begin(); i != graph[source].end(); i++) {
        if (find_path(i->child, target, path)) {
            path.insert(path.begin(), source);
            return true;
        }
    }
    return false;
}

OutgoingEdge& O3Graph::get_outgoing_edge(Vertex& parent, Vertex& child) {
    for (auto iter = graph[parent].begin(); iter != graph[parent].end(); iter++) {
        if (iter->child == child) {
            return *iter;
        }
    }
    // DEG_UNREACHABLE;
    ERROR("cannot find edge: %s => %s.\n",
        hash_inst(parent.inst, parent.type).c_str(),
        hash_inst(child.inst, child.type)
    );
}


void O3Graph::add_edge(Vertex& parent, OutgoingEdge& edge) {
    graph[parent].emplace_back(edge);
    // // check
    // graph[edge.child];
    // each vertex must be connected in the new DEG
    auto key = hash_inst(parent.inst, parent.type);
    inst_to_vertex[key] = parent;
}


void O3Graph::add_edge(Vertex& parent, OutgoingEdge&& edge) {
    graph[parent].emplace_back(edge);
    // // check
    // graph[edge.child];
    // each vertex must be connected in the new DEG
    auto key = hash_inst(parent.inst, parent.type);
    inst_to_vertex[key] = parent;
}


void O3Graph::add_edge(Vertex& child, IngoingEdge& edge) {
    pred[child].emplace_back(edge);
}


void O3Graph::add_edge(Vertex& child, IngoingEdge&& edge) {
    pred[child].emplace_back(edge);
}


void O3Graph::register_vertex(Vertex& vertex) {
    auto key = hash_inst(vertex.inst, vertex.type);
    inst_to_vertex[key] = vertex;
}


std::string O3Graph::hash_inst(RiscvInstruction* inst, VertexType& type) {
    return boost::lexical_cast<std::string>(inst->inst_seq) + \
        '-' + name_of_vertex_type.at(type);
}


std::string O3Graph::hash_inst(RiscvInstruction* inst, VertexType&& type) {
    return boost::lexical_cast<std::string>(inst->inst_seq) + \
        '-' + name_of_vertex_type.at(type);
}


Vertex O3Graph::get_vertex_from_inst(RiscvInstruction* inst, VertexType& type) {
    Vertex ret;
    std::string key;

    try {
        ret = inst_to_vertex.at(hash_inst(inst, type));
    } catch (std::out_of_range& e) {
        try {
            if (type == VertexType::F2 || type == VertexType::F) {
                key = hash_inst(inst, VertexType::F2F);
            } else if (type == VertexType::DS || type == VertexType::I) {
                key = hash_inst(inst, VertexType::DI);
            } else if (type == VertexType::M || type == VertexType::P) {
                key = hash_inst(inst, VertexType::MP);
            } else if (type == VertexType::C) {
                key = hash_inst(inst, VertexType::C);
            }
            ret = inst_to_vertex.at(key);
        } catch (std::out_of_range& e) {
            ERROR("cannot find key = %s for inst: %s\n",
                key, inst->inst.c_str()
            );
        }
    }

    return ret;
}


Vertex O3Graph::get_vertex_from_inst(RiscvInstruction* inst, VertexType&& type) {
    /*
     * Since we merge some vertices, we need to check all potential combinations.
     * 1. F2, F, F2F
     * 2. DS, I, DI
     * 3. M, P, MP
     */

    Vertex ret;
    std::string key;

    try {
        ret = inst_to_vertex.at(hash_inst(inst, type));
    } catch (std::out_of_range& e) {
        try {
            if (type == VertexType::F2 || type == VertexType::F) {
                key = hash_inst(inst, VertexType::F2F);
            } else if (type == VertexType::DS || type == VertexType::I) {
                key = hash_inst(inst, VertexType::DI);
            } else if (type == VertexType::M || type == VertexType::P) {
                key = hash_inst(inst, VertexType::MP);
            } else if (type == VertexType::C) {
                key = hash_inst(inst, VertexType::C);
            }
            ret = inst_to_vertex.at(key);
        } catch (std::out_of_range& e) {
            ERROR("cannot find key = %s for inst: %s\n",
                key, inst->inst.c_str()
            );
        }
    }

    return ret;
}


void O3Graph::from_f1_to_f2f(
    RiscvInstruction* inst,
    Vertex& F1,
    Vertex& F2F
) {
    latency weight;

    weight = inst->process_cache_completion() - inst->fetch_cache_line();
    Bottleneck bottleneck = (weight == PipelineDelay::icache_hit_latency) ? \
        static_cast<Bottleneck>(Base()) : static_cast<Bottleneck>(IcacheMiss());
    add_edge(F1, OutgoingEdge(F2F, weight, 0, bottleneck));
    add_edge(F2F, IngoingEdge(F1));
}


void O3Graph::from_f2f_to_d(
    RiscvInstruction* inst,
    Vertex& F2F,
    Vertex& D
) {
    latency weight;

    weight = inst->decode() - inst->fetch();
    add_edge(F2F, OutgoingEdge(D, weight, 0, Base()));
    add_edge(D, IngoingEdge(F2F));
}


void O3Graph::from_f1_to_f2(
    RiscvInstruction* inst,
    Vertex& F1,
    Vertex& F2
) {
    latency weight;

    weight = inst->process_cache_completion() - inst->fetch_cache_line();
    Bottleneck bottleneck = (weight == PipelineDelay::icache_hit_latency) ? \
        static_cast<Bottleneck>(Base()) : static_cast<Bottleneck>(IcacheMiss());
    add_edge(F1, OutgoingEdge(F2, weight, 0, bottleneck));
    add_edge(F2, IngoingEdge(F1));
}


void O3Graph::from_f2_to_f(
    RiscvInstruction* inst,
    Vertex& F2,
    Vertex& F
) {
    latency weight;

    weight = inst->fetch() - inst->process_cache_completion();
    add_edge(F2, OutgoingEdge(F, weight, 0, Base()));
    add_edge(F, IngoingEdge(F2));
}


void O3Graph::from_f_to_d(
    RiscvInstruction* inst,
    Vertex& F,
    Vertex& D
) {
    latency weight;

    weight = inst->decode() - inst->fetch();
    add_edge(F, OutgoingEdge(D, weight, 0, Base()));
    add_edge(D, IngoingEdge(F));
}


void O3Graph::from_d_to_r(
    RiscvInstruction* inst,
    Vertex& D,
    Vertex& R
) {
    latency weight;

    weight = inst->rename() - inst->decode();
    add_edge(D, OutgoingEdge(R, weight, 0, Base()));
    add_edge(R, IngoingEdge(D));
}


void O3Graph::from_r_to_di(
    RiscvInstruction* inst,
    Vertex& R,
    Vertex& DI
) {
    latency weight;

    weight = inst->issue() - inst->rename();
    add_edge(R, OutgoingEdge(DI, weight, 0, Base()));
    add_edge(DI, IngoingEdge(R));
}


void O3Graph::from_r_to_ds(
    RiscvInstruction* inst,
    Vertex& R,
    Vertex& DS
) {
    latency weight;

    weight = inst->dispatch() - inst->rename();
    add_edge(R, OutgoingEdge(DS, weight, 0, Base()));
    add_edge(DS, IngoingEdge(R));
}


void O3Graph::from_ds_to_i(
    RiscvInstruction* inst,
    Vertex& DS,
    Vertex& I
) {
    latency weight;

    weight = inst->issue() - inst->dispatch();
    add_edge(DS, OutgoingEdge(I, weight, 0, Base()));
    add_edge(I, IngoingEdge(DS));
}


void O3Graph::from_di_to_mp(
    RiscvInstruction* inst,
    Vertex& DI,
    Vertex& MP
) {
    latency weight;

    weight = inst->memory() - inst->dispatch();
    add_edge(DI, OutgoingEdge(MP, weight, 0, Base()));
    add_edge(MP, IngoingEdge(DI));
}


void O3Graph::from_i_to_mp(
    RiscvInstruction* inst,
    Vertex& I,
    Vertex& MP
) {
    latency weight;

    weight = inst->complete() - inst->issue();
    add_edge(I, OutgoingEdge(MP, weight, 0, Base()));
    add_edge(MP, IngoingEdge(I));
}


void O3Graph::from_di_to_m(
    RiscvInstruction* inst,
    Vertex& DI,
    Vertex& M
) {
    latency weight;

    weight = inst->memory() - inst->issue();
    add_edge(DI, OutgoingEdge(M, weight, 0, Base()));
    add_edge(M, IngoingEdge(DI));
}


void O3Graph::from_m_to_p(
    RiscvInstruction* inst,
    Vertex& M,
    Vertex& P
) {
    latency weight;

    weight = inst->complete_memory() - inst->memory();
    Bottleneck bottleneck = (weight == PipelineDelay::dcache_int_latency) ? \
        static_cast<Bottleneck>(Base()) : static_cast<Bottleneck>(DcacheMiss());
    add_edge(M, OutgoingEdge(P, weight, 0, bottleneck));
    add_edge(P, IngoingEdge(M));
}


void O3Graph::from_i_to_m(
    RiscvInstruction* inst,
    Vertex& I,
    Vertex& M
) {
    latency weight;

    weight = inst->memory() - inst->issue();
    add_edge(I, OutgoingEdge(M, weight, 0, Base()));
    add_edge(M, IngoingEdge(I));
}


void O3Graph::from_di_to_p(
    RiscvInstruction* inst,
    Vertex& DI,
    Vertex& P
) {
    latency weight;

    weight = inst->complete() - inst->issue();
    add_edge(DI, OutgoingEdge(P, weight, 0, Base()));
    add_edge(P, IngoingEdge(DI));
}


void O3Graph::from_i_to_p(
    RiscvInstruction* inst,
    Vertex& I,
    Vertex& P
) {
    latency weight;

    weight = inst->complete() - inst->issue();
    add_edge(I, OutgoingEdge(P, weight, 0, Base()));
    add_edge(P, IngoingEdge(I));
}


void O3Graph::from_mp_to_c(
    RiscvInstruction* inst,
    Vertex& MP,
    Vertex& C
) {
    latency weight;

    weight = inst->commit() - inst->complete();
    add_edge(MP, OutgoingEdge(C, weight, 0, Base()));
    add_edge(C, IngoingEdge(MP));
    register_vertex(C);
}


void O3Graph::from_p_to_c(
    RiscvInstruction* inst,
    Vertex& P,
    Vertex& C
) {
    latency weight;

    weight = inst->commit() - inst->complete();
    add_edge(P, OutgoingEdge(C, weight, 0, Base()));
    add_edge(C, IngoingEdge(P));
    register_vertex(C);
}


void O3Graph::from_p_to_f1(Vertex& P, Vertex& F1) {
    latency weight;

    weight = F1.inst->fetch_cache_line() - P.inst->complete();
    add_edge(P, OutgoingEdge(F1, weight, weight, BPMiss()));
    add_edge(F1, IngoingEdge(P));
}

void O3Graph::from_r_to_r_rob(Vertex& _R, Vertex& R) {
    latency weight;

    weight = R.inst->rename() - _R.inst->rename();
    add_edge(_R, OutgoingEdge(R, weight, weight, ROB()));
    add_edge(R, IngoingEdge(_R));
}


void O3Graph::from_r_to_r_lq(Vertex& _R, Vertex& R) {
    latency weight;

    weight = R.inst->rename() - _R.inst->rename();
    add_edge(_R, OutgoingEdge(R, weight, weight, LQ()));
    add_edge(R, IngoingEdge(_R));
}


void O3Graph::from_r_to_r_sq(Vertex& _R, Vertex& R) {
    latency weight;

    weight = R.inst->rename() - _R.inst->rename();
    add_edge(_R, OutgoingEdge(R, weight, weight, SQ()));
    add_edge(R, IngoingEdge(_R));
}


void O3Graph::from_r_to_r_rf(Vertex& _R, Vertex& R, Bottleneck& bottleneck) {
    latency weight;

    weight = R.inst->rename() - _R.inst->rename();
    add_edge(_R, OutgoingEdge(R, weight, weight, bottleneck));
    add_edge(R, IngoingEdge(_R));
}


void O3Graph::from_r_to_r_iq(Vertex& _R, Vertex& R) {
    latency weight;

    weight = R.inst->rename() - _R.inst->rename();
    add_edge(_R, OutgoingEdge(R, weight, weight, IQ()));
    add_edge(R, IngoingEdge(_R));
}


void O3Graph::from_i_to_i_fu(Vertex& _I, Vertex& I, Bottleneck& bottleneck) {
    latency weight;

    weight = I.inst->issue() - _I.inst->issue();
    add_edge(_I, OutgoingEdge(I, weight, weight, bottleneck));
    add_edge(I, IngoingEdge(_I));
}


void O3Graph::from_i_to_i_dcache(Vertex& _I, Vertex& I) {
    latency weight;
    weight = I.inst->issue() - _I.inst->issue();
    Bottleneck bottleneck = (weight == PipelineDelay::dcache_int_latency) ? \
        static_cast<Bottleneck>(Base()) : static_cast<Bottleneck>(DcacheMiss());
    add_edge(_I, OutgoingEdge(I, weight, weight, bottleneck));
    add_edge(I, IngoingEdge(_I));
}


void O3Graph::from_i_to_i_raw(Vertex& _I, Vertex& I) {
    latency weight;

    assert(I.inst->issue() > _I.inst->issue());
    weight = I.inst->issue() - _I.inst->issue();
    add_edge(_I, OutgoingEdge(I, weight, weight, RAW()));
    add_edge(I, IngoingEdge(_I));
}


bool O3Graph::from_c_to_sink(Vertex& vertex, Vertex& sink, std::vector<Vertex>& path) {
    for (auto j = graph[vertex].begin(); j != graph[vertex].end(); j++) {
        if (j->child.inst_seq == vertex.inst_seq) {
            if (j->child.type == VertexType::C) {
                add_virtual_edge(const_cast<Vertex&>(j->child), sink);
                path.insert(path.begin(), j->child);
                path.insert(path.begin(), vertex);
                return true;
            } else {
                if (from_c_to_sink(j->child, sink, path)) {
                    path.insert(path.begin(), vertex);
                    return true;
                }
                return false;
            }
        }
    }
    return false;
}

void O3Graph::from_c_to_ds_serial(Vertex& C, Vertex& DS) {
    latency weight;

    weight = DS.inst->dispatch() - C.inst->commit();
    add_edge(C, OutgoingEdge(DS, weight, weight, Serial()));
    add_edge(DS, IngoingEdge(C));
}

void O3Graph::visualize() {
    if (view) {
        auto name = get_plain_file_name(output_file) + ".dot";

        INFO("construct dot for visualization: %s\n", name);

        sys_nanoseconds runtime = timestamp();

        Visualizer visualizer(graph, start, end);
        visualizer.dot(name);

        time_stats.visualization_runtime += (timestamp() - runtime).count();
    }
}


void O3Graph::dump_time_stats() {
    INFO(
        "runtime breakdown:\n" \
        "\tinstruction stream runtime: %ld ms.\n" \
        "\tNew DEG construction runtime: %ld ms.\n" \
        "\tInduced DEG construction runtime: %ld ms.\n" \
        "\tlongest path runtime: %ld ms.\n" \
        "\tNew DEG analysis runtime: %ld ms.\n" \
        "\tVisualization runtime: %ld ms.\n",
        time_stats.inst_stream_runtime / MACHINE_TICK,
        time_stats.new_deg_construction_runtime / MACHINE_TICK,
        time_stats.induced_deg_construction_runtime / MACHINE_TICK,
        time_stats.longest_path_runtime / MACHINE_TICK,
        time_stats.analysis_runtime / MACHINE_TICK,
        time_stats.visualization_runtime / MACHINE_TICK
    );
}


void O3Graph::clear_time_stats() {
    std::memset(&time_stats, 0, sizeof(TimeStats));
}
