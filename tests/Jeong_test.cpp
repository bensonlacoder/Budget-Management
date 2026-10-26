// Name: Nathan Jeong
// Unit tests for Balance and Budget

#include "doctest.h"


#include "doctest.h"

#include <stdexcept>

#include "balance.h"
#include "budget.h"

// negative case: setting a negative balance should throw an error
// and keep the original balance
TEST_CASE("Balance cannot be set to a negative amount") {
    Balance balance;
    balance.setBalance(500.0);

    CHECK_THROWS_AS(balance.setBalance(-50.0), std::invalid_argument);
    CHECK(balance.getBalance() == 500.0);
}

// negative case: remaining days cannot exceed 90
// and should keep the original remaining days
TEST_CASE("Remaining days cannot exceed 90") {
    Balance balance(500.0);
    Budget budget(balance, 30);

    CHECK_THROWS_AS(budget.setRemainingDays(91), std::invalid_argument);
    CHECK(budget.getRemainingDays() == 30);
}
