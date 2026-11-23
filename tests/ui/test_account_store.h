#ifndef TEST_ACCOUNT_STORE_H
#define TEST_ACCOUNT_STORE_H

#include "localaccounts.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>

// Points LocalAccounts at a fresh, disposable database for one test and deletes it
// afterwards, so tests never touch a real user's saved accounts.
struct TestAccountStore {
    QString previous = QCoreApplication::applicationName();
    TestAccountStore() {
        QCoreApplication::setApplicationName(
            "DiningPointsTest-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    ~TestAccountStore() {
        LocalAccounts::closeStorage(); // Windows can't delete an open database file.
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
        for (const char *name : {"budget.sqlite", "budget.sqlite-journal"}) {
            QFile::remove(dir.filePath(name));
        }
        QDir().rmdir(dir.absolutePath());
        QCoreApplication::setApplicationName(previous);
    }
    TestAccountStore(const TestAccountStore &) = delete;
    TestAccountStore &operator=(const TestAccountStore &) = delete;
};

#endif // TEST_ACCOUNT_STORE_H
