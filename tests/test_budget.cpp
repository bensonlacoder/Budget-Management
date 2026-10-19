#include "doctest.h"

#include <stdexcept>

#include "balance.h"
#include "budget.h"

// normal case: daily and weekly budgets come from balance and remaining days
TEST_CASE("Budget calculates daily and weekly budgets") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    CHECK(budget.getDailyBudget() == doctest::Approx(10.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(70.0));
}

// normal case: changes to the balance and remaining days update the budget
TEST_CASE("Budget follows balance and remaining day updates") {
    Balance balance(700.0);
    Budget budget(balance, 70);

    balance.subtract(140.0);
    CHECK(budget.getDailyBudget() == doctest::Approx(8.0));

    budget.setRemainingDays(56);
    CHECK(budget.getRemainingDays() == 56);
    CHECK(budget.getDailyBudget() == doctest::Approx(10.0));
}

// wrong case: remaining days below 1 are rejected
TEST_CASE("Budget rejects invalid remaining days") {
    Balance balance(100.0);
    CHECK_THROWS_AS(Budget(balance, 0), std::invalid_argument);

    Budget budget(balance, 10);
    CHECK_THROWS_AS(budget.setRemainingDays(-3), std::invalid_argument);
    CHECK(budget.getRemainingDays() == 10);
}

// edge case: last day, fewer than 7 days left, and an empty balance
TEST_CASE("Budget handles end-of-quarter edge cases") {
    Balance balance(30.0);
    Budget budget(balance, 1);
    CHECK(budget.getDailyBudget() == doctest::Approx(30.0));

    budget.setRemainingDays(3);
    CHECK(budget.getWeeklyBudget() == doctest::Approx(30.0));

    balance.setBalance(0.0);
    CHECK(budget.getDailyBudget() == doctest::Approx(0.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(0.0));
}
