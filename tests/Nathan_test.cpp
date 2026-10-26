#include "doctest.h"

#include <stdexcept>

#include "balance.h"
#include "budget.h"

// normal case: set, add, and subtract update the balance
TEST_CASE("Balance can be set, added to, and subtracted from") {
    Balance balance;
    balance.setBalance(500.0);
    CHECK(balance.getBalance() == 500.0);

    balance.add(50.0);
    CHECK(balance.getBalance() == 550.0);

    balance.subtract(12.5);
    CHECK(balance.getBalance() == 537.5);
}

// normal case: Budget can be initialized with a valid balance and remaining days
TEST_CASE("Budget can be initialized with a valid balance and remaining days") {
    Balance balance(500.0);
    Budget budget(balance, 30);
    CHECK(budget.getRemainingDays() == 30);
    CHECK(budget.getDailyBudget() == doctest::Approx(500.0 / 30));
}