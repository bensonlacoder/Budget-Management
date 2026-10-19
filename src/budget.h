// Budget model class.
// Responsibilities: create a daily budget, create a weekly budget, and
// update the remaining days in the quarter.
// Knows: Balance, Remaining Days
//
// Budget does not own the Balance. It keeps a reference to it, so any
// add/subtract done on the Balance is reflected in the next recommendation.

#pragma once

#include "balance.h"

class Budget {
public:
    // remainingDays counts today, so the last day of the quarter is 1.
    // Throws std::invalid_argument if remainingDays is less than 1.
    Budget(const Balance &balance, int remainingDays);

    // Updates the number of days left in the quarter. Throws
    // std::invalid_argument if days is less than 1.
    void setRemainingDays(int days);

    // Returns the number of days left in the quarter.
    int getRemainingDays() const;

    // Recommended maximum spending per day: balance / remaining days.
    double getDailyBudget() const;

    // Recommended maximum spending per week: daily budget * 7. If fewer than
    // 7 days remain, this is the whole remaining balance.
    double getWeeklyBudget() const;

private:
    const Balance &balance;
    int remainingDays;
};
