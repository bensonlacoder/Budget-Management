#ifndef MAINUI_H
#define MAINUI_H

#include <QDate>
#include <QMainWindow>

#include "balance.h"
#include "budget.h"

#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainUI;
}
QT_END_NAMESPACE

class MainUI : public QMainWindow {
    Q_OBJECT

public:
    explicit MainUI(QWidget *parent = nullptr);
    // Days left at the start of the quarter; Reset days returns to this count.
    static constexpr int DefaultRemainingDays = 77;

    // saveBalance(previous, next) and saveRemainingDays(days) return false if the
    // value could not be saved; the change is then undone. Either may be empty.
    // An out-of-range startingDays falls back to DefaultRemainingDays.
    MainUI(double startingBalance, std::function<bool(double, double)> saveBalance,
           int startingDays = DefaultRemainingDays,
           std::function<bool(int)> saveRemainingDays = {}, QWidget *parent = nullptr);
    ~MainUI() override;
    void setPurchasePage(QWidget *page);
    void showSavedBalance(double points);
    // Counts days left down by one for each day from the last date shown to `today`.
    // A timer calls this with the current date so the count drops at midnight.
    void showDate(const QDate &today);

signals:
    void balanceChanged(double points);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void connectControls();
    void setBalance();
    void addPoints();
    void spendPoints();
    bool updateRemainingDays(int days);
    void resetRemainingDays();
    bool validateAmount();
    void finishBalanceUpdate(double previousBalance);
    void refreshDisplay();
    static QString formatPoints(double points);

    // Budget refers to balance, so balance must be constructed first.
    Balance balance;
    Budget budget;
    Ui::MainUI *ui;
    std::function<bool(double, double)> saveBalance;
    std::function<bool(int)> saveRemainingDays;
    QDate shownDate;
};
#endif // MAINUI_H
