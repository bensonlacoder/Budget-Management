#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>

#include "mainui.h"

TEST_CASE("Invalid day entry explains the limit and preserves the budget") {
    MainUI window;
    window.show();
    auto *days = window.findChild<QSpinBox *>("remainingDaysSpinBox");
    auto *error = window.findChild<QLabel *>("errorLabel");
    REQUIRE(days != nullptr);
    REQUIRE(error != nullptr);
    auto *input = days->findChild<QLineEdit *>();
    REQUIRE(input != nullptr);
    auto *amount = window.findChild<QDoubleSpinBox *>("amountSpinBox");
    auto *setBalance = window.findChild<QPushButton *>("setBalanceButton");
    auto *daily = window.findChild<QLabel *>("dailyBudgetValueLabel");
    auto *weekly = window.findChild<QLabel *>("weeklyBudgetValueLabel");
    REQUIRE(amount != nullptr);
    REQUIRE(setBalance != nullptr);
    REQUIRE(daily != nullptr);
    REQUIRE(weekly != nullptr);
    amount->setValue(700);
    setBalance->click();
    QApplication::processEvents();
    input->setFocus();
    bool commitOnFocusChange = false;

    SUBCASE("91 days") {
        input->selectAll();
        QTest::keyClicks(input, "91");
    }
    SUBCASE("366 days") {
        input->selectAll();
        QTest::keyClicks(input, "366");
    }
    SUBCASE("Original input range allowing more than 90 days") {
        days->setMaximum(366);
        input->selectAll();
        QTest::keyClicks(input, "91");
    }
    SUBCASE("Leaving the field with an invalid value") {
        input->selectAll();
        QTest::keyClicks(input, "91");
        commitOnFocusChange = true;
    }
    if (commitOnFocusChange) {
        amount->setFocus();
    } else {
        QTest::keyClick(input, Qt::Key_Return);
    }

    CHECK(days->value() == 70);
    CHECK(daily->text() == "10.00 points");
    CHECK(weekly->text() == "70.00 points");
    CHECK(error->text() == "Remaining days must be between 0 and 90, including today. Your previous day count has been kept.");

    // A valid entry after the rejection still works and clears the error.
    input->selectAll();
    QTest::keyClicks(input, "90");
    QTest::keyClick(input, Qt::Key_Return);
    CHECK(days->value() == 90);
    CHECK(error->text().isEmpty());

    // Stepping past the upper boundary must also be handled safely.
    QTest::keyClick(input, Qt::Key_Up);
    CHECK(days->value() == 90);
    CHECK_FALSE(error->text().isEmpty());

    input->selectAll();
    QTest::keyClicks(input, "0");
    QTest::keyClick(input, Qt::Key_Return);
    CHECK(days->value() == 0);
    CHECK(daily->text() == "700.00 points");
    CHECK(weekly->text() == "700.00 points");
    CHECK(error->text().isEmpty());

    input->selectAll();
    QTest::keyClicks(input, "1");
    QTest::keyClick(input, Qt::Key_Return);
    CHECK(days->value() == 1);
    CHECK(daily->text() == "700.00 points");
    CHECK(weekly->text() == "700.00 points");
    CHECK(error->text().isEmpty());
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    return doctest::Context(argc, argv).run();
}
