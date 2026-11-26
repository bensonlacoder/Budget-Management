#include "appstyle.h"

#include <QDateEdit>
#include <QPalette>
#include <QWidget>

namespace {
const QString DATE_FORMAT = QStringLiteral("MMM d, yyyy");

struct Tint {
    const char *text;
    const char *background;
    const char *border;
};

// Each pair keeps text at 4.5:1 or more against its own background and the window.
constexpr Tint LIGHT_ALERT{"#8c1d18", "#fdecea", "#f2b8b5"};
constexpr Tint DARK_ALERT{"#ffb4ab", "#3b1a1a", "#6b2b27"};
constexpr Tint LIGHT_NOTICE{"#174ea6", "#e8f0fe", "#c6dafc"};
constexpr Tint DARK_NOTICE{"#a8c7fa", "#1a2b45", "#2c4a75"};
constexpr const char *LIGHT_ERROR = "#b00020";
constexpr const char *DARK_ERROR = "#ff8a80";

QString banner(const Tint &tint) {
    return QStringLiteral("QLabel { color: %1; background-color: %2; border: 1px solid %3;"
                          " border-radius: 4px; padding: 8px; }")
        .arg(QLatin1String(tint.text), QLatin1String(tint.background),
             QLatin1String(tint.border));
}
} // namespace

namespace AppStyle {

QLocale locale() {
    return QLocale(QLocale::English, QLocale::UnitedStates);
}

QString number(double points) {
    return locale().toString(points, 'f', 2);
}

QString points(double points) {
    return QWidget::tr("%1 points").arg(number(points));
}

QString date(const QDate &date) {
    return locale().toString(date, DATE_FORMAT);
}

QString shortDate(const QDate &date) {
    return locale().toString(date, QStringLiteral("MMM d"));
}

QString time(const QTime &time) {
    // An explicit format: the locale's short format puts a narrow no-break space
    // before AM/PM in newer Qt versions.
    return locale().toString(time, QStringLiteral("h:mm AP"));
}

QString dateTime(const QDateTime &dateTime) {
    return QWidget::tr("%1, %2").arg(date(dateTime.date()), time(dateTime.time()));
}

void useDateFormat(QDateEdit *edit) {
    edit->setLocale(locale());
    edit->setDisplayFormat(DATE_FORMAT);
}

bool isDark(const QWidget *widget) {
    return widget->palette().color(QPalette::Window).lightness() < 128;
}

QString errorText(const QWidget *widget) {
    return QStringLiteral("color: %1;")
        .arg(QLatin1String(isDark(widget) ? DARK_ERROR : LIGHT_ERROR));
}

QString alertBanner(const QWidget *widget) {
    return banner(isDark(widget) ? DARK_ALERT : LIGHT_ALERT);
}

QString noticeBanner(const QWidget *widget) {
    return banner(isDark(widget) ? DARK_NOTICE : LIGHT_NOTICE);
}

} // namespace AppStyle
