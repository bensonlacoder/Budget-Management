// Tests for the SQLite account store. Every storage function has at least one success
// and one failure case. Each test uses a fresh, temporary database (TestAccountStore).
// Author: Nathan Gu

#include "doctest.h"
#include "localaccounts.h"
#include "test_account_store.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>

#include <limits>
#include <tuple>

using LocalAccounts::CorrectionMode;
using LocalAccounts::HistoryEntry;
using LocalAccounts::Result;

namespace {
// Creates a verified account (password "correct-horse") in the running test's store.
LocalAccounts::Profile createAccount(const QString &email, const QString &displayName,
                                     const QString &plan = "basic") {
    REQUIRE(TestAccountStore::current());
    const auto profile = TestAccountStore::current()->createVerified(email, displayName, plan);
    REQUIRE_FALSE(profile.accountId.isEmpty());
    return profile;
}

double balanceOf(const QString &account) {
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::loadProfile(account, profile));
    return profile.balance;
}

// Runs SQL statements directly against the test database on one connection, to
// simulate a file damaged or edited outside the app.
void tamper(std::initializer_list<const char *> statements) {
    LocalAccounts::closeStorage();
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                             .filePath("budget.sqlite");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "tamper");
        db.setDatabaseName(path);
        REQUIRE(db.open());
        QSqlQuery query(db);
        for (const char *sql : statements) {
            REQUIRE(query.exec(sql));
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("tamper");
}
}

// ---- Accounts ----

// Author: Nathan Gu
TEST_CASE("The dining plans are SCU's five quarter plans with their starting points") {
    const auto &plans = LocalAccounts::diningPlans();
    REQUIRE(plans.size() == 5);
    const QList<std::tuple<QString, QString, int>> expected = {
        {"basic", "Basic Plan", 2237},
        {"preferred", "Preferred Plan", 2617},
        {"junior-senior", "Junior/Senior Plan", 1536},
        {"apartment", "Apartment Plan", 336},
        {"grad", "Graduate Plan", 336},
    };
    for (int i = 0; i < plans.size(); ++i) {
        CAPTURE(i);
        CHECK(plans[i].id == std::get<0>(expected[i]));
        CHECK(plans[i].name == std::get<1>(expected[i]));
        CHECK(plans[i].startingPoints == std::get<2>(expected[i]));
    }
    // The same list every time, so pages can keep a reference to it.
    CHECK(&LocalAccounts::diningPlans() == &plans);
}

// Author: Nathan Gu
TEST_CASE("Every dining plan can be chosen at sign-up and starts at its points") {
    TestAccountStore store;
    int number = 0;
    for (const auto &plan : LocalAccounts::diningPlans()) {
        CAPTURE(plan.id.toStdString());
        const auto created =
            createAccount(QString("plan%1@scu.edu").arg(number++), "Planner", plan.id);
        CHECK(created.diningPlanId == plan.id);
        CHECK(created.balance == plan.startingPoints);
    }
}

TEST_CASE("Sign-up details are validated before anything is saved") {
    TestAccountStore store;
    LocalAccounts::Profile profile;
    for (const char *bad : {"", "no-at-sign", "a@b", "two@@scu.edu", "spa ce@scu.edu", "x@scu."}) {
        CAPTURE(bad);
        CHECK(LocalAccounts::create(bad, "long-password", "Name", "basic", profile)
              == Result::InvalidEmail);
    }
    CHECK(LocalAccounts::create(QString(250, 'a') + "@scu.edu", "long-password", "Name", "basic",
                                profile) == Result::InvalidEmail);
    CHECK(LocalAccounts::create("ok@scu.edu", "long-password", "   ", "basic", profile)
          == Result::InvalidDisplayName);
    CHECK(LocalAccounts::create("ok@scu.edu", "long-password", QString(33, 'n'), "basic", profile)
          == Result::InvalidDisplayName);
    CHECK(LocalAccounts::create("ok@scu.edu", "short", "Name", "basic", profile)
          == Result::ShortPassword);
    CHECK(LocalAccounts::create("ok@scu.edu", "long-password", "Name", "not-a-plan", profile)
          == Result::InvalidDiningPlan);
    // The Law Plan is per semester, so it was removed.
    CHECK(LocalAccounts::create("ok@scu.edu", "long-password", "Name", "law", profile)
          == Result::InvalidDiningPlan);
    CHECK(LocalAccounts::signIn("ok@scu.edu", "long-password", profile)
          == Result::IncorrectCredentials);
}

TEST_CASE("Accounts sign in by email, ignoring case and spaces") {
    TestAccountStore store;
    const auto created = createAccount("  Alice@SCU.edu ", "  Alice  ", "preferred");
    CHECK(created.email == "alice@scu.edu");
    CHECK(created.displayName == "Alice");
    CHECK(created.balance == 2617.0);
    CHECK(created.friendId.size() == 12);
    CHECK(created.diningPlanId == "preferred");
    CHECK(created.remainingDays == LocalAccounts::DefaultRemainingDays);
    CHECK_FALSE(created.daysCorrected);
    CHECK_FALSE(created.accountId.isEmpty());

    LocalAccounts::Profile duplicate;
    CHECK(LocalAccounts::create("ALICE@scu.edu", "another-password", "Other", "basic", duplicate)
          == Result::EmailTaken);

    LocalAccounts::Profile signedIn;
    CHECK(LocalAccounts::signIn("alice@scu.edu", "wrong-password", signedIn)
          == Result::IncorrectCredentials);
    CHECK(LocalAccounts::signIn("bob@scu.edu", "correct-horse", signedIn)
          == Result::IncorrectCredentials);
    REQUIRE(LocalAccounts::signIn(" ALICE@scu.edu ", "correct-horse", signedIn) == Result::Success);
    CHECK(signedIn.accountId == created.accountId);
    CHECK(signedIn.friendId == created.friendId);
    CHECK(signedIn.balance == created.balance);
}

TEST_CASE("Profiles load and update, and the account ID survives an email change") {
    TestAccountStore store;
    const auto alice = createAccount("alice@scu.edu", "Alice");
    createAccount("bob@scu.edu", "Bob");

    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::loadProfile(alice.accountId, profile));
    CHECK(profile.email == "alice@scu.edu");
    CHECK_FALSE(LocalAccounts::loadProfile("no-such-account", profile));
    CHECK(profile.accountId.isEmpty());

    CHECK(LocalAccounts::updateProfile(alice.accountId, " ", "alice@scu.edu")
          == Result::InvalidDisplayName);
    CHECK(LocalAccounts::updateProfile(alice.accountId, "Alice", "not-an-email")
          == Result::InvalidEmail);
    CHECK(LocalAccounts::updateProfile(alice.accountId, "Alice", "BOB@scu.edu")
          == Result::EmailTaken);
    CHECK(LocalAccounts::updateProfile("no-such-account", "Alice", "new@scu.edu")
          == Result::StorageError);

    REQUIRE(LocalAccounts::updateProfile(alice.accountId, " Ally ", "Ally@SCU.edu")
            == Result::Success);
    REQUIRE(LocalAccounts::loadProfile(alice.accountId, profile));
    CHECK(profile.displayName == "Ally");
    CHECK(profile.email == "ally@scu.edu");
    LocalAccounts::Profile signedIn;
    CHECK(LocalAccounts::signIn("alice@scu.edu", "correct-horse", signedIn)
          == Result::IncorrectCredentials);
    REQUIRE(LocalAccounts::signIn("ally@scu.edu", "correct-horse", signedIn) == Result::Success);
    CHECK(signedIn.accountId == alice.accountId);
}

TEST_CASE("Passwords change only with the current password") {
    TestAccountStore store;
    const auto alice = createAccount("alice@scu.edu", "Alice");
    CHECK(LocalAccounts::changePassword(alice.accountId, "wrong-password", "new-password")
          == Result::IncorrectCredentials);
    CHECK(LocalAccounts::changePassword(alice.accountId, "correct-horse", "short")
          == Result::ShortPassword);
    CHECK(LocalAccounts::changePassword("no-such-account", "correct-horse", "new-password")
          == Result::StorageError);

    REQUIRE(LocalAccounts::changePassword(alice.accountId, "correct-horse", "new-password")
            == Result::Success);
    LocalAccounts::Profile signedIn;
    CHECK(LocalAccounts::signIn("alice@scu.edu", "correct-horse", signedIn)
          == Result::IncorrectCredentials);
    CHECK(LocalAccounts::signIn("alice@scu.edu", "new-password", signedIn) == Result::Success);
}

TEST_CASE("A damaged saved balance is reported instead of crashing sign-in") {
    TestAccountStore store;
    createAccount("damaged@scu.edu", "Damaged");
    // The table refuses negative balances, so switch that rule off to simulate damage.
    tamper({"PRAGMA ignore_check_constraints = ON",
            "UPDATE accounts SET balance_cents = -500 WHERE email = 'damaged@scu.edu'"});
    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::signIn("damaged@scu.edu", "correct-horse", profile)
          == Result::StorageError);
}

TEST_CASE("Storage errors are reported when the database can't be opened") {
    TestAccountStore store;
    // A directory where the database file should be makes opening it fail.
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                             .filePath("budget.sqlite");
    LocalAccounts::closeStorage();
    REQUIRE(QDir().mkpath(path));
    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::signIn("alice@scu.edu", "correct-horse", profile) == Result::StorageError);
    CHECK(LocalAccounts::create("alice@scu.edu", "correct-horse", "Alice", "basic", profile)
          == Result::StorageError);
    CHECK_FALSE(LocalAccounts::loadProfile("anything", profile));
    LocalAccounts::closeStorage();
    QDir().rmdir(path);
}

// ---- Balance and days left ----

TEST_CASE("Saving a balance refuses stale or invalid values") {
    TestAccountStore store;
    const auto saver = createAccount("saver@scu.edu", "Saver"); // Basic plan: 2237 points
    CHECK(LocalAccounts::saveBalance(saver.accountId, 2237.0, 2000.0));
    // Another window still thinks the balance is 2237, so its write is refused.
    CHECK_FALSE(LocalAccounts::saveBalance(saver.accountId, 2237.0, 1500.0));
    CHECK_FALSE(LocalAccounts::saveBalance(saver.accountId, 2000.0, -1.0));
    CHECK_FALSE(LocalAccounts::saveBalance(saver.accountId, 2000.0,
                                           std::numeric_limits<double>::quiet_NaN()));
    CHECK_FALSE(LocalAccounts::saveBalance("nobody", 0.0, 10.0));
    CHECK(balanceOf(saver.accountId) == 2000.0);
    // Cents are compared exactly: 0.1 + 0.2 is saved and matched as 0.30.
    CHECK(LocalAccounts::saveBalance(saver.accountId, 2000.0, 0.1 + 0.2));
    CHECK(LocalAccounts::saveBalance(saver.accountId, 0.3, 5.0));
}

TEST_CASE("Balance corrections set, add or subtract and appear in history") {
    TestAccountStore store;
    const auto account = createAccount("fix@scu.edu", "Fixer", "apartment").accountId; // 336
    double balance = -1.0;

    CHECK(LocalAccounts::correctBalance(account, CorrectionMode::Add, 0.0, balance)
          == Result::InvalidAmount);
    CHECK(LocalAccounts::correctBalance(account, CorrectionMode::Set, -1.0, balance)
          == Result::InvalidAmount);
    CHECK(LocalAccounts::correctBalance(account, CorrectionMode::Add,
                                        std::numeric_limits<double>::infinity(), balance)
          == Result::InvalidAmount);
    CHECK(LocalAccounts::correctBalance(account, CorrectionMode::Subtract, 336.01, balance)
          == Result::InsufficientPoints);
    CHECK(LocalAccounts::correctBalance("nobody", CorrectionMode::Add, 5.0, balance)
          == Result::StorageError);
    CHECK(balance == -1.0); // Untouched when nothing was saved.

    REQUIRE(LocalAccounts::correctBalance(account, CorrectionMode::Subtract, 36.0, balance)
            == Result::Success);
    CHECK(balance == 300.0);
    REQUIRE(LocalAccounts::correctBalance(account, CorrectionMode::Add, 10.5, balance)
            == Result::Success);
    CHECK(balance == 310.5);
    REQUIRE(LocalAccounts::correctBalance(account, CorrectionMode::Set, 250.0, balance)
            == Result::Success);
    CHECK(balance == 250.0);
    // Setting the same value again changes nothing and adds no history entry.
    REQUIRE(LocalAccounts::correctBalance(account, CorrectionMode::Set, 250.0, balance)
            == Result::Success);
    CHECK(balanceOf(account) == 250.0);

    QList<HistoryEntry> history;
    REQUIRE(LocalAccounts::loadHistory(account, history));
    REQUIRE(history.size() == 3);
    CHECK(history.at(0).kind == HistoryEntry::Kind::Correction);
    CHECK(history.at(0).balanceChange == -60.5);
    CHECK(history.at(1).balanceChange == 10.5);
    CHECK(history.at(2).balanceChange == -36.0);
}

TEST_CASE("Days left corrections are saved, count down and can be cleared") {
    TestAccountStore store;
    const auto account = createAccount("days@scu.edu", "Planner").accountId;
    const QDate today = QDate::currentDate();

    CHECK_FALSE(LocalAccounts::saveRemainingDays(account, -1));
    CHECK_FALSE(LocalAccounts::saveRemainingDays(account, LocalAccounts::MaxRemainingDays + 1));
    CHECK_FALSE(LocalAccounts::saveRemainingDays(account, 30, QDate()));
    CHECK_FALSE(LocalAccounts::saveRemainingDays("nobody", 5));

    REQUIRE(LocalAccounts::saveRemainingDays(account, 30, today.addDays(-5)));
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::loadProfile(account, profile));
    CHECK(profile.remainingDays == 25);
    CHECK(profile.daysCorrected);

    REQUIRE(LocalAccounts::saveRemainingDays(account, 30, today.addDays(-45)));
    REQUIRE(LocalAccounts::loadProfile(account, profile));
    CHECK(profile.remainingDays == 0);

    REQUIRE(LocalAccounts::clearDaysCorrection(account) == Result::Success);
    REQUIRE(LocalAccounts::loadProfile(account, profile));
    CHECK(profile.remainingDays == LocalAccounts::DefaultRemainingDays);
    CHECK_FALSE(profile.daysCorrected);
    CHECK(LocalAccounts::clearDaysCorrection("nobody") == Result::StorageError);
}

TEST_CASE("Days left count down by one for each day since they were saved") {
    const QDate monday(2026, 11, 16);
    CHECK(LocalAccounts::remainingDaysOn(77, monday, monday) == 77);
    CHECK(LocalAccounts::remainingDaysOn(77, monday, monday.addDays(1)) == 76);
    CHECK(LocalAccounts::remainingDaysOn(77, monday, monday.addDays(77)) == 0);
    CHECK(LocalAccounts::remainingDaysOn(77, monday, monday.addDays(200)) == 0);
    // A clock set backwards, or a missing date, never changes the count.
    CHECK(LocalAccounts::remainingDaysOn(77, monday, monday.addDays(-3)) == 77);
    CHECK(LocalAccounts::remainingDaysOn(77, QDate(), monday) == 77);
}

// ---- Purchases, refunds and history ----

TEST_CASE("Purchases are validated, deducted, listed newest first and can be removed") {
    TestAccountStore store;
    const auto buyer = createAccount("buyer@scu.edu", "Buyer", "apartment").accountId; // 336
    const QDate today = QDate::currentDate();
    double balance = -1.0;

    CHECK(LocalAccounts::recordPurchase(buyer, {today.addDays(1), "Marketplace", "", 5.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase(buyer, {today, "   ", "", 5.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase(buyer, {today, "Marketplace", "", 0.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase(buyer, {QDate(), "Marketplace", "", 5.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase(
              buyer, {today, QString(LocalAccounts::PurchaseTextLimit + 1, 'x'), "", 5.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase(buyer, {today, "Marketplace", "", 336.01}, balance)
          == Result::InsufficientPoints);
    CHECK(LocalAccounts::recordPurchase("nobody", {today, "Marketplace", "", 1.0}, balance)
          == Result::StorageError);
    CHECK(balance == -1.0);

    REQUIRE(LocalAccounts::recordPurchase(buyer, {today.addDays(-2), "Fresh Bytes", "Boba", 6.20},
                                          balance) == Result::Success);
    CHECK(balance == doctest::Approx(329.80));
    REQUIRE(LocalAccounts::recordPurchase(buyer, {today, "  Marketplace  ", "  Pasta  ", 11.9},
                                          balance) == Result::Success);
    CHECK(balance == doctest::Approx(317.90));

    QList<LocalAccounts::Purchase> purchases;
    CHECK_FALSE(LocalAccounts::loadPurchases("nobody", purchases));
    REQUIRE(LocalAccounts::loadPurchases(buyer, purchases));
    REQUIRE(purchases.size() == 2);
    CHECK(purchases.at(0).date == today);
    CHECK(purchases.at(0).location == "Marketplace");
    CHECK(purchases.at(0).item == "Pasta");
    CHECK_FALSE(purchases.at(0).id.isEmpty());
    CHECK(purchases.at(1).item == "Boba");

    REQUIRE(LocalAccounts::removePurchase(buyer, purchases.at(1), balance) == Result::Success);
    CHECK(balance == doctest::Approx(324.10));
    CHECK(LocalAccounts::removePurchase(buyer, purchases.at(1), balance)
          == Result::PurchaseUnavailable);

    // Spending the exact remaining balance is allowed and leaves 0.
    REQUIRE(LocalAccounts::recordPurchase(buyer, {today, "Marketplace", "", 324.10}, balance)
            == Result::Success);
    CHECK(balance == 0.0);
}

TEST_CASE("Purchases can be edited, adjusting the balance by the difference") {
    TestAccountStore store;
    const auto buyer = createAccount("edit@scu.edu", "Editor", "apartment").accountId; // 336
    const QDate today = QDate::currentDate();
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordPurchase(buyer, {today, "Marketplace", "Lunch", 10.0}, balance)
            == Result::Success);
    QList<LocalAccounts::Purchase> purchases;
    REQUIRE(LocalAccounts::loadPurchases(buyer, purchases));
    auto purchase = purchases.first();

    purchase.points = 12.5;
    purchase.location = "The Bronco";
    REQUIRE(LocalAccounts::editPurchase(buyer, purchase, balance) == Result::Success);
    CHECK(balance == 323.5);
    REQUIRE(LocalAccounts::loadPurchases(buyer, purchases));
    CHECK(purchases.first().location == "The Bronco");
    CHECK(purchases.first().points == 12.5);

    auto tooBig = purchase;
    tooBig.points = 400.0; // 336 - 400 would be negative.
    CHECK(LocalAccounts::editPurchase(buyer, tooBig, balance) == Result::InsufficientPoints);
    auto invalid = purchase;
    invalid.location = "";
    CHECK(LocalAccounts::editPurchase(buyer, invalid, balance) == Result::InvalidPurchase);
    auto missing = purchase;
    missing.id = "no-such-purchase";
    CHECK(LocalAccounts::editPurchase(buyer, missing, balance) == Result::PurchaseUnavailable);
    CHECK(balanceOf(buyer) == 323.5);
}

// Author: Nathan Gu
TEST_CASE("A linked refund can't be dated before its purchase") {
    TestAccountStore store;
    const auto buyer = createAccount("early-refund@scu.edu", "Early", "apartment").accountId;
    const QDate today = QDate::currentDate();
    const QDate bought = today.addDays(-3);
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordPurchase(buyer, {bought, "Marketplace", "Pasta", 20.0}, balance)
            == Result::Success);
    QList<LocalAccounts::Purchase> purchases;
    REQUIRE(LocalAccounts::loadPurchases(buyer, purchases));
    const QString pasta = purchases.first().id;
    const double afterPurchase = balanceOf(buyer);

    CHECK(LocalAccounts::recordRefund(buyer, {bought.addDays(-1), 5.0, pasta}, balance)
          == Result::RefundBeforePurchase);
    CHECK(balanceOf(buyer) == afterPurchase);
    // A standalone refund has no purchase to come after.
    CHECK(LocalAccounts::recordRefund(buyer, {bought.addDays(-1), 1.0}, balance)
          == Result::Success);
    // The same day as the purchase is fine.
    REQUIRE(LocalAccounts::recordRefund(buyer, {bought, 5.0, pasta}, balance) == Result::Success);

    QList<LocalAccounts::HistoryEntry> history;
    REQUIRE(LocalAccounts::loadHistory(buyer, history));
    QString refundId;
    for (const auto &entry : history) {
        if (entry.kind == HistoryEntry::Kind::Refund && entry.purchaseId == pasta) {
            refundId = entry.id;
        }
    }
    REQUIRE_FALSE(refundId.isEmpty());
    CHECK(LocalAccounts::editRefund(buyer, {bought.addDays(-2), 5.0, pasta, {}, refundId}, balance)
          == Result::RefundBeforePurchase);

    // Moving the purchase after its refund is refused too; its own day still works.
    CHECK(LocalAccounts::editPurchase(buyer, {today, "Marketplace", "Pasta", 20.0, pasta}, balance)
          == Result::RefundBeforePurchase);
    CHECK(LocalAccounts::editPurchase(buyer, {bought, "Marketplace", "Pasta", 25.0, pasta}, balance)
          == Result::Success);
    REQUIRE(LocalAccounts::loadPurchases(buyer, purchases));
    CHECK(purchases.first().date == bought);
}

TEST_CASE("Refunds return points, respect the purchase total and can be edited or removed") {
    TestAccountStore store;
    const auto buyer = createAccount("refund@scu.edu", "Refunder", "apartment").accountId; // 336
    const QDate today = QDate::currentDate();
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordPurchase(buyer, {today, "Marketplace", "Pasta", 20.0}, balance)
            == Result::Success);
    QList<LocalAccounts::Purchase> purchases;
    REQUIRE(LocalAccounts::loadPurchases(buyer, purchases));
    const QString pasta = purchases.first().id;

    CHECK(LocalAccounts::recordRefund(buyer, {today, 0.0, pasta}, balance) == Result::InvalidAmount);
    CHECK(LocalAccounts::recordRefund(buyer, {today.addDays(1), 5.0, pasta}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordRefund(buyer, {today, 20.01, pasta}, balance)
          == Result::RefundTooLarge);
    CHECK(LocalAccounts::recordRefund(buyer, {today, 5.0, "no-such-purchase"}, balance)
          == Result::PurchaseUnavailable);

    // A partial refund, then the rest of it.
    REQUIRE(LocalAccounts::recordRefund(buyer, {today, 8.0, pasta, "Cold"}, balance)
            == Result::Success);
    CHECK(balance == 324.0);
    CHECK(LocalAccounts::recordRefund(buyer, {today, 12.01, pasta}, balance)
          == Result::RefundTooLarge);
    REQUIRE(LocalAccounts::recordRefund(buyer, {today, 12.0, pasta}, balance) == Result::Success);
    CHECK(balance == 336.0);
    // A refund not linked to any purchase.
    REQUIRE(LocalAccounts::recordRefund(buyer, {today, 3.0}, balance) == Result::Success);
    CHECK(balance == 339.0);

    QList<HistoryEntry> history;
    REQUIRE(LocalAccounts::loadHistory(buyer, history));
    REQUIRE(history.size() == 4);
    CHECK(history.at(0).kind == HistoryEntry::Kind::Refund);
    CHECK(history.at(0).purchaseId.isEmpty());
    CHECK(history.at(2).item == "Cold");
    CHECK(history.at(2).purchaseId == pasta);
    CHECK(history.at(3).kind == HistoryEntry::Kind::Purchase);
    CHECK(history.at(3).balanceChange == -20.0);

    // Editing the first refund: it can't push the pasta's refunds past 20.
    LocalAccounts::Refund cold{today, 9.0, pasta, "Cold", history.at(2).id};
    CHECK(LocalAccounts::editRefund(buyer, cold, balance) == Result::RefundTooLarge);
    cold.points = 6.0;
    REQUIRE(LocalAccounts::editRefund(buyer, cold, balance) == Result::Success);
    CHECK(balance == 337.0);
    cold.id = "no-such-refund";
    CHECK(LocalAccounts::editRefund(buyer, cold, balance) == Result::RefundUnavailable);

    REQUIRE(LocalAccounts::removeRefund(buyer, history.at(0).id, balance) == Result::Success);
    CHECK(balance == 334.0);
    CHECK(LocalAccounts::removeRefund(buyer, history.at(0).id, balance)
          == Result::RefundUnavailable);

    // The purchase can't be edited below what was refunded (6 + 12).
    auto edited = purchases.first();
    edited.points = 17.99;
    CHECK(LocalAccounts::editPurchase(buyer, edited, balance) == Result::RefundTooLarge);

    // Removing the purchase removes its refunds and returns only the unrefunded 2 points.
    REQUIRE(LocalAccounts::removePurchase(buyer, purchases.first(), balance) == Result::Success);
    CHECK(balance == 336.0);
    REQUIRE(LocalAccounts::loadHistory(buyer, history));
    CHECK(history.isEmpty());
    CHECK_FALSE(LocalAccounts::loadHistory("nobody", history));
}

TEST_CASE("History keeps the recorded order even when entries are saved in the same instant") {
    // Regression: ordering by save time tied when a fast machine saved several entries
    // within one millisecond, so purchases always sorted ahead of refunds.
    TestAccountStore store;
    const auto account = createAccount("order@scu.edu", "Orderly").accountId;
    const QDate today = QDate::currentDate();
    double balance = 0.0;
    QStringList expected; // Newest first.
    for (int round = 0; round < 20; ++round) {
        REQUIRE(LocalAccounts::recordPurchase(account, {today, "Marketplace", QString::number(round), 1.0},
                                              balance) == Result::Success);
        expected.prepend("purchase " + QString::number(round));
        REQUIRE(LocalAccounts::recordRefund(account, {today, 0.5, {}, QString::number(round)}, balance)
                == Result::Success);
        expected.prepend("refund " + QString::number(round));
        REQUIRE(LocalAccounts::correctBalance(account, CorrectionMode::Add, 0.25, balance)
                == Result::Success);
        expected.prepend("correction");
    }
    QList<HistoryEntry> history;
    REQUIRE(LocalAccounts::loadHistory(account, history));
    QStringList actual;
    for (const auto &entry : history) {
        actual << (entry.kind == HistoryEntry::Kind::Purchase ? "purchase " + entry.item
                   : entry.kind == HistoryEntry::Kind::Refund ? "refund " + entry.item
                                                              : QStringLiteral("correction"));
    }
    CHECK(actual == expected);
}

TEST_CASE("A version-1 database is upgraded without losing data") {
    TestAccountStore store;
    const auto account = createAccount("old@scu.edu", "Old Timer").accountId;
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordPurchase(account, {QDate::currentDate(), "Marketplace", "Old", 2.0},
                                          balance) == Result::Success);
    // Turn the database back into the version-1 layout.
    tamper({"ALTER TABLE purchases RENAME COLUMN seq TO created_ms",
            "ALTER TABLE refunds RENAME COLUMN seq TO created_ms",
            "ALTER TABLE corrections RENAME COLUMN seq TO created_ms",
            "PRAGMA user_version = 1"});

    REQUIRE(LocalAccounts::recordPurchase(account, {QDate::currentDate(), "Marketplace", "New", 3.0},
                                          balance) == Result::Success);
    QList<HistoryEntry> history;
    REQUIRE(LocalAccounts::loadHistory(account, history));
    REQUIRE(history.size() == 2);
    CHECK(history.at(0).item == "New");
    CHECK(history.at(1).item == "Old");
    CHECK(balance == 2232.0);
}

TEST_CASE("Removing a refund whose points were already spent is refused") {
    TestAccountStore store;
    const auto buyer = createAccount("spent@scu.edu", "Spender", "apartment").accountId; // 336
    const QDate today = QDate::currentDate();
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordRefund(buyer, {today, 4.0}, balance) == Result::Success);
    REQUIRE(LocalAccounts::recordPurchase(buyer, {today, "Marketplace", "", 340.0}, balance)
            == Result::Success);
    QList<HistoryEntry> history;
    REQUIRE(LocalAccounts::loadHistory(buyer, history));
    const QString refund = history.at(1).id;
    CHECK(LocalAccounts::removeRefund(buyer, refund, balance) == Result::InsufficientPoints);
    CHECK(balanceOf(buyer) == 0.0);
}

// ---- Last updated (T12) ----

namespace {
// Sets the saved last-updated time to an old value, so a test can tell whether the
// next operation changed it.
void markOld(const QString &account) {
    const QString sql = QStringLiteral("UPDATE accounts SET last_updated = '2000-01-01T00:00:00.000Z'"
                                       " WHERE id = '%1'").arg(account);
    const QByteArray utf8 = sql.toUtf8();
    tamper({utf8.constData()});
}

bool wasUpdated(const QString &account) {
    QDateTime updated;
    REQUIRE(LocalAccounts::loadLastUpdated(account, updated));
    REQUIRE(updated.isValid());
    return updated.date().year() > 2000;
}
}

TEST_CASE("Last updated starts empty and records successful changes in local time") {
    TestAccountStore store;
    const auto account = createAccount("time@scu.edu", "Timer").accountId;
    QDateTime updated = QDateTime::currentDateTime();
    REQUIRE(LocalAccounts::loadLastUpdated(account, updated));
    CHECK_FALSE(updated.isValid()); // Nothing has changed yet.
    CHECK_FALSE(LocalAccounts::loadLastUpdated("nobody", updated));

    // Saved with millisecond precision, so compare against times truncated to the second.
    const QDateTime before = QDateTime::currentDateTime().addSecs(-1);
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordPurchase(account, {QDate::currentDate(), "Marketplace", "", 1.0},
                                          balance) == Result::Success);
    const QDateTime after = QDateTime::currentDateTime().addSecs(1);
    REQUIRE(LocalAccounts::loadLastUpdated(account, updated));
    CHECK(updated >= before);
    CHECK(updated <= after);
    CHECK(updated.timeSpec() == Qt::LocalTime);

    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::loadProfile(account, profile));
    CHECK(profile.lastUpdated == updated);
}

TEST_CASE("Every successful balance or history change updates the time") {
    TestAccountStore store;
    const auto account = createAccount("every@scu.edu", "Everyone").accountId; // 2237
    const QDate today = QDate::currentDate();
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordPurchase(account, {today, "Marketplace", "Lunch", 10.0}, balance)
            == Result::Success);
    QList<LocalAccounts::Purchase> purchases;
    REQUIRE(LocalAccounts::loadPurchases(account, purchases));
    auto purchase = purchases.first();

    SUBCASE("Recording a purchase") {
        markOld(account);
        REQUIRE(LocalAccounts::recordPurchase(account, {today, "Marketplace", "", 1.0}, balance)
                == Result::Success);
    }
    SUBCASE("Editing a purchase, even without changing its points") {
        purchase.location = "The Bronco";
        markOld(account);
        REQUIRE(LocalAccounts::editPurchase(account, purchase, balance) == Result::Success);
    }
    SUBCASE("Removing a purchase") {
        markOld(account);
        REQUIRE(LocalAccounts::removePurchase(account, purchase, balance) == Result::Success);
    }
    SUBCASE("Recording, editing and removing a refund") {
        markOld(account);
        REQUIRE(LocalAccounts::recordRefund(account, {today, 4.0, purchase.id}, balance)
                == Result::Success);
        CHECK(wasUpdated(account));
        QList<HistoryEntry> history;
        REQUIRE(LocalAccounts::loadHistory(account, history));
        const QString refundId = history.first().id;
        markOld(account);
        REQUIRE(LocalAccounts::editRefund(account, {today, 5.0, purchase.id, {}, refundId}, balance)
                == Result::Success);
        CHECK(wasUpdated(account));
        markOld(account);
        REQUIRE(LocalAccounts::removeRefund(account, refundId, balance) == Result::Success);
    }
    SUBCASE("Correcting the balance") {
        markOld(account);
        REQUIRE(LocalAccounts::correctBalance(account, CorrectionMode::Add, 5.0, balance)
                == Result::Success);
    }
    SUBCASE("Saving a balance from the Budget tab") {
        markOld(account);
        REQUIRE(LocalAccounts::saveBalance(account, 2227.0, 2000.0));
    }
    CHECK(wasUpdated(account));
}

TEST_CASE("Rejected or empty changes leave the time unchanged") {
    TestAccountStore store;
    const auto account = createAccount("still@scu.edu", "Still", "apartment").accountId; // 336
    const QDate today = QDate::currentDate();
    double balance = 0.0;
    markOld(account);

    CHECK(LocalAccounts::recordPurchase(account, {today, "Marketplace", "", 400.0}, balance)
          == Result::InsufficientPoints);
    CHECK(LocalAccounts::recordPurchase(account, {today, "", "", 1.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::correctBalance(account, CorrectionMode::Subtract, 400.0, balance)
          == Result::InsufficientPoints);
    // Setting the balance to its current value isn't a change.
    CHECK(LocalAccounts::correctBalance(account, CorrectionMode::Set, 336.0, balance)
          == Result::Success);
    CHECK(LocalAccounts::recordRefund(account, {today, 1.0, "no-such-purchase"}, balance)
          == Result::PurchaseUnavailable);
    CHECK_FALSE(LocalAccounts::saveBalance(account, 1.0, 2.0)); // Stale previous balance.
    // Settings and days corrections aren't balance or history changes.
    CHECK(LocalAccounts::savePlanSettings(account, {}) == Result::Success);
    CHECK(LocalAccounts::saveRemainingDays(account, 30));
    CHECK_FALSE(wasUpdated(account));
}

// ---- Plan tab ----

TEST_CASE("Plan settings are saved together and invalid values are refused") {
    TestAccountStore store;
    const auto account = createAccount("plan@scu.edu", "Planner").accountId;
    LocalAccounts::PlanSettings settings;
    REQUIRE(LocalAccounts::loadPlanSettings(account, settings));
    CHECK(settings.reserveGoal == 0.0);
    CHECK(settings.habits == LocalAccounts::DiningHabits{});
    CHECK_FALSE(settings.alertOverDailyBudget);

    settings.reserveGoal = 50.25;
    settings.habits = {2, 2, 2, 2, 1, 0, 0};
    settings.alertOverDailyBudget = true;
    REQUIRE(LocalAccounts::savePlanSettings(account, settings) == Result::Success);
    LocalAccounts::PlanSettings loaded;
    REQUIRE(LocalAccounts::loadPlanSettings(account, loaded));
    CHECK(loaded.reserveGoal == 50.25);
    CHECK(loaded.habits == settings.habits);
    CHECK(loaded.alertOverDailyBudget);
    CHECK_FALSE(loaded.alertRunOutBeforeQuarterEnd);

    auto bad = settings;
    bad.reserveGoal = -1.0;
    CHECK(LocalAccounts::savePlanSettings(account, bad) == Result::InvalidSettings);
    bad = settings;
    bad.habits[3] = LocalAccounts::MaxMealsPerDay + 1;
    CHECK(LocalAccounts::savePlanSettings(account, bad) == Result::InvalidSettings);
    CHECK(LocalAccounts::savePlanSettings("nobody", settings) == Result::StorageError);
    CHECK_FALSE(LocalAccounts::loadPlanSettings("nobody", loaded));
    CHECK(loaded.reserveGoal == 0.0);
}

// ---- Friends ----

TEST_CASE("Friend requests can be sent, accepted and declined") {
    TestAccountStore store;
    const auto alice = createAccount("alice@scu.edu", "Alice");
    const auto bob = createAccount("bob@scu.edu", "Bob", "grad");
    const auto carol = createAccount("carol@scu.edu", "Carol");

    CHECK(LocalAccounts::sendRequest(alice.accountId, "NOT-A-REAL-ID") == Result::UnknownFriendId);
    CHECK(LocalAccounts::sendRequest(alice.accountId, alice.friendId) == Result::OwnFriendId);
    // Friend IDs are matched regardless of case and surrounding spaces.
    CHECK(LocalAccounts::sendRequest(alice.accountId, "  " + bob.friendId.toLower() + " ")
          == Result::Success);
    CHECK(LocalAccounts::sendRequest(alice.accountId, bob.friendId) == Result::RequestAlreadySent);
    CHECK(LocalAccounts::sendRequest(bob.accountId, alice.friendId)
          == Result::IncomingRequestExists);

    LocalAccounts::Friends alices;
    REQUIRE(LocalAccounts::loadFriends(alice.accountId, alices));
    REQUIRE(alices.outgoing.size() == 1);
    CHECK(alices.outgoing.first().displayName == "Bob");
    LocalAccounts::Friends bobs;
    REQUIRE(LocalAccounts::loadFriends(bob.accountId, bobs));
    REQUIRE(bobs.incoming.size() == 1);
    CHECK(bobs.incoming.first().displayName == "Alice");
    CHECK(bobs.incoming.first().friendId == alice.friendId);
    CHECK(bobs.accepted.isEmpty());

    REQUIRE(LocalAccounts::respondToRequest(bob.accountId, alice.friendId, true) == Result::Success);
    CHECK(LocalAccounts::respondToRequest(bob.accountId, alice.friendId, true)
          == Result::RequestUnavailable);
    CHECK(LocalAccounts::sendRequest(alice.accountId, bob.friendId) == Result::AlreadyFriends);
    CHECK(LocalAccounts::sendRequest(bob.accountId, alice.friendId) == Result::AlreadyFriends);

    REQUIRE(LocalAccounts::loadFriends(bob.accountId, bobs));
    CHECK(bobs.incoming.isEmpty());
    REQUIRE(bobs.accepted.size() == 1);
    CHECK(bobs.accepted.first().displayName == "Alice");
    CHECK(bobs.accepted.first().balance == alice.balance);
    CHECK(bobs.accepted.first().friendId == alice.friendId);
    REQUIRE(LocalAccounts::loadFriends(alice.accountId, alices));
    CHECK(alices.outgoing.isEmpty());
    REQUIRE(alices.accepted.size() == 1);
    CHECK(alices.accepted.first().balance == 336.0);

    REQUIRE(LocalAccounts::sendRequest(carol.accountId, alice.friendId) == Result::Success);
    REQUIRE(LocalAccounts::respondToRequest(alice.accountId, carol.friendId, false)
            == Result::Success);
    REQUIRE(LocalAccounts::loadFriends(alice.accountId, alices));
    CHECK(alices.incoming.isEmpty());
    CHECK(alices.accepted.size() == 1);
    // A declined request can be sent again.
    CHECK(LocalAccounts::sendRequest(carol.accountId, alice.friendId) == Result::Success);
    CHECK_FALSE(LocalAccounts::loadFriends("nobody", alices));
}

TEST_CASE("Removing a friend ends the friendship for both accounts") {
    TestAccountStore store;
    const auto alice = createAccount("alice@scu.edu", "Alice");
    const auto bob = createAccount("bob@scu.edu", "Bob");
    REQUIRE(LocalAccounts::sendRequest(alice.accountId, bob.friendId) == Result::Success);
    REQUIRE(LocalAccounts::respondToRequest(bob.accountId, alice.friendId, true) == Result::Success);

    CHECK(LocalAccounts::removeFriend(alice.accountId, "NOT-A-REAL-ID") == Result::FriendUnavailable);
    REQUIRE(LocalAccounts::removeFriend(alice.accountId, bob.friendId) == Result::Success);
    CHECK(LocalAccounts::removeFriend(alice.accountId, bob.friendId) == Result::FriendUnavailable);

    LocalAccounts::Friends friends;
    REQUIRE(LocalAccounts::loadFriends(bob.accountId, friends));
    CHECK(friends.accepted.isEmpty());
    // Either of them can send a new request afterwards.
    CHECK(LocalAccounts::sendRequest(bob.accountId, alice.friendId) == Result::Success);
}

// ---- Email codes (T15) ----

namespace {
bool isSixDigits(const QString &code) {
    return QRegularExpression(QRegularExpression::anchoredPattern("[0-9]{6}")).match(code).hasMatch();
}

LocalAccounts::Profile createUnverified(const QString &email, const QString &password = "correct-horse") {
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::create(email, password, "New", "basic", profile) == Result::Success);
    return profile;
}
}

TEST_CASE("A new account can't sign in until its email is verified") {
    TestAccountStore store;
    createUnverified("new@scu.edu");
    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::signIn("new@scu.edu", "wrong-password", profile)
          == Result::IncorrectCredentials);
    CHECK(LocalAccounts::signIn("new@scu.edu", "correct-horse", profile)
          == Result::EmailNotVerified);
    // No code has been requested yet.
    CHECK(LocalAccounts::verifyEmail("new@scu.edu", "123456") == Result::InvalidCode);

    REQUIRE(LocalAccounts::sendVerificationCode(" NEW@scu.edu ") == Result::Success);
    const QString code = store.sentCodes.value("new@scu.edu");
    CHECK(isSixDigits(code));
    const QString wrong = code == "000000" ? "111111" : "000000";
    CHECK(LocalAccounts::verifyEmail("new@scu.edu", wrong) == Result::InvalidCode);
    CHECK(LocalAccounts::verifyEmail("nobody@scu.edu", code) == Result::InvalidEmail);
    REQUIRE(LocalAccounts::verifyEmail("new@scu.edu", " " + code + " ") == Result::Success);
    CHECK(LocalAccounts::signIn("new@scu.edu", "correct-horse", profile) == Result::Success);

    // A used code can't be used again, and a verified account needs no more codes.
    CHECK(LocalAccounts::verifyEmail("new@scu.edu", code) == Result::InvalidCode);
    const int sent = store.emailsSent;
    CHECK(LocalAccounts::sendVerificationCode("new@scu.edu") == Result::Success);
    CHECK(store.emailsSent == sent);
    CHECK(LocalAccounts::sendVerificationCode("nobody@scu.edu") == Result::InvalidEmail);
}

TEST_CASE("Resending a code cancels the previous one") {
    TestAccountStore store;
    createUnverified("resend@scu.edu");
    REQUIRE(LocalAccounts::sendVerificationCode("resend@scu.edu") == Result::Success);
    const QString first = store.sentCodes.value("resend@scu.edu");
    QString second = first;
    // Codes are random; resend until a different one arrives (almost always at once).
    for (int tries = 0; second == first && tries < 5; ++tries) {
        REQUIRE(LocalAccounts::sendVerificationCode("resend@scu.edu") == Result::Success);
        second = store.sentCodes.value("resend@scu.edu");
    }
    REQUIRE(second != first);
    CHECK(LocalAccounts::verifyEmail("resend@scu.edu", first) == Result::InvalidCode);
    CHECK(LocalAccounts::verifyEmail("resend@scu.edu", second) == Result::Success);
}

TEST_CASE("Expired codes are refused") {
    TestAccountStore store;
    createUnverified("late@scu.edu");
    REQUIRE(LocalAccounts::sendVerificationCode("late@scu.edu") == Result::Success);
    const QString code = store.sentCodes.value("late@scu.edu");
    tamper({"UPDATE email_codes SET expires_at = '2000-01-01T00:00:00.000Z'"});
    CHECK(LocalAccounts::verifyEmail("late@scu.edu", code) == Result::CodeExpired);
    // A new code works.
    REQUIRE(LocalAccounts::sendVerificationCode("late@scu.edu") == Result::Success);
    CHECK(LocalAccounts::verifyEmail("late@scu.edu", store.sentCodes.value("late@scu.edu"))
          == Result::Success);
}

TEST_CASE("Too many wrong codes cancel the code") {
    TestAccountStore store;
    createUnverified("guess@scu.edu");
    REQUIRE(LocalAccounts::sendVerificationCode("guess@scu.edu") == Result::Success);
    const QString code = store.sentCodes.value("guess@scu.edu");
    const QString wrong = code == "000000" ? "111111" : "000000";

    SUBCASE("One try short of the limit still allows the right code") {
        for (int i = 0; i < LocalAccounts::MaxCodeAttempts - 1; ++i) {
            CHECK(LocalAccounts::verifyEmail("guess@scu.edu", wrong) == Result::InvalidCode);
        }
        CHECK(LocalAccounts::verifyEmail("guess@scu.edu", code) == Result::Success);
    }
    SUBCASE("Reaching the limit cancels the code") {
        for (int i = 0; i < LocalAccounts::MaxCodeAttempts; ++i) {
            CHECK(LocalAccounts::verifyEmail("guess@scu.edu", wrong) == Result::InvalidCode);
        }
        CHECK(LocalAccounts::verifyEmail("guess@scu.edu", code) == Result::InvalidCode);
    }
}

TEST_CASE("A sign-up whose code couldn't be sent leaves nothing behind") {
    TestAccountStore store;
    store.sendingFails = true;
    createUnverified("offline@scu.edu");
    CHECK(LocalAccounts::sendVerificationCode("offline@scu.edu") == Result::CodeNotSent);
    LocalAccounts::cancelSignUp("offline@scu.edu");
    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::signIn("offline@scu.edu", "correct-horse", profile)
          == Result::IncorrectCredentials);

    // An unverified sign-up is replaced by a new one with the same email.
    store.sendingFails = false;
    createUnverified("offline@scu.edu", "first-password");
    createUnverified("OFFLINE@scu.edu", "second-password");
    REQUIRE(LocalAccounts::sendVerificationCode("offline@scu.edu") == Result::Success);
    REQUIRE(LocalAccounts::verifyEmail("offline@scu.edu", store.sentCodes.value("offline@scu.edu"))
            == Result::Success);
    CHECK(LocalAccounts::signIn("offline@scu.edu", "first-password", profile)
          == Result::IncorrectCredentials);
    CHECK(LocalAccounts::signIn("offline@scu.edu", "second-password", profile) == Result::Success);

    // Once verified, the email is taken and cancelSignUp leaves the account alone.
    CHECK(LocalAccounts::create("offline@scu.edu", "third-password", "Other", "basic", profile)
          == Result::EmailTaken);
    LocalAccounts::cancelSignUp("offline@scu.edu");
    CHECK(LocalAccounts::signIn("offline@scu.edu", "second-password", profile) == Result::Success);
}

TEST_CASE("A forgotten password is reset with an emailed code") {
    TestAccountStore store;
    createAccount("forgot@scu.edu", "Forgetful");
    createUnverified("pending@scu.edu");
    CHECK(LocalAccounts::sendPasswordResetCode("nobody@scu.edu") == Result::InvalidEmail);
    CHECK(LocalAccounts::sendPasswordResetCode("pending@scu.edu") == Result::EmailNotVerified);
    CHECK(LocalAccounts::resetPassword("forgot@scu.edu", "123456", "new-password")
          == Result::InvalidCode); // No reset code requested yet.

    REQUIRE(LocalAccounts::sendPasswordResetCode("Forgot@SCU.edu") == Result::Success);
    const QString code = store.sentCodes.value("forgot@scu.edu");
    CHECK(isSixDigits(code));
    const QString wrong = code == "000000" ? "111111" : "000000";
    CHECK(LocalAccounts::resetPassword("forgot@scu.edu", code, "short") == Result::ShortPassword);
    CHECK(LocalAccounts::resetPassword("forgot@scu.edu", wrong, "new-password")
          == Result::InvalidCode);
    // A reset code isn't a sign-up code.
    CHECK(LocalAccounts::verifyEmail("forgot@scu.edu", code) == Result::InvalidCode);
    REQUIRE(LocalAccounts::resetPassword("forgot@scu.edu", code, "new-password") == Result::Success);

    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::signIn("forgot@scu.edu", "correct-horse", profile)
          == Result::IncorrectCredentials);
    CHECK(LocalAccounts::signIn("forgot@scu.edu", "new-password", profile) == Result::Success);
    CHECK(LocalAccounts::resetPassword("forgot@scu.edu", code, "another-password")
          == Result::InvalidCode); // Used up.

    // When the code can't be sent, nothing is left that could be used.
    store.sendingFails = true;
    CHECK(LocalAccounts::sendPasswordResetCode("forgot@scu.edu") == Result::CodeNotSent);
    CHECK(LocalAccounts::resetPassword("forgot@scu.edu", code, "another-password")
          == Result::InvalidCode);
}

TEST_CASE("Accounts made before email verification existed can still sign in") {
    TestAccountStore store;
    createUnverified("early@scu.edu");
    // Make the database look like version 2, before verification was required.
    tamper({"PRAGMA user_version = 2"});
    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::signIn("early@scu.edu", "correct-horse", profile) == Result::Success);
}

// Writes a settings file for the real email sender and points it there.
static QString writeSenderSettings(const QByteArray &contents) {
    const QString path =
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath("email-sender.ini");
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    file.write(contents);
    file.close();
    qputenv("DINING_POINTS_EMAIL_CONFIG", path.toUtf8());
    return path;
}

TEST_CASE("The real email sender works without setup and can be pointed elsewhere") {
    TestAccountStore store;
    const QString path =
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath("no-such-email-sender.ini");
    qputenv("DINING_POINTS_EMAIL_CONFIG", path.toUtf8());
    CHECK(EmailSender::configPath() == path);
    // No settings file: the built-in web app, so it works on any computer.
    CHECK(EmailSender::url() == QUrl(QString::fromLatin1(EmailSender::DefaultUrl)));
    CHECK(EmailSender::url().scheme() == "https");
    CHECK(EmailSender::url().host() == "script.google.com");

    writeSenderSettings("[sender]\nurl=https://script.google.com/macros/s/other/exec\n");
    CHECK(EmailSender::url() == QUrl("https://script.google.com/macros/s/other/exec"));
    qunsetenv("DINING_POINTS_EMAIL_CONFIG");
}

TEST_CASE("The real email sender refuses to send when turned off or not https") {
    TestAccountStore store;
    createUnverified("real@scu.edu");
    for (const QByteArray contents : {QByteArray("[sender]\nurl=\n"),
                                      QByteArray("[sender]\nurl=http://example.com/exec\n")}) {
        CAPTURE(contents);
        writeSenderSettings(contents);
        EmailSender::setSender({}); // The real sender; it must fail before any network use.
        CHECK_FALSE(EmailSender::send("someone@scu.edu", "123456",
                                      EmailSender::Purpose::Verification));
        CHECK(LocalAccounts::sendVerificationCode("real@scu.edu") == Result::CodeNotSent);
    }
    qunsetenv("DINING_POINTS_EMAIL_CONFIG");
}

// ---- Two-step sign-in ----

// Author: Nathan Gu
TEST_CASE("Two-step sign-in is off by default and asks for an emailed code once turned on") {
    TestAccountStore store;
    const auto created = createAccount("careful@scu.edu", "Careful");
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::signIn("careful@scu.edu", "correct-horse", profile) == Result::Success);
    CHECK_FALSE(profile.twoStepSignIn);

    REQUIRE(LocalAccounts::setTwoStepSignIn(created.accountId, true) == Result::Success);
    REQUIRE(LocalAccounts::loadProfile(created.accountId, profile));
    CHECK(profile.twoStepSignIn);

    // The password alone no longer signs in, and a wrong password still says so.
    profile = {};
    CHECK(LocalAccounts::signIn("careful@scu.edu", "correct-horse", profile) == Result::CodeRequired);
    CHECK(profile.accountId.isEmpty());
    CHECK(LocalAccounts::signIn("careful@scu.edu", "wrong-password", profile)
          == Result::IncorrectCredentials);

    // A code is only sent with the right password.
    const int sentBefore = store.emailsSent;
    CHECK(LocalAccounts::sendSignInCode("careful@scu.edu", "wrong-password")
          == Result::IncorrectCredentials);
    CHECK(LocalAccounts::sendSignInCode("nobody@scu.edu", "correct-horse")
          == Result::IncorrectCredentials);
    CHECK(store.emailsSent == sentBefore);
    REQUIRE(LocalAccounts::sendSignInCode("Careful@SCU.edu", "correct-horse") == Result::Success);
    CHECK(store.emailsSent == sentBefore + 1);
    const QString code = store.sentCodes.value("careful@scu.edu");
    CHECK(isSixDigits(code));
    const QString wrong = code == "000000" ? "111111" : "000000";

    CHECK(LocalAccounts::finishSignIn("careful@scu.edu", "correct-horse", wrong, profile)
          == Result::InvalidCode);
    // The right code with the wrong password doesn't sign in.
    CHECK(LocalAccounts::finishSignIn("careful@scu.edu", "wrong-password", code, profile)
          == Result::IncorrectCredentials);
    REQUIRE(LocalAccounts::finishSignIn("careful@scu.edu", "correct-horse", code, profile)
            == Result::Success);
    CHECK(profile.accountId == created.accountId);
    CHECK(profile.twoStepSignIn);
    // Each code works once.
    CHECK(LocalAccounts::finishSignIn("careful@scu.edu", "correct-horse", code, profile)
          == Result::InvalidCode);

    // Turning it off goes back to the password alone.
    REQUIRE(LocalAccounts::setTwoStepSignIn(created.accountId, false) == Result::Success);
    CHECK(LocalAccounts::signIn("careful@scu.edu", "correct-horse", profile) == Result::Success);
}

// Author: Nathan Gu
TEST_CASE("Sign-in codes can't be swapped with other codes, and failures are reported") {
    TestAccountStore store;
    const auto created = createAccount("swap@scu.edu", "Swapper");
    REQUIRE(LocalAccounts::setTwoStepSignIn(created.accountId, true) == Result::Success);
    CHECK(LocalAccounts::setTwoStepSignIn("no-such-account", true) == Result::StorageError);
    LocalAccounts::Profile profile;

    // A password reset code doesn't finish a sign-in, and a sign-in code doesn't reset.
    REQUIRE(LocalAccounts::sendPasswordResetCode("swap@scu.edu") == Result::Success);
    const QString resetCode = store.sentCodes.value("swap@scu.edu");
    CHECK(LocalAccounts::finishSignIn("swap@scu.edu", "correct-horse", resetCode, profile)
          == Result::InvalidCode);
    REQUIRE(LocalAccounts::sendSignInCode("swap@scu.edu", "correct-horse") == Result::Success);
    const QString signInCode = store.sentCodes.value("swap@scu.edu");
    CHECK(LocalAccounts::resetPassword("swap@scu.edu", signInCode, "new-password")
          == Result::InvalidCode);

    // A new code replaces the old one.
    REQUIRE(LocalAccounts::sendSignInCode("swap@scu.edu", "correct-horse") == Result::Success);
    const QString newest = store.sentCodes.value("swap@scu.edu");
    if (newest != signInCode) {
        CHECK(LocalAccounts::finishSignIn("swap@scu.edu", "correct-horse", signInCode, profile)
              == Result::InvalidCode);
    }

    // An unsent code can't be used, and an unverified account gets no sign-in code.
    store.sendingFails = true;
    CHECK(LocalAccounts::sendSignInCode("swap@scu.edu", "correct-horse") == Result::CodeNotSent);
    CHECK(LocalAccounts::finishSignIn("swap@scu.edu", "correct-horse", newest, profile)
          == Result::InvalidCode);
    store.sendingFails = false;
    createUnverified("pending@scu.edu");
    CHECK(LocalAccounts::sendSignInCode("pending@scu.edu", "correct-horse")
          == Result::EmailNotVerified);
}

// Author: Nathan Gu
TEST_CASE("A version-3 database gains two-step sign-in, turned off") {
    TestAccountStore store;
    createAccount("older@scu.edu", "Older");
    // Make the database look like version 3, before two-step sign-in existed.
    tamper({"ALTER TABLE accounts DROP COLUMN two_step", "PRAGMA user_version = 3"});
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::signIn("older@scu.edu", "correct-horse", profile) == Result::Success);
    CHECK_FALSE(profile.twoStepSignIn);
    CHECK(LocalAccounts::setTwoStepSignIn(profile.accountId, true) == Result::Success);
}
