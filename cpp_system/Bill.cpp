#include "Bill.hpp"

#include <iostream>

Bill::Bill(double amount) : amount_(amount) {}

double Bill::amount() const { return amount_; }

Bill Bill::operator+(const Bill& other) const
{
    return Bill(amount_ + other.amount_);
}

std::ostream& operator<<(std::ostream& output, const Bill& bill)
{
    return output << bill.amount_;
}
