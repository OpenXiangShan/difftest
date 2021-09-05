#include "logger.h"

bool dump_tl;
sqlite3 *mem_db;
sqlite3 *disk_db;
sqlite3_backup *pBackup;
char* zErrMsg = 0;
int rc;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   return 0;
}

void init_logger(bool dump) {
    dump_tl = dump;
    if(!dump) return;
    rc = sqlite3_open(":memory:", &mem_db);
    if(rc) {
        printf("Can't open database: %s\n", sqlite3_errmsg(mem_db));
        exit(0);
    } else {
        printf("Open database successfully\n");
    }
    char* sql =  "CREATE TABLE TL_LOG("  \
                 "ID                INTEGER     PRIMARY KEY AUTOINCREMENT," \
                 "NAME              TEXT    NOT NULL," \
                 "CHANNEL           INT     NOT NULL," \
                 "OPCODE            INT     NOT NULL," \
                 "PARAM             INT     NOT NULL," \
                 "SOURCE            INT," \
                 "SINK              INT," \
                 "ADDRESS           INT     NOT NULL," \
                 "DATA_0            INT," \
                 "DATA_1            INT," \
                 "DATA_2            INT," \
                 "DATA_3            INT," \
                 "STAMP             INT    NOT NULL);";
    rc = sqlite3_exec(mem_db, sql, callback, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        printf("SQL error: %s\n", zErrMsg);
        exit(0);
    } else {
        printf("TL_LOG table created successfully!\n");
    }

    sql =  "CREATE TABLE DIR_LOG("  \
           "ID                INTEGER     PRIMARY KEY AUTOINCREMENT," \
           "NAME              TEXT    NOT NULL," \
           "TAG               INT     NOT NULL," \
           "IDX               INT     NOT NULL," \
           "DIR               INT     NOT NULL," \
           "WAY               INT     NOT NULL," \
           "TYPEID            INT     NOT NULL," \
           "STAMP             INT     NOT NULL);";
    rc = sqlite3_exec(mem_db, sql, callback, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        printf("SQL error: %s\n", zErrMsg);
        exit(0);
    } else {
        printf("DIR_LOG table created successfully!\n");
    }
}


void save_db(const char *zFilename) {
    printf("Saving memedb to %s ...\n", zFilename);
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

extern "C" void tl_log_write_helper(
    uint8_t channel,
    uint8_t opcode,
    uint8_t param,
    uint8_t source,
    uint8_t sink,
    uint64_t address,
    uint64_t data_0,
    uint64_t data_1,
    uint64_t data_2,
    uint64_t data_3,
    uint64_t stamp,
    char*  prefix
) {
    if(!dump_tl) return;
    // insert to log db
    char sql[256];
    sprintf(sql,
        "INSERT INTO TL_LOG(NAME,CHANNEL,OPCODE,PARAM,SOURCE,SINK,ADDRESS,DATA_0,DATA_1,DATA_2,DATA_3,STAMP) VALUES('%s',%d,%d,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld);",
        prefix, channel, opcode, param, source, sink, address,
        data_0, data_1, data_2, data_3, stamp
    );

    rc = sqlite3_exec(mem_db, sql, callback, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        printf("SQL error: %s\n", zErrMsg);
        exit(0);
    };
}

extern "C" void dir_log_write_helper(
    uint64_t tag,
    uint64_t set_idx,
    uint64_t dir,
    uint64_t stamp,
    uint8_t way,
    uint8_t typeId,
    char* prefix
) {
    if(!dump_tl) return;
    char sql[256];
    sprintf(sql,
        "INSERT INTO DIR_LOG(NAME,TAG,IDX,DIR,WAY,TYPEID,STAMP) VALUES('%s',%ld,%ld,%ld,%d,%d,%ld);",
        prefix, tag, set_idx, dir, way, typeId, stamp
    );

    rc = sqlite3_exec(mem_db, sql, callback, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        printf("SQL error: %s\n", zErrMsg);
        exit(0);
    };
}
