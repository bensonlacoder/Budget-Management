#include "budget.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
// Rounds down to whole hundredths of a point. The small tolerance stops values such as
// 0.29, stored as 0.28999..., from dropping a cent.
double floorToHundredths(double points) {
    return std::floor(points * 100.0 + 1e-6) / 100.0;
}

// True if two point amounts are the same to the hundredth.
bool samePoints(double left, double right) {
    return std::llround(left * 100.0) == std::llround(right * 100.0);
}
}

Budget::Budget(const Balance &balance, int remainingDays)
    : balance(balance), remainingDays(1), reserveGoal(0.0), recentSpendingRate(0.0),
      habits{} {
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
