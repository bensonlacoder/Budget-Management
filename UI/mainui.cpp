#include "mainui.h"
#include "appstyle.h"
#include "ui_mainui.h"

#include "quarter.h"

#include <QEvent>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTimer>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
constexpr int DATE_CHECK_INTERVAL_MS = 60 * 1000;
constexpr int MAX_REMAINING_DAYS = 90; // Budget's upper limit.

std::optional<MainUI::RealNumbers> noSavedNumbers() {
    return MainUI::RealNumbers{0.0, Quarter::daysLeft(QDate::currentDate(), false, 0)};
}
}

MainUI::MainUI(QWidget *parent) : MainUI(RealNumbersLoader(noSavedNumbers), parent) {}

MainUI::MainUI(RealNumbersLoader loadRealNumbers, QWidget *parent)
    : QMainWindow(parent), budget(balance, 0), ui(new Ui::MainUI),
      loadRealNumbers(loadRealNumbers ? std::move(loadRealNumbers) : noSavedNumbers),
      shownDate(QDate::currentDate()) {
    ui->setupUi(this);
    // A tinted notice, so the sandbox is never mistaken for the real numbers.
    ui->sandboxLabel->setStyleSheet(AppStyle::noticeBanner(this));
    AppStyle::setTextRole(ui->titleLabel, "headline");
    AppStyle::setTextRole(ui->balanceValueLabel, "balance");
    ui->amountSpinBox->installEventFilter(this);
    budgetPage = centralWidget();

    connectControls();
    useRealNumbers();

    auto *clock = new QTimer(this);
    connect(clock, &QTimer::timeout, this, [this] { showDate(QDate::currentDate()); });
    clock->start(DATE_CHECK_INTERVAL_MS);
}

MainUI::~MainUI() {
    delete ui;
}

void MainUI::setPurchasePage(QWidget *page) {
    takeCentralWidget();
    pages = new QTabWidget(this);
    pages->addTab(page, tr("&Purchases"));
    pages->addTab(budgetPage, tr("&Budget"));
    // The sandbox starts over from the real numbers each time the tab is opened.
    connect(pages, &QTabWidget::currentChanged, this, [this](int index) {
        if (pages->widget(index) == budgetPage) {
            useRealNumbers();
        }
    });
    setCentralWidget(pages);
    resize(750, 640);
}

void MainUI::addPage(QWidget *page, const QString &title) {
    if (pages) {
        pages->addTab(page, title);
    }
}

bool MainUI::useRealNumbers() {
    const std::optional<RealNumbers> real = loadRealNumbers();
    if (!real) {
        ui->errorLabel->setText(tr("Could not load your real numbers. Try again."));
        return false;
    }
    try {
        balance.setBalance(real->balance);
        budget.setRemainingDays(std::clamp(real->days, 0, MAX_REMAINING_DAYS));
    } catch (const std::invalid_argument &) {
        ui->errorLabel->setText(tr("Could not load your real numbers. Try again."));
        return false;
    }
    shownDate = QDate::currentDate();
    {
        const QSignalBlocker blocker(ui->remainingDaysSpinBox);
        ui->remainingDaysSpinBox->setValue(budget.getRemainingDays());
    }
    ui->amountSpinBox->setValue(0.0);
    refreshDisplay();
    return true;
}

void MainUI::showDate(const QDate &today) {
    if (!today.isValid() || !shownDate.isValid() || today <= shownDate) {
        return;
    }
    const qint64 passed = shownDate.daysTo(today);
    shownDate = today;
    const int days = static_cast<int>(std::max<qint64>(0, budget.getRemainingDays() - passed));
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
    connect(ui->useRealNumbersButton, &QPushButton::clicked, this, [this] {
        if (useRealNumbers()) {
            ui->errorLabel->setText(tr("Showing your real balance and days left."));
        }
    });
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

bool MainUI::updateRemainingDays(int days) {
    try {
        budget.setRemainingDays(days);
        refreshDisplay();
        return true;
    } catch (const std::invalid_argument &) {
    }
    // Restore the last valid value without triggering another update.
    const QSignalBlocker blocker(ui->remainingDaysSpinBox);
    ui->remainingDaysSpinBox->setValue(budget.getRemainingDays());
    ui->errorLabel->setText(tr("Remaining days must be between 0 and 90, including today. "
                               "Your previous day count has been kept."));
    return false;
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
    return AppStyle::points(points);
}
