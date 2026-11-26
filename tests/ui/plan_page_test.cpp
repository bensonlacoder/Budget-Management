// Tests for the Plan tab (T06): real advice, the reserve goal and the end-of-quarter
// projection. Each test uses a temporary database.

#include "doctest.h"
#include "planpage.h"
#include "test_account_store.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>

using LocalAccounts::CorrectionMode;
using LocalAccounts::HistoryEntry;
using LocalAccounts::Result;

namespace {
struct PlanUi {
    PlanPage &page;

    template <typename T> T *find(const char *name) {
        auto *widget = page.findChild<T *>(name);
        REQUIRE_MESSAGE(widget, name);
        return widget;
    }
    QString text(const char *label) { return find<QLabel>(label)->text(); }
    void saveReserve(double points) {
        find<QDoubleSpinBox>("reserveGoalSpinBox")->setValue(points);
        find<QPushButton>("saveReserveButton")->click();
    }
};

// An Apartment Plan account (336 points) with `points` and `days` left. The balance
// is raised with Add, which doesn't count as spending.
QString accountWith(TestAccountStore &store, double points, int days) {
    const auto created = store.createVerified("plan@scu.edu", "Planner", "apartment");
    REQUIRE_FALSE(created.accountId.isEmpty());
    double balance = 0.0;
    REQUIRE(LocalAccounts::correctBalance(created.accountId, CorrectionMode::Add,
                                          points - created.balance, balance)
            == Result::Success);
    REQUIRE(LocalAccounts::saveRemainingDays(created.accountId, days));
    return created.accountId;
}

void spendToday(const QString &account, double points) {
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordPurchase(
                account, {QDate::currentDate(), "Marketplace", "Lunch", points}, balance)
            == Result::Success);
}

double savedReserve(const QString &account) {
    LocalAccounts::PlanSettings settings;
    REQUIRE(LocalAccounts::loadPlanSettings(account, settings));
    return settings.reserveGoal;
}
}

// Author: Nathan Gu
TEST_CASE("The Plan tab shows the real balance, days left and advice") {
    TestAccountStore store;
    const QString account = accountWith(store, 700.0, 70);
    PlanPage page(account);
    PlanUi ui{page};

    CHECK(ui.text("planBalanceLabel") == "700.00 points");
    CHECK(ui.text("planDaysLabel") == "70 days");
    CHECK(ui.text("spendableLabel") == "700.00 points");
    CHECK(ui.text("dailyBudgetLabel") == "10.00 points");
    CHECK(ui.text("weeklyBudgetLabel") == "70.00 points");
    CHECK(ui.find<QDoubleSpinBox>("reserveGoalSpinBox")->value() == 0.0);
    CHECK(ui.find<QLabel>("reserveWarningLabel")->isHidden());
}

// Author: Nathan Gu
TEST_CASE("The Plan tab refreshes after the balance or days change elsewhere") {
    TestAccountStore store;
    const QString account = accountWith(store, 700.0, 70);
    PlanPage page(account);
    PlanUi ui{page};

    double balance = 0.0;
    REQUIRE(LocalAccounts::correctBalance(account, CorrectionMode::Add, 70.0, balance)
            == Result::Success);
    REQUIRE(LocalAccounts::saveRemainingDays(account, 7));
    page.refresh();
    CHECK(ui.text("planBalanceLabel") == "770.00 points");
    CHECK(ui.text("planDaysLabel") == "7 days");
    CHECK(ui.text("dailyBudgetLabel") == "110.00 points");
}

// Author: Nathan Gu
TEST_CASE("A saved reserve goal is left out of the advice and kept after reopening") {
    TestAccountStore store;
    const QString account = accountWith(store, 700.0, 70);
    LocalAccounts::PlanSettings before;
    before.habits = {2, 2, 2, 2, 2, 0, 0};
    before.alertOverDailyBudget = true;
    REQUIRE(LocalAccounts::savePlanSettings(account, before) == Result::Success);
    {
        PlanPage page(account);
        PlanUi ui{page};
        ui.saveReserve(140.0);
        CHECK(ui.text("reserveMessage")
              == "Reserve saved. You'll keep 140.00 points at the end of the quarter.");
        CHECK(ui.text("spendableLabel") == "560.00 points");
        CHECK(ui.text("dailyBudgetLabel") == "8.00 points");
        CHECK(ui.text("weeklyBudgetLabel") == "56.00 points");
    }
    LocalAccounts::PlanSettings after;
    REQUIRE(LocalAccounts::loadPlanSettings(account, after));
    CHECK(after.reserveGoal == doctest::Approx(140.0));
    CHECK(after.habits == before.habits); // Saving the reserve keeps the other settings.
    CHECK(after.alertOverDailyBudget);

    PlanPage reopened(account);
    PlanUi ui{reopened};
    CHECK(ui.find<QDoubleSpinBox>("reserveGoalSpinBox")->value() == doctest::Approx(140.0));
    CHECK(ui.text("dailyBudgetLabel") == "8.00 points");
}

// Author: Nathan Gu
TEST_CASE("A reserve larger than the balance is rejected and the previous one is kept") {
    TestAccountStore store;
    const QString account = accountWith(store, 700.0, 70);
    PlanPage page(account);
    PlanUi ui{page};
    ui.saveReserve(100.0);

    ui.saveReserve(700.01);
    CHECK(ui.text("reserveMessage")
          == "Your reserve can't be more than your 700.00 points. Your previous reserve "
             "of 100.00 points has been kept.");
    CHECK(ui.find<QDoubleSpinBox>("reserveGoalSpinBox")->value() == doctest::Approx(100.0));
    CHECK(savedReserve(account) == doctest::Approx(100.0));
    CHECK(ui.text("spendableLabel") == "600.00 points");

    // The spin box itself can't go below 0.
    CHECK(ui.find<QDoubleSpinBox>("reserveGoalSpinBox")->minimum() == 0.0);
}

// Author: Nathan Gu
TEST_CASE("A saved reserve above a balance that has since fallen leaves nothing to spend") {
    TestAccountStore store;
    const QString account = accountWith(store, 700.0, 70);
    LocalAccounts::PlanSettings settings;
    settings.reserveGoal = 500.0;
    REQUIRE(LocalAccounts::savePlanSettings(account, settings) == Result::Success);
    spendToday(account, 300.0);

    PlanPage page(account);
    PlanUi ui{page};
    CHECK(ui.text("planBalanceLabel") == "400.00 points");
    CHECK(ui.find<QDoubleSpinBox>("reserveGoalSpinBox")->value() == doctest::Approx(500.0));
    CHECK(ui.text("spendableLabel") == "0.00 points");
    CHECK(ui.text("dailyBudgetLabel") == "0.00 points");
}

// Author: Nathan Gu
TEST_CASE("The reserve warning appears only when recent spending would end below it") {
    TestAccountStore store;
    // 763 points, then 63 spent today: 700 left and 9 points a day over the last week.
    const QString account = accountWith(store, 763.0, 70);
    spendToday(account, 63.0);
    PlanPage page(account);
    PlanUi ui{page};
    auto *warning = ui.find<QLabel>("reserveWarningLabel");

    CHECK(ui.text("projectionLabel")
          == "At your recent spending of 9.00 points a day, you'd end the quarter with "
             "70.00 points.");
    CHECK(warning->isHidden()); // No reserve yet.

    ui.saveReserve(140.0);
    CHECK_FALSE(warning->isHidden());
    CHECK(warning->text()
          == "That's less than your reserve goal of 140.00 points. Spend up to 8.00 points "
             "a day to keep it.");

    ui.saveReserve(70.0); // Ending with exactly the reserve is fine.
    CHECK(warning->isHidden());
}

// Author: Nathan Gu
TEST_CASE("The projection says when the points would run out") {
    TestAccountStore store;
    const QString account = accountWith(store, 777.0, 70);
    spendToday(account, 77.0); // 11 a day; 700 - 11 x 70 is below 0.
    PlanPage page(account);
    PlanUi ui{page};
    CHECK(ui.text("projectionLabel")
          == "At your recent spending of 11.00 points a day, you'd run out of points "
             "before the quarter ends.");
}

// Author: Nathan Gu
TEST_CASE("SP-07: the recent spending rate counts the last 7 days of net spending") {
    const QDate today(2026, 11, 25);
    using Kind = HistoryEntry::Kind;
    const QList<HistoryEntry> history = {
        {Kind::Purchase, today, "Marketplace", "Lunch", -50.0, "p1"},
        {Kind::Purchase, today.addDays(-6), "Marketplace", "Dinner", -30.0, "p2"},
        {Kind::Purchase, today.addDays(-7), "Marketplace", "Too old", -1000.0, "p3"},
        {Kind::Refund, today.addDays(-1), "", "Wrong order", 10.0, "r1"},
        {Kind::Correction, today.addDays(-2), "", "", -7.0, "c1"},  // Unlogged spending.
        {Kind::Correction, today.addDays(-3), "", "", 500.0, "c2"}, // Not spending.
    };
    // (50 + 30 - 10 + 7) / 7 = 11.
    CHECK(PlanPage::recentSpendingRate(history, today) == doctest::Approx(11.0));
    CHECK(PlanPage::recentSpendingRate({}, today) == 0.0);

    // More refunded than spent counts as no spending, not negative spending.
    const QList<HistoryEntry> refundsOnly = {
        {Kind::Refund, today, "", "Refund", 70.0, "r2"},
    };
    CHECK(PlanPage::recentSpendingRate(refundsOnly, today) == 0.0);
}
