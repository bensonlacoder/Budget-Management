#pragma once

#include <QDate>
#include <QList>
#include <QString>

// SCU's undergraduate quarter calendar, used to work out the days left in the
// budgeting period. Each quarter runs from the first day of classes through the last
// day of finals. See SPECIFICATIONS.md (SP-04) for the dates and rules.
namespace Quarter {

struct Term {
    QString name;
    QDate start; // First day of classes.
    QDate end;   // Last day of finals.

    // Days from start through end, counting both.
    int length() const;
};

// Quarters in date order. Add the next academic year here when SCU publishes it.
const QList<Term> &calendar();

enum class Phase {
    InQuarter,     // Today is between a quarter's start and end, inclusive.
    BeforeQuarter, // A break: the next quarter has not started yet.
    CalendarEnded  // Today is after the last listed quarter; the calendar needs updating.
};

struct Status {
    Phase phase;
    Term term;    // The current or next quarter. Empty when the calendar has ended.
    int daysLeft; // Days left counting today, or the next quarter's full length. 0 if ended.
};

// Where `today` falls in the calendar.
Status statusOn(const QDate &today);

// The days left to budget for. A manual correction (`corrected`, with the count
// already counted down to `today`) takes priority over the calendar. When the
// calendar has ended, the saved count is used even if it wasn't corrected.
int daysLeft(const QDate &today, bool corrected, int savedDays);

} // namespace Quarter
