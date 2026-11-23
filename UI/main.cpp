#include "authdialog.h"
#include "friendsdialog.h"
#include "mainui.h"
#include "purchasespage.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMenuBar>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DiningPointsBudget");
    app.setApplicationDisplayName(QObject::tr("Dining Points Budget"));

    AuthDialog auth;
    auth.setAttribute(Qt::WA_QuitOnClose, false);
    QObject::connect(&auth, &QDialog::rejected, &app, &QApplication::quit);
    QObject::connect(&auth, &QDialog::accepted, &app, [&auth] {
        const QString accountName = auth.accountName();
        const auto profile = auth.profile();
        auto *window = new MainUI(
            profile.balance,
            [accountName](double previous, double next) {
                return LocalAccounts::saveBalance(accountName, previous, next);
            },
            profile.remainingDays,
            [accountName](int days) { return LocalAccounts::saveRemainingDays(accountName, days); });
        window->setAttribute(Qt::WA_DeleteOnClose);
        auto *purchases = new PurchasesPage(accountName, window);
        QObject::connect(purchases, &PurchasesPage::balanceUpdated, window, &MainUI::showSavedBalance);
        QObject::connect(window, &MainUI::balanceChanged, purchases, &PurchasesPage::showBalance);
        purchases->showBalance(profile.balance);
        window->setPurchasePage(purchases);
        auto *accountMenu = window->menuBar()->addMenu(QObject::tr("&Account"));
        auto *friends = accountMenu->addAction(QObject::tr("&Friends"));
        QObject::connect(friends, &QAction::triggered, window,
                         [window, accountName, friendId = profile.friendId] {
            auto *dialog = new FriendsDialog(accountName, friendId, window);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setWindowModality(Qt::WindowModal);
            dialog->show();
        });
        auto *signOut = accountMenu->addAction(QObject::tr("Sign &out"));
        QObject::connect(signOut, &QAction::triggered, window, [&auth, window] {
            auth.reset();
            auth.show();
            window->hide();
            window->deleteLater();
        });
        window->show();
    });
    auth.show();

    return app.exec();
}
