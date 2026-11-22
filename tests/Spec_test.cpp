// Test cases written exactly as stated in SPECIFICATIONS.md, plus regression
// tests for two-decimal rounding of the balance.

#include "doctest.h"

#include <limits>
#include <stdexcept>

#include "balance.h"
#include "budget.h"

TEST_CASE("SP-01: a new balance is 0 and can be set to 700") {
    Balance balance;
    CHECK(balance.getBalance() == 0.0);
    balance.setBalance(700);
    CHECK(balance.getBalance() == 700.0);
}

TEST_CASE("SP-02: successive changes use the latest balance and can spend it all") {
    Balance balance(700);
    balance.add(50);
    CHECK(balance.getBalance() == 750.0);
    balance.subtract(25);
    CHECK(balance.getBalance() == 725.0);
    balance.subtract(725);
    CHECK(balance.getBalance() == 0.0);
}

TEST_CASE("SP-03: overspending and negative amounts are rejected without changing the balance") {
    Balance balance(725);
    CHECK_THROWS_AS(balance.subtract(726), std::invalid_argument);
    CHECK_THROWS_AS(balance.add(-5), std::invalid_argument);
    CHECK_THROWS_AS(balance.subtract(-5), std::invalid_argument);
    CHECK(balance.getBalance() == 725.0);
}

TEST_CASE("SP-04: 0, 1 and 90 remaining days are accepted") {
    Balance balance(700);
    Budget budget(balance, 70);
    for (int days : {0, 1, 90}) {
        CAPTURE(days);
        CHECK_NOTHROW(budget.setRemainingDays(days));
        CHECK(budget.getRemainingDays() == days);
    }
    CHECK(budget.getDailyBudget() == doctest::Approx(700.0 / 90));
}

TEST_CASE("Recommendations follow balance changes made after the budget is created") {
    Balance balance(700);
    Budget budget(balance, 70);
    balance.subtract(140);
    CHECK(budget.getDailyBudget() == doctest::Approx(8.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(56.0));
}

TEST_CASE("A displayed balance can always be spent in full") {
    // 0.7 + 0.1 is 0.7999... in binary floating point. The balance shows 0.80,
    // so spending 0.80 must succeed and leave exactly 0.
    Balance balance(0.7);
    balance.add(0.1);
    CHECK(balance.getBalance() == 0.8);
    CHECK_NOTHROW(balance.subtract(0.8));
    CHECK(balance.getBalance() == 0.0);

    Balance many;
    for (int i = 0; i < 10; ++i) {
        many.add(0.1);
    }
    CHECK(many.getBalance() == 1.0);
}

TEST_CASE("Balance values are kept to two decimal places") {
    Balance balance(10.006);
    CHECK(balance.getBalance() == 10.01);
    balance.setBalance(3.333);
    CHECK(balance.getBalance() == 3.33);
    balance.subtract(0.004); // rounds to 0.00 points
    CHECK(balance.getBalance() == 3.33);
}

TEST_CASE("Non-finite amounts are rejected without changing the balance") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    CHECK_THROWS_AS(Balance{nan}, std::invalid_argument);
    Balance balance(50);
    CHECK_THROWS_AS(balance.setBalance(nan), std::invalid_argument);
    CHECK_THROWS_AS(balance.setBalance(infinity), std::invalid_argument);
    CHECK_THROWS_AS(balance.add(nan), std::invalid_argument);
    CHECK_THROWS_AS(balance.add(infinity), std::invalid_argument);
    CHECK_THROWS_AS(balance.subtract(nan), std::invalid_argument);
    CHECK(balance.getBalance() == 50.0);
}
