#pragma once

#include "balance.h"

// The referenced Balance must outlive Budget. Recommendations use its latest value.
class Budget {
public:
    // remainingDays counts today, so the last day of the quarter is 1.
    // Throws std::invalid_argument if remainingDays is less than 1.
    Budget(const Balance &balance, int remainingDays);

    // Throws std::invalid_argument if days is less than 1.
    void setRemainingDays(int days);

    int getRemainingDays() const;

    // Recommended maximum spending per day: balance / remaining days.
    double getDailyBudget() const;

    // Recommended maximum spending per week: daily budget * 7.
    double getWeeklyBudget() const;

private:
    const Balance &balance;
    int remainingDays;
    int DAYS_PER_WEEK = 7;
};
