#ifndef PLANPAGE_H
#define PLANPAGE_H

#include "balance.h"
#include "budget.h"
#include "localaccounts.h"

#include <QDate>
#include <QList>
#include <QString>
#include <QWidget>

#include <array>
#include <functional>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QSpinBox;

// Plan tab: real spending advice from the saved balance, days left, reserve goal (T06)
// and dining habits (T07), and spending alerts (T13). The numbers come from Budget;
// this page only loads, saves and shows them.
class PlanPage : public QWidget {
    Q_OBJECT

public:
    explicit PlanPage(const QString &accountName, QWidget *parent = nullptr);
    // Reloads the saved balance, days left and history, for example after the balance
    // changes on another tab.
    void refresh();

    // Average net points spent per day over the 7 days ending `today` (SP-07):
    // purchases and corrections that lowered the balance, minus refunds, divided by 7.
    // Never below 0.
    static double recentSpendingRate(const QList<LocalAccounts::HistoryEntry> &history,
                                     const QDate &today);

    // Net points spent from `first` through `last`, counting both (SP-07): purchases
    // and corrections that lowered the balance, minus refunds. Can be negative.
    static double netSpending(const QList<LocalAccounts::HistoryEntry> &history,
                              const QDate &first, const QDate &last);

    static constexpr int SpendingWindowDays = 7;

    // Reads dining habits from a CSV file (format in SP-08), saves them and shows the
    // result. Upload CSV... calls this with the chosen file. A bad file shows a
    // message and changes nothing.
    bool loadHabitsFile(const QString &path);

signals:
    // An alert that is turned on has just started to apply. Each alert is raised at
    // most once a day; the Plan tab also shows it until it no longer applies (SP-09).
    void alertRaised(const QString &message);

private:
    void saveAlerts();
    void checkAlerts();
    void saveReserve();
    bool saveSettings(const std::function<void(LocalAccounts::PlanSettings &)> &change);
    void habitsEdited();
    bool applyHabits(const Budget::DiningHabits &habits, const QString &savedMessage);
    void showHabits(const Budget::DiningHabits &habits);
    void chooseHabitsFile();
    void showAdvice();
    void showSuggestions();
    static void showMessage(QLabel *label, const QString &text, bool error);
    static QString formatPoints(double points);

    const QString accountName;
    // Budget refers to balance, so balance must be constructed first.
    Balance balance;
    Budget budget;
    QDate shownDate;
    bool loaded = false;

    QLabel *balanceLabel;
    QLabel *daysLabel;
    QLabel *spendableLabel;
    QLabel *dailyLabel;
    QLabel *weeklyLabel;
    QLabel *perMealLabel;
    QLabel *perCampusDayLabel;
    QLabel *campusLabel;
    QLabel *loadMessage;
    QLabel *alertsBanner;

    QDoubleSpinBox *reserveEdit;
    QLabel *reserveMessage;

    std::array<QSpinBox *, 7> mealEdits{}; // Monday first.
    QLabel *habitsMessage;

    QLabel *projectionLabel;
    QLabel *reserveWarning;

    QCheckBox *overDailyAlert;
    QCheckBox *runOutAlert;
    QLabel *alertsMessage;
    double spentToday = 0.0;
    // Alerts aren't raised until the page is shown in a window, so whoever creates it
    // can connect to alertRaised first.
    bool raiseAlerts = false;
    QDate overDailyRaisedOn;
    QDate runOutRaisedOn;
};

#endif // PLANPAGE_H
