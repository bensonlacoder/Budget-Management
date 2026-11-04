// Balance error and edge case tests
// Original test contributors: Benson, Jeong

#include "doctest.h"
#include "balance.h"

#include <stdexcept>

TEST_CASE("Balance cannot be initialized with a negative amount") {
    CHECK_THROWS_AS(Balance(-5.0), std::invalid_argument);
}

TEST_CASE("Invalid additions and purchases preserve the balance") {
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
