#include "appstyle.h"

#include <QApplication>
#include <QDateEdit>
#include <QFont>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QProxyStyle>
#include <QPushButton>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QWidget>

#include <algorithm>
#include <utility>

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

// ---- Theme ----

// SCU Cardinal Red (PMS 202 C). White text on it is 8.6:1.
constexpr const char *CARDINAL = "#862633";
constexpr const char *CARDINAL_HOVER = "#6e1f2a";
constexpr const char *CARDINAL_PRESSED = "#5a1922";
constexpr double BODY_POINTS = 10.5;

// Surface and text colors for one appearance. Text on `card` and `window` is 4.5:1 or
// more, including `muted` and `placeholder`.
struct Theme {
    const char *window;      // Behind the cards.
    const char *card;        // Cards, fields, tables.
    const char *text;
    const char *muted;       // Captions, column headers, unselected tabs.
    const char *placeholder;
    const char *border;      // Card and table edges.
    const char *field;       // Field and button edges.
    const char *hover;
    const char *pressed;
    const char *header;      // Table header background.
    const char *selection;   // Selected table rows, behind normal text.
    const char *disabled;
    const char *link;
    const char *primaryDisabled;
};

constexpr Theme LIGHT{"#f6f3f2", "#ffffff", "#1f1a1b", "#5e5557", "#6e6466", "#e3dcda",
                      "#c9c0be", "#f5efee", "#ebe3e1", "#faf6f5", "#f3dfe2", "#9d9395",
                      CARDINAL, "#ecdcde"};
constexpr Theme DARK{"#1b1819", "#262223", "#f2eded", "#bdb3b4", "#a9a0a1", "#3a3435",
                     "#575051", "#312c2d", "#3b3536", "#2c2728", "#4a2a30", "#7d7475",
                     "#f0a3ab", "#43333a"};

QPalette palette(const Theme &theme) {
    QPalette palette;
    const auto set = [&palette](QPalette::ColorRole role, const char *color) {
        palette.setColor(role, QColor(QLatin1String(color)));
    };
    set(QPalette::Window, theme.window);
    set(QPalette::WindowText, theme.text);
    set(QPalette::Base, theme.card);
    set(QPalette::AlternateBase, theme.header);
    set(QPalette::Text, theme.text);
    set(QPalette::Button, theme.card);
    set(QPalette::ButtonText, theme.text);
    set(QPalette::PlaceholderText, theme.placeholder);
    set(QPalette::Highlight, CARDINAL);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    set(QPalette::Link, theme.link);
    set(QPalette::Light, theme.card);
    set(QPalette::Midlight, theme.hover);
    set(QPalette::Mid, theme.border);
    set(QPalette::Dark, theme.field);
    set(QPalette::ToolTipBase, theme.text);
    set(QPalette::ToolTipText, theme.card);
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        palette.setColor(QPalette::Disabled, role, QColor(QLatin1String(theme.disabled)));
    }
    return palette;
}

QString styleSheet(const Theme &t) {
    QString sheet = QStringLiteral(R"(
QTabWidget::pane { border: none; border-top: 1px solid @border; }
QTabBar::tab { background: transparent; color: @muted; border: none;
    border-bottom: 3px solid transparent; padding: 8px 16px; margin-right: 2px; }
QTabBar::tab:selected { color: @text; border-bottom: 3px solid @cardinal; }
QTabBar::tab:hover:!selected { color: @text; border-bottom: 3px solid @field; }

QGroupBox { background: @card; border: 1px solid @border; border-radius: 10px;
    margin-top: 32px; padding: 12px 12px 10px 12px;
    font-size: 12pt; font-weight: 600; }
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left;
    left: 2px; top: 4px; color: @text; }

/* Type scale. A group box's font is only for its title; everything inside is body
   text unless a widget has a text role (AppStyle::setTextRole). */
QGroupBox * { font-size: 10.5pt; font-weight: 400; }
*[textRole="headline"] { font-size: 18pt; font-weight: 700; }
*[textRole="balance"] { font-size: 14pt; font-weight: 700; }
*[textRole="figure"] { font-size: 12pt; font-weight: 700; }
*[textRole="strong"] { font-weight: 700; }

/* Every button is Cardinal Red. Disabled ones turn pale with gray text, so they read
   as unavailable rather than as a lighter red. */
QPushButton { background: @cardinal; color: #ffffff; border: 1px solid @cardinal;
    border-radius: 6px; padding: 6px 14px; min-height: 20px; font-weight: 600; }
QPushButton:hover { background: @cardinalHover; border-color: @cardinalHover; }
QPushButton:pressed { background: @cardinalPressed; border-color: @cardinalPressed; }
QPushButton:focus { border: 2px solid @text; }
QPushButton:disabled { background: @primaryDisabled; border-color: @primaryDisabled;
    color: @disabled; }
/* A clear button for the gentler of two choices, such as Decline beside Accept. */
QPushButton[secondary="true"] { background: @card; color: @text; border: 1px solid @field;
    font-weight: 400; }
QPushButton[secondary="true"]:hover { background: @hover; }
QPushButton[secondary="true"]:pressed { background: @pressed; }
QPushButton[secondary="true"]:focus { border: 2px solid @cardinal; }
QPushButton[secondary="true"]:disabled { background: @window; color: @disabled;
    border-color: @border; }
QPushButton:flat { background: transparent; border: none; color: @link; padding: 6px 4px; }
QPushButton:flat:hover { text-decoration: underline; }

QLineEdit, QAbstractSpinBox, QComboBox { background: @card; color: @text;
    border: 1px solid @field; border-radius: 6px; padding: 5px 8px; min-height: 20px;
    selection-background-color: @cardinal; selection-color: #ffffff; }
QLineEdit:focus, QAbstractSpinBox:focus, QComboBox:focus { border: 1px solid @cardinal; }
QLineEdit:disabled, QAbstractSpinBox:disabled, QComboBox:disabled {
    background: @window; color: @disabled; }
QComboBox::drop-down, QDateEdit::drop-down { subcontrol-origin: padding;
    subcontrol-position: center right; border: none; width: 24px; }
QComboBox::down-arrow, QDateEdit::down-arrow { image: url(:/icons/chevron-down.png);
    width: 12px; height: 12px; }
QAbstractSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right;
    border: none; width: 22px; }
QAbstractSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right;
    border: none; width: 22px; }
QAbstractSpinBox::up-arrow { image: url(:/icons/chevron-up.png); width: 10px; height: 10px; }
QAbstractSpinBox::down-arrow { image: url(:/icons/chevron-down.png); width: 10px; height: 10px; }
QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::down-arrow:disabled,
QAbstractSpinBox::up-arrow:off, QAbstractSpinBox::down-arrow:off { image: none; }

QTableView, QListView { background: @card; color: @text; border: 1px solid @border;
    border-radius: 8px; gridline-color: @border; alternate-background-color: @header;
    selection-background-color: @selection; selection-color: @text; }
QHeaderView::section { background: @header; color: @muted; border: none;
    border-bottom: 1px solid @border; padding: 6px 8px; font-weight: 600; }

QScrollArea { background: transparent; border: none; }
QScrollArea > QWidget > QWidget { background: transparent; }

QCheckBox { spacing: 8px; }
QToolTip { background: @text; color: @card; border: none; padding: 6px 8px; }

QFrame#brandPanel { background: @cardinal; }
QFrame#brandPanel QLabel { color: #ffffff; background: transparent; }
)");
    const std::pair<const char *, const char *> colors[] = {
        {"@cardinalPressed", CARDINAL_PRESSED}, {"@cardinalHover", CARDINAL_HOVER},
        {"@cardinal", CARDINAL}, {"@window", t.window}, {"@card", t.card},
        {"@text", t.text}, {"@muted", t.muted}, {"@border", t.border}, {"@field", t.field},
        {"@hover", t.hover}, {"@pressed", t.pressed}, {"@header", t.header},
        {"@selection", t.selection}, {"@disabled", t.disabled}, {"@link", t.link},
        {"@primaryDisabled", t.primaryDisabled}};
    for (const auto &[name, color] : colors) {
        sheet.replace(QLatin1String(name), QLatin1String(color));
    }
    return sheet;
}

// Fusion, except that shortcut letters are underlined only while Alt is held, as on
// Windows, instead of always.
class AppProxyStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                  QStyleHintReturn *returnData) const override {
        if (hint == SH_UnderlineShortcut) {
            return 0;
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

bool prefersDark() {
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

void applyColors(QApplication &app) {
    const Theme &theme = prefersDark() ? DARK : LIGHT;
    app.setPalette(palette(theme));
    app.setStyleSheet(styleSheet(theme));
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

void applyTheme(QApplication &app) {
    // Fusion draws the same on Windows and macOS and follows the palette fully.
    app.setStyle(new AppProxyStyle(QStyleFactory::create(QStringLiteral("Fusion"))));
    QFont font = app.font();
    font.setPointSizeF(std::max(font.pointSizeF(), BODY_POINTS));
    app.setFont(font);
    applyColors(app);
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, &app,
                     [&app] { applyColors(app); });
}

QColor accent() {
    return QColor(QLatin1String(CARDINAL));
}

void markSecondary(QPushButton *button) {
    button->setProperty("secondary", true);
    // Re-apply the style sheet now that the property it matches on is set.
    button->style()->unpolish(button);
    button->style()->polish(button);
}

void setTextRole(QWidget *widget, const char *role) {
    widget->setProperty("textRole", QString::fromLatin1(role));
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

QPixmap scuLogo(const QWidget *widget, int height, const QColor &color) {
    // A mask: the seal's shape on a transparent background (UI/images/scu-logo.png).
    const QPixmap mask(QStringLiteral(":/images/scu-logo.png"));
    if (mask.isNull() || height <= 0) {
        return {};
    }
    const qreal ratio = widget->devicePixelRatioF();
    QPixmap logo = mask.scaledToHeight(qRound(height * ratio), Qt::SmoothTransformation);
    QPainter painter(&logo);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(logo.rect(), color);
    painter.end();
    logo.setDevicePixelRatio(ratio);
    return logo;
}

QColor logoColor(const QWidget *widget) {
    return isDark(widget) ? QColor(QLatin1String(DARK.link)) : accent();
}

} // namespace AppStyle
