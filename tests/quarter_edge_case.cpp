// Quarter calendar error and edge case tests (T03; SPECIFICATIONS.md SP-05)
// Original test contributors: Benson, Nathan Gu

#include "doctest.h"
#include "quarter.h"

using Quarter::Phase;

// Author: Nathan Gu
TEST_CASE("A one-day term has a length of 1") {
    const Quarter::Term oneDay{"Test", QDate(2026, 1, 1), QDate(2026, 1, 1)};
    CHECK(oneDay.length() == 1);
}

// Author: Nathan Gu
TEST_CASE("The quarters don't overlap and each ends after it starts") {
    const auto &terms = Quarter::calendar();
    for (int i = 0; i < terms.size(); ++i) {
        CAPTURE(terms[i].name.toStdString());
        CHECK(terms[i].start <= terms[i].end);
        if (i > 0) {
            CHECK(terms[i - 1].end < terms[i].start);
        }
    }
}

// Author: Benson
TEST_CASE("The day after finals is already a break") {
    const auto status = Quarter::statusOn(QDate(2026, 12, 12));
    CHECK(status.phase == Phase::BeforeQuarter);
    CHECK(status.term.name == "Winter 2027");
}

// Author: Nathan Gu
TEST_CASE("The day before classes is still a break, and spring break works too") {
    CHECK(Quarter::statusOn(QDate(2026, 9, 20)).phase == Phase::BeforeQuarter);
    const auto springBreak = Quarter::statusOn(QDate(2027, 3, 20));
    CHECK(springBreak.phase == Phase::BeforeQuarter);
    CHECK(springBreak.term.name == "Spring 2027");
    CHECK(springBreak.daysLeft == 73);
    CHECK(Quarter::statusOn(QDate(2027, 3, 29)).phase == Phase::BeforeQuarter);
    CHECK(Quarter::statusOn(QDate(2027, 3, 30)).phase == Phase::InQuarter);
}

// Author: Benson
TEST_CASE("Before the first listed quarter, Fall is next") {
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
TEST_CASE("A correction of 0 is kept, and the saved count is the fallback") {
    const QDate inFall(2026, 11, 22);
    CHECK(Quarter::daysLeft(inFall, true, 0) == 0);

    const QDate afterCalendar(2027, 7, 1);
    CHECK(Quarter::daysLeft(afterCalendar, false, 12) == 12);
    CHECK(Quarter::daysLeft(afterCalendar, true, 12) == 12);
}

// Author: Nathan Gu
TEST_CASE("A correction also takes priority between quarters") {
    CHECK(Quarter::daysLeft(QDate(2026, 12, 20), true, 5) == 5);
}
