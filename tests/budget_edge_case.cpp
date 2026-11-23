// Budget error and edge case tests
// Original test contributors: Jeong, and the SPECIFICATIONS.md test cases
// Recommendations are rounded down to the nearest hundredth of a point.

#include "doctest.h"
#include "balance.h"
#include "budget.h"

#include <cmath>
#include <stdexcept>

TEST_CASE("Ninety remaining days is accepted by construction and updates") {
    Balance balance(700.0);
    Budget budget(balance, 90);
    CHECK(budget.getRemainingDays() == 90);
    CHECK(budget.getDailyBudget() == doctest::Approx(7.77));   // 7.777...
    CHECK(budget.getWeeklyBudget() == doctest::Approx(54.44)); // 54.444...

    budget.setRemainingDays(30);
    CHECK(budget.getDailyBudget() == doctest::Approx(23.33));   // 23.333...
    CHECK(budget.getWeeklyBudget() == doctest::Approx(163.33)); // 163.333...
    budget.setRemainingDays(90);
    CHECK(budget.getRemainingDays() == 90);
    CHECK(budget.getDailyBudget() == doctest::Approx(7.77));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(54.44));
}

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
            CHECK(budget.getDailyBudget() == doctest::Approx(std::floor(70000.0 / days) / 100.0));
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

TEST_CASE("Remaining days cannot exceed 90") {
    Balance balance(500.0);
    CHECK_THROWS_AS(Budget(balance, 91), std::invalid_argument);
    Budget budget(balance, 30);

    CHECK_THROWS_AS(budget.setRemainingDays(91),
                    std::invalid_argument);

    CHECK(budget.getRemainingDays() == 30);
}

TEST_CASE("SP-04: 0, 1 and 90 remaining days are accepted") {
    Balance balance(700);
    Budget budget(balance, 70);
    for (int days : {0, 1, 90}) {
        CAPTURE(days);
        CHECK_NOTHROW(budget.setRemainingDays(days));
        CHECK(budget.getRemainingDays() == days);
    }
}

TEST_CASE("Recommendations round down so following them never overspends") {
    Balance balance(700.0);
    Budget budget(balance, 77);
    CHECK(budget.getDailyBudget() == doctest::Approx(9.09));   // 9.0909...
    CHECK(budget.getWeeklyBudget() == doctest::Approx(63.63)); // 63.636...
    CHECK(budget.getDailyBudget() * 77 <= balance.getBalance());

    // 0.29 is stored as 0.28999... and must still give 0.29, not 0.28.
    balance.setBalance(0.29);
    budget.setRemainingDays(1);
    CHECK(budget.getDailyBudget() == doctest::Approx(0.29));
    balance.setBalance(2.03);
    budget.setRemainingDays(7);
    CHECK(budget.getDailyBudget() == doctest::Approx(0.29));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(2.03));
}
