#include "doctest.h"

#include <stdexcept>

#include "balance.h"

// normal case: set, add, and subtract update the balance
TEST_CASE("Balance can be set, added to, and subtracted from") {
    Balance balance;
    balance.setBalance(500.0);
    CHECK(balance.getBalance() == doctest::Approx(500.0));

    balance.add(50.0);
    CHECK(balance.getBalance() == doctest::Approx(550.0));

    balance.subtract(12.5);
    CHECK(balance.getBalance() == doctest::Approx(537.5));
}

// wrong case: negative amounts and overspending are rejected
TEST_CASE("Balance rejects invalid amounts") {
    Balance balance(100.0);
    CHECK_THROWS_AS(balance.setBalance(-1.0), std::invalid_argument);
    CHECK_THROWS_AS(balance.add(-5.0), std::invalid_argument);
    CHECK_THROWS_AS(balance.subtract(-5.0), std::invalid_argument);
    CHECK_THROWS_AS(balance.subtract(100.01), std::invalid_argument);
    CHECK(balance.getBalance() == doctest::Approx(100.0));
}

// edge case: default balance is zero and can be spent down to exactly zero
TEST_CASE("Balance handles zero") {
    Balance empty;
    CHECK(empty.getBalance() == doctest::Approx(0.0));

    Balance balance(20.0);
    balance.subtract(20.0);
    CHECK(balance.getBalance() == doctest::Approx(0.0));
}
