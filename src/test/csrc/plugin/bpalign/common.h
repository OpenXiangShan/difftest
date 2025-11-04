#ifndef __PLUGIN_BPALIGN_COMMON_H__
#define __PLUGIN_BPALIGN_COMMON_H__

#include <cstdint>

#define BPSTAGES 3
#define FetchBlockSize 64
#define instBytes 2
#define PageOffsetWidth 12

#define FetchBlockInstNum (FetchBlockSize / instBytes)

#define BITS(value, high, low) \
    (((value) >> (low)) & ((1ULL << ((high) - (low) + 1)) - 1))

#define BIT(value, index) \
    (((value) >> (index)) & 1)

// 位连接宏
#define CONCAT4(b3, b2, b1, b0) \
    ((((b3) & 0xFF) << 24) | (((b2) & 0xFF) << 16) | (((b1) & 0xFF) << 8) | ((b0) & 0xFF))

#if defined(__GNUC__) || defined(__clang__)
constexpr int log2Ceil(uint32_t n) {
    if (n == 0) return 0;
    if (n == 1) return 0;
    
    // __builtin_clz: 计算前导0的个数
    int bits = 31 - __builtin_clz(n);
    return ((1u << bits) < n) ? bits + 1 : bits;
}
#endif

inline int getVpn(uint64_t addr) {
    return BITS(addr, 63, PageOffsetWidth);
}

inline bool isCrossPage(uint64_t addr, uint64_t target) {
    return BIT(addr, PageOffsetWidth) != BIT(target, PageOffsetWidth);
}

inline bool isCrossPageFull(uint64_t addr1, uint64_t addr2) {
    return getVpn(addr1) != getVpn(addr2);
}

inline uint64_t getPageAlignedAddr(uint64_t addr) {
    return getVpn(addr) << PageOffsetWidth;
}

template <typename T>
class RegEnable {
private:
    T Dout;
public:
    T Din;
    bool en;
    int tick() { if (en) Dout = Din; }
    T read() {return Dout;}
};

#endif