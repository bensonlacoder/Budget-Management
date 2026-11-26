// Dining habits CSV reader tests (T07, SP-08).

#include "doctest.h"
#include "habitscsv.h"

namespace {
const Budget::DiningHabits UNCHANGED = {9, 9, 9, 9, 9, 9, 9};

// Parses `text` into habits that start as UNCHANGED, so a failure can be seen to
// leave them alone.
bool parse(const QString &text, Budget::DiningHabits &habits, QString &error) {
    habits = UNCHANGED;
    error.clear();
    return HabitsCsv::parse(text, habits, error);
}
}

// Author: Nathan Gu
TEST_CASE("SP-08: a CSV of days and meals becomes the weekly habits") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("day,meals\nMonday,2\nWednesday,3\nFri,1\n", habits, error));
    CHECK(habits == Budget::DiningHabits{2, 0, 3, 0, 1, 0, 0});
    CHECK(error.isEmpty());
}

// Author: Nathan Gu
TEST_CASE("SP-08: the CSV header is optional, and case, spaces and line endings don't matter") {
    Budget::DiningHabits habits;
    QString error;
    REQUIRE(parse("  SUNDAY , 10 \r\n\r\ntue,0\r\nsat,4", habits, error));
    CHECK(habits == Budget::DiningHabits{0, 0, 0, 0, 0, 4, 10});

    // Excel adds a byte order mark at the start of UTF-8 files.
    REQUIRE(parse(QString(QChar(0xFEFF)) + "Day,Meals\nThursday,1", habits, error));
    CHECK(habits == Budget::DiningHabits{0, 0, 0, 1, 0, 0, 0});
}

// Author: Nathan Gu
TEST_CASE("SP-08: a bad CSV names the first bad line and changes nothing") {
    Budget::DiningHabits habits;
    QString error;

    CHECK_FALSE(parse("day,meals\nMonday,2\nFunday,3\n", habits, error));
    CHECK(error == "Line 3: \"Funday\" isn't a day of the week.");
    CHECK(habits == UNCHANGED);

    CHECK_FALSE(parse("Monday,11", habits, error));
    CHECK(error == "Line 1: meals must be a whole number from 0 to 10.");
    CHECK_FALSE(parse("Monday,-1", habits, error));
    CHECK_FALSE(parse("Monday,2.5", habits, error));
    CHECK_FALSE(parse("Monday,two", habits, error));
    CHECK(error == "Line 1: meals must be a whole number from 0 to 10.");

    CHECK_FALSE(parse("Monday,2\nmon,3", habits, error));
    CHECK(error == "Line 2: mon is listed more than once.");

    CHECK_FALSE(parse("Monday;2", habits, error));
    CHECK(error == "Line 1: expected a day and a number of meals, like \"Monday,2\".");
    CHECK_FALSE(parse("Monday,2,extra", habits, error));

    // The header only counts as one before the first day.
    CHECK_FALSE(parse("Monday,2\nday,meals", habits, error));
    CHECK(error == "Line 2: \"day\" isn't a day of the week.");
    CHECK(habits == UNCHANGED);
}

// Author: Nathan Gu
TEST_CASE("SP-08: a CSV with no days is rejected") {
    Budget::DiningHabits habits;
    QString error;
    CHECK_FALSE(parse("", habits, error));
    CHECK(error == "The file doesn't list any days.");
    CHECK_FALSE(parse("day,meals\n\n", habits, error));
    CHECK(habits == UNCHANGED);
}
