/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* DiffTest is licensed under Mulan PSL v2.
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

#include "uart16550.h"
#include "common.h"
#include "stdlib.h"

// All constants are now defined in uart16550.h

// RX FIFO for incoming data (simulation input)
static char rx_fifo[RX_FIFO_SIZE] = {};
static int rx_f = 0, rx_r = 0;

// 16550 Register state
static uint8_t IER = 0;   // Interrupt Enable Register
static uint8_t IIR = 1;   // Interrupt Identification Register (bit 0 = 1: no interrupt)
static uint8_t FCR = 0;   // FIFO Control Register
static uint8_t LCR = 0;   // Line Control Register
static uint8_t MCR = 0;   // Modem Control Register
static uint8_t MSR = 0;   // Modem Status Register
static uint8_t SCR = 0;   // Scratch Register

// RX FIFO management
static void uart16550_rx_enqueue(char ch) {
  int next = (rx_r + 1) % RX_FIFO_SIZE;
  if (next != rx_f) {
    // not full
    rx_fifo[rx_r] = ch;
    rx_r = next;
  }
}

// RX FIFO dequeue (same as original uart.cpp)
static int uart16550_rx_dequeue(void) {
  int k = 0;
  if (rx_f != rx_r) {
    k = rx_fifo[rx_f];
    rx_f = (rx_f + 1) % RX_FIFO_SIZE;
  } else {
    static int last = 0;
    k = "root\n"[last++];
    if (last == 5)
      last = 0;
    // generate a random key every 1s for pal
    //k = -1;//"uiojkl"[rand()% 6];
  }
  return k;
}

// No TX FIFO needed - output goes directly to screen

uint32_t uptime(void);

// Check if RX FIFO has data
extern "C" int uart16550_rx_ready(void) {
  return (rx_f != rx_r) ? LSR_RX_READY : 0;
}

// TX is always ready in simulation (no FIFO needed)
extern "C" int uart16550_tx_empty(void) {
  return LSR_TX_READY | LSR_FIFO_EMPTY;
}

// Get LSR (Line Status Register) value
extern "C" uint8_t uart16550_get_lsr(void) {
  return LSR_TX_READY | LSR_FIFO_EMPTY | uart16550_rx_ready();
}

// Main UART putc function for 16550
extern "C" void uart16550_putc(uint8_t ch) {
  printf("%c", ch);
  fflush(stdout);
}

// Main UART getc function for 16550
extern "C" uint8_t uart16550_getc() {
  static uint32_t lasttime = 0;
  uint32_t now = uptime();

  uint8_t ch = -1; // Default value like original UART
  if (now - lasttime > 60 * 1000) {
    // 1 minute
    eprintf(ANSI_COLOR_RED "uart16550: now = %ds\n" ANSI_COLOR_RESET, now / 1000);
    lasttime = now;
  }
  
  // Return data from RX FIFO if available
  if (rx_f != rx_r) {
    ch = uart16550_rx_dequeue();
  }
  
  return ch;
}

// Legacy getc function for compatibility
extern "C" void uart16550_getc_legacy(uint8_t *ch) {
  static uint32_t lasttime = 0;
  uint32_t now = uptime();

  *ch = -1; // Default value like original UART
  if (now - lasttime > 60 * 1000) {
    // 1 minute
    eprintf(ANSI_COLOR_RED "uart16550: now = %ds\n" ANSI_COLOR_RESET, now / 1000);
    lasttime = now;
  }
  
  // Return data from RX FIFO if available
  if (rx_f != rx_r) {
    *ch = uart16550_rx_dequeue();
  }
}


// Read 16550 register
extern "C" uint8_t uart16550_read_reg(uint8_t offset) {
  switch (offset) {
    case RBR_THR_OFFSET:
      return uart16550_rx_dequeue(); // RBR (read)
    case IER_OFFSET:
      return IER;
    case IIR_FCR_OFFSET:
      return IIR; // IIR (read)
    case LCR_OFFSET:
      return LCR;
    case MCR_OFFSET:
      return MCR;
    case LSR_OFFSET:
      return uart16550_get_lsr(); // LSR (read)
    case MSR_OFFSET:
      return MSR; // MSR (read)
    case SCR_OFFSET:
      return SCR;
    default:
      return 0xff;
  }
}

// Write 16550 register
extern "C" void uart16550_write_reg(uint8_t offset, uint8_t data) {
  switch (offset) {
    case RBR_THR_OFFSET:
      uart16550_putc(data); // THR (write)
      break;
    case IER_OFFSET:
      IER = data;
      break;
    case IIR_FCR_OFFSET:
      FCR = data; // FCR (write)
      break;
    case LCR_OFFSET:
      LCR = data;
      break;
    case MCR_OFFSET:
      MCR = data;
      break;
    case MSR_OFFSET:
      // MSR is read-only
      break;
    case SCR_OFFSET:
      SCR = data;
      break;
    default:
      break;
  }
}

// Preset input for testing (same as original uart.cpp)
static void uart16550_preset_input() {
  char rtthread_cmd[128] = "memtrace\n";
  char init_cmd[128] =
      "2"             // choose PAL
      "jjjjjjjkkkkkk" // walk to enemy
      ;
  char busybox_cmd[128] =
      "ls\n"
      "echo 123\n"
      "cd /root/benchmark\n"
      "ls\n"
      "./stream\n"
      "ls\n"
      "cd /root/redis\n"
      "ls\n"
      "ifconfig -a\n"
      "./redis-server\n";
  char debian_cmd[128] = "root\n";
  char *buf = debian_cmd;
  int i;
  for (i = 0; i < strlen(buf); i++) {
    uart16550_rx_enqueue(buf[i]);
  }
}

// Initialize 16550 UART
extern "C" void init_uart16550(void) {
  // Initialize RX FIFO only
  memset(rx_fifo, 0, sizeof(rx_fifo));
  rx_f = rx_r = 0;
  
  // Initialize registers
  IER = 0;
  IIR = 1; // No interrupt pending
  FCR = 0;
  LCR = 0;
  MCR = 0;
  MSR = 0;
  SCR = 0;
  
  // Preset input
  uart16550_preset_input();
}

// Cleanup 16550 UART
extern "C" void finish_uart16550(void) {
  memset(rx_fifo, 0, sizeof(rx_fifo));
  rx_f = rx_r = 0;
}

// Get FIFO status for debugging
extern "C" void uart16550_get_fifo_status(int *rx_count, int *tx_count) {
  *rx_count = (rx_r - rx_f + RX_FIFO_SIZE) % RX_FIFO_SIZE;
  *tx_count = 0; // No TX FIFO in simulation
}

// 16550 specific functions are already defined above 