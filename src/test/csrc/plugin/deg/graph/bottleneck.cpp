/* Author: baichen.bai@alibaba-inc.com */


#include "deg/bottleneck.h"


void register_bottleneck(
    std::vector<std::string>& bottlenecks,
    Bottleneck&& bottleneck
) {
    bottlenecks.emplace_back(bottleneck.__repr__());
}


std::vector<std::string> register_bottlenecks() {
    std::vector<std::string> bottlenecks;
    register_bottleneck(bottlenecks, Base());
    register_bottleneck(bottlenecks, IcacheMiss());
    register_bottleneck(bottlenecks, DcacheMiss());
    register_bottleneck(bottlenecks, BPMiss());
    register_bottleneck(bottlenecks, ROB());
    register_bottleneck(bottlenecks, LQ());
    register_bottleneck(bottlenecks, SQ());
    register_bottleneck(bottlenecks, IntRF());
    register_bottleneck(bottlenecks, FpRF());
    register_bottleneck(bottlenecks, IQ());
    register_bottleneck(bottlenecks, IntALU());
    register_bottleneck(bottlenecks, IntMultDiv());
    register_bottleneck(bottlenecks, FpAlu());
    register_bottleneck(bottlenecks, FpMultDiv());
    register_bottleneck(bottlenecks, RdWrPort());
    register_bottleneck(bottlenecks, RAW());
    register_bottleneck(bottlenecks, Virtual());
    register_bottleneck(bottlenecks, Serial());
    return bottlenecks;
}
