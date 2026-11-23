// Tests for the Budget tab's balance buttons and their error messages.

#include "doctest.h"
#include "mainui.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

namespace {
struct BudgetPage {
    MainUI &window;
    QDoubleSpinBox *amount = window.findChild<QDoubleSpinBox *>("amountSpinBox");
    QLabel *balance = window.findChild<QLabel *>("balanceValueLabel");
    QLabel *message = window.findChild<QLabel *>("errorLabel");

    explicit BudgetPage(MainUI &w) : window(w) {
        REQUIRE(amount);
        REQUIRE(balance);
        REQUIRE(message);
    }

    void press(const char *button, double value) {
        amount->setValue(value);
        auto *target = window.findChild<QPushButton *>(button);
        REQUIRE(target);
        target->click();
    }
};
}

TEST_CASE("Budget tab balance buttons update the balance and report success") {
    MainUI window;
    BudgetPage page(window);
    double reported = -1.0;
    QObject::connect(&window, &MainUI::balanceChanged, [&](double value) { reported = value; });

    page.press("setBalanceButton", 700);
    CHECK(page.balance->text() == "700.00 points");
    CHECK(page.message->text() == "Successfully Updated");
    CHECK(reported == 700.0);
    page.press("addPointsButton", 50);
    CHECK(page.balance->text() == "750.00 points");
    page.press("spendPointsButton", 750);
    CHECK(page.balance->text() == "0.00 points");
    CHECK(reported == 0.0);
}

TEST_CASE("Budget tab rejects overspending and blank amounts") {
    MainUI window;
    BudgetPage page(window);
    page.press("setBalanceButton", 100);

    page.press("spendPointsButton", 150);
    CHECK(page.balance->text() == "100.00 points");
    CHECK(page.message->text() == "Not enough points. Enter an amount no greater than your balance.");

    page.amount->clear();
    window.findChild<QPushButton *>("addPointsButton")->click();
    CHECK(page.balance->text() == "100.00 points");
    CHECK(page.message->text() == "Enter an amount first.");
}

TEST_CASE("Budget tab can spend a displayed balance built from cents") {
    MainUI window;
    BudgetPage page(window);
    page.press("setBalanceButton", 0.70);
    page.press("addPointsButton", 0.10);
    CHECK(page.balance->text() == "0.80 points");
    page.press("spendPointsButton", 0.80);
    CHECK(page.balance->text() == "0.00 points");
    CHECK(page.message->text() == "Successfully Updated");
}

TEST_CASE("A balance that cannot be saved is rolled back") {
    double attemptedPrevious = -1.0;
    double attemptedNext = -1.0;
    MainUI window(50.0, [&](double previous, double next) {
        attemptedPrevious = previous;
        attemptedNext = next;
        return false;
    });
    BudgetPage page(window);
    CHECK(page.balance->text() == "50.00 points");

    page.press("setBalanceButton", 80);
    CHECK(attemptedPrevious == 50.0);
    CHECK(attemptedNext == 80.0);
    CHECK(page.balance->text() == "50.00 points");
    CHECK(page.message->text() == "Could not save the balance. Try again or sign in again to reload it.");
}

TEST_CASE("Balances saved from the Purchases tab refresh the Budget tab") {
    MainUI window(200.0, [](double, double) { return true; });
    BudgetPage page(window);
    window.showSavedBalance(187.5);
    CHECK(page.balance->text() == "187.50 points");
    CHECK(window.findChild<QLabel *>("dailyBudgetValueLabel")->text() == "2.43 points");
}

TEST_CASE("Days left start from the saved value and each change is saved") {
    QList<int> saved;
    MainUI window(700.0, {}, 35, [&](int days) {
        saved.append(days);
        return true;
    });
    auto *days = window.findChild<QSpinBox *>("remainingDaysSpinBox");
    auto *daily = window.findChild<QLabel *>("dailyBudgetValueLabel");
    REQUIRE(days);
    REQUIRE(daily);
    CHECK(days->value() == 35);
    CHECK(daily->text() == "20.00 points");
    CHECK(saved.isEmpty()); // Loading the saved value does not save it again.

    days->setValue(14);
    CHECK(saved == QList<int>{14});
    CHECK(daily->text() == "50.00 points");

    days->setValue(91); // Rejected by the budget, so nothing is saved.
    CHECK(days->value() == 14);
    CHECK(saved == QList<int>{14});
}

TEST_CASE("Days left that cannot be saved are rolled back") {
    MainUI window(700.0, {}, MainUI::DefaultRemainingDays, [](int) { return false; });
    auto *days = window.findChild<QSpinBox *>("remainingDaysSpinBox");
    REQUIRE(days);
    days->setValue(10);
    CHECK(days->value() == 77);
    CHECK(window.findChild<QLabel *>("dailyBudgetValueLabel")->text() == "9.09 points");
    CHECK(window.findChild<QLabel *>("errorLabel")->text()
          == "Could not save the days left. Your previous day count has been kept.");
}

TEST_CASE("An out-of-range saved day count falls back to the default") {
    MainUI window(700.0, {}, 500);
    CHECK(window.findChild<QSpinBox *>("remainingDaysSpinBox")->value() == MainUI::DefaultRemainingDays);
}

TEST_CASE("New windows start with 77 days left") {
    MainUI window;
    CHECK(MainUI::DefaultRemainingDays == 77);
    CHECK(window.findChild<QSpinBox *>("remainingDaysSpinBox")->value() == 77);
}

TEST_CASE("Days left count down when the date changes while the app is open") {
    QList<int> saved;
    MainUI window(700.0, {}, 10, [&](int days) {
        saved.append(days);
        return true;
    });
    auto *days = window.findChild<QSpinBox *>("remainingDaysSpinBox");
    auto *daily = window.findChild<QLabel *>("dailyBudgetValueLabel");
    const QDate today = QDate::currentDate();

    window.showDate(today); // Same day: no change.
    CHECK(days->value() == 10);

    window.showDate(today.addDays(1));
    CHECK(days->value() == 9);
    CHECK(daily->text() == "77.77 points");

    window.showDate(today.addDays(4));
    CHECK(days->value() == 6);

    window.showDate(today.addDays(2)); // A clock moved backwards never adds days.
    CHECK(days->value() == 6);

    window.showDate(today.addDays(30)); // Never drops below 0.
    CHECK(days->value() == 0);
    CHECK(daily->text() == "700.00 points");
    // The saved count and its date already produce this value at sign-in.
    CHECK(saved.isEmpty());
}

TEST_CASE("Reset days returns to the starting count and saves it") {
    QList<int> saved;
    MainUI window(700.0, {}, 20, [&](int days) {
        saved.append(days);
        return true;
    });
    auto *days = window.findChild<QSpinBox *>("remainingDaysSpinBox");
    auto *reset = window.findChild<QPushButton *>("resetDaysButton");
    auto *message = window.findChild<QLabel *>("errorLabel");
    REQUIRE(reset);

    reset->click();
    CHECK(days->value() == 77);
    CHECK(saved == QList<int>{77});
    CHECK(message->text() == "Days left reset to 77.");

    // Resetting when already at 77 still saves, which restarts the countdown today.
    reset->click();
    CHECK(saved == QList<int>{77, 77});
}

TEST_CASE("A reset that cannot be saved keeps the previous day count") {
    MainUI window(700.0, {}, 20, [](int) { return false; });
    auto *days = window.findChild<QSpinBox *>("remainingDaysSpinBox");
    window.findChild<QPushButton *>("resetDaysButton")->click();
    CHECK(days->value() == 20);
    CHECK(window.findChild<QLabel *>("errorLabel")->text()
          == "Could not save the days left. Your previous day count has been kept.");
}
