// Dining habits and per-meal suggestion happy case tests (T07; SPECIFICATIONS.md SP-08)
// Original test contributors: Nathan Gu
// Covers Budget::setDiningHabits, getDiningHabits, getRemainingCampusDays,
// getRemainingCampusMeals, getPerMealBudget and getPerCampusDayBudget.
// Weekdays use QDate::dayOfWeek: 1 is Monday, 7 is Sunday.

#include "doctest.h"
#include "balance.h"
#include "budget.h"

namespace {
constexpr int MONDAY = 1;
constexpr int SATURDAY = 6;
const Budget::DiningHabits WEEKDAYS_TWO_MEALS = {2, 2, 2, 2, 2, 0, 0};
}

TEST_CASE("Dining habits start empty and are kept once set") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    CHECK(budget.getDiningHabits() == Budget::DiningHabits{});
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK(budget.getDiningHabits() == WEEKDAYS_TWO_MEALS);
    budget.setDiningHabits({1, 0, 3, 0, 1, 0, 2});
    CHECK(budget.getDiningHabits() == Budget::DiningHabits{1, 0, 3, 0, 1, 0, 2});
}

TEST_CASE("SP-08: two weeks of weekday meals count 10 campus days and 20 meals") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK(budget.getRemainingCampusDays(MONDAY) == 10);
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 20);
}

TEST_CASE("SP-08: suggestions spread the spendable points across campus meals and days") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setReserveGoal(100.0);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK(budget.getPerMealBudget(MONDAY) == doctest::Approx(20.0));       // 400 / 20
    CHECK(budget.getPerCampusDayBudget(MONDAY) == doctest::Approx(40.0));  // 400 / 10
}

TEST_CASE("SP-08: suggestions follow changes to the balance and days left") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setReserveGoal(100.0);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    balance.add(100.0);
    CHECK(budget.getPerMealBudget(MONDAY) == doctest::Approx(25.0)); // 500 / 20
    budget.setRemainingDays(7);
    CHECK(budget.getPerCampusDayBudget(MONDAY) == doctest::Approx(100.0)); // 500 / 5
}

TEST_CASE("SP-08: campus days count from today's weekday and wrap into the next week") {
    Balance balance(90.0);
    Budget budget(balance, 3);
    budget.setDiningHabits({3, 0, 0, 0, 0, 0, 0}); // Mondays only.
    // Saturday, Sunday, Monday.
    CHECK(budget.getRemainingCampusDays(SATURDAY) == 1);
    CHECK(budget.getRemainingCampusMeals(SATURDAY) == 3);
    CHECK(budget.getPerMealBudget(SATURDAY) == doctest::Approx(30.0));
    CHECK(budget.getPerCampusDayBudget(SATURDAY) == doctest::Approx(90.0));
}

TEST_CASE("A full week counts every habit once, whatever day it starts on") {
    Balance balance(700.0);
    Budget budget(balance, 7);
    budget.setDiningHabits({1, 2, 3, 0, 1, 0, 2});
    for (int weekday = 1; weekday <= 7; ++weekday) {
        CAPTURE(weekday);
        CHECK(budget.getRemainingCampusDays(weekday) == 5);
        CHECK(budget.getRemainingCampusMeals(weekday) == 9);
    }
}
