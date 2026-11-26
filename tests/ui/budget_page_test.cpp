// Tests for the Budget tab sandbox (T04): its balance buttons and error messages, and
// that it starts from the real numbers but never saves anything.

#include "doctest.h"
#include "mainui.h"
#include "test_account_store.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>

namespace {
MainUI::RealNumbersLoader fixedNumbers(double balance, int days) {
    return [balance, days] { return std::optional<MainUI::RealNumbers>({balance, days}); };
}

struct BudgetPage {
    MainUI &window;
    QDoubleSpinBox *amount = window.findChild<QDoubleSpinBox *>("amountSpinBox");
    QLabel *balance = window.findChild<QLabel *>("balanceValueLabel");
    QLabel *daily = window.findChild<QLabel *>("dailyBudgetValueLabel");
    QSpinBox *days = window.findChild<QSpinBox *>("remainingDaysSpinBox");
    QLabel *message = window.findChild<QLabel *>("errorLabel");

    explicit BudgetPage(MainUI &w) : window(w) {
        REQUIRE(amount);
        REQUIRE(balance);
        REQUIRE(daily);
        REQUIRE(days);
        REQUIRE(message);
    }

    void press(const char *button, double value) {
        amount->setValue(value);
        click(button);
    }

    void click(const char *button) {
        auto *target = window.findChild<QPushButton *>(button);
        REQUIRE(target);
        target->click();
    }
};
}

TEST_CASE("Budget tab balance buttons update the balance and report success") {
    MainUI window(fixedNumbers(0.0, 77));
    BudgetPage page(window);

    page.press("setBalanceButton", 700);
    CHECK(page.balance->text() == "700.00 points");
    CHECK(page.message->text() == "Successfully Updated");
    page.press("addPointsButton", 50);
    CHECK(page.balance->text() == "750.00 points");
    page.press("spendPointsButton", 750);
    CHECK(page.balance->text() == "0.00 points");
}

TEST_CASE("Budget tab rejects overspending and blank amounts") {
    MainUI window(fixedNumbers(0.0, 77));
    BudgetPage page(window);
    page.press("setBalanceButton", 100);

    page.press("spendPointsButton", 150);
    CHECK(page.balance->text() == "100.00 points");
    CHECK(page.message->text() == "Not enough points. Enter an amount no greater than your balance.");

    page.amount->clear();
    page.click("addPointsButton");
    CHECK(page.balance->text() == "100.00 points");
    CHECK(page.message->text() == "Enter an amount first.");
}

TEST_CASE("Budget tab can spend a displayed balance built from cents") {
    MainUI window(fixedNumbers(0.0, 77));
    BudgetPage page(window);
    page.press("setBalanceButton", 0.70);
    page.press("addPointsButton", 0.10);
    CHECK(page.balance->text() == "0.80 points");
    page.press("spendPointsButton", 0.80);
    CHECK(page.balance->text() == "0.00 points");
    CHECK(page.message->text() == "Successfully Updated");
}

// Author: Benson
TEST_CASE("The sandbox starts from the real balance and days left") {
    MainUI window(fixedNumbers(700.0, 35));
    BudgetPage page(window);
    CHECK(page.balance->text() == "700.00 points");
    CHECK(page.days->value() == 35);
    CHECK(page.daily->text() == "20.00 points");
    CHECK(page.message->text().isEmpty());
}

// Author: Benson
TEST_CASE("The sandbox shows that it isn't saved and has no Reset days button") {
    MainUI window(fixedNumbers(0.0, 10));
    auto *label = window.findChild<QLabel *>("sandboxLabel");
    REQUIRE(label);
    CHECK(label->text().startsWith("Sandbox: changes here aren't saved"));
    CHECK(window.findChild<QPushButton *>("useRealNumbersButton"));
    CHECK_FALSE(window.findChild<QPushButton *>("resetDaysButton"));
}

// Author: Benson
TEST_CASE("Use my real numbers replaces sandbox changes with the real values") {
    MainUI window(fixedNumbers(300.0, 30));
    BudgetPage page(window);
    page.press("spendPointsButton", 100);
    page.days->setValue(5);
    CHECK(page.balance->text() == "200.00 points");
    CHECK(page.daily->text() == "40.00 points");

    page.click("useRealNumbersButton");
    CHECK(page.balance->text() == "300.00 points");
    CHECK(page.days->value() == 30);
    CHECK(page.daily->text() == "10.00 points");
    CHECK(page.message->text() == "Showing your real balance and days left.");
}

// Author: Benson
TEST_CASE("Opening the Budget tab reloads the latest real numbers") {
    double realBalance = 100.0;
    MainUI window([&] { return std::optional<MainUI::RealNumbers>({realBalance, 10}); });
    BudgetPage page(window);
    auto *purchases = new QWidget;
    window.setPurchasePage(purchases);
    window.addPage(new QWidget, "Profile");
    auto *tabs = qobject_cast<QTabWidget *>(window.centralWidget());
    REQUIRE(tabs);
    REQUIRE(tabs->count() == 3);
    CHECK(tabs->tabText(1) == "&Budget");

    page.press("addPointsButton", 50); // Sandbox only.
    realBalance = 80.0;                // For example, a purchase on another tab.
    tabs->setCurrentIndex(2);
    CHECK(page.balance->text() == "150.00 points"); // Not reloaded while away.
    tabs->setCurrentIndex(1);
    CHECK(page.balance->text() == "80.00 points");
}

// Author: Benson
TEST_CASE("If the real numbers can't be loaded, the sandbox says so") {
    bool fail = false;
    MainUI window([&]() -> std::optional<MainUI::RealNumbers> {
        if (fail) {
            return std::nullopt;
        }
        return MainUI::RealNumbers{50.0, 5};
    });
    BudgetPage page(window);
    fail = true;
    page.click("useRealNumbersButton");
    CHECK(page.balance->text() == "50.00 points");
    CHECK(page.message->text() == "Could not load your real numbers. Try again.");
}

// Author: Benson
TEST_CASE("Out-of-range real days are kept within the sandbox's limits") {
    MainUI window(fixedNumbers(90.0, 500));
    CHECK(BudgetPage(window).days->value() == 90);
}

// Author: Benson
TEST_CASE("Nothing done in the sandbox is saved to the account") {
    TestAccountStore store;
    const auto created = store.createVerified("sandbox@scu.edu", "Sandbox");
    const QString account = created.accountId;
    REQUIRE_FALSE(account.isEmpty());
    double balance = 0.0;
    REQUIRE(LocalAccounts::correctBalance(account, LocalAccounts::CorrectionMode::Set, 400.0,
                                          balance)
            == LocalAccounts::Result::Success);
    QList<LocalAccounts::HistoryEntry> historyBefore;
    REQUIRE(LocalAccounts::loadHistory(account, historyBefore));

    MainUI window([account]() -> std::optional<MainUI::RealNumbers> {
        LocalAccounts::Profile saved;
        if (!LocalAccounts::loadProfile(account, saved)) {
            return std::nullopt;
        }
        return MainUI::RealNumbers{saved.balance, 20};
    });
    BudgetPage page(window);
    CHECK(page.balance->text() == "400.00 points");

    page.press("setBalanceButton", 900);
    page.press("addPointsButton", 10);
    page.press("spendPointsButton", 500);
    page.days->setValue(3);
    CHECK(page.balance->text() == "410.00 points");

    LocalAccounts::Profile saved;
    REQUIRE(LocalAccounts::loadProfile(account, saved));
    CHECK(saved.balance == 400.0);
    CHECK_FALSE(saved.daysCorrected);
    QList<LocalAccounts::HistoryEntry> historyAfter;
    REQUIRE(LocalAccounts::loadHistory(account, historyAfter));
    CHECK(historyAfter.size() == historyBefore.size());
}

TEST_CASE("Days left count down when the date changes while the app is open") {
    MainUI window(fixedNumbers(700.0, 10));
    BudgetPage page(window);
    const QDate today = QDate::currentDate();

    window.showDate(today); // Same day: no change.
    CHECK(page.days->value() == 10);

    window.showDate(today.addDays(1));
    CHECK(page.days->value() == 9);
    CHECK(page.daily->text() == "77.77 points");

    window.showDate(today.addDays(4));
    CHECK(page.days->value() == 6);

    window.showDate(today.addDays(2)); // A clock moved backwards never adds days.
    CHECK(page.days->value() == 6);

    window.showDate(today.addDays(30)); // Never drops below 0.
    CHECK(page.days->value() == 0);
    CHECK(page.daily->text() == "700.00 points");
}

// Author: Nathan Gu
TEST_CASE("useRealNumbers replaces the sandbox numbers and reports whether it could") {
    bool fail = false;
    double realBalance = 50.0;
    MainUI window([&]() -> std::optional<MainUI::RealNumbers> {
        if (fail) {
            return std::nullopt;
        }
        return MainUI::RealNumbers{realBalance, 5};
    });
    BudgetPage page(window);
    page.press("setBalanceButton", 900);
    page.days->setValue(30);

    realBalance = 70.0;
    CHECK(window.useRealNumbers());
    CHECK(page.balance->text() == "70.00 points");
    CHECK(page.days->value() == 5);
    CHECK(page.daily->text() == "14.00 points");

    // When the real numbers can't be read, the sandbox numbers stay.
    page.press("setBalanceButton", 900);
    fail = true;
    CHECK_FALSE(window.useRealNumbers());
    CHECK(page.balance->text() == "900.00 points");
}
