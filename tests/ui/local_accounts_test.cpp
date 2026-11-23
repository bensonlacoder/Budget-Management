// Tests for the SQLite account store. Every storage function has at least one success
// and one failure case. Each test uses a fresh, temporary database (TestAccountStore).
// Author: Nathan Gu

#include "doctest.h"
#include "localaccounts.h"
#include "test_account_store.h"

#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>

#include <limits>

using LocalAccounts::CorrectionMode;
using LocalAccounts::HistoryEntry;
using LocalAccounts::Result;

namespace {
LocalAccounts::Profile createAccount(const QString &email, const QString &displayName,
                                     const QString &plan = "basic") {
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::create(email, "correct-horse", displayName, plan, profile)
            == Result::Success);
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

// ---- Saved menu sections ----

TEST_CASE("Menu sections can be saved once, listed and removed") {
    TestAccountStore store;
    const auto account = createAccount("menu@scu.edu", "Diner").accountId;
    CHECK(LocalAccounts::saveSection(account, "   ") == Result::InvalidSettings);
    CHECK(LocalAccounts::saveSection(account, QString(LocalAccounts::PurchaseTextLimit + 1, 'x'))
          == Result::InvalidSettings);
    CHECK(LocalAccounts::saveSection("nobody", "Grill") == Result::StorageError);

    REQUIRE(LocalAccounts::saveSection(account, " Marketplace / Grill ") == Result::Success);
    REQUIRE(LocalAccounts::saveSection(account, "Marketplace / Grill") == Result::Success);
    REQUIRE(LocalAccounts::saveSection(account, "Sidebar Cafe") == Result::Success);
    QStringList sections;
    REQUIRE(LocalAccounts::loadSavedSections(account, sections));
    CHECK(sections == QStringList{"Marketplace / Grill", "Sidebar Cafe"});

    REQUIRE(LocalAccounts::removeSection(account, "Marketplace / Grill") == Result::Success);
    CHECK(LocalAccounts::removeSection(account, "Marketplace / Grill") == Result::SectionUnavailable);
    REQUIRE(LocalAccounts::loadSavedSections(account, sections));
    CHECK(sections == QStringList{"Sidebar Cafe"});
    CHECK_FALSE(LocalAccounts::loadSavedSections("nobody", sections));
}

// ---- Not implemented yet ----

TEST_CASE("T12 and T15 functions fail safely until those tickets implement them") {
    TestAccountStore store;
    const auto account = createAccount("later@scu.edu", "Later").accountId;
    QDateTime updated = QDateTime::currentDateTime();
    CHECK_FALSE(LocalAccounts::loadLastUpdated(account, updated));
    CHECK_FALSE(updated.isValid());
    CHECK(LocalAccounts::sendVerificationCode("later@scu.edu") == Result::StorageError);
    CHECK(LocalAccounts::resetPassword("later@scu.edu", "123456", "new-password")
          == Result::StorageError);
}
