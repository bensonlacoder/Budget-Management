// Balance error and edge case tests
// Original test contributors: Benson, Jeong, and the SPECIFICATIONS.md test cases

#include "doctest.h"
#include "balance.h"

#include <limits>
#include <stdexcept>

TEST_CASE("Balance cannot be initialized with a negative amount") {
    CHECK_THROWS_AS(Balance(-5.0), std::invalid_argument);
}

TEST_CASE("SP-03: invalid additions and purchases preserve the balance") {
    Balance balance(725.0);

    SUBCASE("Spending more than the balance") {
        CHECK_THROWS_AS(balance.subtract(726.0), std::invalid_argument);
    }
    SUBCASE("Adding a negative amount") {
        CHECK_THROWS_AS(balance.add(-5.0), std::invalid_argument);
    }
    SUBCASE("Spending a negative amount") {
        CHECK_THROWS_AS(balance.subtract(-5.0), std::invalid_argument);
    }

    CHECK(balance.getBalance() == 725.0);
}

TEST_CASE("Balance stays the same when adding or spending zero") {
    Balance balance(725.0);

    balance.add(0.0);
    CHECK(balance.getBalance() == 725.0);

    balance.subtract(0.0);
    CHECK(balance.getBalance() == 725.0);
}

TEST_CASE("Balance cannot be set to a negative amount") {
    Balance balance;
    balance.setBalance(500.0);

    CHECK_THROWS_AS(balance.setBalance(-50.0),
                    std::invalid_argument);

    CHECK(balance.getBalance() == 500.0);
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
