/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifndef __SPLITVIEW_H
#define __SPLITVIEW_H

void common_splitview_preinit(const char *program_name);
void common_splitview_finish();
void common_splitview_force_cleanup();
void common_splitview_release_input_capture();
void common_splitview_set_uart_input_fd(int fd);
void common_splitview_set_log_path(const char *path);
bool common_splitview_is_active();
int common_splitview_uart_fd();

#endif // __SPLITVIEW_H
