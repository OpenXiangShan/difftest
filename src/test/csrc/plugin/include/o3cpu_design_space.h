#ifndef O3CPU_DESIGN_SPACE_H
#define O3CPU_DESIGN_SPACE_H

#include <vector>
#include <map>
#include <string>
#include "deg/utils.h"

enum EMDIdx {
    FTQ = 0,
    IBUF,
    INTDQ,
    FPDQ,
    LSDQ,
    LQ,
    SQ,
    ROB,
    L2MSHRS,
    L2SETS,
    L3MSHRS,
    L3SETS,
    INTPHYREGS,
    FPPHYREGS,
    RASSIZE,

    // size
    EMD_SIZE
};

// const parameters

class O3CPUDesignSpace {
protected:
    std::map<std::string, std::vector<int>> components_params;
    std::map<std::string, std::map<int, double>> component_areas;
    void initialize_areas();

public:
    O3CPUDesignSpace();
    void initialize();
    void insert_component(const std::string& component, const std::vector<int>& params);
    std::vector<int> get_component_params(const std::string& component) const;
    std::vector<int> get_init_embedding() const;
    std::vector<int> get_embedding_from_file(const std::string& filename) const;
    void print_embedding(const std::vector<int>& embedding) const;
    void compare_embeddings(const std::vector<int>& emb1, const std::vector<int>& emb2) const;
    bool check_embedding(const std::vector<int>& embedding) const;
    const std::vector<std::string> param_names = {
        "FTQ", "IBUF", "INTDQ", "FPDQ", "LSDQ", "LQ", "SQ", "ROB", 
        "L2MSHRS", "L2SETS", "L3MSHRS", "L3SETS", "INTPHYREGS", "FPPHYREGS",
        "RASSIZE"
    };
    double get_component_area(const std::string& component, int param) const;
    double calculate_total_area(const std::vector<int>& embedding) const;
     // 获取设计空间中可能的最大面积配置
    std::vector<int> get_max_area_embedding() const;
     // 计算相对于最大面积的归一化面积
    double calculate_normalized_area_to_max(const std::vector<int>& embedding) const;
     // 获取相对于最大面积的组件归一化面积
    std::map<std::string, double> get_normalized_component_areas_to_max(
         const std::vector<int>& embedding) const;
     // 打印相对于最大面积的归一化信息
    void print_normalized_area_to_max(const std::vector<int>& embedding) const;

    void get_configs(const std::vector<int>& embedding) const;

    const int RenameWidth = 4;
    const int PredictWidth = 16;
    const int FetchWidth = 8;
    const int DecodeWidth = 4;
    const int IssueWidth = 4;
    const int CommitWidth = 6;
    const int ALUCnt = 2;
    const int MULCnt = 1;
    const int FPUCnt = 2;
    const int FTBSize = 2048;
    const int ITLBSize = 4;
    const int DTLBSize = 32;
    const int ICacheSize = 256;
    const int ICacheAssoc = 4;
    const int ICacheBlockSize = 64;
    const int ICache_response_latency = 2;
    const int ICacheMSHRs = 2;
    const int ICache_prefetch_buffer_size = 2;
    const int DCacheSize = 512;
    const int DCacheAssoc = 4;
    const int DCacheBlockSize = 64;
    const int DCache_response_latency = 2;
    const int DCacheMSHRs = 16;
    const int DCache_prefetch_buffer_size = 2;
    const int DCache_wb_buffer_size = 18;
    const int BTBSize = 1024;
    const int L2_assoc = 8;
    const int L2_banks = 4;
    const int L2_block_width = 64;
};





// class O3CPUDesignSpace : public DesignSpace, public O3CPUMacros {
//     protected:
//         // 设计空间描述
//         std::map<std::string, std::map<std::string, std::vector<int>>> descriptions;
//         // 设计列表
//         std::vector<std::string> designs;
//         // 组件列表
//         std::vector<std::string> components;
//         // 设计大小
//         std::vector<int> design_size;
//         // 累积设计大小
//         std::vector<int> acc_design_size;
    
//     public:
//         // 构造函数
//         O3CPUDesignSpace(const std::map<std::string, std::map<std::string, std::vector<int>>>& descriptions,
//                          const std::map<std::string, std::map<int, std::vector<int>>>& components_mappings,
//                          int size);
    
//         // 构造设计大小
//         std::vector<int> construct_design_size() const;
    
//         // 构造组件维度
//         std::vector<std::vector<int>> construct_component_dims() const;
    
//         // 验证索引是否有效
//         void valid(int idx) const;
    
//         // 索引转换为向量
//         std::vector<int> idx_to_vec(int idx) const;
    
//         // 向量转换为索引
//         int vec_to_idx(const std::vector<int>& vec) const;
    
//         // 索引转换为嵌入
//         std::vector<int> idx_to_embedding(int idx) const;
    
//         // 嵌入转换为索引
//         int embedding_to_idx(const std::vector<int>& embedding) const;
    
//         virtual ~O3CPUDesignSpace() = default;
//     };

#endif // O3CPU_DESIGN_SPACE_H