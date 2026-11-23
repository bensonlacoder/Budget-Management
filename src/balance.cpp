#include "balance.h"

#include <cmath>
#include <stdexcept>

namespace {
// Dining points have two decimal places. Rounding every stored value to the
// nearest hundredth keeps the balance equal to what the user sees, so a
// displayed balance of 0.80 can always be spent with an amount of 0.80.
double toHundredths(double amount) {
    return std::round(amount * 100.0) / 100.0;
}

void requireValidAmount(double amount, const char *message) {
    if (!std::isfinite(amount) || amount < 0) {
        throw std::invalid_argument(message);
    }
}
}

Balance::Balance(double startingBalance) : balance(0.0) {
    setBalance(startingBalance);
}

void Balance::setBalance(double amount) {
    requireValidAmount(amount, "Balance must be a finite, non-negative amount");
    balance = toHundredths(amount);
}

double Balance::getBalance() const {
    return balance;
}

void Balance::add(double amount) {
    requireValidAmount(amount, "Amount to add must be a finite, non-negative amount");
    balance = toHundredths(balance + toHundredths(amount));
}

void Balance::subtract(double amount) {
    requireValidAmount(amount, "Amount to subtract must be a finite, non-negative amount");
    const double rounded = toHundredths(amount);
    if (rounded > balance) {
        throw std::invalid_argument("Amount to subtract exceeds balance");
    }
    balance = toHundredths(balance - rounded);
}
