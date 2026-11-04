#include "budget.h"

#include <stdexcept>

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
    return balance.getBalance() / remainingDays;
}

double Budget::getWeeklyBudget() const {
    if (remainingDays < DAYS_PER_WEEK) {
        return balance.getBalance();
    }
    return getDailyBudget() * DAYS_PER_WEEK;
}
