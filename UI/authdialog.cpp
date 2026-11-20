#include "authdialog.h"
#include "localaccounts.h"

#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

AuthDialog::AuthDialog(QWidget *parent)
    : QDialog(parent), heading(new QLabel(this)), username(new QLineEdit(this)),
      password(new QLineEdit(this)), confirmationLabel(new QLabel(tr("Con&firm password"), this)),
      confirmation(new QLineEdit(this)), message(new QLabel(this)),
      submitButton(new QPushButton(this)), switchButton(new QPushButton(this)) {
    setWindowTitle(tr("Dining Points Budget"));
    setMinimumWidth(400);

    QFont headingFont = heading->font();
    headingFont.setPointSize(18);
    headingFont.setBold(true);
    heading->setFont(headingFont);

    username->setObjectName("usernameEdit");
    password->setObjectName("passwordEdit");
    confirmation->setObjectName("confirmationEdit");
    password->setEchoMode(QLineEdit::Password);
    confirmation->setEchoMode(QLineEdit::Password);
    confirmationLabel->setBuddy(confirmation);
    message->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("&Username"), username);
    form->addRow(tr("&Password"), password);
    form->addRow(confirmationLabel, confirmation);

    submitButton->setDefault(true);
    switchButton->setAutoDefault(false);
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(switchButton);
    buttons->addStretch();
    buttons->addWidget(submitButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(message);
    layout->addLayout(buttons);

    connect(submitButton, &QPushButton::clicked, this, &AuthDialog::submit);
    connect(switchButton, &QPushButton::clicked, this, [this] {
        setCreatingAccount(!creatingAccount);
    });
    reset();
}

void AuthDialog::reset() {
    username->clear();
    setCreatingAccount(false);
}

void AuthDialog::setCreatingAccount(bool creating) {
    creatingAccount = creating;
    heading->setText(creating ? tr("Create an account") : tr("Sign in"));
    submitButton->setText(creating ? tr("&Create account") : tr("&Sign in"));
    switchButton->setText(creating ? tr("Back to sign in") : tr("Create an account"));
    confirmationLabel->setVisible(creating);
    confirmation->setVisible(creating);
    password->clear();
    confirmation->clear();
    message->clear();
    username->setFocus();
}

void AuthDialog::submit() {
    const QString enteredUsername = username->text().trimmed();
    const QString enteredPassword = password->text();
    if (enteredUsername.isEmpty() || enteredPassword.isEmpty()) {
        message->setText(tr("Enter your username and password."));
        return;
    }
    if (creatingAccount && enteredPassword != confirmation->text()) {
        message->setText(tr("Passwords do not match."));
        return;
    }

    using LocalAccounts::Result;
    const Result result = creatingAccount
        ? LocalAccounts::create(enteredUsername, enteredPassword)
        : LocalAccounts::signIn(enteredUsername, enteredPassword);

    switch (result) {
    case Result::Success:
        password->clear();
        confirmation->clear();
        accept();
        return;
    case Result::InvalidUsername:
        message->setText(tr("Use 3 to 32 letters, numbers, or underscores for your username."));
        break;
    case Result::ShortPassword:
        message->setText(tr("Choose a password with at least 8 characters."));
        break;
    case Result::UsernameTaken:
        message->setText(tr("That username is already in use."));
        break;
    case Result::IncorrectCredentials:
        message->setText(tr("Incorrect username or password."));
        password->clear();
        password->setFocus();
        break;
    case Result::StorageError:
        message->setText(tr("Could not access local accounts. Try again."));
        break;
    }
}
