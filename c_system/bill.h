#ifndef CAMPUS_BILL_H
#define CAMPUS_BILL_H

#include "product.h"

double bill_subtotal(const Product products[], const int indexes[],
                     const int quantities[], int count);
double bill_coupon_discount(double subtotal, double percent, double flat);
double bill_gst(double taxable_total, double percent);
void bill_show_previous(MYSQL *db);

#endif
