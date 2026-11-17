#ifndef CAMPUS_PRODUCT_H
#define CAMPUS_PRODUCT_H

#include <mysql.h>

#define MAX_PRODUCTS 200
#define MAX_NAME 100
#define MAX_CATEGORY 50

typedef struct {
    int id;
    char name[MAX_NAME];
    char category[MAX_CATEGORY];
    double price;
    double discount;
    int stock;
} Product;

int load_products(MYSQL *db, Product products[], int *count);
double selling_price(const Product *product);

#endif
