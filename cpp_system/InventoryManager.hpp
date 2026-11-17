#ifndef CAMPUS_INVENTORY_MANAGER_HPP
#define CAMPUS_INVENTORY_MANAGER_HPP

#include "Database.hpp"

class InventoryManager {
    Database& database_;

public:
    explicit InventoryManager(Database& database) : database_(database) {}
    bool addStock(int productId, int quantity);
    bool setStock(int productId, int quantity);
    int getStock(int productId) const;
    void display() const;
};

#endif
