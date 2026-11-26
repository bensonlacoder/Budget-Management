#include "menusearchdialog.h"
#include "localaccounts.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {
constexpr int POINT_DECIMALS = 2;
constexpr double DEFAULT_LIMIT = 10.0;
constexpr double MAX_LIMIT = 100.0;

bool inArea(const ScuMenu::Location &place, const QString &location) {
    return location.isEmpty() || place.name == location;
}

// Calls `visit(name, price)` for each priced choice of an item, or the item itself
// when it has no choices. Choices give the full price of that configuration.
template <typename Visit>
void forEachOption(const ScuMenu::Item &item, Visit visit) {
    if (item.choices.isEmpty()) {
        visit(item.name, item.priceInHundredths);
        return;
    }
    for (const auto &choice : item.choices) {
        visit(QStringLiteral("%1 (%2)").arg(item.name, choice.name), choice.priceInHundredths);
    }
}
}

namespace MenuSearch {
QList<Match> matches(const QList<ScuMenu::Location> &catalog, int maxHundredths,
                     const QString &location) {
    QList<Match> found;
    for (const auto &place : catalog) {
        if (!inArea(place, location)) {
            continue;
        }
        for (const auto &item : place.items) {
            forEachOption(item, [&](const QString &name, int price) {
                if (price != ScuMenu::ReceiptPrice && price <= maxHundredths) {
                    found.append({place.name, name, price});
                }
            });
        }
    }
    std::sort(found.begin(), found.end(), [](const Match &left, const Match &right) {
        if (left.priceInHundredths != right.priceInHundredths) {
            return left.priceInHundredths < right.priceInHundredths;
        }
        if (left.location != right.location) {
            return left.location < right.location;
        }
        return left.item < right.item;
    });
    return found;
}

int receiptPricedCount(const QList<ScuMenu::Location> &catalog, const QString &location) {
    int count = 0;
    for (const auto &place : catalog) {
        if (!inArea(place, location)) {
            continue;
        }
        for (const auto &item : place.items) {
            forEachOption(item, [&](const QString &, int price) {
                count += price == ScuMenu::ReceiptPrice ? 1 : 0;
            });
        }
    }
    return count;
}
}

MenuSearchDialog::MenuSearchDialog(const QString &account, QWidget *parent)
    : QDialog(parent), account(account), maxPoints(new QDoubleSpinBox(this)),
      location(new QComboBox(this)), results(new QTableWidget(0, 3, this)),
      status(new QLabel(this)), saveButton(new QPushButton(tr("&Save this section"), this)),
      saved(new QListWidget(this)), showButton(new QPushButton(tr("Sh&ow items"), this)),
      removeButton(new QPushButton(tr("&Remove"), this)), feedback(new QLabel(this)) {
    setWindowTitle(tr("Find meals"));
    resize(640, 600);

    maxPoints->setObjectName("maxPoints");
    maxPoints->setDecimals(POINT_DECIMALS);
    maxPoints->setRange(0.0, MAX_LIMIT);
    maxPoints->setSingleStep(0.5);
    maxPoints->setSuffix(tr(" pts"));
    maxPoints->setValue(DEFAULT_LIMIT);
    location->setObjectName("locationFilter");
    location->addItem(tr("All locations"), QString());
    for (const auto &place : ScuMenu::locations()) {
        if (!place.items.isEmpty()) {
            location->addItem(place.name, place.name);
        }
    }

    auto *searchForm = new QFormLayout;
    searchForm->addRow(tr("&Up to"), maxPoints);
    searchForm->addRow(tr("&Location"), location);

    results->setObjectName("mealResults");
    results->setHorizontalHeaderLabels({tr("Item"), tr("Location"), tr("Points")});
    results->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    results->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    results->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    results->verticalHeader()->hide();
    results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    results->setSelectionBehavior(QAbstractItemView::SelectRows);
    results->setSelectionMode(QAbstractItemView::SingleSelection);
    status->setObjectName("searchStatus");
    status->setWordWrap(true);
    saveButton->setObjectName("saveSectionButton");
    auto *note = new QLabel(tr("Prices come from the dated SCU reference menu. Check your receipt."),
                            this);
    note->setWordWrap(true);

    auto *found = new QGroupBox(tr("Meals within your limit"), this);
    auto *foundLayout = new QVBoxLayout(found);
    foundLayout->addLayout(searchForm);
    foundLayout->addWidget(results, 1);
    foundLayout->addWidget(status);
    foundLayout->addWidget(note);
    foundLayout->addWidget(saveButton, 0, Qt::AlignRight);

    saved->setObjectName("savedSections");
    showButton->setObjectName("showSectionButton");
    removeButton->setObjectName("removeSectionButton");
    feedback->setObjectName("sectionFeedback");
    feedback->setWordWrap(true);
    auto *savedButtons = new QHBoxLayout;
    savedButtons->addStretch();
    savedButtons->addWidget(showButton);
    savedButtons->addWidget(removeButton);
    auto *keep = new QGroupBox(tr("Saved sections"), this);
    auto *keepLayout = new QVBoxLayout(keep);
    keepLayout->addWidget(saved);
    keepLayout->addLayout(savedButtons);
    keepLayout->addWidget(feedback);

    auto *close = new QPushButton(tr("Close"), this);
    for (auto *button : {saveButton, showButton, removeButton, close}) {
        button->setAutoDefault(false);
    }
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(found, 3);
    layout->addWidget(keep, 1);
    layout->addWidget(close, 0, Qt::AlignRight);

    connect(maxPoints, &QDoubleSpinBox::valueChanged, this, &MenuSearchDialog::search);
    connect(location, &QComboBox::currentIndexChanged, this, &MenuSearchDialog::search);
    connect(results, &QTableWidget::itemSelectionChanged, this, [this] {
        saveButton->setEnabled(!selectedLocation().isEmpty());
    });
    connect(saveButton, &QPushButton::clicked, this, &MenuSearchDialog::saveSection);
    connect(saved, &QListWidget::itemSelectionChanged, this, [this] {
        const bool chosen = !saved->selectedItems().isEmpty();
        showButton->setEnabled(chosen);
        removeButton->setEnabled(chosen);
    });
    connect(saved, &QListWidget::itemDoubleClicked, this, &MenuSearchDialog::showSection);
    connect(showButton, &QPushButton::clicked, this, &MenuSearchDialog::showSection);
    connect(removeButton, &QPushButton::clicked, this, &MenuSearchDialog::removeSection);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);

    search();
    refreshSaved();
}

void MenuSearchDialog::search() {
    const int limit = static_cast<int>(std::lround(maxPoints->value() * 100.0));
    const QString area = location->currentData().toString();
    const auto found = MenuSearch::matches(ScuMenu::locations(), limit, area);
    results->clearSelection();
    results->setRowCount(static_cast<int>(found.size()));
    for (int row = 0; row < found.size(); ++row) {
        const auto &match = found.at(row);
        results->setItem(row, 0, new QTableWidgetItem(match.item));
        results->setItem(row, 1, new QTableWidgetItem(match.location));
        auto *price = new QTableWidgetItem(
            locale().toString(match.priceInHundredths / 100.0, 'f', POINT_DECIMALS));
        price->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        results->setItem(row, 2, price);
    }

    const QString limitText = locale().toString(maxPoints->value(), 'f', POINT_DECIMALS);
    QString text = found.isEmpty() ? tr("No meals cost %1 points or less.").arg(limitText)
                 : found.size() == 1 ? tr("1 meal costs %1 points or less.").arg(limitText)
                                     : tr("%1 meals cost %2 points or less, cheapest first.")
                                           .arg(found.size())
                                           .arg(limitText);
    const int receiptOnly = MenuSearch::receiptPricedCount(ScuMenu::locations(), area);
    if (receiptOnly == 1) {
        text += QLatin1Char(' ') + tr("1 item priced at the receipt total isn't shown.");
    } else if (receiptOnly > 1) {
        text += QLatin1Char(' ')
            + tr("%1 items priced at the receipt total aren't shown.").arg(receiptOnly);
    }
    status->setText(text);
    saveButton->setEnabled(!selectedLocation().isEmpty());
}

QString MenuSearchDialog::selectedLocation() const {
    const auto selected = results->selectedItems();
    if (!selected.isEmpty()) {
        return results->item(selected.first()->row(), 1)->text();
    }
    return location->currentData().toString();
}

void MenuSearchDialog::saveSection() {
    const QString section = selectedLocation();
    if (section.isEmpty()) {
        feedback->setText(tr("Select a meal or a location to save its section."));
        return;
    }
    if (LocalAccounts::saveSection(account, section) != LocalAccounts::Result::Success) {
        feedback->setText(tr("Could not save the section. Try again."));
        return;
    }
    refreshSaved();
    feedback->setText(tr("Saved %1.").arg(section));
}

void MenuSearchDialog::showSection() {
    const auto selected = saved->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int index = location->findData(selected.first()->text());
    if (index < 0) {
        feedback->setText(tr("%1 isn't on the current menu.").arg(selected.first()->text()));
        return;
    }
    location->setCurrentIndex(index);
    feedback->setText(tr("Showing %1.").arg(selected.first()->text()));
}

void MenuSearchDialog::removeSection() {
    const auto selected = saved->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const QString section = selected.first()->text();
    QMessageBox confirm(QMessageBox::Question, tr("Remove saved section?"),
                        tr("Remove %1 from your saved sections? You can save it again later.")
                            .arg(section),
                        QMessageBox::Cancel, this);
    confirm.setObjectName("confirmRemoveSection");
    QPushButton *removeChoice = confirm.addButton(tr("&Remove"), QMessageBox::DestructiveRole);
    confirm.setDefaultButton(QMessageBox::Cancel); // Enter alone doesn't remove anything.
    confirm.exec();
    if (confirm.clickedButton() != removeChoice) {
        feedback->setText(tr("%1 is still saved.").arg(section));
        return;
    }
    const auto result = LocalAccounts::removeSection(account, section);
    refreshSaved();
    feedback->setText(result == LocalAccounts::Result::Success
                          ? tr("Removed %1.").arg(section)
                          : result == LocalAccounts::Result::SectionUnavailable
                              ? tr("That section was already removed.")
                              : tr("Could not remove the section. Try again."));
}

void MenuSearchDialog::refreshSaved() {
    QStringList sections;
    const bool loaded = LocalAccounts::loadSavedSections(account, sections);
    saved->clear();
    saved->addItems(sections);
    showButton->setEnabled(false);
    removeButton->setEnabled(false);
    if (!loaded) {
        feedback->setText(tr("Could not load saved sections."));
    } else if (sections.isEmpty()) {
        feedback->setText(tr("No saved sections yet. Select a meal and choose Save this section."));
    }
}
