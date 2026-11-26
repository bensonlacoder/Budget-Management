#ifndef PURCHASESPAGE_H
#define PURCHASESPAGE_H

#include "localaccounts.h"
#include "scumenu.h"

#include <QDateTime>
#include <QSet>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QGridLayout;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QPushButton;
class QTableWidget;

class PurchasesPage : public QWidget {
    Q_OBJECT

public:
    explicit PurchasesPage(const QString &accountName, QWidget *parent = nullptr);
    void showBalance(double balance);

signals:
    void balanceUpdated(double newBalance);

private:
    void updateMenuItems();
    void refreshHours(bool force = false);
    void updateLocationHours();
    QString selectedLocation() const;
    ScuMenu::Schedule locationSchedule(const QString &name) const;
    QString hoursText(const ScuMenu::Schedule &schedule, QTime now) const;
    void useMenuItem(int index);
    void updateConfiguredPrice();
    void recordPurchase();
    void removeSelectedEntry();
    void editSelectedEntry();
    void refundSelectedPurchase();
    void refundDialog(const LocalAccounts::Refund &refund);
    void refreshHistory();
    void renderHistory();
    const LocalAccounts::HistoryEntry *selectedEntry() const;

    const QString accountName;
    QDateEdit *date;
    QComboBox *location;
    QComboBox *menuItem;
    QFormLayout *form;
    QComboBox *choice;
    QLabel *choiceLabel;
    QWidget *extras;
    QGridLayout *extrasLayout;
    QList<QCheckBox *> extraChecks;
    const ScuMenu::Item *currentItem = nullptr;
    QLineEdit *item;
    QDoubleSpinBox *points;
    QLabel *balanceLabel;
    QLabel *hoursLabel;
    QLabel *menuStatus;
    QLabel *feedback;
    QLabel *historyStatus;
    QTableWidget *history;
    QPushButton *removePurchase;
    QPushButton *editEntry;
    QPushButton *refundPurchase;
    QCheckBox *filterDates;
    QDateEdit *filterFrom;
    QDateEdit *filterTo;
    QComboBox *filterLocation;
    QList<LocalAccounts::HistoryEntry> entries;
    QList<LocalAccounts::HistoryEntry> visibleEntries;
    bool historyLoaded = false;
    QNetworkAccessManager *network;
    QDate menuDate;
    QDateTime lastMenuRequest;
    QSet<QString> pendingMenus;
    QHash<QString, ScuMenu::DailyMenu> dailyMenus;
};

#endif // PURCHASESPAGE_H
