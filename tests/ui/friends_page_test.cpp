// Tests for the Friends page (T05): pending sent requests and removing a friend.
// Each test uses a temporary database.

#include "doctest.h"
#include "friendspage.h"
#include "test_account_store.h"
#include "test_dialogs.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>

using LocalAccounts::Result;

namespace {
struct FriendsUi {
    FriendsPage &page;

    template <typename T> T *find(const char *name) {
        auto *widget = page.findChild<T *>(name);
        REQUIRE_MESSAGE(widget, name);
        return widget;
    }
    QPushButton *button(const QString &text) {
        for (auto *candidate : page.findChildren<QPushButton *>()) {
            if (candidate->text().remove('&') == text) {
                return candidate;
            }
        }
        FAIL("No button labelled " << text.toStdString());
        return nullptr;
    }
    QString status() { return find<QLabel>("friendsStatus")->text(); }
    QStringList sent() {
        QStringList items;
        auto *list = find<QListWidget>("sentRequests");
        for (int row = 0; row < list->count(); ++row) {
            items << list->item(row)->text();
        }
        return items;
    }
    QTableWidget *friends() { return find<QTableWidget>("acceptedFriends"); }
    void sendRequest(const QString &friendId) {
        find<QLineEdit>("requestFriendId")->setText(friendId);
        button("Send request")->click();
    }
};

struct People {
    LocalAccounts::Profile alice;
    LocalAccounts::Profile bob;
    LocalAccounts::Profile carol;
};

People createPeople(TestAccountStore &store) {
    People people{store.createVerified("alice@scu.edu", "Alice"),
                  store.createVerified("bob@scu.edu", "Bob"),
                  store.createVerified("carol@scu.edu", "Carol")};
    REQUIRE_FALSE(people.alice.accountId.isEmpty());
    REQUIRE_FALSE(people.bob.accountId.isEmpty());
    REQUIRE_FALSE(people.carol.accountId.isEmpty());
    return people;
}

void makeFriends(const LocalAccounts::Profile &from, const LocalAccounts::Profile &to) {
    REQUIRE(LocalAccounts::sendRequest(from.accountId, to.friendId) == Result::Success);
    REQUIRE(LocalAccounts::respondToRequest(to.accountId, from.friendId, true)
            == Result::Success);
}

QStringList friendNames(const QString &account) {
    LocalAccounts::Friends friends;
    REQUIRE(LocalAccounts::loadFriends(account, friends));
    QStringList names;
    for (const auto &entry : friends.accepted) {
        names << entry.displayName;
    }
    return names;
}
}

// Author: Nathan Gu
TEST_CASE("Sent requests are listed until the other person answers") {
    TestAccountStore store;
    const People people = createPeople(store);
    {
        FriendsPage page(people.alice.accountId, people.alice.friendId);
        FriendsUi ui{page};
        CHECK(ui.sent().isEmpty());
        CHECK_FALSE(ui.find<QLabel>("noSentRequests")->isHidden());

        ui.sendRequest(people.bob.friendId);
        ui.sendRequest(people.carol.friendId);
        CHECK(ui.status() == "Friend request sent.");
        CHECK(ui.sent()
              == QStringList{"Bob (" + people.bob.friendId + "), waiting for an answer",
                             "Carol (" + people.carol.friendId + "), waiting for an answer"});
        CHECK(ui.find<QLabel>("noSentRequests")->isHidden());
    }

    // Bob accepts and Carol declines; both leave Alice's sent list.
    REQUIRE(LocalAccounts::respondToRequest(people.bob.accountId, people.alice.friendId, true)
            == Result::Success);
    FriendsPage page(people.alice.accountId, people.alice.friendId);
    FriendsUi ui{page};
    CHECK(ui.sent() == QStringList{"Carol (" + people.carol.friendId + "), waiting for an answer"});
    REQUIRE(LocalAccounts::respondToRequest(people.carol.accountId, people.alice.friendId, false)
            == Result::Success);
    ui.button("Refresh")->click();
    CHECK(ui.sent().isEmpty());
    CHECK_FALSE(ui.find<QLabel>("noSentRequests")->isHidden());
    REQUIRE(ui.friends()->rowCount() == 1);
    CHECK(ui.friends()->item(0, 0)->text() == "Bob");
    CHECK(ui.friends()->item(0, 1)->text() == people.bob.friendId);
}

// Author: Nathan Gu
TEST_CASE("Removing a friend asks first and ends the friendship for both people") {
    TestAccountStore store;
    const People people = createPeople(store);
    makeFriends(people.alice, people.bob);
    makeFriends(people.carol, people.alice);

    FriendsPage page(people.alice.accountId, people.alice.friendId);
    FriendsUi ui{page};
    auto *remove = ui.find<QPushButton>("removeFriendButton");
    CHECK_FALSE(remove->isEnabled()); // Nothing selected yet.
    REQUIRE(ui.friends()->rowCount() == 2);
    const int bobRow = ui.friends()->item(0, 0)->text() == "Bob" ? 0 : 1;
    ui.friends()->selectRow(bobRow);
    CHECK(remove->isEnabled());

    QString question;
    answerConfirmation("Cancel", question);
    remove->click();
    CHECK(question
          == "Remove Bob from your friends? You'll no longer see each other's balances. "
             "Either of you can send a new request later.");
    CHECK(ui.status() == "Bob is still your friend.");
    CHECK(friendNames(people.bob.accountId) == QStringList{"Alice"});

    answerConfirmation("Remove", question);
    remove->click();
    CHECK(ui.status() == "Removed Bob. You no longer see each other's balances.");
    REQUIRE(ui.friends()->rowCount() == 1);
    CHECK(ui.friends()->item(0, 0)->text() == "Carol");
    CHECK_FALSE(remove->isEnabled());

    // Neither side sees the other any more; the other friendship is untouched.
    CHECK(friendNames(people.alice.accountId) == QStringList{"Carol"});
    CHECK(friendNames(people.bob.accountId).isEmpty());
    CHECK(friendNames(people.carol.accountId) == QStringList{"Alice"});
}

// Author: Nathan Gu
TEST_CASE("After removing a friend, either person can send a new request") {
    TestAccountStore store;
    const People people = createPeople(store);
    makeFriends(people.alice, people.bob);
    REQUIRE(LocalAccounts::removeFriend(people.bob.accountId, people.alice.friendId)
            == Result::Success);

    FriendsPage page(people.alice.accountId, people.alice.friendId);
    FriendsUi ui{page};
    CHECK(ui.friends()->rowCount() == 0);
    ui.sendRequest(people.bob.friendId);
    CHECK(ui.status() == "Friend request sent.");
    CHECK(ui.sent() == QStringList{"Bob (" + people.bob.friendId + "), waiting for an answer"});
}

// Author: Nathan Gu
TEST_CASE("Removing a friend who already removed you says so") {
    TestAccountStore store;
    const People people = createPeople(store);
    makeFriends(people.alice, people.bob);

    FriendsPage page(people.alice.accountId, people.alice.friendId);
    FriendsUi ui{page};
    ui.friends()->selectRow(0);
    // Bob removes Alice while her page is open.
    REQUIRE(LocalAccounts::removeFriend(people.bob.accountId, people.alice.friendId)
            == Result::Success);

    QString question;
    answerConfirmation("Remove", question);
    ui.find<QPushButton>("removeFriendButton")->click();
    CHECK(ui.status() == "Bob is no longer your friend.");
    CHECK(ui.friends()->rowCount() == 0);
}
