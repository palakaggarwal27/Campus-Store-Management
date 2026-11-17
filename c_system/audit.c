#include "audit.h"

#include <stdio.h>

void audit_product(const Product *product)
{
    printf("Product ID: %d | Name: %s | Category: %s | Price: Rs %.2f\n",
           product->id, product->name, product->category,
           selling_price(product));
}
