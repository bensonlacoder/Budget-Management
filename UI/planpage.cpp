#include "planpage.h"

#include "quarter.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <stdexcept>

using LocalAccounts::HistoryEntry;
using LocalAccounts::Result;

namespace {
constexpr int DATE_CHECK_INTERVAL_MS = 60 * 1000;

QLabel *namedLabel(const char *name, QWidget *parent, bool bold = false) {
    auto *label = new QLabel(parent);
    label->setObjectName(QLatin1String(name));
    label->setWordWrap(true);
    if (bold) {
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
    }
    return label;
}

QString dayCount(int days) {
    return days == 1 ? PlanPage::tr("1 day") : PlanPage::tr("%1 days").arg(days);
}
}

PlanPage::PlanPage(const QString &accountName, QWidget *parent)
    : QWidget(parent), accountName(accountName), balance(0.0), budget(balance, 0),
      shownDate(QDate::currentDate()) {
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);

    // ---- Advice ----
    auto *adviceBox = new QGroupBox(tr("Your spending plan"), content);
    auto *adviceForm = new QFormLayout(adviceBox);
    balanceLabel = namedLabel("planBalanceLabel", adviceBox);
    daysLabel = namedLabel("planDaysLabel", adviceBox);
    spendableLabel = namedLabel("spendableLabel", adviceBox);
    dailyLabel = namedLabel("dailyBudgetLabel", adviceBox, true);
    weeklyLabel = namedLabel("weeklyBudgetLabel", adviceBox, true);
    adviceForm->addRow(tr("Points left:"), balanceLabel);
    adviceForm->addRow(tr("Days left (including today):"), daysLabel);
    adviceForm->addRow(tr("Points to spend (after reserve):"), spendableLabel);
    adviceForm->addRow(tr("Spend up to per day:"), dailyLabel);
    adviceForm->addRow(tr("Spend up to over the next 7 days:"), weeklyLabel);
    loadMessage = namedLabel("planLoadMessage", adviceBox);
    adviceForm->addRow(QString(), loadMessage);
    layout->addWidget(adviceBox);

    // ---- Reserve goal ----
    auto *reserveBox = new QGroupBox(tr("Reserve goal"), content);
    auto *reserveForm = new QFormLayout(reserveBox);
    auto *reserveHelp = new QLabel(
        tr("Points to keep at the end of the quarter. The advice above leaves them out."),
        reserveBox);
    reserveHelp->setWordWrap(true);
    reserveForm->addRow(reserveHelp);
    reserveEdit = new QDoubleSpinBox(reserveBox);
    reserveEdit->setObjectName("reserveGoalSpinBox");
    reserveEdit->setDecimals(2);
    reserveEdit->setRange(0.0, LocalAccounts::MaxPurchasePoints);
    reserveEdit->setSuffix(tr(" pts"));
    auto *saveButton = new QPushButton(tr("Save reserve"), reserveBox);
    saveButton->setObjectName("saveReserveButton");
    auto *reserveRow = new QHBoxLayout;
    reserveRow->addWidget(reserveEdit, 1);
    reserveRow->addWidget(saveButton);
    reserveForm->addRow(tr("&Keep in reserve:"), reserveRow);
    reserveMessage = namedLabel("reserveMessage", reserveBox);
    reserveForm->addRow(QString(), reserveMessage);
    layout->addWidget(reserveBox);

    // ---- Projection ----
    auto *projectionBox = new QGroupBox(tr("End of quarter"), content);
    auto *projectionLayout = new QVBoxLayout(projectionBox);
    projectionLabel = namedLabel("projectionLabel", projectionBox);
    reserveWarning = namedLabel("reserveWarningLabel", projectionBox, true);
    reserveWarning->setStyleSheet(QStringLiteral("color: #b00020;"));
    projectionLayout->addWidget(projectionLabel);
    projectionLayout->addWidget(reserveWarning);
    layout->addWidget(projectionBox);
    layout->addStretch();

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    connect(saveButton, &QPushButton::clicked, this, &PlanPage::saveReserve);

    refresh();

    // Days left and the 7-day spending window move on at midnight.
    auto *clock = new QTimer(this);
    connect(clock, &QTimer::timeout, this, [this] {
        if (QDate::currentDate() != shownDate) {
            refresh();
        }
    });
    clock->start(DATE_CHECK_INTERVAL_MS);
}

double PlanPage::recentSpendingRate(const QList<HistoryEntry> &history, const QDate &today) {
    const QDate first = today.addDays(1 - SpendingWindowDays);
    double spent = 0.0;
    for (const HistoryEntry &entry : history) {
        if (entry.date < first || entry.date > today) {
            continue;
        }
        // Corrections that raised the balance aren't income from spending less.
        if (entry.kind == HistoryEntry::Kind::Correction && entry.balanceChange > 0.0) {
            continue;
        }
        spent -= entry.balanceChange;
    }
    return std::max(0.0, spent / SpendingWindowDays);
}

void PlanPage::refresh() {
    shownDate = QDate::currentDate();
    LocalAccounts::Profile profile;
    QList<HistoryEntry> history;
    LocalAccounts::PlanSettings settings;
    if (!LocalAccounts::loadProfile(accountName, profile)
        || !LocalAccounts::loadHistory(accountName, history)
        || (!loaded && !LocalAccounts::loadPlanSettings(accountName, settings))) {
        showMessage(loadMessage, tr("Could not load your plan. Try signing in again."), true);
        return;
    }
    const int days = std::clamp(
        Quarter::daysLeft(shownDate, profile.daysCorrected, profile.remainingDays), 0,
        LocalAccounts::MaxRemainingDays);

    if (!loaded) {
        // The saved reserve may be more than today's balance if points were spent
        // after it was set. Budget only refuses that when the goal is entered, so set
        // the goal against a balance that covers it, then use the real balance.
        balance.setBalance(std::max(profile.balance, settings.reserveGoal));
        budget.setReserveGoal(settings.reserveGoal);
        reserveEdit->setValue(settings.reserveGoal);
        loaded = true;
    }
    balance.setBalance(profile.balance);
    budget.setRemainingDays(days);
    budget.setRecentSpendingRate(recentSpendingRate(history, shownDate));
    loadMessage->clear();
    showAdvice();
}

void PlanPage::saveReserve() {
    if (!loaded) {
        showMessage(reserveMessage, tr("Could not load your plan. Try signing in again."), true);
        return;
    }
    const double previous = budget.getReserveGoal();
    const double goal = reserveEdit->value();
    try {
        budget.setReserveGoal(goal);
    } catch (const std::invalid_argument &) {
        reserveEdit->setValue(previous);
        showMessage(reserveMessage,
                    tr("Your reserve can't be more than your %1. Your previous reserve of "
                       "%2 has been kept.")
                        .arg(formatPoints(balance.getBalance()), formatPoints(previous)),
                    true);
        return;
    }

    // Keep the habits and alert settings saved with the reserve.
    LocalAccounts::PlanSettings settings;
    Result result = LocalAccounts::loadPlanSettings(accountName, settings)
        ? Result::Success
        : Result::StorageError;
    if (result == Result::Success) {
        settings.reserveGoal = budget.getReserveGoal();
        result = LocalAccounts::savePlanSettings(accountName, settings);
    }
    if (result != Result::Success) {
        budget.setReserveGoal(previous);
        reserveEdit->setValue(previous);
        showMessage(reserveMessage, tr("Could not save your reserve. Try again."), true);
        return;
    }
    showMessage(reserveMessage,
                budget.getReserveGoal() > 0.0
                    ? tr("Reserve saved. You'll keep %1 at the end of the quarter.")
                          .arg(formatPoints(budget.getReserveGoal()))
                    : tr("Reserve saved. The advice uses all of your points."),
                false);
    showAdvice();
}

void PlanPage::showAdvice() {
    const int days = budget.getRemainingDays();
    balanceLabel->setText(formatPoints(balance.getBalance()));
    daysLabel->setText(days == 0 ? tr("0 days. The quarter has ended.") : dayCount(days));
    spendableLabel->setText(formatPoints(budget.getSpendablePoints()));
    dailyLabel->setText(formatPoints(budget.getDailyBudget()));
    weeklyLabel->setText(formatPoints(budget.getWeeklyBudget()));

    const double rate = budget.getRecentSpendingRate();
    const double projected = budget.getProjectedEndBalance();
    if (days == 0) {
        projectionLabel->setText(tr("The quarter has ended."));
    } else if (rate < 0.005) { // Less than a hundredth of a point a day.
        projectionLabel->setText(
            tr("You haven't spent any points in the last 7 days. At that rate you'd end "
               "the quarter with all %1.")
                .arg(formatPoints(balance.getBalance())));
    } else if (budget.isProjectedToRunOut()) {
        projectionLabel->setText(
            tr("At your recent spending of %1 a day, you'd run out of points before the "
               "quarter ends.")
                .arg(formatPoints(rate)));
    } else {
        projectionLabel->setText(
            tr("At your recent spending of %1 a day, you'd end the quarter with %2.")
                .arg(formatPoints(rate), formatPoints(projected)));
    }

    const bool warn = days > 0 && budget.getReserveGoal() > 0.0
        && budget.isProjectedBelowReserve();
    reserveWarning->setVisible(warn);
    reserveWarning->setText(
        warn ? tr("That's less than your reserve goal of %1. Spend up to %2 a day to keep it.")
                   .arg(formatPoints(budget.getReserveGoal()),
                        formatPoints(budget.getDailyBudget()))
             : QString());
}

void PlanPage::showMessage(QLabel *label, const QString &text, bool error) {
    label->setStyleSheet(error ? QStringLiteral("color: #b00020;") : QString());
    label->setText(text);
}

QString PlanPage::formatPoints(double points) {
    return tr("%1 points").arg(points, 0, 'f', 2);
}
