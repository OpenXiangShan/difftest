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

#include "device.h"
#include "flash.h"
#include "sdcard.h"
#include "uart.h"
#include "vga.h"
#ifdef SHOW_SCREEN
#include <SDL2/SDL.h>
#endif

void send_key(uint8_t, bool);

void init_device(void) {
#ifdef SHOW_SCREEN
  init_sdl();
#endif
  init_uart();
  init_sd();
}

void finish_device(void) {
#ifdef SHOW_SCREEN
  finish_sdl();
#endif
  finish_uart();
  finish_sd();
}

void poll_event() {
#ifdef SHOW_SCREEN
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        break; //set_abort();

        // If a key was pressed
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        uint8_t k = event.key.keysym.scancode;
        bool is_keydown = (event.key.type == SDL_KEYDOWN);
        send_key(k, is_keydown);
        break;
      }
      default: break;
    }
  }
#endif
}
