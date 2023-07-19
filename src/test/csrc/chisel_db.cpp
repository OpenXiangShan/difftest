
#include"chisel_db.h"

extern bool dump;
extern sqlite3 *mem_db;
extern char * zErrMsg;
extern int rc;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  return 0;
}

  
void init_db_L2MP() {
  // create table
  char * sql = "CREATE TABLE L2MP(" \
    "ID INTEGER PRIMARY KEY AUTOINCREMENT," \
    "METAWWAY INT NOT NULL," \
    "METAWVALID INT NOT NULL," \
    "MSHRID INT NOT NULL," \
    "ALLOCPTR INT NOT NULL," \
    "ALLOCVALID INT NOT NULL," \
    "DIRWAY INT NOT NULL," \
    "DIRHIT INT NOT NULL," \
    "SSET INT NOT NULL," \
    "TAG INT NOT NULL," \
    "OPCODE INT NOT NULL," \
    "CHANNEL INT NOT NULL," \
    "MSHRTASK INT NOT NULL," \
    "STAMP INT NOT NULL," \
    "SITE TEXT);";
  rc = sqlite3_exec(mem_db, sql, callback, 0, &zErrMsg);
  if(rc != SQLITE_OK) {
    printf("SQL error: %s\n", zErrMsg);
    exit(0);
  } else {
    printf("%s table created successfully!\n", "L2MP");
  }
}


  
extern "C" void L2MP_write(
  uint64_t metaWway,
  uint64_t metaWvalid,
  uint64_t mshrId,
  uint64_t allocPtr,
  uint64_t allocValid,
  uint64_t dirWay,
  uint64_t dirHit,
  uint64_t sset,
  uint64_t tag,
  uint64_t opcode,
  uint64_t channel,
  uint64_t mshrTask,
  uint64_t stamp,
  char * site
) {
}


