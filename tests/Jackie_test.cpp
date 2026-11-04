// Name: Jackie Munoz Arevalo
// Unit Tests for FR-06, FR-07, FR-08
#include "doctest.h"
#include <stdexcept>
#include "balance.h"
#include "budget.h"

TEST_CASE("TEST: SP-04"){
  Balance balance;
  balance.setBalance(700.0);
  Budget budget(balance,70);
  CHECK(budget.getDailyBudget() == 10.0);
  CHECK(budget.getWeeklyBudget() == 70.0);

  budget.setRemainingDays(3);
  CHECK(budget.getWeeklyBudget() == 700.0);
;}
