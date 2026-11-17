#include "bill.h"
#include "database.h"

#include <stdio.h>

double bill_subtotal(const Product products[], const int indexes[],
                     const int quantities[], int count)
{
    double subtotal = 0.0;
    int i;
    for (i = 0; i < count; ++i)
        subtotal += selling_price(&products[indexes[i]]) * quantities[i];
    return subtotal;
}

double bill_coupon_discount(double subtotal, double percent, double flat)
{
    double discount = subtotal * percent / 100.0 + flat;
    return discount > subtotal ? subtotal : discount;
}

double bill_gst(double taxable_total, double percent)
{
    return taxable_total * percent / 100.0;
}

void bill_show_previous(MYSQL *db)
{
    MYSQL_RES *result = db_query(
        db, "SELECT o.order_id,o.total_amount,SUM(i.quantity),o.created_at "
            "FROM orders o JOIN order_items i ON i.order_id=o.order_id "
            "GROUP BY o.order_id ORDER BY o.order_id DESC;");
    MYSQL_ROW row;
    if (!result) return;
    while ((row = mysql_fetch_row(result)) != NULL)
        printf("Order ID: %s | Bill: Rs %s | Items: %s | %s\n",
               row[0], row[1], row[2], row[3]);
    mysql_free_result(result);
}
