#include <mysql.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "database.h"
#include "categories.h"
#include "product.h"
#include "ui_engine.h"

static const char *schema[] = {
    "CREATE TABLE IF NOT EXISTS inventory ("
    "product_id INT PRIMARY KEY,name VARCHAR(100) NOT NULL,"
    "base_price DECIMAL(10,2) NOT NULL CHECK(base_price>=0),"
    "category VARCHAR(50) NOT NULL,stock_quantity INT NOT NULL DEFAULT 0,"
    "total_sold INT NOT NULL DEFAULT 0,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);",
    "CREATE TABLE IF NOT EXISTS categories (category_name VARCHAR(50) PRIMARY KEY,"
    "discount_percent DECIMAL(5,2) NOT NULL DEFAULT 0);",
    "CREATE TABLE IF NOT EXISTS coupons (coupon_code VARCHAR(30) PRIMARY KEY,"
    "discount_percent DECIMAL(5,2) NOT NULL DEFAULT 0,flat_amount DECIMAL(10,2) NOT NULL DEFAULT 0,"
    "active BOOLEAN NOT NULL DEFAULT TRUE);",
    "CREATE TABLE IF NOT EXISTS orders (order_id INT AUTO_INCREMENT PRIMARY KEY,"
    "total_amount DECIMAL(10,2) NOT NULL,paid_amount DECIMAL(10,2) NOT NULL,"
    "change_amount DECIMAL(10,2) NOT NULL,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);",
    "CREATE TABLE IF NOT EXISTS order_items (order_item_id INT AUTO_INCREMENT PRIMARY KEY,"
    "order_id INT NOT NULL,product_id INT NOT NULL,product_name VARCHAR(100) NOT NULL,"
    "unit_price DECIMAL(10,2) NOT NULL,quantity INT NOT NULL,line_total DECIMAL(10,2) NOT NULL,"
    "FOREIGN KEY(order_id) REFERENCES orders(order_id),FOREIGN KEY(product_id) REFERENCES inventory(product_id));",
    "CREATE TABLE IF NOT EXISTS order_details (order_id INT PRIMARY KEY,subtotal DECIMAL(10,2) NOT NULL,"
    "category_discount DECIMAL(10,2) NOT NULL,coupon_discount DECIMAL(10,2) NOT NULL,"
    "gst_percent DECIMAL(5,2) NOT NULL,gst_amount DECIMAL(10,2) NOT NULL,"
    "payment_method VARCHAR(30) NOT NULL,FOREIGN KEY(order_id) REFERENCES orders(order_id));",
    "CREATE TABLE IF NOT EXISTS accounts (account_name VARCHAR(30) PRIMARY KEY,"
    "balance DECIMAL(12,2) NOT NULL DEFAULT 0);",
    "CREATE TABLE IF NOT EXISTS financial_transactions (transaction_id INT AUTO_INCREMENT PRIMARY KEY,"
    "transaction_type VARCHAR(20) NOT NULL,order_id INT NULL,amount DECIMAL(12,2) NOT NULL,"
    "payment_method VARCHAR(30) NOT NULL,account_name VARCHAR(30) NOT NULL,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,FOREIGN KEY(order_id) REFERENCES orders(order_id));",
    "INSERT IGNORE INTO accounts(account_name,balance) VALUES('cash_counter',0),('cash_bank',0);",
    "INSERT IGNORE INTO categories(category_name,discount_percent) VALUES"
    "('Book',10),('Accessory',0),('Stationery',5);",
    "UPDATE categories SET discount_percent=0 WHERE category_name='Accessory' "
    "AND discount_percent<0;",
    "INSERT IGNORE INTO inventory(product_id,name,base_price,category,stock_quantity) VALUES"
    "(101,'C++ Textbook',500,'Book',20),(202,'USB Mouse',800,'Accessory',15),"
    "(303,'Pen Set',200,'Stationery',50);"
};

static int initialize(MYSQL *db)
{
    size_t i;
    for (i = 0; i < sizeof(schema) / sizeof(schema[0]); ++i)
        if (!db_execute(db, schema[i])) return 0;
    return 1;
}

static void show_inventory(const Product products[], int count)
{
    int i;
    printf("\n+------+----------------------+----------------+-------+------------+------------+-------+\n"
           "| ID   | Product              | Category       | Disc%% | Base Price | Sale Price | Stock |\n"
           "+------+----------------------+----------------+-------+------------+------------+-------+\n");
    for (i = 0; i < count; ++i)
        printf("| %-4d | %-20s | %-14s | %5.2f | %10.2f | %10.2f | %5d |\n",
               products[i].id, products[i].name, products[i].category,
               products[i].discount, products[i].price, selling_price(&products[i]),
               products[i].stock);
    printf("+------+----------------------+----------------+-------+------------+------------+-------+\n");
}

static const Product *find_product(const Product products[], int count, int id)
{
    int i;
    for (i = 0; i < count; ++i)
        if (products[i].id == id) return &products[i];
    return NULL;
}

static void show_categories(MYSQL *db)
{
    MYSQL_RES *result = db_query(db,
        "SELECT category_name,discount_percent FROM categories ORDER BY category_name;");
    MYSQL_ROW row;
    int number = 1;
    if (!result) return;
    printf("\n+-----+----------------------+----------+\n"
           "| No. | Category             | Discount |\n"
           "+-----+----------------------+----------+\n");
    while ((row = mysql_fetch_row(result)) != NULL)
        printf("| %-3d | %-20s | %7s%% |\n", number++, row[0], row[1]);
    printf("+-----+----------------------+----------+\n");
    mysql_free_result(result);
}

static int choose_category(MYSQL *db, char category[], size_t size)
{
    MYSQL_RES *result;
    MYSQL_ROW row;
    show_categories(db);
    printf("Enter existing category name (or 0 to cancel): ");
    if (scanf(" %49[^\n]", category) != 1 || strcmp(category, "0") == 0) return 0;
    {
        char sql[256];
        snprintf(sql, sizeof(sql),
                 "SELECT category_name FROM categories WHERE category_name='%s';",
                 category);
        result = db_query(db, sql);
    }
    if (!result) return 0;
    row = mysql_fetch_row(result);
    mysql_free_result(result);
    if (!row) {
        printf("Category not found.\n");
        return 0;
    }
    (void)size;
    return 1;
}

static void show_coupons(MYSQL *db)
{
    MYSQL_RES *result = db_query(db,
        "SELECT coupon_code,discount_percent,flat_amount,active "
        "FROM coupons WHERE active=TRUE ORDER BY coupon_code;");
    MYSQL_ROW row;
    if (!result) return;
    printf("\n+----------------+----------+------------+----------+\n"
           "| Coupon Code    | Percent  | Flat       | Status   |\n"
           "+----------------+----------+------------+----------+\n");
    while ((row = mysql_fetch_row(result)) != NULL)
        printf("| %-14s | %7s%% | Rs %-7s | %-8s |\n",
               row[0], row[1], row[2],
               strcmp(row[3], "1") == 0 ? "Active" : "Inactive");
    printf("+----------------+----------+------------+----------+\n");
    mysql_free_result(result);
}

static void print_bill(const Product products[], const int ids[],
                       const int quantities[], int item_count,
                       double subtotal, double coupon_discount, double gst,
                       double total, double paid, const char *method)
{
    int i;
    printf("\n============================== BILL ==============================\n"
           "%-6s %-22s %8s %8s %12s %14s\n"
           "---------------------------------------------------------------------------\n",
           "ID", "Product", "Qty", "Disc%", "Price", "Amount");
    for (i = 0; i < item_count; ++i) {
        const Product *p = &products[ids[i]];
        printf("%-6d %-22s %8d %8.2f %12.2f %14.2f\n",
               p->id, p->name, quantities[i], p->discount,
               selling_price(p), selling_price(p) * quantities[i]);
    }
    printf("---------------------------------------------------------------------------\n"
           "%-46s %14.2f\n%-46s %14.2f\n%-46s %14.2f\n"
           "%-46s %14.2f\n%-46s %14s\n%-46s %14.2f\n%-46s %14.2f\n"
           "===================================================================\n",
           "Subtotal:", subtotal, "Coupon discount:", coupon_discount,
           "GST (18%):", gst, "TOTAL:", total, "Payment method:", method,
           "Paid:", paid, "Change:", paid - total);
}

static void generate_cart(MYSQL *db, const Product products[], int count)
{
    int ids[MAX_PRODUCTS], quantities[MAX_PRODUCTS], item_count = 0;
    double subtotal = 0.0, coupon_discount = 0.0;
    char more = 'y';
    while ((more == 'y' || more == 'Y') && item_count < MAX_PRODUCTS) {
        int choice, quantity;
        show_inventory(products, count);
        printf("Product ID: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid product ID.\n");
            ui_clear_input();
            continue;
        }
        {
            const Product *product = find_product(products, count, choice);
            printf("Quantity: ");
            if (product == NULL || scanf("%d", &quantity) != 1 ||
                quantity <= 0 || quantity > product->stock) {
                printf("Invalid product ID or quantity.\n");
                ui_clear_input();
                continue;
            }
            ids[item_count] = (int)(product - products);
        }
        quantities[item_count] = quantity;
        subtotal += selling_price(&products[ids[item_count]]) * quantity;
        ++item_count;
        printf("Add another? (y/n): ");
        scanf(" %c", &more);
    }
    if (item_count == 0) return;

    {
        char code[40] = "", sql[512], method[30];
        double gst, total, paid = 0.0;
        int payment;
        long long order_id;
        int coupon_choice;
        show_coupons(db);
        printf("Choose coupon number (0 for none): ");
        if (scanf("%d", &coupon_choice) != 1) return;
        if (coupon_choice > 0) {
            MYSQL_RES *coupon_list = db_query(db,
                "SELECT coupon_code FROM coupons WHERE active=TRUE ORDER BY coupon_code;");
            MYSQL_ROW coupon_row;
            int current = 1;
            while (coupon_list != NULL &&
                   (coupon_row = mysql_fetch_row(coupon_list)) != NULL) {
                if (current++ == coupon_choice)
                    snprintf(code, sizeof(code), "%s", coupon_row[0]);
            }
            if (coupon_list != NULL) mysql_free_result(coupon_list);
        }
        if (code[0] != '\0') {
            {
                double percent, flat;
                if (coupon_find(db, code, &percent, &flat)) {
                    coupon_discount = subtotal * percent / 100.0 + flat;
                    if (coupon_discount > subtotal) coupon_discount = subtotal;
                } else {
                    printf("Coupon not found; continuing without it.\n");
                }
            }
        }
        gst = (subtotal - coupon_discount) * 0.18;
        total = subtotal - coupon_discount + gst;
        printf("Subtotal: Rs %.2f\nCoupon discount: Rs %.2f\n"
               "GST (18%%): Rs %.2f\nTotal: Rs %.2f\n",
               subtotal, coupon_discount, gst, total);
        printf("Payment method (1 Cash Counter, 2 Online/UPI): ");
        if (scanf("%d", &payment) != 1 || payment < 1 || payment > 2) return;
        if (payment == 1) {
            printf("Cash received: Rs ");
            if (scanf("%lf", &paid) != 1 || paid < total) return;
        } else {
            paid = total;
            printf("Online payment received: Rs %.2f\n", paid);
        }
        snprintf(method, sizeof(method), "%s",
                 payment == 1 ? "Cash Counter" : "Bank/UPI");
        if (!db_execute(db, "START TRANSACTION;")) return;
        snprintf(sql, sizeof(sql),
                 "INSERT INTO orders(total_amount,paid_amount,change_amount)"
                 "VALUES(%.2f,%.2f,%.2f);", total, paid, paid - total);
        if (!db_execute(db, sql)) {
            db_execute(db, "ROLLBACK;");
            return;
        }
        order_id = (long long)mysql_insert_id(db);
        {
            int i;
            double category_discount = 0.0;
            for (i = 0; i < item_count; ++i) {
                const Product *p = &products[ids[i]];
                const double price = selling_price(p);
                category_discount += (p->price - price) * quantities[i];
                snprintf(sql, sizeof(sql),
                    "UPDATE inventory SET stock_quantity=stock_quantity-%d,"
                    "total_sold=total_sold+%d WHERE product_id=%d AND stock_quantity>=%d;",
                    quantities[i], quantities[i], p->id, quantities[i]);
                if (!db_execute(db, sql) || mysql_affected_rows(db) != 1) {
                    db_execute(db, "ROLLBACK;");
                    return;
                }
                snprintf(sql, sizeof(sql),
                    "INSERT INTO order_items(order_id,product_id,product_name,"
                    "unit_price,quantity,line_total) VALUES(%lld,%d,'%s',%.2f,%d,%.2f);",
                    order_id, p->id, p->name, price, quantities[i],
                    price * quantities[i]);
                if (!db_execute(db, sql)) {
                    db_execute(db, "ROLLBACK;");
                    return;
                }
            }
            snprintf(sql, sizeof(sql),
                "INSERT INTO order_details VALUES(%lld,%.2f,%.2f,%.2f,18,%.2f,'%s');",
                order_id, subtotal, category_discount, coupon_discount, gst, method);
            if (!db_execute(db, sql)) return;
            snprintf(sql, sizeof(sql),
                "INSERT INTO financial_transactions(transaction_type,order_id,amount,"
                "payment_method,account_name) VALUES('SALE',%lld,%.2f,'%s','%s');",
                order_id, total, method,
                payment == 1 ? "cash_counter" : "cash_bank");
            if (!db_execute(db, sql)) return;
            snprintf(sql, sizeof(sql),
                "UPDATE accounts SET balance=balance+%.2f WHERE account_name='%s';",
                total, payment == 1 ? "cash_counter" : "cash_bank");
            if (!db_execute(db, sql) || !db_execute(db, "COMMIT;")) return;
        }
        print_bill(products, ids, quantities, item_count, subtotal,
                   coupon_discount, gst, total, paid, method);
        printf("Sale completed. Order ID: %lld\n", order_id);
    }
}

static void show_bills(MYSQL *db)
{
    MYSQL_RES *result = db_query(db,
        "SELECT o.order_id,o.total_amount,SUM(i.quantity),o.created_at "
        "FROM orders o JOIN order_items i ON i.order_id=o.order_id "
        "GROUP BY o.order_id ORDER BY o.order_id DESC;");
    MYSQL_ROW row;
    if (!result) return;
    printf("\n+----------+--------------+-------+---------------------+\n"
           "| Order ID | Bill Amount  | Items | Date                |\n"
           "+----------+--------------+-------+---------------------+\n");
    while ((row = mysql_fetch_row(result)) != NULL)
        printf("| %-8s | %-12s | %-5s | %-19s |\n",
               row[0], row[1], row[2], row[3]);
    printf("+----------+--------------+-------+---------------------+\n");
    mysql_free_result(result);
    {
        int order_id;
        printf("Enter Order ID for full details (0 to return): ");
        if (scanf("%d", &order_id) != 1 || order_id == 0) {
            ui_clear_input();
            return;
        }
        {
            char sql[256];
            MYSQL_RES *payment_result;
            MYSQL_ROW payment_row;
            snprintf(sql, sizeof(sql),
                     "SELECT o.paid_amount,o.change_amount,d.payment_method,"
                     "d.subtotal,d.category_discount,d.coupon_discount,d.gst_amount,"
                     "o.total_amount FROM orders o JOIN order_details d "
                     "ON d.order_id=o.order_id WHERE o.order_id=%d;", order_id);
            payment_result = db_query(db, sql);
            if (!payment_result) return;
            payment_row = mysql_fetch_row(payment_result);
            if (payment_row != NULL) {
                printf("\nPayment summary\n"
                       "+--------------------+----------------+\n"
                       "| Field              | Value          |\n"
                       "+--------------------+----------------+\n"
                       "| %-18s | %-14s |\n"
                       "| %-18s | Rs %11s |\n"
                       "| %-18s | Rs %11s |\n"
                       "| %-18s | Rs %11s |\n"
                       "| %-18s | Rs %11s |\n"
                       "| %-18s | Rs %11s |\n"
                       "| %-18s | Rs %11s |\n"
                       "| %-18s | Rs %11s |\n"
                       "+--------------------+----------------+\n",
                       "Payment method", payment_row[2],
                       "Subtotal", payment_row[3],
                       "Category discount", payment_row[4],
                       "Coupon discount", payment_row[5],
                       "GST", payment_row[6],
                       "Total", payment_row[7],
                       "Amount paid", payment_row[0],
                       "Change returned", payment_row[1]);
            }
            mysql_free_result(payment_result);
            snprintf(sql, sizeof(sql),
                     "SELECT product_id,product_name,quantity,unit_price,line_total "
                     "FROM order_items WHERE order_id=%d;", order_id);
            result = db_query(db, sql);
            if (!result) return;
            printf("\nOrder %d details\n"
                   "+------+----------------------+-------+------------+------------+\n"
                   "| ID   | Product              | Qty   | Unit Price | Amount     |\n"
                   "+------+----------------------+-------+------------+------------+\n",
                   order_id);
            while ((row = mysql_fetch_row(result)) != NULL)
                printf("| %-4s | %-20s | %-5s | %-10s | %-10s |\n",
                       row[0], row[1], row[2], row[3], row[4]);
            printf("+------+----------------------+-------+------------+------------+\n");
            mysql_free_result(result);
        }
    }
}

static void manage_coupons(MYSQL *db)
{
    int choice;
    char code[40], sql[512];
    double percent, flat;
    while (1) {
    printf("\n1. View Existing Coupons\n2. Add New Coupon\n"
           "3. Update Existing Coupon\n4. Delete Coupon\n0. Return\nChoose: ");
    if (scanf("%d", &choice) != 1) return;
    if (choice == 0) return;
    if (choice == 1) {
        show_coupons(db);
        continue;
    }
    printf("Coupon code: ");
    if (scanf("%39s", code) != 1) return;
    if (choice == 3 || choice == 4) {
        MYSQL_RES *current;
        MYSQL_ROW row;
        snprintf(sql, sizeof(sql),
                 "SELECT discount_percent,flat_amount,active FROM coupons "
                 "WHERE coupon_code='%s';", code);
        current = db_query(db, sql);
        if (!current) return;
        row = mysql_fetch_row(current);
        if (!row) {
            mysql_free_result(current);
            printf("Coupon not found.\n");
            return;
        }
        printf("Current coupon: %s%%, Rs %s, %s\n", row[0], row[1],
               strcmp(row[2], "1") == 0 ? "Active" : "Inactive");
        mysql_free_result(current);
    }
    if (choice == 4) {
        snprintf(sql, sizeof(sql),
                 "DELETE FROM coupons WHERE coupon_code='%s';", code);
        if (db_execute(db, sql)) printf("Coupon deleted.\n");
        continue;
    }
    printf("Percentage discount: ");
    if (scanf("%lf", &percent) != 1) return;
    printf("Flat discount in Rs: ");
    if (scanf("%lf", &flat) != 1 || percent < 0 || flat < 0) return;
    snprintf(sql, sizeof(sql),
             "INSERT INTO coupons(coupon_code,discount_percent,flat_amount,active)"
             " VALUES('%s',%.2f,%.2f,TRUE) ON DUPLICATE KEY UPDATE "
             "discount_percent=%.2f,flat_amount=%.2f,active=TRUE;",
             code, percent, flat, percent, flat);
    if (db_execute(db, sql))
        printf(choice == 2 ? "Coupon added.\n" : "Coupon updated.\n");
    }
}

static void manage_products(MYSQL *db, Product products[], int *count)
{
    int choice;
    while (1) {
    printf("\n1. Manage Products\n2. Manage Categories\n3. View Low Stock Products\n"
           "0. Return\nChoose: ");
    if (scanf("%d", &choice) != 1) return;
    if (choice == 0) return;
    if (choice == 1) {
        int action;
        printf("Product options: 1 View  2 Add  3 Update  4 Delete\nChoose: ");
        if (scanf("%d", &action) != 1) continue;
        choice = action == 1 ? 1 : action == 2 ? 3 : action == 3 ? 4 : 5;
    } else if (choice == 2) {
        int action;
        printf("Category options: 1 View  2 Add  3 Update Discount  4 Delete\nChoose: ");
        if (scanf("%d", &action) != 1) continue;
        choice = action == 1 ? 6 : action == 2 ? 7 : action == 3 ? 8 : 9;
    } else if (choice == 3) {
        choice = 2;
    }
    if (choice == 1) {
        show_inventory(products, *count);
        continue;
    }
    if (choice == 2) {
        int limit, i;
        printf("Show products with stock at or below: ");
        if (scanf("%d", &limit) != 1) return;
        printf("\nLow stock products:\n");
        for (i = 0; i < *count; ++i)
            if (products[i].stock <= limit)
                printf("ID %d | %s | Stock: %d\n",
                       products[i].id, products[i].name, products[i].stock);
        continue;
    }
    if (choice == 5) {
        int id;
        char sql[256];
        printf("Product ID to delete: ");
        if (scanf("%d", &id) == 1) {
            const Product *product = find_product(products, *count, id);
            char confirm;
            if (product == NULL) {
                printf("Product ID not found.\n");
                continue;
            }
            printf("Delete %s (ID %d)? (y/n): ", product->name, id);
            scanf(" %c", &confirm);
            if (confirm != 'y' && confirm != 'Y') return;
            snprintf(sql, sizeof(sql),
                     "DELETE FROM inventory WHERE product_id=%d;", id);
            if (db_execute(db, sql)) printf("Product deleted.\n");
        }
        continue;
    }
    if (choice == 3) {
        int id, stock;
        double price;
        char name[MAX_NAME], category[MAX_CATEGORY], sql[512];
        printf("Product ID: "); scanf("%d", &id);
        printf("Name: "); scanf(" %99[^\n]", name);
        if (!choose_category(db, category, sizeof(category))) return;
        printf("Price: "); scanf("%lf", &price);
        printf("Opening stock: "); scanf("%d", &stock);
        snprintf(sql, sizeof(sql),
                 "SELECT product_id FROM inventory WHERE product_id=%d OR name='%s';",
                 id, name);
        {
            MYSQL_RES *conflict = db_query(db, sql);
            int exists = conflict != NULL && mysql_fetch_row(conflict) != NULL;
            if (conflict != NULL) mysql_free_result(conflict);
            if (exists) {
                printf("Product ID or name already exists.\n");
                continue;
            }
        }
        snprintf(sql, sizeof(sql),
                 "INSERT INTO inventory(product_id,name,base_price,category,stock_quantity)"
                 " VALUES(%d,'%s',%.2f,'%s',%d);", id, name, price, category, stock);
        if (db_execute(db, sql)) printf("Product added.\n");
    } else if (choice == 4) {
        int id, field;
        double price;
        int stock;
        char sql[512];
        show_inventory(products, *count);
        printf("Product ID to update: ");
        if (scanf("%d", &id) != 1) return;
        {
            const Product *product = find_product(products, *count, id);
            if (product == NULL) {
                printf("Product ID not found.\n");
                continue;
            }
            printf("Current: %s, Rs %.2f, stock %d\n",
                   product->name, product->price, product->stock);
        }
        do {
            printf("1. Change price\n2. Change quantity\n3. Change name\n"
                   "4. Change category\n0. Finish\nChoose: ");
            if (scanf("%d", &field) != 1) return;
            if (field == 1) {
                printf("New price: ");
                if (scanf("%lf", &price) == 1 && price >= 0) {
                    snprintf(sql, sizeof(sql),
                             "UPDATE inventory SET base_price=%.2f WHERE product_id=%d;",
                             price, id);
                    db_execute(db, sql);
                }
            } else if (field == 2) {
                printf("New quantity: ");
                if (scanf("%d", &stock) == 1 && stock >= 0) {
                    snprintf(sql, sizeof(sql),
                             "UPDATE inventory SET stock_quantity=%d WHERE product_id=%d;",
                             stock, id);
                    db_execute(db, sql);
                }
            } else if (field == 3) {
                char name[MAX_NAME];
                printf("New name: ");
                scanf(" %99[^\n]", name);
                snprintf(sql, sizeof(sql),
                         "SELECT product_id FROM inventory WHERE name='%s' AND product_id<>%d;",
                         name, id);
                {
                    MYSQL_RES *conflict = db_query(db, sql);
                    int exists = conflict != NULL && mysql_fetch_row(conflict) != NULL;
                    if (conflict != NULL) mysql_free_result(conflict);
                    if (!exists) {
                        snprintf(sql, sizeof(sql),
                                 "UPDATE inventory SET name='%s' WHERE product_id=%d;",
                                 name, id);
                        db_execute(db, sql);
                    } else printf("Product name already exists.\n");
                }
            } else if (field == 4) {
                char category[MAX_CATEGORY];
                if (choose_category(db, category, sizeof(category))) {
                    snprintf(sql, sizeof(sql),
                             "UPDATE inventory SET category='%s' WHERE product_id=%d;",
                             category, id);
                    db_execute(db, sql);
                }
            }
        } while (field != 0);
    } else if (choice == 6) {
        show_categories(db);
    } else if (choice == 7) {
        char name[MAX_CATEGORY], sql[256];
        double discount;
        printf("New category name: "); scanf(" %49[^\n]", name);
        printf("Category discount percentage: ");
        if (scanf("%lf", &discount) != 1 || discount < 0) return;
        snprintf(sql, sizeof(sql),
                 "INSERT INTO categories(category_name,discount_percent) "
                 "VALUES('%s',%.2f);", name, discount);
        if (db_execute(db, sql)) printf("Category added.\n");
    } else if (choice == 8) {
        char name[MAX_CATEGORY], sql[256];
        double discount;
        MYSQL_RES *result;
        MYSQL_ROW row;
        show_categories(db);
        printf("Category name to update: "); scanf(" %49[^\n]", name);
        snprintf(sql, sizeof(sql),
                 "SELECT discount_percent FROM categories WHERE category_name='%s';",
                 name);
        result = db_query(db, sql);
        if (!result) return;
        row = mysql_fetch_row(result);
        if (!row) {
            mysql_free_result(result);
            printf("Category not found.\n");
            continue;
        }
        printf("Current discount: %s%%\nNew discount percentage: ", row[0]);
        mysql_free_result(result);
        if (scanf("%lf", &discount) == 1 && discount >= 0)
            category_update_discount(db, name, discount);
    } else if (choice == 9) {
        char name[MAX_CATEGORY], sql[256];
        show_categories(db);
        printf("Category name to delete: "); scanf(" %49[^\n]", name);
        snprintf(sql, sizeof(sql),
                 "DELETE FROM categories WHERE category_name='%s';", name);
        if (db_execute(db, sql)) printf("Category deleted.\n");
    }
    load_products(db, products, count);
    }
}

int main(void)
{
    MYSQL *db = mysql_init(NULL);
    Product products[MAX_PRODUCTS];
    int count = 0, choice = -1;
    if (db == NULL || mysql_real_connect(db, "localhost", "root", "@1234",
                                         "campus_store", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "Unable to connect to MySQL: %s\n",
                db ? mysql_error(db) : "initialization failed");
        return 1;
    }
    if (!initialize(db) || !load_products(db, products, &count)) {
        mysql_close(db);
        return 1;
    }
    while (choice != 0) {
        ui_show_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n') {}
            continue;
        }
        if (choice == 1) {
            generate_cart(db, products, count);
            load_products(db, products, &count);
        } else if (choice == 2) {
            manage_products(db, products, &count);
        } else if (choice == 3) {
            int id, quantity = 0;
            char sql[256];
            printf("Product ID: ");
            if (scanf("%d", &id) == 1) {
                printf("Quantity to add: ");
                scanf("%d", &quantity);
            }
            if (quantity > 0) {
                snprintf(sql, sizeof(sql),
                    "UPDATE inventory SET stock_quantity=stock_quantity+%d WHERE product_id=%d;",
                    quantity, id);
                db_execute(db, sql);
                load_products(db, products, &count);
            }
        } else if (choice == 4) {
            manage_coupons(db);
        } else if (choice == 5) {
            show_bills(db);
        }
    }
    mysql_close(db);
    return 0;
}
