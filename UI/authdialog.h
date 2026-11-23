#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include "localaccounts.h"

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

class AuthDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuthDialog(QWidget *parent = nullptr);
    void reset();
    // The signed-in account's ID (Profile::accountId), passed to every storage call.
    QString accountName() const;
    const LocalAccounts::Profile &profile() const;

private:
    void setCreatingAccount(bool creating);
    void submit();

    QLabel *heading;
    QLineEdit *email;
    QLabel *displayNameLabel;
    QLineEdit *displayName;
    QLabel *diningPlanLabel;
    QComboBox *diningPlan;
    QLineEdit *password;
    QLabel *confirmationLabel;
    QLineEdit *confirmation;
    QLabel *message;
    QPushButton *submitButton;
    QPushButton *switchButton;
    bool creatingAccount = false;
    LocalAccounts::Profile signedInProfile;
};

#endif // AUTHDIALOG_H
