#include "inventory_manager.h"

#include <stdio.h>

int inventory_add_stock(MYSQL *db, int product_id, int quantity)
{
    char sql[256];
    if (quantity <= 0) return 0;
    snprintf(sql, sizeof(sql),
             "UPDATE inventory SET stock_quantity=stock_quantity+%d WHERE product_id=%d;",
             quantity, product_id);
    return db_execute(db, sql);
}

int inventory_set_stock(MYSQL *db, int product_id, int quantity)
{
    char sql[256];
    if (quantity < 0) return 0;
    snprintf(sql, sizeof(sql),
             "UPDATE inventory SET stock_quantity=%d WHERE product_id=%d;",
             quantity, product_id);
    return db_execute(db, sql);
}

void inventory_display(MYSQL *db)
{
    MYSQL_RES *result = db_query(
        db, "SELECT product_id,name,category,base_price,stock_quantity "
            "FROM inventory ORDER BY name;");
    MYSQL_ROW row;
    if (!result) return;
    printf("\n%-8s %-21s %-15s %-12s %s\n",
           "ID", "Name", "Category", "Price", "Stock");
    while ((row = mysql_fetch_row(result)) != NULL)
        printf("%-8s %-21s %-15s %-12s %s\n",
               row[0], row[1], row[2], row[3], row[4]);
    mysql_free_result(result);
}
