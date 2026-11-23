// Tests for SCU menu data, hours, and parsing the published cafe pages.

#include "doctest.h"
#include "scumenu.h"

#include <QSet>

namespace {
ScuMenu::Cafe cafeNamed(const QString &name) {
    for (const auto &cafe : ScuMenu::cafes()) {
        if (cafe.name == name) {
            return cafe;
        }
    }
    FAIL("Unknown cafe " << name.toStdString());
    return ScuMenu::cafes().first();
}

QByteArray cafePage(const QString &slug, const QDate &day, const QString &body) {
    return QString(
        "<html><head><link rel=\"canonical\" href=\"https://scudining.cafebonappetit.com/cafe/%1/\">"
        "</head><body><div class=\"site-panel__cafeinfo-header\"></div>"
        "<div data-end-date=\"%2\"></div>%3</body></html>")
        .arg(slug, day.toString(Qt::ISODate), body)
        .toUtf8();
}
}

TEST_CASE("Opening hours include the opening time but not the closing time") {
    const ScuMenu::Hours lunch{QTime(11, 0), QTime(14, 0)};
    CHECK(lunch.includes(QTime(11, 0)));
    CHECK(lunch.includes(QTime(13, 59)));
    CHECK_FALSE(lunch.includes(QTime(14, 0)));
    CHECK_FALSE(lunch.includes(QTime(10, 59)));

    const ScuMenu::Hours lateNight{QTime(22, 0), QTime(2, 0)};
    CHECK(lateNight.includes(QTime(23, 30)));
    CHECK(lateNight.includes(QTime(1, 0)));
    CHECK_FALSE(lateNight.includes(QTime(2, 0)));
    CHECK_FALSE(lateNight.includes(QTime(12, 0)));
}

TEST_CASE("Locations map to their cafe page") {
    CHECK(ScuMenu::cafeSlug("Marketplace") == "marketplace-2");
    CHECK(ScuMenu::cafeSlug("Marketplace / Trattoria") == "marketplace-2");
    CHECK(ScuMenu::cafeSlug("fresh bytes") == "fresh-bytes");
    CHECK(ScuMenu::cafeSlug("Somewhere off campus").isEmpty());
}

TEST_CASE("The built-in menu catalog is well formed") {
    QSet<QString> names;
    for (const auto &location : ScuMenu::locations()) {
        CAPTURE(location.name.toStdString());
        CHECK_FALSE(location.name.isEmpty());
        CHECK_FALSE(names.contains(location.name));
        names.insert(location.name);
        if (location.name.startsWith("Marketplace / ")) {
            CHECK(ScuMenu::cafeSlug(location.name) == "marketplace-2");
        }
        for (const auto &item : location.items) {
            CAPTURE(item.name.toStdString());
            CHECK_FALSE(item.name.isEmpty());
            CHECK((item.priceInHundredths > 0 || item.priceInHundredths == ScuMenu::ReceiptPrice));
            CHECK(item.choices.isEmpty() == item.choiceLabel.isEmpty());
            for (const auto &choice : item.choices) {
                CHECK((choice.priceInHundredths > 0 || choice.priceInHundredths == ScuMenu::ReceiptPrice));
            }
            for (const auto &extra : item.extras) {
                CHECK(extra.priceInHundredths > 0);
            }
        }
    }
}

TEST_CASE("Pages for another cafe or without cafe details are ignored") {
    const auto marketplace = cafeNamed("Marketplace");
    const QDate day(2026, 11, 20);
    CHECK_FALSE(ScuMenu::parseDailyMenu(cafePage("fresh-bytes", day, {}), marketplace, day).received);
    CHECK_FALSE(ScuMenu::parseDailyMenu("<html>Not a menu</html>", marketplace, day).received);
    CHECK(ScuMenu::parseDailyMenu(cafePage("marketplace-2", day, {}), marketplace, day).received);
}

TEST_CASE("Meal periods give the cafe and its stations their hours") {
    const auto marketplace = cafeNamed("Marketplace");
    const QDate day(2026, 11, 20);
    const QString dayparts =
        "<script>\n"
        "Bamco.dayparts['1'] = {\"starttime\":\"11:00\",\"endtime\":\"14:00\","
        "\"stations\":[{\"label\":\"The Fire Lunch\"},{\"label\":\"Trattoria\"}]};\n"
        "Bamco.dayparts['2'] = {\"starttime\":\"17:00\",\"endtime\":\"20:00\","
        "\"stations\":[{\"label\":\"The Fire Dinner\"}]};\n"
        "</script>";
    const auto menu = ScuMenu::parseDailyMenu(cafePage("marketplace-2", day, dayparts), marketplace, day);
    REQUIRE(menu.received);
    CHECK_FALSE(menu.specialHours);
    CHECK(menu.cafeSchedule.published);
    REQUIRE(menu.cafeSchedule.hours.size() == 2);
    CHECK(menu.cafeSchedule.hours.at(0).opens == QTime(11, 0));
    CHECK(menu.cafeSchedule.hours.at(1).closes == QTime(20, 0));

    REQUIRE(menu.stations.contains("Marketplace / Fire Grill"));
    CHECK(menu.stations.value("Marketplace / Fire Grill").hours.size() == 2);
    REQUIRE(menu.stations.contains("Marketplace / Trattoria"));
    CHECK(menu.stations.value("Marketplace / Trattoria").hours.size() == 1);

    // A page published for a different day must not supply today's hours.
    const auto stale = ScuMenu::parseDailyMenu(
        cafePage("marketplace-2", day.addDays(-1), dayparts), marketplace, day);
    CHECK(stale.stations.isEmpty());
}

TEST_CASE("Date-specific closures override regular hours") {
    const auto bakery = cafeNamed("Mission Bakery & Cafe");
    const QString special =
        "<div class=\"cafe-hours-special\"><span class=\"pull-left\">Thanksgiving</span> "
        "<span class=\"pull-right\">November 26, Closed</span></div>"
        "<div class=\"cafe-hours-special\"><span class=\"pull-left\">Winter break</span> "
        "<span class=\"pull-right\">December 19 - January 4, 9:00 am - 1:00 pm</span></div>";

    const QDate thanksgiving(2026, 11, 26);
    const auto closed = ScuMenu::parseDailyMenu(cafePage("mission-bakery", thanksgiving, special),
                                                bakery, thanksgiving);
    CHECK(closed.specialHours);
    CHECK(closed.cafeSchedule.published);
    CHECK(closed.cafeSchedule.hours.isEmpty());

    // The winter break range crosses into the next year.
    const QDate newYear(2027, 1, 2);
    const auto reduced = ScuMenu::parseDailyMenu(cafePage("mission-bakery", newYear, special),
                                                 bakery, newYear);
    CHECK(reduced.specialHours);
    REQUIRE(reduced.cafeSchedule.hours.size() == 1);
    CHECK(reduced.cafeSchedule.hours.first().opens == QTime(9, 0));
    CHECK(reduced.cafeSchedule.hours.first().closes == QTime(13, 0));
    CHECK(reduced.cafeSchedule.note == "Winter break");

    const QDate ordinary(2026, 11, 20);
    CHECK_FALSE(ScuMenu::parseDailyMenu(cafePage("mission-bakery", ordinary, special),
                                        bakery, ordinary).specialHours);
}

TEST_CASE("The weekly schedule is used when no meal periods are published") {
    const auto cellar = cafeNamed("The Cellar Market");
    const QString weekly =
        "<p>Weekly Schedule</p><ul>"
        "<li><span class=\"pull-left\">Open</span><span class=\"pull-right\">Mon-Fri, 8:00 am - 9:00 pm</span></li>"
        "<li><span class=\"pull-left\">Closed</span><span class=\"pull-right\">Sat-Sun</span></li>"
        "</ul>";

    const QDate friday(2026, 11, 20);
    const auto weekday = ScuMenu::parseDailyMenu(cafePage("cellar-market", friday, weekly), cellar, friday);
    CHECK(weekday.cafeSchedule.published);
    REQUIRE(weekday.cafeSchedule.hours.size() == 1);
    CHECK(weekday.cafeSchedule.hours.first().opens == QTime(8, 0));
    CHECK(weekday.cafeSchedule.hours.first().closes == QTime(21, 0));

    const QDate saturday(2026, 11, 21);
    const auto weekend = ScuMenu::parseDailyMenu(cafePage("cellar-market", saturday, weekly), cellar, saturday);
    CHECK(weekend.cafeSchedule.published);
    CHECK(weekend.cafeSchedule.hours.isEmpty());
}
