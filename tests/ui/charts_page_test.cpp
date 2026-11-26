#include "doctest.h"
#include "chartspage.h"
#include "test_account_store.h"

#include <QDateEdit>
#include <QTableWidget>

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
