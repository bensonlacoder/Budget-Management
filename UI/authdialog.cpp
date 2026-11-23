#include "authdialog.h"
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

AuthDialog::AuthDialog(QWidget *parent)
    : QDialog(parent), heading(new QLabel(this)), email(new QLineEdit(this)),
      displayNameLabel(new QLabel(tr("&Display name"), this)), displayName(new QLineEdit(this)),
      diningPlanLabel(new QLabel(tr("Dining &plan"), this)), diningPlan(new QComboBox(this)),
      password(new QLineEdit(this)), confirmationLabel(new QLabel(tr("Con&firm password"), this)),
      confirmation(new QLineEdit(this)), message(new QLabel(this)),
      submitButton(new QPushButton(this)), switchButton(new QPushButton(this)) {
    setWindowTitle(tr("Dining Points Budget"));
    setMinimumWidth(400);

    QFont headingFont = heading->font();
    headingFont.setPointSize(18);
    headingFont.setBold(true);
    heading->setFont(headingFont);

    email->setObjectName("emailEdit");
    email->setPlaceholderText(tr("you@scu.edu"));
    email->setMaxLength(LocalAccounts::MaxEmailLength);
    displayName->setObjectName("displayNameEdit");
    displayName->setPlaceholderText(tr("Visible to friends"));
    displayName->setMaxLength(LocalAccounts::MaxDisplayNameLength);
    displayNameLabel->setBuddy(displayName);
    diningPlan->setObjectName("diningPlanComboBox");
    diningPlan->setPlaceholderText(tr("Select a dining plan"));
    diningPlanLabel->setBuddy(diningPlan);
    for (const auto &plan : LocalAccounts::diningPlans()) {
        const QString points = locale().toString(plan.startingPoints);
        diningPlan->addItem(tr("%1 (%2 points per quarter)").arg(plan.name, points), plan.id);
    }
    password->setObjectName("passwordEdit");
    confirmation->setObjectName("confirmationEdit");
    password->setEchoMode(QLineEdit::Password);
    confirmation->setEchoMode(QLineEdit::Password);
    confirmationLabel->setBuddy(confirmation);
    message->setWordWrap(true);
    message->setObjectName("authMessage");
    submitButton->setObjectName("submitButton");
    switchButton->setObjectName("switchButton");

    auto *form = new QFormLayout;
    form->addRow(tr("&Email"), email);
    form->addRow(displayNameLabel, displayName);
    form->addRow(diningPlanLabel, diningPlan);
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
    email->clear();
    signedInProfile = {};
    setCreatingAccount(false);
}

QString AuthDialog::accountName() const {
    return signedInProfile.accountId;
}

const LocalAccounts::Profile &AuthDialog::profile() const {
    return signedInProfile;
}

void AuthDialog::setCreatingAccount(bool creating) {
    creatingAccount = creating;
    heading->setText(creating ? tr("Create an account") : tr("Sign in"));
    submitButton->setText(creating ? tr("&Create account") : tr("&Sign in"));
    switchButton->setText(creating ? tr("Back to sign in") : tr("Create an account"));
    displayNameLabel->setVisible(creating);
    displayName->setVisible(creating);
    displayName->clear();
    diningPlanLabel->setVisible(creating);
    diningPlan->setVisible(creating);
    diningPlan->setCurrentIndex(-1);
    confirmationLabel->setVisible(creating);
    confirmation->setVisible(creating);
    password->clear();
    confirmation->clear();
    message->clear();
    email->setFocus();
}

void AuthDialog::submit() {
    const QString enteredEmail = email->text().trimmed();
    const QString enteredPassword = password->text();
    if (enteredEmail.isEmpty() || enteredPassword.isEmpty()) {
        message->setText(tr("Enter your email and password."));
        return;
    }
    if (creatingAccount && enteredPassword != confirmation->text()) {
        message->setText(tr("Passwords do not match."));
        return;
    }

    using LocalAccounts::Result;
    const Result result = creatingAccount
        ? LocalAccounts::create(enteredEmail, enteredPassword, displayName->text(),
                                diningPlan->currentData().toString(), signedInProfile)
        : LocalAccounts::signIn(enteredEmail, enteredPassword, signedInProfile);

    switch (result) {
    case Result::Success:
        password->clear();
        confirmation->clear();
        accept();
        return;
    case Result::InvalidEmail:
        message->setText(tr("Enter a valid email address, such as you@scu.edu."));
        email->setFocus();
        break;
    case Result::InvalidDisplayName:
        message->setText(tr("Enter a display name of up to 32 characters."));
        displayName->setFocus();
        break;
    case Result::InvalidDiningPlan:
        message->setText(tr("Select a dining plan to create your account."));
        diningPlan->setFocus();
        break;
    case Result::ShortPassword:
        message->setText(tr("Choose a password with at least 8 characters."));
        break;
    case Result::EmailTaken:
        message->setText(tr("An account already uses that email. Sign in instead."));
        break;
    case Result::IncorrectCredentials:
        message->setText(tr("Incorrect email or password."));
        password->clear();
        password->setFocus();
        break;
    default:
        message->setText(tr("Could not open the account database. Try again."));
        break;
    }
}
