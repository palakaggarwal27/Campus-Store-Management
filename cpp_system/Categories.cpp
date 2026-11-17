#include "Categories.hpp"

bool CategoryManager::load(Database& database,
                            std::vector<Category>& categories)
{
    MYSQL_RES* result = database.query(
        "SELECT category_name,discount_percent FROM categories ORDER BY category_name;");
    if (!result) return false;
    categories.clear();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr)
        categories.push_back({row[0], std::stod(row[1])});
    mysql_free_result(result);
    return true;
}

bool CategoryManager::add(Database& database, const std::string& name)
{
    return database.execute(
        "INSERT INTO categories(category_name,discount_percent) VALUES('" +
        database.escape(name) + "',0);");
}

bool CategoryManager::updateDiscount(Database& database,
                                      const std::string& name,
                                      double discountPercent)
{
    return database.execute(
        "UPDATE categories SET discount_percent=" +
        std::to_string(discountPercent) + " WHERE category_name='" +
        database.escape(name) + "';");
}

bool CouponManager::find(Database& database, const std::string& code,
                          double& percent, double& flatAmount)
{
    MYSQL_RES* result = database.query(
        "SELECT discount_percent,flat_amount FROM coupons "
        "WHERE active=TRUE AND coupon_code='" + database.escape(code) + "';");
    if (!result) return false;
    MYSQL_ROW row = mysql_fetch_row(result);
    const bool found = row != nullptr;
    if (found) {
        percent = std::stod(row[0]);
        flatAmount = std::stod(row[1]);
    }
    mysql_free_result(result);
    return found;
}

bool CouponManager::save(Database& database, const std::string& code,
                          double percent, double flatAmount)
{
    return database.execute(
        "INSERT INTO coupons(coupon_code,discount_percent,flat_amount,active) VALUES('" +
        database.escape(code) + "'," + std::to_string(percent) + "," +
        std::to_string(flatAmount) +
        ",TRUE) ON DUPLICATE KEY UPDATE discount_percent=" +
        std::to_string(percent) + ",flat_amount=" + std::to_string(flatAmount) +
        ",active=TRUE;");
}
