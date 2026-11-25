#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include "localaccounts.h"

#include <QDialog>

class QAction;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QMainWindow;
class QMenu;
class QPushButton;

// Sign in, create an account, verify its email with a code, or reset a forgotten
// password with a code. Accounts with two-step sign-in also enter an emailed code after
// the password. Accepted once the user is signed in.
class AuthDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuthDialog(QWidget *parent = nullptr);
    void reset();
    // The signed-in account's ID (Profile::accountId), passed to every storage call.
    QString accountName() const;
    const LocalAccounts::Profile &profile() const;

private:
    enum class Screen { SignIn, CreateAccount, VerifyEmail, ResetPassword, SignInCode };

    void showScreen(Screen next);
    void submit();
    void signIn();
    void createAccount();
    void verifyEmail();
    void resetPassword();
    void finishSignIn();
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
    QAction *revealPassword;
    QAction *revealConfirmation;
    QLineEdit *code;
    QLabel *message;
    QPushButton *submitButton;
    QPushButton *switchButton;
    QPushButton *codeButton;
    QPushButton *forgotButton;
    // Kept while a sign-up or sign-in code is being entered, to sign in once it is.
    QString pendingEmail;
    QString pendingPassword;
    LocalAccounts::Profile signedInProfile;
};

// Adds a checkable "Two-step sign-in" item to `menu` for the signed-in account. Each
// change is saved at once and confirmed in the window's status bar; if it can't be
// saved, the item goes back to its previous state.
QAction *addTwoStepSignInAction(QMenu *menu, QMainWindow *window, const QString &account,
                                bool enabled);

#endif // AUTHDIALOG_H
