/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __AXI4_H
#define __AXI4_H

#include <stdint.h>

// #define DEBUG_LOG_AXI4

// 4*64 bits
#define AXI_DATA_WIDTH_64 4

typedef uint64_t axi_addr_t;
typedef uint64_t axi_data_t[AXI_DATA_WIDTH_64];
#define axi_copy_data(dest, src) \
  memcpy(dest, src, sizeof(uint64_t)*AXI_DATA_WIDTH_64);

struct axi_aw_channel {
  uint8_t       ready;
  uint8_t       valid;
  axi_addr_t    addr;
  uint8_t       prot;
  uint8_t       id;
  uint8_t       user;
  uint8_t       len;
  uint8_t       size;
  uint8_t       burst;
  uint8_t       lock;
  uint8_t       cache;
  uint8_t       qos;
};

struct axi_w_channel {
  uint8_t       ready;
  uint8_t       valid;
  axi_data_t    data;
  uint8_t       strb;
  uint8_t       last;
};

struct axi_b_channel {
  uint8_t       ready;
  uint8_t       valid;
  uint8_t       resp;
  uint8_t       id;
  uint8_t       user;
};

struct axi_ar_channel {
  uint8_t       ready;
  uint8_t       valid;
  axi_addr_t    addr;
  uint8_t       prot;
  uint8_t       id;
  uint8_t       user;
  uint8_t       len;
  uint8_t       size;
  uint8_t       burst;
  uint8_t       lock;
  uint8_t       cache;
  uint8_t       qos;
};

struct axi_r_channel {
  uint8_t       ready;
  uint8_t       valid;
  uint8_t       resp;
  axi_data_t    data;
  uint8_t       last;
  uint8_t       id;
  uint8_t       user;
};

struct axi_channel {
  struct axi_aw_channel aw;
  struct axi_w_channel  w;
  struct axi_b_channel  b;
  struct axi_ar_channel ar;
  struct axi_r_channel  r;
};

// dut helper for AXI

// NOTE: change this when migrating between different hardware designs
#define DUT_AXI(channel, field) io_memAXI_0_##channel##field

#define axi_aw_copy_from_dut_ptr(dut_ptr, _aw)            \
  do {                                                    \
    _aw.ready = dut_ptr->DUT_AXI(aw, ready);              \
    _aw.valid = dut_ptr->DUT_AXI(aw, valid);              \
    _aw.addr = dut_ptr->DUT_AXI(aw, addr);                \
    _aw.prot = dut_ptr->DUT_AXI(aw, prot);                \
    _aw.id = dut_ptr->DUT_AXI(aw, id);                    \
    _aw.len = dut_ptr->DUT_AXI(aw, len);                  \
    _aw.size = dut_ptr->DUT_AXI(aw, size);                \
    _aw.burst = dut_ptr->DUT_AXI(aw, burst);              \
    _aw.lock = dut_ptr->DUT_AXI(aw, lock);                \
    _aw.cache = dut_ptr->DUT_AXI(aw, cache);              \
    _aw.qos = dut_ptr->DUT_AXI(aw, qos);                  \
  } while (0);

#define axi_aw_set_dut_ptr(dut_ptr, _aw)                  \
  do {                                                    \
    dut_ptr->DUT_AXI(aw, ready) = _aw.ready;              \
  } while (0);

#define axi_w_copy_from_dut_ptr(dut_ptr, _w)              \
  do {                                                    \
    _w.ready = dut_ptr->DUT_AXI(w, ready);                \
    _w.valid = dut_ptr->DUT_AXI(w, valid);                \
    axi_copy_data(_w.data, dut_ptr->DUT_AXI(w, data))     \
    _w.strb = dut_ptr->DUT_AXI(w, strb);                  \
    _w.last = dut_ptr->DUT_AXI(w, last);                  \
  } while (0);

#define axi_w_set_dut_ptr(dut_ptr, _w)                    \
  do {                                                    \
    dut_ptr->DUT_AXI(w, ready) = _w.ready;                \
  } while (0);

#define axi_b_copy_from_dut_ptr(dut_ptr, _b)              \
  do {                                                    \
    _b.ready = dut_ptr->DUT_AXI(b, ready);                \
    _b.valid = dut_ptr->DUT_AXI(b, valid);                \
    _b.resp = dut_ptr->DUT_AXI(b, resp);                  \
    _b.id = dut_ptr->DUT_AXI(b, id);                      \
  } while (0);

#define axi_b_set_dut_ptr(dut_ptr, _b)                    \
  do {                                                    \
    dut_ptr->DUT_AXI(b, valid) = _b.valid;                \
    dut_ptr->DUT_AXI(b, resp) = _b.resp;                  \
    dut_ptr->DUT_AXI(b, id) = _b.id;                      \
  } while (0);

#define axi_ar_copy_from_dut_ptr(dut_ptr, _ar)            \
  do {                                                    \
    _ar.ready = dut_ptr->DUT_AXI(ar, ready);              \
    _ar.valid = dut_ptr->DUT_AXI(ar, valid);              \
    _ar.addr = dut_ptr->DUT_AXI(ar, addr);                \
    _ar.prot = dut_ptr->DUT_AXI(ar, prot);                \
    _ar.id = dut_ptr->DUT_AXI(ar, id);                    \
    _ar.len = dut_ptr->DUT_AXI(ar, len);                  \
    _ar.size = dut_ptr->DUT_AXI(ar, size);                \
    _ar.burst = dut_ptr->DUT_AXI(ar, burst);              \
    _ar.lock = dut_ptr->DUT_AXI(ar, lock);                \
    _ar.cache = dut_ptr->DUT_AXI(ar, cache);              \
    _ar.qos = dut_ptr->DUT_AXI(ar, qos);                  \
  } while (0);

#define axi_ar_set_dut_ptr(dut_ptr, _ar)                  \
  do {                                                    \
    dut_ptr->DUT_AXI(ar, ready) = _ar.ready;              \
  } while (0);

#define axi_r_copy_from_dut_ptr(dut_ptr, _r)              \
  do {                                                    \
    _r.ready = dut_ptr->DUT_AXI(r, ready);                \
    _r.valid = dut_ptr->DUT_AXI(r, valid);                \
    _r.resp = dut_ptr->DUT_AXI(r, resp);                  \
    axi_copy_data(_r.data, dut_ptr->DUT_AXI(r, data))     \
    _r.last = dut_ptr->DUT_AXI(r, last);                  \
    _r.id = dut_ptr->DUT_AXI(r, id);                      \
  } while (0);

#define axi_r_set_dut_ptr(dut_ptr, _r)                    \
  do {                                                    \
    dut_ptr->DUT_AXI(r, valid) = _r.valid;                \
    dut_ptr->DUT_AXI(r, resp) = _r.resp;                  \
    axi_copy_data(dut_ptr->DUT_AXI(r, data), _r.data)     \
    dut_ptr->DUT_AXI(r, last) = _r.last;                  \
    dut_ptr->DUT_AXI(r, id) = _r.id;                      \
  } while (0);

#define axi_copy_from_dut_ptr(dut_ptr, axi)               \
  do {                                                    \
    axi_aw_copy_from_dut_ptr(dut_ptr, axi.aw)             \
    axi_w_copy_from_dut_ptr(dut_ptr, axi.w)               \
    axi_b_copy_from_dut_ptr(dut_ptr, axi.b)               \
    axi_ar_copy_from_dut_ptr(dut_ptr, axi.ar)             \
    axi_r_copy_from_dut_ptr(dut_ptr, axi.r)               \
  } while (0);

#define axi_set_dut_ptr(dut_ptr, axi)                     \
  do {                                                    \
    axi_aw_set_dut_ptr(dut_ptr, axi.aw)                   \
    axi_w_set_dut_ptr(dut_ptr, axi.w)                     \
    axi_b_set_dut_ptr(dut_ptr, axi.b)                     \
    axi_ar_set_dut_ptr(dut_ptr, axi.ar)                   \
    axi_r_set_dut_ptr(dut_ptr, axi.r)                     \
  } while (0);

// ar channel: (1) read raddr; (2) try to accept the address; (3) check raddr fire
bool axi_get_raddr(const axi_channel &axi, axi_addr_t &addr);
void axi_accept_raddr(axi_channel &axi);
bool axi_check_raddr_fire(const axi_channel &axi);

// r channel: (1) put rdata; (2) check rdata fire
void axi_put_rdata(axi_channel &axi, void *src, size_t n, bool last, uint8_t id);
bool axi_check_rdata_fire(const axi_channel &axi);

// aw channel: (1) read waddr; (2) try to accept the address; (3) check waddr fire
bool axi_get_waddr(const axi_channel &axi, axi_addr_t &addr);
void axi_accept_waddr(axi_channel &axi);
bool axi_check_waddr_fire(const axi_channel &axi);

// w channel: (1) accept wdata; (2) get wdata; (3) check wdata fire
void axi_accept_wdata(axi_channel &axi);
void axi_get_wdata(const axi_channel &axi, void *dest, const void *src, size_t n);
bool axi_check_wdata_fire(const axi_channel &axi);

// b channel: (1) put response; (2) check response fire
void axi_put_wack(axi_channel &axi, uint8_t id);
bool axi_check_wack_fire(const axi_channel &axi);

#endif
