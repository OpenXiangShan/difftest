// Source file: ArchExplorerEngine.cpp
#include "ArchExplorerEngine.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

bool ArchExplorerEngine::adjust_component(const std::string& component_name,
                                        std::vector<int>& embedding, 
                                        EMDIdx idx,
                                        bool increase) {
    bool adjust = false;
    int value = embedding[idx];
    std::vector<int> params = design_space.get_component_params(component_name);
    int tot_size = params.size();

    if (increase) {
        auto it = std::find(params.begin(), params.end(), value);
        if (it != params.end()) {
            int param_idx = std::distance(params.begin(), it);
            if (param_idx < tot_size - 1) {
                embedding[idx] = params[param_idx + 1];
                adjust = true;
            }
        }
    } else {
        auto it = std::find(params.begin(), params.end(), value);
        if (it != params.end()) {
            int param_idx = std::distance(params.begin(), it);
            if (param_idx > 0) {
                embedding[idx] = params[param_idx - 1];
                adjust = true;
            }
        }
    }
    return adjust;
}

void ArchExplorerEngine::decrease_hardware_resource(std::vector<int>& embedding, std::vector<std::tuple<std::string, int>>& contribution) {
    std::vector<std::string> candidates;
    for (const auto& [btnk_name, contrib] : contribution) {
        if (contrib == 0) {
            candidates.push_back(btnk_name);
        }
    }


    // 随机抽样：如果候选数超过 top_k，则抽样 size 为 top_k，否则全部抽样
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(candidates.begin(), candidates.end(), gen);
    // size_t sample_num = std::min(static_cast<size_t>(top_k), candidates.size());
    // candidates.resize(sample_num);

    int adjust_num = 0;
    for (const auto& btnk_name : candidates) {
        bool adjust = false;

        if (adjust_num >= top_k) {
            break;
        }

        if (btnk_name == "Base") {
            // Base瓶颈不需要调整
            continue;
        }
        else if (btnk_name == "Lack ROB") {
            adjust = adjust_component("ROB", embedding, EMDIdx::ROB, false);
        }
        else if (btnk_name == "I-cache miss") {
            adjust = adjust_component("FTQ", embedding, EMDIdx::FTQ, false);
            adjust = adjust_component("L2SETS", embedding, EMDIdx::L2SETS, false);
        }
        else if (btnk_name == "D-cache miss") {
            adjust = adjust_component("L2SETS", embedding, EMDIdx::L2SETS, false);
        }
        else if (btnk_name == "BP miss") {
            adjust = adjust_component("RASSIZE", embedding, EMDIdx::RASSIZE, false);
        }
        else if (btnk_name == "Lack LQ") {
            adjust = adjust_component("LQ", embedding, EMDIdx::LQ, false);
        }
        else if (btnk_name == "Lack SQ") {
            adjust = adjust_component("SQ", embedding, EMDIdx::SQ, false);
        }
        else if (btnk_name == "Lack INT RF") {
            adjust = adjust_component("INTPHYREGS", embedding, EMDIdx::INTPHYREGS, false);
        }
        else if (btnk_name == "Lack FP RF") {
            adjust = adjust_component("FPPHYREGS", embedding, EMDIdx::FPPHYREGS, false);
        }
        else if (btnk_name == "Lack IQ") {
            adjust = adjust_component("IBUF", embedding, EMDIdx::IBUF, false);
        }
        else if (btnk_name == "Lack IntAlu") {
            adjust = false;
            INFO("We do not support IntAlu adjustment\n");
        }
        else if (btnk_name == "Lack IntMultDiv") {
            adjust = false;
            INFO("We do not support IntMultDiv adjustment\n");
        }
        else if (btnk_name == "Lack FpAlu") {
            adjust = false;
            INFO("We do not support FpAlu adjustment\n");
        }
        else if (btnk_name == "Lack FpMultDiv") {
            adjust = false;
            INFO("We do not support FpMultDiv adjustment\n");
        }
        else if (btnk_name == "Lack RdWrPort") {
            adjust = false;
            INFO("We do not support RdWrPort adjustment\n");
        }
        else if (btnk_name == "Read after write dependence") {
            continue;
        }
        else if (btnk_name == "Virtual") {
            continue;
        }
        else if (btnk_name == "Serial") {
            continue;
        }
        else {
            ERROR("Unknown bottleneck: %s\n", btnk_name.c_str());
        }
        if (adjust) {
            adjust_num++;
        }
    }
}

void ArchExplorerEngine::increase_hardware_resource(std::vector<int>& embedding, std::vector<std::tuple<std::string, int>>& contribution) {
    int num_of_adjust = 0;
    for (const auto& [btnk_name, contrib] : contribution) {
        bool adjust = false;

        if (num_of_adjust >= top_k || contrib == 0) {
            break;
        }
        
        if (btnk_name == "Base") {
            // Base瓶颈不需要调整
            continue;
        }
        else if (btnk_name == "Lack ROB") {
            adjust = adjust_component("ROB", embedding, EMDIdx::ROB, true);
        }
        else if (btnk_name == "I-cache miss") {
            adjust = adjust_component("FTQ", embedding, EMDIdx::FTQ, true);
            adjust = adjust_component("L2SETS", embedding, EMDIdx::L2SETS, true);
        }
        else if (btnk_name == "D-cache miss") {
            adjust = adjust_component("L2SETS", embedding, EMDIdx::L2SETS, true);
        }
        else if (btnk_name == "BP miss") {
            adjust = adjust_component("RASSIZE", embedding, EMDIdx::RASSIZE, true);
        }
        else if (btnk_name == "Lack LQ") {
            adjust = adjust_component("LQ", embedding, EMDIdx::LQ, true);
        }
        else if (btnk_name == "Lack SQ") {
            adjust = adjust_component("SQ", embedding, EMDIdx::SQ, true);
        }
        else if (btnk_name == "Lack INT RF") {
            adjust = adjust_component("INTPHYREGS", embedding, EMDIdx::INTPHYREGS, true);
        }
        else if (btnk_name == "Lack FP RF") {
            adjust = adjust_component("FPPHYREGS", embedding, EMDIdx::FPPHYREGS, true);
        }
        else if (btnk_name == "Lack IQ") {
            adjust = adjust_component("IBUF", embedding, EMDIdx::IBUF, true);
        }
        else if (btnk_name == "Lack IntAlu") {
            adjust = false;
            INFO("We do not support IntAlu adjustment\n");
        }
        else if (btnk_name == "Lack IntMultDiv") {
            adjust = false;
            INFO("We do not support IntMultDiv adjustment\n");
        }
        else if (btnk_name == "Lack FpAlu") {
            adjust = false;
            INFO("We do not support FpAlu adjustment\n");
        }
        else if (btnk_name == "Lack FpMultDiv") {
            adjust = false;
            INFO("We do not support FpMultDiv adjustment\n");
        }
        else if (btnk_name == "Lack RdWrPort") {
            adjust = false;
            INFO("We do not support RdWrPort adjustment\n");
        }
        else if (btnk_name == "Read after write dependence") {
            // it is due to the benchmark parallelism
            adjust = false;
        }
        else if (btnk_name == "Virtual") {
            adjust = false;
        }
        else if (btnk_name == "Serial") {
            adjust = false;
        }
        else {
            ERROR("Unknown bottleneck: %s\n", btnk_name);
        }
        if (adjust) {
            num_of_adjust++;
        }
    }
}


std::vector<int> ArchExplorerEngine::bottleneck_removal(std::vector<int>& embedding, std::vector<std::tuple<std::string, int>>& contribution) {
    increase_hardware_resource(embedding, contribution);
    decrease_hardware_resource(embedding, contribution);
    return embedding;
}

std::vector<std::tuple<std::string, int>> ArchExplorerEngine::get_bottleneck_contribution(int idx, std::string btnk_rpt) {
    std::string simulator_root = ".";
    std::map<std::string, int> btnk;
    
    if (if_file_exist(btnk_rpt)) {
        std::ifstream fin(btnk_rpt);
        std::vector<std::string> cnt;
        std::string line;
        
        while(std::getline(fin, line)) {
            cnt.push_back(line);
        }

        if(cnt.empty()) {
            INFO("Empty file: %s\n", btnk_rpt);
            assert(false);
        }

        size_t pos = cnt[0].find_last_of(":");
        int length = std::stoi(cnt[0].substr(pos + 1));

        const auto& bottlenecks = O3Graph::get_bottlenecks();
        for(const auto& b : bottlenecks) {
            std::cout << "  - " << b << std::endl;
        }
        
        std::vector<std::string> bottleneck_lines(
            cnt.end() - bottlenecks.size(),
            cnt.end()
        );

        for(const auto& line : bottleneck_lines) {
            pos = line.find(":");
            if(pos == std::string::npos) continue;
            
            std::string btnk_name = line.substr(0, pos);
            int contrib = std::stoi(line.substr(pos + 1));
            btnk[btnk_name] = contrib;
        }

        btnk["length"] = length;
    } else {
        INFO("File not found: %s\n", btnk_rpt);
        assert(false);
    }
    
    auto contribution = calc_bottleneck_contribution(btnk);
    
    std::vector<std::tuple<std::string, int>> summary;
    summary.reserve(contribution.size());
    
    for (const auto& [btnk_name, contrib] : contribution) {
        summary.emplace_back(btnk_name, static_cast<int>(contrib));
    }
    
    std::sort(summary.begin(), summary.end(),
              [](const auto& a, const auto& b) {
                  return std::get<1>(a) > std::get<1>(b);
              });

    std::cout << "\nFinal contribution values:" << std::endl;
    for (const auto& [name, value] : summary) {
        std::cout << "  " << name << ": " << value << "%" << std::endl;
    }
              
    return summary;
}

std::map<std::string, double> ArchExplorerEngine::calc_bottleneck_contribution(
    const std::map<std::string, int>& btnk)
{
    std::cout << "\n=== Debug calc_bottleneck_contribution ===" << std::endl;
    
    // Initialize contribution map
    std::map<std::string, double> contribution;
    const auto& bottlenecks = O3Graph::get_bottlenecks();
    
    std::cout << "Input btnk map contents:" << std::endl;
    for (const auto& [name, value] : btnk) {
        std::cout << "  " << name << ": " << value << std::endl;
    }
    
    for (const auto& name : bottlenecks) {
        contribution[name] = 0.0;
    }

    // Calculate contribution for each bottleneck
    int length = btnk.at("length");
    std::cout << "\nLength value: " << length << std::endl;
    
    for (const auto& [btnk_name, contrib] : btnk) {
        if (btnk_name == "length") {
            std::cout << "Skipping length entry" << std::endl;
            continue;
        }
        
        // Calculate normalized contribution
        double normalized = static_cast<double>(contrib) / length;
        std::cout << "Processing " << btnk_name << ": " 
                  << contrib << " / " << length << " = " << normalized << std::endl;
        contribution[btnk_name] = normalized * 100; // 转换为百分比
    }
    
    
    return contribution;
}

std::vector<int> ArchExplorerEngine::bottleneck_analysis(std::vector<int> embedding, std::string btnk_rpt) {
    auto contribution = get_bottleneck_contribution(0, btnk_rpt);
    return bottleneck_removal(embedding, contribution);
    // return embedding;
}

void ArchExplorerEngine::init(std::string output_file) {
    // 创建O3Graph实例
    auto args = deg_parse_args(0, nullptr);
    args["output"] = output_file;
    if (visualize) {
        args["view"] = "1";
    }
    this->o3graph = new O3Graph(
      args, new RiscvInstructionStream("")
    );
}