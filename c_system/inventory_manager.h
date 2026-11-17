#ifndef CAMPUS_INVENTORY_MANAGER_H
#define CAMPUS_INVENTORY_MANAGER_H

#include "database.h"
#include "product.h"

int inventory_add_stock(MYSQL *db, int product_id, int quantity);
int inventory_set_stock(MYSQL *db, int product_id, int quantity);
void inventory_display(MYSQL *db);

#endif
