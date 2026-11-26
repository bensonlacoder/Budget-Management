#include "chartspage.h"
#include "historylocations.h"

#include "budget.h"
#include "quarter.h"

#include <QComboBox>
#include <QDateEdit>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QShowEvent>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {
QDate periodStart(const QDate &day, bool weekly) {
    return weekly ? day.addDays(1 - day.dayOfWeek()) : day;
}

qint64 cents(double points) {
    return std::llround(points * 100);
}

void setRow(QTableWidget *table, int row, const QStringList &values) {
    for (int column = 0; column < values.size(); ++column) {
        auto *item = new QTableWidgetItem(values.at(column));
        if (column > 0) {
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
        table->setItem(row, column, item);
    }
}
}

class SpendingChart : public QWidget {
public:
    explicit SpendingChart(QWidget *parent) : QWidget(parent) {
        setMinimumHeight(145);
        setAccessibleName(tr("Net spending and budget by period"));
    }

    void setData(const QList<qint64> &values, const QList<qint64> &targets) {
        actual = values;
        budget = targets;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        if (actual.isEmpty()) {
            return;
        }
        const auto bounds = std::minmax_element(actual.cbegin(), actual.cend());
        const qint64 bottom = std::min<qint64>(0, *bounds.first);
        const qint64 top = std::max({qint64(1), *std::max_element(budget.cbegin(), budget.cend()), *bounds.second});
        QPainter painter(this);
        const QString high = locale().toString(top / 100.0, 'f', 2);
        const QString low = locale().toString(bottom / 100.0, 'f', 2);
        const int labelWidth = qMax(fontMetrics().horizontalAdvance(high),
                                   fontMetrics().horizontalAdvance(low));
        const QRectF plot = QRectF(rect()).adjusted(labelWidth + 12, 12, -12, -25);
        const auto y = [&](qint64 value) {
            return plot.bottom() - plot.height() * (value - bottom) / (top - bottom);
        };
        const auto axis = palette().color(QPalette::Text);
        painter.setPen(axis);
        painter.drawText(QPointF(0, plot.top() + 5), high);
        painter.drawText(QPointF(0, plot.bottom() + 5), low);
        painter.drawLine(QPointF(plot.left(), y(0)), QPointF(plot.right(), y(0)));
        const double step = plot.width() / actual.size();
        const double barWidth = std::min(26.0, step * 0.8);
        for (int i = 0; i < actual.size(); ++i) {
            const double end = y(actual.at(i));
            painter.fillRect(QRectF(plot.left() + (i + 0.5) * step - barWidth / 2,
                                  std::min(y(0), end), barWidth, std::abs(y(0) - end)),
                             palette().brush(QPalette::Highlight));
        }
        painter.setPen(QPen(axis, 2, Qt::DashLine));
        QPolygonF reference;
        for (int i = 0; i < budget.size(); ++i) {
            const double level = y(budget.at(i));
            reference << QPointF(plot.left() + i * step, level)
                      << QPointF(plot.left() + (i + 1) * step, level);
        }
        painter.drawPolyline(reference);
        painter.setPen(axis);
        painter.drawText(QRectF(0, plot.bottom() + 7, width(), 18), Qt::AlignCenter,
                         tr("Bars: actual net points   ·   Dashed line: budget"));
    }

private:
    QList<qint64> actual;
    QList<qint64> budget;
};

ChartsPage::ChartsPage(const QString &accountName, QWidget *parent)
    : QWidget(parent), accountName(accountName),
      from(new QDateEdit(QDate::currentDate().addDays(-27), this)),
      to(new QDateEdit(QDate::currentDate(), this)), period(new QComboBox(this)),
      status(new QLabel(this)), locations(new QTableWidget(this)),
      comparison(new QTableWidget(this)), chart(new SpendingChart(this)) {
    from->setObjectName("chartsFrom");
    to->setObjectName("chartsTo");
    from->setCalendarPopup(true);
    to->setCalendarPopup(true);
    // Budgets are only known up to today.
    from->setMaximumDate(QDate::currentDate());
    to->setMaximumDate(QDate::currentDate());
    period->setObjectName("chartsPeriod");
    period->addItems({tr("Daily"), tr("Weekly")});

    auto *filter = new QHBoxLayout;
    auto *fromLabel = new QLabel(tr("&From"), this);
    fromLabel->setBuddy(from);
    auto *toLabel = new QLabel(tr("&Through"), this);
    toLabel->setBuddy(to);
    filter->addWidget(fromLabel);
    filter->addWidget(from);
    filter->addWidget(toLabel);
    filter->addWidget(to);
    filter->addWidget(period);
    filter->addStretch();

    locations->setObjectName("locationTotals");
    locations->setColumnCount(2);
    locations->setHorizontalHeaderLabels({tr("Location"), tr("Net points")});
    comparison->setObjectName("budgetComparison");
    comparison->setColumnCount(4);
    comparison->setHorizontalHeaderLabels({tr("Period"), tr("Actual"), tr("Budget"), tr("Difference")});
    for (auto *table : {locations, comparison}) {
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->hide();
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
    status->setObjectName("chartsStatus");
    status->setWordWrap(true);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(filter);
    layout->addWidget(status);
    layout->addWidget(chart);
    layout->addWidget(comparison, 2);
    layout->addWidget(locations, 1);

    connect(from, &QDateEdit::dateChanged, this, &ChartsPage::refresh);
    connect(to, &QDateEdit::dateChanged, this, &ChartsPage::refresh);
    connect(period, &QComboBox::currentIndexChanged, this, &ChartsPage::refresh);
    refresh();
}

void ChartsPage::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}

ChartsPage::Spending ChartsPage::summarize(const QList<LocalAccounts::HistoryEntry> &history,
                                          const QDate &from, const QDate &to, bool weekly) {
    using Kind = LocalAccounts::HistoryEntry::Kind;
    Spending spending;
    for (const auto &entry : history) {
        if (entry.date < from || entry.date > to
            || (entry.kind == Kind::Correction && entry.balanceChange >= 0)) {
            continue;
        }
        QString venue = entry.location;
        if (entry.kind == Kind::Correction) {
            venue = tr("Unlogged");
        } else if (entry.kind == Kind::Refund && venue.isEmpty()) {
            venue = tr("Standalone refunds");
        }
        const auto net = -cents(entry.balanceChange);
        spending.locations[venue] += net;
        spending.periods[periodStart(entry.date, weekly)] += net;
    }
    return spending;
}

QMap<QDate, qint64> ChartsPage::dailyBudgets(const QList<LocalAccounts::HistoryEntry> &history,
                                             double balanceToday, double reserveGoal,
                                             int daysLeftToday, const QDate &today,
                                             const QDate &from, const QDate &to) {
    QMap<QDate, qint64> budgets;
    for (QDate day = from; day <= to; day = day.addDays(1)) {
        const QDate asOf = std::min(day, today);
        // Undo every change made on or after the day to get its starting balance.
        qint64 startCents = cents(balanceToday);
        for (const auto &entry : history) {
            if (entry.date >= asOf) {
                startCents -= cents(entry.balanceChange);
            }
        }
        const int daysLeft = std::clamp(
            daysLeftToday + static_cast<int>(asOf.daysTo(today)), 0,
            LocalAccounts::MaxRemainingDays);
        const double start = std::max<qint64>(0, startCents) / 100.0;
        // Set the reserve against a balance that covers it, as the Plan tab does.
        Balance balance(std::max(start, reserveGoal));
        Budget budget(balance, daysLeft);
        budget.setReserveGoal(reserveGoal);
        balance.setBalance(start);
        budgets.insert(day, cents(budget.getDailyBudget()));
    }
    return budgets;
}

void ChartsPage::refresh() {
    locations->setRowCount(0);
    comparison->setRowCount(0);
    chart->setData({}, {});
    chart->hide();
    locations->hide();
    comparison->hide();
    status->show();
    const QDate start = from->date();
    const QDate end = to->date();
    if (start > end) {
        status->setText(tr("Choose a start date on or before the end date."));
        return;
    }

    QList<LocalAccounts::HistoryEntry> history;
    LocalAccounts::Profile profile;
    LocalAccounts::PlanSettings settings;
    if (!LocalAccounts::loadHistory(accountName, history)
        || !LocalAccounts::loadProfile(accountName, profile)
        || !LocalAccounts::loadPlanSettings(accountName, settings)) {
        status->setText(tr("Could not load spending or budget data."));
        return;
    }
    resolveRefundLocations(history);
    const bool weekly = period->currentIndex() == 1;
    const auto spending = summarize(history, start, end, weekly);
    if (spending.periods.isEmpty()) {
        status->setText(tr("No spending or refunds in this date range."));
        return;
    }

    const QDate today = QDate::currentDate();
    const int days = std::clamp(
        Quarter::daysLeft(today, profile.daysCorrected, profile.remainingDays), 0,
        LocalAccounts::MaxRemainingDays);
    const auto budgets = dailyBudgets(history, profile.balance, settings.reserveGoal, days,
                                      today, start, end);
    const auto format = [this](qint64 value) { return locale().toString(value / 100.0, 'f', 2); };

    // Biggest net spending first; ties stay in name order.
    QList<std::pair<QString, qint64>> byPoints;
    for (auto it = spending.locations.cbegin(); it != spending.locations.cend(); ++it) {
        byPoints.append({it.key(), it.value()});
    }
    std::stable_sort(byPoints.begin(), byPoints.end(),
                     [](const auto &left, const auto &right) { return left.second > right.second; });
    locations->setRowCount(byPoints.size());
    int row = 0;
    for (const auto &[venue, points] : byPoints) {
        setRow(locations, row++, {venue, format(points)});
    }

    QList<qint64> actual;
    QList<qint64> targets;
    const QDate firstPeriod = periodStart(start, weekly);
    const int step = weekly ? 7 : 1;
    comparison->setRowCount(firstPeriod.daysTo(end) / step + 1);
    for (int i = 0; i < comparison->rowCount(); ++i) {
        const QDate date = firstPeriod.addDays(qint64(i) * step);
        // A week's budget is the sum of its days' budgets inside the range.
        qint64 target = 0;
        for (QDate day = std::max(start, date); day <= std::min(end, date.addDays(step - 1));
             day = day.addDays(1)) {
            target += budgets.value(day);
        }
        const qint64 total = spending.periods.value(date);
        const qint64 difference = total - target;
        const QString label = weekly
            ? tr("Week of %1").arg(locale().toString(date, QLocale::ShortFormat))
            : locale().toString(date, QLocale::ShortFormat);
        const QString delta = difference == 0 ? tr("On budget")
            : difference > 0 ? tr("%1 over").arg(format(difference))
                             : tr("%1 under").arg(format(-difference));
        setRow(comparison, i, {label, format(total), format(target), delta});
        actual.append(total);
        targets.append(target);
    }
    chart->setData(actual, targets);
    chart->show();
    comparison->show();
    locations->show();
    status->setText(weekly
        ? tr("Each week's budget adds up the daily budgets you had on its days. Weeks run "
             "Monday to Sunday.")
        : tr("Each day is compared with the daily budget you had at the start of that day."));
}
