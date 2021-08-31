#include "logger.h"

sqlite3 *mem_db;
sqlite3 *disk_db;
sqlite3_backup *pBackup;
char* zErrMsg = 0;
int rc;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   return 0;
}

void init_logger() {
    rc = sqlite3_open(":memory:", &mem_db);
    if(rc) {
        printf("Can't open database: %s\n", sqlite3_errmsg(mem_db));
        exit(0);
    } else {
        printf("Open database successfully\n");
    }
    char* sql =  "CREATE TABLE LOG("  \
                 "ID                INTEGER     PRIMARY KEY AUTOINCREMENT," \
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
        printf("LOG table created successfully!\n");
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

extern "C" void log_write_helper(
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
    uint64_t stamp
) {
    // insert to log db
    // printf("channel:[%d] address:[%lx]\n", channel, address);
    char sql[256];
    sprintf(sql, "INSERT INTO LOG(CHANNEL,OPCODE,PARAM,SOURCE,SINK,ADDRESS,DATA_0,DATA_1,DATA_2,DATA_3,STAMP) VALUES(%d,%d,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld);",
        channel, opcode, param, source, sink, address,
        data_0, data_1, data_2, data_3, stamp
    );
    // printf("s=%s\n", sql);

    rc = sqlite3_exec(mem_db, sql, callback, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        printf("SQL error: %s\n", zErrMsg);
        exit(0);
    };
}