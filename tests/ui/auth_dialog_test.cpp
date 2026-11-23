// Tests for the sign-in dialog: creating an account, verifying its email with a code,
// signing in, and resetting a forgotten password. Codes come from the fake email
// sender in TestAccountStore; no real email is sent.
// Author: Nathan Gu

#include "authdialog.h"
#include "doctest.h"
#include "test_account_store.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace {
struct AuthForm {
    AuthDialog &dialog;
    QLineEdit *email = dialog.findChild<QLineEdit *>("emailEdit");
    QLineEdit *displayName = dialog.findChild<QLineEdit *>("displayNameEdit");
    QComboBox *plan = dialog.findChild<QComboBox *>("diningPlanComboBox");
    QLineEdit *password = dialog.findChild<QLineEdit *>("passwordEdit");
    QLineEdit *confirmation = dialog.findChild<QLineEdit *>("confirmationEdit");
    QLineEdit *code = dialog.findChild<QLineEdit *>("codeEdit");
    QLabel *message = dialog.findChild<QLabel *>("authMessage");
    QPushButton *submit = dialog.findChild<QPushButton *>("submitButton");
    QPushButton *toggle = dialog.findChild<QPushButton *>("switchButton");
    QPushButton *sendCode = dialog.findChild<QPushButton *>("codeButton");
    QPushButton *forgot = dialog.findChild<QPushButton *>("forgotButton");

    explicit AuthForm(AuthDialog &d) : dialog(d) {
        for (QObject *widget : std::initializer_list<QObject *>{
                 email, displayName, plan, password, confirmation, code, message, submit, toggle,
                 sendCode, forgot}) {
            REQUIRE(widget);
        }
        dialog.show();
    }

    // Fills in the create-account screen and submits it.
    void createAccount(const QString &address, const QString &secret = "long-password") {
        if (submit->text() != "&Create account") {
            toggle->click();
        }
        REQUIRE(submit->text() == "&Create account");
        email->setText(address);
        displayName->setText("Nathan");
        plan->setCurrentIndex(0);
        password->setText(secret);
        confirmation->setText(secret);
        submit->click();
    }

    bool accepted() const { return dialog.result() == QDialog::Accepted; }
};
}

TEST_CASE("Creating an account sends a code, and the account is ready once it's entered") {
    TestAccountStore store;
    AuthDialog dialog;
    AuthForm form(dialog);

    // Only quarter plans are offered.
    for (int i = 0; i < form.plan->count(); ++i) {
        CHECK(form.plan->itemText(i).contains("per quarter"));
        CHECK(form.plan->itemData(i).toString() != "law");
    }

    form.createAccount("not-an-email");
    CHECK(form.message->text() == "Enter a valid email address, such as you@scu.edu.");
    CHECK_FALSE(form.accepted());

    form.createAccount("  Nathan@SCU.edu ");
    CHECK(form.submit->text() == "&Verify");
    CHECK(form.code->isVisibleTo(&dialog));
    CHECK_FALSE(form.email->isVisibleTo(&dialog));
    CHECK(form.message->text()
          == "We sent a 6-digit code to nathan@scu.edu. Enter it to finish creating your account.");
    CHECK_FALSE(form.accepted());
    const QString code = store.sentCodes.value("nathan@scu.edu");
    REQUIRE(code.size() == 6);

    form.code->setText(code == "000000" ? "111111" : "000000");
    form.submit->click();
    CHECK(form.message->text() == "That code isn't right. Check the email, or send a new code.");
    CHECK_FALSE(form.accepted());

    form.sendCode->click(); // Resend: the old code stops working.
    const QString resent = store.sentCodes.value("nathan@scu.edu");
    CHECK(form.message->text().startsWith("We sent a 6-digit code to nathan@scu.edu."));
    form.code->setText(resent);
    form.submit->click();
    REQUIRE(form.accepted());
    CHECK(dialog.profile().email == "nathan@scu.edu");
    CHECK_FALSE(dialog.accountName().isEmpty());
    CHECK(dialog.accountName() == dialog.profile().accountId);
    const QString accountId = dialog.accountName();

    // The email is now taken, and signing in returns the same account.
    dialog.reset();
    dialog.setResult(QDialog::Rejected);
    form.createAccount("nathan@scu.edu");
    CHECK(form.message->text() == "An account already uses that email. Sign in instead.");
    form.toggle->click(); // Back to sign in.
    form.email->setText("NATHAN@scu.edu");
    form.password->setText("wrong-password");
    form.submit->click();
    CHECK(form.message->text() == "Incorrect email or password.");
    CHECK_FALSE(form.accepted());
    form.password->setText("long-password");
    form.submit->click();
    REQUIRE(form.accepted());
    CHECK(dialog.accountName() == accountId);
}

TEST_CASE("Signing in before verifying sends a code first") {
    TestAccountStore store;
    AuthDialog dialog;
    AuthForm form(dialog);
    form.createAccount("later@scu.edu");
    REQUIRE(form.submit->text() == "&Verify");
    form.toggle->click(); // Leave without entering the code.

    form.email->setText("later@scu.edu");
    form.password->setText("long-password");
    form.submit->click();
    CHECK(form.submit->text() == "&Verify");
    CHECK(form.message->text() == "Verify your email first. We sent a 6-digit code to later@scu.edu.");
    form.code->setText(store.sentCodes.value("later@scu.edu"));
    form.submit->click();
    REQUIRE(form.accepted());
    CHECK(dialog.profile().email == "later@scu.edu");
}

TEST_CASE("A sign-up whose code can't be sent is undone") {
    TestAccountStore store;
    store.sendingFails = true;
    AuthDialog dialog;
    AuthForm form(dialog);
    form.createAccount("offline@scu.edu");
    CHECK(form.message->text() == "Couldn't send the code. Try again.");
    CHECK(form.submit->text() == "&Create account");
    LocalAccounts::Profile profile;
    CHECK(LocalAccounts::signIn("offline@scu.edu", "long-password", profile)
          == LocalAccounts::Result::IncorrectCredentials);

    // Trying again once sending works goes through.
    store.sendingFails = false;
    form.createAccount("offline@scu.edu");
    CHECK(form.submit->text() == "&Verify");
}

TEST_CASE("A forgotten password is reset from the sign-in screen") {
    TestAccountStore store;
    store.createVerified("forgot@scu.edu", "Forgetful");
    AuthDialog dialog;
    AuthForm form(dialog);

    form.forgot->click();
    CHECK(form.submit->text() == "&Reset password");
    CHECK(form.code->isVisibleTo(&dialog));
    CHECK(form.confirmation->isVisibleTo(&dialog));

    form.email->setText("nobody@scu.edu");
    form.sendCode->click();
    CHECK(form.message->text() == "No account uses that email. Check it or create an account.");

    form.email->setText("forgot@scu.edu");
    form.sendCode->click();
    CHECK(form.message->text() == "We sent a 6-digit code to forgot@scu.edu. It expires in 15 minutes.");
    form.code->setText(store.sentCodes.value("forgot@scu.edu"));
    form.password->setText("new-password");
    form.confirmation->setText("different");
    form.submit->click();
    CHECK(form.message->text() == "Passwords do not match.");
    form.confirmation->setText("new-password");
    form.submit->click();
    CHECK(form.submit->text() == "&Sign in");
    CHECK(form.message->text() == "Password changed. Sign in with your new password.");
    CHECK(form.email->text() == "forgot@scu.edu");

    form.password->setText("new-password");
    form.submit->click();
    REQUIRE(form.accepted());
}
