#include "authdialog.h"
#include "chartspage.h"
#include "friendspage.h"
#include "mainui.h"
#include "planpage.h"
#include "profilepage.h"
#include "purchasespage.h"

#include <QApplication>
#include <QMessageBox>

#include <optional>

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
        // The Budget tab is a sandbox: it only reads the real numbers, never saves them.
        auto *window = new MainUI([accountName]() -> std::optional<MainUI::RealNumbers> {
            LocalAccounts::Profile saved;
            if (!LocalAccounts::loadProfile(accountName, saved)) {
                return std::nullopt;
            }
            return MainUI::RealNumbers{saved.balance, ProfilePage::daysLeftFor(saved)};
        });
        window->setAttribute(Qt::WA_DeleteOnClose);
        auto *purchases = new PurchasesPage(accountName, window);
        purchases->showBalance(profile.balance);
        window->setPurchasePage(purchases);
        auto *plan = new PlanPage(accountName, window);
        auto *charts = new ChartsPage(accountName, window);
        auto *profilePage = new ProfilePage(accountName, window);
        // Spending alerts from the Plan tab pop up without blocking the window.
        QObject::connect(plan, &PlanPage::alertRaised, window, [window](const QString &message) {
            auto *box = new QMessageBox(QMessageBox::Warning, QObject::tr("Spending alert"),
                                        message, QMessageBox::Ok, window);
            box->setAttribute(Qt::WA_DeleteOnClose);
            box->setModal(false);
            box->show();
        });
        auto *friendsPage = new FriendsPage(accountName, profile.friendId, window);
        window->addPage(plan, QObject::tr("P&lan"));
        window->addPage(charts, QObject::tr("&Charts"));
        window->addPage(friendsPage, QObject::tr("&Friends"));
        window->addPage(profilePage, QObject::tr("P&rofile"));

        // Keep every page showing the real balance after a change on another tab.
        QObject::connect(purchases, &PurchasesPage::balanceUpdated, plan, &PlanPage::refresh);
        QObject::connect(purchases, &PurchasesPage::balanceUpdated, charts, &ChartsPage::refresh);
        QObject::connect(purchases, &PurchasesPage::balanceUpdated, profilePage,
                         &ProfilePage::refresh);
        QObject::connect(profilePage, &ProfilePage::balanceUpdated, purchases,
                         &PurchasesPage::showBalance);
        QObject::connect(profilePage, &ProfilePage::balanceUpdated, plan, &PlanPage::refresh);
        QObject::connect(profilePage, &ProfilePage::balanceUpdated, charts, &ChartsPage::refresh);
        QObject::connect(profilePage, &ProfilePage::daysUpdated, plan, &PlanPage::refresh);
        QObject::connect(profilePage, &ProfilePage::signOutRequested, window, [&auth, window] {
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
