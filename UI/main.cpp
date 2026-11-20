#include "authdialog.h"
#include "mainui.h"

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
        auto *window = new MainUI;
        window->setAttribute(Qt::WA_DeleteOnClose);
        auto *accountMenu = window->menuBar()->addMenu(QObject::tr("&Account"));
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
