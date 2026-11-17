# Campus Store Management System

A dual-implementation Case Study developed for the Department of Information Technology, Maharaja Surajmal Institute of Technology (MSIT).

This repository compares an **Object-Oriented C++ system** against an equivalent **Procedural C system** for a Campus Store handling inventory, category-based pricing, discounts, billing, and auditing.

> **Note:** For the complete academic case-study report, theoretical analysis (Bloom's Taxonomy L1–L6), and experimental findings, please refer to the accompanying **Case Study Report PDF**.

---

## 📌 Project Overview

The Campus Store Management System automates retail operations for a college store selling Books, Accessories, and Stationery.

### Key Capabilities
* **Inventory Management:** Full CRUD operations for products and categories with stock tracking and low-stock alerts.
* **Dynamic Category Pricing:**
  * **Books:** 10% discount on base price (`finalPrice = 0.90 × basePrice`)
  * **Accessories:** 5% category markup (`finalPrice = 1.05 × basePrice`)
  * **Stationery:** 5% discount (`finalPrice = 0.95 × basePrice`)
* **Flexible Billing & Cart:** Supports percentage discounts, flat coupons, GST calculations, cash-counter payments, and digital/UPI transactions.
* **Auditing & Live Tracking:** Real-time object tracking and privileged audit inspection of product records.

---

## 🏛️ Architecture: OOP vs Procedural

Both systems share the same relational database schema and provide identical user-facing workflows, but differ in internal design:

* **`cpp_system/` (Object-Oriented C++17):**
  * Employs inheritance (`Book`, `Accessory`, `Stationery` inheriting from `Product`).
  * Polymorphic virtual dispatch eliminates `switch`/`if-else` branching in billing calculations.
  * Encapsulation with copy prevention (`operator=(const Product&) = delete`) and static live-object counter.
  * Function overloading for discount policies (`discount(percent)` and `discount(percent, coupon)`).
  * Operator overloading (`Bill + Bill`) for combining multi-cart bills.
  * Friend function (`auditProduct`) for secure, privileged inspection.
* **`c_system/` (Procedural C17):**
  * Modular C architecture utilizing `struct`, explicit function pointers, and separated procedural modules (`product.c`, `categories.c`, `bill.c`, `audit.c`).

---

## ⚙️ Prerequisites

Before compiling, ensure the following are installed on Windows:
1. **MinGW-w64** (GCC/G++ with C17 and C++17 support).
2. **MySQL Server 8.0** (Community Edition).
3. **MySQL C Connector / Client Library** (default install location: `C:\Program Files\MySQL\MySQL Server 8.0`).

---

## 🗄️ Database Setup

The application features **automatic self-initialization**. When launched, both portals automatically execute `CREATE TABLE IF NOT EXISTS` and seed default sample data for all 8 tables.

### Quick Setup:
1. Open MySQL Command Line Client or MySQL Workbench:
   ```sql
   CREATE DATABASE campus_store;
   ```
2. The application connects using:
   * **Host:** `localhost`
   * **Port:** `3306`
   * **Database:** `campus_store`
   * **User:** `root`
   * **Password:** *(Configured in code)*
   
   > 💡 **Password Configuration:** If your local MySQL root password differs from the default set in the source, update the connection credentials in:
   > - `cpp_system/Database.cpp` (inside the `connect()` method)
   > - `c_system/main.c` (inside the `main()` database connection block)

### Manual Database Schema & Seed Data (Optional)
If you prefer to inspect or manually initialize the database tables beforehand use the table desc below:

```sql
USE campus_store;

CREATE TABLE IF NOT EXISTS inventory (
    product_id INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    base_price DECIMAL(10,2) NOT NULL CHECK (base_price >= 0),
    category VARCHAR(50) NOT NULL,
    stock_quantity INT NOT NULL DEFAULT 0,
    total_sold INT NOT NULL DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS categories (
    category_name VARCHAR(50) PRIMARY KEY,
    discount_percent DECIMAL(5,2) NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS coupons (
    coupon_code VARCHAR(30) PRIMARY KEY,
    discount_percent DECIMAL(5,2) NOT NULL DEFAULT 0,
    flat_amount DECIMAL(10,2) NOT NULL DEFAULT 0,
    active BOOLEAN NOT NULL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS orders (
    order_id INT AUTO_INCREMENT PRIMARY KEY,
    total_amount DECIMAL(10,2) NOT NULL,
    paid_amount DECIMAL(10,2) NOT NULL,
    change_amount DECIMAL(10,2) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS order_items (
    order_item_id INT AUTO_INCREMENT PRIMARY KEY,
    order_id INT NOT NULL,
    product_id INT NOT NULL,
    product_name VARCHAR(100) NOT NULL,
    unit_price DECIMAL(10,2) NOT NULL,
    quantity INT NOT NULL,
    line_total DECIMAL(10,2) NOT NULL,
    FOREIGN KEY(order_id) REFERENCES orders(order_id),
    FOREIGN KEY(product_id) REFERENCES inventory(product_id)
);

CREATE TABLE IF NOT EXISTS order_details (
    order_id INT PRIMARY KEY,
    subtotal DECIMAL(10,2) NOT NULL,
    category_discount DECIMAL(10,2) NOT NULL,
    coupon_discount DECIMAL(10,2) NOT NULL,
    gst_percent DECIMAL(5,2) NOT NULL,
    gst_amount DECIMAL(10,2) NOT NULL,
    payment_method VARCHAR(30) NOT NULL,
    FOREIGN KEY(order_id) REFERENCES orders(order_id)
);

CREATE TABLE IF NOT EXISTS accounts (
    account_name VARCHAR(30) PRIMARY KEY,
    balance DECIMAL(12,2) NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS financial_transactions (
    transaction_id INT AUTO_INCREMENT PRIMARY KEY,
    transaction_type VARCHAR(20) NOT NULL,
    order_id INT NULL,
    amount DECIMAL(12,2) NOT NULL,
    payment_method VARCHAR(30) NOT NULL,
    account_name VARCHAR(30) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(order_id) REFERENCES orders(order_id)
);

-- Seed initial accounts, categories, and inventory items
INSERT IGNORE INTO accounts(account_name, balance) VALUES ('cash_counter', 0), ('cash_bank', 0);
INSERT IGNORE INTO categories(category_name, discount_percent) VALUES ('Book', 10), ('Accessory', 0), ('Stationery', 5);
INSERT IGNORE INTO inventory(product_id, name, base_price, category, stock_quantity) VALUES
(101, 'C++ Textbook', 500, 'Book', 20),
(202, 'USB Mouse', 800, 'Accessory', 15),
(303, 'Pen Set', 200, 'Stationery', 50);
```

---

## 🔨 How to Build

Open PowerShell or Command Prompt in the project root directory:

### 1. Build C++ System (OOP)
```cmd
g++ -std=c++17 -Wall -Wextra -pedantic ^
  -I"C:\Program Files\MySQL\MySQL Server 8.0\include" ^
  cpp_system\main.cpp cpp_system\Database.cpp cpp_system\Product.cpp ^
  cpp_system\InventoryManager.cpp cpp_system\Categories.cpp ^
  cpp_system\Bill.cpp cpp_system\Audit.cpp cpp_system\UIEngine.cpp ^
  -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" -lmysql ^
  -o cpp_system\campus-store-cpp.exe
```

### 2. Build C System (Procedural)
```cmd
gcc -std=c17 -Wall -Wextra -pedantic ^
  -I"C:\Program Files\MySQL\MySQL Server 8.0\include" ^
  c_system\main.c c_system\database.c c_system\product.c ^
  c_system\inventory_manager.c c_system\categories.c ^
  c_system\bill.c c_system\audit.c c_system\ui_engine.c ^
  -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" -lmysql ^
  -o c_system\campus-store-c.exe
```

---

## 🚀 How to Run

> ⚠️ **Runtime Dependency:** Make sure `libmysql.dll` (located in `C:\Program Files\MySQL\MySQL Server 8.0\lib`) is accessible in your system `PATH` or copied into the respective `cpp_system` / `c_system` directory before running the executables.

### Run C++ System:
```cmd
.\cpp_system\campus-store-cpp.exe
```

### Run C System:
```cmd
.\c_system\campus-store-c.exe
```

---

## 📁 Repository Directory Structure

```text
├── README.md               # Project documentation & run guide
├── BUILD.txt               # Build flags & compiler commands reference
├── cpp_system/             # Object-Oriented C++ implementation
│   ├── Product.hpp/cpp     # Product hierarchy, categories & discounting
│   ├── Bill.hpp/cpp        # Bill representation & operator+
│   ├── Audit.hpp/cpp       # Friend audit facility
│   ├── Database.hpp/cpp    # MySQL RAII database wrapper
│   ├── InventoryManager.*  # Stock and category operations
│   ├── Categories.hpp/cpp  # Category rules & validation
│   ├── UIEngine.hpp/cpp    # Console interface & formatting
│   └── main.cpp            # Application loop & auto-initialization
└── c_system/               # Procedural C implementation
    ├── product.h/c         # Procedural product structures & functions
    ├── bill.h/c            # Procedural billing logic
    ├── audit.h/c           # Procedural audit helper
    ├── database.h/c        # Direct MySQL C-API calls
    ├── inventory_manager.* # Procedural inventory routines
    ├── categories.h/c      # Procedural category discount handlers
    ├── ui_engine.h/c       # Console output routines
    └── main.c              # Main menu & schema initialization
```
