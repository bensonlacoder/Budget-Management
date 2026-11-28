// Quarter calendar happy case tests (T03; SPECIFICATIONS.md SP-05)
// Original test contributors: Benson, Nathan Gu
// Covers Quarter::Term::length, Quarter::calendar, Quarter::statusOn and
// Quarter::daysLeft.

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

// Author: Nathan Gu
TEST_CASE("The calendar lists SCU's 2026-27 dates in order") {
    const auto &terms = Quarter::calendar();
    REQUIRE(terms.size() == 3);
    CHECK(terms[0].name == "Fall 2026");
    CHECK(terms[0].start == QDate(2026, 9, 21));
    CHECK(terms[0].end == QDate(2026, 12, 11));
    CHECK(terms[1].name == "Winter 2027");
    CHECK(terms[1].start == QDate(2027, 1, 4));
    CHECK(terms[1].end == QDate(2027, 3, 19));
    CHECK(terms[2].name == "Spring 2027");
    CHECK(terms[2].start == QDate(2027, 3, 30));
    CHECK(terms[2].end == QDate(2027, 6, 10));
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
}

// Author: Benson
TEST_CASE("A manual correction takes priority over the calendar") {
    const QDate inFall(2026, 11, 22);
    CHECK(Quarter::daysLeft(inFall, false, 40) == 20);
    CHECK(Quarter::daysLeft(inFall, true, 40) == 40);
}

// Author: Nathan Gu
TEST_CASE("Without a correction, days left between quarters are the next quarter's length") {
    CHECK(Quarter::daysLeft(QDate(2027, 3, 25), false, 12) == 73); // Spring break.
}
