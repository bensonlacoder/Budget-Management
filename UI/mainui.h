#ifndef MAINUI_H
#define MAINUI_H

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
    MainUI(double startingBalance, std::function<bool(double, double)> saveBalance,
           QWidget *parent = nullptr);
    ~MainUI() override;
    void setPurchasePage(QWidget *page);
    void showSavedBalance(double points);

signals:
    void balanceChanged(double points);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void connectControls();
    void setBalance();
    void addPoints();
    void spendPoints();
    void updateRemainingDays(int days);
    bool validateAmount();
    void finishBalanceUpdate(double previousBalance);
    void refreshDisplay();
    static QString formatPoints(double points);

    // Budget refers to balance, so balance must be constructed first.
    Balance balance;
    Budget budget;
    Ui::MainUI *ui;
    std::function<bool(double, double)> saveBalance;
};
#endif // MAINUI_H
