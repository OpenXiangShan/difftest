// See LICENSE.SiFive for license details.

#include <cstdlib>
#include "remote_bitbang.h"
#include "common.h"
#include "jtag_utils.h"

remote_bitbang_t* jtag;

jtag_dump_helper* jtag_dumper;
jtag_testcase_driver* jtag_driver;

bool dump_jtag;
bool enable_simjtag;
bool enable_jtag_testcase;

extern "C" int jtag_tick
(
 unsigned char * jtag_TCK,
 unsigned char * jtag_TMS,
 unsigned char * jtag_TDI,
 unsigned char * jtag_TRSTn,
 unsigned char jtag_TDO
)
{
  if (!enable_simjtag && !enable_jtag_testcase) return 0;
  // if (!jtag) {
  //   // TODO: Pass in real port number
  //   jtag = new remote_bitbang_t(23334);
  // }

  if (enable_simjtag) {
    jtag->tick(jtag_TCK, jtag_TMS, jtag_TDI, jtag_TRSTn, jtag_TDO);
  }

  if (enable_jtag_testcase) {
    jtag_driver->tick(jtag_TCK, jtag_TMS, jtag_TDI, jtag_TRSTn, jtag_TDO);
  }

  if (dump_jtag && *jtag_TCK == 1) {
    jtag_dumper->dump(* jtag_TMS, * jtag_TDI, jtag_TDO);
  }

  if (enable_simjtag) {
    return jtag->done() ? (jtag->exit_code() << 1 | 1) : 0;
  } else {
    return 0;
  }

}
