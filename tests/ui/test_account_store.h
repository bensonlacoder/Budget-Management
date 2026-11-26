#ifndef TEST_ACCOUNT_STORE_H
#define TEST_ACCOUNT_STORE_H

#include "emailsender.h"
#include "localaccounts.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QStandardPaths>
#include <QUuid>

// Points LocalAccounts at a fresh, disposable database for one test and deletes it
// afterwards, so tests never touch a real user's saved accounts.
//
// It also replaces the email sender with a fake that records each code instead of
// sending it, so no test ever sends real email. Set `sendingFails` to simulate an
// unreachable sender. Afterwards it installs refuseToSend, not the real sender.
struct TestAccountStore {
    // The sender outside a TestAccountStore: the real one has a built-in URL, so a test
    // that forgot the fake sender would otherwise send real email.
    static bool refuseToSend(const QString &, const QString &, EmailSender::Purpose) {
        return false;
    }

    QString previous = QCoreApplication::applicationName();
    QHash<QString, QString> sentCodes; // Last code sent to each email.
    int emailsSent = 0;
    bool sendingFails = false;

    // The store of the test that is running, for helpers outside the test body.
    static TestAccountStore *&current() {
        static TestAccountStore *store = nullptr;
        return store;
    }

    TestAccountStore() {
        current() = this;
        QCoreApplication::setApplicationName(
            "DiningPointsTest-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
        EmailSender::setSender([this](const QString &email, const QString &code,
                                      EmailSender::Purpose) {
            if (sendingFails) {
                return false;
            }
            sentCodes.insert(email, code);
            ++emailsSent;
            return true;
        });
    }
    ~TestAccountStore() {
        current() = nullptr;
        EmailSender::setSender(refuseToSend);
        LocalAccounts::closeStorage(); // Windows can't delete an open database file.
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
        for (const char *name : {"budget.sqlite", "budget.sqlite-journal", "email-sender.ini"}) {
            QFile::remove(dir.filePath(name));
        }
        QDir().rmdir(dir.absolutePath());
        QCoreApplication::setApplicationName(previous);
    }
    TestAccountStore(const TestAccountStore &) = delete;
    TestAccountStore &operator=(const TestAccountStore &) = delete;

    // Creates an account and verifies its email, ready to sign in.
    LocalAccounts::Profile createVerified(const QString &email, const QString &displayName,
                                          const QString &plan = "basic",
                                          const QString &password = "correct-horse") {
        LocalAccounts::Profile profile;
        if (LocalAccounts::create(email, password, displayName, plan, profile)
                != LocalAccounts::Result::Success
            || LocalAccounts::sendVerificationCode(email) != LocalAccounts::Result::Success
            || LocalAccounts::verifyEmail(email, sentCodes.value(profile.email))
                   != LocalAccounts::Result::Success) {
            return {};
        }
        return profile;
    }
};

#endif // TEST_ACCOUNT_STORE_H
