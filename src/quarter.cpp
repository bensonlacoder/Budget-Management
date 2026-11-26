#include "quarter.h"

namespace Quarter {

int Term::length() const {
    return static_cast<int>(start.daysTo(end)) + 1;
}

const QList<Term> &calendar() {
    // SCU Undergraduate Bulletin 2026-27. Spring's start differs by one day between
    // SCU sources; March 30 is used.
    static const QList<Term> terms = {
        {QStringLiteral("Fall 2026"), QDate(2026, 9, 21), QDate(2026, 12, 11)},
        {QStringLiteral("Winter 2027"), QDate(2027, 1, 4), QDate(2027, 3, 19)},
        {QStringLiteral("Spring 2027"), QDate(2027, 3, 30), QDate(2027, 6, 10)},
    };
    return terms;
}

Status statusOn(const QDate &today) {
    for (const Term &term : calendar()) {
        if (today < term.start) {
            return {Phase::BeforeQuarter, term, term.length()};
        }
        if (today <= term.end) {
            return {Phase::InQuarter, term, static_cast<int>(today.daysTo(term.end)) + 1};
        }
    }
    return {Phase::CalendarEnded, {}, 0};
}

int daysLeft(const QDate &today, bool corrected, int savedDays) {
    const Status status = statusOn(today);
    if (corrected || status.phase == Phase::CalendarEnded) {
        return savedDays;
    }
    return status.daysLeft;
}

} // namespace Quarter
