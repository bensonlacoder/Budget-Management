#ifndef PLANPAGE_H
#define PLANPAGE_H

#include "balance.h"
#include "budget.h"
#include "localaccounts.h"

#include <QDate>
#include <QList>
#include <QString>
#include <QWidget>

class QDoubleSpinBox;
class QLabel;

// Plan tab: real spending advice from the saved balance, days left and reserve goal
// (T06), with dining habits (T07) and alerts (T13) to follow. The numbers come from
// Budget; this page only loads, saves and shows them.
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

    static constexpr int SpendingWindowDays = 7;

private:
    void saveReserve();
    void showAdvice();
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
    QLabel *loadMessage;

    QDoubleSpinBox *reserveEdit;
    QLabel *reserveMessage;

    QLabel *projectionLabel;
    QLabel *reserveWarning;
};

#endif // PLANPAGE_H
