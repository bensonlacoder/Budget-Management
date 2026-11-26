// Reserve goal, spending rate, projection and today's budget happy case tests
// (T06, T13; SPECIFICATIONS.md SP-07 and SP-09)
// Original test contributors: Nathan Gu
// Covers Budget::setReserveGoal, getReserveGoal, getSpendablePoints, getDailyBudget,
// getWeeklyBudget, setRecentSpendingRate, getRecentSpendingRate,
// getProjectedEndBalance, isProjectedBelowReserve, isProjectedToRunOut,
// getTodaysBudget and isOverTodaysBudget.

#include "doctest.h"
#include "balance.h"
#include "budget.h"

TEST_CASE("The reserve goal starts at 0 and leaves the advice unchanged") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    CHECK(budget.getReserveGoal() == 0.0);
    CHECK(budget.getSpendablePoints() == doctest::Approx(700.0));
    CHECK(budget.getDailyBudget() == doctest::Approx(10.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(70.0));
}

TEST_CASE("SP-07: a reserve goal is saved and left out of the daily and weekly advice") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(140.0);
    CHECK(budget.getReserveGoal() == doctest::Approx(140.0));
    CHECK(budget.getSpendablePoints() == doctest::Approx(560.0));
    CHECK(budget.getDailyBudget() == doctest::Approx(8.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(56.0));
}

TEST_CASE("The reserve goal can be changed, and the advice follows") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(140.0);
    budget.setReserveGoal(70.0);
    CHECK(budget.getReserveGoal() == doctest::Approx(70.0));
    CHECK(budget.getDailyBudget() == doctest::Approx(9.0));
    budget.setReserveGoal(0.0);
    CHECK(budget.getDailyBudget() == doctest::Approx(10.0));
}

TEST_CASE("Spendable points follow the latest balance") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(100.0);
    balance.add(50.0);
    CHECK(budget.getSpendablePoints() == doctest::Approx(650.0));
    balance.subtract(250.0);
    CHECK(budget.getSpendablePoints() == doctest::Approx(400.0));
}

TEST_CASE("The recent spending rate is saved") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    CHECK(budget.getRecentSpendingRate() == 0.0);
    budget.setRecentSpendingRate(9.5);
    CHECK(budget.getRecentSpendingRate() == doctest::Approx(9.5));
}

TEST_CASE("SP-07: the projection subtracts the recent rate for every day left") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(700.0)); // No spending yet.
    budget.setRecentSpendingRate(9.0);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(70.0));
    budget.setRemainingDays(10);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(610.0));
}

TEST_CASE("SP-07: the reserve warning shows when the projection ends below the reserve") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(140.0);
    budget.setRecentSpendingRate(9.0); // Ends with 70.
    CHECK(budget.isProjectedBelowReserve());
    budget.setRecentSpendingRate(5.0); // Ends with 350.
    CHECK_FALSE(budget.isProjectedBelowReserve());
}

TEST_CASE("SP-07: spending faster than the balance allows runs out before the quarter ends") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setRecentSpendingRate(11.0);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(-70.0));
    CHECK(budget.isProjectedToRunOut());
    budget.setRecentSpendingRate(5.0);
    CHECK_FALSE(budget.isProjectedToRunOut());
}

TEST_CASE("SP-09: today's budget comes from the balance before today's spending") {
    Balance balance(695.0); // 25 already spent today.
    Budget budget(balance, 72);
    CHECK(budget.getTodaysBudget(25.0) == doctest::Approx(10.0)); // 720 / 72
    CHECK(budget.getTodaysBudget(0.0) == doctest::Approx(budget.getDailyBudget()));
}

TEST_CASE("SP-09: spending more than today's budget is over it") {
    Balance balance(695.0);
    Budget budget(balance, 72);
    CHECK(budget.isOverTodaysBudget(25.0));
    balance.setBalance(715.0); // Only 5 spent.
    CHECK_FALSE(budget.isOverTodaysBudget(5.0));
}

TEST_CASE("SP-09: today's budget leaves out the reserve") {
    Balance balance(695.0);
    Budget budget(balance, 72);
    budget.setReserveGoal(72.0);
    CHECK(budget.getTodaysBudget(25.0) == doctest::Approx(9.0)); // (720 - 72) / 72
    CHECK(budget.isOverTodaysBudget(25.0));
}
