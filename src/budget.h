#pragma once

#include "balance.h"

#include <array>

// The referenced Balance must outlive Budget. Recommendations use its latest value.
class Budget {
public:
    // Meals eaten on campus for each weekday, Monday first. 0 means not on campus.
    using DiningHabits = std::array<int, 7>;

    // remainingDays counts today; 0 means the budgeting period has ended.
    // Throws std::invalid_argument if remainingDays is outside 0 through 90.
    Budget(const Balance &balance, int remainingDays);

    // Throws std::invalid_argument if days is outside 0 through 90.
    // A rejected update leaves the remaining-day count unchanged.
    void setRemainingDays(int days);

    int getRemainingDays() const;

    // Points to keep at the end of the quarter, excluded from the advice.
    // Throws std::invalid_argument if points is negative, not finite, or larger
    // than the balance. A rejected goal leaves the previous one in place.
    void setReserveGoal(double points);

    double getReserveGoal() const;

    // Balance minus the reserve, never below 0 (the balance can fall later).
    double getSpendablePoints() const;

    // Recommended maximum spending per day: (balance - reserve) / remaining days,
    // rounded down to the nearest hundredth so the advice never adds up to more than
    // the spendable points. Returns the spendable points when no days remain.
    double getDailyBudget() const;

    // Recommended maximum spending per week: (balance - reserve) * 7 / remaining days,
    // rounded down to the nearest hundredth.
    // Returns the spendable points when fewer than 7 days remain, including 0.
    double getWeeklyBudget() const;

    // Average net points spent per day over the last 7 days (SP-07). Defaults to 0.
    // Throws std::invalid_argument if pointsPerDay is negative or not finite.
    void setRecentSpendingRate(double pointsPerDay);

    double getRecentSpendingRate() const;

    // Balance minus the recent spending rate times the remaining days, to the nearest
    // hundredth. Negative when the points would run out before the quarter ends.
    double getProjectedEndBalance() const;

    // True when the projection leaves fewer points than the reserve goal (FR-43).
    bool isProjectedBelowReserve() const;

    // True when the projection runs out of points before the quarter ends (FR-42).
    bool isProjectedToRunOut() const;

    // Throws std::invalid_argument if a meal count is outside 0 through 10.
    // A rejected update leaves the previous habits in place.
    void setDiningHabits(const DiningHabits &habits);

    const DiningHabits &getDiningHabits() const;

    // The numbers below count `getRemainingDays()` days starting today, where
    // firstWeekday is today's QDate::dayOfWeek (1 = Monday ... 7 = Sunday).
    // An invalid weekday gives 0. See SP-08.
    int getRemainingCampusDays(int firstWeekday) const;
    int getRemainingCampusMeals(int firstWeekday) const;
    // Spendable points per on-campus meal or campus day, rounded down.
    // 0 when there are no habits or no campus days left.
    double getPerMealBudget(int firstWeekday) const;
    double getPerCampusDayBudget(int firstWeekday) const;

private:
    const Balance &balance;
    int remainingDays;
    double reserveGoal;
    double recentSpendingRate;
    DiningHabits habits;
    static constexpr int DAYS_PER_WEEK = 7;
    static constexpr int MAX_MEALS_PER_DAY = 10;
};
