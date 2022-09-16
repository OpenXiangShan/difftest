// See LICENSE.SiFive for license details.

#include <cstdlib>
#include "remote_bitbang.h"
#include "common.h"
#include "jtag-utils.h"

remote_bitbang_t* jtag;
bool enable_simjtag;
bool enable_jtag_testcase;
pin_ctrl global_pin_buf;
uint64_t read_val;
jtag_cmd *cmd_ptr;  // points to the current cmd
int cmd_cnt;

int jtag_pin_tick( unsigned char *tck_pin,
  unsigned char *tms_pin,
  unsigned char *tdi_pin,
  unsigned char *trstn_pin,
  unsigned char tdo_pin
);

int register_cmd(jtag_cmd *cmd_ptr);

extern "C" int jtag_tick
(
 unsigned char * jtag_TCK,
 unsigned char * jtag_TMS,
 unsigned char * jtag_TDI,
 unsigned char * jtag_TRSTn,
 unsigned char jtag_TDO
)
{
  if (!enable_simjtag && !enable_jtag_testcase) {
    *jtag_TCK = 0;
    return 0;
  }
  if (!jtag && enable_simjtag) {
    // TODO: Pass in real port number
    jtag = new remote_bitbang_t(23334);
  }
  if (enable_simjtag){
    jtag->tick(jtag_TCK, jtag_TMS, jtag_TDI, jtag_TRSTn, jtag_TDO);
    return jtag->done() ? (jtag->exit_code() << 1 | 1) : 0;
  }

  if (enable_jtag_testcase) {
    switch (cmd_ptr->status) {
    case CMD_DONE:
      register_cmd(cmd_ptr);
      break;
    case CMD_EXECUTING:
      jtag_pin_tick(jtag_TCK, jtag_TMS, jtag_TDI, jtag_TRSTn, jtag_TDO);
      break;
    case CMD_FINISHED:
      printf("JTAG testcase commands finished!\n");
      break;
    default:
      printf("%s: cmd failed due to unknown reason?!\n", __func__);
      assert(0);
      break;
    }

  return 1;
  }
  return 1;
}

int jtag_pin_tick( unsigned char *tck_pin,
  unsigned char *tms_pin,
  unsigned char *tdi_pin,
  unsigned char *trstn_pin,
  unsigned char tdo_pin
) {
  *tck_pin = global_pin_buf.pin_buf_ptr->tck;
  *tms_pin = global_pin_buf.pin_buf_ptr->tms;
  *tdi_pin = global_pin_buf.pin_buf_ptr->tdi;
  *trstn_pin = global_pin_buf.pin_buf_ptr->trst;
  if (global_pin_buf.pin_buf_ptr->capture) {
      // read_val is written from high to low
      read_val |= ((uint64_t) tdo_pin & 0x1) << global_pin_buf.pos;
      global_pin_buf.pos ++;
  }

  if (global_pin_buf.pin_buf_ptr->end) {
    if (cmd_ptr->cmd == JTAG_DR_SCAN) {
      if (cmd_ptr->result_print) {
        printf("%s read value %lx\n", cmd_ptr->result_print, read_val);
      }
    }

    assert(cmd_ptr->result_print || !cmd_ptr->retry_func);
    if (cmd_ptr->retry_func && cmd_ptr->retry_func(read_val)){
      cmd_ptr->status = CMD_DONE; // Maybe add a retry status bit later
      cmd_ptr = cmd_ptr + cmd_ptr->retry_target;
    } else {
      cmd_ptr->status = CMD_DONE;
      cmd_ptr ++;
    }    
    // cleanup pin buffer

  } else {
    global_pin_buf.pin_buf_ptr ++;
  }
  return 0;
}