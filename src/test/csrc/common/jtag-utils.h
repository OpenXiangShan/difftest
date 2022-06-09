#include "common.h"

// JTAG commands
#define JTAG_RESET 0
#define JTAG_WRITE_IR 1
#define JTAG_DR_SCAN 2
#define JTAG_IDLE 3
#define JTAG_FINISH 4

// command states
#define CMD_DONE 0
#define CMD_EXECUTING 1
#define CMD_FINISHED 2
#define CMD_FAILED -1

#define PIN_BUFFER_SIZE 1000

//============= Command structure
typedef struct {
  int cmd;
  int status = CMD_DONE;
  uint8_t size = 0;
  uint64_t arg;  // For ir scans this is ir addr, for dr scan this is reg content
  const char *result_print = NULL;
  bool (* retry_func)(uint64_t) = NULL;  // test if retry is needed. NULL if
  int retry_target = 0;        // retry target, relative to current cmd position
} jtag_cmd;

//============ PIN Stuff ==============
typedef union {
  uint8_t value;
  struct {
    unsigned char tck    : 1;
    unsigned char tms    : 1;
    unsigned char tdi    : 1;
    unsigned char trst   : 1;
    unsigned char capture: 1;
    unsigned char end    : 1;
    unsigned char zero   : 2;
  };
} jtag_pin;

typedef struct {
  jtag_pin pin_buf[PIN_BUFFER_SIZE];
  jtag_pin *pin_buf_ptr;
  uint64_t tdo_out;
  uint64_t tdi_in;
  uint8_t pos;
} pin_ctrl;