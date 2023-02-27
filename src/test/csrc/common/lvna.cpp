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

#include <sys/mman.h>

#include "common.h"
#include "lvna.h"

static char *uart1_out_path = NULL;
static FILE *uart1_fp = NULL;

void putc_uart1(char c) {
  if (uart1_fp)
  {
    fputc(c,uart1_fp);
  }
}

void init_lvna(const char *uart1_path) {
  if (uart1_path) {
    uart1_out_path = (char *)uart1_path;
    printf("[info]use %s as uart1 path\n",uart1_out_path);
    uart1_fp = fopen(uart1_out_path, "w");
    if (!uart1_fp) {
      printf("[error]open uart1 path %s failed\n",uart1_out_path);
      exit(1);
    }
    setvbuf(uart1_fp,NULL,_IOLBF,1024);
  }
}

void release_lvna(){
  if (uart1_fp) {
    fclose(uart1_fp);
  }
}

