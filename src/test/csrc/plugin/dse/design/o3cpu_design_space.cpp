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
    components_params["FTQ"] = {2, 4, 8, 12, 16, 32, 64, 128};
    components_params["IBUF"] = {2, 4, 8, 12, 16, 32, 64, 128, 256}; 
    components_params["INTDQ"] = {2, 4, 8, 12};
    components_params["FPDQ"] = {2, 4, 8, 12};
    components_params["LSDQ"] = {2, 4, 8, 12};
    components_params["LQ"] = {2, 4, 8, 12, 16, 32, 64, 128, 256};
    components_params["SQ"] = {2, 4, 8, 12, 16, 32, 64, 128, 256};
    components_params["ROB"] = {2, 4, 8, 16, 32, 64, 128, 256};
    components_params["L2MSHRS"] = {1, 6, 14};
    components_params["L2SETS"] = {64, 128};
    components_params["L3MSHRS"] = {1, 6, 14};
    components_params["L3SETS"] = {512, 1024};
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
    std::vector<int> embedding(12, 0);  
    embedding[EMDIdx::FTQ] = 16;
    embedding[EMDIdx::IBUF] = 16;
    embedding[EMDIdx::INTDQ] = 12;
    embedding[EMDIdx::FPDQ] = 12;
    embedding[EMDIdx::LSDQ] = 12;
    embedding[EMDIdx::LQ] = 32;
    embedding[EMDIdx::SQ] = 24;
    embedding[EMDIdx::ROB] = 64;
    embedding[EMDIdx::L2MSHRS] = 14;
    embedding[EMDIdx::L2SETS] = 64;
    embedding[EMDIdx::L3MSHRS] = 14;
    embedding[EMDIdx::L3SETS] = 64;
    return embedding;
}


void O3CPUDesignSpace::print_embedding(const std::vector<int>& embedding) const {
    const std::vector<std::string> param_names = {
        "FTQ", "IBUF", "INTDQ", "FPDQ", "LSDQ", "LQ", "SQ", "ROB", 
        "L2MSHRS", "L2SETS", "L3MSHRS", "L3SETS"
    };

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

    const std::vector<std::string> param_names = {
        "FTQ", "IBUF", "INTDQ", "FPDQ", "LSDQ", "LQ", "SQ", "ROB", 
        "L2MSHRS", "L2SETS", "L3MSHRS", "L3SETS"
    };

    std::cout << "{\n";

    for (size_t i = 0; i < emb1.size(); ++i) {
        std::cout << param_names[i] << ": " << emb1[i] << " -> " << emb2[i] << std::endl;
    }
    std::cout << "}" << std::endl;
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

