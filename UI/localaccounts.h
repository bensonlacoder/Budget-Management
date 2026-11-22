#ifndef LOCALACCOUNTS_H
#define LOCALACCOUNTS_H

#include <QDate>
#include <QList>
#include <QString>

namespace LocalAccounts {
enum class Result {
    Success,
    InvalidUsername,
    InvalidDisplayName,
    InvalidDiningPlan,
    ShortPassword,
    UsernameTaken,
    IncorrectCredentials,
    UnknownFriendId,
    OwnFriendId,
    AlreadyFriends,
    RequestAlreadySent,
    IncomingRequestExists,
    RequestUnavailable,
    InvalidPurchase,
    InsufficientPoints,
    PurchaseUnavailable,
    StorageError
};

struct Profile {
    QString displayName;
    QString friendId;
    double balance = 0.0;
};

struct DiningPlan {
    QString id;
    QString name;
    int startingPoints;
    bool perSemester;
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

struct Purchase {
    QDate date;
    QString location;
    QString item;
    double points;
    QString id = {};
};

constexpr int PurchaseTextLimit = 100;
constexpr double MaxPurchasePoints = 1000000.0;

const QList<DiningPlan> &diningPlans();
Result create(const QString &username, const QString &password, const QString &displayName,
              const QString &diningPlanId, Profile &profile);
Result signIn(const QString &username, const QString &password, Profile &profile);
bool saveBalance(const QString &username, double previous, double next);
Result recordPurchase(const QString &username, const Purchase &purchase, double &newBalance);
Result removePurchase(const QString &username, const Purchase &purchase, double &newBalance);
bool loadPurchases(const QString &username, QList<Purchase> &purchases);
bool loadFriends(const QString &username, Friends &friends);
Result sendRequest(const QString &username, const QString &friendId);
Result respondToRequest(const QString &username, const QString &friendId, bool accept);
}

#endif // LOCALACCOUNTS_H
