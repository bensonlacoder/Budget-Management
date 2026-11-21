#ifndef LOCALACCOUNTS_H
#define LOCALACCOUNTS_H

#include <QList>
#include <QString>

namespace LocalAccounts {
enum class Result {
    Success,
    InvalidUsername,
    InvalidDisplayName,
    ShortPassword,
    UsernameTaken,
    IncorrectCredentials,
    UnknownFriendId,
    OwnFriendId,
    AlreadyFriends,
    RequestAlreadySent,
    IncomingRequestExists,
    RequestUnavailable,
    StorageError
};

struct Profile {
    QString displayName;
    QString friendId;
    double balance = 0.0;
};

struct FriendSummary {
    QString displayName;
    double balance;
};

struct FriendRequest {
    QString displayName;
    QString friendId;
};

struct Friends {
    QList<FriendSummary> accepted;
    QList<FriendRequest> incoming;
};

Result create(const QString &username, const QString &password,
              const QString &displayName, Profile &profile);
Result signIn(const QString &username, const QString &password, Profile &profile);
bool saveBalance(const QString &username, double previous, double next);
bool loadFriends(const QString &username, Friends &friends);
Result sendRequest(const QString &username, const QString &friendId);
Result respondToRequest(const QString &username, const QString &friendId, bool accept);
}

#endif // LOCALACCOUNTS_H
