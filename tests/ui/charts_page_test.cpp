#include "doctest.h"
#include "chartspage.h"
#include "test_account_store.h"

#include <QDateEdit>
#include <QTableWidget>

// Author: Nathan Jeong
TEST_CASE("Location and weekly totals use net spending within the selected dates") {
    using Entry = LocalAccounts::HistoryEntry;
    using Kind = Entry::Kind;
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

// Author: Nathan Jeong
TEST_CASE("Budget comparison labels days over, under and on budget") {
    TestAccountStore store;
    const auto profile = store.createVerified("charts@example.com", "Student");
    const auto today = QDate::currentDate();
    const auto account = profile.accountId;
    double balance = 0;
    REQUIRE(LocalAccounts::recordPurchase(account, {today.addDays(-2), "A", {}, 8}, balance)
            == LocalAccounts::Result::Success);
    REQUIRE(LocalAccounts::recordPurchase(account, {today.addDays(-1), "A", {}, 10}, balance)
            == LocalAccounts::Result::Success);
    REQUIRE(LocalAccounts::recordPurchase(account, {today, "A", {}, 12}, balance)
            == LocalAccounts::Result::Success);
    // 58 left with 7 days: each day's budget at its start is 88/9 = 9.77 (under 8),
    // 80/8 = 10 (exactly the 10 spent) and 70/7 = 10 (under the 12 spent today).
    REQUIRE(LocalAccounts::saveBalance(account, balance, 58));
    REQUIRE(LocalAccounts::saveRemainingDays(account, 7));

    ChartsPage page(account);
    page.findChild<QDateEdit *>("chartsFrom")->setDate(today.addDays(-2));
    auto *comparison = page.findChild<QTableWidget *>("budgetComparison");
    REQUIRE(comparison->rowCount() == 3);
    CHECK(comparison->item(0, 3)->text().endsWith("under"));
    CHECK(comparison->item(1, 3)->text() == "On budget");
    CHECK(comparison->item(2, 3)->text().endsWith("over"));
}

// Author: Nathan Gu
TEST_CASE("Each day is compared with the budget it had at its start") {
    using Entry = LocalAccounts::HistoryEntry;
    using Kind = Entry::Kind;
    const QDate today(2026, 11, 25);
    QList<Entry> history = {
        {Kind::Purchase, today.addDays(-2), "A", {}, -8, "p1"},
        {Kind::Purchase, today.addDays(-1), "A", {}, -10, "p2"},
        {Kind::Purchase, today, "A", {}, -12, "p3"},
    };
    auto budgets = ChartsPage::dailyBudgets(history, 58, 0, 7, today, today.addDays(-2), today);
    REQUIRE(budgets.size() == 3);
    CHECK(budgets.value(today.addDays(-2)) == 977); // 88 / 9
    CHECK(budgets.value(today.addDays(-1)) == 1000); // 80 / 8
    CHECK(budgets.value(today) == 1000);             // 70 / 7

    // Spending more today lowers no earlier day's budget, and not today's either:
    // today's budget is set at the start of the day.
    history.append({Kind::Purchase, today, "A", {}, -30, "p4"});
    budgets = ChartsPage::dailyBudgets(history, 28, 0, 7, today, today.addDays(-2), today);
    CHECK(budgets.value(today.addDays(-2)) == 977);
    CHECK(budgets.value(today.addDays(-1)) == 1000);
    CHECK(budgets.value(today) == 1000);
}

// Author: Nathan Gu
TEST_CASE("Past budgets count refunds, corrections and the reserve goal") {
    using Entry = LocalAccounts::HistoryEntry;
    using Kind = Entry::Kind;
    const QDate today(2026, 11, 25);
    const QList<Entry> history = {
        {Kind::Purchase, today.addDays(-1), "A", {}, -20, "p1"},
        {Kind::Refund, today, "A", {}, 5, "r1", "p1"},       // Raised the balance today.
        {Kind::Correction, today, {}, {}, -15, "c1"},         // Lowered it today.
    };
    // 100 left now: today started at 100 - 5 + 15 = 110, yesterday at 130.
    const auto budgets = ChartsPage::dailyBudgets(history, 100, 20, 9, today,
                                                  today.addDays(-1), today.addDays(1));
    CHECK(budgets.value(today) == 1000);              // (110 - 20) / 9
    CHECK(budgets.value(today.addDays(-1)) == 1100);  // (130 - 20) / 10
    CHECK(budgets.value(today.addDays(1)) == 1000);   // After today: today's budget.

    // A reserve above a past day's balance leaves nothing to spend that day.
    const auto reserved = ChartsPage::dailyBudgets(history, 100, 120, 9, today, today, today);
    CHECK(reserved.value(today) == 0);
}
