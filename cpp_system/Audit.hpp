#ifndef CAMPUS_AUDIT_HPP
#define CAMPUS_AUDIT_HPP

#include "Product.hpp"

#include <ostream>

void auditProduct(const Product& product);
void printProductSummary(std::ostream& output, const StoreProduct& product);

#endif
