#pragma once

#include "balance.h"

// The referenced Balance must outlive Budget. Recommendations use its latest value.
class Budget {
public:
    // remainingDays counts today; 0 means the budgeting period has ended.
    // Throws std::invalid_argument if remainingDays is outside 0 through 90.
    Budget(const Balance &balance, int remainingDays);

    // Throws std::invalid_argument if days is outside 0 through 90.
    // A rejected update leaves the remaining-day count unchanged.
    void setRemainingDays(int days);

    int getRemainingDays() const;

    // Recommended maximum spending per day: balance / remaining days.
    // Returns the full balance when no days remain.
    double getDailyBudget() const;

    // Recommended maximum spending per week: daily budget * 7, capped at balance.
    // Returns the full balance when fewer than 7 days remain, including 0.
    double getWeeklyBudget() const;

private:
    const Balance &balance;
    int remainingDays;
    int DAYS_PER_WEEK = 7;
};
