#ifndef CAMPUS_BILL_HPP
#define CAMPUS_BILL_HPP

#include <iosfwd>

class Bill {
    double amount_;

public:
    explicit Bill(double amount = 0.0);
    double amount() const;
    Bill operator+(const Bill& other) const;
    friend std::ostream& operator<<(std::ostream& output, const Bill& bill);
};

#endif
