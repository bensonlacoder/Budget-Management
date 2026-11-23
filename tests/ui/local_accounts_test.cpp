// Tests for the saved account store: sign-in, balances, purchases and friends.

#include "doctest.h"
#include "localaccounts.h"
#include "test_account_store.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

using LocalAccounts::Result;

namespace {
LocalAccounts::Profile createAccount(const QString &username, const QString &displayName,
                                     const QString &plan = "basic") {
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::create(username, "correct-horse", displayName, plan, profile)
            == Result::Success);
    return profile;
}
}

TEST_CASE("Account details are validated before anything is saved") {
    TestAccountStore store;
    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::create("ab", "long-password", "Name", "basic", profile)
          == Result::InvalidUsername);
    CHECK(LocalAccounts::create("has space", "long-password", "Name", "basic", profile)
          == Result::InvalidUsername);
    CHECK(LocalAccounts::create(QString(33, 'a'), "long-password", "Name", "basic", profile)
          == Result::InvalidUsername);
    CHECK(LocalAccounts::create("valid_user", "long-password", "   ", "basic", profile)
          == Result::InvalidDisplayName);
    CHECK(LocalAccounts::create("valid_user", "long-password", QString(33, 'n'), "basic", profile)
          == Result::InvalidDisplayName);
    CHECK(LocalAccounts::create("valid_user", "short", "Name", "basic", profile)
          == Result::ShortPassword);
    CHECK(LocalAccounts::create("valid_user", "long-password", "Name", "not-a-plan", profile)
          == Result::InvalidDiningPlan);
    CHECK(LocalAccounts::signIn("valid_user", "long-password", profile)
          == Result::IncorrectCredentials);
}

TEST_CASE("Accounts start with their plan's points and sign in case-insensitively") {
    TestAccountStore store;
    const auto created = createAccount("Alice_1", "  Alice  ", "preferred");
    CHECK(created.displayName == "Alice");
    CHECK(created.balance == 2617.0);
    CHECK(created.friendId.size() == 12);

    LocalAccounts::Profile duplicate;
    CHECK(LocalAccounts::create("alice_1", "another-password", "Other", "basic", duplicate)
          == Result::UsernameTaken);

    LocalAccounts::Profile signedIn;
    CHECK(LocalAccounts::signIn("ALICE_1", "wrong-password", signedIn)
          == Result::IncorrectCredentials);
    REQUIRE(LocalAccounts::signIn(" ALICE_1 ", "correct-horse", signedIn) == Result::Success);
    CHECK(signedIn.displayName == created.displayName);
    CHECK(signedIn.friendId == created.friendId);
    CHECK(signedIn.balance == created.balance);
}

TEST_CASE("Saving a balance refuses stale or invalid values") {
    TestAccountStore store;
    createAccount("saver", "Saver"); // Basic plan: 2237 points
    CHECK(LocalAccounts::saveBalance("saver", 2237.0, 2000.0));
    // Another window still thinks the balance is 2237, so its write is refused.
    CHECK_FALSE(LocalAccounts::saveBalance("saver", 2237.0, 1500.0));
    CHECK_FALSE(LocalAccounts::saveBalance("saver", 2000.0, -1.0));
    CHECK_FALSE(LocalAccounts::saveBalance("nobody", 0.0, 10.0));

    QList<LocalAccounts::Purchase> none;
    REQUIRE(LocalAccounts::loadPurchases("saver", none));
    CHECK(none.isEmpty());
    double balance = 0.0;
    REQUIRE(LocalAccounts::recordPurchase(
                "saver", {QDate::currentDate(), "Marketplace", "Lunch", 1.0}, balance)
            == Result::Success);
    CHECK(balance == 1999.0);
}

TEST_CASE("Purchases are validated, deducted, listed newest first and can be removed") {
    TestAccountStore store;
    createAccount("buyer", "Buyer", "apartment"); // 336 points
    const QDate today = QDate::currentDate();
    double balance = -1.0;

    CHECK(LocalAccounts::recordPurchase("buyer", {today.addDays(1), "Marketplace", "", 5.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase("buyer", {today, "   ", "", 5.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase("buyer", {today, "Marketplace", "", 0.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase("buyer", {QDate(), "Marketplace", "", 5.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase(
              "buyer", {today, QString(LocalAccounts::PurchaseTextLimit + 1, 'x'), "", 5.0}, balance)
          == Result::InvalidPurchase);
    CHECK(LocalAccounts::recordPurchase("buyer", {today, "Marketplace", "", 336.01}, balance)
          == Result::InsufficientPoints);
    CHECK(balance == -1.0);

    REQUIRE(LocalAccounts::recordPurchase("buyer", {today.addDays(-2), "Fresh Bytes", "Boba", 6.20},
                                          balance) == Result::Success);
    CHECK(balance == doctest::Approx(329.80));
    REQUIRE(LocalAccounts::recordPurchase("buyer", {today, "  Marketplace  ", "  Pasta  ", 11.9},
                                          balance) == Result::Success);
    CHECK(balance == doctest::Approx(317.90));

    QList<LocalAccounts::Purchase> history;
    REQUIRE(LocalAccounts::loadPurchases("buyer", history));
    REQUIRE(history.size() == 2);
    CHECK(history.at(0).date == today);
    CHECK(history.at(0).location == "Marketplace");
    CHECK(history.at(0).item == "Pasta");
    CHECK_FALSE(history.at(0).id.isEmpty());
    CHECK(history.at(1).item == "Boba");

    REQUIRE(LocalAccounts::removePurchase("buyer", history.at(1), balance) == Result::Success);
    CHECK(balance == doctest::Approx(324.10));
    CHECK(LocalAccounts::removePurchase("buyer", history.at(1), balance)
          == Result::PurchaseUnavailable);

    // Spending the exact remaining balance is allowed and leaves 0.
    REQUIRE(LocalAccounts::recordPurchase("buyer", {today, "Marketplace", "", 324.10}, balance)
            == Result::Success);
    CHECK(balance == 0.0);
}

TEST_CASE("Friend requests can be sent, accepted and declined") {
    TestAccountStore store;
    const auto alice = createAccount("alice", "Alice");
    const auto bob = createAccount("bob", "Bob", "law");
    const auto carol = createAccount("carol", "Carol");

    CHECK(LocalAccounts::sendRequest("alice", "NOT-A-REAL-ID") == Result::UnknownFriendId);
    CHECK(LocalAccounts::sendRequest("alice", alice.friendId) == Result::OwnFriendId);
    // Friend IDs are matched regardless of case and surrounding spaces.
    CHECK(LocalAccounts::sendRequest("alice", "  " + bob.friendId.toLower() + " ")
          == Result::Success);
    CHECK(LocalAccounts::sendRequest("alice", bob.friendId) == Result::RequestAlreadySent);
    CHECK(LocalAccounts::sendRequest("bob", alice.friendId) == Result::IncomingRequestExists);

    LocalAccounts::Friends bobs;
    REQUIRE(LocalAccounts::loadFriends("bob", bobs));
    REQUIRE(bobs.incoming.size() == 1);
    CHECK(bobs.incoming.first().displayName == "Alice");
    CHECK(bobs.incoming.first().friendId == alice.friendId);
    CHECK(bobs.accepted.isEmpty());

    REQUIRE(LocalAccounts::respondToRequest("bob", alice.friendId, true) == Result::Success);
    CHECK(LocalAccounts::respondToRequest("bob", alice.friendId, true) == Result::RequestUnavailable);
    CHECK(LocalAccounts::sendRequest("alice", bob.friendId) == Result::AlreadyFriends);
    CHECK(LocalAccounts::sendRequest("bob", alice.friendId) == Result::AlreadyFriends);

    REQUIRE(LocalAccounts::loadFriends("bob", bobs));
    CHECK(bobs.incoming.isEmpty());
    REQUIRE(bobs.accepted.size() == 1);
    CHECK(bobs.accepted.first().displayName == "Alice");
    CHECK(bobs.accepted.first().balance == alice.balance);
    LocalAccounts::Friends alices;
    REQUIRE(LocalAccounts::loadFriends("alice", alices));
    REQUIRE(alices.accepted.size() == 1);
    CHECK(alices.accepted.first().displayName == "Bob");
    CHECK(alices.accepted.first().balance == 504.0);

    REQUIRE(LocalAccounts::sendRequest("carol", alice.friendId) == Result::Success);
    REQUIRE(LocalAccounts::respondToRequest("alice", carol.friendId, false) == Result::Success);
    REQUIRE(LocalAccounts::loadFriends("alice", alices));
    CHECK(alices.incoming.isEmpty());
    CHECK(alices.accepted.size() == 1);
    // A declined request can be sent again.
    CHECK(LocalAccounts::sendRequest("carol", alice.friendId) == Result::Success);
}

TEST_CASE("Days left are saved per account and restored at sign-in") {
    TestAccountStore store;
    const auto created = createAccount("planner", "Planner");
    CHECK(created.remainingDays == LocalAccounts::DefaultRemainingDays);

    CHECK(LocalAccounts::saveRemainingDays("planner", 12));
    CHECK_FALSE(LocalAccounts::saveRemainingDays("planner", -1));
    CHECK_FALSE(LocalAccounts::saveRemainingDays("planner", LocalAccounts::MaxRemainingDays + 1));
    CHECK_FALSE(LocalAccounts::saveRemainingDays("nobody", 5));

    LocalAccounts::Profile signedIn;
    REQUIRE(LocalAccounts::signIn("PLANNER", "correct-horse", signedIn) == Result::Success);
    CHECK(signedIn.remainingDays == 12);

    // 0 and the maximum are valid saved values.
    CHECK(LocalAccounts::saveRemainingDays("planner", 0));
    REQUIRE(LocalAccounts::signIn("planner", "correct-horse", signedIn) == Result::Success);
    CHECK(signedIn.remainingDays == 0);
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

TEST_CASE("Signing in on a later day shows fewer days left") {
    TestAccountStore store;
    const auto created = createAccount("counter", "Counter");
    CHECK(created.remainingDays == 77);

    const QDate today = QDate::currentDate();
    LocalAccounts::Profile signedIn;
    REQUIRE(LocalAccounts::saveRemainingDays("counter", 30, today.addDays(-5)));
    REQUIRE(LocalAccounts::signIn("counter", "correct-horse", signedIn) == Result::Success);
    CHECK(signedIn.remainingDays == 25);

    REQUIRE(LocalAccounts::saveRemainingDays("counter", 30, today.addDays(-45)));
    REQUIRE(LocalAccounts::signIn("counter", "correct-horse", signedIn) == Result::Success);
    CHECK(signedIn.remainingDays == 0);

    CHECK_FALSE(LocalAccounts::saveRemainingDays("counter", 30, QDate()));
}

TEST_CASE("A damaged saved balance is reported instead of crashing sign-in") {
    TestAccountStore store;
    createAccount("damaged", "Damaged");
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                             .filePath("accounts.json");
    QFile file(path);
    REQUIRE(file.open(QIODevice::ReadOnly));
    QJsonObject accounts = QJsonDocument::fromJson(file.readAll()).object();
    file.close();
    QJsonObject account = accounts.value("damaged").toObject();
    account.insert("balance", -5.0);
    accounts.insert("damaged", account);
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(QJsonDocument(accounts).toJson());
    file.close();

    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::signIn("damaged", "correct-horse", profile) == Result::StorageError);
}

// Author: Nathan Gu
// Remove each check as T01 implements the function.
TEST_CASE("Storage stubs fail safely until T01 implements them") {
    TestAccountStore store;
    createAccount("stubbed", "Stubbed");
    double balance = 123.0;

    LocalAccounts::Profile profile;
    profile.displayName = "stale";
    CHECK_FALSE(LocalAccounts::loadProfile("stubbed", profile));
    CHECK(profile.displayName.isEmpty());
    CHECK(LocalAccounts::correctBalance("stubbed", LocalAccounts::CorrectionMode::Add, 5, balance)
          == Result::StorageError);
    CHECK(balance == 123.0); // Output is untouched when nothing was saved.

    QList<LocalAccounts::HistoryEntry> history{{}};
    CHECK_FALSE(LocalAccounts::loadHistory("stubbed", history));
    CHECK(history.isEmpty());

    LocalAccounts::PlanSettings settings;
    settings.reserveGoal = 50;
    CHECK_FALSE(LocalAccounts::loadPlanSettings("stubbed", settings));
    CHECK(settings.reserveGoal == 0.0);
    CHECK(LocalAccounts::savePlanSettings("stubbed", settings) == Result::StorageError);
    CHECK(LocalAccounts::removeFriend("stubbed", "ABC") == Result::StorageError);
}
