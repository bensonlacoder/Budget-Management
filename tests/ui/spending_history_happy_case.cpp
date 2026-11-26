// Spending history calculation happy case tests (T10, T11, T13; SP-07, SP-09, SP-11)
// Original test contributors: Nathan Jeong, Nathan Gu
// Covers ChartsPage::summarize, ChartsPage::dailyBudgets, PlanPage::netSpending,
// PlanPage::recentSpendingRate and resolveRefundLocations.

#include "doctest.h"
#include "chartspage.h"
#include "historylocations.h"
#include "planpage.h"

using Entry = LocalAccounts::HistoryEntry;
using Kind = Entry::Kind;

namespace {
const QDate TODAY(2026, 11, 25); // A Wednesday.
}

// Author: Nathan Jeong
TEST_CASE("Location and weekly totals use net spending within the selected dates") {
    const QDate sunday(2026, 11, 22);
    const QDate monday = sunday.addDays(1);
    const QList<Entry> history = {
        {Kind::Purchase, sunday, "Marketplace", {}, -10, "p1"},
        {Kind::Refund, monday, "Marketplace", {}, 4, "r1", "p1"},
        {Kind::Refund, monday, {}, {}, 1, "r2"},
        {Kind::Correction, monday, {}, {}, -2, "c1"},
        {Kind::Correction, monday, {}, {}, 30, "c2"},
        {Kind::Purchase, sunday.addDays(-1), "Outside", {}, -6, "p2"},
    };

    const auto weekly = ChartsPage::summarize(history, sunday, monday, true);
    CHECK(weekly.locations.size() == 3);
    CHECK(weekly.locations.value("Marketplace") == 600);
    CHECK(weekly.locations.value("Unlogged") == 200);
    CHECK(weekly.locations.value("Standalone refunds") == -100);
    CHECK(weekly.periods.size() == 2);
    CHECK(weekly.periods.value(sunday.addDays(-6)) == 1000);
    CHECK(weekly.periods.value(monday) == -300);
    const auto daily = ChartsPage::summarize(history, sunday, monday, false);
    CHECK(daily.periods.value(sunday) == 1000);
    CHECK(daily.periods.value(monday) == -300);
}

// Author: Nathan Gu
TEST_CASE("Daily totals add up every purchase on the same day and location") {
    const QList<Entry> history = {
        {Kind::Purchase, TODAY, "Marketplace", "Lunch", -11.25, "p1"},
        {Kind::Purchase, TODAY, "Marketplace", "Coffee", -3.50, "p2"},
        {Kind::Purchase, TODAY.addDays(-1), "The Bronco", "Burrito", -12, "p3"},
    };
    const auto daily = ChartsPage::summarize(history, TODAY.addDays(-1), TODAY, false);
    CHECK(daily.periods.value(TODAY) == 1475);
    CHECK(daily.periods.value(TODAY.addDays(-1)) == 1200);
    CHECK(daily.locations.value("Marketplace") == 1475);
    CHECK(daily.locations.value("The Bronco") == 1200);
}

// Author: Nathan Gu
TEST_CASE("Each day is compared with the budget it had at its start") {
    const QList<Entry> history = {
        {Kind::Purchase, TODAY.addDays(-2), "A", {}, -8, "p1"},
        {Kind::Purchase, TODAY.addDays(-1), "A", {}, -10, "p2"},
        {Kind::Purchase, TODAY, "A", {}, -12, "p3"},
    };
    const auto budgets =
        ChartsPage::dailyBudgets(history, 58, 0, 7, TODAY, TODAY.addDays(-2), TODAY);
    REQUIRE(budgets.size() == 3);
    CHECK(budgets.value(TODAY.addDays(-2)) == 977); // 88 / 9
    CHECK(budgets.value(TODAY.addDays(-1)) == 1000); // 80 / 8
    CHECK(budgets.value(TODAY) == 1000);             // 70 / 7
}

// Author: Nathan Gu
TEST_CASE("Past budgets leave out the reserve goal") {
    const QList<Entry> history = {{Kind::Purchase, TODAY, "A", {}, -20, "p1"}};
    // Today started at 110: (110 - 20) / 9 = 10.
    const auto budgets = ChartsPage::dailyBudgets(history, 90, 20, 9, TODAY, TODAY, TODAY);
    CHECK(budgets.value(TODAY) == 1000);
}

// Author: Nathan Gu
TEST_CASE("Net spending adds purchases and lowering corrections, minus refunds") {
    const QList<Entry> history = {
        {Kind::Purchase, TODAY, "A", {}, -50, "p1"},
        {Kind::Refund, TODAY, "A", {}, 10, "r1", "p1"},
        {Kind::Correction, TODAY.addDays(-1), {}, {}, -7, "c1"},
    };
    CHECK(PlanPage::netSpending(history, TODAY, TODAY) == doctest::Approx(40.0));
    CHECK(PlanPage::netSpending(history, TODAY.addDays(-1), TODAY) == doctest::Approx(47.0));
}

// Author: Nathan Gu
TEST_CASE("SP-07: the recent spending rate is the last 7 days of net spending over 7") {
    const QList<Entry> history = {
        {Kind::Purchase, TODAY, "Marketplace", "Lunch", -50.0, "p1"},
        {Kind::Purchase, TODAY.addDays(-6), "Marketplace", "Dinner", -30.0, "p2"},
        {Kind::Refund, TODAY.addDays(-1), "", "Wrong order", 10.0, "r1"},
        {Kind::Correction, TODAY.addDays(-2), "", "", -7.0, "c1"},
    };
    // (50 + 30 - 10 + 7) / 7 = 11.
    CHECK(PlanPage::recentSpendingRate(history, TODAY) == doctest::Approx(11.0));
    CHECK(PlanPage::SpendingWindowDays == 7);
}

// Author: Nathan Gu
TEST_CASE("A linked refund takes the location of its purchase") {
    QList<Entry> history = {
        {Kind::Purchase, TODAY, "Cafe Bernardo", "Latte", -6.5, "p1"},
        {Kind::Refund, TODAY, {}, "Wrong order", 6.5, "r1", "p1"},
    };
    resolveRefundLocations(history);
    CHECK(history.at(1).location == "Cafe Bernardo");
    CHECK(history.at(0).location == "Cafe Bernardo"); // The purchase is unchanged.
}
