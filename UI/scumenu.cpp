#include "scumenu.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <QTextDocumentFragment>

#include <optional>

namespace ScuMenu {
const QList<Cafe> &cafes() {
    static const QList<Cafe> cafes{
        {"Marketplace", "marketplace-2"},
        {"Mission Bakery & Cafe", "mission-bakery"},
        {"Cadence Cyber Cafe", "cadence-cyber-cafe"},
        {"Fresh Bytes", "fresh-bytes"},
        {"Sidebar Cafe", "side-bar-cafe"},
        {"Sunstream Cafe", "the-sunstream-cafe"},
        {"The Cellar Market", "cellar-market"}
    };
    return cafes;
}

QString cafeSlug(const QString &location) {
    const QString venue = location.section(" / ", 0, 0);
    for (const auto &cafe : cafes()) {
        if (cafe.name.compare(venue, Qt::CaseInsensitive) == 0) {
            return cafe.slug;
        }
    }
    return {};
}

bool Hours::includes(QTime time) const {
    return opens < closes ? time >= opens && time < closes : time >= opens || time < closes;
}

namespace {
QString plainText(const QString &html) {
    return QTextDocumentFragment::fromHtml(html).toPlainText().simplified();
}

QString stationLocation(const Cafe &cafe, QString station) {
    if (cafe.slug != "marketplace-2") {
        return cafe.name;
    }
    static const QRegularExpression daypart(R"(\s+(Breakfast|Brunch|Lunch|Dinner)$)",
                                            QRegularExpression::CaseInsensitiveOption);
    station.remove(daypart);
    if (station == "The Fire") {
        station = "Fire Grill";
    } else if (station == "Soup" || station == "The Garden") {
        station = "Soup & Salad Bar";
    } else if (station != "The Slice" && station.startsWith("The ")) {
        station.remove(0, 4);
    }
    return cafe.name + " / " + station;
}

std::optional<Hours> readHours(const QString &text) {
    static const QRegularExpression times(
        R"((\d{1,2}:\d{2})\s*(am|pm)\s*[-\x{2013}]\s*(\d{1,2}:\d{2})\s*(am|pm))",
        QRegularExpression::CaseInsensitiveOption);
    const auto match = times.match(text);
    if (!match.hasMatch()) {
        return std::nullopt;
    }
    const auto parse = [&](int index) {
        return QLocale::c().toTime(match.captured(index) + " " + match.captured(index + 1).toLower(),
                                   "h:mm ap");
    };
    const auto opens = parse(1);
    const auto closes = parse(3);
    if (!opens.isValid() || !closes.isValid() || opens == closes) {
        return std::nullopt;
    }
    return Hours{opens, closes};
}

const QRegularExpression hoursRows(
    R"re(pull-left['"][^>]*>([^<]*)</span>\s*<span[^>]*pull-right['"][^>]*>([^<]*)</span>)re");

std::optional<Schedule> readSpecialHours(const QString &html, const QDate &date) {
    static const QRegularExpression blocks(R"re(class=['"]cafe-hours-special['"][^>]*>(.*?)</div>)re",
                                            QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression dates(
        R"(^([A-Za-z]+)\s+(\d{1,2})(?:\s*[-\x{2013}]\s*(?:([A-Za-z]+)\s+)?(\d{1,2}))?(?:,\s*(.*))?$)");
    const QLocale english(QLocale::English);
    const auto monthNumber = [&](const QString &month) {
        for (int number = 1; number <= 12; ++number) {
            if (month.compare(english.monthName(number), Qt::CaseInsensitive) == 0) {
                return number;
            }
        }
        return 0;
    };
    auto iter = blocks.globalMatch(html);
    while (iter.hasNext()) {
        const QString block = iter.next().captured(1);
        auto rows = hoursRows.globalMatch(block);
        while (rows.hasNext()) {
            const auto row = rows.next();
            const auto range = dates.match(plainText(row.captured(2)));
            if (!range.hasMatch()) {
                continue;
            }
            const int firstMonth = monthNumber(range.captured(1));
            const int lastMonth = range.captured(3).isEmpty() ? firstMonth : monthNumber(range.captured(3));
            const int lastDay = range.captured(4).isEmpty() ? range.captured(2).toInt() : range.captured(4).toInt();
            const bool crossesYear = lastMonth < firstMonth;
            const int firstYear = date.year() - (crossesYear && date.month() <= lastMonth ? 1 : 0);
            const QDate first(firstYear, firstMonth, range.captured(2).toInt());
            const QDate last(firstYear + (crossesYear ? 1 : 0), lastMonth, lastDay);
            if (!first.isValid() || !last.isValid() || date < first || date > last) {
                continue;
            }

            const QString title = plainText(row.captured(1));
            const QString detail = range.captured(5);
            if (title.contains("Closed", Qt::CaseInsensitive) ||
                detail.compare("Closed", Qt::CaseInsensitive) == 0) {
                return Schedule{true, {}, {}};
            }
            if (const auto hours = readHours(detail)) {
                return Schedule{true, {*hours}, title};
            }
            return Schedule{false, {}, title};
        }
    }
    return std::nullopt;
}

Schedule readWeeklyHours(const QString &html, const QDate &date) {
    const auto weekly = html.indexOf("Weekly Schedule</p>");
    if (weekly < 0) {
        return {};
    }
    const auto end = html.indexOf("</ul>", weekly);
    auto rows = hoursRows.globalMatch(html.mid(weekly, end < 0 ? -1 : end - weekly));
    const QStringList days{"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    Schedule schedule;
    while (rows.hasNext()) {
        const auto row = rows.next();
        const QString description = plainText(row.captured(2));
        const QString range = description.section(',', 0, 0);
        const int first = days.indexOf(range.section('-', 0, 0)) + 1;
        const int last = days.indexOf(range.section('-', -1)) + 1;
        if (first == 0 || last == 0) {
            continue;
        }
        schedule.published = true;
        if (date.dayOfWeek() >= first && date.dayOfWeek() <= last &&
            plainText(row.captured(1)).compare("Closed", Qt::CaseInsensitive) != 0) {
            if (const auto hours = readHours(description)) {
                schedule.hours.append(*hours);
            }
        }
    }
    return schedule;
}
}

DailyMenu parseDailyMenu(const QByteArray &html, const Cafe &cafe, const QDate &date) {
    const QString page = QString::fromUtf8(html);
    static const QRegularExpression canonical(
        R"re(<link rel="canonical" href="https://scudining\.cafebonappetit\.com/cafe/([^/"]+)/")re");
    if (canonical.match(page).captured(1) != cafe.slug || !page.contains("site-panel__cafeinfo-header")) {
        return {};
    }
    static const QRegularExpression partJson(R"re(Bamco\.dayparts\[['"]\d+['"]\]\s*=\s*(\{[^\r\n]*\});)re");

    DailyMenu result;
    result.received = true;
    const QDate publishedDay = QDate::fromString(
        QRegularExpression(R"re(data-end-date="(\d{4}-\d{2}-\d{2})")re").match(page).captured(1),
        Qt::ISODate);
    auto parts = partJson.globalMatch(page);
    while (parts.hasNext() && publishedDay == date) {
        const auto part = QJsonDocument::fromJson(parts.next().captured(1).toUtf8()).object();
        const QTime opens = QTime::fromString(part.value("starttime").toString(), "HH:mm");
        const QTime closes = QTime::fromString(part.value("endtime").toString(), "HH:mm");
        const bool hasHours = opens.isValid() && closes.isValid() && opens != closes;
        if (hasHours) {
            result.cafeSchedule.published = true;
            result.cafeSchedule.hours.append({opens, closes});
        }

        const auto stations = part.value("stations").toArray();
        for (const auto &entry : stations) {
            const auto station = entry.toObject();
            const QString name = plainText(station.value("label").toString());
            if (name.isEmpty()) {
                continue;
            }
            const QString location = stationLocation(cafe, name);
            if (hasHours) {
                auto &schedule = result.stations[location];
                schedule.published = true;
                schedule.hours.append({opens, closes});
            }
        }
    }

    // Date-specific closures and reduced hours take precedence over regular service.
    if (const auto special = readSpecialHours(page, date)) {
        result.cafeSchedule = *special;
        result.specialHours = true;
    } else if (!result.cafeSchedule.published) {
        if (page.contains("site-panel__cafeinfo-currently--closed")) {
            result.cafeSchedule.published = true;
        } else {
            result.cafeSchedule = readWeeklyHours(page, date);
        }
    }
    return result;
}
}
