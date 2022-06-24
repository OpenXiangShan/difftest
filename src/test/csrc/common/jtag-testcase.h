#include "jtag-utils.h"

#define MAX_CMDS 1000

#define DMI_ADDR_SIZE   7
#define DMI_DATA_SIZE   32
#define DMI_OP_SIZE     2
#define DMI_SIZE        DMI_ADDR_SIZE + DMI_DATA_SIZE + DMI_OP_SIZE

#define GEN_MASK(size)            (1UL << size) - 1UL
#define GEN_DMI_READ(addr)        ((addr & GEN_MASK(DMI_ADDR_SIZE)) << (DMI_DATA_SIZE + DMI_OP_SIZE)) | \
                                  0x1
#define GEN_DMI_WRITE(addr, data) ((addr & GEN_MASK(DMI_ADDR_SIZE)) << (DMI_DATA_SIZE + DMI_OP_SIZE)) | \
                                  ((data & GEN_MASK(DMI_DATA_SIZE)) << DMI_OP_SIZE) | \
                                  0x2

#define GET_DMI_DATA(data)  (data >> DMI_OP_SIZE) & GEN_MASK(DMI_DATA_SIZE)


// some useful commands

// a "read dr" command is literally a write 0
#define CMD_JTAG_RESET                        {.cmd = JTAG_RESET, \
  .status = CMD_DONE, .size = 0, .arg = 0, .result_print = NULL, \
  .retry_func = NULL, .retry_target = 0}
#define CMD_WRITE_IR(ir_addr)                 {.cmd = JTAG_WRITE_IR, \
  .status = CMD_DONE, .size = 5, .arg = ir_addr, .result_print = NULL, \
  .retry_func = NULL, .retry_target = 0}
#define CMD_READ_DR(s, str)                   {.cmd = JTAG_DR_SCAN, \
  .status = CMD_DONE, .size = s, .arg = 0, .result_print = str, \
  .retry_func = NULL, .retry_target = 0}
#define CMD_WRITE_DR(s, content)              {.cmd = JTAG_DR_SCAN, \
  .status = CMD_DONE, .size = s, .arg = content, .result_print = NULL, \
  .retry_func = NULL, .retry_target = 0}
#define CMD_READ_DR_RETRY(s, func, target)    {.cmd = JTAG_DR_SCAN, \
  .status = CMD_DONE, .size = s, .arg = 0, .result_print = NULL, \
  .retry_func = func, .retry_target = target}
#define CMD_RUN_IDLE(cycles)                  {.cmd = JTAG_IDLE, \
  .status = CMD_DONE, .size = 0, .arg = cycles, .result_print = NULL, \
  .retry_func = NULL, .retry_target = 0}
#define CMD_FINISH                            {.cmd = JTAG_FINISH, \
  .status = CMD_DONE, .size = 0, .arg = 0, .result_print = NULL, \
  .retry_func = NULL, .retry_target = 0}

// explicit clr_req needed if no CMD_DMI_GET_RES
#define CMD_DMI_CLR_REQ                 CMD_WRITE_IR(0x11), \
                                        CMD_WRITE_DR(DMI_SIZE, 0)
#define CMD_DMI_WRITE_REQ(addr, data)   CMD_WRITE_IR(0x11), \
                                        CMD_WRITE_DR(DMI_SIZE, GEN_DMI_WRITE(addr, data))
#define CMD_DMI_READ_REQ(addr)          CMD_WRITE_IR(0x11), \
                                        CMD_WRITE_DR(DMI_SIZE, GEN_DMI_READ(addr))
#define CMD_DMI_GET_RES                 CMD_WRITE_IR(0x11), \
                                        CMD_READ_DR(DMI_SIZE, dmi_str)                                        
#define CMD_DMI_GET_RES_CLEAR_BUSY      CMD_WRITE_IR(0x11), \
                                        CMD_READ_DR_RETRY(DMI_SIZE, retry_dmi_req_no_busy, 3), \
                                        CMD_WRITE_IR(0x10), \
                                        CMD_READ_DR_RETRY(32, retry_always, -3)

#define CMD_DTMCS_WRITE(data)   CMD_WRITE_IR(0x10), CMD_WRITE_DR(32, data)
#define CMD_DTMCS_READ          CMD_WRITE_IR(0x10), CMD_READ_DR(32, dtmcs_str)

#define CMD_IDCODE_READ         CMD_WRITE_IR(0x01), CMD_READ_DR(32, NULL)


// retry functions
bool retry_not_halted(uint64_t read_data);
bool retry_dmi_error(uint64_t dtmcs);
bool retry_dmi_req_no_busy(uint64_t dmi);
bool retry_always(uint64_t dummy) {
  return true;
}
#define MAX_RETRIES 10
#define HAS_MAX_RETRY if (num_retries >= MAX_RETRIES) { printf("Exceeded maximum retries in function %s!!!", __func__); exit(2);}

// Print strings
const char *dmi_str = "DMI Result";
const char *dtmcs_str = "dtmcs result";

// Put your commands here
// all commands begin and end with state run_test_idle
jtag_cmd cmd_list[MAX_CMDS] = {
  CMD_JTAG_RESET,
  CMD_IDCODE_READ,              // read idcode. 0x1 in southlake
  CMD_DTMCS_READ,               // should be 0x5071
  CMD_DMI_WRITE_REQ(0x10, 0),   // write to dmi to request clearing dmcontrol
  CMD_DMI_CLR_REQ,              // just to check dmi failed or not
  CMD_DMI_WRITE_REQ(0x10, 0x1), // set dmactive in dmcontrol
  CMD_DMI_CLR_REQ,
  CMD_DMI_READ_REQ(0x11),       // read dmstatus
  CMD_DMI_GET_RES,              // this clears read req
  CMD_DMI_WRITE_REQ(0x10, 0x80000001),      // halt req to dmcontrol, fill hasel and hartsel bits
  CMD_DMI_CLR_REQ,
  CMD_RUN_IDLE(20),             // idle cycles should not be larger than 64
  CMD_DMI_READ_REQ(0x11),       // check dmstatus to see if halted
  CMD_DMI_GET_RES,
  CMD_DMI_WRITE_REQ(0x20, 0x30102473),      // progbuf 0 "csrr s0, misa"
  CMD_DMI_CLR_REQ,
  CMD_DMI_WRITE_REQ(0x21, 0x00100073),      // progbuf 1 "ebreak"
  CMD_DMI_CLR_REQ,
  CMD_DMI_WRITE_REQ(0x17, 0x00040000),      // postexec command
  CMD_DMI_CLR_REQ,
  CMD_DMI_WRITE_REQ(0x17, 0x00021008),      // transfer, s0 = 0x1008
  CMD_DMI_CLR_REQ,
  CMD_RUN_IDLE(40),
  CMD_DMI_READ_REQ(0x4),        // read data0
  CMD_DMI_GET_RES,
  CMD_FINISH
};