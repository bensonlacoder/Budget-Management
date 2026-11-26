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
