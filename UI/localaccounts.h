#ifndef LOCALACCOUNTS_H
#define LOCALACCOUNTS_H

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

#include <array>

// All saved account data goes through these functions. Pages never open storage
// directly. `account` identifies the signed-in account (its email once ticket T01
// lands). Functions marked "Stub until T01" compile and are safe to call, but return
// Result::StorageError or false until storage is implemented.
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
    StorageError,
    // Added for the remaining tickets.
    InvalidEmail,        // Not a valid email address.
    EmailTaken,          // Another account already uses this email.
    EmailNotVerified,    // Sign-in before the sign-up code was entered (T15).
    InvalidCode,         // Wrong verification or reset code (T15).
    CodeExpired,         // Code older than 15 minutes (T15).
    CodeNotSent,         // The email sender could not be reached (T15).
    InvalidAmount,       // Negative, non-finite or too large an amount.
    InvalidSettings,     // Reserve goal, habits or alert settings out of range.
    RefundTooLarge,      // More than the unrefunded part of the purchase.
    RefundUnavailable,   // The refund no longer exists.
    FriendUnavailable,   // Not (or no longer) a friend.
    SectionUnavailable   // The saved menu section no longer exists.
};

// Days left in the budgeting period, including today. Matches Budget's 0 to 90 range.
constexpr int DefaultRemainingDays = 77;
constexpr int MaxRemainingDays = 90;

struct Profile {
    QString displayName;
    QString friendId;
    double balance = 0.0;
    int remainingDays = DefaultRemainingDays;
    // Filled in from T01 on. Empty or invalid until then.
    QString email;
    QString diningPlanId;
    // True while a manual days-left correction overrides the quarter calendar (T02, T03).
    bool daysCorrected = false;
    // When the balance or history last changed successfully (T12).
    QDateTime lastUpdated;
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
    QString friendId = {}; // Needed to remove the friend (T05).
};

struct FriendRequest {
    QString displayName;
    QString friendId;
};

struct Friends {
    QList<FriendSummary> accepted;
    QList<FriendRequest> incoming;
    QList<FriendRequest> outgoing; // Requests this account sent that are still pending (T05).
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

// A full or partial refund (T09). purchaseId is empty for a refund not linked to a purchase.
struct Refund {
    QDate date;
    double points; // Points returned to the balance; greater than 0.
    QString purchaseId = {};
    QString note = {};
    QString id = {};
};

// How a balance correction changes the real balance (T02).
enum class CorrectionMode { Set, Add, Subtract };

// One row of the combined history (T08, T10). balanceChange is negative when points
// left the balance: purchases, and corrections that lowered it ("Unlogged" spending).
struct HistoryEntry {
    enum class Kind { Purchase, Refund, Correction };
    Kind kind;
    QDate date;
    QString location; // Empty for refunds and corrections.
    QString item;     // Item for purchases, note for refunds.
    double balanceChange;
    QString id;
    QString purchaseId = {}; // For refunds linked to a purchase.
};

// Meals eaten on campus for each weekday, Monday first (T07). 0 means not on campus.
using DiningHabits = std::array<int, 7>;
constexpr int MaxMealsPerDay = 10;

// Settings for the Plan tab (T06, T07, T13).
struct PlanSettings {
    double reserveGoal = 0.0;
    DiningHabits habits = {};
    bool alertOverDailyBudget = false;
    bool alertRunOutBeforeQuarterEnd = false;
};

const QList<DiningPlan> &diningPlans();
Result create(const QString &username, const QString &password, const QString &displayName,
              const QString &diningPlanId, Profile &profile);
Result signIn(const QString &username, const QString &password, Profile &profile);
bool saveBalance(const QString &username, double previous, double next);
// Days left on `today` for a count of `savedDays` saved on `savedOn`: one fewer for each
// day that has passed, never below 0. An invalid savedOn counts as no days passed.
int remainingDaysOn(int savedDays, const QDate &savedOn, const QDate &today);
// Saves days as the count for `countedOn`; sign-in counts down from that date.
// Returns false if days is outside 0 through MaxRemainingDays or cannot be saved.
bool saveRemainingDays(const QString &username, int days,
                       const QDate &countedOn = QDate::currentDate());
Result recordPurchase(const QString &username, const Purchase &purchase, double &newBalance);
Result removePurchase(const QString &username, const Purchase &purchase, double &newBalance);
bool loadPurchases(const QString &username, QList<Purchase> &purchases);
bool loadFriends(const QString &username, Friends &friends);
Result sendRequest(const QString &username, const QString &friendId);
Result respondToRequest(const QString &username, const QString &friendId, bool accept);

// ---- Stubs until T01. Signatures are agreed; build against them now. ----

// Profile page (T02).
bool loadProfile(const QString &account, Profile &profile);
// Rejects an empty or too-long display name, an invalid email, or an email in use.
Result updateProfile(const QString &account, const QString &displayName, const QString &email);
// IncorrectCredentials if currentPassword is wrong; ShortPassword if the new one is too short.
Result changePassword(const QString &account, const QString &currentPassword,
                      const QString &newPassword);
// Records a "Balance correction" history entry. Subtracting more than the balance gives
// InsufficientPoints; a negative or non-finite amount gives InvalidAmount.
Result correctBalance(const QString &account, CorrectionMode mode, double amount,
                      double &newBalance);
// Removes a manual days-left correction so the quarter calendar is used again (T02, T03).
// saveRemainingDays above records a correction.
Result clearDaysCorrection(const QString &account);

// Purchases tab: editing, refunds and combined history (T08, T09).
// Finds the purchase by updated.id and adjusts the balance by the difference.
Result editPurchase(const QString &account, const Purchase &updated, double &newBalance);
Result recordRefund(const QString &account, const Refund &refund, double &newBalance);
// Finds the refund by updated.id.
Result editRefund(const QString &account, const Refund &updated, double &newBalance);
Result removeRefund(const QString &account, const QString &refundId, double &newBalance);
// Purchases, refunds and balance corrections, newest first.
bool loadHistory(const QString &account, QList<HistoryEntry> &history);

// Plan tab (T06, T07, T13). Settings are saved together; an invalid reserve goal,
// meal count or habit gives InvalidSettings and nothing is saved.
bool loadPlanSettings(const QString &account, PlanSettings &settings);
Result savePlanSettings(const QString &account, const PlanSettings &settings);

// Friends (T05). Ends the friendship for both accounts.
Result removeFriend(const QString &account, const QString &friendId);

// Saved menu sections (T14), for example "Marketplace / Grill".
bool loadSavedSections(const QString &account, QStringList &sections);
Result saveSection(const QString &account, const QString &section);
Result removeSection(const QString &account, const QString &section);

// Last successful change to the balance or history (T12). Invalid if never changed.
bool loadLastUpdated(const QString &account, QDateTime &lastUpdated);

// Email verification and password reset (T15). Codes expire after 15 minutes.
Result sendVerificationCode(const QString &email);
Result verifyEmail(const QString &email, const QString &code);
Result sendPasswordResetCode(const QString &email);
Result resetPassword(const QString &email, const QString &code, const QString &newPassword);
}

#endif // LOCALACCOUNTS_H
