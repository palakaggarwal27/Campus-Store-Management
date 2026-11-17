#include "UIEngine.hpp"

#include <iostream>
#include <limits>

void UIEngine::showMenu()
{
    std::cout << "\n1. Generate New Cart\n2. View Inventory\n3. Restock Products\n"
              << "4. Add New Product\n5. Update Product Details\n"
              << "6. View Previous Bills\n0. Exit\nChoose: "
              << std::flush;
}

void UIEngine::clearInput()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
