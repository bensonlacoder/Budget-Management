#ifndef LOCALACCOUNTS_H
#define LOCALACCOUNTS_H

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

#include <array>

// All saved account data goes through these functions; pages never open storage
// directly. Data lives in a SQLite database in the app's local data folder.
//
// `account` is the opaque ID in Profile::accountId, returned by create() and signIn().
// It never changes, even when the user changes their email.
namespace LocalAccounts {
enum class Result {
    Success,
    InvalidEmail,        // Not a valid email address.
    EmailTaken,          // Another account already uses this email.
    InvalidDisplayName,  // Empty, or longer than MaxDisplayNameLength.
    InvalidDiningPlan,
    ShortPassword,       // Shorter than MinPasswordLength.
    IncorrectCredentials,
    UnknownFriendId,
    OwnFriendId,
    AlreadyFriends,
    RequestAlreadySent,
    IncomingRequestExists,
    RequestUnavailable,
    InvalidPurchase,     // Bad date (invalid or after today), location, item or note.
    InsufficientPoints,  // The change would make the balance negative.
    PurchaseUnavailable,
    InvalidAmount,       // Not greater than 0 (or negative for Set), non-finite, or too large.
    InvalidSettings,     // Reserve goal or habits out of range.
    RefundTooLarge,      // More than the unrefunded part of the purchase.
    RefundBeforePurchase, // A linked refund dated before its purchase.
    RefundUnavailable,
    FriendUnavailable,
    EmailNotVerified,    // Sign-in before the sign-up code was entered.
    InvalidCode,         // Wrong code, no code requested, or too many wrong tries.
    CodeExpired,         // Older than CodeLifetimeMinutes; request a new one.
    CodeNotSent,         // The email sender could not be reached.
    CodeRequired,        // Right password, but two-step sign-in also needs an emailed code.
    StorageError        // The database could not be opened, read or written.
};

// Days left in the budgeting period, including today. Matches Budget's 0 to 90 range.
// Used until the quarter calendar (T03) replaces it.
constexpr int DefaultRemainingDays = 77;
constexpr int MaxRemainingDays = 90;
constexpr int MinPasswordLength = 8;
constexpr int MaxDisplayNameLength = 32;
constexpr int MaxEmailLength = 254;

struct Profile {
    QString displayName;
    QString friendId;
    double balance = 0.0;
    int remainingDays = DefaultRemainingDays;
    QString email;
    QString diningPlanId;
    // True while a manual days-left correction overrides the quarter calendar (T02, T03).
    bool daysCorrected = false;
    // When the balance or history last changed successfully (T12). Invalid until then.
    QDateTime lastUpdated;
    // When true, signing in also needs a code emailed to the account.
    bool twoStepSignIn = false;
    // Pass this as `account` to every other function.
    QString accountId;
};

struct DiningPlan {
    QString id;
    QString name;
    int startingPoints; // Per quarter.
};

struct FriendSummary {
    QString displayName;
    double balance;
    QString friendId = {};
};

struct FriendRequest {
    QString displayName;
    QString friendId;
};

struct Friends {
    QList<FriendSummary> accepted;
    QList<FriendRequest> incoming;
    QList<FriendRequest> outgoing; // Requests this account sent that are still pending.
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

// A full or partial refund. purchaseId is empty for a refund not linked to a purchase.
struct Refund {
    QDate date;
    double points; // Points returned to the balance; greater than 0.
    QString purchaseId = {};
    QString note = {};
    QString id = {};
};

// How a balance correction changes the real balance.
enum class CorrectionMode { Set, Add, Subtract };

// One row of the combined history. balanceChange is negative when points left the
// balance: purchases, and corrections that lowered it ("Unlogged" spending).
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

// Meals eaten on campus for each weekday, Monday first. 0 means not on campus.
using DiningHabits = std::array<int, 7>;
constexpr int MaxMealsPerDay = 10;

// Settings for the Plan tab, saved together.
struct PlanSettings {
    double reserveGoal = 0.0;
    DiningHabits habits = {};
    bool alertOverDailyBudget = false;
    bool alertRunOutBeforeQuarterEnd = false;
};

// ---- Accounts ----
const QList<DiningPlan> &diningPlans();
// Creates an unverified account; call sendVerificationCode() next. The email is
// trimmed and matched without regard to case. An unverified account with the same
// email is replaced, so someone whose code never arrived can sign up again.
Result create(const QString &email, const QString &password, const QString &displayName,
              const QString &diningPlanId, Profile &profile);
// EmailNotVerified if the password is right but the email hasn't been verified yet.
// CodeRequired if two-step sign-in is on: call sendSignInCode(), then finishSignIn().
Result signIn(const QString &email, const QString &password, Profile &profile);
bool loadProfile(const QString &account, Profile &profile);
Result updateProfile(const QString &account, const QString &displayName, const QString &email);
// IncorrectCredentials if currentPassword is wrong; ShortPassword if the new one is too short.
Result changePassword(const QString &account, const QString &currentPassword,
                      const QString &newPassword);
// Closes the database so its file can be moved or deleted (tests, shutdown).
// The next call to any function opens it again.
void closeStorage();

// ---- Balance and days left ----
// Saves `next` only if the saved balance still equals `previous` (another window may
// have changed it). Returns false if refused or not saved.
bool saveBalance(const QString &account, double previous, double next);
// Records a "Balance correction" history entry. Subtracting more than the balance gives
// InsufficientPoints. Setting the balance to its current value saves nothing.
Result correctBalance(const QString &account, CorrectionMode mode, double amount,
                      double &newBalance);
// Days left on `today` for a count of `savedDays` saved on `savedOn`: one fewer for each
// day that has passed, never below 0. An invalid savedOn counts as no days passed.
int remainingDaysOn(int savedDays, const QDate &savedOn, const QDate &today);
// Saves a manual days-left correction for `countedOn`; it counts down from that date.
// Returns false if days is outside 0 through MaxRemainingDays or cannot be saved.
bool saveRemainingDays(const QString &account, int days,
                       const QDate &countedOn = QDate::currentDate());
// Removes the manual correction so the quarter calendar is used again. Until T03 the
// count returns to DefaultRemainingDays from today.
Result clearDaysCorrection(const QString &account);

// ---- Purchases, refunds and history ----
Result recordPurchase(const QString &account, const Purchase &purchase, double &newBalance);
// Finds the purchase by updated.id and adjusts the balance by the difference. The new
// points can't be less than what has already been refunded (RefundTooLarge), and the
// new date can't be after one of its refunds (RefundBeforePurchase).
Result editPurchase(const QString &account, const Purchase &updated, double &newBalance);
// Finds the purchase by purchase.id. Its refunds are removed with it, and the balance
// gets back the points that had not already been refunded.
Result removePurchase(const QString &account, const Purchase &purchase, double &newBalance);
// Newest first.
bool loadPurchases(const QString &account, QList<Purchase> &purchases);
Result recordRefund(const QString &account, const Refund &refund, double &newBalance);
// Finds the refund by updated.id.
Result editRefund(const QString &account, const Refund &updated, double &newBalance);
Result removeRefund(const QString &account, const QString &refundId, double &newBalance);
// Purchases, refunds and balance corrections, newest first.
bool loadHistory(const QString &account, QList<HistoryEntry> &history);
// Last successful change to the balance or history, in local time. Returns true with
// an invalid QDateTime if nothing has changed since the account was created, and false
// if the account doesn't exist or the database can't be read.
bool loadLastUpdated(const QString &account, QDateTime &lastUpdated);

// ---- Plan tab ----
// An invalid reserve goal or meal count gives InvalidSettings and nothing is saved.
// Whether the reserve may exceed the balance is the Plan tab's rule (T06).
bool loadPlanSettings(const QString &account, PlanSettings &settings);
Result savePlanSettings(const QString &account, const PlanSettings &settings);

// ---- Friends ----
bool loadFriends(const QString &account, Friends &friends);
// Friend IDs are matched regardless of case and surrounding spaces.
Result sendRequest(const QString &account, const QString &friendId);
Result respondToRequest(const QString &account, const QString &friendId, bool accept);
// Ends the friendship for both accounts.
Result removeFriend(const QString &account, const QString &friendId);

// ---- Email codes ----
// Codes are 6 digits, sent with EmailSender, and stored only as a salted hash.
// Sending a new code replaces the previous one. After MaxCodeAttempts wrong tries the
// code is cancelled and a new one must be requested.
constexpr int CodeLifetimeMinutes = 15;
constexpr int MaxCodeAttempts = 5;

// For an unverified account. InvalidEmail if no account uses the email; Success
// without sending if it is already verified; CodeNotSent if sending failed.
Result sendVerificationCode(const QString &email);
// Marks the email verified so the account can sign in.
Result verifyEmail(const QString &email, const QString &code);
// Deletes the account if its email was never verified, for example when the first
// code couldn't be sent. Does nothing to a verified account.
void cancelSignUp(const QString &email);
// For a verified account. InvalidEmail if no account uses the email.
Result sendPasswordResetCode(const QString &email);
// ShortPassword if the new password is too short (checked before the code).
Result resetPassword(const QString &email, const QString &code, const QString &newPassword);

// ---- Two-step sign-in ----
// Turns on or off asking for an emailed code after the password at every sign-in.
Result setTwoStepSignIn(const QString &account, bool enabled);
// Emails a sign-in code. The password is checked first (IncorrectCredentials), so the
// email alone can't be used to send codes.
Result sendSignInCode(const QString &email, const QString &password);
// Signs in with the password and the emailed code. The code works once.
Result finishSignIn(const QString &email, const QString &password, const QString &code,
                    Profile &profile);
}

#endif // LOCALACCOUNTS_H
