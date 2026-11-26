#include "localaccounts.h"
#include "emailsender.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>

namespace {
using LocalAccounts::Result;

constexpr int SALT_BYTES = 16;
constexpr int HASH_BYTES = 32;
constexpr int HASH_ITERATIONS = 600000;
constexpr int FRIEND_ID_LENGTH = 12;
// Wait this long if another window of the app is writing the database.
constexpr int BUSY_TIMEOUT_MS = 3000;
constexpr int SCHEMA_VERSION = 4;
const QString CONNECTION = QStringLiteral("dining-points-budget");

// ---- Points are stored as whole cents so comparisons are exact. ----

qint64 toCents(double points) {
    return std::llround(points * 100.0);
}

double fromCents(qint64 cents) {
    return static_cast<double>(cents) / 100.0;
}

// True if points is a finite amount from 0 (or above 0) up to MaxPurchasePoints.
bool validAmount(double points, bool allowZero) {
    if (!std::isfinite(points) || points > LocalAccounts::MaxPurchasePoints) {
        return false;
    }
    const qint64 cents = toCents(points);
    return allowZero ? cents >= 0 : cents > 0;
}

// ---- Database connection ----

QString databasePath() {
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return directory.isEmpty() ? QString() : QDir(directory).filePath("budget.sqlite");
}

// Prepares and runs one statement. Failures are logged with SQLite's message, since
// callers only report StorageError.
//
// Qt binds a default-constructed QString as SQL NULL, which breaks NOT NULL text
// columns and comparisons like `id != ?`. Strings are therefore always bound as text
// (possibly empty). Pass an invalid QVariant() for an intentional NULL.
bool run(QSqlQuery &query, const QString &sql, std::initializer_list<QVariant> values = {}) {
    if (query.prepare(sql)) {
        for (const QVariant &value : values) {
            const bool nullString =
                value.typeId() == QMetaType::QString && value.toString().isNull();
            query.addBindValue(nullString ? QVariant(QStringLiteral("")) : value);
        }
        if (query.exec()) {
            return true;
        }
    }
    qWarning("LocalAccounts: SQL failed: %s (%s)", qPrintable(query.lastError().text()),
             qPrintable(sql));
    return false;
}

bool createSchema(QSqlDatabase &db) {
    static const char *const statements[] = {
        "PRAGMA foreign_keys = ON",
        "CREATE TABLE IF NOT EXISTS accounts ("
        " id TEXT PRIMARY KEY,"
        " email TEXT NOT NULL UNIQUE COLLATE NOCASE,"
        " salt TEXT NOT NULL,"
        " hash TEXT NOT NULL,"
        " display_name TEXT NOT NULL,"
        " friend_id TEXT NOT NULL UNIQUE,"
        " dining_plan TEXT NOT NULL,"
        " balance_cents INTEGER NOT NULL CHECK (balance_cents >= 0),"
        " remaining_days INTEGER NOT NULL,"
        " days_counted_on TEXT NOT NULL,"
        " days_corrected INTEGER NOT NULL DEFAULT 0,"
        " email_verified INTEGER NOT NULL DEFAULT 0,"
        " last_updated TEXT,"
        " reserve_cents INTEGER NOT NULL DEFAULT 0,"
        " habits TEXT NOT NULL DEFAULT '0,0,0,0,0,0,0',"
        " alert_over_daily INTEGER NOT NULL DEFAULT 0,"
        " alert_run_out INTEGER NOT NULL DEFAULT 0,"
        " two_step INTEGER NOT NULL DEFAULT 0)",
        "CREATE TABLE IF NOT EXISTS purchases ("
        " id TEXT PRIMARY KEY,"
        " account_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " date TEXT NOT NULL,"
        " location TEXT NOT NULL,"
        " item TEXT NOT NULL,"
        " points_cents INTEGER NOT NULL CHECK (points_cents > 0),"
        " seq INTEGER NOT NULL)",
        "CREATE TABLE IF NOT EXISTS refunds ("
        " id TEXT PRIMARY KEY,"
        " account_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " purchase_id TEXT REFERENCES purchases(id) ON DELETE CASCADE,"
        " date TEXT NOT NULL,"
        " points_cents INTEGER NOT NULL CHECK (points_cents > 0),"
        " note TEXT NOT NULL,"
        " seq INTEGER NOT NULL)",
        "CREATE TABLE IF NOT EXISTS corrections ("
        " id TEXT PRIMARY KEY,"
        " account_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " date TEXT NOT NULL,"
        " change_cents INTEGER NOT NULL,"
        " seq INTEGER NOT NULL)",
        "CREATE TABLE IF NOT EXISTS friendships ("
        " account_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " friend_account_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " PRIMARY KEY (account_id, friend_account_id))",
        "CREATE TABLE IF NOT EXISTS friend_requests ("
        " sender_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " recipient_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " PRIMARY KEY (sender_id, recipient_id))",
        "CREATE TABLE IF NOT EXISTS saved_sections ("
        " account_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " section TEXT NOT NULL,"
        " PRIMARY KEY (account_id, section))",
        "CREATE INDEX IF NOT EXISTS purchases_by_account ON purchases(account_id)",
        "CREATE INDEX IF NOT EXISTS refunds_by_purchase ON refunds(purchase_id)",
        // Version 3: sign-up verification and password reset codes (T15).
        "CREATE TABLE IF NOT EXISTS email_codes ("
        " account_id TEXT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,"
        " purpose TEXT NOT NULL,"
        " salt TEXT NOT NULL,"
        " hash TEXT NOT NULL,"
        " expires_at TEXT NOT NULL,"
        " attempts INTEGER NOT NULL DEFAULT 0,"
        " PRIMARY KEY (account_id, purpose))",
    };
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("PRAGMA busy_timeout = %1").arg(BUSY_TIMEOUT_MS))
        || !query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
        return false;
    }
    const int version = query.value(0).toInt();
    // Version 1 ordered history by save time in milliseconds, which ties when entries
    // are saved within the same millisecond. Version 2 uses a shared sequence instead;
    // the old values are still increasing, so they are kept as sequence numbers.
    if (version == 1) {
        for (const char *table : {"purchases", "refunds", "corrections"}) {
            if (!query.exec(QStringLiteral("ALTER TABLE %1 RENAME COLUMN created_ms TO seq")
                                .arg(QLatin1String(table)))) {
                return false;
            }
        }
    }
    // Version 3 requires a verified email to sign in. Accounts made before then had no
    // way to verify, so they count as verified rather than being locked out.
    if (version >= 1 && version < 3
        && !query.exec(QStringLiteral("UPDATE accounts SET email_verified = 1"))) {
        return false;
    }
    // Version 4 adds two-step sign-in, off for existing accounts.
    if (version >= 1 && version < 4) {
        if (!query.exec(QStringLiteral(
                "SELECT COUNT(*) FROM pragma_table_info('accounts') WHERE name = 'two_step'"))
            || !query.next()) {
            return false;
        }
        if (query.value(0).toInt() == 0
            && !query.exec(QStringLiteral(
                "ALTER TABLE accounts ADD COLUMN two_step INTEGER NOT NULL DEFAULT 0"))) {
            return false;
        }
    }
    for (const char *statement : statements) {
        if (!query.exec(QString::fromLatin1(statement))) {
            return false;
        }
    }
    return query.exec(QStringLiteral("PRAGMA user_version = %1").arg(SCHEMA_VERSION));
}

// Returns the open database for the current data folder, or an invalid one if it
// can't be opened. Reopens when the folder changes (tests use a fresh folder each).
QSqlDatabase database() {
    const QString path = databasePath();
    if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).absolutePath())) {
        return {};
    }
    QSqlDatabase db = QSqlDatabase::contains(CONNECTION)
        ? QSqlDatabase::database(CONNECTION, false)
        : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), CONNECTION);
    if (db.isOpen() && db.databaseName() == path) {
        return db;
    }
    db.close();
    db.setDatabaseName(path);
    if (!db.open() || !createSchema(db)) {
        db.close();
        return {};
    }
    return db;
}

// Runs `work` in one transaction: every change is saved, or none is.
template <typename Work>
Result inTransaction(Work work) {
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        return Result::StorageError;
    }
    if (!db.transaction()) {
        qWarning("LocalAccounts: could not start a transaction: %s",
                 qPrintable(db.lastError().text()));
        return Result::StorageError;
    }
    const Result result = work(db);
    if (result != Result::Success) {
        db.rollback();
        return result;
    }
    if (!db.commit()) {
        qWarning("LocalAccounts: could not commit: %s", qPrintable(db.lastError().text()));
        db.rollback();
        return Result::StorageError;
    }
    return Result::Success;
}

// Runs a read-only query; returns false if the database can't be opened.
template <typename Work>
bool reading(Work work) {
    QSqlDatabase db = database();
    return db.isOpen() && work(db);
}

// ---- Accounts ----

QString normalizedEmail(const QString &email) {
    return email.trimmed().toLower();
}

bool isValidEmail(const QString &email) {
    static const QRegularExpression pattern(
        QRegularExpression::anchoredPattern(QStringLiteral(R"([^@\s]+@[^@\s]+\.[^@\s.]+)")));
    return email.size() <= LocalAccounts::MaxEmailLength && pattern.match(email).hasMatch();
}

bool isValidDisplayName(const QString &name) {
    return !name.isEmpty() && name.size() <= LocalAccounts::MaxDisplayNameLength;
}

QByteArray newSalt() {
    std::array<quint32, SALT_BYTES / sizeof(quint32)> words;
    QRandomGenerator::system()->fillRange(words.data(), words.size());
    return QByteArray(reinterpret_cast<const char *>(words.data()), SALT_BYTES);
}

QByteArray deriveHash(const QString &password, const QByteArray &salt) {
    return QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256, password.toUtf8(), salt, HASH_ITERATIONS, HASH_BYTES);
}

// Compares every byte so the time taken doesn't reveal how much of the hash matched.
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

bool passwordMatches(const QString &password, const QString &saltHex, const QString &hashHex) {
    const auto salt = QByteArray::fromHex(saltHex.toLatin1());
    const auto hash = QByteArray::fromHex(hashHex.toLatin1());
    return salt.size() == SALT_BYTES && hashesMatch(hash, deriveHash(password, salt));
}

QString newId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString normalizedFriendId(const QString &friendId) {
    return friendId.trimmed().toUpper();
}

constexpr auto PROFILE_COLUMNS =
    "id, email, display_name, friend_id, dining_plan, balance_cents, remaining_days,"
    " days_counted_on, days_corrected, last_updated, two_step";

// Last-updated times are saved in UTC (ISO 8601 with milliseconds) and returned in
// local time. Returns an invalid QDateTime for a missing or empty value.
QDateTime lastUpdatedFrom(const QVariant &value) {
    const QDateTime saved = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    return saved.isValid() ? saved.toLocalTime() : QDateTime();
}

// Reads a row selected with PROFILE_COLUMNS.
LocalAccounts::Profile profileFrom(const QSqlQuery &row) {
    LocalAccounts::Profile profile;
    profile.accountId = row.value(0).toString();
    profile.email = row.value(1).toString();
    profile.displayName = row.value(2).toString();
    profile.friendId = row.value(3).toString();
    profile.diningPlanId = row.value(4).toString();
    profile.balance = fromCents(row.value(5).toLongLong());
    profile.remainingDays = LocalAccounts::remainingDaysOn(
        row.value(6).toInt(), QDate::fromString(row.value(7).toString(), Qt::ISODate),
        QDate::currentDate());
    profile.daysCorrected = row.value(8).toBool();
    profile.lastUpdated = lastUpdatedFrom(row.value(9));
    profile.twoStepSignIn = row.value(10).toBool();
    return profile;
}

bool selectProfile(QSqlDatabase &db, const QString &account, LocalAccounts::Profile &profile) {
    QSqlQuery query(db);
    if (!run(query, QStringLiteral("SELECT %1 FROM accounts WHERE id = ?")
                        .arg(QLatin1String(PROFILE_COLUMNS)), {account})
        || !query.next()) {
        return false;
    }
    profile = profileFrom(query);
    return true;
}

// Reads the balance of `account` in cents. Returns false if there is no such account.
bool balanceOf(QSqlDatabase &db, const QString &account, qint64 &cents) {
    QSqlQuery query(db);
    if (!run(query, QStringLiteral("SELECT balance_cents FROM accounts WHERE id = ?"), {account})
        || !query.next()) {
        return false;
    }
    cents = query.value(0).toLongLong();
    return true;
}

// Saves the balance and records now as the last-updated time (T12). Every successful
// change to the balance or history ends here, including edits that leave the amount
// the same; a failed change rolls back with its transaction, timestamp included.
bool setBalance(QSqlDatabase &db, const QString &account, qint64 cents) {
    QSqlQuery query(db);
    return run(query, QStringLiteral("UPDATE accounts SET balance_cents = ?, last_updated = ?"
                                     " WHERE id = ?"),
               {cents, QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs), account});
}

// Adds `change` cents to the balance. Returns InsufficientPoints if it would go negative.
Result changeBalance(QSqlDatabase &db, const QString &account, qint64 change, double &newBalance) {
    qint64 cents = 0;
    if (!balanceOf(db, account, cents)) {
        return Result::StorageError;
    }
    if (cents + change < 0) {
        return Result::InsufficientPoints;
    }
    if (!setBalance(db, account, cents + change)) {
        return Result::StorageError;
    }
    newBalance = fromCents(cents + change);
    return Result::Success;
}

QString accountByFriendId(QSqlDatabase &db, const QString &friendId) {
    const QString normalized = normalizedFriendId(friendId);
    if (normalized.size() != FRIEND_ID_LENGTH) {
        return {};
    }
    QSqlQuery query(db);
    if (!run(query, QStringLiteral("SELECT id FROM accounts WHERE friend_id = ?"), {normalized})
        || !query.next()) {
        return {};
    }
    return query.value(0).toString();
}

bool exists(QSqlDatabase &db, const QString &sql, std::initializer_list<QVariant> values) {
    QSqlQuery query(db);
    return run(query, sql, values) && query.next();
}

QString newFriendId(QSqlDatabase &db) {
    QString id;
    do {
        id = QUuid::createUuid().toString(QUuid::Id128).left(FRIEND_ID_LENGTH).toUpper();
    } while (!accountByFriendId(db, id).isEmpty());
    return id;
}

// ---- Purchases and refunds ----

bool validPurchaseText(const QDate &date, const QString &location, const QString &item) {
    return date.isValid() && date <= QDate::currentDate() && !location.isEmpty()
        && location.size() <= LocalAccounts::PurchaseTextLimit
        && item.size() <= LocalAccounts::PurchaseTextLimit;
}

bool validRefundText(const QDate &date, const QString &note) {
    return date.isValid() && date <= QDate::currentDate()
        && note.size() <= LocalAccounts::PurchaseTextLimit;
}

// Reads a purchase's points and how much of it has been refunded, excluding one refund.
bool purchaseTotals(QSqlDatabase &db, const QString &account, const QString &purchaseId,
                    qint64 &points, qint64 &refunded, const QString &excludeRefund = {}) {
    QSqlQuery query(db);
    if (!run(query, QStringLiteral("SELECT points_cents FROM purchases WHERE id = ? AND account_id = ?"),
             {purchaseId, account})
        || !query.next()) {
        return false;
    }
    points = query.value(0).toLongLong();
    if (!run(query, QStringLiteral("SELECT COALESCE(SUM(points_cents), 0) FROM refunds"
                                   " WHERE purchase_id = ? AND id != ?"),
             {purchaseId, excludeRefund})
        || !query.next()) {
        return false;
    }
    refunded = query.value(0).toLongLong();
    return true;
}

// Checks a refund's link: the purchase must exist, be dated on or before the refund,
// and have enough left to refund.
Result checkRefundLink(QSqlDatabase &db, const QString &account, const QString &purchaseId,
                       qint64 cents, const QDate &date, const QString &excludeRefund = {}) {
    if (purchaseId.isEmpty()) {
        return Result::Success;
    }
    qint64 points = 0;
    qint64 refunded = 0;
    if (!purchaseTotals(db, account, purchaseId, points, refunded, excludeRefund)) {
        return Result::PurchaseUnavailable;
    }
    QSqlQuery query(db);
    if (!run(query, QStringLiteral("SELECT date FROM purchases WHERE id = ? AND account_id = ?"),
             {purchaseId, account})
        || !query.next()) {
        return Result::StorageError;
    }
    if (date < QDate::fromString(query.value(0).toString(), Qt::ISODate)) {
        return Result::RefundBeforePurchase;
    }
    return cents > points - refunded ? Result::RefundTooLarge : Result::Success;
}

// The next number in the order history entries were recorded, shared by purchases,
// refunds and corrections. Call it inside the transaction that inserts the entry.
// Returns NULL on failure, which the NOT NULL column then rejects.
QVariant nextSequence(QSqlDatabase &db) {
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "SELECT MAX(COALESCE((SELECT MAX(seq) FROM purchases), 0),"
            " COALESCE((SELECT MAX(seq) FROM refunds), 0),"
            " COALESCE((SELECT MAX(seq) FROM corrections), 0)) + 1"))
        || !query.next()) {
        return {};
    }
    return query.value(0).toLongLong();
}

QString habitsText(const LocalAccounts::DiningHabits &habits) {
    QStringList parts;
    for (int meals : habits) {
        parts << QString::number(meals);
    }
    return parts.join(',');
}

LocalAccounts::DiningHabits habitsFrom(const QString &text) {
    LocalAccounts::DiningHabits habits = {};
    const QStringList parts = text.split(',');
    for (int day = 0; day < static_cast<int>(habits.size()) && day < parts.size(); ++day) {
        habits[static_cast<size_t>(day)] = std::clamp(parts.at(day).toInt(), 0,
                                                      LocalAccounts::MaxMealsPerDay);
    }
    return habits;
}
}

namespace LocalAccounts {
const QList<DiningPlan> &diningPlans() {
    static const QList<DiningPlan> plans{
        {"basic", "Basic Plan", 2237},
        {"preferred", "Preferred Plan", 2617},
        {"junior-senior", "Junior/Senior Plan", 1536},
        {"apartment", "Apartment Plan", 336},
        {"grad", "Graduate Plan", 336}
    };
    return plans;
}

// ---- Accounts ----

Result create(const QString &email, const QString &password, const QString &displayName,
              const QString &diningPlanId, Profile &profile) {
    const QString address = normalizedEmail(email);
    if (!isValidEmail(address)) {
        return Result::InvalidEmail;
    }
    const QString name = displayName.trimmed();
    if (!isValidDisplayName(name)) {
        return Result::InvalidDisplayName;
    }
    if (password.size() < MinPasswordLength) {
        return Result::ShortPassword;
    }
    const auto &plans = diningPlans();
    const auto plan = std::find_if(plans.cbegin(), plans.cend(),
                                   [&](const DiningPlan &option) { return option.id == diningPlanId; });
    if (plan == plans.cend()) {
        return Result::InvalidDiningPlan;
    }

    // Hash outside the transaction so the database isn't locked while it runs.
    const QByteArray salt = newSalt();
    const QByteArray hash = deriveHash(password, salt);
    if (hash.size() != HASH_BYTES) {
        return Result::StorageError;
    }
    return inTransaction([&](QSqlDatabase &db) {
        if (exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE email = ? AND email_verified = 1"),
                   {address})) {
            return Result::EmailTaken;
        }
        QSqlQuery query(db);
        // An earlier sign-up that was never verified gives way to this one.
        if (!run(query, QStringLiteral("DELETE FROM accounts WHERE email = ? AND email_verified = 0"),
                 {address})) {
            return Result::StorageError;
        }
        const QString id = newId();
        if (!run(query, QStringLiteral(
                     "INSERT INTO accounts (id, email, salt, hash, display_name, friend_id,"
                     " dining_plan, balance_cents, remaining_days, days_counted_on)"
                     " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
                 {id, address, QString::fromLatin1(salt.toHex()), QString::fromLatin1(hash.toHex()),
                  name, newFriendId(db), plan->id, qint64(plan->startingPoints) * 100,
                  DefaultRemainingDays, QDate::currentDate().toString(Qt::ISODate)})) {
            return Result::StorageError;
        }
        return selectProfile(db, id, profile) ? Result::Success : Result::StorageError;
    });
}

namespace {
// What a correct password tells about its account.
struct PasswordCheck {
    QString account;
    bool verified = false;
    bool twoStep = false;
};

// Checks the email and password: Success, IncorrectCredentials or StorageError.
Result checkPassword(const QString &email, const QString &password, PasswordCheck &checked) {
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        return Result::StorageError;
    }
    QSqlQuery query(db);
    if (!run(query, QStringLiteral("SELECT id, salt, hash, balance_cents, email_verified, two_step"
                                   " FROM accounts WHERE email = ?"),
             {normalizedEmail(email)})) {
        return Result::StorageError;
    }
    if (!query.next()
        || !passwordMatches(password, query.value(1).toString(), query.value(2).toString())) {
        return Result::IncorrectCredentials;
    }
    // A damaged balance would make Balance throw when the window opens.
    if (query.value(3).toLongLong() < 0) {
        return Result::StorageError;
    }
    checked = {query.value(0).toString(), query.value(4).toBool(), query.value(5).toBool()};
    return Result::Success;
}
}

Result signIn(const QString &email, const QString &password, Profile &profile) {
    PasswordCheck checked;
    const Result result = checkPassword(email, password, checked);
    if (result != Result::Success) {
        return result;
    }
    if (!checked.verified) {
        return Result::EmailNotVerified;
    }
    if (checked.twoStep) {
        return Result::CodeRequired;
    }
    QSqlDatabase db = database();
    return selectProfile(db, checked.account, profile) ? Result::Success : Result::StorageError;
}

bool loadProfile(const QString &account, Profile &profile) {
    profile = {};
    return reading([&](QSqlDatabase &db) { return selectProfile(db, account, profile); });
}

Result updateProfile(const QString &account, const QString &displayName, const QString &email) {
    const QString name = displayName.trimmed();
    if (!isValidDisplayName(name)) {
        return Result::InvalidDisplayName;
    }
    const QString address = normalizedEmail(email);
    if (!isValidEmail(address)) {
        return Result::InvalidEmail;
    }
    return inTransaction([&](QSqlDatabase &db) {
        if (exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE email = ? AND id != ?"),
                   {address, account})) {
            return Result::EmailTaken;
        }
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("UPDATE accounts SET display_name = ?, email = ? WHERE id = ?"),
                 {name, address, account})) {
            return Result::StorageError;
        }
        return query.numRowsAffected() == 1 ? Result::Success : Result::StorageError;
    });
}

Result changePassword(const QString &account, const QString &currentPassword,
                      const QString &newPassword) {
    if (newPassword.size() < MinPasswordLength) {
        return Result::ShortPassword;
    }
    QString saltHex;
    QString hashHex;
    if (!reading([&](QSqlDatabase &db) {
            QSqlQuery query(db);
            if (!run(query, QStringLiteral("SELECT salt, hash FROM accounts WHERE id = ?"), {account})
                || !query.next()) {
                return false;
            }
            saltHex = query.value(0).toString();
            hashHex = query.value(1).toString();
            return true;
        })) {
        return Result::StorageError;
    }
    if (!passwordMatches(currentPassword, saltHex, hashHex)) {
        return Result::IncorrectCredentials;
    }
    const QByteArray salt = newSalt();
    const QByteArray hash = deriveHash(newPassword, salt);
    return inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        // Only replace the hash that was checked, in case another window changed it.
        if (!run(query, QStringLiteral("UPDATE accounts SET salt = ?, hash = ? WHERE id = ? AND hash = ?"),
                 {QString::fromLatin1(salt.toHex()), QString::fromLatin1(hash.toHex()), account,
                  hashHex})) {
            return Result::StorageError;
        }
        return query.numRowsAffected() == 1 ? Result::Success : Result::IncorrectCredentials;
    });
}

void closeStorage() {
    if (!QSqlDatabase::contains(CONNECTION)) {
        return;
    }
    {
        QSqlDatabase db = QSqlDatabase::database(CONNECTION, false);
        db.close();
    }
    QSqlDatabase::removeDatabase(CONNECTION);
}

// ---- Balance and days left ----

bool saveBalance(const QString &account, double previous, double next) {
    if (!validAmount(next, true)) {
        return false;
    }
    return inTransaction([&](QSqlDatabase &db) {
        qint64 cents = 0;
        // Refuse if another window changed the balance since this one loaded it.
        if (!balanceOf(db, account, cents) || cents != toCents(previous)) {
            return Result::StorageError;
        }
        return setBalance(db, account, toCents(next)) ? Result::Success : Result::StorageError;
    }) == Result::Success;
}

Result correctBalance(const QString &account, CorrectionMode mode, double amount,
                      double &newBalance) {
    if (!validAmount(amount, mode == CorrectionMode::Set)) {
        return Result::InvalidAmount;
    }
    const qint64 cents = toCents(amount);
    double updated = 0.0;
    const Result result = inTransaction([&](QSqlDatabase &db) {
        qint64 balance = 0;
        if (!balanceOf(db, account, balance)) {
            return Result::StorageError;
        }
        const qint64 change = mode == CorrectionMode::Set ? cents - balance
                            : mode == CorrectionMode::Add ? cents
                                                          : -cents;
        if (change == 0) {
            updated = fromCents(balance);
            return Result::Success;
        }
        const Result changed = changeBalance(db, account, change, updated);
        if (changed != Result::Success) {
            return changed;
        }
        QSqlQuery query(db);
        return run(query, QStringLiteral("INSERT INTO corrections (id, account_id, date, change_cents,"
                                         " seq) VALUES (?, ?, ?, ?, ?)"),
                   {newId(), account, QDate::currentDate().toString(Qt::ISODate), change,
                    nextSequence(db)})
            ? Result::Success
            : Result::StorageError;
    });
    if (result == Result::Success) {
        newBalance = updated;
    }
    return result;
}

int remainingDaysOn(int savedDays, const QDate &savedOn, const QDate &today) {
    if (!savedOn.isValid() || !today.isValid()) {
        return savedDays;
    }
    // A clock set backwards never adds days.
    const qint64 passed = std::max<qint64>(0, savedOn.daysTo(today));
    return static_cast<int>(std::max<qint64>(0, savedDays - passed));
}

bool saveRemainingDays(const QString &account, int days, const QDate &countedOn) {
    if (days < 0 || days > MaxRemainingDays || !countedOn.isValid()) {
        return false;
    }
    return inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("UPDATE accounts SET remaining_days = ?, days_counted_on = ?,"
                                       " days_corrected = 1 WHERE id = ?"),
                 {days, countedOn.toString(Qt::ISODate), account})) {
            return Result::StorageError;
        }
        return query.numRowsAffected() == 1 ? Result::Success : Result::StorageError;
    }) == Result::Success;
}

Result clearDaysCorrection(const QString &account) {
    return inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("UPDATE accounts SET remaining_days = ?, days_counted_on = ?,"
                                       " days_corrected = 0 WHERE id = ?"),
                 {DefaultRemainingDays, QDate::currentDate().toString(Qt::ISODate), account})) {
            return Result::StorageError;
        }
        return query.numRowsAffected() == 1 ? Result::Success : Result::StorageError;
    });
}

// ---- Purchases, refunds and history ----

Result recordPurchase(const QString &account, const Purchase &purchase, double &newBalance) {
    const QString location = purchase.location.trimmed();
    const QString item = purchase.item.trimmed();
    if (!validPurchaseText(purchase.date, location, item) || !validAmount(purchase.points, false)) {
        return Result::InvalidPurchase;
    }
    const qint64 cents = toCents(purchase.points);
    double updated = 0.0;
    const Result result = inTransaction([&](QSqlDatabase &db) {
        const Result changed = changeBalance(db, account, -cents, updated);
        if (changed != Result::Success) {
            return changed;
        }
        QSqlQuery query(db);
        return run(query, QStringLiteral("INSERT INTO purchases (id, account_id, date, location, item,"
                                         " points_cents, seq) VALUES (?, ?, ?, ?, ?, ?, ?)"),
                   {newId(), account, purchase.date.toString(Qt::ISODate), location, item, cents,
                    nextSequence(db)})
            ? Result::Success
            : Result::StorageError;
    });
    if (result == Result::Success) {
        newBalance = updated;
    }
    return result;
}

Result editPurchase(const QString &account, const Purchase &updatedPurchase, double &newBalance) {
    const QString location = updatedPurchase.location.trimmed();
    const QString item = updatedPurchase.item.trimmed();
    if (!validPurchaseText(updatedPurchase.date, location, item)
        || !validAmount(updatedPurchase.points, false)) {
        return Result::InvalidPurchase;
    }
    const qint64 cents = toCents(updatedPurchase.points);
    double updated = 0.0;
    const Result result = inTransaction([&](QSqlDatabase &db) {
        qint64 oldCents = 0;
        qint64 refunded = 0;
        if (!purchaseTotals(db, account, updatedPurchase.id, oldCents, refunded)) {
            return Result::PurchaseUnavailable;
        }
        if (cents < refunded) {
            return Result::RefundTooLarge;
        }
        QSqlQuery firstRefund(db);
        if (!run(firstRefund, QStringLiteral("SELECT MIN(date) FROM refunds WHERE purchase_id = ?"),
                 {updatedPurchase.id})
            || !firstRefund.next()) {
            return Result::StorageError;
        }
        if (!firstRefund.value(0).isNull()
            && updatedPurchase.date
                > QDate::fromString(firstRefund.value(0).toString(), Qt::ISODate)) {
            return Result::RefundBeforePurchase;
        }
        const Result changed = changeBalance(db, account, oldCents - cents, updated);
        if (changed != Result::Success) {
            return changed;
        }
        QSqlQuery query(db);
        return run(query, QStringLiteral("UPDATE purchases SET date = ?, location = ?, item = ?,"
                                         " points_cents = ? WHERE id = ? AND account_id = ?"),
                   {updatedPurchase.date.toString(Qt::ISODate), location, item, cents,
                    updatedPurchase.id, account})
            ? Result::Success
            : Result::StorageError;
    });
    if (result == Result::Success) {
        newBalance = updated;
    }
    return result;
}

Result removePurchase(const QString &account, const Purchase &purchase, double &newBalance) {
    double updated = 0.0;
    const Result result = inTransaction([&](QSqlDatabase &db) {
        qint64 points = 0;
        qint64 refunded = 0;
        if (!purchaseTotals(db, account, purchase.id, points, refunded)) {
            return Result::PurchaseUnavailable;
        }
        // Refunds are deleted with the purchase (ON DELETE CASCADE), so return only
        // the points that hadn't already come back.
        const Result changed = changeBalance(db, account, points - refunded, updated);
        if (changed != Result::Success) {
            return changed;
        }
        QSqlQuery query(db);
        return run(query, QStringLiteral("DELETE FROM purchases WHERE id = ? AND account_id = ?"),
                   {purchase.id, account})
            ? Result::Success
            : Result::StorageError;
    });
    if (result == Result::Success) {
        newBalance = updated;
    }
    return result;
}

bool loadPurchases(const QString &account, QList<Purchase> &purchases) {
    purchases.clear();
    return reading([&](QSqlDatabase &db) {
        if (!exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE id = ?"), {account})) {
            return false;
        }
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("SELECT date, location, item, points_cents, id FROM purchases"
                                       " WHERE account_id = ? ORDER BY date DESC, seq DESC,"
                                       " rowid DESC"),
                 {account})) {
            return false;
        }
        while (query.next()) {
            purchases.append(Purchase{QDate::fromString(query.value(0).toString(), Qt::ISODate),
                                      query.value(1).toString(), query.value(2).toString(),
                                      fromCents(query.value(3).toLongLong()),
                                      query.value(4).toString()});
        }
        return true;
    });
}

Result recordRefund(const QString &account, const Refund &refund, double &newBalance) {
    const QString note = refund.note.trimmed();
    if (!validRefundText(refund.date, note)) {
        return Result::InvalidPurchase;
    }
    if (!validAmount(refund.points, false)) {
        return Result::InvalidAmount;
    }
    const qint64 cents = toCents(refund.points);
    double updated = 0.0;
    const Result result = inTransaction([&](QSqlDatabase &db) {
        const Result linked = checkRefundLink(db, account, refund.purchaseId, cents, refund.date);
        if (linked != Result::Success) {
            return linked;
        }
        const Result changed = changeBalance(db, account, cents, updated);
        if (changed != Result::Success) {
            return changed;
        }
        QSqlQuery query(db);
        const QVariant purchaseId = refund.purchaseId.isEmpty() ? QVariant() : QVariant(refund.purchaseId);
        return run(query, QStringLiteral("INSERT INTO refunds (id, account_id, purchase_id, date,"
                                         " points_cents, note, seq) VALUES (?, ?, ?, ?, ?, ?, ?)"),
                   {newId(), account, purchaseId, refund.date.toString(Qt::ISODate), cents, note,
                    nextSequence(db)})
            ? Result::Success
            : Result::StorageError;
    });
    if (result == Result::Success) {
        newBalance = updated;
    }
    return result;
}

Result editRefund(const QString &account, const Refund &updatedRefund, double &newBalance) {
    const QString note = updatedRefund.note.trimmed();
    if (!validRefundText(updatedRefund.date, note)) {
        return Result::InvalidPurchase;
    }
    if (!validAmount(updatedRefund.points, false)) {
        return Result::InvalidAmount;
    }
    const qint64 cents = toCents(updatedRefund.points);
    double updated = 0.0;
    const Result result = inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("SELECT points_cents FROM refunds WHERE id = ? AND account_id = ?"),
                 {updatedRefund.id, account})) {
            return Result::StorageError;
        }
        if (!query.next()) {
            return Result::RefundUnavailable;
        }
        const qint64 oldCents = query.value(0).toLongLong();
        const Result linked =
            checkRefundLink(db, account, updatedRefund.purchaseId, cents, updatedRefund.date,
                            updatedRefund.id);
        if (linked != Result::Success) {
            return linked;
        }
        const Result changed = changeBalance(db, account, cents - oldCents, updated);
        if (changed != Result::Success) {
            return changed;
        }
        const QVariant purchaseId =
            updatedRefund.purchaseId.isEmpty() ? QVariant() : QVariant(updatedRefund.purchaseId);
        return run(query, QStringLiteral("UPDATE refunds SET purchase_id = ?, date = ?, points_cents = ?,"
                                         " note = ? WHERE id = ? AND account_id = ?"),
                   {purchaseId, updatedRefund.date.toString(Qt::ISODate), cents, note,
                    updatedRefund.id, account})
            ? Result::Success
            : Result::StorageError;
    });
    if (result == Result::Success) {
        newBalance = updated;
    }
    return result;
}

Result removeRefund(const QString &account, const QString &refundId, double &newBalance) {
    double updated = 0.0;
    const Result result = inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("SELECT points_cents FROM refunds WHERE id = ? AND account_id = ?"),
                 {refundId, account})) {
            return Result::StorageError;
        }
        if (!query.next()) {
            return Result::RefundUnavailable;
        }
        // The refunded points leave the balance again; refused if they've been spent.
        const Result changed = changeBalance(db, account, -query.value(0).toLongLong(), updated);
        if (changed != Result::Success) {
            return changed;
        }
        return run(query, QStringLiteral("DELETE FROM refunds WHERE id = ? AND account_id = ?"),
                   {refundId, account})
            ? Result::Success
            : Result::StorageError;
    });
    if (result == Result::Success) {
        newBalance = updated;
    }
    return result;
}

bool loadHistory(const QString &account, QList<HistoryEntry> &history) {
    history.clear();
    QList<std::pair<qint64, HistoryEntry>> rows;
    const bool loaded = reading([&](QSqlDatabase &db) {
        if (!exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE id = ?"), {account})) {
            return false;
        }
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("SELECT date, location, item, points_cents, id, seq"
                                       " FROM purchases WHERE account_id = ?"), {account})) {
            return false;
        }
        while (query.next()) {
            rows.append({query.value(5).toLongLong(),
                         HistoryEntry{HistoryEntry::Kind::Purchase,
                                      QDate::fromString(query.value(0).toString(), Qt::ISODate),
                                      query.value(1).toString(), query.value(2).toString(),
                                      -fromCents(query.value(3).toLongLong()),
                                      query.value(4).toString()}});
        }
        if (!run(query, QStringLiteral("SELECT date, note, points_cents, id, purchase_id, seq"
                                       " FROM refunds WHERE account_id = ?"), {account})) {
            return false;
        }
        while (query.next()) {
            rows.append({query.value(5).toLongLong(),
                         HistoryEntry{HistoryEntry::Kind::Refund,
                                      QDate::fromString(query.value(0).toString(), Qt::ISODate), {},
                                      query.value(1).toString(),
                                      fromCents(query.value(2).toLongLong()),
                                      query.value(3).toString(), query.value(4).toString()}});
        }
        if (!run(query, QStringLiteral("SELECT date, change_cents, id, seq FROM corrections"
                                       " WHERE account_id = ?"), {account})) {
            return false;
        }
        while (query.next()) {
            rows.append({query.value(3).toLongLong(),
                         HistoryEntry{HistoryEntry::Kind::Correction,
                                      QDate::fromString(query.value(0).toString(), Qt::ISODate), {},
                                      {}, fromCents(query.value(1).toLongLong()),
                                      query.value(2).toString()}});
        }
        return true;
    });
    if (!loaded) {
        return false;
    }
    // Newest date first; within a day, the most recently recorded (highest seq) first.
    std::stable_sort(rows.begin(), rows.end(), [](const auto &left, const auto &right) {
        if (left.second.date != right.second.date) {
            return left.second.date > right.second.date;
        }
        return left.first > right.first;
    });
    for (const auto &row : rows) {
        history.append(row.second);
    }
    return true;
}

bool loadLastUpdated(const QString &account, QDateTime &lastUpdated) {
    lastUpdated = {};
    return reading([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("SELECT last_updated FROM accounts WHERE id = ?"), {account})
            || !query.next()) {
            return false;
        }
        lastUpdated = lastUpdatedFrom(query.value(0));
        return true;
    });
}

// ---- Plan tab ----

bool loadPlanSettings(const QString &account, PlanSettings &settings) {
    settings = {};
    return reading([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("SELECT reserve_cents, habits, alert_over_daily, alert_run_out"
                                       " FROM accounts WHERE id = ?"), {account})
            || !query.next()) {
            return false;
        }
        settings.reserveGoal = fromCents(query.value(0).toLongLong());
        settings.habits = habitsFrom(query.value(1).toString());
        settings.alertOverDailyBudget = query.value(2).toBool();
        settings.alertRunOutBeforeQuarterEnd = query.value(3).toBool();
        return true;
    });
}

Result savePlanSettings(const QString &account, const PlanSettings &settings) {
    const bool validHabits = std::all_of(settings.habits.cbegin(), settings.habits.cend(),
                                         [](int meals) { return meals >= 0 && meals <= MaxMealsPerDay; });
    if (!validAmount(settings.reserveGoal, true) || !validHabits) {
        return Result::InvalidSettings;
    }
    return inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("UPDATE accounts SET reserve_cents = ?, habits = ?,"
                                       " alert_over_daily = ?, alert_run_out = ? WHERE id = ?"),
                 {toCents(settings.reserveGoal), habitsText(settings.habits),
                  settings.alertOverDailyBudget, settings.alertRunOutBeforeQuarterEnd, account})) {
            return Result::StorageError;
        }
        return query.numRowsAffected() == 1 ? Result::Success : Result::StorageError;
    });
}

// ---- Friends ----

bool loadFriends(const QString &account, Friends &friends) {
    friends = {};
    return reading([&](QSqlDatabase &db) {
        if (!exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE id = ?"), {account})) {
            return false;
        }
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("SELECT a.display_name, a.balance_cents, a.friend_id"
                                       " FROM friendships f JOIN accounts a ON a.id = f.friend_account_id"
                                       " WHERE f.account_id = ? ORDER BY a.display_name COLLATE NOCASE"),
                 {account})) {
            return false;
        }
        while (query.next()) {
            friends.accepted.append(FriendSummary{query.value(0).toString(),
                                                  fromCents(query.value(1).toLongLong()),
                                                  query.value(2).toString()});
        }
        if (!run(query, QStringLiteral("SELECT a.display_name, a.friend_id FROM friend_requests r"
                                       " JOIN accounts a ON a.id = r.sender_id"
                                       " WHERE r.recipient_id = ? ORDER BY r.rowid"), {account})) {
            return false;
        }
        while (query.next()) {
            friends.incoming.append(FriendRequest{query.value(0).toString(), query.value(1).toString()});
        }
        if (!run(query, QStringLiteral("SELECT a.display_name, a.friend_id FROM friend_requests r"
                                       " JOIN accounts a ON a.id = r.recipient_id"
                                       " WHERE r.sender_id = ? ORDER BY r.rowid"), {account})) {
            return false;
        }
        while (query.next()) {
            friends.outgoing.append(FriendRequest{query.value(0).toString(), query.value(1).toString()});
        }
        return true;
    });
}

Result sendRequest(const QString &account, const QString &friendId) {
    return inTransaction([&](QSqlDatabase &db) {
        if (!exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE id = ?"), {account})) {
            return Result::StorageError;
        }
        const QString recipient = accountByFriendId(db, friendId);
        if (recipient.isEmpty()) {
            return Result::UnknownFriendId;
        }
        if (recipient == account) {
            return Result::OwnFriendId;
        }
        if (exists(db, QStringLiteral("SELECT 1 FROM friendships WHERE account_id = ? AND friend_account_id = ?"),
                   {account, recipient})) {
            return Result::AlreadyFriends;
        }
        if (exists(db, QStringLiteral("SELECT 1 FROM friend_requests WHERE sender_id = ? AND recipient_id = ?"),
                   {recipient, account})) {
            return Result::IncomingRequestExists;
        }
        if (exists(db, QStringLiteral("SELECT 1 FROM friend_requests WHERE sender_id = ? AND recipient_id = ?"),
                   {account, recipient})) {
            return Result::RequestAlreadySent;
        }
        QSqlQuery query(db);
        return run(query, QStringLiteral("INSERT INTO friend_requests (sender_id, recipient_id) VALUES (?, ?)"),
                   {account, recipient})
            ? Result::Success
            : Result::StorageError;
    });
}

Result respondToRequest(const QString &account, const QString &friendId, bool accept) {
    return inTransaction([&](QSqlDatabase &db) {
        if (!exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE id = ?"), {account})) {
            return Result::StorageError;
        }
        const QString sender = accountByFriendId(db, friendId);
        if (sender.isEmpty() || sender == account) {
            return Result::RequestUnavailable;
        }
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("DELETE FROM friend_requests WHERE sender_id = ? AND recipient_id = ?"),
                 {sender, account})) {
            return Result::StorageError;
        }
        if (query.numRowsAffected() != 1) {
            return Result::RequestUnavailable;
        }
        if (!accept) {
            return Result::Success;
        }
        const QString insert = QStringLiteral(
            "INSERT OR IGNORE INTO friendships (account_id, friend_account_id) VALUES (?, ?)");
        return run(query, insert, {account, sender}) && run(query, insert, {sender, account})
            ? Result::Success
            : Result::StorageError;
    });
}

Result removeFriend(const QString &account, const QString &friendId) {
    return inTransaction([&](QSqlDatabase &db) {
        const QString other = accountByFriendId(db, friendId);
        if (other.isEmpty()) {
            return Result::FriendUnavailable;
        }
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("DELETE FROM friendships WHERE (account_id = ? AND friend_account_id = ?)"
                                       " OR (account_id = ? AND friend_account_id = ?)"),
                 {account, other, other, account})) {
            return Result::StorageError;
        }
        return query.numRowsAffected() > 0 ? Result::Success : Result::FriendUnavailable;
    });
}

// ---- Saved menu sections ----

bool loadSavedSections(const QString &account, QStringList &sections) {
    sections.clear();
    return reading([&](QSqlDatabase &db) {
        if (!exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE id = ?"), {account})) {
            return false;
        }
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("SELECT section FROM saved_sections WHERE account_id = ?"
                                       " ORDER BY rowid"), {account})) {
            return false;
        }
        while (query.next()) {
            sections.append(query.value(0).toString());
        }
        return true;
    });
}

Result saveSection(const QString &account, const QString &section) {
    const QString name = section.trimmed();
    if (name.isEmpty() || name.size() > PurchaseTextLimit) {
        return Result::InvalidSettings;
    }
    return inTransaction([&](QSqlDatabase &db) {
        if (!exists(db, QStringLiteral("SELECT 1 FROM accounts WHERE id = ?"), {account})) {
            return Result::StorageError;
        }
        QSqlQuery query(db);
        return run(query, QStringLiteral("INSERT OR IGNORE INTO saved_sections (account_id, section)"
                                         " VALUES (?, ?)"), {account, name})
            ? Result::Success
            : Result::StorageError;
    });
}

Result removeSection(const QString &account, const QString &section) {
    return inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("DELETE FROM saved_sections WHERE account_id = ? AND section = ?"),
                 {account, section.trimmed()})) {
            return Result::StorageError;
        }
        return query.numRowsAffected() > 0 ? Result::Success : Result::SectionUnavailable;
    });
}

// ---- Email codes ----

namespace {
constexpr int CODE_DIGITS = 6;
const QString VERIFY = QStringLiteral("verify");
const QString RESET = QStringLiteral("reset");
const QString SIGN_IN = QStringLiteral("signin");

QByteArray codeHash(const QString &code, const QByteArray &salt) {
    return QCryptographicHash::hash(salt + code.toUtf8(), QCryptographicHash::Sha256);
}

// Finds an account by email. Returns false if there is none.
bool accountByEmail(QSqlDatabase &db, const QString &email, QString &account, bool &verified) {
    QSqlQuery query(db);
    if (!run(query, QStringLiteral("SELECT id, email_verified FROM accounts WHERE email = ?"),
             {normalizedEmail(email)})
        || !query.next()) {
        return false;
    }
    account = query.value(0).toString();
    verified = query.value(1).toBool();
    return true;
}

// Stores a new code for `purpose`, replacing any earlier one, then emails it. The code
// is removed again if the email can't be sent.
//   verify: for unverified accounts; an already verified one needs nothing (Success).
//   reset, signin: for verified accounts; an unverified one gives EmailNotVerified.
Result issueCode(const QString &email, const QString &purpose) {
    const QString code = QStringLiteral("%1").arg(
        QRandomGenerator::system()->bounded(1000000), CODE_DIGITS, 10, QLatin1Char('0'));
    const QByteArray salt = newSalt();
    QString account;
    bool nothingToDo = false;
    const Result stored = inTransaction([&](QSqlDatabase &db) {
        bool verified = false;
        if (!accountByEmail(db, email, account, verified)) {
            return Result::InvalidEmail;
        }
        if (purpose == VERIFY && verified) {
            nothingToDo = true;
            return Result::Success;
        }
        if (purpose != VERIFY && !verified) {
            return Result::EmailNotVerified;
        }
        const QString expires = QDateTime::currentDateTimeUtc()
                                    .addSecs(LocalAccounts::CodeLifetimeMinutes * 60)
                                    .toString(Qt::ISODateWithMs);
        QSqlQuery query(db);
        return run(query, QStringLiteral("INSERT OR REPLACE INTO email_codes (account_id, purpose, salt,"
                                         " hash, expires_at, attempts) VALUES (?, ?, ?, ?, ?, 0)"),
                   {account, purpose, QString::fromLatin1(salt.toHex()),
                    QString::fromLatin1(codeHash(code, salt).toHex()), expires})
            ? Result::Success
            : Result::StorageError;
    });
    if (stored != Result::Success || nothingToDo) {
        return stored;
    }
    const auto kind = purpose == RESET     ? EmailSender::Purpose::PasswordReset
                      : purpose == SIGN_IN ? EmailSender::Purpose::SignIn
                                           : EmailSender::Purpose::Verification;
    if (EmailSender::send(normalizedEmail(email), code, kind)) {
        return Result::Success;
    }
    inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        return run(query, QStringLiteral("DELETE FROM email_codes WHERE account_id = ? AND purpose = ?"),
                   {account, purpose})
            ? Result::Success
            : Result::StorageError;
    });
    return Result::CodeNotSent;
}

// Checks a code inside a transaction. On success the code is used up; a wrong code
// counts an attempt, and the last allowed attempt cancels the code.
Result checkCode(QSqlDatabase &db, const QString &account, const QString &purpose,
                 const QString &code) {
    QSqlQuery query(db);
    if (!run(query, QStringLiteral("SELECT salt, hash, expires_at, attempts FROM email_codes"
                                   " WHERE account_id = ? AND purpose = ?"), {account, purpose})) {
        return Result::StorageError;
    }
    if (!query.next()) {
        return Result::InvalidCode;
    }
    const QByteArray salt = QByteArray::fromHex(query.value(0).toString().toLatin1());
    const QByteArray hash = QByteArray::fromHex(query.value(1).toString().toLatin1());
    const QDateTime expires = QDateTime::fromString(query.value(2).toString(), Qt::ISODateWithMs);
    const int attempts = query.value(3).toInt();
    if (!expires.isValid() || QDateTime::currentDateTimeUtc() > expires) {
        return Result::CodeExpired;
    }
    if (!hashesMatch(hash, codeHash(code.trimmed(), salt))) {
        const bool lastTry = attempts + 1 >= LocalAccounts::MaxCodeAttempts;
        const bool saved = lastTry
            ? run(query, QStringLiteral("DELETE FROM email_codes WHERE account_id = ? AND purpose = ?"),
                  {account, purpose})
            : run(query, QStringLiteral("UPDATE email_codes SET attempts = attempts + 1"
                                        " WHERE account_id = ? AND purpose = ?"), {account, purpose});
        return saved ? Result::InvalidCode : Result::StorageError;
    }
    return run(query, QStringLiteral("DELETE FROM email_codes WHERE account_id = ? AND purpose = ?"),
               {account, purpose})
        ? Result::Success
        : Result::StorageError;
}

// Runs `onSuccess` in the same transaction as a successful code check. A wrong code's
// attempt count must be saved even though the check failed, so it commits separately.
template <typename OnSuccess>
Result useCode(const QString &email, const QString &purpose, const QString &code,
               OnSuccess onSuccess) {
    Result checked = Result::StorageError;
    const Result result = inTransaction([&](QSqlDatabase &db) {
        QString account;
        bool verified = false;
        if (!accountByEmail(db, email, account, verified)) {
            checked = Result::InvalidEmail;
            return Result::Success;
        }
        checked = checkCode(db, account, purpose, code);
        if (checked != Result::Success) {
            return Result::Success; // Keep the attempt count.
        }
        return onSuccess(db, account);
    });
    return result == Result::Success ? checked : result;
}
}

Result sendVerificationCode(const QString &email) {
    return issueCode(email, VERIFY);
}

Result verifyEmail(const QString &email, const QString &code) {
    return useCode(email, VERIFY, code, [](QSqlDatabase &db, const QString &account) {
        QSqlQuery query(db);
        return run(query, QStringLiteral("UPDATE accounts SET email_verified = 1 WHERE id = ?"), {account})
            ? Result::Success
            : Result::StorageError;
    });
}

void cancelSignUp(const QString &email) {
    inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        return run(query, QStringLiteral("DELETE FROM accounts WHERE email = ? AND email_verified = 0"),
                   {normalizedEmail(email)})
            ? Result::Success
            : Result::StorageError;
    });
}

Result sendPasswordResetCode(const QString &email) {
    return issueCode(email, RESET);
}

Result resetPassword(const QString &email, const QString &code, const QString &newPassword) {
    if (newPassword.size() < MinPasswordLength) {
        return Result::ShortPassword;
    }
    // Hash before the transaction so the database isn't locked while it runs.
    const QByteArray salt = newSalt();
    const QByteArray hash = deriveHash(newPassword, salt);
    return useCode(email, RESET, code, [&](QSqlDatabase &db, const QString &account) {
        QSqlQuery query(db);
        return run(query, QStringLiteral("UPDATE accounts SET salt = ?, hash = ? WHERE id = ?"),
                   {QString::fromLatin1(salt.toHex()), QString::fromLatin1(hash.toHex()), account})
            ? Result::Success
            : Result::StorageError;
    });
}

// ---- Two-step sign-in ----

Result setTwoStepSignIn(const QString &account, bool enabled) {
    return inTransaction([&](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!run(query, QStringLiteral("UPDATE accounts SET two_step = ? WHERE id = ?"),
                 {enabled ? 1 : 0, account})) {
            return Result::StorageError;
        }
        return query.numRowsAffected() == 1 ? Result::Success : Result::StorageError;
    });
}

Result sendSignInCode(const QString &email, const QString &password) {
    PasswordCheck checked;
    const Result result = checkPassword(email, password, checked);
    return result == Result::Success ? issueCode(email, SIGN_IN) : result;
}

Result finishSignIn(const QString &email, const QString &password, const QString &code,
                    Profile &profile) {
    // The password is checked first so a wrong one doesn't use up a code attempt.
    PasswordCheck checked;
    const Result result = checkPassword(email, password, checked);
    if (result != Result::Success) {
        return result;
    }
    return useCode(email, SIGN_IN, code, [&](QSqlDatabase &db, const QString &account) {
        return selectProfile(db, account, profile) ? Result::Success : Result::StorageError;
    });
}
}
