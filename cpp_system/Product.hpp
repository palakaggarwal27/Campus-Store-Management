#ifndef CAMPUS_PRODUCT_HPP
#define CAMPUS_PRODUCT_HPP

#include <string>
#include <stdexcept>
#include <vector>

struct StoreProduct {
    int id;
    std::string name;
    std::string category;
    double basePrice;
    double discountPercent;
    int stock;

    double sellingPrice() const;
};

struct CartItem {
    StoreProduct product;
    int quantity;
};

class Cart {
    std::vector<CartItem> items_;
    double couponDiscount_ = 0.0;
    double gstAmount_ = 0.0;
    double total_ = 0.0;

public:
    void add(const StoreProduct& product, int quantity);
    double subtotal() const;
    void applyCoupon(double percent, double flatAmount);
    void applyGst(double percent);
    const std::vector<CartItem>& items() const;
    double couponDiscount() const;
    double gstAmount() const;
    double total() const;
};

class Product {
    int id_;
    static int productCount_;

protected:
    std::string name_;
    double basePrice_;

public:
    Product(int id, std::string name, double basePrice);
    Product(const Product& other);
    Product& operator=(const Product&) = delete;
    virtual ~Product();

    inline int getId() const { return id_; }
    inline const std::string& getName() const { return name_; }
    inline double getPrice() const { return basePrice_; }
    inline static int getProductCount() { return productCount_; }
    virtual double finalPrice() const { return basePrice_; }
    double discount(double percent) const;
    double discount(double percent, double coupon) const;
    friend void auditProduct(const Product& product);
};

class Book : public Product {
public:
    Book(int id, std::string name, double basePrice);
    double finalPrice() const override;
};

class Accessory : public Product {
public:
    Accessory(int id, std::string name, double basePrice);
    double finalPrice() const override;
};

class Stationery : public Product {
public:
    Stationery(int id, std::string name, double basePrice);
    double finalPrice() const override;
};

#endif
