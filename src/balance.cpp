#include "balance.h"

#include <stdexcept>

Balance::Balance(double startingBalance) : balance(0.0) {
    setBalance(startingBalance);
}

void Balance::setBalance(double amount) {
    if (amount < 0) {
        throw std::invalid_argument("Balance cannot be negative");
    }
    balance = amount;
}

double Balance::getBalance() const {
    return balance;
}

void Balance::add(double amount) {
    if (amount < 0) {
        throw std::invalid_argument("Amount to add cannot be negative");
    }
    balance += amount;
}

void Balance::subtract(double amount) {
    if (amount < 0) {
        throw std::invalid_argument("Amount to subtract cannot be negative");
    }
    if (amount > balance) {
        throw std::invalid_argument("Amount to subtract exceeds balance");
    }
    balance -= amount;
}
