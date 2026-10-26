#include "mainui.h"
#include "ui_mainui.h"

#include <QEvent>

#include <stdexcept>

namespace {
constexpr int DEFAULT_REMAINING_DAYS = 70;
constexpr int POINT_DECIMAL_PLACES = 2;
}

MainUI::MainUI(QWidget *parent)
    : QMainWindow(parent), balance(), budget(balance, DEFAULT_REMAINING_DAYS),
      ui(new Ui::MainUI) {
    ui->setupUi(this);
    ui->amountSpinBox->installEventFilter(this);
    ui->remainingDaysSpinBox->setValue(budget.getRemainingDays());

    connectControls();
    refreshDisplay();
}

MainUI::~MainUI() {
    delete ui;
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
}

void MainUI::setBalance() {
    if (!validateAmount()) {
        return;
    }
    balance.setBalance(ui->amountSpinBox->value());
    finishBalanceUpdate();
}

void MainUI::addPoints() {
    if (!validateAmount()) {
        return;
    }
    balance.add(ui->amountSpinBox->value());
    finishBalanceUpdate();
}

void MainUI::spendPoints() {
    if (!validateAmount()) {
        return;
    }
    try {
        balance.subtract(ui->amountSpinBox->value());
        finishBalanceUpdate();
    } catch (const std::invalid_argument &) {
        ui->errorLabel->setText(
            tr("Not enough points. Enter an amount no greater than your balance."));
    }
}

void MainUI::updateRemainingDays(int days) {
    budget.setRemainingDays(days);
    refreshDisplay();
}

bool MainUI::validateAmount() {
    if (ui->amountSpinBox->cleanText().trimmed().isEmpty()) {
        ui->errorLabel->setText(tr("Enter an amount first."));
        return false;
    }
    return true;
}

void MainUI::finishBalanceUpdate() {
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
}

QString MainUI::formatPoints(double points) {
    return tr("%1 points").arg(points, 0, 'f', POINT_DECIMAL_PLACES);
}
