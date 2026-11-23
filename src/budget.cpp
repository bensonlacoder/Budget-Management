#include "budget.h"

#include <cmath>
#include <stdexcept>

namespace {
// Rounds down to whole hundredths of a point. The small tolerance stops values such as
// 0.29, stored as 0.28999..., from dropping a cent.
double floorToHundredths(double points) {
    return std::floor(points * 100.0 + 1e-6) / 100.0;
}
}

Budget::Budget(const Balance &balance, int remainingDays)
    : balance(balance), remainingDays(1) {
    setRemainingDays(remainingDays);
}

void Budget::setRemainingDays(int days) {
    if (days < 0) {
        throw std::invalid_argument("Remaining days cannot be negative");
    }
    if (days > 90) {
        throw std::invalid_argument("Remaining days cannot exceed 90");
    }
    remainingDays = days;
}

int Budget::getRemainingDays() const {
    return remainingDays;
}

double Budget::getDailyBudget() const {
    if (remainingDays == 0) {
        return balance.getBalance();
    }
    return floorToHundredths(balance.getBalance() / remainingDays);
}

double Budget::getWeeklyBudget() const {
    if (remainingDays < DAYS_PER_WEEK) {
        return balance.getBalance();
    }
    return floorToHundredths(balance.getBalance() * DAYS_PER_WEEK / remainingDays);
}
