#include "authdialog.h"

#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

using LocalAccounts::Result;

namespace {
// Shows a busy cursor while an email is being sent, which can take a few seconds.
struct BusyCursor {
    BusyCursor() { QGuiApplication::setOverrideCursor(Qt::WaitCursor); }
    ~BusyCursor() { QGuiApplication::restoreOverrideCursor(); }
    BusyCursor(const BusyCursor &) = delete;
    BusyCursor &operator=(const BusyCursor &) = delete;
};
}

AuthDialog::AuthDialog(QWidget *parent)
    : QDialog(parent), heading(new QLabel(this)), form(new QFormLayout),
      email(new QLineEdit(this)), displayName(new QLineEdit(this)),
      diningPlan(new QComboBox(this)), passwordLabel(new QLabel(this)),
      password(new QLineEdit(this)), confirmation(new QLineEdit(this)),
      code(new QLineEdit(this)), message(new QLabel(this)), submitButton(new QPushButton(this)),
      switchButton(new QPushButton(this)), codeButton(new QPushButton(this)),
      forgotButton(new QPushButton(tr("&Forgot password?"), this)) {
    setWindowTitle(tr("Dining Points Budget"));
    setMinimumWidth(420);

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
    diningPlan->setObjectName("diningPlanComboBox");
    diningPlan->setPlaceholderText(tr("Select a dining plan"));
    for (const auto &plan : LocalAccounts::diningPlans()) {
        const QString points = locale().toString(plan.startingPoints);
        diningPlan->addItem(tr("%1 (%2 points per quarter)").arg(plan.name, points), plan.id);
    }
    password->setObjectName("passwordEdit");
    confirmation->setObjectName("confirmationEdit");
    password->setEchoMode(QLineEdit::Password);
    confirmation->setEchoMode(QLineEdit::Password);
    passwordLabel->setBuddy(password);
    code->setObjectName("codeEdit");
    code->setPlaceholderText(tr("6-digit code"));
    code->setMaxLength(6);
    code->setInputMethodHints(Qt::ImhDigitsOnly);
    message->setWordWrap(true);
    message->setObjectName("authMessage");
    submitButton->setObjectName("submitButton");
    switchButton->setObjectName("switchButton");
    codeButton->setObjectName("codeButton");
    forgotButton->setObjectName("forgotButton");
    forgotButton->setFlat(true);

    form->addRow(tr("&Email"), email);
    form->addRow(tr("&Display name"), displayName);
    form->addRow(tr("Dining &plan"), diningPlan);
    form->addRow(tr("&Code"), code);
    form->addRow(passwordLabel, password);
    form->addRow(tr("Con&firm password"), confirmation);

    submitButton->setDefault(true);
    for (auto *button : {switchButton, codeButton, forgotButton}) {
        button->setAutoDefault(false);
    }
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(switchButton);
    buttons->addWidget(forgotButton);
    buttons->addStretch();
    buttons->addWidget(codeButton);
    buttons->addWidget(submitButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(message);
    layout->addLayout(buttons);

    connect(submitButton, &QPushButton::clicked, this, &AuthDialog::submit);
    connect(codeButton, &QPushButton::clicked, this, &AuthDialog::sendCode);
    connect(forgotButton, &QPushButton::clicked, this, [this] { showScreen(Screen::ResetPassword); });
    connect(switchButton, &QPushButton::clicked, this, [this] {
        showScreen(screen == Screen::SignIn ? Screen::CreateAccount : Screen::SignIn);
    });
    reset();
}

void AuthDialog::reset() {
    email->clear();
    signedInProfile = {};
    pendingEmail.clear();
    pendingPassword.clear();
    showScreen(Screen::SignIn);
}

QString AuthDialog::accountName() const {
    return signedInProfile.accountId;
}

const LocalAccounts::Profile &AuthDialog::profile() const {
    return signedInProfile;
}

void AuthDialog::showScreen(Screen next) {
    screen = next;
    const bool signingIn = next == Screen::SignIn;
    const bool creating = next == Screen::CreateAccount;
    const bool verifying = next == Screen::VerifyEmail;
    const bool resetting = next == Screen::ResetPassword;

    static const char *const headings[] = {"Sign in", "Create an account", "Verify your email",
                                           "Reset your password"};
    static const char *const actions[] = {"&Sign in", "&Create account", "&Verify",
                                          "&Reset password"};
    heading->setText(tr(headings[static_cast<int>(next)]));
    submitButton->setText(tr(actions[static_cast<int>(next)]));
    switchButton->setText(signingIn ? tr("Create an account") : tr("Back to sign in"));
    codeButton->setText(verifying ? tr("Resend code") : tr("Send code"));
    passwordLabel->setText(resetting ? tr("&New password") : tr("&Password"));

    form->setRowVisible(email, !verifying);
    form->setRowVisible(displayName, creating);
    form->setRowVisible(diningPlan, creating);
    form->setRowVisible(code, verifying || resetting);
    form->setRowVisible(password, !verifying);
    form->setRowVisible(confirmation, creating || resetting);
    codeButton->setVisible(verifying || resetting);
    forgotButton->setVisible(signingIn);

    if (creating) {
        displayName->clear();
        diningPlan->setCurrentIndex(-1);
    }
    password->clear();
    confirmation->clear();
    code->clear();
    message->clear();
    (verifying ? code : email)->setFocus();
}

void AuthDialog::submit() {
    switch (screen) {
    case Screen::SignIn:
        signIn();
        break;
    case Screen::CreateAccount:
        createAccount();
        break;
    case Screen::VerifyEmail:
        verifyEmail();
        break;
    case Screen::ResetPassword:
        resetPassword();
        break;
    }
}

void AuthDialog::signIn() {
    const QString enteredEmail = email->text().trimmed();
    if (enteredEmail.isEmpty() || password->text().isEmpty()) {
        message->setText(tr("Enter your email and password."));
        return;
    }
    const Result result = LocalAccounts::signIn(enteredEmail, password->text(), signedInProfile);
    if (result == Result::Success) {
        password->clear();
        accept();
        return;
    }
    if (result == Result::EmailNotVerified) {
        pendingEmail = enteredEmail;
        pendingPassword = password->text();
        if (startVerification() == Result::Success) {
            message->setText(tr("Verify your email first. We sent a 6-digit code to %1.")
                                 .arg(pendingEmail));
        }
        return;
    }
    showError(result);
}

void AuthDialog::createAccount() {
    const QString enteredEmail = email->text().trimmed();
    if (enteredEmail.isEmpty() || password->text().isEmpty()) {
        message->setText(tr("Enter your email and password."));
        return;
    }
    if (password->text() != confirmation->text()) {
        message->setText(tr("Passwords do not match."));
        return;
    }
    const Result result = LocalAccounts::create(enteredEmail, password->text(), displayName->text(),
                                                diningPlan->currentData().toString(),
                                                signedInProfile);
    if (result != Result::Success) {
        showError(result);
        return;
    }
    pendingEmail = signedInProfile.email;
    pendingPassword = password->text();
    signedInProfile = {}; // Not signed in until the email is verified.
    if (startVerification() == Result::Success) {
        message->setText(tr("We sent a 6-digit code to %1. Enter it to finish creating your account.")
                             .arg(pendingEmail));
        return;
    }
    // The code couldn't be sent: remove the half-made account so nothing is left behind.
    LocalAccounts::cancelSignUp(pendingEmail);
    pendingEmail.clear();
    pendingPassword.clear();
}

Result AuthDialog::startVerification() {
    Result result;
    {
        BusyCursor busy;
        result = LocalAccounts::sendVerificationCode(pendingEmail);
    }
    if (result != Result::Success) {
        showError(result);
        return result;
    }
    showScreen(Screen::VerifyEmail);
    return result;
}

void AuthDialog::verifyEmail() {
    if (code->text().trimmed().isEmpty()) {
        message->setText(tr("Enter the 6-digit code from the email."));
        return;
    }
    const Result result = LocalAccounts::verifyEmail(pendingEmail, code->text());
    if (result != Result::Success) {
        showError(result);
        code->clear();
        code->setFocus();
        return;
    }
    if (LocalAccounts::signIn(pendingEmail, pendingPassword, signedInProfile) != Result::Success) {
        showScreen(Screen::SignIn);
        email->setText(pendingEmail);
        message->setText(tr("Your email is verified. Sign in to continue."));
        return;
    }
    pendingPassword.clear();
    accept();
}

void AuthDialog::resetPassword() {
    if (email->text().trimmed().isEmpty() || code->text().trimmed().isEmpty()
        || password->text().isEmpty()) {
        message->setText(tr("Enter your email, the code we sent, and a new password."));
        return;
    }
    if (password->text() != confirmation->text()) {
        message->setText(tr("Passwords do not match."));
        return;
    }
    const Result result = LocalAccounts::resetPassword(email->text(), code->text(), password->text());
    if (result != Result::Success) {
        showError(result);
        return;
    }
    const QString address = email->text();
    showScreen(Screen::SignIn);
    email->setText(address);
    message->setText(tr("Password changed. Sign in with your new password."));
    password->setFocus();
}

void AuthDialog::sendCode() {
    Result result;
    QString address;
    {
        BusyCursor busy;
        if (screen == Screen::VerifyEmail) {
            address = pendingEmail;
            result = LocalAccounts::sendVerificationCode(address);
        } else {
            address = email->text().trimmed();
            if (address.isEmpty()) {
                message->setText(tr("Enter the email for your account."));
                return;
            }
            result = LocalAccounts::sendPasswordResetCode(address);
        }
    }
    if (result == Result::Success) {
        message->setText(tr("We sent a 6-digit code to %1. It expires in %2 minutes.")
                             .arg(address)
                             .arg(LocalAccounts::CodeLifetimeMinutes));
        code->setFocus();
        return;
    }
    showError(result);
}

void AuthDialog::showError(Result result) {
    switch (result) {
    case Result::InvalidEmail:
        message->setText(screen == Screen::ResetPassword
                             ? tr("No account uses that email. Check it or create an account.")
                             : tr("Enter a valid email address, such as you@scu.edu."));
        email->setFocus();
        break;
    case Result::InvalidDisplayName:
        message->setText(tr("Enter a display name of up to %1 characters.")
                             .arg(LocalAccounts::MaxDisplayNameLength));
        displayName->setFocus();
        break;
    case Result::InvalidDiningPlan:
        message->setText(tr("Select a dining plan to create your account."));
        diningPlan->setFocus();
        break;
    case Result::ShortPassword:
        message->setText(tr("Choose a password with at least %1 characters.")
                             .arg(LocalAccounts::MinPasswordLength));
        password->setFocus();
        break;
    case Result::EmailTaken:
        message->setText(tr("An account already uses that email. Sign in instead."));
        break;
    case Result::IncorrectCredentials:
        message->setText(tr("Incorrect email or password."));
        password->clear();
        password->setFocus();
        break;
    case Result::EmailNotVerified:
        message->setText(tr("This account's email isn't verified yet. Sign in to get a new code."));
        break;
    case Result::InvalidCode:
        message->setText(tr("That code isn't right. Check the email, or send a new code."));
        break;
    case Result::CodeExpired:
        message->setText(tr("That code has expired. Send a new code and try again."));
        break;
    case Result::CodeNotSent:
        message->setText(tr("Couldn't send the code. Try again."));
        break;
    default:
        message->setText(tr("Could not open the account database. Try again."));
        break;
    }
}
