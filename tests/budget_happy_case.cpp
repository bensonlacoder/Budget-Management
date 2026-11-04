// Budget happy case tests
// Original test contributors: Nathan Gu, Jackie Munoz Arevalo

#include "doctest.h"
#include "balance.h"
#include "budget.h"

TEST_CASE("Budget can be initialized with a valid balance and remaining days") {
    Balance balance(500.0);
    Budget budget(balance, 30);

    CHECK(budget.getRemainingDays() == 30);
    CHECK(budget.getDailyBudget() ==
          doctest::Approx(500.0 / 30));
}

TEST_CASE("Budget calculates daily and weekly allowances") {
    Balance balance;
    balance.setBalance(700.0);
    Budget budget(balance, 70);

    CHECK(budget.getDailyBudget() == 10.0);
    CHECK(budget.getWeeklyBudget() == 70.0);

    budget.setRemainingDays(3);
    CHECK(budget.getWeeklyBudget() == 700.0);
}

TEST_CASE("Recommendations use the latest balance with positive remaining days") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    double expectedDaily = 11.0;
    double expectedWeekly = 77.0;

    SUBCASE("Setting the balance") {
        balance.setBalance(770.0);
    }
    SUBCASE("Adding points") {
        balance.add(70.0);
    }
    SUBCASE("Spending points") {
        balance.subtract(70.0);
        expectedDaily = 9.0;
        expectedWeekly = 63.0;
    }

    CHECK(budget.getDailyBudget() == doctest::Approx(expectedDaily));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(expectedWeekly));
}
