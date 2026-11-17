#ifndef CAMPUS_CATEGORIES_H
#define CAMPUS_CATEGORIES_H

#include "database.h"

int category_add(MYSQL *db, const char *name);
int category_update_discount(MYSQL *db, const char *name, double percent);
int coupon_find(MYSQL *db, const char *code, double *percent, double *flat);

#endif
