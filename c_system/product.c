#include "product.h"
#include "database.h"

#include <stdio.h>
#include <stdlib.h>

int load_products(MYSQL *db, Product products[], int *count)
{
    MYSQL_RES *result = db_query(db,
        "SELECT i.product_id,i.name,i.category,i.base_price,"
        "COALESCE(c.discount_percent,0),i.stock_quantity FROM inventory i "
        "LEFT JOIN categories c ON i.category=c.category_name ORDER BY i.name;");
    MYSQL_ROW row;
    *count = 0;
    if (!result) return 0;
    while ((row = mysql_fetch_row(result)) != NULL && *count < MAX_PRODUCTS) {
        Product *product = &products[*count];
        product->id = atoi(row[0]);
        snprintf(product->name, sizeof(product->name), "%s", row[1]);
        snprintf(product->category, sizeof(product->category), "%s", row[2]);
        product->price = atof(row[3]);
        product->discount = atof(row[4]);
        product->stock = atoi(row[5]);
        ++*count;
    }
    mysql_free_result(result);
    return 1;
}

double selling_price(const Product *product)
{
    return product->price * (1.0 - product->discount / 100.0);
}
