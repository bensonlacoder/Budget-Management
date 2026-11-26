// Quarter calendar tests (T03, SP-04).

#include "doctest.h"
#include "quarter.h"

using Quarter::Phase;

// Author: Benson
TEST_CASE("The 2026-27 quarters have the expected lengths") {
    const auto &terms = Quarter::calendar();
    REQUIRE(terms.size() == 3);
    CHECK(terms[0].length() == 82); // Fall 2026
    CHECK(terms[1].length() == 75); // Winter 2027
    CHECK(terms[2].length() == 73); // Spring 2027
}

// Author: Benson
TEST_CASE("Days left during a quarter count today") {
    const auto status = Quarter::statusOn(QDate(2026, 11, 22));
    CHECK(status.phase == Phase::InQuarter);
    CHECK(status.term.name == "Fall 2026");
    CHECK(status.daysLeft == 20);
}

// Author: Benson
TEST_CASE("The first and last day of a quarter") {
    const auto first = Quarter::statusOn(QDate(2027, 1, 4));
    CHECK(first.phase == Phase::InQuarter);
    CHECK(first.term.name == "Winter 2027");
    CHECK(first.daysLeft == 75);

    const auto last = Quarter::statusOn(QDate(2027, 3, 19));
    CHECK(last.phase == Phase::InQuarter);
    CHECK(last.term.name == "Winter 2027");
    CHECK(last.daysLeft == 1);
}

// Author: Benson
TEST_CASE("Between quarters, the next quarter's full length and start are shown") {
    const auto winterBreak = Quarter::statusOn(QDate(2026, 12, 20));
    CHECK(winterBreak.phase == Phase::BeforeQuarter);
    CHECK(winterBreak.term.name == "Winter 2027");
    CHECK(winterBreak.term.start == QDate(2027, 1, 4));
    CHECK(winterBreak.daysLeft == 75);

    // The day after finals is already a break.
    CHECK(Quarter::statusOn(QDate(2026, 12, 12)).phase == Phase::BeforeQuarter);

    // Before the first listed quarter, Fall is next.
    const auto summer = Quarter::statusOn(QDate(2026, 8, 1));
    CHECK(summer.phase == Phase::BeforeQuarter);
    CHECK(summer.term.name == "Fall 2026");
    CHECK(summer.daysLeft == 82);
}

// Author: Benson
TEST_CASE("After the last listed quarter, the calendar needs updating") {
    CHECK(Quarter::statusOn(QDate(2027, 6, 10)).phase == Phase::InQuarter);
    const auto ended = Quarter::statusOn(QDate(2027, 6, 11));
    CHECK(ended.phase == Phase::CalendarEnded);
    CHECK(ended.term.name.isEmpty());
    CHECK(ended.daysLeft == 0);
}

// Author: Benson
TEST_CASE("A manual correction takes priority, and the saved count is the fallback") {
    const QDate inFall(2026, 11, 22);
    CHECK(Quarter::daysLeft(inFall, false, 40) == 20);
    CHECK(Quarter::daysLeft(inFall, true, 40) == 40);
    CHECK(Quarter::daysLeft(inFall, true, 0) == 0);

    const QDate afterCalendar(2027, 7, 1);
    CHECK(Quarter::daysLeft(afterCalendar, false, 12) == 12);
    CHECK(Quarter::daysLeft(afterCalendar, true, 12) == 12);
}
