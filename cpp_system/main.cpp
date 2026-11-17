#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "Categories.hpp"
#include "Database.hpp"
#include "Product.hpp"

static void clearInput()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static bool initialize(Database& db)
{
    const char* statements[] = {
        "CREATE TABLE IF NOT EXISTS inventory ("
        "product_id INT PRIMARY KEY, name VARCHAR(100) NOT NULL, "
        "base_price DECIMAL(10,2) NOT NULL CHECK (base_price >= 0), "
        "category VARCHAR(50) NOT NULL, stock_quantity INT NOT NULL DEFAULT 0, "
        "total_sold INT NOT NULL DEFAULT 0, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);",
        "CREATE TABLE IF NOT EXISTS categories ("
        "category_name VARCHAR(50) PRIMARY KEY, discount_percent DECIMAL(5,2) NOT NULL DEFAULT 0);",
        "CREATE TABLE IF NOT EXISTS coupons ("
        "coupon_code VARCHAR(30) PRIMARY KEY, discount_percent DECIMAL(5,2) NOT NULL DEFAULT 0, "
        "flat_amount DECIMAL(10,2) NOT NULL DEFAULT 0, active BOOLEAN NOT NULL DEFAULT TRUE);",
        "CREATE TABLE IF NOT EXISTS orders ("
        "order_id INT AUTO_INCREMENT PRIMARY KEY, total_amount DECIMAL(10,2) NOT NULL, "
        "paid_amount DECIMAL(10,2) NOT NULL, change_amount DECIMAL(10,2) NOT NULL, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);",
        "CREATE TABLE IF NOT EXISTS order_items ("
        "order_item_id INT AUTO_INCREMENT PRIMARY KEY, order_id INT NOT NULL, product_id INT NOT NULL, "
        "product_name VARCHAR(100) NOT NULL, unit_price DECIMAL(10,2) NOT NULL, quantity INT NOT NULL, "
        "line_total DECIMAL(10,2) NOT NULL, FOREIGN KEY(order_id) REFERENCES orders(order_id), "
        "FOREIGN KEY(product_id) REFERENCES inventory(product_id));",
        "CREATE TABLE IF NOT EXISTS order_details ("
        "order_id INT PRIMARY KEY, subtotal DECIMAL(10,2) NOT NULL, category_discount DECIMAL(10,2) NOT NULL, "
        "coupon_discount DECIMAL(10,2) NOT NULL, gst_percent DECIMAL(5,2) NOT NULL, "
        "gst_amount DECIMAL(10,2) NOT NULL, payment_method VARCHAR(30) NOT NULL, "
        "FOREIGN KEY(order_id) REFERENCES orders(order_id));",
        "CREATE TABLE IF NOT EXISTS accounts ("
        "account_name VARCHAR(30) PRIMARY KEY, balance DECIMAL(12,2) NOT NULL DEFAULT 0);",
        "CREATE TABLE IF NOT EXISTS financial_transactions ("
        "transaction_id INT AUTO_INCREMENT PRIMARY KEY, transaction_type VARCHAR(20) NOT NULL, "
        "order_id INT NULL, amount DECIMAL(12,2) NOT NULL, payment_method VARCHAR(30) NOT NULL, "
        "account_name VARCHAR(30) NOT NULL, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(order_id) REFERENCES orders(order_id));",
        "INSERT IGNORE INTO accounts(account_name,balance) VALUES('cash_counter',0),('cash_bank',0);",
        "INSERT IGNORE INTO categories(category_name,discount_percent) VALUES"
        "('Book',10),('Accessory',0),('Stationery',5);",
        "UPDATE categories SET discount_percent=0 WHERE category_name='Accessory' "
        "AND discount_percent<0;",
        "INSERT IGNORE INTO inventory(product_id,name,base_price,category,stock_quantity) VALUES"
        "(101,'C++ Textbook',500,'Book',20),(202,'USB Mouse',800,'Accessory',15),"
        "(303,'Pen Set',200,'Stationery',50);"
    };
    for (const char* statement : statements) {
        if (!db.execute(statement)) return false;
    }
    return true;
}

static bool loadProducts(Database& db, std::vector<StoreProduct>& products)
{
    MYSQL_RES* result = db.query(
        "SELECT i.product_id,i.name,i.category,i.base_price,"
        "COALESCE(c.discount_percent,0),i.stock_quantity FROM inventory i "
        "LEFT JOIN categories c ON i.category=c.category_name ORDER BY i.name;");
    if (!result) return false;
    products.clear();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        products.push_back({std::stoi(row[0]), row[1], row[2], std::stod(row[3]),
                            std::stod(row[4]), std::stoi(row[5])});
    }
    mysql_free_result(result);
    return true;
}

static void showInventory(const std::vector<StoreProduct>& products)
{
    std::cout << "\n+------+----------------------+----------------+-------+------------+------------+-------+\n"
              << "| ID   | Product              | Category       | Disc% | Base Price | Sale Price | Stock |\n"
              << "+------+----------------------+----------------+-------+------------+------------+-------+\n";
    for (const StoreProduct& product : products) {
        std::cout << "| " << std::left << std::setw(4) << product.id
                  << " | " << std::setw(20) << product.name
                  << " | " << std::setw(14) << product.category
                  << " | " << std::setw(5) << product.discountPercent
                  << " | " << std::right << std::setw(10) << product.basePrice
                  << " | " << std::setw(10) << product.sellingPrice()
                  << " | " << std::setw(5) << product.stock << " |\n";
    }
    std::cout << "+------+----------------------+----------------+-------+------------+------------+-------+\n";
}

static const StoreProduct* findProduct(const std::vector<StoreProduct>& products, int id)
{
    for (const StoreProduct& product : products)
        if (product.id == id) return &product;
    return nullptr;
}

static void printBill(const Cart& cart, double paid, const std::string& method)
{
    std::cout << "\n============================== BILL ==============================\n"
              << std::left << std::setw(6) << "ID" << std::setw(22) << "Product"
              << std::right << std::setw(8) << "Qty" << std::setw(8) << "Disc%"
              << std::setw(12) << "Price" << std::setw(14) << "Amount" << "\n"
              << "---------------------------------------------------------------------------\n";
    for (const CartItem& item : cart.items()) {
        std::cout << std::left << std::setw(6) << item.product.id
                  << std::setw(22) << item.product.name
                  << std::right << std::setw(8) << item.quantity
                  << std::setw(8) << item.product.discountPercent
                  << std::setw(12) << item.product.sellingPrice()
                  << std::setw(14) << item.product.sellingPrice() * item.quantity
                  << "\n";
    }
    std::cout << "---------------------------------------------------------------------------\n"
              << std::left << std::setw(46) << "Subtotal:"
              << std::right << std::setw(14) << cart.subtotal() << "\n"
              << std::left << std::setw(46) << "Coupon discount:"
              << std::right << std::setw(14) << cart.couponDiscount() << "\n"
              << std::left << std::setw(46) << "GST (18%):"
              << std::right << std::setw(14) << cart.gstAmount() << "\n"
              << std::left << std::setw(46) << "TOTAL:"
              << std::right << std::setw(14) << cart.total() << "\n"
              << std::left << std::setw(46) << "Payment method:"
              << std::right << std::setw(14) << method << "\n"
              << std::left << std::setw(46) << "Paid:"
              << std::right << std::setw(14) << paid << "\n"
              << std::left << std::setw(46) << "Change:"
              << std::right << std::setw(14) << paid - cart.total() << "\n"
              << "===================================================================\n";
}

static bool loadCoupon(Database& db, const std::string& code,
                       double& percent, double& flat)
{
    MYSQL_RES* result = db.query(
        "SELECT discount_percent,flat_amount FROM coupons WHERE active=TRUE AND coupon_code='" +
        db.escape(code) + "';");
    if (!result) return false;
    MYSQL_ROW row = mysql_fetch_row(result);
    const bool found = row != nullptr;
    if (found) {
        percent = std::stod(row[0]);
        flat = std::stod(row[1]);
    }
    mysql_free_result(result);
    return found;
}

static void showCategories(Database& db)
{
    MYSQL_RES* result = db.query(
        "SELECT category_name,discount_percent FROM categories ORDER BY category_name;");
    if (!result) return;
    std::cout << "\n+-----+----------------------+----------+\n"
              << "| No. | Category             | Discount |\n"
              << "+-----+----------------------+----------+\n";
    int number = 1;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr)
        std::cout << "| " << std::left << std::setw(3) << number++
                  << " | " << std::setw(20) << row[0]
                  << " | " << std::right << std::setw(7) << row[1] << "% |\n";
    std::cout << "+-----+----------------------+----------+\n";
    mysql_free_result(result);
}

static bool chooseCategory(Database& db, std::string& category)
{
    showCategories(db);
    std::cout << "Enter existing category name (or 0 to cancel): ";
    std::cin >> category;
    if (category == "0") return false;
    MYSQL_RES* result = db.query(
        "SELECT category_name FROM categories WHERE category_name='" +
        db.escape(category) + "';");
    if (!result) return false;
    const bool found = mysql_fetch_row(result) != nullptr;
    mysql_free_result(result);
    if (!found) std::cout << "Category not found.\n";
    return found;
}

static void showCoupons(Database& db)
{
    MYSQL_RES* result = db.query(
        "SELECT coupon_code,discount_percent,flat_amount,active "
        "FROM coupons WHERE active=TRUE ORDER BY coupon_code;");
    if (!result) return;
    std::cout << "\n+----------------+----------+------------+----------+\n"
              << "| Coupon Code    | Percent  | Flat       | Status   |\n"
              << "+----------------+----------+------------+----------+\n";
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr)
        std::cout << "| " << std::left << std::setw(14) << row[0]
                  << " | " << std::setw(8) << (std::string(row[1]) + "%")
                  << " | Rs " << std::setw(7) << row[2]
                  << " | " << std::setw(8)
                  << (std::string(row[3]) == "1" ? "Active" : "Inactive") << " |\n";
    std::cout << "+----------------+----------+------------+----------+\n";
    mysql_free_result(result);
}

static bool saveCart(Database& db, const Cart& cart, double paid,
                     const std::string& method)
{
    const double subtotal = cart.subtotal();
    const double total = cart.total();
    const double change = paid - total;
    if (!db.execute("START TRANSACTION;")) return false;
    if (!db.execute("INSERT INTO orders(total_amount,paid_amount,change_amount) VALUES(" +
                    std::to_string(total) + "," + std::to_string(paid) + "," +
                    std::to_string(change) + ");")) {
        db.execute("ROLLBACK;");
        return false;
    }
    const auto orderId = db.lastInsertId();
    double categoryDiscount = 0.0;
    for (const CartItem& item : cart.items()) {
        categoryDiscount += (item.product.basePrice - item.product.sellingPrice()) *
                            item.quantity;
        const std::string stockQuery =
            "UPDATE inventory SET stock_quantity=stock_quantity-" +
            std::to_string(item.quantity) + ",total_sold=total_sold+" +
            std::to_string(item.quantity) + " WHERE product_id=" +
            std::to_string(item.product.id) + " AND stock_quantity>=" +
            std::to_string(item.quantity) + ";";
        const double lineTotal = item.product.sellingPrice() * item.quantity;
        const std::string itemQuery =
            "INSERT INTO order_items(order_id,product_id,product_name,unit_price,quantity,line_total) VALUES(" +
            std::to_string(orderId) + "," + std::to_string(item.product.id) + ",'" +
            db.escape(item.product.name) + "'," + std::to_string(item.product.sellingPrice()) +
            "," + std::to_string(item.quantity) + "," + std::to_string(lineTotal) + ");";
        if (!db.execute(stockQuery) || db.affectedRows() != 1 ||
            !db.execute(itemQuery)) {
            db.execute("ROLLBACK;");
            return false;
        }
    }
    const std::string detailQuery =
        "INSERT INTO order_details VALUES(" + std::to_string(orderId) + "," +
        std::to_string(subtotal) + "," + std::to_string(categoryDiscount) + "," +
        std::to_string(cart.couponDiscount()) + ",18," +
        std::to_string(cart.gstAmount()) + ",'" + db.escape(method) + "');";
    const std::string transactionQuery =
        "INSERT INTO financial_transactions(transaction_type,order_id,amount,payment_method,account_name) "
        "VALUES('SALE'," + std::to_string(orderId) + "," + std::to_string(total) +
        ",'" + db.escape(method) + "','" +
        (method == "Cash Counter" ? "cash_counter" : "cash_bank") + "');";
    if (!db.execute(detailQuery) || !db.execute(transactionQuery) ||
        !db.execute("UPDATE accounts SET balance=balance+" + std::to_string(total) +
                    " WHERE account_name='" +
                    (method == "Cash Counter" ? "cash_counter" : "cash_bank") + "';") ||
        !db.execute("COMMIT;")) {
        db.execute("ROLLBACK;");
        return false;
    }
    printBill(cart, paid, method);
    std::cout << "Sale completed. Order ID: " << orderId << "\n";
    return true;
}

static void generateCart(Database& db, const std::vector<StoreProduct>& products)
{
    Cart cart;
    char more = 'y';
    while ((more == 'y' || more == 'Y') && !products.empty()) {
        showInventory(products);
        int choice, quantity;
        std::cout << "Product ID: ";
        if (!(std::cin >> choice)) {
            std::cout << "Invalid product ID.\n";
            clearInput();
            continue;
        }
        const StoreProduct* product = findProduct(products, choice);
        std::cout << "Quantity: ";
        if (product == nullptr || !(std::cin >> quantity) || quantity <= 0 ||
            quantity > product->stock) {
            std::cout << "Invalid product ID or quantity.\n";
            clearInput();
            continue;
        }
        cart.add(*product, quantity);
        std::cout << "Add another? (y/n): ";
        std::cin >> more;
    }
    if (cart.items().empty()) return;

    showCoupons(db);
    int couponChoice;
    std::cout << "Choose coupon number (0 for none): ";
    if (!(std::cin >> couponChoice)) return;
    std::string code;
    if (couponChoice > 0) {
        MYSQL_RES* result = db.query(
            "SELECT coupon_code FROM coupons WHERE active=TRUE ORDER BY coupon_code;");
        MYSQL_ROW row;
        int current = 1;
        while (result != nullptr && (row = mysql_fetch_row(result)) != nullptr) {
            if (current++ == couponChoice) code = row[0];
        }
        if (result != nullptr) mysql_free_result(result);
    }
    if (!code.empty()) {
        double percent, flat;
        if (loadCoupon(db, code, percent, flat)) cart.applyCoupon(percent, flat);
        else std::cout << "Coupon not found; continuing without it.\n";
    }
    cart.applyGst(18.0);
    std::cout << "Subtotal: Rs " << cart.subtotal()
              << "\nCoupon discount: Rs " << cart.couponDiscount()
              << "\nGST (18%): Rs " << cart.gstAmount()
              << "\nTotal: Rs " << cart.total() << "\n";
    int payment;
    double paid;
    std::cout << "Payment method (1 Cash Counter, 2 Online/UPI): ";
    if (!(std::cin >> payment) || payment < 1 || payment > 2) return;
    if (payment == 1) {
        std::cout << "Cash received: Rs ";
        if (!(std::cin >> paid) || paid < cart.total()) return;
    } else {
        paid = cart.total();
        std::cout << "Online payment received: Rs " << paid << "\n";
    }
    saveCart(db, cart, paid, payment == 1 ? "Cash Counter" : "Bank/UPI");
}

static void showBills(Database& db)
{
    MYSQL_RES* result = db.query(
        "SELECT o.order_id,o.total_amount,SUM(i.quantity),o.created_at "
        "FROM orders o JOIN order_items i ON i.order_id=o.order_id "
        "GROUP BY o.order_id ORDER BY o.order_id DESC;");
    if (!result) return;
    MYSQL_ROW row;
    std::cout << "\n+----------+--------------+-------+---------------------+\n"
              << "| Order ID | Bill Amount  | Items | Date                |\n"
              << "+----------+--------------+-------+---------------------+\n";
    while ((row = mysql_fetch_row(result)) != nullptr)
        std::cout << "| " << std::left << std::setw(8) << row[0]
                  << " | " << std::setw(12) << row[1]
                  << " | " << std::setw(5) << row[2]
                  << " | " << std::setw(19) << row[3] << " |\n";
    std::cout << "+----------+--------------+-------+---------------------+\n";
    mysql_free_result(result);
    int orderId;
    std::cout << "Enter Order ID for full details (0 to return): ";
    if (!(std::cin >> orderId) || orderId == 0) {
        clearInput();
        return;
    }
    result = db.query(
        "SELECT o.paid_amount,o.change_amount,d.payment_method,d.subtotal,"
        "d.category_discount,d.coupon_discount,d.gst_amount,o.total_amount "
        "FROM orders o JOIN order_details d ON d.order_id=o.order_id WHERE o.order_id=" +
        std::to_string(orderId) + ";");
    if (!result) return;
    row = mysql_fetch_row(result);
    if (row != nullptr) {
        std::cout << "\nPayment summary\n"
                  << "+--------------------+----------------+\n"
                  << "| Field              | Value          |\n"
                  << "+--------------------+----------------+\n"
                  << "| Payment method     | " << std::left << std::setw(14) << row[2] << " |\n"
                  << "| Subtotal           | Rs " << std::right << std::setw(11) << row[3] << " |\n"
                  << "| Category discount  | Rs " << std::setw(11) << row[4] << " |\n"
                  << "| Coupon discount    | Rs " << std::setw(11) << row[5] << " |\n"
                  << "| GST                | Rs " << std::setw(11) << row[6] << " |\n"
                  << "| Total              | Rs " << std::setw(11) << row[7] << " |\n"
                  << "| Amount paid        | Rs " << std::setw(11) << row[0] << " |\n"
                  << "| Change returned    | Rs " << std::setw(11) << row[1] << " |\n"
                  << "+--------------------+----------------+\n";
    }
    mysql_free_result(result);
    result = db.query(
        "SELECT product_id,product_name,quantity,unit_price,line_total "
        "FROM order_items WHERE order_id=" + std::to_string(orderId) + ";");
    if (!result) return;
    std::cout << "\nOrder " << orderId << " details\n"
              << "+------+----------------------+-------+------------+------------+\n"
              << "| ID   | Product              | Qty   | Unit Price | Amount     |\n"
              << "+------+----------------------+-------+------------+------------+\n";
    while ((row = mysql_fetch_row(result)) != nullptr)
        std::cout << "| " << std::left << std::setw(4) << row[0]
                  << " | " << std::setw(20) << row[1]
                  << " | " << std::setw(5) << row[2]
                  << " | " << std::setw(10) << row[3]
                  << " | " << std::setw(10) << row[4] << " |\n";
    std::cout << "+------+----------------------+-------+------------+------------+\n";
    mysql_free_result(result);
}

static void manageCoupons(Database& db)
{
    int choice;
    while (true) {
    std::cout << "\n1. View Existing Coupons\n2. Add New Coupon\n"
              << "3. Update Existing Coupon\n4. Delete Coupon\n0. Return\nChoose: ";
    if (!(std::cin >> choice)) return;
    if (choice == 0) return;
    if (choice == 1) {
        showCoupons(db);
        continue;
    }
    std::string code;
    std::cout << "Coupon code: ";
    std::cin >> code;
    if (choice == 3 || choice == 4) {
        MYSQL_RES* current = db.query(
            "SELECT discount_percent,flat_amount,active FROM coupons WHERE coupon_code='" +
            db.escape(code) + "';");
        if (!current) return;
        MYSQL_ROW row = mysql_fetch_row(current);
        if (!row) {
            mysql_free_result(current);
            std::cout << "Coupon not found.\n";
            return;
        }
        std::cout << "Current coupon: " << row[0] << "%, Rs " << row[1]
                  << ", " << (std::string(row[2]) == "1" ? "Active" : "Inactive") << "\n";
        mysql_free_result(current);
    }
    if (choice == 4) {
        if (db.execute("DELETE FROM coupons WHERE coupon_code='" +
                       db.escape(code) + "';"))
            std::cout << "Coupon deleted.\n";
        continue;
    }
    double percent, flat;
    std::cout << "Percentage discount: ";
    if (!(std::cin >> percent)) return;
    std::cout << "Flat discount in Rs: ";
    if (!(std::cin >> flat) || percent < 0 || flat < 0) return;
    if (CouponManager::save(db, code, percent, flat))
        std::cout << (choice == 2 ? "Coupon added.\n" : "Coupon updated.\n");
    }
}

static void manageInventory(Database& db, std::vector<StoreProduct>& products)
{
    int choice;
    while (true) {
    std::cout << "\n1. Manage Products\n2. Manage Categories\n3. View Low Stock Products\n"
              << "0. Return\nChoose: ";
    if (!(std::cin >> choice)) return;
    if (choice == 0) return;
    if (choice == 1) {
        std::cout << "Product options: 1 View  2 Add  3 Update  4 Delete\nChoose: ";
        int action;
        if (!(std::cin >> action)) continue;
        choice = action == 1 ? 1 : action == 2 ? 3 : action == 3 ? 4 : 5;
    } else if (choice == 2) {
        std::cout << "Category options: 1 View  2 Add  3 Update Discount  4 Delete\nChoose: ";
        int action;
        if (!(std::cin >> action)) continue;
        choice = action == 1 ? 6 : action == 2 ? 7 : action == 3 ? 8 : 9;
    } else if (choice == 3) {
        choice = 2;
    }
    if (choice == 1) {
        showInventory(products);
        continue;
    }
    if (choice == 2) {
        int limit;
        std::cout << "Show products with stock at or below: ";
        if (!(std::cin >> limit)) continue;
        std::cout << "\nLow stock products:\n";
        for (const StoreProduct& product : products)
            if (product.stock <= limit)
                std::cout << "ID " << product.id << " | " << product.name
                          << " | Stock: " << product.stock << "\n";
        continue;
    }
    if (choice == 5) {
        int id;
        std::cout << "Product ID to delete: ";
        if (!(std::cin >> id)) continue;
        const StoreProduct* product = findProduct(products, id);
        if (product == nullptr) {
            std::cout << "Product ID not found.\n";
            continue;
        }
        std::cout << "Delete " << product->name << " (ID " << id << ")? (y/n): ";
        char confirm;
        std::cin >> confirm;
        if ((confirm == 'y' || confirm == 'Y') &&
            db.execute("DELETE FROM inventory WHERE product_id=" +
                       std::to_string(id) + ";"))
            std::cout << "Product deleted.\n";
        continue;
    }
    if (choice == 3) {
        int id, stock;
        double price;
        std::string name, category;
        std::cout << "Product ID: "; std::cin >> id;
        std::cout << "Product name: "; std::cin >> std::ws; std::getline(std::cin, name);
        if (!chooseCategory(db, category)) continue;
        std::cout << "Price: "; std::cin >> price;
        std::cout << "Opening stock: "; std::cin >> stock;
        MYSQL_RES* conflict = db.query(
            "SELECT product_id FROM inventory WHERE product_id=" +
            std::to_string(id) + " OR name='" + db.escape(name) + "';");
        const bool exists = conflict != nullptr && mysql_fetch_row(conflict) != nullptr;
        if (conflict != nullptr) mysql_free_result(conflict);
        if (exists) {
            std::cout << "Product ID or name already exists.\n";
            continue;
        }
        if (db.execute("INSERT INTO inventory(product_id,name,base_price,category,stock_quantity) VALUES(" +std::to_string(id) + ",'" + db.escape(name) + "'," +std::to_string(price) + ",'" + db.escape(category) + "'," +std::to_string(stock) + ");"))std::cout << "Product added.\n";
    } else if (choice == 4) {
        int id;
        std::cout << "Available products:\n";
        showInventory(products);
        std::cout << "Product ID to update: ";
        if (!(std::cin >> id)) continue;
        const StoreProduct* product = findProduct(products, id);
        if (product == nullptr) {
            std::cout << "Product ID not found.\n";
            continue;
        }
        std::cout << "Current: " << product->name << ", Rs " << product->basePrice
                  << ", stock " << product->stock << "\n";
        int field = -1;
        while (field != 0) {
            std::cout << "1. Change price\n2. Change quantity\n3. Change name\n"
                      << "4. Change category\n0. Finish\nChoose: ";
            if (!(std::cin >> field)) return;
            if (field == 1) {
                double price;
                std::cout << "New price: ";
                if (std::cin >> price && price >= 0)
                    db.execute("UPDATE inventory SET base_price=" +
                               std::to_string(price) + " WHERE product_id=" +
                               std::to_string(id) + ";");
            } else if (field == 2) {
                int stock;
                std::cout << "New quantity: ";
                if (std::cin >> stock && stock >= 0)
                    db.execute("UPDATE inventory SET stock_quantity=" +
                               std::to_string(stock) + " WHERE product_id=" +
                               std::to_string(id) + ";");
            } else if (field == 3) {
                std::string name;
                std::cout << "New name: "; std::cin >> std::ws; std::getline(std::cin, name);
                MYSQL_RES* conflict = db.query(
                    "SELECT product_id FROM inventory WHERE name='" + db.escape(name) +
                    "' AND product_id<>" + std::to_string(id) + ";");
                const bool exists = conflict != nullptr && mysql_fetch_row(conflict) != nullptr;
                if (conflict != nullptr) mysql_free_result(conflict);
                if (!exists) db.execute("UPDATE inventory SET name='" + db.escape(name) +
                                        "' WHERE product_id=" + std::to_string(id) + ";");
                else std::cout << "Product name already exists.\n";
            } else if (field == 4) {
                std::string category;
                if (chooseCategory(db, category))
                    db.execute("UPDATE inventory SET category='" + db.escape(category) +
                               "' WHERE product_id=" + std::to_string(id) + ";");
            }
        }
    } else if (choice == 6) {
        showCategories(db);
    } else if (choice == 7) {
        std::string name;
        double discount;
        std::cout << "New category name: "; std::cin >> name;
        std::cout << "Category discount percentage: ";
        if (!(std::cin >> discount) || discount < 0) continue;
        MYSQL_RES* conflict = db.query(
            "SELECT category_name FROM categories WHERE category_name='" + db.escape(name) + "';");
        const bool exists = conflict != nullptr && mysql_fetch_row(conflict) != nullptr;
        if (conflict != nullptr) mysql_free_result(conflict);
        if (exists) {
            std::cout << "Category already exists.\n";
            continue;
        }
        if (db.execute("INSERT INTO categories(category_name,discount_percent) VALUES('" +
                       db.escape(name) + "'," + std::to_string(discount) + ");"))
            std::cout << "Category added.\n";
    } else if (choice == 8) {
        showCategories(db);
        std::string name;
        double discount;
        std::cout << "Category name to update: "; std::cin >> name;
        MYSQL_RES* result = db.query(
            "SELECT discount_percent FROM categories WHERE category_name='" +
            db.escape(name) + "';");
        if (!result) continue;
        MYSQL_ROW row = mysql_fetch_row(result);
        if (!row) {
            mysql_free_result(result);
            std::cout << "Category not found.\n";
            continue;
        }
        std::cout << "Current discount: " << row[0] << "%\n";
        mysql_free_result(result);
        std::cout << "New discount percentage: ";
        if (std::cin >> discount && discount >= 0 &&
            CategoryManager::updateDiscount(db, name, discount))
            std::cout << "Category discount updated.\n";
    } else if (choice == 9) {
        showCategories(db);
        std::string name;
        std::cout << "Category name to delete: "; std::cin >> name;
        if (db.execute("DELETE FROM categories WHERE category_name='" +
                       db.escape(name) + "';"))
            std::cout << "Category deleted.\n";
    }
    loadProducts(db, products);
    }
}

int main()
{
    std::cout << std::fixed << std::setprecision(2);
    Database db;
    if (!db.connect() || !initialize(db)) return 1;
    std::vector<StoreProduct> products;
    if (!loadProducts(db, products)) return 1;

    int choice = -1;
    while (choice != 0) {
        std::cout << "\n1. Generate New Cart\n2. Manage Inventory\n3. Restock Products\n"
                  << "4. Manage Coupons\n5. View Previous Bills\n0. Exit\nChoose: ";
        if (!(std::cin >> choice)) {
            if (std::cin.eof()) break;
            clearInput();
            continue;
        }
        if (choice == 1) generateCart(db, products);
        else if (choice == 2) manageInventory(db, products);
        else if (choice == 3) {
            int id = 0, quantity = 0;
            std::cout << "Product ID: ";
            if (std::cin >> id) {
                std::cout << "Quantity to add: ";
                std::cin >> quantity;
            }
            if (quantity > 0)
                db.execute("UPDATE inventory SET stock_quantity=stock_quantity+" +
                           std::to_string(quantity) + " WHERE product_id=" +
                           std::to_string(id) + ";");
        } else if (choice == 4) manageCoupons(db);
        else if (choice == 5) showBills(db);
        loadProducts(db, products);
    }
    return 0;
}
