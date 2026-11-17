#include "database.h"

#include <stdio.h>

int db_execute(MYSQL *db, const char *sql)
{
    if (mysql_query(db, sql) != 0) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(db));
        return 0;
    }
    return 1;
}

MYSQL_RES *db_query(MYSQL *db, const char *sql)
{
    if (!db_execute(db, sql)) return NULL;
    return mysql_store_result(db);
}
