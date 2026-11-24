#ifndef MENUSEARCHDIALOG_H
#define MENUSEARCHDIALOG_H

#include "scumenu.h"

#include <QDialog>
#include <QList>
#include <QString>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QPushButton;
class QTableWidget;

// Finding meals within a point limit (FR-44), as plain functions so they can be tested
// without the dialog.
namespace MenuSearch {
struct Match {
    QString location; // A catalog location, which is also a saveable section.
    QString item;     // The item, with its option in parentheses when it has options.
    int priceInHundredths;
};

// Every item, or item option, that costs at most maxHundredths, cheapest first (ties
// by location, then item). Items priced at the receipt total are left out. With
// `location` set, only that location's items are considered.
QList<Match> matches(const QList<ScuMenu::Location> &catalog, int maxHundredths,
                     const QString &location = {});

// How many items or options in the search area are priced only at the receipt total.
int receiptPricedCount(const QList<ScuMenu::Location> &catalog, const QString &location = {});
}

// Finds meals at or under a point limit and keeps the user's saved menu sections
// (FR-44, FR-45). Opened from Menus > Find meals.
class MenuSearchDialog : public QDialog {
    Q_OBJECT

public:
    explicit MenuSearchDialog(const QString &account, QWidget *parent = nullptr);

private:
    void search();
    void saveSection();
    void removeSection();
    void showSection();
    void refreshSaved();
    QString selectedLocation() const;

    QString account;
    QDoubleSpinBox *maxPoints;
    QComboBox *location;
    QTableWidget *results;
    QLabel *status;
    QPushButton *saveButton;
    QListWidget *saved;
    QPushButton *showButton;
    QPushButton *removeButton;
    QLabel *feedback;
};

#endif // MENUSEARCHDIALOG_H
