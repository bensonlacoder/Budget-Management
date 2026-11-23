// Budget happy-path tests: daily and weekly recommendations for a normal number of days.

#include "doctest.h"

#include "balance.h"
#include "budget.h"

// Author: Jackie Munoz Arevalo (FR-06, FR-07, FR-08)
TEST_CASE("SP-04: 700 points over 70 days recommends 10 per day and 70 per week") {
    Balance balance;
    balance.setBalance(700.0);
    Budget budget(balance, 70);
    CHECK(budget.getDailyBudget() == 10.0);
    CHECK(budget.getWeeklyBudget() == 70.0);

    budget.setRemainingDays(3);
    CHECK(budget.getWeeklyBudget() == 700.0);
}

// Author: Nathan Gu
TEST_CASE("Budget can be initialized with a valid balance and remaining days") {
    Balance balance(500.0);
    Budget budget(balance, 30);
    CHECK(budget.getRemainingDays() == 30);
    CHECK(budget.getDailyBudget() == doctest::Approx(500.0 / 30));
}

TEST_CASE("Recommendations follow balance changes made after the budget is created") {
    Balance balance(700);
    Budget budget(balance, 70);
    balance.subtract(140);
    CHECK(budget.getDailyBudget() == doctest::Approx(8.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(56.0));
}
