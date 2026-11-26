#ifndef CHARTSPAGE_H
#define CHARTSPAGE_H

#include "localaccounts.h"

#include <QMap>
#include <QWidget>

class QComboBox;
class QDateEdit;
class QLabel;
class QShowEvent;
class QTableWidget;
class SpendingChart;

class ChartsPage : public QWidget {
    Q_OBJECT

public:
    struct Spending {
        // Store hundredths of a point so totals and budget differences compare exactly.
        QMap<QString, qint64> locations;
        QMap<QDate, qint64> periods;
    };

    explicit ChartsPage(const QString &accountName, QWidget *parent = nullptr);
    void refresh();

    static Spending summarize(const QList<LocalAccounts::HistoryEntry> &history,
                              const QDate &from, const QDate &to, bool weekly);

    // The daily budget each day from `from` through `to` had at its start, in
    // hundredths of a point (SP-11). A past day's budget is rebuilt from the history:
    // its starting balance is today's balance plus every change since, and its days
    // left are today's plus the days that have passed. So a day's budget doesn't
    // change after the day is over. Days after today use today's budget.
    static QMap<QDate, qint64> dailyBudgets(const QList<LocalAccounts::HistoryEntry> &history,
                                            double balanceToday, double reserveGoal,
                                            int daysLeftToday, const QDate &today,
                                            const QDate &from, const QDate &to);

protected:
    void showEvent(QShowEvent *event) override;

private:
    const QString accountName;
    QDateEdit *from;
    QDateEdit *to;
    QComboBox *period;
    QLabel *status;
    QTableWidget *locations;
    QTableWidget *comparison;
    SpendingChart *chart;
};

#endif // CHARTSPAGE_H
