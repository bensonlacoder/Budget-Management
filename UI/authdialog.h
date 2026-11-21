#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include "localaccounts.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

class AuthDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuthDialog(QWidget *parent = nullptr);
    void reset();
    QString accountName() const;
    const LocalAccounts::Profile &profile() const;

private:
    void setCreatingAccount(bool creating);
    void submit();

    QLabel *heading;
    QLineEdit *username;
    QLabel *displayNameLabel;
    QLineEdit *displayName;
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
