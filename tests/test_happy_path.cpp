
/*
M2: Happy Path Unit Tests
Description: Verifies that the core Balance and Budget
features behave correctly with valid inputs.

Original test contributors:
- Benson
- Nathan Gu
- Jackie Munoz Arevalo
*/

#include "doctest.h"
#include "balance.h"
#include "budget.h"

// Benson: Setting the balance replaces the old balance
TEST_CASE("Balance can be set to a new number") {
    Balance balance(725.0);
    CHECK(balance.getBalance() == 725.0);

    balance.setBalance(300.0);
    CHECK(balance.getBalance() == 300.0);
}

// Nathan Gu: Setting, adding, and subtracting balance
TEST_CASE("Balance can be set, added to, and subtracted from") {
    Balance balance;
    balance.setBalance(500.0);
    CHECK(balance.getBalance() == 500.0);

    balance.add(50.0);
    CHECK(balance.getBalance() == 550.0);

    balance.subtract(12.5);
    CHECK(balance.getBalance() == 537.5);
}

// Nathan Gu: Budget initialization with valid inputs
TEST_CASE("Budget can be initialized with a valid balance and remaining days") {
    Balance balance(500.0);
    Budget budget(balance, 30);

    CHECK(budget.getRemainingDays() == 30);
    CHECK(budget.getDailyBudget() ==
          doctest::Approx(500.0 / 30));
}

// Jackie: Daily and weekly budget calculations
TEST_CASE("Budget calculates daily and weekly allowances") {
    Balance balance;
    balance.setBalance(700.0);
    Budget budget(balance, 70);

    CHECK(budget.getDailyBudget() == 10.0);
    CHECK(budget.getWeeklyBudget() == 70.0);

    budget.setRemainingDays(3);
    CHECK(budget.getWeeklyBudget() == 700.0);
}
