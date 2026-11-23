#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include "localaccounts.h"

#include <QDialog>

class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;

// Sign in, create an account, verify its email with a code, or reset a forgotten
// password with a code. Accepted once the user is signed in.
class AuthDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuthDialog(QWidget *parent = nullptr);
    void reset();
    // The signed-in account's ID (Profile::accountId), passed to every storage call.
    QString accountName() const;
    const LocalAccounts::Profile &profile() const;

private:
    enum class Screen { SignIn, CreateAccount, VerifyEmail, ResetPassword };

    void showScreen(Screen next);
    void submit();
    void signIn();
    void createAccount();
    void verifyEmail();
    void resetPassword();
    void sendCode();
    // Sends a sign-up code to pendingEmail and moves to the verify screen.
    LocalAccounts::Result startVerification();
    void showError(LocalAccounts::Result result);

    Screen screen = Screen::SignIn;
    QLabel *heading;
    QFormLayout *form;
    QLineEdit *email;
    QLineEdit *displayName;
    QComboBox *diningPlan;
    QLabel *passwordLabel;
    QLineEdit *password;
    QLineEdit *confirmation;
    QLineEdit *code;
    QLabel *message;
    QPushButton *submitButton;
    QPushButton *switchButton;
    QPushButton *codeButton;
    QPushButton *forgotButton;
    // Kept while the email is being verified, to sign in once it is.
    QString pendingEmail;
    QString pendingPassword;
    LocalAccounts::Profile signedInProfile;
};

#endif // AUTHDIALOG_H
