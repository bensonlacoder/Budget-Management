#include "profilepage.h"
#include "appstyle.h"

#include "quarter.h"

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

using LocalAccounts::CorrectionMode;
using LocalAccounts::Result;

namespace {
constexpr int DATE_CHECK_INTERVAL_MS = 60 * 1000;

QString formatDate(const QDate &date) {
    return AppStyle::date(date);
}

QString dayCount(int days) {
    return days == 1 ? ProfilePage::tr("1 day") : ProfilePage::tr("%1 days").arg(days);
}

QLabel *messageLabel(const char *name, QWidget *parent) {
    auto *label = new QLabel(parent);
    label->setObjectName(QLatin1String(name));
    label->setWordWrap(true);
    label->hide(); // Takes space only once showMessage() gives it text.
    return label;
}

QPushButton *button(const QString &text, const char *name, QWidget *parent) {
    auto *result = new QPushButton(text, parent);
    result->setObjectName(QLatin1String(name));
    return result;
}

// A row holding just `button`, at its normal width on the right.
QHBoxLayout *rightAligned(QPushButton *button) {
    auto *row = new QHBoxLayout;
    row->addStretch();
    row->addWidget(button);
    return row;
}

QLineEdit *passwordEdit(const char *name, QWidget *parent) {
    auto *edit = new QLineEdit(parent);
    edit->setObjectName(QLatin1String(name));
    edit->setEchoMode(QLineEdit::Password);
    return edit;
}
}

ProfilePage::ProfilePage(const QString &accountName, QWidget *parent)
    : QWidget(parent), accountName(accountName), shownDate(QDate::currentDate()) {
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);

    // ---- Account ----
    auto *accountBox = new QGroupBox(tr("Account"), content);
    auto *accountForm = new QFormLayout(accountBox);
    displayName = new QLineEdit(accountBox);
    displayName->setObjectName("displayNameEdit");
    displayName->setMaxLength(LocalAccounts::MaxDisplayNameLength);
    email = new QLineEdit(accountBox);
    email->setObjectName("emailEdit");
    email->setMaxLength(LocalAccounts::MaxEmailLength);
    accountForm->addRow(tr("Display &name:"), displayName);
    accountForm->addRow(tr("&Email:"), email);
    auto *saveProfileButton = button(tr("Save profile"), "saveProfileButton", accountBox);
    profileMessage = messageLabel("profileMessage", accountBox);
    accountForm->addRow(QString(), rightAligned(saveProfileButton));
    accountForm->addRow(QString(), profileMessage);

    friendId = new QLabel(accountBox);
    friendId->setObjectName("friendIdLabel");
    friendId->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *copyButton = button(tr("Copy"), "copyFriendIdButton", accountBox);
    auto *friendRow = new QHBoxLayout;
    friendRow->addWidget(friendId);
    friendRow->addWidget(copyButton);
    friendRow->addStretch();
    accountForm->addRow(tr("Friend ID:"), friendRow);
    diningPlan = new QLabel(accountBox);
    diningPlan->setObjectName("diningPlanLabel");
    accountForm->addRow(tr("Dining plan:"), diningPlan);
    layout->addWidget(accountBox);

    // ---- Balance ----
    auto *balanceBox = new QGroupBox(tr("Points"), content);
    auto *balanceForm = new QFormLayout(balanceBox);
    balance = new QLabel(balanceBox);
    balance->setObjectName("balanceLabel");
    AppStyle::setTextRole(balance, "strong");
    balanceForm->addRow(tr("Points left:"), balance);
    lastUpdated = new QLabel(balanceBox);
    lastUpdated->setObjectName("lastUpdatedLabel");
    balanceForm->addRow(tr("Last updated:"), lastUpdated);
    correctionMode = new QComboBox(balanceBox);
    correctionMode->setObjectName("correctionModeBox");
    correctionMode->addItem(tr("Set to"), static_cast<int>(CorrectionMode::Set));
    correctionMode->addItem(tr("Add"), static_cast<int>(CorrectionMode::Add));
    correctionMode->addItem(tr("Subtract"), static_cast<int>(CorrectionMode::Subtract));
    correctionAmount = new QDoubleSpinBox(balanceBox);
    correctionAmount->setObjectName("correctionAmountSpinBox");
    correctionAmount->setDecimals(2);
    correctionAmount->setRange(0.0, LocalAccounts::MaxPurchasePoints);
    auto *correctButton = button(tr("Correct balance"), "correctBalanceButton", balanceBox);
    auto *correctionRow = new QHBoxLayout;
    correctionRow->addWidget(correctionMode);
    correctionRow->addWidget(correctionAmount, 1);
    correctionRow->addWidget(correctButton);
    // Rows whose field is a layout need their buddy set by hand (see planpage.cpp).
    auto *correctionLabel = new QLabel(tr("Correc&t balance:"), balanceBox);
    correctionLabel->setBuddy(correctionMode);
    balanceForm->addRow(correctionLabel, correctionRow);
    balanceMessage = messageLabel("balanceMessage", balanceBox);
    balanceForm->addRow(QString(), balanceMessage);
    layout->addWidget(balanceBox);

    // ---- Days left ----
    auto *daysBox = new QGroupBox(tr("Days left"), content);
    auto *daysForm = new QFormLayout(daysBox);
    days = new QLabel(daysBox);
    days->setObjectName("daysLabel");
    AppStyle::setTextRole(days, "strong");
    daysForm->addRow(tr("Days left (including today):"), days);
    quarter = new QLabel(daysBox);
    quarter->setObjectName("quarterLabel");
    quarter->setWordWrap(true);
    daysForm->addRow(tr("Quarter:"), quarter);
    daysCorrection = new QSpinBox(daysBox);
    daysCorrection->setObjectName("daysCorrectionSpinBox");
    daysCorrection->setRange(0, LocalAccounts::MaxRemainingDays);
    daysCorrection->setToolTip(tr("Enter 0 to %1 days, including today. A corrected count "
                                  "goes down by one each day.")
                                   .arg(LocalAccounts::MaxRemainingDays));
    auto *correctDaysButton = button(tr("Correct days"), "correctDaysButton", daysBox);
    resetDaysButton = button(tr("Reset days"), "resetDaysButton", daysBox);
    resetDaysButton->setToolTip(tr("Use the days left from the quarter calendar again."));
    auto *daysRow = new QHBoxLayout;
    daysRow->addWidget(daysCorrection, 1);
    daysRow->addWidget(correctDaysButton);
    daysRow->addWidget(resetDaysButton);
    auto *daysCorrectionLabel = new QLabel(tr("Correct &days:"), daysBox);
    daysCorrectionLabel->setBuddy(daysCorrection);
    daysForm->addRow(daysCorrectionLabel, daysRow);
    daysMessage = messageLabel("daysMessage", daysBox);
    daysForm->addRow(QString(), daysMessage);
    layout->addWidget(daysBox);

    // ---- Password ----
    auto *passwordBox = new QGroupBox(tr("Change password"), content);
    auto *passwordForm = new QFormLayout(passwordBox);
    currentPassword = passwordEdit("currentPasswordEdit", passwordBox);
    newPassword = passwordEdit("newPasswordEdit", passwordBox);
    confirmPassword = passwordEdit("confirmPasswordEdit", passwordBox);
    passwordForm->addRow(tr("C&urrent password:"), currentPassword);
    passwordForm->addRow(tr("Ne&w password:"), newPassword);
    passwordForm->addRow(tr("Confir&m new password:"), confirmPassword);
    auto *changePasswordButton =
        button(tr("Change password"), "changePasswordButton", passwordBox);
    passwordMessage = messageLabel("passwordMessage", passwordBox);
    passwordForm->addRow(QString(), rightAligned(changePasswordButton));
    passwordForm->addRow(QString(), passwordMessage);
    layout->addWidget(passwordBox);

    // ---- Sign-in ----
    auto *signInBox = new QGroupBox(tr("Sign-in"), content);
    auto *signInLayout = new QVBoxLayout(signInBox);
    twoStepSignIn = new QCheckBox(
        tr("Two-step sign-&in: also ask for a code sent to my email each time I sign in"),
        signInBox);
    twoStepSignIn->setObjectName("twoStepSignInCheckBox");
    signInMessage = messageLabel("signInMessage", signInBox);
    auto *signOutButton = button(tr("Sign &out"), "signOutButton", signInBox);
    auto *signOutRow = new QHBoxLayout;
    signOutRow->addStretch();
    signOutRow->addWidget(signOutButton);
    signInLayout->addWidget(twoStepSignIn);
    signInLayout->addWidget(signInMessage);
    signInLayout->addLayout(signOutRow);
    layout->addWidget(signInBox);
    layout->addStretch();

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    connect(saveProfileButton, &QPushButton::clicked, this, &ProfilePage::saveProfile);
    connect(copyButton, &QPushButton::clicked, this, &ProfilePage::copyFriendId);
    connect(correctButton, &QPushButton::clicked, this, &ProfilePage::correctBalance);
    connect(correctDaysButton, &QPushButton::clicked, this, &ProfilePage::correctDays);
    connect(resetDaysButton, &QPushButton::clicked, this, &ProfilePage::resetDays);
    connect(changePasswordButton, &QPushButton::clicked, this, &ProfilePage::changePassword);
    connect(twoStepSignIn, &QCheckBox::toggled, this, &ProfilePage::setTwoStepSignIn);
    connect(signOutButton, &QPushButton::clicked, this, &ProfilePage::signOutRequested);

    refresh();
    loadEditors();

    // Days left drop at midnight while the app is open.
    auto *clock = new QTimer(this);
    connect(clock, &QTimer::timeout, this, [this] {
        if (QDate::currentDate() != shownDate) {
            refresh();
        }
    });
    clock->start(DATE_CHECK_INTERVAL_MS);
}

int ProfilePage::daysLeftFor(const LocalAccounts::Profile &profile, const QDate &today) {
    return Quarter::daysLeft(today, profile.daysCorrected, profile.remainingDays);
}

void ProfilePage::refresh() {
    shownDate = QDate::currentDate();
    LocalAccounts::Profile loaded;
    if (!LocalAccounts::loadProfile(accountName, loaded)) {
        showMessage(profileMessage, tr("Could not load your profile. Try signing in again."),
                    true);
        return;
    }
    profile = loaded;

    friendId->setText(profile.friendId);
    {
        const QSignalBlocker quiet(twoStepSignIn); // Showing the saved choice saves nothing.
        twoStepSignIn->setChecked(profile.twoStepSignIn);
    }
    QString planName = profile.diningPlanId;
    for (const auto &plan : LocalAccounts::diningPlans()) {
        if (plan.id == profile.diningPlanId) {
            planName = plan.name;
        }
    }
    diningPlan->setText(planName);
    balance->setText(formatPoints(profile.balance));
    lastUpdated->setText(profile.lastUpdated.isValid()
                             ? AppStyle::dateTime(profile.lastUpdated)
                             : tr("Not yet"));

    const int left = daysLeftFor(profile, shownDate);
    days->setText(profile.daysCorrected ? tr("%1 (corrected)").arg(dayCount(left))
                                        : dayCount(left));
    const Quarter::Status status = Quarter::statusOn(shownDate);
    const QString range = tr("%1 to %2").arg(formatDate(status.term.start),
                                             formatDate(status.term.end));
    switch (status.phase) {
    case Quarter::Phase::InQuarter:
        quarter->setText(tr("%1: %2").arg(status.term.name, range));
        break;
    case Quarter::Phase::BeforeQuarter:
        quarter->setText(tr("Break. %1 starts %2 (%3, %4).")
                             .arg(status.term.name, formatDate(status.term.start), range,
                                  dayCount(status.term.length())));
        break;
    case Quarter::Phase::CalendarEnded:
        quarter->setText(tr("The quarter calendar has ended and needs updating. "
                            "Use Correct days to enter your days left."));
        break;
    }
    daysCorrection->setValue(left);
    // Reset needs a calendar value to return to.
    resetDaysButton->setEnabled(profile.daysCorrected
                                && status.phase != Quarter::Phase::CalendarEnded);
}

void ProfilePage::loadEditors() {
    displayName->setText(profile.displayName);
    email->setText(profile.email);
}

void ProfilePage::saveProfile() {
    switch (LocalAccounts::updateProfile(accountName, displayName->text(), email->text())) {
    case Result::Success:
        refresh();
        loadEditors();
        showMessage(profileMessage, tr("Profile saved."), false);
        return;
    case Result::InvalidDisplayName:
        showMessage(profileMessage,
                    tr("Enter a display name of 1 to %1 characters.")
                        .arg(LocalAccounts::MaxDisplayNameLength),
                    true);
        return;
    case Result::InvalidEmail:
        showMessage(profileMessage, tr("Enter a valid email address."), true);
        return;
    case Result::EmailTaken:
        showMessage(profileMessage, tr("Another account already uses that email."), true);
        return;
    default:
        showMessage(profileMessage, tr("Could not save your profile. Try again."), true);
        return;
    }
}

void ProfilePage::copyFriendId() {
    QGuiApplication::clipboard()->setText(profile.friendId);
    showMessage(profileMessage, tr("Friend ID copied."), false);
}

void ProfilePage::correctBalance() {
    const auto mode = static_cast<CorrectionMode>(correctionMode->currentData().toInt());
    const double amount = correctionAmount->value();
    double updated = 0.0;
    switch (LocalAccounts::correctBalance(accountName, mode, amount, updated)) {
    case Result::Success:
        refresh();
        correctionAmount->setValue(0.0);
        showMessage(balanceMessage, tr("Balance is now %1.").arg(formatPoints(updated)), false);
        emit balanceUpdated(updated);
        return;
    case Result::InsufficientPoints:
        showMessage(balanceMessage,
                    tr("You can't subtract more than your balance of %1.")
                        .arg(formatPoints(profile.balance)),
                    true);
        return;
    case Result::InvalidAmount:
        showMessage(balanceMessage, tr("Enter an amount greater than 0."), true);
        return;
    default:
        showMessage(balanceMessage, tr("Could not save the balance. Try again."), true);
        return;
    }
}

void ProfilePage::correctDays() {
    const int value = daysCorrection->value();
    if (!LocalAccounts::saveRemainingDays(accountName, value)) {
        showMessage(daysMessage, tr("Could not save the days left. Try again."), true);
        return;
    }
    refresh();
    showMessage(daysMessage,
                tr("Days left set to %1. The count goes down by one each day.").arg(value),
                false);
    emit daysUpdated(value);
}

void ProfilePage::resetDays() {
    if (LocalAccounts::clearDaysCorrection(accountName) != Result::Success) {
        showMessage(daysMessage, tr("Could not reset the days left. Try again."), true);
        return;
    }
    refresh();
    const int left = daysLeftFor(profile, shownDate);
    showMessage(daysMessage, tr("Days left reset to the quarter calendar: %1.").arg(left),
                false);
    emit daysUpdated(left);
}

void ProfilePage::changePassword() {
    if (newPassword->text() != confirmPassword->text()) {
        showMessage(passwordMessage, tr("The new passwords don't match."), true);
        return;
    }
    switch (LocalAccounts::changePassword(accountName, currentPassword->text(),
                                          newPassword->text())) {
    case Result::Success:
        currentPassword->clear();
        newPassword->clear();
        confirmPassword->clear();
        showMessage(passwordMessage, tr("Password changed."), false);
        return;
    case Result::ShortPassword:
        showMessage(passwordMessage,
                    tr("The new password must be at least %1 characters.")
                        .arg(LocalAccounts::MinPasswordLength),
                    true);
        return;
    case Result::IncorrectCredentials:
        showMessage(passwordMessage, tr("Your current password is incorrect."), true);
        return;
    default:
        showMessage(passwordMessage, tr("Could not change your password. Try again."), true);
        return;
    }
}

void ProfilePage::setTwoStepSignIn(bool on) {
    if (LocalAccounts::setTwoStepSignIn(accountName, on) != Result::Success) {
        const QSignalBlocker quiet(twoStepSignIn); // Undo the check without saving again.
        twoStepSignIn->setChecked(!on);
        showMessage(signInMessage, tr("Couldn't change two-step sign-in. Try again."), true);
        return;
    }
    profile.twoStepSignIn = on;
    showMessage(signInMessage,
                on ? tr("Two-step sign-in is on. Each time you sign in, also enter the code "
                        "we email you.")
                   : tr("Two-step sign-in is off. You'll sign in with just your password."),
                false);
}

void ProfilePage::showMessage(QLabel *label, const QString &text, bool error) {
    label->setStyleSheet(error ? AppStyle::errorText(label) : QString());
    label->setText(text);
    label->setVisible(!text.isEmpty());
}

QString ProfilePage::formatPoints(double points) {
    return AppStyle::points(points);
}
