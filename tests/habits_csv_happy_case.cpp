// Dining habits CSV upload happy case tests (T07; SPECIFICATIONS.md SP-08)
// Original test contributors: Nathan Gu
// Covers HabitsCsv::parse.

#include "doctest.h"
#include "habitscsv.h"

namespace {
const Budget::DiningHabits UNCHANGED = {9, 9, 9, 9, 9, 9, 9};

// Parses `text` into habits that start as UNCHANGED.
bool parse(const QString &text, Budget::DiningHabits &habits, QString &error) {
    habits = UNCHANGED;
    error.clear();
    return HabitsCsv::parse(text, habits, error);
}
}

TEST_CASE("SP-08: a CSV of days and meals becomes the weekly habits") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("day,meals\nMonday,2\nWednesday,3\nFriday,1\n", habits, error));
    CHECK(habits == Budget::DiningHabits{2, 0, 3, 0, 1, 0, 0});
    CHECK(error.isEmpty());
}

TEST_CASE("Every day of the week can be listed, in any order") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("Sunday,7\nSaturday,6\nFriday,5\nThursday,4\nWednesday,3\nTuesday,2\nMonday,1",
                  habits, error));
    CHECK(habits == Budget::DiningHabits{1, 2, 3, 4, 5, 6, 7});
}

TEST_CASE("The header line is optional") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("Tuesday,2", habits, error));
    CHECK(habits == Budget::DiningHabits{0, 2, 0, 0, 0, 0, 0});
}

TEST_CASE("Days may be three-letter abbreviations in any case") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("MON,1\ntue,2\nWeD,3\nthu,4\nFRI,5\nsat,6\nSun,7", habits, error));
    CHECK(habits == Budget::DiningHabits{1, 2, 3, 4, 5, 6, 7});
}

TEST_CASE("Days not listed have 0 meals, and 0 can be written out") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("day,meals\nMonday,0\nThursday,2", habits, error));
    CHECK(habits == Budget::DiningHabits{0, 0, 0, 2, 0, 0, 0});
}

TEST_CASE("Spaces, blank lines and Windows line endings don't matter") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("  Day , Meals \r\n\r\n  Monday , 2 \r\n\r\nFriday,1\r\n", habits, error));
    CHECK(habits == Budget::DiningHabits{2, 0, 0, 0, 1, 0, 0});
}

TEST_CASE("A byte order mark at the start of the file, as Excel writes, is ignored") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse(QString(QChar(0xFEFF)) + "day,meals\nThursday,1", habits, error));
    CHECK(habits == Budget::DiningHabits{0, 0, 0, 1, 0, 0, 0});
}

TEST_CASE("The most meals allowed in a day is 10") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("Saturday,10", habits, error));
    CHECK(habits == Budget::DiningHabits{0, 0, 0, 0, 0, 10, 0});
}

TEST_CASE("Files up to 10 KB are accepted") {
    CHECK(HabitsCsv::MaxFileBytes == 10 * 1024);
}
