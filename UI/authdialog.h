#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

class AuthDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuthDialog(QWidget *parent = nullptr);
    void reset();

private:
    void setCreatingAccount(bool creating);
    void submit();

    QLabel *heading;
    QLineEdit *username;
    QLineEdit *password;
    QLabel *confirmationLabel;
    QLineEdit *confirmation;
    QLabel *message;
    QPushButton *submitButton;
    QPushButton *switchButton;
    bool creatingAccount = false;
};

#endif // AUTHDIALOG_H
