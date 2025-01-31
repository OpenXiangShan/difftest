/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifdef CONFIG_DIFFTEST_QUERY
#include "query.h"
#include "difftest-query.h"

QueryStats *qStats;

void difftest_query_init() {
  char query_path[128];
  snprintf(query_path, 128, "%s/build/%s", getenv("NOOP_HOME"), "difftest_query.db");
  // remove exist file
  FILE *fp = fopen(query_path, "r");
  if (fp) {
    fclose(fp);
    remove(query_path);
  }
  qStats = new QueryStats(query_path);
}

void difftest_query_step() {
  qStats->step();
}

void difftest_query_finish() {
  delete qStats;
}

#endif // CONFIG_DIFFTEST_QUERY
