#include "doctest.h"
#include "purchasespage.h"
#include "quarter.h"
#include "test_account_store.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>

namespace {
using LocalAccounts::Result;

void submitAmount(const char *name, double value,
                  const char *errorName = nullptr) {
    auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
    CHECK(dialog);
    if (!dialog) {
        return;
    }
    dialog->findChild<QDoubleSpinBox *>(name)->setValue(value);
    auto *buttons = dialog->findChild<QDialogButtonBox *>();
    const int initialHeight = dialog->height();
    buttons->button(QDialogButtonBox::Save)->click();
    if (errorName) {
        auto *error = dialog->findChild<QLabel *>(errorName);
        CHECK(error->isVisible());
        CHECK_FALSE(error->text().isEmpty());
        CHECK(error->wordWrap());
        CHECK(dialog->height() > initialHeight);
        CHECK(error->height() >= error->heightForWidth(error->width()));
        CHECK(dialog->rect().contains(error->geometry()));
        CHECK(error->geometry().bottom() < buttons->geometry().top());
        CHECK(dialog->rect().contains(buttons->geometry()));
        dialog->reject();
    }
}
}

// Author: Nathan Jeong
TEST_CASE("History filters by date and location and shows corrections") {
    TestAccountStore store;
    const auto profile = store.createVerified("history@example.com", "Student");
    const auto today = QDate::currentDate();
    double balance;
    REQUIRE(LocalAccounts::recordPurchase(profile.accountId, {today, "A", "Lunch", 2}, balance)
            == Result::Success);
    REQUIRE(LocalAccounts::recordPurchase(profile.accountId, {today.addDays(-1), "A", "Tea", 3}, balance)
            == Result::Success);
    REQUIRE(LocalAccounts::recordPurchase(profile.accountId, {today.addDays(-1), "B", "Soup", 4}, balance)
            == Result::Success);
    REQUIRE(LocalAccounts::correctBalance(profile.accountId, LocalAccounts::CorrectionMode::Subtract,
                                           1, balance) == Result::Success);

    PurchasesPage page(profile.accountId);
    auto *table = page.findChild<QTableWidget *>("purchaseHistory");
    auto *location = page.findChild<QComboBox *>("historyLocation");
    auto *dates = page.findChild<QCheckBox *>("historyDateFilter");
    REQUIRE(table->rowCount() == 4);
    CHECK(table->item(0, 2)->text() == "Balance correction");
    CHECK(table->item(0, 2)->font().italic());
    location->setCurrentIndex(location->findText("A"));
    CHECK(table->rowCount() == 2);
    page.findChild<QDateEdit *>("historyFrom")->setDate(today);
    dates->setChecked(true);
    REQUIRE(table->rowCount() == 1);
    CHECK(table->item(0, 2)->text() == "Lunch");
    location->setCurrentIndex(location->findText("B"));
    CHECK(table->rowCount() == 0);
    CHECK(page.findChild<QLabel *>("historyStatus")->text().contains("No history matches"));
    location->setCurrentIndex(0);
    CHECK(table->rowCount() == 2);
    page.findChild<QPushButton *>("clearHistoryFilters")->click();
    CHECK(table->rowCount() == 4);
}

// Author: Jeong
TEST_CASE("Editing a purchase saves the new details and refuses an unaffordable edit") {
    TestAccountStore store;
    const auto profile = store.createVerified("edit-history@example.com", "Student");
    const auto today = QDate::currentDate();
    double balance;
    REQUIRE(LocalAccounts::recordPurchase(profile.accountId, {today, "A", "Tea", 3}, balance)
            == Result::Success);
    PurchasesPage page(profile.accountId);
    auto *table = page.findChild<QTableWidget *>("purchaseHistory");
    auto *edit = page.findChild<QPushButton *>("editHistoryButton");
    QObject::connect(&page, &PurchasesPage::balanceUpdated, [&](double value) { balance = value; });

    table->selectRow(0);
    QTimer::singleShot(0, &page, [&] {
        auto *dialog = QApplication::activeModalWidget();
        dialog->findChild<QLineEdit *>("editPurchaseLocation")->setText("B");
        dialog->findChild<QLineEdit *>("editPurchaseItem")->setText("Latte");
        submitAmount("editPurchasePoints", 5);
    });
    edit->click();
    CHECK(balance == profile.balance - 5);
    CHECK(table->item(0, 1)->text() == "B");
    CHECK(table->item(0, 2)->text() == "Latte");

    table->selectRow(0);
    QTimer::singleShot(0, &page, [&] {
        submitAmount("editPurchasePoints", profile.balance + 1, "editPurchaseError");
    });
    edit->click();
    QList<LocalAccounts::Purchase> purchases;
    REQUIRE(LocalAccounts::loadPurchases(profile.accountId, purchases));
    REQUIRE(purchases.size() == 1);
    CHECK(purchases.first().points == 5);
    CHECK(balance == profile.balance - 5);
}

// Author: Jeong
TEST_CASE("Linked refunds respect the remaining amount and can be edited and removed") {
    TestAccountStore store;
    const auto profile = store.createVerified("refund-history@example.com", "Student");
    const auto today = QDate::currentDate();
    double balance;
    REQUIRE(LocalAccounts::recordPurchase(profile.accountId, {today, "A", "Tea", 10}, balance)
            == Result::Success);
    PurchasesPage page(profile.accountId);
    auto *table = page.findChild<QTableWidget *>("purchaseHistory");
    auto *refund = page.findChild<QPushButton *>("linkedRefundButton");
    QObject::connect(&page, &PurchasesPage::balanceUpdated, [&](double value) { balance = value; });
    table->selectRow(0);
    QTimer::singleShot(0, &page, [&] { submitAmount("refundPoints", 4); });
    refund->click();
    CHECK(balance == profile.balance - 6);
    CHECK(table->item(0, 1)->text() == "A");

    table->selectRow(1);
    QTimer::singleShot(0, &page, [&] { submitAmount("refundPoints", 7, "refundError"); });
    refund->click();
    CHECK(table->rowCount() == 2);
    QTimer::singleShot(0, &page, [&] { submitAmount("refundPoints", 6); });
    refund->click();
    CHECK(balance == profile.balance);

    table->selectRow(0);
    QTimer::singleShot(0, &page, [&] { submitAmount("refundPoints", 5); });
    page.findChild<QPushButton *>("editHistoryButton")->click();
    CHECK(balance == profile.balance - 1);
    table->selectRow(0);
    QTimer::singleShot(0, &page, [] {
        auto *confirmation = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        confirmation->button(QMessageBox::Yes)->click();
    });
    page.findChild<QPushButton *>("removePurchaseButton")->click();
    CHECK(table->rowCount() == 2);
    CHECK(balance == profile.balance - 6);
}

// Author: Nathan Gu
TEST_CASE("Editing a purchase from an earlier quarter keeps its date") {
    TestAccountStore store;
    const auto profile = store.createVerified("old-purchase@example.com", "Student");
    const auto today = QDate::currentDate();
    const auto quarter = Quarter::statusOn(today);
    // Before this quarter's first day when there is one; otherwise just an older date.
    const QDate old = quarter.phase == Quarter::Phase::InQuarter ? quarter.term.start.addDays(-1)
                                                                 : today.addDays(-30);
    double balance;
    REQUIRE(LocalAccounts::recordPurchase(profile.accountId, {old, "A", "Tea", 3}, balance)
            == Result::Success);
    PurchasesPage page(profile.accountId);
    auto *table = page.findChild<QTableWidget *>("purchaseHistory");
    table->selectRow(0);
    QDate shown;
    QTimer::singleShot(0, &page, [&] {
        auto *dialog = QApplication::activeModalWidget();
        shown = dialog->findChild<QDateEdit *>("editPurchaseDate")->date();
        dialog->findChild<QLineEdit *>("editPurchaseItem")->setText("Green tea");
        submitAmount("editPurchasePoints", 3);
    });
    page.findChild<QPushButton *>("editHistoryButton")->click();
    CHECK(shown == old);
    QList<LocalAccounts::Purchase> purchases;
    REQUIRE(LocalAccounts::loadPurchases(profile.accountId, purchases));
    REQUIRE(purchases.size() == 1);
    CHECK(purchases.first().item == "Green tea");
    CHECK(purchases.first().date == old);
}

// Author: Nathan Gu
TEST_CASE("A linked refund's date can't be set before its purchase") {
    TestAccountStore store;
    const auto profile = store.createVerified("refund-date@example.com", "Student");
    const QDate bought = QDate::currentDate().addDays(-2);
    double balance;
    REQUIRE(LocalAccounts::recordPurchase(profile.accountId, {bought, "A", "Tea", 3}, balance)
            == Result::Success);
    PurchasesPage page(profile.accountId);
    page.findChild<QTableWidget *>("purchaseHistory")->selectRow(0);
    QDate earliest;
    QTimer::singleShot(0, &page, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        REQUIRE(dialog);
        earliest = dialog->findChild<QDateEdit *>("refundDate")->minimumDate();
        dialog->reject();
    });
    page.findChild<QPushButton *>("linkedRefundButton")->click();
    CHECK(earliest == bought);
}
