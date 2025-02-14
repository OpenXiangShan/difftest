// Header file: ArchExplorerEngine.h
#include "deg/utils.h"
#include "deg/bottleneck.h"
#include "deg/o3.h"
#include "o3cpu_design_space.h"

#include <algorithm>
#include <random>
#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <cassert>

class ArchExplorerEngine {
    public:
        ArchExplorerEngine() {
            init();
        }
        ~ArchExplorerEngine() {
            if (o3graph) delete o3graph;
        }
    
        void init();
        void run();
        
        std::vector<std::tuple<std::string, int>> get_bottleneck_contribution(int idx);
        std::vector<int> bottleneck_removal(std::vector<int>& embedding, std::vector<std::tuple<std::string, int>>& contribution);
        void increase_hardware_resource(std::vector<int>& embedding, std::vector<std::tuple<std::string, int>>& contribution);
        void decrease_hardware_resource(std::vector<int>& embedding, std::vector<std::tuple<std::string, int>>& contribution);
        std::vector<int> bottleneck_analysis(std::vector<int> embedding);
        O3CPUDesignSpace get_design_space() { return design_space; }
        void step(std::string line) {
            o3graph->step(line);
        }
        void finalize_deg() {
            o3graph->finalize();
        }

        
    private:
        std::map<std::string, double> calc_bottleneck_contribution(const std::map<std::string, int>& btnk);
        std::string get_simulator_root(int idx);
        bool adjust_component(const std::string& component_name,
            std::vector<int>& embedding, 
            EMDIdx idx,
            bool increase);
    
        O3Graph* o3graph = nullptr;
        O3CPUDesignSpace design_space;
        int top_k = 3;  // default
};