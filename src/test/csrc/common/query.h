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

#ifndef __QUERY_H__
#define __QUERY_H__

#include "common.h"

#ifdef CONFIG_DIFFTEST_QUERY
#include <sqlite3.h>

class Query {
protected:
  sqlite3_stmt *pPrepare = nullptr;
  sqlite3 *query_db = nullptr;

public:
  Query(sqlite3 *db, const char *createSql, const char *insertSql) {
    query_db = db;
    char *errMsg;
    int rc;
    rc = sqlite3_exec(query_db, createSql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
      printf("SQL error: %s\n", errMsg);
      assert(0);
    }
    rc = sqlite3_prepare_v2(query_db, insertSql, strlen(insertSql), &pPrepare, 0);
    if (rc != SQLITE_OK) {
      printf("SQL error: %s\n", sqlite3_errmsg(query_db));
      assert(0);
    }
  }
  ~Query() {
    sqlite3_finalize(pPrepare);
  }
  void write(int count, ...) {
    va_list args;
    va_start(args, count);
    sqlite3_reset(pPrepare);
    for (int i = 0; i < count; i++) {
      sqlite3_bind_int(pPrepare, i + 1, va_arg(args, int));
    }
    va_end(args);
    sqlite3_step(pPrepare);
  }
};

class QueryStatsBase {
public:
  char path[128];
  long long query_step = 0;
  sqlite3 *mem_db = nullptr;
  QueryStatsBase(char *_path) {
    strncpy(path, _path, 128);
    sqlite3_open(":memory:", &mem_db);
    sqlite3_exec(mem_db, "PRAGMA synchronous = OFF", 0, 0, 0);
    sqlite3_exec(mem_db, "BEGIN;", 0, 0, 0);
  }
  ~QueryStatsBase() {
    sqlite3_exec(mem_db, "COMMIT;", 0, 0, 0);
    sqlite3 *disk_db = nullptr;
    sqlite3_backup *pBackup;
    int rc = sqlite3_open(path, &disk_db);
    if (rc == SQLITE_OK) {
      pBackup = sqlite3_backup_init(disk_db, "main", mem_db, "main");
      if (pBackup) {
        (void)sqlite3_backup_step(pBackup, -1);
        (void)sqlite3_backup_finish(pBackup);
      }
      rc = sqlite3_errcode(disk_db);
    }
    sqlite3_close(disk_db);
    sqlite3_close(mem_db);
  }
  virtual void step() {
    query_step++;
    if (query_step % 10000 == 0) {
      sqlite3_exec(mem_db, "COMMIT;", 0, 0, 0);
      sqlite3_exec(mem_db, "BEGIN;", 0, 0, 0);
    }
  }
};

class QueryStats;
extern QueryStats *qStats;

void difftest_query_init();
void difftest_query_step();
void difftest_query_finish();
#endif // CONFIG_DIFFTEST_QUERY
#endif // __QUERY_H__
