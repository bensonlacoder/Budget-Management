// Tests for the sign-in dialog: creating an account and signing in with an email.
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
    QLabel *message = dialog.findChild<QLabel *>("authMessage");
    QPushButton *submit = dialog.findChild<QPushButton *>("submitButton");
    QPushButton *toggle = dialog.findChild<QPushButton *>("switchButton");

    explicit AuthForm(AuthDialog &d) : dialog(d) {
        for (QObject *widget : std::initializer_list<QObject *>{
                 email, displayName, plan, password, confirmation, message, submit, toggle}) {
            REQUIRE(widget);
        }
    }
};
}

TEST_CASE("The sign-in dialog creates an account and signs in with an email") {
    TestAccountStore store;
    AuthDialog dialog;
    AuthForm form(dialog);

    // Only quarter plans are offered.
    for (int i = 0; i < form.plan->count(); ++i) {
        CHECK(form.plan->itemText(i).contains("per quarter"));
        CHECK(form.plan->itemData(i).toString() != "law");
    }

    form.toggle->click(); // Switch to "Create an account".
    form.email->setText("not-an-email");
    form.displayName->setText("Nathan");
    form.plan->setCurrentIndex(0);
    form.password->setText("long-password");
    form.confirmation->setText("long-password");
    form.submit->click();
    CHECK(form.message->text() == "Enter a valid email address, such as you@scu.edu.");
    CHECK(dialog.result() != QDialog::Accepted);

    form.email->setText("  Nathan@SCU.edu ");
    form.password->setText("long-password");
    form.confirmation->setText("long-password");
    form.submit->click();
    REQUIRE(dialog.result() == QDialog::Accepted);
    CHECK(dialog.profile().email == "nathan@scu.edu");
    CHECK(dialog.accountName() == dialog.profile().accountId);
    CHECK_FALSE(dialog.accountName().isEmpty());
    const QString accountId = dialog.accountName();

    // Signing up again with the same email is refused.
    dialog.reset();
    dialog.setResult(QDialog::Rejected);
    form.toggle->click();
    form.email->setText("nathan@scu.edu");
    form.displayName->setText("Someone else");
    form.plan->setCurrentIndex(0);
    form.password->setText("long-password");
    form.confirmation->setText("long-password");
    form.submit->click();
    CHECK(form.message->text() == "An account already uses that email. Sign in instead.");

    // Sign in: a wrong password is refused, the right one returns the same account.
    form.toggle->click(); // Back to sign in.
    form.email->setText("NATHAN@scu.edu");
    form.password->setText("wrong-password");
    form.submit->click();
    CHECK(form.message->text() == "Incorrect email or password.");
    CHECK(dialog.result() != QDialog::Accepted);
    form.password->setText("long-password");
    form.submit->click();
    REQUIRE(dialog.result() == QDialog::Accepted);
    CHECK(dialog.accountName() == accountId);
}
