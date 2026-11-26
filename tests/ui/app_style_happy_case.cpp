// Shared number, date and color formatting happy case tests (UI/appstyle.h)
// Original test contributors: Nathan Gu
// Covers AppStyle::locale, number, points, date, shortDate, time, dateTime,
// useDateFormat, isDark, errorText, alertBanner and noticeBanner.

#include "doctest.h"
#include "appstyle.h"

#include <QDateEdit>
#include <QPalette>
#include <QWidget>

namespace {
// A widget whose window color is `window`, for the light and dark colors.
void paint(QWidget &widget, const char *window) {
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, QColor(window));
    widget.setPalette(palette);
}
}

TEST_CASE("Numbers and dates use US English") {
    CHECK(AppStyle::locale().language() == QLocale::English);
    CHECK(AppStyle::locale().territory() == QLocale::UnitedStates);
}

TEST_CASE("Point amounts have two decimals and a thousands separator") {
    CHECK(AppStyle::number(2192.0) == "2,192.00");
    CHECK(AppStyle::number(10.5) == "10.50");
    CHECK(AppStyle::points(2192.0) == "2,192.00 points");
    CHECK(AppStyle::points(6.5) == "6.50 points");
}

TEST_CASE("Dates read as month, day and year") {
    CHECK(AppStyle::date(QDate(2026, 11, 26)) == "Nov 26, 2026");
    CHECK(AppStyle::shortDate(QDate(2026, 11, 26)) == "Nov 26");
}

TEST_CASE("Times read as hours and minutes with AM or PM") {
    CHECK(AppStyle::time(QTime(8, 14)) == "8:14 AM");
    CHECK(AppStyle::time(QTime(20, 5)) == "8:05 PM");
    CHECK(AppStyle::dateTime(QDateTime(QDate(2026, 11, 26), QTime(8, 14)))
          == "Nov 26, 2026, 8:14 AM");
}

TEST_CASE("Date pickers show the same date format") {
    QDateEdit edit(QDate(2026, 11, 26));
    AppStyle::useDateFormat(&edit);
    CHECK(edit.displayFormat() == "MMM d, yyyy");
    CHECK(edit.text() == "Nov 26, 2026");
    CHECK(edit.date() == QDate(2026, 11, 26));
}

TEST_CASE("A light window uses the light colors") {
    QWidget widget;
    paint(widget, "#f3f3f3");
    CHECK_FALSE(AppStyle::isDark(&widget));
    CHECK(AppStyle::errorText(&widget) == "color: #b00020;");
    CHECK(AppStyle::alertBanner(&widget).contains("background-color: #fdecea"));
    CHECK(AppStyle::noticeBanner(&widget).contains("background-color: #e8f0fe"));
}

TEST_CASE("A dark window uses the dark colors") {
    QWidget widget;
    paint(widget, "#202020");
    CHECK(AppStyle::isDark(&widget));
    CHECK(AppStyle::errorText(&widget) == "color: #ff8a80;");
    CHECK(AppStyle::alertBanner(&widget).contains("background-color: #3b1a1a"));
    CHECK(AppStyle::noticeBanner(&widget).contains("background-color: #1a2b45"));
}

TEST_CASE("Banners are QLabel stylesheets with text, background and border colors") {
    QWidget widget;
    paint(widget, "#f3f3f3");
    const QString alert = AppStyle::alertBanner(&widget);
    CHECK(alert.startsWith("QLabel {"));
    CHECK(alert.contains("color: #8c1d18"));
    CHECK(alert.contains("border: 1px solid #f2b8b5"));
    CHECK(alert.contains("padding: 8px"));
}
