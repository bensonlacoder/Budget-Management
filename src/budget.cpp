#include "budget.h"

#include <stdexcept>

Budget::Budget(const Balance &balance, int remainingDays)
    : balance(balance), remainingDays(1) {
    setRemainingDays(remainingDays);
}

void Budget::setRemainingDays(int days) {
    if (days < 1) {
        throw std::invalid_argument("Remaining days must be at least 1");
    }
    remainingDays = days;
}

int Budget::getRemainingDays() const {
    return remainingDays;
}

double Budget::getDailyBudget() const {
    return balance.getBalance() / remainingDays;
}

double Budget::getWeeklyBudget() const {
    if (remainingDays < 7) {
        return balance.getBalance();
    }
    return getDailyBudget() * 7;
}
