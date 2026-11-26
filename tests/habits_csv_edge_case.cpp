// Dining habits CSV upload error and edge case tests (T07; SPECIFICATIONS.md SP-08)
// Original test contributors: Nathan Gu
// A rejected file changes nothing, and the message names the first bad line.

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

// Checks that `text` is rejected with `message`, leaving the habits unchanged.
void checkRejected(const QString &text, const QString &message) {
    Budget::DiningHabits habits;
    QString error;
    CAPTURE(text.toStdString());
    CHECK_FALSE(parse(text, habits, error));
    CHECK(error == message);
    CHECK(habits == UNCHANGED);
}
}

TEST_CASE("A name that isn't a day of the week is rejected") {
    checkRejected("day,meals\nMonday,2\nFunday,3\n", "Line 3: \"Funday\" isn't a day of the week.");
    checkRejected("Mo,2", "Line 1: \"Mo\" isn't a day of the week.");
    checkRejected("Mondays,2", "Line 1: \"Mondays\" isn't a day of the week.");
}

TEST_CASE("A meal count outside 0 to 10 is rejected") {
    checkRejected("Monday,11", "Line 1: meals must be a whole number from 0 to 10.");
    checkRejected("Monday,-1", "Line 1: meals must be a whole number from 0 to 10.");
}

TEST_CASE("A meal count that isn't a whole number is rejected") {
    checkRejected("Monday,2.5", "Line 1: meals must be a whole number from 0 to 10.");
    checkRejected("Monday,two", "Line 1: meals must be a whole number from 0 to 10.");
    checkRejected("Monday,", "Line 1: meals must be a whole number from 0 to 10.");
}

TEST_CASE("A day listed twice is rejected, even with a different spelling") {
    checkRejected("Monday,2\nmon,3", "Line 2: mon is listed more than once.");
}

TEST_CASE("A line without exactly one comma is rejected") {
    const QString message =
        "Line 1: expected a day and a number of meals, like \"Monday,2\".";
    checkRejected("Monday;2", message);
    checkRejected("Monday", message);
    checkRejected("Monday,2,extra", message);
}

TEST_CASE("The header only counts as one before the first day") {
    checkRejected("Monday,2\nday,meals", "Line 2: \"day\" isn't a day of the week.");
}

TEST_CASE("A file with no days is rejected") {
    checkRejected("", "The file doesn't list any days.");
    checkRejected("day,meals\n", "The file doesn't list any days.");
    checkRejected("\n\n  \n", "The file doesn't list any days.");
}

TEST_CASE("Line numbers count blank lines, so the message matches the file") {
    checkRejected("\n\nMonday,2\n\nFunday,1", "Line 5: \"Funday\" isn't a day of the week.");
}

TEST_CASE("Only the first bad line is reported") {
    checkRejected("Funday,1\nMonday,99", "Line 1: \"Funday\" isn't a day of the week.");
}
