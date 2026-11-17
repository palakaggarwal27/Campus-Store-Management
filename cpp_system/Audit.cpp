#include "Audit.hpp"

#include <iostream>

void auditProduct(const Product& product)
{
    std::cout << "Audit: ID=" << product.id_
              << ", Name=" << product.name_ << "\n";
}

void printProductSummary(std::ostream& output, const StoreProduct& product)
{
    output << product.id << " | " << product.name
           << " | " << product.category << " | Rs "
           << product.sellingPrice();
}
