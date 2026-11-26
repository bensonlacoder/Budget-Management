// Reserve goal, spending projection and dining habit tests (T06, T07; SP-07, SP-08).

#include "doctest.h"
#include "balance.h"
#include "budget.h"

#include <limits>
#include <stdexcept>

namespace {
constexpr int MONDAY = 1;
constexpr int SATURDAY = 6;
const Budget::DiningHabits WEEKDAYS_TWO_MEALS = {2, 2, 2, 2, 2, 0, 0};
}

// ---- Reserve goal ----

// Author: Nathan Gu
TEST_CASE("SP-07: the reserve goal defaults to 0 and leaves the advice unchanged") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    CHECK(budget.getReserveGoal() == 0.0);
    CHECK(budget.getSpendablePoints() == doctest::Approx(700.0));
    CHECK(budget.getDailyBudget() == doctest::Approx(10.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(70.0));
}

// Author: Nathan Gu
TEST_CASE("SP-07: advice leaves out the reserve goal") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(140.0);
    CHECK(budget.getReserveGoal() == doctest::Approx(140.0));
    CHECK(budget.getSpendablePoints() == doctest::Approx(560.0));
    CHECK(budget.getDailyBudget() == doctest::Approx(8.0));
    CHECK(budget.getWeeklyBudget() == doctest::Approx(56.0));

    // Fewer than 7 days, or none: the whole spendable amount, never the reserve.
    budget.setRemainingDays(3);
    CHECK(budget.getWeeklyBudget() == doctest::Approx(560.0));
    budget.setRemainingDays(0);
    CHECK(budget.getDailyBudget() == doctest::Approx(560.0));

    // A reserve equal to the balance is allowed and leaves nothing to spend.
    budget.setReserveGoal(700.0);
    CHECK(budget.getDailyBudget() == 0.0);
}

// Author: Nathan Gu
TEST_CASE("SP-07: advice with a reserve still rounds down to the hundredth") {
    Balance balance(500.0);
    Budget budget(balance, 30);
    budget.setReserveGoal(0.01);
    CHECK(budget.getSpendablePoints() == doctest::Approx(499.99));
    CHECK(budget.getDailyBudget() == doctest::Approx(16.66)); // 16.6663...
}

// Author: Nathan Gu
TEST_CASE("SP-07: an invalid reserve goal is rejected and the previous one is kept") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(100.0);
    CHECK_THROWS_AS(budget.setReserveGoal(-1.0), std::invalid_argument);
    CHECK_THROWS_AS(budget.setReserveGoal(701.0), std::invalid_argument);
    CHECK_THROWS_AS(budget.setReserveGoal(700.01), std::invalid_argument);
    CHECK_THROWS_AS(budget.setReserveGoal(std::numeric_limits<double>::quiet_NaN()),
                    std::invalid_argument);
    CHECK_THROWS_AS(budget.setReserveGoal(std::numeric_limits<double>::infinity()),
                    std::invalid_argument);
    CHECK(budget.getReserveGoal() == doctest::Approx(100.0));
}

// Author: Nathan Gu
TEST_CASE("SP-07: spending below the reserve leaves 0 spendable points, never less") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(140.0);
    balance.subtract(600.0);
    CHECK(budget.getSpendablePoints() == 0.0);
    CHECK(budget.getDailyBudget() == 0.0);
    CHECK(budget.getWeeklyBudget() == 0.0);
    // The goal itself is kept; the user decides whether to lower it.
    CHECK(budget.getReserveGoal() == doctest::Approx(140.0));
}

// ---- Spending rate and projection ----

// Author: Nathan Gu
TEST_CASE("SP-07: the warning shows only when the projection falls below the reserve") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setReserveGoal(140.0);

    budget.setRecentSpendingRate(9.0);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(70.0));
    CHECK(budget.isProjectedBelowReserve());
    CHECK_FALSE(budget.isProjectedToRunOut());

    // Exactly the reserve is not below it.
    budget.setRecentSpendingRate(8.0);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(140.0));
    CHECK_FALSE(budget.isProjectedBelowReserve());

    budget.setRecentSpendingRate(0.0);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(700.0));
    CHECK_FALSE(budget.isProjectedBelowReserve());
}

// Author: Nathan Gu
TEST_CASE("SP-07: a projection below 0 means the points run out before the quarter ends") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    CHECK(budget.getRecentSpendingRate() == 0.0);
    CHECK_FALSE(budget.isProjectedToRunOut());

    budget.setRecentSpendingRate(11.0);
    CHECK(budget.getProjectedEndBalance() == doctest::Approx(-70.0));
    CHECK(budget.isProjectedToRunOut());
    CHECK(budget.isProjectedBelowReserve()); // Even with no reserve.

    // Spending exactly the balance by the last day doesn't run out.
    budget.setRecentSpendingRate(10.0);
    CHECK_FALSE(budget.isProjectedToRunOut());
}

// Author: Nathan Gu
TEST_CASE("SP-07: an invalid spending rate is rejected and the previous one is kept") {
    Balance balance(700.0);
    Budget budget(balance, 70);
    budget.setRecentSpendingRate(5.0);
    CHECK_THROWS_AS(budget.setRecentSpendingRate(-0.01), std::invalid_argument);
    CHECK_THROWS_AS(budget.setRecentSpendingRate(std::numeric_limits<double>::infinity()),
                    std::invalid_argument);
    CHECK(budget.getRecentSpendingRate() == doctest::Approx(5.0));
}

// ---- Dining habits ----

// Author: Nathan Gu
TEST_CASE("SP-08: suggestions spread the spendable points across campus meals and days") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setReserveGoal(100.0);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK(budget.getDiningHabits() == WEEKDAYS_TWO_MEALS);

    CHECK(budget.getRemainingCampusDays(MONDAY) == 10);
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 20);
    CHECK(budget.getPerMealBudget(MONDAY) == doctest::Approx(20.0));
    CHECK(budget.getPerCampusDayBudget(MONDAY) == doctest::Approx(40.0));

    // Suggestions follow changes to the balance and days left.
    balance.add(100.0);
    CHECK(budget.getPerMealBudget(MONDAY) == doctest::Approx(25.0));
    budget.setRemainingDays(7);
    CHECK(budget.getPerCampusDayBudget(MONDAY) == doctest::Approx(100.0));
}

// Author: Nathan Gu
TEST_CASE("SP-08: campus days count from today's weekday and wrap into the next week") {
    Balance balance(90.0);
    Budget budget(balance, 3);
    budget.setDiningHabits({3, 0, 0, 0, 0, 0, 0}); // Mondays only.
    // Saturday, Sunday, Monday.
    CHECK(budget.getRemainingCampusDays(SATURDAY) == 1);
    CHECK(budget.getRemainingCampusMeals(SATURDAY) == 3);
    CHECK(budget.getPerMealBudget(SATURDAY) == doctest::Approx(30.0));
    // Monday, Tuesday, Wednesday.
    CHECK(budget.getRemainingCampusDays(MONDAY) == 1);
}

// Author: Nathan Gu
TEST_CASE("SP-08: without habits or campus days left, there are no suggestions") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 0);
    CHECK(budget.getPerMealBudget(MONDAY) == 0.0);
    CHECK(budget.getPerCampusDayBudget(MONDAY) == 0.0);

    // Weekdays only, but just the weekend is left.
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    budget.setRemainingDays(2);
    CHECK(budget.getRemainingCampusDays(SATURDAY) == 0);
    CHECK(budget.getPerMealBudget(SATURDAY) == 0.0);

    budget.setRemainingDays(0);
    CHECK(budget.getPerCampusDayBudget(MONDAY) == 0.0);
}

// Author: Nathan Gu
TEST_CASE("SP-08: invalid habits or weekdays are rejected without changing anything") {
    Balance balance(500.0);
    Budget budget(balance, 14);
    budget.setDiningHabits(WEEKDAYS_TWO_MEALS);
    CHECK_THROWS_AS(budget.setDiningHabits({2, 2, 11, 2, 2, 0, 0}), std::invalid_argument);
    CHECK_THROWS_AS(budget.setDiningHabits({2, -1, 2, 2, 2, 0, 0}), std::invalid_argument);
    CHECK(budget.getDiningHabits() == WEEKDAYS_TWO_MEALS);

    // 10 meals a day is the most allowed.
    budget.setDiningHabits({10, 0, 0, 0, 0, 0, 0});
    CHECK(budget.getRemainingCampusMeals(MONDAY) == 20);

    CHECK(budget.getRemainingCampusDays(0) == 0);
    CHECK(budget.getRemainingCampusMeals(8) == 0);
    CHECK(budget.getPerMealBudget(0) == 0.0);
}
