#include "localaccounts.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr int SALT_BYTES = 16;
constexpr int HASH_BYTES = 32;
constexpr int HASH_ITERATIONS = 600000;
constexpr int MIN_PASSWORD_LENGTH = 8;
constexpr int FRIEND_ID_LENGTH = 12;
// Wait briefly if another window of the app is writing the same file.
constexpr int LOCK_TIMEOUT_MS = 3000;

using LocalAccounts::Result;

QString accountsPath() {
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (directory.isEmpty()) {
        return {};
    }
    return QDir(directory).filePath("accounts.json");
}

bool readAccounts(const QString &path, QJsonObject &accounts) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        // A missing file is normal before the first account is created.
        return !QFileInfo::exists(path);
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (file.error() != QFileDevice::NoError || !document.isObject()) {
        return false;
    }

    accounts = document.object();
    return true;
}

bool loadAccounts(QJsonObject &accounts) {
    const QString path = accountsPath();
    return !path.isEmpty() && readAccounts(path, accounts);
}

template <typename Update>
Result updateAccounts(Update update) {
    const QString path = accountsPath();
    if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).absolutePath())) {
        return Result::StorageError;
    }

    QLockFile lock(path + ".lock");
    QJsonObject accounts;
    if (!lock.tryLock(LOCK_TIMEOUT_MS) || !readAccounts(path, accounts)) {
        return Result::StorageError;
    }

    const Result result = update(accounts);
    if (result != Result::Success) {
        return result;
    }

    const QByteArray data = QJsonDocument(accounts).toJson();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        return Result::StorageError;
    }
    return Result::Success;
}

QString accountKey(const QString &username) {
    return username.trimmed().toLower();
}

QString findByFriendId(const QJsonObject &accounts, const QString &friendId) {
    const QString normalized = friendId.trimmed().toUpper();
    if (normalized.size() != FRIEND_ID_LENGTH) {
        return {};
    }
    for (auto it = accounts.constBegin(); it != accounts.constEnd(); ++it) {
        if (it.value().toObject().value("friendId").toString() == normalized) {
            return it.key();
        }
    }
    return {};
}

QString newFriendId(const QJsonObject &accounts) {
    QString id;
    do {
        id = QUuid::createUuid().toString(QUuid::Id128).left(FRIEND_ID_LENGTH).toUpper();
    } while (!findByFriendId(accounts, id).isEmpty());
    return id;
}

LocalAccounts::Profile profileFrom(const QJsonObject &account) {
    // Accounts saved before days were stored, or with an invalid value, use the default.
    const int days = account.value("remainingDays").toInt(-1);
    const bool validDays = days >= 0 && days <= LocalAccounts::MaxRemainingDays;
    return {account.value("displayName").toString(), account.value("friendId").toString(),
            account.value("balance").toDouble(),
            validDays ? days : LocalAccounts::DefaultRemainingDays};
}

QByteArray deriveHash(const QString &password, const QByteArray &salt) {
    return QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256, password.toUtf8(), salt, HASH_ITERATIONS, HASH_BYTES);
}

bool hashesMatch(const QByteArray &expected, const QByteArray &actual) {
    if (expected.size() != HASH_BYTES || actual.size() != HASH_BYTES) {
        return false;
    }

    unsigned char difference = 0;
    for (int i = 0; i < HASH_BYTES; ++i) {
        difference |= static_cast<unsigned char>(expected[i] ^ actual[i]);
    }
    return difference == 0;
}
}

namespace LocalAccounts {
const QList<DiningPlan> &diningPlans() {
    static const QList<DiningPlan> plans{
        {"basic", "Basic Plan", 2237, false},
        {"preferred", "Preferred Plan", 2617, false},
        {"junior-senior", "Junior/Senior Plan", 1536, false},
        {"apartment", "Apartment Plan", 336, false},
        {"grad", "Graduate Plan", 336, false},
        {"law", "Law Plan", 504, true}
    };
    return plans;
}

Result create(const QString &username, const QString &password, const QString &displayName,
              const QString &diningPlanId, Profile &profile) {
    static const QRegularExpression allowedUsername(
        QRegularExpression::anchoredPattern("[A-Za-z0-9_]{3,32}"));
    const QString trimmedUsername = username.trimmed();
    if (!allowedUsername.match(trimmedUsername).hasMatch()) {
        return Result::InvalidUsername;
    }
    const QString name = displayName.trimmed();
    if (name.isEmpty() || name.size() > 32) {
        return Result::InvalidDisplayName;
    }
    if (password.size() < MIN_PASSWORD_LENGTH) {
        return Result::ShortPassword;
    }
    const auto &plans = diningPlans();
    const auto plan = std::find_if(plans.cbegin(), plans.cend(), [&](const DiningPlan &option) {
        return option.id == diningPlanId;
    });
    if (plan == plans.cend()) {
        return Result::InvalidDiningPlan;
    }

    std::array<quint32, SALT_BYTES / sizeof(quint32)> randomWords;
    QRandomGenerator::system()->fillRange(randomWords.data(), randomWords.size());
    const QByteArray salt(reinterpret_cast<const char *>(randomWords.data()), SALT_BYTES);
    const QByteArray hash = deriveHash(password, salt);
    if (hash.size() != HASH_BYTES) {
        return Result::StorageError;
    }
    return updateAccounts([&](QJsonObject &accounts) {
        const QString key = accountKey(username);
        if (accounts.contains(key)) {
            return Result::UsernameTaken;
        }
        const QJsonObject account{
            {"salt", QString::fromLatin1(salt.toHex())},
            {"hash", QString::fromLatin1(hash.toHex())},
            {"displayName", name},
            {"friendId", newFriendId(accounts)},
            {"diningPlan", plan->id},
            {"balance", plan->startingPoints}
        };
        accounts.insert(key, account);
        profile = profileFrom(account);
        return Result::Success;
    });
}

Result signIn(const QString &username, const QString &password, Profile &profile) {
    QJsonObject accounts;
    if (!loadAccounts(accounts)) {
        return Result::StorageError;
    }

    const QString key = accountKey(username);
    const auto entry = accounts.value(key);
    if (entry.isUndefined()) {
        return Result::IncorrectCredentials;
    }

    const auto account = entry.toObject();
    const auto salt = QByteArray::fromHex(account.value("salt").toString().toLatin1());
    const auto hash = QByteArray::fromHex(account.value("hash").toString().toLatin1());
    if (salt.size() != SALT_BYTES || hash.size() != HASH_BYTES) {
        return Result::StorageError;
    }

    const auto candidate = deriveHash(password, salt);
    if (candidate.size() != HASH_BYTES) {
        return Result::StorageError;
    }
    if (!hashesMatch(hash, candidate)) {
        return Result::IncorrectCredentials;
    }

    profile = profileFrom(account);
    if (!profile.friendId.isEmpty()) {
        return Result::Success;
    }

    // Older accounts contain only password credentials.
    return updateAccounts([&](QJsonObject &current) {
        auto existing = current.value(key).toObject();
        if (existing.isEmpty()) {
            return Result::StorageError;
        }
        if (existing.value("friendId").toString().isEmpty()) {
            const QString id = newFriendId(current);
            existing.insert("friendId", id);
            existing.insert("displayName", QStringLiteral("Diner %1").arg(id));
            current.insert(key, existing);
        }
        profile = profileFrom(existing);
        return Result::Success;
    });
}

bool saveBalance(const QString &username, double previous, double next) {
    if (!std::isfinite(next) || next < 0.0) {
        return false;
    }
    return updateAccounts([&](QJsonObject &accounts) {
        const QString key = accountKey(username);
        auto account = accounts.value(key).toObject();
        // Preserve a newer balance if this account is open in another window.
        if (account.isEmpty() || account.value("balance").toDouble() != previous) {
            return Result::StorageError;
        }
        account.insert("balance", next);
        accounts.insert(key, account);
        return Result::Success;
    }) == Result::Success;
}

bool saveRemainingDays(const QString &username, int days) {
    if (days < 0 || days > MaxRemainingDays) {
        return false;
    }
    return updateAccounts([&](QJsonObject &accounts) {
        const QString key = accountKey(username);
        auto account = accounts.value(key).toObject();
        if (account.isEmpty()) {
            return Result::StorageError;
        }
        account.insert("remainingDays", days);
        accounts.insert(key, account);
        return Result::Success;
    }) == Result::Success;
}

Result recordPurchase(const QString &username, const Purchase &purchase, double &newBalance) {
    const QString location = purchase.location.trimmed();
    const QString item = purchase.item.trimmed();
    const double amount = std::round(purchase.points * 100.0) / 100.0;
    if (!purchase.date.isValid() || purchase.date > QDate::currentDate() ||
        location.isEmpty() || location.size() > PurchaseTextLimit || item.size() > PurchaseTextLimit ||
        !std::isfinite(amount) || amount <= 0.0 || amount > MaxPurchasePoints) {
        return Result::InvalidPurchase;
    }

    double remaining = 0.0;
    const Result result = updateAccounts([&](QJsonObject &accounts) {
        const QString key = accountKey(username);
        auto account = accounts.value(key).toObject();
        const auto savedPurchases = account.value("purchases");
        if (account.isEmpty() || (!savedPurchases.isUndefined() && !savedPurchases.isArray())) {
            return Result::StorageError;
        }

        const double balance = std::round(account.value("balance").toDouble() * 100.0) / 100.0;
        if (!std::isfinite(balance)) {
            return Result::StorageError;
        }
        if (amount > balance) {
            return Result::InsufficientPoints;
        }
        remaining = std::round((balance - amount) * 100.0) / 100.0;

        auto purchases = savedPurchases.toArray();
        purchases.prepend(QJsonObject{
            {"id", QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {"date", purchase.date.toString(Qt::ISODate)},
            {"location", location},
            {"item", item},
            {"points", amount}
        });
        account.insert("balance", remaining);
        account.insert("purchases", purchases);
        accounts.insert(key, account);
        return Result::Success;
    });
    if (result == Result::Success) {
        newBalance = remaining;
    }
    return result;
}

Result removePurchase(const QString &username, const Purchase &purchase, double &newBalance) {
    double restored = 0.0;
    const Result result = updateAccounts([&](QJsonObject &accounts) {
        const QString key = accountKey(username);
        auto account = accounts.value(key).toObject();
        const auto savedPurchases = account.value("purchases");
        if (account.isEmpty() || (!savedPurchases.isUndefined() && !savedPurchases.isArray())) {
            return Result::StorageError;
        }

        auto purchases = savedPurchases.toArray();
        const QString date = purchase.date.toString(Qt::ISODate);
        const auto match = std::find_if(purchases.begin(), purchases.end(), [&](const QJsonValue &entry) {
            const auto row = entry.toObject();
            const QString id = row.value("id").toString();
            if (!purchase.id.isEmpty()) {
                return id == purchase.id;
            }
            // Purchases saved before IDs were introduced can still be removed.
            return id.isEmpty() && row.value("date").toString() == date &&
                   row.value("location").toString() == purchase.location &&
                   row.value("item").toString() == purchase.item &&
                   row.value("points").toDouble() == purchase.points;
        });
        if (match == purchases.end()) {
            return Result::PurchaseUnavailable;
        }

        const double points = match->toObject().value("points").toDouble();
        restored = std::round((account.value("balance").toDouble() + points) * 100.0) / 100.0;
        if (!std::isfinite(restored) || !std::isfinite(points) || points <= 0.0 || restored < 0.0) {
            return Result::StorageError;
        }
        purchases.erase(match);
        account.insert("balance", restored);
        account.insert("purchases", purchases);
        accounts.insert(key, account);
        return Result::Success;
    });
    if (result == Result::Success) {
        newBalance = restored;
    }
    return result;
}

bool loadPurchases(const QString &username, QList<Purchase> &purchases) {
    purchases.clear();
    QJsonObject accounts;
    if (!loadAccounts(accounts)) {
        return false;
    }
    const auto account = accounts.value(accountKey(username)).toObject();
    const auto savedPurchases = account.value("purchases");
    if (account.isEmpty() || (!savedPurchases.isUndefined() && !savedPurchases.isArray())) {
        return false;
    }

    const auto rows = savedPurchases.toArray();
    for (const auto &entry : rows) {
        const auto row = entry.toObject();
        purchases.append(Purchase{
            QDate::fromString(row.value("date").toString(), Qt::ISODate),
            row.value("location").toString(),
            row.value("item").toString(),
            row.value("points").toDouble(),
            row.value("id").toString()
        });
    }
    std::stable_sort(purchases.begin(), purchases.end(), [](const Purchase &left, const Purchase &right) {
        return left.date > right.date;
    });
    return true;
}

bool loadFriends(const QString &username, Friends &friends) {
    friends = {};
    QJsonObject accounts;
    if (!loadAccounts(accounts)) {
        return false;
    }

    const QString key = accountKey(username);
    const auto account = accounts.value(key).toObject();
    if (account.isEmpty()) {
        return false;
    }
    for (const auto entry : account.value("friends").toArray()) {
        const auto other = accounts.value(entry.toString()).toObject();
        if (other.value("friends").toArray().contains(key)) {
            const auto profile = profileFrom(other);
            friends.accepted.append(FriendSummary{profile.displayName, profile.balance});
        }
    }
    for (const auto entry : account.value("incoming").toArray()) {
        const auto profile = profileFrom(accounts.value(entry.toString()).toObject());
        if (!profile.friendId.isEmpty()) {
            friends.incoming.append(FriendRequest{profile.displayName, profile.friendId});
        }
    }
    return true;
}

Result sendRequest(const QString &username, const QString &friendId) {
    return updateAccounts([&](QJsonObject &accounts) {
        const QString senderKey = accountKey(username);
        const auto sender = accounts.value(senderKey).toObject();
        const QString recipientKey = findByFriendId(accounts, friendId);
        if (sender.isEmpty()) {
            return Result::StorageError;
        }
        if (recipientKey.isEmpty()) {
            return Result::UnknownFriendId;
        }
        if (senderKey == recipientKey) {
            return Result::OwnFriendId;
        }
        if (sender.value("friends").toArray().contains(recipientKey)) {
            return Result::AlreadyFriends;
        }
        if (sender.value("incoming").toArray().contains(recipientKey)) {
            return Result::IncomingRequestExists;
        }

        auto recipient = accounts.value(recipientKey).toObject();
        auto incoming = recipient.value("incoming").toArray();
        if (incoming.contains(senderKey)) {
            return Result::RequestAlreadySent;
        }
        incoming.append(senderKey);
        recipient.insert("incoming", incoming);
        accounts.insert(recipientKey, recipient);
        return Result::Success;
    });
}

Result respondToRequest(const QString &username, const QString &friendId, bool accept) {
    return updateAccounts([&](QJsonObject &accounts) {
        const QString recipientKey = accountKey(username);
        auto recipient = accounts.value(recipientKey).toObject();
        const QString senderKey = findByFriendId(accounts, friendId);
        if (recipient.isEmpty()) {
            return Result::StorageError;
        }
        if (senderKey.isEmpty() || senderKey == recipientKey) {
            return Result::RequestUnavailable;
        }

        auto incoming = recipient.value("incoming").toArray();
        auto request = incoming.begin();
        while (request != incoming.end() && *request != QJsonValue(senderKey)) {
            ++request;
        }
        if (request == incoming.end()) {
            return Result::RequestUnavailable;
        }
        incoming.erase(request);
        recipient.insert("incoming", incoming);

        if (accept) {
            auto sender = accounts.value(senderKey).toObject();
            auto senderFriends = sender.value("friends").toArray();
            auto recipientFriends = recipient.value("friends").toArray();
            senderFriends.append(recipientKey);
            recipientFriends.append(senderKey);
            sender.insert("friends", senderFriends);
            recipient.insert("friends", recipientFriends);
            accounts.insert(senderKey, sender);
        }
        accounts.insert(recipientKey, recipient);
        return Result::Success;
    });
}
}
