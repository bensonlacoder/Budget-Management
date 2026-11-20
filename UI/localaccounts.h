#ifndef LOCALACCOUNTS_H
#define LOCALACCOUNTS_H

#include <QString>

namespace LocalAccounts {
enum class Result {
    Success,
    InvalidUsername,
    ShortPassword,
    UsernameTaken,
    IncorrectCredentials,
    StorageError
};

Result create(const QString &username, const QString &password);
Result signIn(const QString &username, const QString &password);
}

#endif // LOCALACCOUNTS_H
