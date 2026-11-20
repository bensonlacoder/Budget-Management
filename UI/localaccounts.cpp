#include "localaccounts.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

#include <array>

namespace {
constexpr int SALT_BYTES = 16;
constexpr int HASH_BYTES = 32;
constexpr int HASH_ITERATIONS = 600000;
constexpr int MIN_PASSWORD_LENGTH = 8;

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
Result create(const QString &username, const QString &password) {
    static const QRegularExpression allowedUsername(
        QRegularExpression::anchoredPattern("[A-Za-z0-9_]{3,32}"));
    const QString trimmedUsername = username.trimmed();
    if (!allowedUsername.match(trimmedUsername).hasMatch()) {
        return Result::InvalidUsername;
    }
    if (password.size() < MIN_PASSWORD_LENGTH) {
        return Result::ShortPassword;
    }

    const QString path = accountsPath();
    if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).absolutePath())) {
        return Result::StorageError;
    }

    QLockFile lock(path + ".lock");
    QJsonObject accounts;
    if (!lock.tryLock() || !readAccounts(path, accounts)) {
        return Result::StorageError;
    }

    const QString key = trimmedUsername.toLower();
    if (accounts.contains(key)) {
        return Result::UsernameTaken;
    }

    std::array<quint32, SALT_BYTES / sizeof(quint32)> randomWords;
    QRandomGenerator::system()->fillRange(randomWords.data(), randomWords.size());
    const QByteArray salt(reinterpret_cast<const char *>(randomWords.data()), SALT_BYTES);
    const QByteArray hash = deriveHash(password, salt);
    if (hash.size() != HASH_BYTES) {
        return Result::StorageError;
    }
    accounts.insert(key, QJsonObject{
        {"salt", QString::fromLatin1(salt.toHex())},
        {"hash", QString::fromLatin1(hash.toHex())}
    });

    const QByteArray data = QJsonDocument(accounts).toJson();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        return Result::StorageError;
    }
    return Result::Success;
}

Result signIn(const QString &username, const QString &password) {
    const QString path = accountsPath();
    QJsonObject accounts;
    if (path.isEmpty() || !readAccounts(path, accounts)) {
        return Result::StorageError;
    }

    const auto entry = accounts.value(username.trimmed().toLower());
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
    return hashesMatch(hash, candidate) ? Result::Success : Result::IncorrectCredentials;
}
}
