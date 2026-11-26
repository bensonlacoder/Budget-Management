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
    REQUIRE(LocalAccounts::saveBalance(account, balance, 70));
    REQUIRE(LocalAccounts::saveRemainingDays(account, 7));

    ChartsPage page(account);
    page.findChild<QDateEdit *>("chartsFrom")->setDate(today.addDays(-2));
    auto *comparison = page.findChild<QTableWidget *>("budgetComparison");
    REQUIRE(comparison->rowCount() == 3);
    CHECK(comparison->item(0, 3)->text().endsWith("under"));
    CHECK(comparison->item(1, 3)->text() == "On budget");
    CHECK(comparison->item(2, 3)->text().endsWith("over"));
}
