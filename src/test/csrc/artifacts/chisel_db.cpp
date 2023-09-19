
#include"chisel_db.h"
#include <string.h>
#include <stdbool.h>

bool dump;
sqlite3 *mem_db;
char * zErrMsg;
int rc;

bool enable_dump_TLLog = true;



static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  return 0;
}

  
void init_db_TLLog() {
  // create table
  if (!enable_dump_TLLog) return;

  char * sql = "CREATE TABLE TLLog(" \
    "ID INTEGER PRIMARY KEY AUTOINCREMENT," \
    "ECHO INT NOT NULL," \
    "USER INT NOT NULL," \
    "DATA_0 INT NOT NULL," \
    "DATA_1 INT NOT NULL," \
    "DATA_2 INT NOT NULL," \
    "DATA_3 INT NOT NULL," \
    "ADDRESS INT NOT NULL," \
    "SINK INT NOT NULL," \
    "SOURCE INT NOT NULL," \
    "PARAM INT NOT NULL," \
    "OPCODE INT NOT NULL," \
    "CHANNEL INT NOT NULL," \
    "STAMP INT NOT NULL," \
    "SITE TEXT);";
  rc = sqlite3_exec(mem_db, sql, callback, 0, &zErrMsg);
  if(rc != SQLITE_OK) {
    printf("SQL error: %s\n", zErrMsg);
    exit(0);
  } else {
    printf("%s table created successfully!\n", "TLLog");
  }
}


  
extern "C" void TLLog_write(
  uint64_t echo,
  uint64_t user,
  uint64_t data_0,
  uint64_t data_1,
  uint64_t data_2,
  uint64_t data_3,
  uint64_t address,
  uint64_t sink,
  uint64_t source,
  uint64_t param,
  uint64_t opcode,
  uint64_t channel,
  uint64_t stamp,
  char * site
) {
  if(!dump || !enable_dump_TLLog) return;

  char * format = "INSERT INTO TLLog(ECHO,USER,DATA_0,DATA_1,DATA_2,DATA_3,ADDRESS,SINK,SOURCE,PARAM,OPCODE,CHANNEL, STAMP, SITE) " \
                  "VALUES(%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, '%s');";
  char * sql = (char *)malloc(13 * sizeof(uint64_t) + (strlen(format)+strlen(site)) * sizeof(char));
  sprintf(sql,
    format,
    echo,user,data_0,data_1,data_2,data_3,address,sink,source,param,opcode,channel, stamp, site
  );
  rc = sqlite3_exec(mem_db, sql, callback, 0, &zErrMsg);
  free(sql);
  if(rc != SQLITE_OK) {
    printf("SQL error: %s\n", zErrMsg);
    exit(0);
  };
}



void init_db(bool en, bool select_enable, const char *select_db){
  dump = en;
  if(!en) return;
  rc = sqlite3_open(":memory:", &mem_db);
  if(rc) {
    printf("Can't open database: %s\n", sqlite3_errmsg(mem_db));
    exit(0);
  } else {
    printf("Open database successfully\n");
  }

  if (select_enable) {
    char *select_db_list[256];
    int select_db_num = 0;

    const char *split_word = " ";
    char select_db_copy[1024];
    select_db_copy[0] = '\0';
    strncpy(select_db_copy, select_db, 1024);
    select_db_copy[1023] = '\0';
    char *str_p = strtok(select_db_copy, split_word);
    while (str_p) {
      select_db_list[select_db_num] = str_p;
      select_db_num ++;
      str_p = strtok(NULL, split_word);
    }
    enable_dump_TLLog = false;

    {
      char table_name[] = "TLLog";
      for (int idx = 0; idx < select_db_num; idx++) {
        char *str_p = select_db_list[idx];
        int s_idx = 0;
        bool match = true;
        for (; (str_p[s_idx] != '\0') && (table_name[s_idx] != '\0'); s_idx ++) {
          if (str_p[s_idx] != table_name[s_idx]) {
            match = false;
            break;
          }
        }
        if (!match || (str_p[s_idx] != '\0'))  continue;

        enable_dump_TLLog = true;
        break;
      }
    }

  }

  init_db_TLLog();

}

void save_db(const char *zFilename) {
  printf("saving memdb to %s ...\n", zFilename);
  sqlite3 *disk_db;
  sqlite3_backup *pBackup;
  rc = sqlite3_open(zFilename, &disk_db);
  if(rc == SQLITE_OK){
    pBackup = sqlite3_backup_init(disk_db, "main", mem_db, "main");
    if(pBackup){
      (void)sqlite3_backup_step(pBackup, -1);
      (void)sqlite3_backup_finish(pBackup);
    }
    rc = sqlite3_errcode(disk_db);
  }
  sqlite3_close(disk_db);
}


