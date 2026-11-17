#include "InventoryManager.hpp"

#include <iomanip>
#include <iostream>

bool InventoryManager::addStock(int productId, int quantity)
{
    if (quantity <= 0) return false;
    return database_.execute(
        "UPDATE inventory SET stock_quantity=stock_quantity+" +
        std::to_string(quantity) + " WHERE product_id=" +
        std::to_string(productId) + ";");
}

bool InventoryManager::setStock(int productId, int quantity)
{
    if (quantity < 0) return false;
    return database_.execute(
        "UPDATE inventory SET stock_quantity=" + std::to_string(quantity) +
        " WHERE product_id=" + std::to_string(productId) + ";");
}

int InventoryManager::getStock(int productId) const
{
    MYSQL_RES* result = database_.query(
        "SELECT stock_quantity FROM inventory WHERE product_id=" +
        std::to_string(productId) + ";");
    if (!result) return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    const int stock = row ? std::stoi(row[0]) : 0;
    mysql_free_result(result);
    return stock;
}

void InventoryManager::display() const
{
    MYSQL_RES* result = database_.query(
        "SELECT product_id,name,category,base_price,stock_quantity "
        "FROM inventory ORDER BY name;");
    if (!result) return;
    std::cout << "\nID      Name                 Category       Price       Stock\n";
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        std::cout << std::left << std::setw(8) << row[0]
                  << std::setw(21) << row[1] << std::setw(15) << row[2]
                  << std::setw(12) << row[3] << row[4] << "\n";
    }
    mysql_free_result(result);
}
