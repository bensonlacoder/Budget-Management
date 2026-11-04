
/*
M2: Error and Edge Case Unit Tests
Description: Verifies that Balance and Budget handle
invalid inputs and boundary conditions correctly.

Original test contributors:
- Benson
- Jeong
*/

#include "doctest.h"
#include <stdexcept>
#include "balance.h"
#include "budget.h"

// Benson: Adding or spending zero leaves balance unchanged
TEST_CASE("Balance stays the same when adding or spending zero") {
    Balance balance(725.0);

    balance.add(0.0);
    CHECK(balance.getBalance() == 725.0);

    balance.subtract(0.0);
    CHECK(balance.getBalance() == 725.0);
}

// Jeong: Cannot set balance to a negative number
// and the original balance should remain unchanged
TEST_CASE("Balance cannot be set to a negative amount") {
    Balance balance;
    balance.setBalance(500.0);

    CHECK_THROWS_AS(balance.setBalance(-50.0),
                    std::invalid_argument);

    CHECK(balance.getBalance() == 500.0);
}

// Jeong: Remaining days cannot exceed 90
// and the original remaining days should remain unchanged
TEST_CASE("Remaining days cannot exceed 90") {
    Balance balance(500.0);
    Budget budget(balance, 30);

    CHECK_THROWS_AS(budget.setRemainingDays(91),
                    std::invalid_argument);

    CHECK(budget.getRemainingDays() == 30);
}
