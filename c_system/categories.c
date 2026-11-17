#include "categories.h"

#include <stdio.h>

int category_add(MYSQL *db, const char *name)
{
    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT INTO categories(category_name,discount_percent) VALUES('%s',0);",
             name);
    return db_execute(db, sql);
}

int category_update_discount(MYSQL *db, const char *name, double percent)
{
    char sql[256];
    snprintf(sql, sizeof(sql),
             "UPDATE categories SET discount_percent=%.2f WHERE category_name='%s';",
             percent, name);
    return db_execute(db, sql);
}

int coupon_find(MYSQL *db, const char *code, double *percent, double *flat)
{
    char sql[256];
    MYSQL_RES *result;
    MYSQL_ROW row;
    snprintf(sql, sizeof(sql),
             "SELECT discount_percent,flat_amount FROM coupons "
             "WHERE active=TRUE AND coupon_code='%s';", code);
    result = db_query(db, sql);
    if (!result) return 0;
    row = mysql_fetch_row(result);
    if (row == NULL) {
        mysql_free_result(result);
        return 0;
    }
    *percent = atof(row[0]);
    *flat = atof(row[1]);
    mysql_free_result(result);
    return 1;
}
