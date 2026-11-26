#include "doctest.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>

#include "mainui.h"

TEST_CASE("Balance buttons update the displayed balance and recommendations") {
    // Start the sandbox from 0 points and 77 days so the expected advice is fixed.
    MainUI window([] { return std::optional<MainUI::RealNumbers>({0.0, 77}); });
    window.show();
    auto *amount = window.findChild<QDoubleSpinBox *>("amountSpinBox");
    auto *setBalance = window.findChild<QPushButton *>("setBalanceButton");
    auto *add = window.findChild<QPushButton *>("addPointsButton");
    auto *spend = window.findChild<QPushButton *>("spendPointsButton");
    auto *balance = window.findChild<QLabel *>("balanceValueLabel");
    auto *daily = window.findChild<QLabel *>("dailyBudgetValueLabel");
    auto *weekly = window.findChild<QLabel *>("weeklyBudgetValueLabel");
    auto *days = window.findChild<QSpinBox *>("remainingDaysSpinBox");
    auto *error = window.findChild<QLabel *>("errorLabel");
    REQUIRE(amount != nullptr);
    REQUIRE(setBalance != nullptr);
    REQUIRE(add != nullptr);
    REQUIRE(spend != nullptr);
    REQUIRE(balance != nullptr);
    REQUIRE(daily != nullptr);
    REQUIRE(weekly != nullptr);
    REQUIRE(days != nullptr);
    REQUIRE(error != nullptr);

    CHECK(balance->text() == "0.00 points");
    CHECK(daily->text() == "0.00 points");
    CHECK(weekly->text() == "0.00 points");
    CHECK(days->value() == 77);

    amount->setValue(700.0);
    QTest::mouseClick(setBalance, Qt::LeftButton);
    CHECK(balance->text() == "700.00 points");
    CHECK(daily->text() == "9.09 points");
    CHECK(weekly->text() == "63.63 points");

    SUBCASE("Refills and purchases refresh every displayed value") {
        amount->setValue(70.0);
        QTest::mouseClick(add, Qt::LeftButton);
        CHECK(balance->text() == "770.00 points");
        CHECK(daily->text() == "10.00 points");
        CHECK(weekly->text() == "70.00 points");

        amount->setValue(12.5);
        QTest::mouseClick(spend, Qt::LeftButton);
        CHECK(balance->text() == "757.50 points");
        CHECK(daily->text() == "9.83 points");
        CHECK(weekly->text() == "68.86 points");

        amount->setValue(757.5);
        QTest::mouseClick(spend, Qt::LeftButton);
        CHECK(balance->text() == "0.00 points");
        CHECK(daily->text() == "0.00 points");
        CHECK(weekly->text() == "0.00 points");
    }
    SUBCASE("Setting a new balance replaces the displayed amount") {
        amount->setValue(350.0);
        QTest::mouseClick(setBalance, Qt::LeftButton);
        CHECK(balance->text() == "350.00 points");
        CHECK(daily->text() == "4.54 points");
        CHECK(weekly->text() == "31.81 points");
    }
    SUBCASE("Overspending shows an error and preserves all displayed values") {
        amount->setValue(701.0);
        QTest::mouseClick(spend, Qt::LeftButton);
        CHECK(error->text() == "Not enough points. Enter an amount no greater than your balance.");
        CHECK(balance->text() == "700.00 points");
        CHECK(daily->text() == "9.09 points");
        CHECK(weekly->text() == "63.63 points");

        amount->setValue(70.0);
        QTest::mouseClick(spend, Qt::LeftButton);
        CHECK(balance->text() == "630.00 points");
        CHECK(daily->text() == "8.18 points");
        CHECK(weekly->text() == "57.27 points");
        CHECK(error->text() == "Successfully Updated");
    }
}
