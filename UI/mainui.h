#ifndef MAINUI_H
#define MAINUI_H

#include <QDate>
#include <QMainWindow>

#include "balance.h"
#include "budget.h"

#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainUI;
}
QT_END_NAMESPACE

class QTabWidget;

// The main window. Its Budget tab is a sandbox (T04): it starts from the real balance
// and days left, but nothing changed there is saved or seen by friends.
class MainUI : public QMainWindow {
    Q_OBJECT

public:
    struct RealNumbers {
        double balance;
        int days;
    };
    // Returns the saved balance and days left, or nothing if they can't be read.
    using RealNumbersLoader = std::function<std::optional<RealNumbers>()>;

    // Without a loader, the real numbers are 0 points and the quarter calendar's days.
    explicit MainUI(QWidget *parent = nullptr);
    explicit MainUI(RealNumbersLoader loadRealNumbers, QWidget *parent = nullptr);
    ~MainUI() override;

    // Puts the Purchases tab first and the Budget tab second.
    void setPurchasePage(QWidget *page);
    // Adds a tab after the existing ones. Call setPurchasePage first.
    void addPage(QWidget *page, const QString &title);
    // Replaces the sandbox numbers with the real ones. Opening the Budget tab and the
    // "Use my real numbers" button both call this. Returns false, keeping the sandbox
    // numbers, if the real ones can't be loaded.
    bool useRealNumbers();
    // Counts days left down by one for each day from the last date shown to `today`.
    // A timer calls this with the current date so the count drops at midnight.
    void showDate(const QDate &today);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void connectControls();
    void setBalance();
    void addPoints();
    void spendPoints();
    bool updateRemainingDays(int days);
    bool validateAmount();
    void finishBalanceUpdate();
    void refreshDisplay();
    static QString formatPoints(double points);

    // Budget refers to balance, so balance must be constructed first.
    Balance balance;
    Budget budget;
    Ui::MainUI *ui;
    RealNumbersLoader loadRealNumbers;
    QTabWidget *pages = nullptr;
    QWidget *budgetPage = nullptr;
    QDate shownDate;
};
#endif // MAINUI_H
