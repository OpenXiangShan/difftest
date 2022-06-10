#include <stdio.h>
#include "common.h"
#include "jtag-testcase.h"

extern int cmd_cnt;
extern jtag_cmd *cmd_ptr;
extern pin_ctrl global_pin_buf;
extern uint64_t read_val;

void register_soft_reset(void);
void register_scan_chain(uint64_t data_in, uint8_t size);
void register_tms_seq(uint64_t sequence, uint8_t size, bool is_end);

void init_cmd(void) {
  cmd_ptr = cmd_list;

  int i;
  for (i = 0; i < MAX_CMDS && cmd_list[i].cmd != JTAG_FINISH; i++)
    ;
  if (i == MAX_CMDS) {
    printf("Did you forget to put a 'CMD_FINISH' command?\n");
    assert(0);
  }
  cmd_cnt = i + 1;
  printf("CMD cnt %d\n", cmd_cnt);
  // pin stuff init
}

int register_cmd(jtag_cmd *cmd_ptr) {
  // Check cmd_ptr_bound and pin_buf ptr
  assert(cmd_ptr >= cmd_list);
  assert(cmd_ptr <= cmd_list + cmd_cnt);
  read_val = 0;
  global_pin_buf.pos = 0;
  memset(global_pin_buf.pin_buf, 0, PIN_BUFFER_SIZE * sizeof(jtag_pin));

  global_pin_buf.pin_buf_ptr = global_pin_buf.pin_buf;

  switch(cmd_ptr->cmd){
    case JTAG_RESET:
      register_soft_reset();
      register_tms_seq(0x0, 1, true);   // test-logic-reset to run-test-idle
      break;
    case JTAG_WRITE_IR:
      register_tms_seq(0x3, 4, false);  // 0011 from r-t-i to shift ir
      register_scan_chain(cmd_ptr->arg, cmd_ptr->size);
      register_tms_seq(0x1, 2, true);   // 011 from shift-ir to idle
      break;
    case JTAG_DR_SCAN:
      register_tms_seq(0x1, 3, false);
      register_scan_chain(cmd_ptr->arg, cmd_ptr->size);
      register_tms_seq(0x1, 2, true);
      break;
    case JTAG_IDLE:
      register_tms_seq(0x0, cmd_ptr->arg, true);
      break;
    case JTAG_FINISH:
      printf("Registered finish cmd\n");
      exit(0);
      break;
    default:
      printf("NO SUCH JTAG CMD!!!\n");
      assert(0);
      break;
  }

  // Set cmd state
  cmd_ptr->status = CMD_EXECUTING;

  // reset and cleanups
  global_pin_buf.pin_buf_ptr = global_pin_buf.pin_buf;
  read_val = 0;

  return 0;
}

void register_soft_reset(void){
  int i;
  int len = 10;
  for (i = 0; i < len; i++) {
    global_pin_buf.pin_buf_ptr[i].tck = i % 2;
    global_pin_buf.pin_buf_ptr[i].tms = 1;
  }
  global_pin_buf.pin_buf_ptr += len;
}

void register_tms_seq(uint64_t sequence, uint8_t size, bool is_end) {
  int i;
  uint8_t len = size * 2;
  assert((global_pin_buf.pin_buf_ptr - global_pin_buf.pin_buf) / sizeof(jtag_pin) + len <= PIN_BUFFER_SIZE);
  for (i = 0; i < len; i++){
    global_pin_buf.pin_buf_ptr[i].tck = i % 2;
    global_pin_buf.pin_buf_ptr[i].tms = (sequence >> (i / 2)) & 0x1;
  }
  global_pin_buf.pin_buf_ptr[i-1].end = is_end;
  global_pin_buf.pin_buf_ptr += len;
}

/* About pin cmd when scan chain
 * z is tdo from previous cycle; abc... is data to write
 * Read        Write
 * 000000      000a00       <---- In read cmd tdi is always 0
 * 010001      000a01       
 * 000000      000b00       
 * 010001      000b01
 *  ...         ...
 * 010000      000n10       <---- Last cycle, tms is 1 to transfer to exit-1 state
 * 110001      100n11       <---- All data is written. end bit is asserted
 */
void register_scan_chain(uint64_t data_in, uint8_t size) {
  int i;
  int len = 2 * size;
  assert((global_pin_buf.pin_buf_ptr - global_pin_buf.pin_buf) / sizeof(jtag_pin) + len <= PIN_BUFFER_SIZE);
  for (i = 0; i < len; i++){
    global_pin_buf.pin_buf_ptr[i].tck = i % 2;
    global_pin_buf.pin_buf_ptr[i].tms = 0;
    global_pin_buf.pin_buf_ptr[i].tdi = data_in >> (i / 2);
    if (i % 2) {
      global_pin_buf.pin_buf_ptr[i].capture = 1;
    }
  }
  global_pin_buf.pin_buf_ptr[i-2].tms = 1;
  global_pin_buf.pin_buf_ptr[i-1].tms = 1;  // last bit of tms is 1 to switch 
  read_val = 0;
  global_pin_buf.pin_buf_ptr += len;
}

// ========== Some retry functions ===============
bool retry_not_halted(uint64_t read_data) {
  static int num_retries = 0; 
  printf("%s: dm control is %lx\n", __func__, GET_DMI_DATA(read_data));
  if (!(GET_DMI_DATA(read_data) & 0x300)){
    printf("%s: hart has not halted\n", __func__);
    num_retries ++;
    HAS_MAX_RETRY
    return true;
  } else {
    num_retries = 0;
    return false;
  }
}

bool retry_dmi_error(uint64_t dtmcs) {
  printf("%s: dtmcs is %lx\n", __func__, dtmcs);
  if (dtmcs & 0x400) {  // dmistat no error
    printf("%s: dmi error!!\n", __func__);
    return true;
  }
  return false;
}

bool retry_dmi_req_no_busy(uint64_t dmi) {
  return ((dmi & 0x3) == 0x3);
}
