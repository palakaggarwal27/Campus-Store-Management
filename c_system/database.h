#ifndef CAMPUS_DATABASE_H
#define CAMPUS_DATABASE_H

#include <mysql.h>

int db_execute(MYSQL *db, const char *sql);
MYSQL_RES *db_query(MYSQL *db, const char *sql);

#endif
