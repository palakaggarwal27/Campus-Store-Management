#include "Product.hpp"

#include <utility>
double StoreProduct::sellingPrice() const
{
    return basePrice * (1.0 - discountPercent / 100.0);
}

void Cart::add(const StoreProduct& product, int quantity)
{
    items_.push_back({product, quantity});
}

double Cart::subtotal() const
{
    double result = 0.0;
    for (const CartItem& item : items_)
        result += item.product.sellingPrice() * item.quantity;
    return result;
}

void Cart::applyCoupon(double percent, double flatAmount)
{
    const double before = subtotal();
    couponDiscount_ = before * percent / 100.0 + flatAmount;
    if (couponDiscount_ > before) couponDiscount_ = before;
}

void Cart::applyGst(double percent)
{
    const double afterDiscount = subtotal() - couponDiscount_;
    gstAmount_ = afterDiscount * percent / 100.0;
    total_ = afterDiscount + gstAmount_;
}

const std::vector<CartItem>& Cart::items() const { return items_; }
double Cart::couponDiscount() const { return couponDiscount_; }
double Cart::gstAmount() const { return gstAmount_; }
double Cart::total() const { return total_; }

int Product::productCount_ = 0;

Product::Product(int id, std::string name, double basePrice)
    : id_(id), name_(std::move(name)), basePrice_(basePrice)
{
    if (id <= 0 || name_.empty() || basePrice < 0)
        throw std::invalid_argument("Invalid product identifier, name, or price");
    ++productCount_;
}

Product::Product(const Product& other)
    : id_(other.id_), name_(other.name_), basePrice_(other.basePrice_)
{
    ++productCount_;
}

Product::~Product() { --productCount_; }

double Product::discount(double percent) const
{
    return finalPrice() * (1.0 - percent / 100.0);
}

double Product::discount(double percent, double coupon) const
{
    const double amount = discount(percent) - coupon;
    return amount > 0.0 ? amount : 0.0;
}

Book::Book(int id, std::string name, double basePrice)
    : Product(id, std::move(name), basePrice) {}

double Book::finalPrice() const { return basePrice_ * 0.90; }

Accessory::Accessory(int id, std::string name, double basePrice)
    : Product(id, std::move(name), basePrice) {}

double Accessory::finalPrice() const { return basePrice_ * 1.05; }

Stationery::Stationery(int id, std::string name, double basePrice)
    : Product(id, std::move(name), basePrice) {}

double Stationery::finalPrice() const { return basePrice_ * 0.95; }
