#include "o3cpu_design_space.h"



O3CPUDesignSpace::O3CPUDesignSpace() {
    initialize();
}

void O3CPUDesignSpace::insert_component(
    const std::string& component, 
    const std::vector<int>& params) {
    if (components_params.find(component) != components_params.end()) {
        throw std::runtime_error("Component already exists: " + component);
    }
    components_params[component] = params;
}

void O3CPUDesignSpace::initialize() {
    components_params["FTQ"] = {4, 8, 12, 16, 32, 64};
    components_params["IBUF"] = {16, 20, 24, 28, 32}; 
    components_params["INTDQ"] = {6, 8, 12};
    components_params["FPDQ"] = {6, 8, 12};
    components_params["LSDQ"] = {6, 8, 12};
    components_params["LQ"] = {4, 8, 12, 16, 24, 32, 64};
    components_params["SQ"] = {4, 8, 12, 16, 24, 32, 64};
    components_params["ROB"] = {6, 8, 16, 24, 32, 48, 64, 80, 96, 128, 256};
    components_params["L2MSHRS"] = {2, 6, 10, 14};
    components_params["L2SETS"] = {64, 128};
    components_params["L3MSHRS"] = {2, 6, 10, 14};
    components_params["L3SETS"] = {512, 1024};
    components_params["INTPHYREGS"] = {48, 64, 80, 96, 128};
    components_params["FPPHYREGS"] = {32, 48, 64, 96};
    components_params["RASSIZE"] = {8, 16, 24, 32};
    // initialize_areas();
}

void O3CPUDesignSpace::initialize_areas() {
    // 使用临时变量
    std::map<std::string, std::map<int, double>> temp_areas = {
        {"FTQ",        {{4, 4636.38},     {8, 5141.1067},   {12, 5966.6602}, 
                        {16, 8961.6896},  {32, 17325.6858}, {64, 23497.2266}}},
        {"IBUF",       {{16, 7234.2979},  {20, 8590.9631},  {24, 9409.6601},
                        {28, 10497.3411}, {32, 11114.3},    {48, 16137.4845}}},
        {"INTDQ",      {{6, 1972.8000},   {8, 2322.3936},   {12, 2994.8352},  {16, 3590.1696}}},
        {"FPDQ",       {{6, 1088.3520},   {8, 1341.0816},   {12, 1660.7424}, {16, 7046.82}}},
        {"LSDQ",       {{6, 2015.2320},   {8, 2365.0368},   {12, 3031.8528},  {16, 3675.7056}}},
        {"LQ",         {{4, 3248.7},      {8, 4392.6336},   {12, 5731.1424},  {16, 6895.7184},
                        {24, 9594.1631},  {32, 11954.6879}, {64, 22213.3054}, {128, 41827.0461},{256, 81882.78}}},
        {"SQ",         {{4, 1792.5504},   {8, 2887.2768},   {12, 4030.6368},  {16, 4744.0703},  
                        {24, 6846.6815},  {32, 8433.1967},  {64, 16433.2414}, {128, 31736.72}, {256, 62491.28}}},
        {"ROB",        {{6, 2102.83},     {8, 2259.67},     {16, 2887.03},    {24, 3248.2176},
                        {32, 3695.5200},  {48, 5369.9136},  {64, 6799.5648},  {80, 8374.7327},
                        {96, 9565.9775},  {128, 11120.5055},{256, 21707.83} }},
        {"L2SETS",     {{64, 142510.2491}, {128, 216644.8855}}},
        {"L3SETS",     {{512, 480102.9227}, {1024, 808400.8324}}},
        {"IntFreeList",{{48, 640.0364}, {64, 826.8672},   {80, 1207.0080},  {96, 1405.7088},  {128, 1856.5248}}},
        {"IntRegfile", {{48, 16387.2732}, {64, 19222.4637}, {80, 26364.8252}, {96, 31361.4908}, {128, 37565.3371}}},
        {"FpFreeList", {{32, 460.4352},   {48, 788.1024},   {64, 967.2192},   {96, 1463.6160}}},
        {"FpRegfile",  {{64, 17191.6990},  {80, 23497.3053},{96, 27900.6908}, {128, 33315.8012}}},
        {"RAS",        {{8, 1480.0320},   {16, 1678.4640},  {24, 1906.4421},  {32, 2127.0336}}},
    };
    
    // 赋值给成员变量
    component_areas = temp_areas;
}

double O3CPUDesignSpace::get_component_area(const std::string& component, int param) const {
    if (component_areas.find(component) == component_areas.end()) {
        throw std::runtime_error("Component not found: " + component);
    }
    const auto& areas = component_areas.at(component);
    if (areas.find(param) == areas.end()) {
        throw std::runtime_error("Parameter not found for component " + component + ": " + std::to_string(param));
    }
    return areas.at(param);
}

double O3CPUDesignSpace::calculate_total_area(const std::vector<int>& embedding) const {
    double base = 0.0;
    double total = 0.0;
    total = base;

    total += get_component_area("IBUF", embedding[EMDIdx::IBUF]);
    total += get_component_area("INTDQ", embedding[EMDIdx::INTDQ]);
    total += get_component_area("FPDQ", embedding[EMDIdx::FPDQ]);
    total += get_component_area("LSDQ", embedding[EMDIdx::LSDQ]);
    total += get_component_area("LQ", embedding[EMDIdx::LQ]);
    total += get_component_area("SQ", embedding[EMDIdx::SQ]);
    total += get_component_area("ROB", embedding[EMDIdx::ROB]);
    total += get_component_area("L2SETS", embedding[EMDIdx::L2SETS]);
    total += get_component_area("L3SETS", embedding[EMDIdx::L3SETS]);
    total += get_component_area("IntFreeList", embedding[EMDIdx::INTPHYREGS]);
    total += get_component_area("IntRegfile", embedding[EMDIdx::INTPHYREGS]);
    total += get_component_area("FpFreeList", embedding[EMDIdx::FPPHYREGS]);
    total += get_component_area("FpRegfile", embedding[EMDIdx::FPPHYREGS] + 32);
    total += get_component_area("RAS", embedding[EMDIdx::RASSIZE]);

    return total;
}

// 获取设计空间中可能的最大面积配置
std::vector<int> O3CPUDesignSpace::get_max_area_embedding() const {
    std::vector<int> max_area_embedding(EMDIdx::EMD_SIZE, 0);
    max_area_embedding[EMDIdx::L2MSHRS] = 14;
    max_area_embedding[EMDIdx::L3MSHRS] = 14;
    
    // 对每个组件，选择具有最大面积的参数配置
    for (size_t i = 0; i < param_names.size(); ++i) {
        const std::string& component_name = param_names[i];
        const std::vector<int>& valid_params = components_params.at(component_name);
        
        // 找出提供最大面积的参数值
        int max_area_param = valid_params[0];
        double max_area = 0.0;
        
        // 某些组件需要特殊处理
        if (component_name == "INTPHYREGS") {
            // 整数物理寄存器同时影响IntFreeList和IntRegfile
            for (int param : valid_params) {
                double area = get_component_area("IntFreeList", param) + 
                              get_component_area("IntRegfile", param);
                if (area > max_area) {
                    max_area = area;
                    max_area_param = param;
                }
            }
        } 
        else if (component_name == "FPPHYREGS") {
            // 浮点物理寄存器同时影响FpFreeList和FpRegfile
            for (int param : valid_params) {
                double area = get_component_area("FpFreeList", param) + 
                              get_component_area("FpRegfile", param + 32);
                if (area > max_area) {
                    max_area = area;
                    max_area_param = param;
                }
            }
        }
        else {
            // 其他组件直接查找最大面积配置
            std::string area_component = component_name;
            // 针对L2MSHRS和L3MSHRS特殊处理，因为它们在面积计算中没有直接使用
            if (component_name == "L2MSHRS" || component_name == "L3MSHRS") {
                continue;
            }
            
            for (int param : valid_params) {
                try {
                    double area = get_component_area(area_component, param);
                    if (area > max_area) {
                        max_area = area;
                        max_area_param = param;
                    }
                } catch (const std::exception& e) {
                    // 组件可能没有面积定义，跳过
                    continue;
                }
            }
        }
        
        max_area_embedding[i] = max_area_param;
    }
    
    // 确保配置有效
    if (!check_embedding(max_area_embedding)) {
        // 如果最大面积配置无效，回退到使用初始配置
        return get_init_embedding();
    }
    
    return max_area_embedding;
}

// 计算相对于最大面积的归一化面积
double O3CPUDesignSpace::calculate_normalized_area_to_max(const std::vector<int>& embedding) const {
    // 计算当前配置的总面积
    double current_area = calculate_total_area(embedding);
    
    // 计算最大可能面积
    std::vector<int> max_area_embedding = get_max_area_embedding();
    double max_area = calculate_total_area(max_area_embedding);
    
    // 归一化
    if (max_area <= 0.0) {
        throw std::runtime_error("Max area is zero or negative, cannot normalize");
    }
    
    return current_area / max_area;
}

// 获取归一化到最大面积的组件面积
std::map<std::string, double> O3CPUDesignSpace::get_normalized_component_areas_to_max(
    const std::vector<int>& embedding) const {
    
    std::map<std::string, double> normalized_areas;
    std::vector<int> max_area_embedding = get_max_area_embedding();
    
    // 计算最大总面积
    double max_total = calculate_total_area(max_area_embedding);
    
    // 计算各组件的归一化面积
    normalized_areas["FTQ"] = get_component_area("FTQ", embedding[EMDIdx::FTQ]) / max_total;
    normalized_areas["IBUF"] = get_component_area("IBUF", embedding[EMDIdx::IBUF]) / max_total;
    normalized_areas["INTDQ"] = get_component_area("INTDQ", embedding[EMDIdx::INTDQ]) / max_total;
    normalized_areas["FPDQ"] = get_component_area("FPDQ", embedding[EMDIdx::FPDQ]) / max_total;
    normalized_areas["LSDQ"] = get_component_area("LSDQ", embedding[EMDIdx::LSDQ]) / max_total;
    normalized_areas["LQ"] = get_component_area("LQ", embedding[EMDIdx::LQ]) / max_total;
    normalized_areas["SQ"] = get_component_area("SQ", embedding[EMDIdx::SQ]) / max_total;
    normalized_areas["ROB"] = get_component_area("ROB", embedding[EMDIdx::ROB]) / max_total;
    normalized_areas["L2SETS"] = get_component_area("L2SETS", embedding[EMDIdx::L2SETS]) / max_total;
    normalized_areas["L3SETS"] = get_component_area("L3SETS", embedding[EMDIdx::L3SETS]) / max_total;
    normalized_areas["IntFreeList"] = get_component_area("IntFreeList", embedding[EMDIdx::INTPHYREGS]) / max_total;
    normalized_areas["IntRegfile"] = get_component_area("IntRegfile", embedding[EMDIdx::INTPHYREGS]) / max_total;
    normalized_areas["FpFreeList"] = get_component_area("FpFreeList", embedding[EMDIdx::FPPHYREGS]) / max_total;
    normalized_areas["FpRegfile"] = get_component_area("FpRegfile", embedding[EMDIdx::FPPHYREGS] + 32) / max_total;
    normalized_areas["RAS"] = get_component_area("RAS", embedding[EMDIdx::RASSIZE]) / max_total;
    
    return normalized_areas;
}

// 打印归一化到最大面积的信息
void O3CPUDesignSpace::print_normalized_area_to_max(const std::vector<int>& embedding) const {
    double norm_area = calculate_normalized_area_to_max(embedding);
    std::cout << "Area normalized to max possible area: " << norm_area << std::endl;
    
    // 获取各组件的归一化面积
    std::map<std::string, double> component_areas = get_normalized_component_areas_to_max(embedding);
    
    std::cout << "Component normalized areas (to max):" << std::endl;
    for (const auto& pair : component_areas) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    
    // 打印总面积信息
    double total_area = calculate_total_area(embedding);
    std::vector<int> max_area_embedding = get_max_area_embedding();
    double max_area = calculate_total_area(max_area_embedding);
    
    std::cout << "Absolute areas:" << std::endl;
    std::cout << "  Current: " << total_area << std::endl;
    std::cout << "  Max possible: " << max_area << std::endl;
}

std::vector<int> O3CPUDesignSpace::get_component_params(
    const std::string& component) const {
    if (components_params.find(component) == components_params.end()) {
        throw std::runtime_error("Component not found: " + component);
    }
    return components_params.at(component);
}

std::vector<int> O3CPUDesignSpace::get_init_embedding() const {
    // 初始化向量大小为12(EMDIdx的大小)
    std::vector<int> embedding(EMDIdx::EMD_SIZE, 0);  
    embedding[static_cast<size_t>(EMDIdx::FTQ)] = 16;
    embedding[static_cast<size_t>(EMDIdx::IBUF)] = 20;
    embedding[static_cast<size_t>(EMDIdx::INTDQ)] = 12;
    embedding[static_cast<size_t>(EMDIdx::FPDQ)] = 12;
    embedding[static_cast<size_t>(EMDIdx::LSDQ)] = 12;
    embedding[static_cast<size_t>(EMDIdx::LQ)] = 32;
    embedding[static_cast<size_t>(EMDIdx::SQ)] = 24;
    embedding[static_cast<size_t>(EMDIdx::ROB)] = 64;
    embedding[static_cast<size_t>(EMDIdx::L2MSHRS)] = 14;
    embedding[static_cast<size_t>(EMDIdx::L2SETS)] = 64;
    embedding[static_cast<size_t>(EMDIdx::L3MSHRS)] = 14;
    embedding[static_cast<size_t>(EMDIdx::L3SETS)] = 512;
    embedding[static_cast<size_t>(EMDIdx::INTPHYREGS)] = 128;
    embedding[static_cast<size_t>(EMDIdx::FPPHYREGS)] = 96;
    embedding[static_cast<size_t>(EMDIdx::RASSIZE)] = 32;
    check_embedding(embedding);
    return embedding;
}

std::vector<int> O3CPUDesignSpace::get_embedding_from_file(const std::string& filename) const {
    // 初始化向量
    std::vector<int> embedding(EMDIdx::EMD_SIZE, 0);
    
    // 打开文件
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }


    std::string line;
    // 读取每一行
    while (std::getline(file, line)) {
        // 跳过空行和注释行
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 移除空白字符
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        
        // 查找分隔符
        size_t pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }

        // 提取参数名和值
        std::string param = line.substr(0, pos);
        std::string value_str = line.substr(pos + 1);
        
        // 查找参数索引
        auto it = std::find(param_names.begin(), param_names.end(), param);
        if (it != param_names.end()) {
            int index = std::distance(param_names.begin(), it);
            try {
                embedding[index] = std::stoi(value_str);
            } catch (const std::exception& e) {
                throw std::runtime_error("Invalid value for parameter " + param + ": " + value_str);
            }
        }
    }

    file.close();

    // 验证embedding是否有效
    if (!check_embedding(embedding)) {
        throw std::runtime_error("Invalid embedding configuration in file: " + filename);
    }

    return embedding;
}

bool O3CPUDesignSpace::check_embedding(const std::vector<int>& embedding) const {

    // 检查向量大小
    if (embedding.size() != EMDIdx::EMD_SIZE) {
        std::cout << "Invalid embedding size: " << embedding.size() 
                 << ", expected: " << EMDIdx::EMD_SIZE << std::endl;
        return false;
    }

    // 检查每个参数值是否在有效范围内 
    for (size_t i = 0; i < embedding.size(); ++i) {
        const auto& valid_params = components_params.at(param_names[i]);
        if (std::find(valid_params.begin(), valid_params.end(), embedding[i]) 
            == valid_params.end()) {
            std::cout << "Invalid value for " << param_names[i] << ": " 
                     << embedding[i] << std::endl;
            std::cout << "Valid values are: ";
            for (const auto& val : valid_params) {
                std::cout << val << " ";
            }
            std::cout << std::endl;
            return false;
        }
    }

    // 检查特定参数关系
    if (embedding[EMDIdx::ROB] <= RenameWidth) {
        std::cout << "ROB size must be greater than RenameWidth" << std::endl;
        return false;
    }
    if (embedding[EMDIdx::LQ] <= 2) {
        std::cout << "LQ size must be greater than 2" << std::endl;
        return false;
    }
    if (embedding[EMDIdx::SQ] <= 2) {
        std::cout << "SQ size must be greater than 2" << std::endl;
        return false;
    }
    if (embedding[EMDIdx::IBUF] <= PredictWidth) {
        std::cout << "IBUF size must be greater than PredictWidth" << std::endl;
        return false;
    }
    if (embedding[EMDIdx::INTDQ] <= RenameWidth) {
        std::cout << "INTDQ size must be greater than RenameWidth" << std::endl;
        return false;
    }
    if (embedding[EMDIdx::FPDQ] <= RenameWidth) {
        std::cout << "FPDQ size must be greater than RenameWidth" << std::endl;
        return false;
    }
    if (embedding[EMDIdx::LSDQ] <= RenameWidth) {
        std::cout << "LSDQ size must be greater than RenameWidth" << std::endl;
        return false;
    }
    if (embedding[EMDIdx::INTPHYREGS] < 32) {
        std::cout << "INTPHYREGS size must be greater than 32" << std::endl;
        return false;
    }
    if (embedding[EMDIdx::FPPHYREGS] < 32) {
        std::cout << "FPPHYREGS size must be greater than 32" << std::endl;
        return false;
    }

    return true;
}


void O3CPUDesignSpace::print_embedding(const std::vector<int>& embedding) const {

    std::cout << "{\n";
    for (size_t i = 0; i < embedding.size(); ++i) {
        std::cout << "  " << param_names[i] << ": " << embedding[i];
        if (i != embedding.size() - 1) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "}" << std::endl;
}

void O3CPUDesignSpace::compare_embeddings(
    const std::vector<int>& emb1, 
    const std::vector<int>& emb2) const {


    std::cout << "{\n";

    for (size_t i = 0; i < emb1.size(); ++i) {
        std::cout << param_names[i] << ": " << emb1[i] << " -> " << emb2[i] << std::endl;
    }
    std::cout << "}" << std::endl;
}


void O3CPUDesignSpace::get_configs(const std::vector<int>& embedding) const {
    std::ostringstream stats;
    stats << "FetchWidth: " << FetchWidth << "\n"
          << "DecodeWidth: " << DecodeWidth << "\n"
          << "IssueWidth: " << IssueWidth << "\n"
          << "CommitWidth: " << CommitWidth << "\n"
          << "ALUCnt: " << ALUCnt << "\n"
          << "MULCnt: " << MULCnt << "\n"
          << "FPUCnt: " << FPUCnt << "\n"
          << "FTBSize: " << FTBSize << "\n"
          << "IQSize: " << embedding[EMDIdx::INTDQ] << "\n"
          << "FPIQSize: " << embedding[EMDIdx::FPDQ] << "\n"
          << "ROBSize: " << embedding[EMDIdx::ROB] << "\n"
          << "IntPhyRegs: " << embedding[EMDIdx::INTPHYREGS] << "\n"
          << "FpPhyRegs: " << embedding[EMDIdx::FPPHYREGS] << "\n"
          << "SQSize: " << embedding[EMDIdx::SQ] << "\n"
          << "LQSize: " << embedding[EMDIdx::LQ] << "\n"
          << "RASSize: " << embedding[EMDIdx::RASSIZE] << "\n"
          << "ITLBSize: " << ITLBSize << "\n"
          << "DTLBSize: " << DTLBSize << "\n"
          << "ICacheSize: " << ICacheSize << "\n"
          << "ICacheBlockSize: " << ICacheBlockSize << "\n"
          << "ICacheAssoc: " << ICacheAssoc << "\n"
          << "ICache_response_latency: " << ICache_response_latency << "\n"
          << "ICache_MSHRs: " << ICacheMSHRs << "\n"
          << "ICache_prefetch_buffer_size: " << ICache_prefetch_buffer_size << "\n"
          << "DCacheSize: " << DCacheSize << "\n"
          << "DCacheBlockSize: " << DCacheBlockSize << "\n"
          << "DCacheAssoc: " << DCacheAssoc << "\n"
          << "DCache_response_latency: " << DCache_response_latency << "\n"
          << "DCache_MSHRs: " << DCacheMSHRs << "\n"
          << "DCache_prefetch_buffer_size: " << DCache_prefetch_buffer_size << "\n"
          << "DCache_wb_buffer_size: " << DCache_wb_buffer_size << "\n"
          << "MSHRS: " << embedding[EMDIdx::L2MSHRS] + embedding[EMDIdx::L3MSHRS] << "\n"
          << "BTBSize: " << BTBSize << "\n"
          << "L2_capacity: " << embedding[EMDIdx::L2SETS] * 8 * 4 / 16 * 1024 << "\n"
          << "L2_block_width: " << L2_block_width << "\n"
          << "L2_assoc: " << L2_assoc << "\n"
          << "L2_banks: " << L2_banks << "\n"
          << "L2_mshrs: " << embedding[EMDIdx::L2MSHRS] << "\n";
    
    std::ofstream stats_file("stats.txt", std::ios::app);
    if (stats_file.is_open()) {
        stats_file << stats.str();
        stats_file << "\n--------------------------------------------------\n";
        stats_file.close();
      } else {
        std::cerr << "Failed to open stats.txt for writing." << std::endl;
      }
}

// O3CPUDesignSpace::O3CPUDesignSpace(
//     const std::map<std::string, std::map<std::string, std::vector<int>>>& descriptions,
//     const std::map<std::string, std::map<int, std::vector<int>>>& components_mappings,
//     int size)
//     : descriptions(descriptions) {
    
//     // 初始化designs列表
//     for (const auto& design : descriptions) {
//         designs.push_back(design.first);
//     }

//     // 初始化components列表
//     if (!designs.empty()) {
//         const auto& first_design = descriptions.at(designs[0]);
//         for (const auto& component : first_design) {
//             components.push_back(component.first);
//         }
//     }

//     // 构造design_size并验证
//     design_size = construct_design_size();
//     int total_size = std::accumulate(design_size.begin(), design_size.end(), 0);
//     if (total_size != size) {
//         throw std::runtime_error("Size mismatch: " + std::to_string(total_size) + 
//                                " vs " + std::to_string(size));
//     }

//     // 计算累积大小
//     acc_design_size = design_size;
//     std::partial_sum(design_size.begin(), design_size.end(), acc_design_size.begin());

//     // 初始化基类
//     DesignSpace::initialize(size, components.size());
//     O3CPUMacros::initialize(components_mappings, construct_component_dims());
// }

// std::vector<int> O3CPUDesignSpace::construct_design_size() const {
//     std::vector<int> design_size;
//     for (const auto& design : descriptions) {
//         std::vector<int> _design_size;
//         for (const auto& component : design.second) {
//             _design_size.push_back(component.second.size());
//         }
//         int prod = 1;
//         for (int size : _design_size) {
//             prod *= size;
//         }
//         design_size.push_back(prod);
//     }
//     return design_size;
// }

// std::vector<std::vector<int>> O3CPUDesignSpace::construct_component_dims() const {
//     std::vector<std::vector<int>> component_dims;
//     for (const auto& design : descriptions) {
//         std::vector<int> _component_dims;
//         for (const auto& component : design.second) {
//             _component_dims.push_back(component.second.size());
//         }
//         component_dims.push_back(_component_dims);
//     }
//     return component_dims;
// }

// void O3CPUDesignSpace::valid(int idx) const {
//     if (idx <= 0 || idx > size) {
//         throw std::out_of_range("Invalid index: " + std::to_string(idx));
//     }
// }

// std::vector<int> O3CPUDesignSpace::idx_to_vec(int idx) const {
//     valid(idx);
//     idx--;
//     std::vector<int> vec;
    
//     // 找到对应的设计
//     auto it = std::upper_bound(acc_design_size.begin(), acc_design_size.end(), idx);
//     int design = std::distance(acc_design_size.begin(), it);
    
//     // 减去偏移量
//     if (design > 0) {
//         idx -= acc_design_size[design - 1];
//     }

//     // 构造向量
//     const auto& dims = component_dims[design];
//     for (int dim : dims) {
//         vec.push_back(idx % dim);
//         idx /= dim;
//     }

//     // 添加偏移量
//     for (size_t i = 0; i < vec.size(); i++) {
//         vec[i] = descriptions.at(designs[design])
//                            .at(components[i])
//                            [vec[i]];
//     }
//     return vec;
// }

// int O3CPUDesignSpace::vec_to_idx(const std::vector<int>& vec) const {
//     int design = 0;
//     int idx = 0;
//     std::vector<int> temp_vec = vec;

//     // 减去偏移量
//     for (size_t i = 0; i < temp_vec.size(); i++) {
//         const auto& component_values = descriptions.at(designs[design])
//                                                  .at(components[i]);
//         auto it = std::find(component_values.begin(), 
//                            component_values.end(), 
//                            temp_vec[i]);
//         if (it == component_values.end()) {
//             throw std::runtime_error("Invalid vector value");
//         }
//         temp_vec[i] = std::distance(component_values.begin(), it);
//     }

//     // 计算索引
//     for (size_t j = 0; j < temp_vec.size(); j++) {
//         int prod = 1;
//         for (size_t k = 0; k < j; k++) {
//             prod *= component_dims[design][k];
//         }
//         idx += prod * temp_vec[j];
//     }

//     // 添加偏移量
//     if (design > 0) {
//         idx += acc_design_size[design - 1];
//     }
//     idx++;

//     valid(idx);
//     return idx;
// }

// std::vector<int> O3CPUDesignSpace::idx_to_embedding(int idx) const {
//     std::vector<int> vec = idx_to_vec(idx);
//     return vec_to_embedding(vec);
// }

// int O3CPUDesignSpace::embedding_to_idx(const std::vector<int>& embedding) const {
//     std::vector<int> vec = embedding_to_vec(embedding);
//     return vec_to_idx(vec);
// }


// O3CPUDesignSpace parse_o3cpu_design_space(const std::string& design_space_csv,
//                                           const std::string& components_csv) {
//     // read csv
//     io::CSVReader<3> in(design_space_csv);

// }

