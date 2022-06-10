#ifndef _TL_LOGGER_H_
#define _TL_LOGGER_H_

#include "common.h"
#include <sqlite3.h>

void init_logger(bool enable);
void save_db(const char *zFilename);
void checkpoint_db(const char *zFilename);
#endif
