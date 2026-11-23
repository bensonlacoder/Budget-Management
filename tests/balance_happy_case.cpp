// Balance happy case tests
// Original test contributors: Benson, Nathan Gu, and the SPECIFICATIONS.md test cases

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
