// Balance happy case tests
// Original test contributors: Benson, Nathan Gu

#include "doctest.h"
#include "balance.h"

TEST_CASE("Balance can be set to a new number") {
    Balance balance(725.0);
    CHECK(balance.getBalance() == 725.0);

    balance.setBalance(300.0);
    CHECK(balance.getBalance() == 300.0);
}

TEST_CASE("Balance can be set, added to, and subtracted from") {
    Balance balance;
    CHECK(balance.getBalance() == 0.0);
    balance.setBalance(500.0);
    CHECK(balance.getBalance() == 500.0);

    balance.add(50.0);
    CHECK(balance.getBalance() == 550.0);

    balance.subtract(12.5);
    CHECK(balance.getBalance() == 537.5);

    balance.subtract(537.5);
    CHECK(balance.getBalance() == 0.0);
}
