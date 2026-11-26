#include "habitscsv.h"

#include <QCoreApplication>
#include <QStringList>

namespace {
constexpr int MAX_MEALS = 10;

QString tr(const char *text) {
    return QCoreApplication::translate("HabitsCsv", text);
}

// 0 for Monday through 6 for Sunday, or -1 if `name` isn't a day.
int dayIndex(const QString &name) {
    static const char *const days[] = {"monday", "tuesday", "wednesday", "thursday",
                                       "friday", "saturday", "sunday"};
    const QString lower = name.trimmed().toLower();
    for (int day = 0; day < 7; ++day) {
        const QString full = QString::fromLatin1(days[day]);
        if (lower == full || lower == full.left(3)) {
            return day;
        }
    }
    return -1;
}
}

namespace HabitsCsv {

bool parse(const QString &text, Budget::DiningHabits &habits, QString &error) {
    Budget::DiningHabits parsed = {};
    bool seen[7] = {};
    bool anyDay = false;
    QString body = text;
    if (body.startsWith(QChar(0xFEFF))) {
        body.remove(0, 1); // A byte order mark some editors add.
    }
    const QStringList lines = body.split(QLatin1Char('\n'));
    for (int index = 0; index < lines.size(); ++index) {
        const QString line = lines.at(index).trimmed();
        const int lineNumber = index + 1;
        if (line.isEmpty()) {
            continue;
        }
        const QStringList fields = line.split(QLatin1Char(','));
        if (fields.size() != 2) {
            error = tr("Line %1: expected a day and a number of meals, like \"Monday,2\".")
                        .arg(lineNumber);
            return false;
        }
        const QString dayName = fields.at(0).trimmed();
        const QString mealsText = fields.at(1).trimmed();
        if (!anyDay && dayName.compare(QLatin1String("day"), Qt::CaseInsensitive) == 0
            && mealsText.compare(QLatin1String("meals"), Qt::CaseInsensitive) == 0) {
            continue; // The header.
        }
        const int day = dayIndex(dayName);
        if (day < 0) {
            error = tr("Line %1: \"%2\" isn't a day of the week.").arg(lineNumber).arg(dayName);
            return false;
        }
        if (seen[day]) {
            error = tr("Line %1: %2 is listed more than once.").arg(lineNumber).arg(dayName);
            return false;
        }
        bool isNumber = false;
        const int meals = mealsText.toInt(&isNumber);
        if (!isNumber || meals < 0 || meals > MAX_MEALS) {
            error = tr("Line %1: meals must be a whole number from 0 to %2.")
                        .arg(lineNumber)
                        .arg(MAX_MEALS);
            return false;
        }
        seen[day] = true;
        anyDay = true;
        parsed[static_cast<size_t>(day)] = meals;
    }
    if (!anyDay) {
        error = tr("The file doesn't list any days.");
        return false;
    }
    habits = parsed;
    return true;
}

} // namespace HabitsCsv
