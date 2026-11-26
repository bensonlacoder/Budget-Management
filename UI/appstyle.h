#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QDate>
#include <QDateTime>
#include <QLocale>
#include <QString>

class QDateEdit;
class QWidget;

// One way to write points, dates and status colors on every page, so the same value
// never looks different on two tabs.
//
// Numbers and dates use US English on every computer: SCU dining points are a US
// product, and a fixed locale keeps the tests' expected text the same on Windows and
// in CI, whatever the machine's region settings are.
namespace AppStyle {
QLocale locale();

// "2,192.00": two decimals and a thousands separator.
QString number(double points);
// "2,192.00 points"
QString points(double points);
// "Nov 26, 2026"
QString date(const QDate &date);
// "Nov 26"
QString shortDate(const QDate &date);
// "8:14 AM", with a plain space before AM or PM.
QString time(const QTime &time);
// "Nov 26, 2026, 8:14 AM"
QString dateTime(const QDateTime &dateTime);
// Makes a date picker show and accept dates as "Nov 26, 2026".
void useDateFormat(QDateEdit *edit);

// Colors follow the widget's palette, so they stay readable in dark mode. The
// stylesheets are for QLabel.
bool isDark(const QWidget *widget);
// Error text, such as a rejected entry.
QString errorText(const QWidget *widget);
// A tinted box for an alert that needs attention.
QString alertBanner(const QWidget *widget);
// A tinted box for a neutral notice, such as the sandbox explanation.
QString noticeBanner(const QWidget *widget);
} // namespace AppStyle

#endif // APPSTYLE_H
