// Tests for the Profile tab (T02): account details, balance and days corrections,
// and changing the password. Each test uses a temporary database.

#include "doctest.h"
#include "profilepage.h"
#include "quarter.h"
#include "test_account_store.h"

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>

using LocalAccounts::CorrectionMode;
using LocalAccounts::HistoryEntry;

namespace {
struct ProfileUi {
    ProfilePage &page;

    template <typename T> T *find(const char *name) {
        auto *widget = page.findChild<T *>(name);
        REQUIRE_MESSAGE(widget, name);
        return widget;
    }
    QString text(const char *label) { return find<QLabel>(label)->text(); }
    void click(const char *button) { find<QPushButton>(button)->click(); }

    void correctBalance(CorrectionMode mode, double amount) {
        auto *modes = find<QComboBox>("correctionModeBox");
        modes->setCurrentIndex(modes->findData(static_cast<int>(mode)));
        find<QDoubleSpinBox>("correctionAmountSpinBox")->setValue(amount);
        click("correctBalanceButton");
    }
    void changePassword(const QString &current, const QString &next, const QString &confirm) {
        find<QLineEdit>("currentPasswordEdit")->setText(current);
        find<QLineEdit>("newPasswordEdit")->setText(next);
        find<QLineEdit>("confirmPasswordEdit")->setText(confirm);
        click("changePasswordButton");
    }
};

LocalAccounts::Profile saved(const QString &account) {
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::loadProfile(account, profile));
    return profile;
}

QString calendarDays() {
    const int days = Quarter::daysLeft(QDate::currentDate(), false, 0);
    return days == 1 ? QStringLiteral("1 day") : QStringLiteral("%1 days").arg(days);
}
}

// Author: Benson
TEST_CASE("The Profile tab shows the account") {
    TestAccountStore store;
    const auto created = store.createVerified("shown@scu.edu", "Shown", "apartment");
    ProfilePage page(created.accountId);
    ProfileUi ui{page};

    CHECK(ui.find<QLineEdit>("displayNameEdit")->text() == "Shown");
    CHECK(ui.find<QLineEdit>("emailEdit")->text() == "shown@scu.edu");
    CHECK(ui.text("friendIdLabel") == created.friendId);
    CHECK_FALSE(ui.text("diningPlanLabel").isEmpty());
    CHECK(ui.text("balanceLabel") == QString("%1 points").arg(created.balance, 0, 'f', 2));
    CHECK(ui.text("daysLabel") == calendarDays());
    CHECK_FALSE(ui.text("quarterLabel").isEmpty());
    CHECK(ui.text("lastUpdatedLabel") == "Not yet");

    ui.click("copyFriendIdButton");
    CHECK(QGuiApplication::clipboard()->text() == created.friendId);
}

// Author: Benson
TEST_CASE("Display name and email can be edited, and bad values are rejected") {
    TestAccountStore store;
    const QString account = store.createVerified("first@scu.edu", "First").accountId;
    store.createVerified("taken@scu.edu", "Taken");
    ProfilePage page(account);
    ProfileUi ui{page};
    auto *name = ui.find<QLineEdit>("displayNameEdit");
    auto *email = ui.find<QLineEdit>("emailEdit");

    name->setText("Renamed");
    email->setText("  New@SCU.edu ");
    ui.click("saveProfileButton");
    CHECK(ui.text("profileMessage") == "Profile saved.");
    CHECK(saved(account).displayName == "Renamed");
    CHECK(saved(account).email == "new@scu.edu");
    CHECK(email->text() == "new@scu.edu");

    email->setText("not-an-email");
    ui.click("saveProfileButton");
    CHECK(ui.text("profileMessage") == "Enter a valid email address.");

    email->setText("taken@scu.edu");
    ui.click("saveProfileButton");
    CHECK(ui.text("profileMessage") == "Another account already uses that email.");

    email->setText("new@scu.edu");
    name->setText("   ");
    ui.click("saveProfileButton");
    CHECK(ui.text("profileMessage") == "Enter a display name of 1 to 32 characters.");
    CHECK(saved(account).displayName == "Renamed");
    CHECK(saved(account).email == "new@scu.edu");
}

// Author: Benson
TEST_CASE("Set, Add and Subtract corrections are saved as history entries") {
    TestAccountStore store;
    const QString account = store.createVerified("fix@scu.edu", "Fix").accountId;
    ProfilePage page(account);
    ProfileUi ui{page};
    QSignalSpy updates(&page, &ProfilePage::balanceUpdated);

    ui.correctBalance(CorrectionMode::Set, 500);
    CHECK(ui.text("balanceMessage") == "Balance is now 500.00 points.");
    CHECK(ui.text("balanceLabel") == "500.00 points");
    ui.correctBalance(CorrectionMode::Add, 25.5);
    CHECK(ui.text("balanceLabel") == "525.50 points");
    ui.correctBalance(CorrectionMode::Subtract, 125.5);
    CHECK(ui.text("balanceLabel") == "400.00 points");
    CHECK(saved(account).balance == 400.0);
    CHECK(ui.text("lastUpdatedLabel") != "Not yet");

    REQUIRE(updates.size() == 3);
    CHECK(updates.last().first().toDouble() == 400.0);

    QList<HistoryEntry> history;
    REQUIRE(LocalAccounts::loadHistory(account, history));
    int corrections = 0;
    for (const auto &entry : history) {
        corrections += entry.kind == HistoryEntry::Kind::Correction;
    }
    CHECK(corrections == 3);
}

// Author: Benson
TEST_CASE("Balance corrections that would be invalid are rejected") {
    TestAccountStore store;
    const QString account = store.createVerified("reject@scu.edu", "Reject").accountId;
    double balance = 0.0;
    REQUIRE(LocalAccounts::correctBalance(account, CorrectionMode::Set, 100, balance)
            == LocalAccounts::Result::Success);
    ProfilePage page(account);
    ProfileUi ui{page};
    QSignalSpy updates(&page, &ProfilePage::balanceUpdated);

    ui.correctBalance(CorrectionMode::Subtract, 100.01);
    CHECK(ui.text("balanceMessage") == "You can't subtract more than your balance of 100.00 points.");
    ui.correctBalance(CorrectionMode::Add, 0);
    CHECK(ui.text("balanceMessage") == "Enter an amount greater than 0.");
    ui.correctBalance(CorrectionMode::Subtract, 0);
    CHECK(ui.text("balanceMessage") == "Enter an amount greater than 0.");

    CHECK(updates.isEmpty());
    CHECK(saved(account).balance == 100.0);
    CHECK(ui.text("balanceLabel") == "100.00 points");

    ui.correctBalance(CorrectionMode::Set, 0); // Setting to 0 is allowed.
    CHECK(saved(account).balance == 0.0);
}

// Author: Benson
TEST_CASE("Correct days saves a manual count, and Reset days returns to the calendar") {
    TestAccountStore store;
    const QString account = store.createVerified("days@scu.edu", "Days").accountId;
    ProfilePage page(account);
    ProfileUi ui{page};
    QSignalSpy updates(&page, &ProfilePage::daysUpdated);
    auto *reset = ui.find<QPushButton>("resetDaysButton");
    CHECK_FALSE(reset->isEnabled()); // Nothing to reset yet.

    ui.find<QSpinBox>("daysCorrectionSpinBox")->setValue(30);
    ui.click("correctDaysButton");
    CHECK(ui.text("daysLabel") == "30 days (corrected)");
    CHECK(ui.text("daysMessage") == "Days left set to 30. The count goes down by one each day.");
    CHECK(saved(account).daysCorrected);
    CHECK(saved(account).remainingDays == 30);
    CHECK(ProfilePage::daysLeftFor(saved(account)) == 30);

    if (Quarter::statusOn(QDate::currentDate()).phase != Quarter::Phase::CalendarEnded) {
        CHECK(reset->isEnabled());
        reset->click();
        CHECK_FALSE(saved(account).daysCorrected);
        CHECK(ui.text("daysLabel") == calendarDays());
        CHECK(ui.text("daysMessage").startsWith("Days left reset to the quarter calendar"));
        CHECK_FALSE(reset->isEnabled());
        REQUIRE(updates.size() == 2);
        CHECK(updates.last().first().toInt() == Quarter::statusOn(QDate::currentDate()).daysLeft);
    }
}

// Author: Benson
TEST_CASE("The days used come from the correction, or else the quarter calendar") {
    LocalAccounts::Profile profile;
    profile.remainingDays = 12;
    const QDate inFall(2026, 11, 22);
    profile.daysCorrected = false;
    CHECK(ProfilePage::daysLeftFor(profile, inFall) == 20);
    profile.daysCorrected = true;
    CHECK(ProfilePage::daysLeftFor(profile, inFall) == 12);
}

// Author: Benson
TEST_CASE("Changing the password checks each entry") {
    TestAccountStore store;
    const auto created = store.createVerified("pw@scu.edu", "Password");
    ProfilePage page(created.accountId);
    ProfileUi ui{page};

    ui.changePassword("wrong-password", "new-password", "new-password");
    CHECK(ui.text("passwordMessage") == "Your current password is incorrect.");
    ui.changePassword("correct-horse", "short", "short");
    CHECK(ui.text("passwordMessage") == "The new password must be at least 8 characters.");
    ui.changePassword("correct-horse", "new-password", "new-passw0rd");
    CHECK(ui.text("passwordMessage") == "The new passwords don't match.");

    ui.changePassword("correct-horse", "new-password", "new-password");
    CHECK(ui.text("passwordMessage") == "Password changed.");
    CHECK(ui.find<QLineEdit>("currentPasswordEdit")->text().isEmpty());

    LocalAccounts::Profile signedIn;
    CHECK(LocalAccounts::signIn("pw@scu.edu", "correct-horse", signedIn)
          == LocalAccounts::Result::IncorrectCredentials);
    CHECK(LocalAccounts::signIn("pw@scu.edu", "new-password", signedIn)
          == LocalAccounts::Result::Success);
}

// Author: Nathan Gu
TEST_CASE("Two-step sign-in is turned on and off from the Profile tab") {
    TestAccountStore store;
    const auto created = store.createVerified("twostep@scu.edu", "Careful");
    ProfilePage page(created.accountId);
    ProfileUi ui{page};
    auto *twoStep = ui.find<QCheckBox>("twoStepSignInCheckBox");
    CHECK_FALSE(twoStep->isChecked());

    twoStep->setChecked(true);
    CHECK(saved(created.accountId).twoStepSignIn);
    CHECK(ui.text("signInMessage").startsWith("Two-step sign-in is on."));

    twoStep->setChecked(false);
    CHECK_FALSE(saved(created.accountId).twoStepSignIn);
    CHECK(ui.text("signInMessage").startsWith("Two-step sign-in is off."));

    // Reopening the tab shows the saved choice.
    twoStep->setChecked(true);
    ProfilePage reopened(created.accountId);
    CHECK(ProfileUi{reopened}.find<QCheckBox>("twoStepSignInCheckBox")->isChecked());
}

// Author: Nathan Gu
TEST_CASE("If two-step sign-in can't be saved, the check box goes back") {
    TestAccountStore store;
    const auto created = store.createVerified("broken@scu.edu", "Broken");
    ProfilePage page(created.accountId);
    ProfileUi ui{page};
    auto *twoStep = ui.find<QCheckBox>("twoStepSignInCheckBox");
    // Point storage at a new, empty database: the account isn't there, so saving fails.
    LocalAccounts::closeStorage();
    {
        TestAccountStore other;
        twoStep->setChecked(true);
        CHECK_FALSE(twoStep->isChecked());
        CHECK(ui.text("signInMessage") == "Couldn't change two-step sign-in. Try again.");
    }
}

// Author: Nathan Gu
TEST_CASE("Sign out on the Profile tab asks the window to sign out") {
    TestAccountStore store;
    const auto created = store.createVerified("leaving@scu.edu", "Leaving");
    ProfilePage page(created.accountId);
    QSignalSpy signOut(&page, &ProfilePage::signOutRequested);
    ProfileUi{page}.click("signOutButton");
    CHECK(signOut.size() == 1);
}
