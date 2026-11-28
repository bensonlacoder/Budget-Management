// Shared number, date and color formatting error and edge case tests (UI/appstyle.h)
// Original test contributors: Nathan Gu

#include "doctest.h"
#include "appstyle.h"

#include <QDateEdit>
#include <QPixmap>
#include <QPalette>
#include <QWidget>

namespace {
void paint(QWidget &widget, const QColor &window) {
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, window);
    widget.setPalette(palette);
}
}

TEST_CASE("Zero, negative, tiny and very large amounts are formatted the same way") {
    CHECK(AppStyle::number(0.0) == "0.00");
    CHECK(AppStyle::number(-8.0) == "-8.00");
    CHECK(AppStyle::number(0.01) == "0.01");
    CHECK(AppStyle::number(1234567.891) == "1,234,567.89");
    CHECK(AppStyle::points(0.0) == "0.00 points");
    CHECK(AppStyle::points(-11.25) == "-11.25 points");
}

TEST_CASE("Formatting doesn't change with the computer's region settings") {
    const QLocale previous;
    QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
    CHECK(AppStyle::number(2192.5) == "2,192.50");
    CHECK(AppStyle::date(QDate(2026, 11, 26)) == "Nov 26, 2026");
    QDateEdit edit(QDate(2026, 3, 1));
    AppStyle::useDateFormat(&edit);
    CHECK(edit.text() == "Mar 1, 2026");
    QLocale::setDefault(previous);
}

TEST_CASE("Single-digit days and the turn of the year") {
    CHECK(AppStyle::date(QDate(2027, 1, 4)) == "Jan 4, 2027");
    CHECK(AppStyle::shortDate(QDate(2027, 1, 4)) == "Jan 4");
    CHECK(AppStyle::date(QDate(2026, 12, 31)) == "Dec 31, 2026");
}

TEST_CASE("Midnight and noon read as 12, and AM/PM has a plain space") {
    CHECK(AppStyle::time(QTime(0, 0)) == "12:00 AM");
    CHECK(AppStyle::time(QTime(12, 0)) == "12:00 PM");
    CHECK(AppStyle::time(QTime(23, 59)) == "11:59 PM");
    CHECK_FALSE(AppStyle::time(QTime(8, 14)).contains(QChar(0x202F)));
}

TEST_CASE("An invalid date or time formats as empty text") {
    CHECK(AppStyle::date(QDate()).isEmpty());
    CHECK(AppStyle::shortDate(QDate()).isEmpty());
    CHECK(AppStyle::time(QTime()).isEmpty());
}

TEST_CASE("A date picker keeps its range and value after formatting") {
    QDateEdit edit(QDate(2026, 11, 26));
    edit.setMaximumDate(QDate(2026, 11, 26));
    AppStyle::useDateFormat(&edit);
    CHECK(edit.maximumDate() == QDate(2026, 11, 26));
    edit.setDate(QDate(2026, 12, 25)); // Past the maximum: kept at the maximum.
    CHECK(edit.text() == "Nov 26, 2026");
}

TEST_CASE("Dark starts just below half lightness") {
    QWidget widget;
    paint(widget, QColor::fromHsl(0, 0, 127));
    CHECK(AppStyle::isDark(&widget));
    paint(widget, QColor::fromHsl(0, 0, 128));
    CHECK_FALSE(AppStyle::isDark(&widget));
    paint(widget, Qt::black);
    CHECK(AppStyle::isDark(&widget));
    paint(widget, Qt::white);
    CHECK_FALSE(AppStyle::isDark(&widget));
}

TEST_CASE("Alerts and notices never share colors") {
    QWidget widget;
    for (const QColor &window : {QColor("#f3f3f3"), QColor("#202020")}) {
        paint(widget, window);
        CHECK(AppStyle::alertBanner(&widget) != AppStyle::noticeBanner(&widget));
        CHECK_FALSE(AppStyle::alertBanner(&widget).contains(AppStyle::errorText(&widget)));
    }
}

TEST_CASE("In dark mode the seal is a lighter red, so it can be seen") {
    QWidget widget;
    paint(widget, QColor("#202020"));
    const QColor color = AppStyle::logoColor(&widget);
    CHECK(color != AppStyle::accent());
    CHECK(color.lightness() > AppStyle::accent().lightness());
}

TEST_CASE("A seal of no height is empty") {
    QWidget widget;
    CHECK(AppStyle::scuLogo(&widget, 0, Qt::white).isNull());
    CHECK(AppStyle::scuLogo(&widget, -5, Qt::white).isNull());
}
