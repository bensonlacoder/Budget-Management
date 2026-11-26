#ifndef PROFILEPAGE_H
#define PROFILEPAGE_H

#include "localaccounts.h"

#include <QDate>
#include <QString>
#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

// Profile tab (T02): the signed-in account, and corrections to the real balance and
// days left. Everything here is saved; the Budget tab is only a sandbox.
class ProfilePage : public QWidget {
    Q_OBJECT

public:
    explicit ProfilePage(const QString &accountName, QWidget *parent = nullptr);

    // Reloads the saved profile, for example after a purchase on another tab.
    void refresh();

    // The real days left for a loaded profile: its manual correction if it has one,
    // otherwise the quarter calendar (T03).
    static int daysLeftFor(const LocalAccounts::Profile &profile,
                           const QDate &today = QDate::currentDate());

signals:
    // A balance correction was saved.
    void balanceUpdated(double newBalance);
    // A days correction was saved or cleared.
    void daysUpdated(int days);

private:
    void loadEditors();
    void saveProfile();
    void copyFriendId();
    void correctBalance();
    void correctDays();
    void resetDays();
    void changePassword();
    static void showMessage(QLabel *label, const QString &text, bool error);
    static QString formatPoints(double points);

    const QString accountName;
    LocalAccounts::Profile profile;
    QDate shownDate;

    QLineEdit *displayName;
    QLineEdit *email;
    QLabel *profileMessage;
    QLabel *friendId;
    QLabel *diningPlan;
    QLabel *balance;
    QLabel *days;
    QLabel *quarter;
    QLabel *lastUpdated;

    QComboBox *correctionMode;
    QDoubleSpinBox *correctionAmount;
    QLabel *balanceMessage;

    QSpinBox *daysCorrection;
    QPushButton *resetDaysButton;
    QLabel *daysMessage;

    QLineEdit *currentPassword;
    QLineEdit *newPassword;
    QLineEdit *confirmPassword;
    QLabel *passwordMessage;
};

#endif // PROFILEPAGE_H
