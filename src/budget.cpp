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

// Rounds to the nearest hundredth, the precision every point amount is kept at.
double roundToHundredths(double points) {
    return static_cast<double>(std::llround(points * 100.0)) / 100.0;
}

long long hundredths(double points) {
    return std::llround(points * 100.0);
}

bool isWeekday(int day) {
    return day >= 1 && day <= 7;
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

void Budget::setReserveGoal(double points) {
    if (!std::isfinite(points) || points < 0.0) {
        throw std::invalid_argument("Reserve goal must be 0 or more points");
    }
    if (hundredths(points) > hundredths(balance.getBalance())) {
        throw std::invalid_argument("Reserve goal cannot be more than the balance");
    }
    reserveGoal = roundToHundredths(points);
}

double Budget::getReserveGoal() const {
    return reserveGoal;
}

double Budget::getSpendablePoints() const {
    return std::max(0.0, roundToHundredths(balance.getBalance() - reserveGoal));
}

double Budget::getDailyBudget() const {
    const double spendable = getSpendablePoints();
    if (remainingDays == 0) {
        return spendable;
    }
    return floorToHundredths(spendable / remainingDays);
}

double Budget::getWeeklyBudget() const {
    const double spendable = getSpendablePoints();
    if (remainingDays < DAYS_PER_WEEK) {
        return spendable;
    }
    return floorToHundredths(spendable * DAYS_PER_WEEK / remainingDays);
}

void Budget::setRecentSpendingRate(double pointsPerDay) {
    if (!std::isfinite(pointsPerDay) || pointsPerDay < 0.0) {
        throw std::invalid_argument("Spending rate must be 0 or more points per day");
    }
    recentSpendingRate = pointsPerDay;
}

double Budget::getRecentSpendingRate() const {
    return recentSpendingRate;
}

double Budget::getProjectedEndBalance() const {
    return roundToHundredths(balance.getBalance() - recentSpendingRate * remainingDays);
}

bool Budget::isProjectedBelowReserve() const {
    return hundredths(getProjectedEndBalance()) < hundredths(reserveGoal);
}

bool Budget::isProjectedToRunOut() const {
    return hundredths(getProjectedEndBalance()) < 0;
}

void Budget::setDiningHabits(const DiningHabits &newHabits) {
    for (int meals : newHabits) {
        if (meals < 0 || meals > MAX_MEALS_PER_DAY) {
            throw std::invalid_argument("Meals per day must be from 0 to 10");
        }
    }
    habits = newHabits;
}

const Budget::DiningHabits &Budget::getDiningHabits() const {
    return habits;
}

int Budget::getRemainingCampusDays(int firstWeekday) const {
    if (!isWeekday(firstWeekday)) {
        return 0;
    }
    int campusDays = 0;
    for (int day = 0; day < remainingDays; ++day) {
        campusDays += habits[(firstWeekday - 1 + day) % DAYS_PER_WEEK] > 0 ? 1 : 0;
    }
    return campusDays;
}

int Budget::getRemainingCampusMeals(int firstWeekday) const {
    if (!isWeekday(firstWeekday)) {
        return 0;
    }
    int meals = 0;
    for (int day = 0; day < remainingDays; ++day) {
        meals += habits[(firstWeekday - 1 + day) % DAYS_PER_WEEK];
    }
    return meals;
}

double Budget::getPerMealBudget(int firstWeekday) const {
    const int meals = getRemainingCampusMeals(firstWeekday);
    return meals == 0 ? 0.0 : floorToHundredths(getSpendablePoints() / meals);
}

double Budget::getPerCampusDayBudget(int firstWeekday) const {
    const int campusDays = getRemainingCampusDays(firstWeekday);
    return campusDays == 0 ? 0.0 : floorToHundredths(getSpendablePoints() / campusDays);
}
