#include "doctest.h"

#include <stdexcept>

#include "balance.h"
#include "budget.h"

TEST_CASE("Zero remaining days recommends the full balance without spending points") {
    Balance balance(700.0);
    Budget budget(balance, 0);

    CHECK(budget.getRemainingDays() == 0);
    CHECK(budget.getDailyBudget() == 700.0);
    CHECK(budget.getWeeklyBudget() == 700.0);
    CHECK(balance.getBalance() == 700.0);

    budget.setRemainingDays(1);
    CHECK(budget.getDailyBudget() == 700.0);
    CHECK(budget.getWeeklyBudget() == 700.0);

    budget.setRemainingDays(0);
    balance.setBalance(350.0);
    CHECK(budget.getDailyBudget() == 350.0);
    CHECK(budget.getWeeklyBudget() == 350.0);
    balance.setBalance(0.0);
    CHECK(budget.getDailyBudget() == 0.0);
    CHECK(budget.getWeeklyBudget() == 0.0);
}

TEST_CASE("Weekly budget uses the full balance for fewer than seven remaining days") {
    Balance balance(700.0);
    Budget budget(balance, 0);
    for (int days = 0; days < 7; ++days) {
        CAPTURE(days);
        budget.setRemainingDays(days);
        CHECK(budget.getWeeklyBudget() == 700.0);
        if (days > 0) {
            CHECK(budget.getDailyBudget() == doctest::Approx(700.0 / days));
        }
    }
    budget.setRemainingDays(7);
    CHECK(budget.getDailyBudget() == 100.0);
    CHECK(budget.getWeeklyBudget() == 700.0);
    budget.setRemainingDays(8);
    CHECK(budget.getWeeklyBudget() == 612.5);
}

TEST_CASE("Negative remaining days are rejected and preserve the previous count") {
    Balance balance(700.0);
    CHECK_THROWS_AS(Budget(balance, -1), std::invalid_argument);

    Budget budget(balance, 0);
    CHECK_THROWS_AS(budget.setRemainingDays(-1), std::invalid_argument);
    CHECK(budget.getRemainingDays() == 0);
}
