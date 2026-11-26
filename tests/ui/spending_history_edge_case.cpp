// Spending history calculation error and edge case tests (T10, T11, T13; SP-07, SP-11)
// Original test contributors: Nathan Gu

#include "doctest.h"
#include "chartspage.h"
#include "historylocations.h"
#include "planpage.h"

using Entry = LocalAccounts::HistoryEntry;
using Kind = Entry::Kind;

namespace {
const QDate TODAY(2026, 11, 25); // A Wednesday.
}

TEST_CASE("Entries outside the range, and corrections that raised the balance, aren't spending") {
    const QList<Entry> history = {
        {Kind::Purchase, TODAY.addDays(-1), "A", {}, -5, "before"},
        {Kind::Purchase, TODAY.addDays(1), "A", {}, -5, "after"},
        {Kind::Correction, TODAY, {}, {}, 100, "raised"},
    };
    const auto spending = ChartsPage::summarize(history, TODAY, TODAY, false);
    CHECK(spending.periods.isEmpty());
    CHECK(spending.locations.isEmpty());
}

TEST_CASE("The range includes both of its end dates") {
    const QList<Entry> history = {
        {Kind::Purchase, TODAY.addDays(-2), "A", {}, -1, "first"},
        {Kind::Purchase, TODAY, "A", {}, -2, "last"},
    };
    const auto spending = ChartsPage::summarize(history, TODAY.addDays(-2), TODAY, false);
    CHECK(spending.locations.value("A") == 300);
}

TEST_CASE("Weeks are keyed by their Monday, and a refund can make a period negative") {
    const QDate sunday(2026, 11, 29);
    const QList<Entry> history = {
        {Kind::Purchase, sunday, "A", {}, -3, "p1"},
        {Kind::Refund, sunday, "A", {}, 5, "r1"},
    };
    const auto weekly = ChartsPage::summarize(history, sunday, sunday, true);
    REQUIRE(weekly.periods.size() == 1);
    CHECK(weekly.periods.firstKey() == QDate(2026, 11, 23)); // Monday.
    CHECK(weekly.periods.first() == -200);
}

TEST_CASE("Empty history gives no totals") {
    const auto spending = ChartsPage::summarize({}, TODAY.addDays(-7), TODAY, true);
    CHECK(spending.periods.isEmpty());
    CHECK(spending.locations.isEmpty());
}

TEST_CASE("Spending more today changes no past budget, and not today's either") {
    QList<Entry> history = {
        {Kind::Purchase, TODAY.addDays(-1), "A", {}, -10, "p1"},
        {Kind::Purchase, TODAY, "A", {}, -12, "p2"},
    };
    const auto before = ChartsPage::dailyBudgets(history, 58, 0, 7, TODAY, TODAY.addDays(-1), TODAY);
    history.append({Kind::Purchase, TODAY, "A", {}, -30, "p3"});
    const auto after = ChartsPage::dailyBudgets(history, 28, 0, 7, TODAY, TODAY.addDays(-1), TODAY);
    CHECK(after == before);
}

TEST_CASE("Past budgets count refunds and corrections in both directions") {
    const QList<Entry> history = {
        {Kind::Purchase, TODAY.addDays(-1), "A", {}, -20, "p1"},
        {Kind::Refund, TODAY, "A", {}, 5, "r1", "p1"},
        {Kind::Correction, TODAY, {}, {}, -15, "c1"},
        {Kind::Correction, TODAY, {}, {}, 30, "c2"},
    };
    // 130 now: today started at 130 - 5 + 15 - 30 = 110, yesterday at 130.
    const auto budgets =
        ChartsPage::dailyBudgets(history, 130, 20, 9, TODAY, TODAY.addDays(-1), TODAY);
    CHECK(budgets.value(TODAY) == 1000);              // (110 - 20) / 9
    CHECK(budgets.value(TODAY.addDays(-1)) == 1100);  // (130 - 20) / 10
}

TEST_CASE("Days after today use today's budget") {
    const auto budgets = ChartsPage::dailyBudgets({}, 100, 0, 10, TODAY, TODAY, TODAY.addDays(2));
    CHECK(budgets.value(TODAY.addDays(1)) == budgets.value(TODAY));
    CHECK(budgets.value(TODAY.addDays(2)) == 1000);
}

TEST_CASE("A reserve above a day's balance, or no days left, give a budget of 0 or all of it") {
    const auto reserved = ChartsPage::dailyBudgets({}, 100, 120, 9, TODAY, TODAY, TODAY);
    CHECK(reserved.value(TODAY) == 0);
    const auto lastDay = ChartsPage::dailyBudgets({}, 100, 20, 0, TODAY, TODAY, TODAY);
    CHECK(lastDay.value(TODAY) == 8000); // Everything after the reserve.
}

TEST_CASE("Days left far in the past stop at 90") {
    // 85 left today; 10 days ago would be 95, past Budget's limit of 90.
    const auto budgets =
        ChartsPage::dailyBudgets({}, 900, 0, 85, TODAY, TODAY.addDays(-10), TODAY.addDays(-10));
    CHECK(budgets.value(TODAY.addDays(-10)) == 1000); // 900 / 90
}

TEST_CASE("A range that ends before it starts has no days") {
    CHECK(ChartsPage::dailyBudgets({}, 100, 0, 10, TODAY, TODAY, TODAY.addDays(-1)).isEmpty());
}

TEST_CASE("Net spending can be negative, and a range with nothing in it is 0") {
    const QList<Entry> refundsOnly = {{Kind::Refund, TODAY, "", "Refund", 70.0, "r1"}};
    CHECK(PlanPage::netSpending(refundsOnly, TODAY, TODAY) == doctest::Approx(-70.0));
    CHECK(PlanPage::netSpending(refundsOnly, TODAY.addDays(-3), TODAY.addDays(-1)) == 0.0);
    CHECK(PlanPage::netSpending({}, TODAY, TODAY) == 0.0);
}

TEST_CASE("The 7-day window includes 6 days ago but not 7") {
    const QList<Entry> history = {
        {Kind::Purchase, TODAY.addDays(-6), "A", {}, -7, "inside"},
        {Kind::Purchase, TODAY.addDays(-7), "A", {}, -1000, "outside"},
        {Kind::Purchase, TODAY.addDays(1), "A", {}, -1000, "future"},
    };
    CHECK(PlanPage::recentSpendingRate(history, TODAY) == doctest::Approx(1.0));
}

TEST_CASE("More refunded than spent gives a rate of 0, not a negative one") {
    const QList<Entry> history = {{Kind::Refund, TODAY, "", "Refund", 70.0, "r1"}};
    CHECK(PlanPage::recentSpendingRate(history, TODAY) == 0.0);
    CHECK(PlanPage::recentSpendingRate({}, TODAY) == 0.0);
}

TEST_CASE("Refunds without a purchase in the list are left without a location") {
    QList<Entry> history = {
        {Kind::Refund, TODAY, {}, "Standalone", 5, "r1"},
        {Kind::Refund, TODAY, {}, "Old purchase", 5, "r2", "not-in-list"},
        {Kind::Correction, TODAY, {}, {}, -5, "c1"},
    };
    resolveRefundLocations(history);
    CHECK(history.at(0).location.isEmpty());
    CHECK(history.at(1).location.isEmpty());
    CHECK(history.at(2).location.isEmpty());
}

TEST_CASE("A refund's stale location is replaced by its purchase's current one") {
    QList<Entry> history = {
        {Kind::Refund, TODAY, "Old place", {}, 5, "r1", "p1"},
        {Kind::Purchase, TODAY.addDays(-1), "New place", {}, -5, "p1"},
    };
    resolveRefundLocations(history);
    CHECK(history.at(0).location == "New place");
}
