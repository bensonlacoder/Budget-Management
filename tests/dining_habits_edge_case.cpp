// Dining habits and per-meal suggestion error and edge case tests
// (T07; SPECIFICATIONS.md SP-08)
// Original test contributors: Nathan Gu

#include "doctest.h"
#include "balance.h"
#include "budget.h"

#include <stdexcept>

namespace {
constexpr int MONDAY = 1;
constexpr int SATURDAY = 6;
const Budget::DiningHabits WEEKDAYS_TWO_MEALS = {2, 2, 2, 2, 2, 0, 0};
}

TEST_CASE("SP-08: a meal count outside 0 to 10 is rejected and the habits are kept") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK_THROWS_AS(budget.setDiningHabits({2, 2, 11, 2, 2, 0, 0}), std::invalid_argument);
    CHECK_THROWS_AS(budget.setDiningHabits({2, -1, 2, 2, 2, 0, 0}), std::invalid_argument);
    CHECK_THROWS_AS(budget.setDiningHabits({0, 0, 0, 0, 0, 0, 100}), std::invalid_argument);
    CHECK(budget.getDiningHabits() == WEEKDAYS_TWO_MEALS);
}

TEST_CASE("10 meals a day is the most allowed, and 0 everywhere clears the habits") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setDiningHabits({10, 10, 10, 10, 10, 10, 10});
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 140);
    budget.setDiningHabits({});
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 0);
}

TEST_CASE("SP-08: without habits, there are no campus days and no suggestions") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    CHECK(budget.getRemainingCampusDays(MONDAY) == 0);
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 0);
    CHECK(budget.getPerMealBudget(MONDAY) == 0.0);
    CHECK(budget.getPerCampusDayBudget(MONDAY) == 0.0);
}

TEST_CASE("SP-08: weekday-only habits with only the weekend left give no suggestions") {
    Balance balance(500.0);
    Budget budget(balance, 2);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK(budget.getRemainingCampusDays(SATURDAY) == 0);
    CHECK(budget.getPerMealBudget(SATURDAY) == 0.0);
    CHECK(budget.getPerCampusDayBudget(SATURDAY) == 0.0);
}

TEST_CASE("With no days left there are no campus days") {
    Balance balance(500.0);
    Budget budget(balance, 0);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK(budget.getRemainingCampusDays(MONDAY) == 0);
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 0);
    CHECK(budget.getPerMealBudget(MONDAY) == 0.0);
}

TEST_CASE("An invalid weekday counts nothing") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    for (int weekday : {0, 8, -1}) {
        CAPTURE(weekday);
        CHECK(budget.getRemainingCampusDays(weekday) == 0);
        CHECK(budget.getRemainingCampusMeals(weekday) == 0);
        CHECK(budget.getPerMealBudget(weekday) == 0.0);
        CHECK(budget.getPerCampusDayBudget(weekday) == 0.0);
    }
}

TEST_CASE("Suggestions round down to the hundredth") {
    Balance balance(100.0);
    Budget budget(balance, 3);
    budget.setDiningHabits({1, 1, 1, 0, 0, 0, 0});
    CHECK(budget.getPerMealBudget(MONDAY) == doctest::Approx(33.33));
    CHECK(budget.getPerCampusDayBudget(MONDAY) == doctest::Approx(33.33));
}

TEST_CASE("With nothing spendable after the reserve, every suggestion is 0") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setReserveGoal(500.0);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 20);
    CHECK(budget.getPerMealBudget(MONDAY) == 0.0);
    CHECK(budget.getPerCampusDayBudget(MONDAY) == 0.0);
}
