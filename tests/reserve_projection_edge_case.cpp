// Reserve goal, spending rate, projection and today's budget error and edge case tests
// (T06, T13; SPECIFICATIONS.md SP-07 and SP-09)
// Original test contributors: Nathan Gu

#include "doctest.h"
#include "balance.h"
#include "budget.h"

#include <limits>
#include <stdexcept>

namespace {
constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
constexpr double INFINITE = std::numeric_limits<double>::infinity();
}

TEST_CASE("SP-07: an invalid reserve goal is rejected and the previous one is kept") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(100.0);
    CHECK_THROWS_AS(budget.setReserveGoal(-1.0), std::invalid_argument);
    CHECK_THROWS_AS(budget.setReserveGoal(-0.01), std::invalid_argument);
    CHECK_THROWS_AS(budget.setReserveGoal(700.01), std::invalid_argument);
    CHECK_THROWS_AS(budget.setReserveGoal(NOT_A_NUMBER), std::invalid_argument);
    CHECK_THROWS_AS(budget.setReserveGoal(INFINITE), std::invalid_argument);
    CHECK(budget.getReserveGoal() == doctest::Approx(100.0));
    CHECK(budget.getDailyBudget() == doctest::Approx(8.57));
}

TEST_CASE("A reserve equal to the whole balance is allowed and leaves nothing to spend") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(700.0);
    CHECK(budget.getSpendablePoints() == 0.0);
    CHECK(budget.getDailyBudget() == 0.0);
    CHECK(budget.getWeeklyBudget() == 0.0);
}

TEST_CASE("A reserve on a 0 balance can only be 0") {
    Balance balance(0.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(0.0);
    CHECK(budget.getReserveGoal() == 0.0);
    CHECK_THROWS_AS(budget.setReserveGoal(0.01), std::invalid_argument);
}

TEST_CASE("SP-07: spending below the reserve leaves 0 spendable points, never less") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(140.0);
    balance.subtract(600.0);
    CHECK(budget.getSpendablePoints() == 0.0);
    CHECK(budget.getDailyBudget() == 0.0);
    CHECK(budget.getWeeklyBudget() == 0.0);
    // The goal is kept; the user decides whether to lower it.
    CHECK(budget.getReserveGoal() == doctest::Approx(140.0));
}

TEST_CASE("Advice with a reserve still rounds down to the hundredth") {
    Balance balance(500.0);
    Budget budget(balance, 30);
    budget.setReserveGoal(0.01);
    CHECK(budget.getSpendablePoints() == doctest::Approx(499.99));
    CHECK(budget.getDailyBudget() == doctest::Approx(16.66)); // 16.6663...
    CHECK(budget.getWeeklyBudget() == doctest::Approx(116.66)); // 116.6643...
}

TEST_CASE("With fewer than 7 days or none left, advice is all the spendable points") {
    Balance balance(700.0);
    Budget budget(balance, 6);
    budget.setReserveGoal(140.0);
    CHECK(budget.getWeeklyBudget() == doctest::Approx(560.0));
    budget.setRemainingDays(0);
    CHECK(budget.getDailyBudget() == doctest::Approx(560.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(560.0));
}

TEST_CASE("An invalid spending rate is rejected and the previous one is kept") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setRecentSpendingRate(5.0);
    CHECK_THROWS_AS(budget.setRecentSpendingRate(-0.01), std::invalid_argument);
    CHECK_THROWS_AS(budget.setRecentSpendingRate(INFINITE), std::invalid_argument);
    CHECK_THROWS_AS(budget.setRecentSpendingRate(NOT_A_NUMBER), std::invalid_argument);
    CHECK(budget.getRecentSpendingRate() == doctest::Approx(5.0));
    budget.setRecentSpendingRate(0.0); // 0 is allowed.
    CHECK(budget.getRecentSpendingRate() == 0.0);
}

TEST_CASE("Ending exactly at the reserve, or exactly at 0, is not a warning") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(140.0);
    budget.setRecentSpendingRate(8.0); // 700 - 560 = 140.
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(140.0));
    CHECK_FALSE(budget.isProjectedBelowReserve());

    budget.setReserveGoal(0.0);
    budget.setRecentSpendingRate(10.0); // 700 - 700 = 0.
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(0.0));
    CHECK_FALSE(budget.isProjectedToRunOut());
    CHECK_FALSE(budget.isProjectedBelowReserve());
}

TEST_CASE("Running out is also below a reserve of 0") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setRecentSpendingRate(10.01);
    CHECK(budget.isProjectedToRunOut());
    CHECK(budget.isProjectedBelowReserve());
}

TEST_CASE("With no days left the projection is the balance itself") {
    Balance balance(700.0);
    Budget budget(balance, 0);
    budget.setRecentSpendingRate(50.0);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(700.0));
    CHECK_FALSE(budget.isProjectedToRunOut());
}

TEST_CASE("SP-09: spending exactly today's budget is not over it, one hundredth more is") {
    Balance balance(710.0); // 10 spent today.
    Budget budget(balance, 72);
    CHECK(budget.getTodaysBudget(10.0) == doctest::Approx(10.0));
    CHECK_FALSE(budget.isOverTodaysBudget(10.0));
    balance.setBalance(709.99); // 10.01 spent today.
    CHECK(budget.isOverTodaysBudget(10.01));
}

TEST_CASE("SP-09: refunds that make today's spending negative don't raise today's budget") {
    Balance balance(695.0);
    Budget budget(balance, 72);
    CHECK(budget.getTodaysBudget(-50.0) == doctest::Approx(budget.getDailyBudget()));
    CHECK_FALSE(budget.isOverTodaysBudget(-50.0));
}

TEST_CASE("SP-09: on the last day, today's budget is everything spendable") {
    Balance balance(695.0);
    Budget budget(balance, 0);
    budget.setReserveGoal(72.0);
    CHECK(budget.getTodaysBudget(25.0) == doctest::Approx(648.0)); // 720 - 72
}

TEST_CASE("SP-09: a reserve above the start-of-day balance gives a budget of 0") {
    Balance balance(100.0);
    Budget budget(balance, 10);
    budget.setReserveGoal(100.0);
    balance.setBalance(50.0); // 50 spent today.
    CHECK(budget.getTodaysBudget(50.0) == doctest::Approx(0.0));
    CHECK(budget.isOverTodaysBudget(50.0));
    CHECK_FALSE(budget.isOverTodaysBudget(0.0));
}

TEST_CASE("SP-09: a spent amount that isn't a number is rejected") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    CHECK_THROWS_AS(budget.getTodaysBudget(NOT_A_NUMBER), std::invalid_argument);
    CHECK_THROWS_AS(budget.getTodaysBudget(INFINITE), std::invalid_argument);
    CHECK_THROWS_AS(budget.isOverTodaysBudget(NOT_A_NUMBER), std::invalid_argument);
}
