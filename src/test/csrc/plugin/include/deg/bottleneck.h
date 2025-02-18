/* Author: baichen.bai@alibaba-inc.com */


#ifndef __BOTTLENECK_H__
#define __BOTTLENECK_H__


#include "utils.h"


enum BIdx {
    /*
     * `BIdx` specifies the bottleneck
     * via accessing `bottleneck`.
     */
    BTK_Base,           // 0
    BTK_IcacheMiss,     // 1
    BTK_DcacheMiss,     // 2
    BTK_BPMiss,         // 3
    BTK_ROB,            // 4
    BTK_LQ,             // 5
    BTK_SQ,             // 6
    BTK_IntRF,          // 7
    BTK_FpRF,           // 8
    BTK_IQ,             // 9
    BTK_IntAlu,         // 10
    BTK_IntMultDiv,     // 11
    BTK_FpAlu,          // 12
    BTK_FpMultDiv,      // 13
    BTK_RdWrPort,       // 14
    BTK_RAW,            // 15
    BTK_Virtual,        // 16
    BTK_Serial          // 17
};


class Bottleneck {
    std::string name;

public:

    Bottleneck() = default;
    Bottleneck(std::string name): name(name) {}
    ~Bottleneck() = default;

public:
    std::string __repr__() const {
        return name;
    }

};


class Base : public Bottleneck
{
public:
    Base() : Bottleneck("Base") {}
    ~Base() = default;

};


class IcacheMiss : public Bottleneck
{
public:
    IcacheMiss() : Bottleneck("I-cache miss") {}
    ~IcacheMiss() = default;

};


class DcacheMiss : public Bottleneck
{
public:
    DcacheMiss() : Bottleneck("D-cache miss") {}
    ~DcacheMiss() = default;

};


class BPMiss : public Bottleneck
{
public:
    BPMiss() : Bottleneck("BP miss") {}
    ~BPMiss() = default;

};


class ROB : public Bottleneck
{
public:
    ROB() : Bottleneck("Lack ROB") {}
    ~ROB() = default;

};


class LQ : public Bottleneck
{
public:
    LQ() : Bottleneck("Lack LQ") {}
    ~LQ() = default;

};


class SQ : public Bottleneck
{
public:
    SQ() : Bottleneck("Lack SQ") {}
    ~SQ() = default;

};


class IntRF : public Bottleneck
{
public:
    IntRF() : Bottleneck("Lack INT RF") {}
    ~IntRF() = default;

};


class FpRF : public Bottleneck
{
public:
    FpRF() : Bottleneck("Lack FP RF") {}
    ~FpRF() = default;

};


class IQ : public Bottleneck
{
public:
    IQ() : Bottleneck("Lack IQ") {}
    ~IQ() = default;

};


class IntALU : public Bottleneck
{
public:
    IntALU() : Bottleneck("Lack IntAlu") {}
    ~IntALU() = default;

};


class IntMultDiv : public Bottleneck
{
public:
    IntMultDiv() : Bottleneck("Lack IntMultDiv") {}
    ~IntMultDiv() = default;

};


class FpAlu : public Bottleneck
{
public:
    FpAlu() : Bottleneck("Lack FpAlu") {}
    ~FpAlu() = default;

};


class FpMultDiv : public Bottleneck
{
public:
    FpMultDiv() : Bottleneck("Lack FpMultDiv") {}
    ~FpMultDiv() = default;

};


class RdWrPort : public Bottleneck
{
public:
    RdWrPort() : Bottleneck("Lack RdWrPort") {}
    ~RdWrPort() = default;

};


class RAW : public Bottleneck
{
public:
    RAW() : Bottleneck("Read after write dependence") {}
    ~RAW() = default;

};


class Virtual : public Bottleneck
{
public:
    Virtual() : Bottleneck("Virtual") {}
    ~Virtual() = default;

};


class Serial : public Bottleneck
{
public:
    Serial() : Bottleneck("Serial") {}
    ~Serial() = default;

};


class BottleneckReport {
public:
    BottleneckReport() {
        base = 0;
        icache_miss = 0;
        dcache_miss = 0;
        bp_miss = 0;
        lack_rob = 0;
        lack_lq = 0;
        lack_sq = 0;
        lack_int_rf = 0;
        lack_fp_rf = 0;
        lack_iq = 0;
        lack_int_alu = 0;
        lack_int_mult_div = 0;
        lack_fp_alu = 0;
        lack_fp_mult_div = 0;
        lack_rd_wr_port = 0;
        raw = 0;
        _virtual = 0;
        serial = 0;
    };
    ~BottleneckReport() = default;

public:
    void incr_base(latency delay) {
        base += delay;
    }

    void incr_icache_miss(latency delay) {
        icache_miss += delay;
    }

    void incr_dcache_miss(latency delay) {
        dcache_miss += delay;
    }

    void incr_bp_miss(latency delay) {
        bp_miss += delay;
    }

    void incr_rob(latency delay) {
        lack_rob += delay;
    }

    void incr_lq(latency delay) {
        lack_lq += delay;
    }

    void incr_sq(latency delay) {
        lack_sq += delay;
    }

    void incr_int_rf(latency delay) {
        lack_int_rf += delay;
    }

    void incr_fp_rf(latency delay) {
        lack_fp_rf += delay;
    }

    void incr_iq(latency delay) {
        lack_iq += delay;
    }

    void incr_int_alu(latency delay) {
        lack_int_alu += delay;
    }

    void incr_int_mult_div(latency delay) {
        lack_int_mult_div += delay;
    }

    void incr_fp_alu(latency delay) {
        lack_fp_alu += delay;
    }

    void incr_fp_mult_div(latency delay) {
        lack_fp_mult_div += delay;
    }

    void incr_rd_wr_port(latency delay) {
        lack_rd_wr_port += delay;
    }

    void incr_raw(latency delay) {
        raw += delay;
    }

    void incr_virtual(latency delay) {
        _virtual += delay;
    }

    void incr_serial(latency delay) {
        serial += delay;
    }

    void set_length(latency _length) {
        length = _length;
    }

    latency get_length() {
        return length;
    }

    latency get_base() {
        return base;
    }

    latency get_icache_miss() {
        return icache_miss;
    }

    latency get_dcache_miss() {
        return dcache_miss;
    }

    latency get_bp_miss() {
        return bp_miss;
    }

    latency get_lack_rob() {
        return lack_rob;
    }

    latency get_lack_lq() {
        return lack_lq;
    }

    latency get_lack_sq() {
        return lack_sq;
    }

    latency get_lack_int_rf() {
        return lack_int_rf;
    }

    latency get_lack_fp_rf() {
        return lack_fp_rf;
    }

    latency get_lack_iq() {
        return lack_iq;
    }

    latency get_lack_int_alu() {
        return lack_int_alu;
    }

    latency get_lack_int_mult_div() {
        return lack_int_mult_div;
    }

    latency get_lack_fp_alu() {
        return lack_fp_alu;
    }

    latency get_lack_fp_mult_div() {
        return lack_fp_mult_div;
    }

    latency get_lack_rd_wr_port() {
        return lack_rd_wr_port;
    }

    latency get_raw() {
        return raw;
    }

    latency get_virtual() {
        return _virtual;
    }

    latency get_serial() {
        return serial;
    }

private:
    latency length;

    latency base;
    latency icache_miss;
    latency dcache_miss;
    latency bp_miss;
    latency lack_rob;
    latency lack_lq;
    latency lack_sq;
    latency lack_int_rf;
    latency lack_fp_rf;
    latency lack_iq;
    latency lack_int_alu;
    latency lack_int_mult_div;
    latency lack_fp_alu;
    latency lack_fp_mult_div;
    latency lack_rd_wr_port;
    latency raw;
    latency _virtual;
    latency serial;
};


void register_bottleneck(std::vector<std::string>&, Bottleneck&&);
std::vector<std::string> register_bottlenecks();

#endif
