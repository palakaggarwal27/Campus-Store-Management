#ifndef CAMPUS_CATEGORIES_HPP
#define CAMPUS_CATEGORIES_HPP

#include "Database.hpp"

#include <string>
#include <vector>

struct Category {
    std::string name;
    double discountPercent;
};

class CategoryManager {
public:
    static bool load(Database& database, std::vector<Category>& categories);
    static bool add(Database& database, const std::string& name);
    static bool updateDiscount(Database& database, const std::string& name,
                               double discountPercent);
};

class CouponManager {
public:
    static bool find(Database& database, const std::string& code,
                     double& percent, double& flatAmount);
    static bool save(Database& database, const std::string& code,
                     double percent, double flatAmount);
};

#endif
