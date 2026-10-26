#include "doctest.h"

#include "balance.h"

// normal case: setting the balance replaces the old balance
TEST_CASE("Balance can be set to a new number") {
    Balance balance(725.0);
    CHECK(balance.getBalance() == 725.0);

    balance.setBalance(300.0);
    CHECK(balance.getBalance() == 300.0);
}

// edge case: adding or spending zero leaves the balance the same
TEST_CASE("Balance stays the same when adding or spending zero") {
    Balance balance(725.0);
    balance.add(0.0);
    CHECK(balance.getBalance() == 725.0);

    balance.subtract(0.0);
    CHECK(balance.getBalance() == 725.0);
}