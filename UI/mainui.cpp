#include "mainui.h"
#include "ui_mainui.h"

#include <QEvent>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTimer>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
constexpr int POINT_DECIMAL_PLACES = 2;
constexpr int DATE_CHECK_INTERVAL_MS = 60 * 1000;
}

MainUI::MainUI(QWidget *parent)
    : MainUI(0.0, {}, DefaultRemainingDays, {}, parent) {}

MainUI::MainUI(double startingBalance, std::function<bool(double, double)> saveBalance,
               int startingDays, std::function<bool(int)> saveRemainingDays, QWidget *parent)
    : QMainWindow(parent), balance(startingBalance), budget(balance, DefaultRemainingDays),
      ui(new Ui::MainUI), saveBalance(std::move(saveBalance)),
      saveRemainingDays(std::move(saveRemainingDays)), shownDate(QDate::currentDate()) {
    try {
        budget.setRemainingDays(startingDays);
    } catch (const std::invalid_argument &) {
        // Keep the default if a saved value is out of range.
    }
    ui->setupUi(this);
    ui->amountSpinBox->installEventFilter(this);
    ui->remainingDaysSpinBox->setValue(budget.getRemainingDays());

    connectControls();
    refreshDisplay();

    auto *clock = new QTimer(this);
    connect(clock, &QTimer::timeout, this, [this] { showDate(QDate::currentDate()); });
    clock->start(DATE_CHECK_INTERVAL_MS);
}

MainUI::~MainUI() {
    delete ui;
}

void MainUI::setPurchasePage(QWidget *page) {
    auto *budgetPage = takeCentralWidget();
    auto *pages = new QTabWidget(this);
    pages->addTab(page, tr("&Purchases"));
    pages->addTab(budgetPage, tr("&Budget"));
    setCentralWidget(pages);
    resize(750, 640);
}

void MainUI::showSavedBalance(double points) {
    balance.setBalance(points);
    refreshDisplay();
}

void MainUI::showDate(const QDate &today) {
    if (!today.isValid() || !shownDate.isValid() || today <= shownDate) {
        return;
    }
    const qint64 passed = shownDate.daysTo(today);
    shownDate = today;
    const int days = static_cast<int>(std::max<qint64>(0, budget.getRemainingDays() - passed));
    // The saved count and its date already give this value at the next sign-in,
    // so nothing needs to be saved here.
    budget.setRemainingDays(days);
    const QSignalBlocker blocker(ui->remainingDaysSpinBox);
    ui->remainingDaysSpinBox->setValue(days);
    refreshDisplay();
}

bool MainUI::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->amountSpinBox && event->type() == QEvent::FocusIn
        && ui->amountSpinBox->value() == 0.0) {
        ui->amountSpinBox->clear();
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainUI::connectControls() {
    connect(ui->setBalanceButton, &QPushButton::clicked, this, &MainUI::setBalance);
    connect(ui->addPointsButton, &QPushButton::clicked, this, &MainUI::addPoints);
    connect(ui->spendPointsButton, &QPushButton::clicked, this, &MainUI::spendPoints);
    connect(ui->remainingDaysSpinBox, &QSpinBox::valueChanged,
            this, &MainUI::updateRemainingDays);
    connect(ui->resetDaysButton, &QPushButton::clicked, this, &MainUI::resetRemainingDays);
}

void MainUI::setBalance() {
    if (!validateAmount()) {
        return;
    }
    const double previous = balance.getBalance();
    balance.setBalance(ui->amountSpinBox->value());
    finishBalanceUpdate(previous);
}

void MainUI::addPoints() {
    if (!validateAmount()) {
        return;
    }
    const double previous = balance.getBalance();
    balance.add(ui->amountSpinBox->value());
    finishBalanceUpdate(previous);
}

void MainUI::spendPoints() {
    if (!validateAmount()) {
        return;
    }
    try {
        const double previous = balance.getBalance();
        balance.subtract(ui->amountSpinBox->value());
        finishBalanceUpdate(previous);
    } catch (const std::invalid_argument &) {
        ui->errorLabel->setText(
            tr("Not enough points. Enter an amount no greater than your balance."));
    }
}

bool MainUI::updateRemainingDays(int days) {
    const int previous = budget.getRemainingDays();
    QString error;
    try {
        budget.setRemainingDays(days);
        if (!saveRemainingDays || saveRemainingDays(days)) {
            refreshDisplay();
            return true;
        }
        budget.setRemainingDays(previous);
        error = tr("Could not save the days left. Your previous day count has been kept.");
    } catch (const std::invalid_argument &) {
        error = tr("Remaining days must be between 0 and 90, including today. "
                   "Your previous day count has been kept.");
    }
    // Restore the last valid value without triggering another update.
    const QSignalBlocker blocker(ui->remainingDaysSpinBox);
    ui->remainingDaysSpinBox->setValue(budget.getRemainingDays());
    ui->errorLabel->setText(error);
    return false;
}

void MainUI::resetRemainingDays() {
    {
        // Update once below, even when the box already shows the starting count.
        const QSignalBlocker blocker(ui->remainingDaysSpinBox);
        ui->remainingDaysSpinBox->setValue(DefaultRemainingDays);
    }
    if (updateRemainingDays(DefaultRemainingDays)) {
        ui->errorLabel->setText(tr("Days left reset to %1.").arg(DefaultRemainingDays));
    }
}

bool MainUI::validateAmount() {
    if (ui->amountSpinBox->cleanText().trimmed().isEmpty()) {
        ui->errorLabel->setText(tr("Enter an amount first."));
        return false;
    }
    return true;
}

void MainUI::finishBalanceUpdate(double previousBalance) {
    if (saveBalance && !saveBalance(previousBalance, balance.getBalance())) {
        balance.setBalance(previousBalance);
        ui->errorLabel->setText(tr("Could not save the balance. Try again or sign in again to reload it."));
        return;
    }
    refreshDisplay();
    ui->amountSpinBox->setValue(0.0);
    ui->amountSpinBox->setFocus();
    ui->amountSpinBox->clear();
    ui->errorLabel->setText(tr("Successfully Updated"));
}

void MainUI::refreshDisplay() {
    ui->balanceValueLabel->setText(formatPoints(balance.getBalance()));
    ui->dailyBudgetValueLabel->setText(formatPoints(budget.getDailyBudget()));
    ui->weeklyBudgetValueLabel->setText(formatPoints(budget.getWeeklyBudget()));
    ui->errorLabel->clear();
    emit balanceChanged(balance.getBalance());
}

QString MainUI::formatPoints(double points) {
    return tr("%1 points").arg(points, 0, 'f', POINT_DECIMAL_PLACES);
}
