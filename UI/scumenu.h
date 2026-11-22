#ifndef SCUMENU_H
#define SCUMENU_H

#include <QDate>
#include <QHash>
#include <QList>
#include <QString>
#include <QTime>

namespace ScuMenu {
// Prices are hundredths of a point. A missing price requires the receipt total.
constexpr int ReceiptPrice = -1;

struct Choice {
    QString name;
    int priceInHundredths = ReceiptPrice;
};

struct Item {
    QString name;
    int priceInHundredths = ReceiptPrice;
    QString choiceLabel;
    QList<Choice> choices; // Each choice gives the full base price for that configuration.
    QList<Choice> extras;  // Each selected extra is added once.
    QString reference;
};

struct Location {
    QString name;
    QList<Item> items;
    QString reference;
};

struct Cafe {
    QString name;
    QString slug;
};

struct Hours {
    QTime opens;
    QTime closes;
    bool includes(QTime time) const;
};

struct Schedule {
    bool published = false;
    QList<Hours> hours;
    QString note;
};

struct DailyMenu {
    Schedule cafeSchedule;
    QHash<QString, Schedule> stations;
    bool received = false;
    bool specialHours = false;
};

const QList<Location> &locations();
const QList<Cafe> &cafes();
QString cafeSlug(const QString &location);
DailyMenu parseDailyMenu(const QByteArray &html, const Cafe &cafe, const QDate &date);
}

#endif // SCUMENU_H
