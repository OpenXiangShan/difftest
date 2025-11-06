#ifndef __PLUGIN_BPALIGN_COMMON_H__
#define __PLUGIN_BPALIGN_COMMON_H__

#include "BpAlignIO.h"
#include <cstdint>

#define BPSTAGES 3
#define FetchBlockSize 64
#define instBytes 2
#define PageOffsetWidth 12

#define FetchBlockInstNum (FetchBlockSize / instBytes)
#define FetchBlockSizeWidth (log2Ceil(FetchBlockSize))
#define FetchBlockAlignSize (FetchBlockSize / 2)
#define FetchBlockAlignWidth (log2Ceil(FetchBlockAlignSize))
#define FetchBlockAlignInstNum (FetchBlockAlignSize / instBytes)
#define instOffsetBits (log2Ceil(instBytes))
#define CfiPositionWidth (log2Ceil(FetchBlockInstNum))

#define BITS(value, high, low) \
    (((value) >> (low)) & ((1ULL << ((high) - (low) + 1)) - 1))

#define BIT(value, index) \
    (((value) >> (index)) & 1)

#define CONCAT4(b3, b2, b1, b0) \
    ((((b3) & 0xFF) << 24) | (((b2) & 0xFF) << 16) | (((b1) & 0xFF) << 8) | ((b0) & 0xFF))

#if defined(__GNUC__) || defined(__clang__)
constexpr int log2Ceil(uint32_t n) {
    if (n == 0) return 0;
    if (n == 1) return 0;
    
    int bits = 31 - __builtin_clz(n);
    return ((1u << bits) < n) ? bits + 1 : bits;
}
#endif

inline uint64_t getAlignedAddr(uint64_t addr) {
    return addr & ~((1ULL << FetchBlockAlignWidth) - 1);
}

inline uint64_t getNextAlignedAddr(uint64_t addr) {
    return getAlignedAddr(addr) + (1ULL << FetchBlockAlignWidth);
}

inline int getAlignedInstOffset(uint64_t addr) {
    return BITS(addr, FetchBlockAlignWidth - 1, instOffsetBits);
}

inline int getAlignedPosition(uint64_t addr, int ftqOffset) {
    int fullPosition = ftqOffset + getAlignedInstOffset(addr);
    return BITS(fullPosition, CfiPositionWidth - 1, 0);
}

inline int getFtqOffset(uint64_t addr, int position) {
    return position - getAlignedInstOffset(addr);
}

inline uint64_t getVpn(uint64_t addr) {
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

inline uint64_t getFullAddr(PrunedAddr_t addr) {
    return addr.addr << instOffsetBits;
}

inline PrunedAddr_t getPrunedAddr(uint64_t addr) {
    PrunedAddr_t ret;
    ret.addr = addr >> instOffsetBits;
    return ret;
}

template <typename T>
class RegEnable {
private:
    T Dout;
public:
    T Din;
    T resetVector;
    bool en;
    int tick(bool reset) { 
        if (reset) Dout = resetVector;
        else if (en) Dout = Din; 
        return 0;
    }
    T read() {return Dout;}
};



#endif