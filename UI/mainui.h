#ifndef MAINUI_H
#define MAINUI_H

#include <QMainWindow>

#include "balance.h"
#include "budget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainUI;
}
QT_END_NAMESPACE

class MainUI : public QMainWindow {
    Q_OBJECT

public:
    explicit MainUI(QWidget *parent = nullptr);
    ~MainUI() override;

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void connectControls();
    void setBalance();
    void addPoints();
    void spendPoints();
    void updateRemainingDays(int days);
    bool validateAmount();
    void finishBalanceUpdate();
    void refreshDisplay();
    static QString formatPoints(double points);

    // Budget refers to balance, so balance must be constructed first.
    Balance balance;
    Budget budget;
    Ui::MainUI *ui;
};
#endif // MAINUI_H
