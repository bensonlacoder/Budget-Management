#include "planpage.h"
#include "appstyle.h"

#include "habitscsv.h"
#include "quarter.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <stdexcept>

using LocalAccounts::HistoryEntry;
using LocalAccounts::Result;

namespace {
constexpr int DATE_CHECK_INTERVAL_MS = 60 * 1000;
constexpr int DAYS_PER_WEEK = 7;

QLabel *namedLabel(const char *name, QWidget *parent, bool bold = false) {
    auto *label = new QLabel(parent);
    label->setObjectName(QLatin1String(name));
    label->setWordWrap(true);
    if (bold) {
        AppStyle::setTextRole(label, "strong");
    }
    return label;
}

constexpr int GROUP_SPACING = 10;

// A caption above a figure.
QLabel *captionLabel(const QString &text, QWidget *parent) {
    return new QLabel(text, parent);
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

    alertsBanner = namedLabel("alertsBanner", content, true);
    alertsBanner->setStyleSheet(AppStyle::alertBanner(this));
    alertsBanner->hide();
    layout->addWidget(alertsBanner);

    // ---- Advice: today's figure first, then the rest ----
    auto *adviceBox = new QGroupBox(tr("Your spending plan"), content);
    auto *adviceLayout = new QVBoxLayout(adviceBox);
    dailyLabel = namedLabel("dailyBudgetLabel", adviceBox);
    AppStyle::setTextRole(dailyLabel, "headline");
    dailyLabel->setWordWrap(false);
    auto *headline = new QHBoxLayout;
    headline->addWidget(new QLabel(tr("Spend up to"), adviceBox), 0, Qt::AlignBaseline);
    headline->addWidget(dailyLabel, 0, Qt::AlignBaseline);
    headline->addWidget(new QLabel(tr("per day"), adviceBox), 0, Qt::AlignBaseline);
    headline->addStretch();
    adviceLayout->addLayout(headline);
    adviceLayout->addSpacing(GROUP_SPACING);

    // Other limits, each a caption over its figure.
    weeklyLabel = namedLabel("weeklyBudgetLabel", adviceBox, true);
    perMealLabel = namedLabel("perMealLabel", adviceBox, true);
    perCampusDayLabel = namedLabel("perCampusDayLabel", adviceBox, true);
    auto *limits = new QGridLayout;
    limits->setHorizontalSpacing(24);
    const std::pair<QString, QLabel *> limitCells[] = {
        {tr("Next 7 days"), weeklyLabel},
        {tr("Per meal"), perMealLabel},
        {tr("Per campus day"), perCampusDayLabel},
    };
    int column = 0;
    for (const auto &[caption, value] : limitCells) {
        AppStyle::setTextRole(value, "figure");
        limits->addWidget(captionLabel(caption, adviceBox), 0, column);
        limits->addWidget(value, 1, column);
        limits->setColumnStretch(column, 1);
        ++column;
    }
    adviceLayout->addLayout(limits);
    campusLabel = namedLabel("campusDaysLabel", adviceBox);
    adviceLayout->addWidget(campusLabel);
    adviceLayout->addSpacing(GROUP_SPACING);

    // Where the figures come from.
    balanceLabel = namedLabel("planBalanceLabel", adviceBox);
    daysLabel = namedLabel("planDaysLabel", adviceBox);
    spendableLabel = namedLabel("spendableLabel", adviceBox);
    auto *sources = new QFormLayout;
    sources->addRow(tr("Points left:"), balanceLabel);
    sources->addRow(tr("Days left (including today):"), daysLabel);
    sources->addRow(tr("Points to spend after your reserve:"), spendableLabel);
    adviceLayout->addLayout(sources);
    loadMessage = namedLabel("planLoadMessage", adviceBox);
    loadMessage->hide();
    adviceLayout->addWidget(loadMessage);
    layout->addWidget(adviceBox);

    // ---- Projection, right under the plan it checks ----
    auto *projectionBox = new QGroupBox(tr("End of quarter"), content);
    auto *projectionLayout = new QVBoxLayout(projectionBox);
    projectionLabel = namedLabel("projectionLabel", projectionBox);
    reserveWarning = namedLabel("reserveWarningLabel", projectionBox, true);
    reserveWarning->setStyleSheet(AppStyle::errorText(this));
    projectionLayout->addWidget(projectionLabel);
    projectionLayout->addWidget(reserveWarning);
    layout->addWidget(projectionBox);

    // ---- Reserve goal ----
    auto *reserveBox = new QGroupBox(tr("Reserve goal"), content);
    auto *reserveForm = new QFormLayout(reserveBox);
    auto *reserveHelp = new QLabel(
        tr("Points to keep at the end of the quarter. The plan above leaves them out."),
        reserveBox);
    reserveHelp->setWordWrap(true);
    reserveForm->addRow(reserveHelp);
    reserveEdit = new QDoubleSpinBox(reserveBox);
    reserveEdit->setObjectName("reserveGoalSpinBox");
    reserveEdit->setDecimals(2);
    reserveEdit->setRange(0.0, LocalAccounts::MaxPurchasePoints);
    reserveEdit->setSuffix(tr(" points"));
    auto *saveButton = new QPushButton(tr("Save reserve"), reserveBox);
    saveButton->setObjectName("saveReserveButton");
    auto *reserveRow = new QHBoxLayout;
    reserveRow->addWidget(reserveEdit, 1);
    reserveRow->addWidget(saveButton);
    // A row whose field is a layout gets no buddy from addRow, so the label shows a
    // literal "&" unless it is given one.
    auto *reserveLabel = new QLabel(tr("&Keep in reserve:"), reserveBox);
    reserveLabel->setBuddy(reserveEdit);
    reserveForm->addRow(reserveLabel, reserveRow);
    reserveMessage = namedLabel("reserveMessage", reserveBox);
    reserveMessage->hide();
    reserveForm->addRow(reserveMessage);
    layout->addWidget(reserveBox);

    // ---- Dining habits: one row for the week ----
    auto *habitsBox = new QGroupBox(tr("Dining habits"), content);
    auto *habitsLayout = new QVBoxLayout(habitsBox);
    auto *habitsHelp = new QLabel(
        tr("Meals you usually eat on campus each day. Enter 0 for days you're not on "
           "campus. Changes are saved right away."),
        habitsBox);
    habitsHelp->setWordWrap(true);
    habitsLayout->addWidget(habitsHelp);
    const QStringList dayNames = {tr("Mo&n"), tr("Tu&e"), tr("&Wed"), tr("T&hu"),
                                  tr("Fr&i"), tr("&Sat"), tr("S&un")};
    const QStringList fullDayNames = {tr("Monday"), tr("Tuesday"), tr("Wednesday"),
                                      tr("Thursday"), tr("Friday"), tr("Saturday"),
                                      tr("Sunday")};
    static const char *const editNames[] = {"mealsMonday", "mealsTuesday", "mealsWednesday",
                                            "mealsThursday", "mealsFriday", "mealsSaturday",
                                            "mealsSunday"};
    auto *week = new QGridLayout;
    for (int day = 0; day < DAYS_PER_WEEK; ++day) {
        auto *edit = new QSpinBox(habitsBox);
        edit->setObjectName(QLatin1String(editNames[day]));
        edit->setRange(0, LocalAccounts::MaxMealsPerDay);
        edit->setAccessibleName(tr("Meals on campus on %1").arg(fullDayNames.at(day)));
        mealEdits[static_cast<size_t>(day)] = edit;
        auto *dayLabel = new QLabel(dayNames.at(day), habitsBox);
        dayLabel->setBuddy(edit);
        week->addWidget(dayLabel, 0, day);
        week->addWidget(edit, 1, day);
        week->setColumnStretch(day, 1);
        connect(edit, &QSpinBox::valueChanged, this, &PlanPage::habitsEdited);
    }
    habitsLayout->addLayout(week);
    auto *uploadButton = new QPushButton(tr("Upload CS&V..."), habitsBox);
    uploadButton->setObjectName("uploadHabitsButton");
    uploadButton->setToolTip(tr("A file with one line per day, like \"Monday,2\"."));
    habitsMessage = namedLabel("habitsMessage", habitsBox);
    habitsMessage->hide();
    auto *habitsFooter = new QHBoxLayout;
    habitsFooter->addWidget(habitsMessage, 1);
    habitsFooter->addWidget(uploadButton, 0, Qt::AlignRight | Qt::AlignTop);
    habitsLayout->addLayout(habitsFooter);
    layout->addWidget(habitsBox);

    // ---- Alerts ----
    auto *alertsBox = new QGroupBox(tr("Alerts"), content);
    auto *alertsLayout = new QVBoxLayout(alertsBox);
    overDailyAlert = new QCheckBox(tr("Alert me when I spend more than to&day's budget"),
                                   alertsBox);
    overDailyAlert->setObjectName("overDailyAlertCheckBox");
    runOutAlert = new QCheckBox(
        tr("Alert me when I'm on track to run &out of points before the quarter ends"),
        alertsBox);
    runOutAlert->setObjectName("runOutAlertCheckBox");
    alertsMessage = namedLabel("alertsMessage", alertsBox);
    alertsMessage->hide();
    alertsLayout->addWidget(overDailyAlert);
    alertsLayout->addWidget(runOutAlert);
    alertsLayout->addWidget(alertsMessage);
    layout->addWidget(alertsBox);
    layout->addStretch();

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    connect(saveButton, &QPushButton::clicked, this, &PlanPage::saveReserve);
    connect(uploadButton, &QPushButton::clicked, this, &PlanPage::chooseHabitsFile);
    connect(overDailyAlert, &QCheckBox::toggled, this, &PlanPage::saveAlerts);
    connect(runOutAlert, &QCheckBox::toggled, this, &PlanPage::saveAlerts);

    refresh();
    // Raise any alert that already applies once the caller has had a chance to connect.
    QTimer::singleShot(0, this, [this] {
        raiseAlerts = true;
        checkAlerts();
    });

    // Days left and the 7-day spending window move on at midnight.
    auto *clock = new QTimer(this);
    connect(clock, &QTimer::timeout, this, [this] {
        if (QDate::currentDate() != shownDate) {
            refresh();
        }
    });
    clock->start(DATE_CHECK_INTERVAL_MS);
}

double PlanPage::netSpending(const QList<HistoryEntry> &history, const QDate &first,
                             const QDate &last) {
    double spent = 0.0;
    for (const HistoryEntry &entry : history) {
        if (entry.date < first || entry.date > last) {
            continue;
        }
        // Corrections that raised the balance aren't income from spending less.
        if (entry.kind == HistoryEntry::Kind::Correction && entry.balanceChange > 0.0) {
            continue;
        }
        spent -= entry.balanceChange;
    }
    return spent;
}

double PlanPage::recentSpendingRate(const QList<HistoryEntry> &history, const QDate &today) {
    const double spent = netSpending(history, today.addDays(1 - SpendingWindowDays), today);
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
        budget.setDiningHabits(settings.habits);
        showHabits(settings.habits);
        const QSignalBlocker quietOverDaily(overDailyAlert);
        const QSignalBlocker quietRunOut(runOutAlert);
        overDailyAlert->setChecked(settings.alertOverDailyBudget);
        runOutAlert->setChecked(settings.alertRunOutBeforeQuarterEnd);
        loaded = true;
    }
    balance.setBalance(profile.balance);
    budget.setRemainingDays(days);
    budget.setRecentSpendingRate(recentSpendingRate(history, shownDate));
    spentToday = netSpending(history, shownDate, shownDate);
    loadMessage->clear();
    loadMessage->hide();
    showAdvice();
}

void PlanPage::saveAlerts() {
    const bool overDaily = overDailyAlert->isChecked();
    const bool runOut = runOutAlert->isChecked();
    if (!loaded || !saveSettings([overDaily, runOut](LocalAccounts::PlanSettings &settings) {
            settings.alertOverDailyBudget = overDaily;
            settings.alertRunOutBeforeQuarterEnd = runOut;
        })) {
        // Show the saved choices again.
        LocalAccounts::PlanSettings saved;
        LocalAccounts::loadPlanSettings(accountName, saved);
        const QSignalBlocker quietOverDaily(overDailyAlert);
        const QSignalBlocker quietRunOut(runOutAlert);
        overDailyAlert->setChecked(saved.alertOverDailyBudget);
        runOutAlert->setChecked(saved.alertRunOutBeforeQuarterEnd);
        showMessage(alertsMessage, tr("Could not save your alert choices. Try again."), true);
        return;
    }
    showMessage(alertsMessage, tr("Alert choices saved."), false);
    checkAlerts();
}

void PlanPage::checkAlerts() {
    QStringList active;
    const auto raise = [this, &active](const QString &message, QDate &raisedOn) {
        active << message;
        if (raiseAlerts && raisedOn != shownDate) {
            raisedOn = shownDate;
            emit alertRaised(message);
        }
    };
    if (loaded && overDailyAlert->isChecked() && budget.getRemainingDays() > 0
        && budget.isOverTodaysBudget(spentToday)) {
        raise(tr("You've spent %1 today, more than today's budget of %2.")
                  .arg(formatPoints(spentToday),
                       formatPoints(budget.getTodaysBudget(spentToday))),
              overDailyRaisedOn);
    }
    if (loaded && runOutAlert->isChecked() && budget.getRemainingDays() > 0
        && budget.isProjectedToRunOut()) {
        raise(tr("At your recent spending of %1 a day, you'll run out of points before the "
                 "quarter ends.")
                  .arg(formatPoints(budget.getRecentSpendingRate())),
              runOutRaisedOn);
    }
    alertsBanner->setText(active.join(QLatin1Char('\n')));
    alertsBanner->setVisible(!active.isEmpty());
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

    if (!saveSettings([this](LocalAccounts::PlanSettings &settings) {
            settings.reserveGoal = budget.getReserveGoal();
        })) {
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

bool PlanPage::saveSettings(const std::function<void(LocalAccounts::PlanSettings &)> &change) {
    // The reserve, habits and alerts are saved together; keep the ones not changing.
    LocalAccounts::PlanSettings settings;
    if (!LocalAccounts::loadPlanSettings(accountName, settings)) {
        return false;
    }
    change(settings);
    return LocalAccounts::savePlanSettings(accountName, settings) == Result::Success;
}

void PlanPage::habitsEdited() {
    Budget::DiningHabits habits = {};
    for (int day = 0; day < DAYS_PER_WEEK; ++day) {
        habits[static_cast<size_t>(day)] = mealEdits[static_cast<size_t>(day)]->value();
    }
    applyHabits(habits, tr("Habits saved."));
}

bool PlanPage::applyHabits(const Budget::DiningHabits &habits, const QString &savedMessage) {
    if (!loaded) {
        showMessage(habitsMessage, tr("Could not load your plan. Try signing in again."), true);
        showHabits(budget.getDiningHabits());
        return false;
    }
    const Budget::DiningHabits previous = budget.getDiningHabits();
    budget.setDiningHabits(habits); // The spin boxes and the CSV reader keep them in range.
    if (!saveSettings([&habits](LocalAccounts::PlanSettings &settings) {
            settings.habits = habits;
        })) {
        budget.setDiningHabits(previous);
        showHabits(previous);
        showMessage(habitsMessage, tr("Could not save your habits. Try again."), true);
        return false;
    }
    showHabits(habits);
    showMessage(habitsMessage, savedMessage, false);
    showAdvice();
    return true;
}

void PlanPage::showHabits(const Budget::DiningHabits &habits) {
    for (int day = 0; day < DAYS_PER_WEEK; ++day) {
        QSpinBox *edit = mealEdits[static_cast<size_t>(day)];
        const QSignalBlocker quiet(edit);
        edit->setValue(habits[static_cast<size_t>(day)]);
    }
}

void PlanPage::chooseHabitsFile() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Upload dining habits"), QString(), tr("CSV files (*.csv);;All files (*)"));
    if (!path.isEmpty()) {
        loadHabitsFile(path);
    }
}

bool PlanPage::loadHabitsFile(const QString &path) {
    const QString name = QFileInfo(path).fileName();
    const auto fail = [this, &name](const QString &reason) {
        showMessage(habitsMessage,
                    tr("Couldn't upload %1. %2 Your habits haven't changed.").arg(name, reason),
                    true);
        return false;
    };
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return fail(tr("The file couldn't be opened."));
    }
    if (file.size() > HabitsCsv::MaxFileBytes) {
        return fail(tr("The file is too large for a list of days."));
    }
    Budget::DiningHabits habits = {};
    QString error;
    if (!HabitsCsv::parse(QString::fromUtf8(file.readAll()), habits, error)) {
        return fail(error);
    }
    return applyHabits(habits, tr("Uploaded your habits from %1.").arg(name));
}

void PlanPage::showSuggestions() {
    const Budget::DiningHabits &habits = budget.getDiningHabits();
    const bool hasHabits = std::any_of(habits.cbegin(), habits.cend(),
                                       [](int meals) { return meals > 0; });
    const int weekday = shownDate.dayOfWeek();
    const int campusDays = budget.getRemainingCampusDays(weekday);
    const int meals = budget.getRemainingCampusMeals(weekday);
    if (!hasHabits || campusDays == 0) {
        perMealLabel->setText(tr("None"));
        perCampusDayLabel->setText(tr("None"));
        campusLabel->setText(hasHabits
                                 ? tr("None of your campus days are left this quarter.")
                                 : tr("Enter your dining habits below to see how much to "
                                      "spend per meal."));
        return;
    }
    perMealLabel->setText(formatPoints(budget.getPerMealBudget(weekday)));
    perCampusDayLabel->setText(formatPoints(budget.getPerCampusDayBudget(weekday)));
    const QString dayText =
        campusDays == 1 ? tr("1 campus day") : tr("%1 campus days").arg(campusDays);
    const QString mealText = meals == 1 ? tr("1 meal") : tr("%1 meals").arg(meals);
    campusLabel->setText(tr("%1 and %2 left this quarter.").arg(dayText, mealText));
}

void PlanPage::showAdvice() {
    const int days = budget.getRemainingDays();
    balanceLabel->setText(formatPoints(balance.getBalance()));
    daysLabel->setText(days == 0 ? tr("0 days. The quarter has ended.") : dayCount(days));
    spendableLabel->setText(formatPoints(budget.getSpendablePoints()));
    dailyLabel->setText(formatPoints(budget.getDailyBudget()));
    weeklyLabel->setText(formatPoints(budget.getWeeklyBudget()));
    showSuggestions();

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
    checkAlerts();
}

void PlanPage::showMessage(QLabel *label, const QString &text, bool error) {
    label->setStyleSheet(error ? AppStyle::errorText(label) : QString());
    label->setText(text);
    label->setVisible(!text.isEmpty());
}

QString PlanPage::formatPoints(double points) {
    return AppStyle::points(points);
}
