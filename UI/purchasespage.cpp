#include "purchasespage.h"
#include "historylocations.h"
#include "quarter.h"

#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>
#include <QTimeZone>
#include <QVBoxLayout>

namespace {
constexpr int MENU_RETRY_INTERVAL_SECONDS = 15 * 60;
constexpr int MAX_MENU_BYTES = 8 * 1024 * 1024;
constexpr int ITEM_INDEX_ROLE = Qt::UserRole + 1;

using Kind = LocalAccounts::HistoryEntry::Kind;
using LocalAccounts::Result;

// New purchases are limited to the current quarter. `existing` is the saved date of a
// purchase being edited: an older one stays allowed, so opening and saving the edit
// doesn't silently move it into this quarter.
void configurePurchaseDate(QDateEdit *edit, const QDate &existing = {}) {
    const auto today = QDate::currentDate();
    const auto quarter = Quarter::statusOn(today);
    edit->setCalendarPopup(true);
    edit->setMaximumDate(today);
    if (quarter.phase == Quarter::Phase::InQuarter) {
        edit->setMinimumDate(existing.isValid() ? std::min(existing, quarter.term.start)
                                                : quarter.term.start);
    }
}

QString transactionError(Result result) {
    switch (result) {
    case Result::InsufficientPoints:
        return QObject::tr("This change would make your balance negative.");
    case Result::RefundTooLarge:
        return QObject::tr("Refunded points cannot exceed the purchase total.");
    case Result::RefundBeforePurchase:
        return QObject::tr("A refund can't be dated before its purchase.");
    case Result::InvalidPurchase:
        return QObject::tr("Enter a valid date, location and amount greater than zero.");
    case Result::InvalidAmount:
        return QObject::tr("Enter an amount greater than zero.");
    default:
        return QObject::tr("Could not save the change. Refresh the history and try again.");
    }
}

void showDialogError(QDialog &dialog, QFormLayout &fields, QLabel &error, Result result) {
    error.setText(transactionError(result));
    // The label spans the form. Reserve the wrapped height before relaying out the visible dialog.
    error.setMinimumHeight(error.heightForWidth(fields.contentsRect().width()));
    error.show();
    fields.invalidate();
    fields.activate();
    dialog.resize(dialog.size().expandedTo(fields.totalMinimumSize()));
}

QDateTime campusNow() {
    static const QTimeZone campusZone("America/Los_Angeles");
    return QDateTime::currentDateTimeUtc().toTimeZone(campusZone);
}

bool isOpen(const ScuMenu::Schedule &schedule, QTime now) {
    return std::any_of(schedule.hours.cbegin(), schedule.hours.cend(),
                       [now](const ScuMenu::Hours &hours) { return hours.includes(now); });
}
}

PurchasesPage::PurchasesPage(const QString &accountName, QWidget *parent)
    : QWidget(parent), accountName(accountName), date(new QDateEdit(QDate::currentDate(), this)),
      location(new QComboBox(this)), menuItem(new QComboBox(this)), form(new QFormLayout),
      choice(new QComboBox(this)), choiceLabel(new QLabel(this)), extras(new QWidget(this)),
      extrasLayout(new QGridLayout(extras)),
      item(new QLineEdit(this)), points(new QDoubleSpinBox(this)), balanceLabel(new QLabel(this)),
      hoursLabel(new QLabel(this)), menuStatus(new QLabel(this)), feedback(new QLabel(this)),
      historyStatus(new QLabel(this)), history(new QTableWidget(0, 4, this)),
      removePurchase(new QPushButton(tr("Remove selected purchase"), this)),
      editEntry(new QPushButton(tr("Edit selected"), this)),
      refundPurchase(new QPushButton(tr("Refund selected purchase"), this)),
      filterDates(new QCheckBox(tr("Filter by date"), this)),
      filterFrom(new QDateEdit(QDate::currentDate().addDays(-7), this)),
      filterTo(new QDateEdit(QDate::currentDate(), this)),
      filterLocation(new QComboBox(this)),
      network(new QNetworkAccessManager(this)) {
    QFont balanceFont = balanceLabel->font();
    balanceFont.setPointSize(14);
    balanceFont.setBold(true);
    balanceLabel->setFont(balanceFont);
    balanceLabel->setObjectName("purchaseBalance");

    date->setObjectName("purchaseDate");
    configurePurchaseDate(date);
    location->setObjectName("purchaseLocation");
    location->setEditable(true);
    location->setInsertPolicy(QComboBox::NoInsert);
    location->addItem(QString{});
    location->lineEdit()->setPlaceholderText(tr("Choose a campus location or type one"));
    location->lineEdit()->setMaxLength(LocalAccounts::PurchaseTextLimit);
    for (const auto &entry : ScuMenu::locations()) {
        location->addItem(entry.name);
    }
    hoursLabel->setObjectName("locationHours");
    hoursLabel->setTextFormat(Qt::PlainText);
    hoursLabel->setWordWrap(true);
    menuStatus->setObjectName("menuStatus");
    menuStatus->setWordWrap(true);
    choice->setObjectName("menuChoice");
    choiceLabel->setBuddy(choice);
    extrasLayout->setContentsMargins(0, 0, 0, 0);
    menuItem->setObjectName("scuMenuItem");
    item->setObjectName("purchaseItemName");
    item->setMaxLength(LocalAccounts::PurchaseTextLimit);
    item->setPlaceholderText(tr("Optional for manual entries"));
    points->setObjectName("purchasePoints");
    points->setDecimals(2);
    points->setMaximum(LocalAccounts::MaxPurchasePoints);
    points->setSpecialValueText(tr("Enter receipt total"));
    points->setSuffix(tr(" pts"));
    feedback->setWordWrap(true);
    feedback->setObjectName("purchaseFeedback");

    auto *entry = new QGroupBox(tr("Log a purchase"), this);
    entry->setLayout(form);
    form->addRow(tr("&Date"), date);
    form->addRow(tr("L&ocation"), location);
    form->addRow(QString(), hoursLabel);
    form->addRow(tr("SCU menu i&tem"), menuItem);
    form->addRow(choiceLabel, choice);
    form->addRow(tr("Add-ons"), extras);
    form->addRow(tr("&Item name"), item);
    form->addRow(tr("Points &spent"), points);
    auto *source = new QLabel(tr(
        "<a href=\"https://scudining.cafebonappetit.com/\">SCU menus and hours</a>"), entry);
    source->setObjectName("scuMenusAndHoursLink");
    source->setWordWrap(false);
    source->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    source->setOpenExternalLinks(true);
    auto *reload = new QPushButton(tr("Refresh hours"), entry);
    auto *save = new QPushButton(tr("Lo&g purchase"), entry);
    save->setObjectName("logPurchaseButton");
    save->setAutoDefault(false);
    // Menu links on the left, the form's action on the right, in one row.
    auto *sources = new QHBoxLayout;
    sources->addWidget(source);
    sources->addWidget(reload);
    sources->addStretch();
    sources->addWidget(save);
    form->addRow(menuStatus);
    form->addRow(sources);
    form->addRow(feedback);
    form->setRowVisible(feedback, false); // Shown by showFeedback() once there is text.

    auto *historyGroup = new QGroupBox(tr("Purchase history"), this);
    history->setObjectName("purchaseHistory");
    history->setHorizontalHeaderLabels({tr("Date"), tr("Location"), tr("Item"), tr("Balance change")});
    history->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    history->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    history->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    history->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    history->verticalHeader()->hide();
    history->setEditTriggers(QAbstractItemView::NoEditTriggers);
    history->setSelectionBehavior(QAbstractItemView::SelectRows);
    history->setSelectionMode(QAbstractItemView::SingleSelection);
    removePurchase->setEnabled(false);
    removePurchase->setObjectName("removePurchaseButton");
    auto *historyLayout = new QVBoxLayout(historyGroup);
    filterDates->setObjectName("historyDateFilter");
    filterFrom->setObjectName("historyFrom");
    filterTo->setObjectName("historyTo");
    filterLocation->setObjectName("historyLocation");
    filterFrom->setCalendarPopup(true);
    filterTo->setCalendarPopup(true);
    filterFrom->setEnabled(false);
    filterTo->setEnabled(false);
    // All filters in one row: dates, then location, then Clear.
    auto *filters = new QHBoxLayout;
    filters->addWidget(filterDates);
    filters->addWidget(filterFrom);
    filters->addWidget(new QLabel(tr("through"), this));
    filters->addWidget(filterTo);
    filters->addSpacing(12);
    auto *locationLabel = new QLabel(tr("Location"), this);
    locationLabel->setBuddy(filterLocation);
    filters->addWidget(locationLabel);
    filters->addWidget(filterLocation, 1);
    auto *clear = new QPushButton(tr("Clear filters"), this);
    clear->setObjectName("clearHistoryFilters");
    filters->addWidget(clear);
    historyLayout->addLayout(filters);
    historyStatus->setObjectName("historyStatus");
    historyLayout->addWidget(historyStatus);
    historyLayout->addWidget(history);
    editEntry->setObjectName("editHistoryButton");
    refundPurchase->setObjectName("linkedRefundButton");
    editEntry->setEnabled(false);
    refundPurchase->setEnabled(false);
    auto *refund = new QPushButton(tr("Log standalone refund"), this);
    refund->setObjectName("standaloneRefundButton");
    // Actions on the selected row on the left; the one that needs no selection apart.
    auto *actions = new QHBoxLayout;
    actions->addWidget(editEntry);
    actions->addWidget(removePurchase);
    actions->addWidget(refundPurchase);
    actions->addStretch();
    actions->addWidget(refund);
    historyLayout->addLayout(actions);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(balanceLabel);
    // The form keeps its natural height; any extra space goes to the history.
    entry->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    layout->addWidget(entry);
    layout->addWidget(historyGroup, 1);

    connect(location, &QComboBox::currentTextChanged, this, [this] {
        updateMenuItems();
        updateLocationHours();
    });
    connect(choice, &QComboBox::currentIndexChanged, this, &PurchasesPage::updateConfiguredPrice);
    connect(menuItem, &QComboBox::currentIndexChanged, this, &PurchasesPage::useMenuItem);
    connect(reload, &QPushButton::clicked, this, [this] { refreshHours(true); });
    connect(save, &QPushButton::clicked, this, &PurchasesPage::recordPurchase);
    connect(history, &QTableWidget::itemSelectionChanged, this, [this] {
        const auto *entry = selectedEntry();
        const bool editable = entry && entry->kind != Kind::Correction;
        editEntry->setEnabled(editable);
        removePurchase->setEnabled(editable);
        removePurchase->setText(entry && entry->kind == Kind::Refund
            ? tr("Remove selected refund") : tr("Remove selected purchase"));
        refundPurchase->setEnabled(entry && entry->kind == Kind::Purchase);
    });
    connect(removePurchase, &QPushButton::clicked, this, &PurchasesPage::removeSelectedEntry);
    connect(editEntry, &QPushButton::clicked, this, &PurchasesPage::editSelectedEntry);
    connect(refundPurchase, &QPushButton::clicked, this, &PurchasesPage::refundSelectedPurchase);
    connect(refund, &QPushButton::clicked, this, [this] {
        refundDialog({QDate::currentDate(), 0.01});
    });
    connect(filterDates, &QCheckBox::toggled, filterFrom, &QDateEdit::setEnabled);
    connect(filterDates, &QCheckBox::toggled, filterTo, &QDateEdit::setEnabled);
    connect(filterDates, &QCheckBox::toggled, this, &PurchasesPage::renderHistory);
    connect(filterFrom, &QDateEdit::dateChanged, this, &PurchasesPage::renderHistory);
    connect(filterTo, &QDateEdit::dateChanged, this, &PurchasesPage::renderHistory);
    connect(filterLocation, &QComboBox::currentIndexChanged, this, &PurchasesPage::renderHistory);
    connect(clear, &QPushButton::clicked, this, [this] {
        filterDates->setChecked(false);
        filterLocation->setCurrentIndex(0);
    });
    updateMenuItems();
    refreshHistory();
    location->setFocus();
    auto *clock = new QTimer(this);
    connect(clock, &QTimer::timeout, this, [this] { refreshHours(); });
    clock->start(60000);
    QTimer::singleShot(0, this, [this] { refreshHours(); });
}

void PurchasesPage::showFeedback(const QString &text) {
    feedback->setText(text);
    form->setRowVisible(feedback, !text.isEmpty());
}

void PurchasesPage::showBalance(double balance) {
    balanceLabel->setText(tr("Balance: %1 points").arg(locale().toString(balance, 'f', 2)));
    refreshHistory();
}

void PurchasesPage::updateMenuItems() {
    const QSignalBlocker blocker(menuItem);
    menuItem->clear();
    menuItem->addItem(tr("Enter receipt manually"));
    const QString name = selectedLocation();
    const auto &catalog = ScuMenu::locations();
    const bool marketplace = name == "Marketplace";
    for (int loc = 0; loc < catalog.size(); ++loc) {
        const auto &station = catalog.at(loc);
        if (station.name != name && !(marketplace && station.name.startsWith("Marketplace / "))) {
            continue;
        }
        for (int index = 0; index < station.items.size(); ++index) {
            const auto &option = station.items.at(index);
            QString label = option.name;
            if (marketplace) {
                label = station.name.section(" / ", 1) + " / " + label;
            }
            if (option.priceInHundredths == ScuMenu::ReceiptPrice) {
                label += tr(" (receipt total)");
            } else {
                const QString price = locale().toString(option.priceInHundredths / 100.0, 'f', 2);
                label += option.choices.isEmpty() ? tr(" (%1 pts)").arg(price) : tr(" (from %1 pts)").arg(price);
            }
            menuItem->addItem(label, loc);
            menuItem->setItemData(menuItem->count() - 1, index, ITEM_INDEX_ROLE);
        }
    }
    menuItem->setEnabled(menuItem->count() > 1);
    // Preserve an in-progress manual receipt while its location is being typed.
    useMenuItem(0);
}

void PurchasesPage::refreshHours(bool force) {
    const QDateTime now = campusNow();
    if (menuDate != now.date()) {
        date->setMaximumDate(QDate::currentDate());
        const bool carriedToday = menuDate.isValid() && date->date() == menuDate;
        menuDate = now.date();
        dailyMenus.clear();
        lastMenuRequest = {};
        if (carriedToday) {
            date->setDate(QDate::currentDate());
        }
    }
    updateLocationHours();
    if (!force && lastMenuRequest.isValid() && lastMenuRequest.secsTo(now) < MENU_RETRY_INTERVAL_SECONDS) {
        return;
    }
    lastMenuRequest = now;

    for (const auto &cafe : ScuMenu::cafes()) {
        if (pendingMenus.contains(cafe.slug) ||
            (!force && dailyMenus.contains(cafe.slug) && dailyMenus[cafe.slug].received)) {
            continue;
        }
        QUrl url("https://scudining.cafebonappetit.com");
        const QDate requestedDay = menuDate;
        url.setPath(QString("/cafe/%1/%2/").arg(cafe.slug, requestedDay.toString(Qt::ISODate)));
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
        request.setTransferTimeout(12000);
        auto *reply = network->get(request);
        pendingMenus.insert(cafe.slug);
        connect(reply, &QNetworkReply::readyRead, reply, [reply] {
            if (reply->bytesAvailable() > MAX_MENU_BYTES) {
                reply->abort();
            }
        });
        connect(reply, &QNetworkReply::finished, this, [this, reply, cafe, requestedDay] {
            pendingMenus.remove(cafe.slug);
            if (requestedDay == menuDate && reply->error() == QNetworkReply::NoError &&
                reply->url().path().endsWith("/" + requestedDay.toString(Qt::ISODate) + "/")) {
                const auto menu = ScuMenu::parseDailyMenu(reply->readAll(), cafe, requestedDay);
                if (menu.received) {
                    dailyMenus.insert(cafe.slug, menu);
                }
            }
            reply->deleteLater();
            updateLocationHours();
        });
    }
}

ScuMenu::Schedule PurchasesPage::locationSchedule(const QString &name) const {
    const auto cafe = dailyMenus.constFind(ScuMenu::cafeSlug(name));
    if (cafe == dailyMenus.cend()) {
        return {};
    }
    if (!name.contains(" / ") || (cafe->cafeSchedule.published && cafe->cafeSchedule.hours.isEmpty())) {
        return cafe->cafeSchedule;
    }
    if (!cafe->specialHours) {
        return cafe->stations.value(name);
    }
    return {};
}

QString PurchasesPage::hoursText(const ScuMenu::Schedule &schedule, QTime now) const {
    if (!schedule.published) {
        return tr("Hours unavailable");
    }
    if (schedule.hours.isEmpty()) {
        return tr("Closed today");
    }

    QStringList hours;
    for (const auto &window : schedule.hours) {
        const QString range = tr("%1 to %2").arg(locale().toString(window.opens, QLocale::ShortFormat),
                                                  locale().toString(window.closes, QLocale::ShortFormat));
        if (!hours.contains(range)) {
            hours.append(range);
        }
    }
    const QString status = isOpen(schedule, now) ? tr("Open now") : tr("Closed now");
    return schedule.note.isEmpty() ? tr("%1 (%2)").arg(status, hours.join(", "))
                                   : tr("%1 (%2, %3)").arg(status, hours.join(", "), schedule.note);
}

void PurchasesPage::updateLocationHours() {
    const QTime now = campusNow().time();
    const auto openIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);
    const auto closedIcon = style()->standardIcon(QStyle::SP_DialogCancelButton);
    for (int index = 1; index < location->count(); ++index) {
        const auto schedule = locationSchedule(location->itemText(index));
        location->setItemData(index, hoursText(schedule, now), Qt::ToolTipRole);
        location->setItemIcon(index, !schedule.published ? QIcon{} : isOpen(schedule, now) ? openIcon : closedIcon);
    }
    const QString name = selectedLocation();
    hoursLabel->setText(name.isEmpty() ? QString{} : hoursText(locationSchedule(name), now));
    form->setRowVisible(hoursLabel, !name.isEmpty());
}

QString PurchasesPage::selectedLocation() const {
    const QString entered = location->currentText().trimmed();
    const int match = location->findText(entered, Qt::MatchFixedString);
    return match > 0 ? location->itemText(match) : entered;
}

void PurchasesPage::useMenuItem(int index) {
    const QSignalBlocker blocker(choice);
    choice->clear();
    qDeleteAll(extraChecks);
    extraChecks.clear();
    if (currentItem) {
        item->clear();
        points->setValue(0);
    }
    currentItem = nullptr;
    if (index > 0) {
        const auto &station = ScuMenu::locations().at(menuItem->itemData(index).toInt());
        currentItem = &station.items.at(menuItem->itemData(index, ITEM_INDEX_ROLE).toInt());
        choiceLabel->setText(currentItem->choiceLabel);
        for (const auto &option : currentItem->choices) {
            const QString label = option.priceInHundredths == ScuMenu::ReceiptPrice
                ? tr("%1 (receipt total)").arg(option.name)
                : tr("%1 (%2 pts)").arg(option.name, locale().toString(option.priceInHundredths / 100.0, 'f', 2));
            choice->addItem(label);
        }
        for (const auto &extra : currentItem->extras) {
            const QString price = locale().toString(extra.priceInHundredths / 100.0, 'f', 2);
            auto *check = new QCheckBox(tr("%1 (+%2 pts)").arg(extra.name, price), extras);
            const int pos = extraChecks.size();
            extrasLayout->addWidget(check, pos / 2, pos % 2);
            extraChecks.append(check);
            connect(check, &QCheckBox::toggled, this, &PurchasesPage::updateConfiguredPrice);
        }
        updateConfiguredPrice();
    } else {
        const QString name = selectedLocation();
        QString note = tr("Choose a menu item or enter the total from your receipt.");
        for (const auto &station : ScuMenu::locations()) {
            if (station.name == name) {
                note = station.reference.isEmpty() ? tr("Lunch reference menu for Fri Nov 20, 2026.") : station.reference;
                break;
            }
        }
        menuStatus->setText(note);
    }
    form->setRowVisible(choice, choice->count() > 0);
    form->setRowVisible(extras, !extraChecks.isEmpty());
    form->setRowVisible(menuStatus, !selectedLocation().isEmpty());
}

void PurchasesPage::updateConfiguredPrice() {
    if (!currentItem) {
        return;
    }
    int total = currentItem->priceInHundredths;
    QStringList details;
    if (choice->currentIndex() >= 0) {
        const auto &variant = currentItem->choices.at(choice->currentIndex());
        details.append(variant.name);
        total = variant.priceInHundredths;
    }
    for (int i = 0; i < extraChecks.size(); ++i) {
        if (extraChecks.at(i)->isChecked()) {
            const auto &extra = currentItem->extras.at(i);
            details.append(extra.name);
            if (total != ScuMenu::ReceiptPrice) {
                total += extra.priceInHundredths;
            }
        }
    }
    item->setText(details.isEmpty() ? currentItem->name
                                    : tr("%1 (%2)").arg(currentItem->name, details.join(", ")));
    points->setValue(total == ScuMenu::ReceiptPrice ? 0 : total / 100.0);
    const auto &station = ScuMenu::locations().at(menuItem->currentData().toInt());
    QString reference = currentItem->reference.isEmpty() ? station.reference : currentItem->reference;
    if (reference.isEmpty()) {
        reference = tr("Lunch menu, Fri Nov 20, 2026");
    }
    menuStatus->setText(reference + (total == ScuMenu::ReceiptPrice
        ? tr(". Enter the full amount charged on your receipt.")
        : tr(". Adjust the total if your receipt differs.")));
}

void PurchasesPage::recordPurchase() {
    const QString venue = currentItem ? ScuMenu::locations().at(menuItem->currentData().toInt()).name
                                      : selectedLocation();
    const LocalAccounts::Purchase purchase{date->date(), venue, item->text(), points->value()};
    double newBalance = 0.0;
    using LocalAccounts::Result;
    switch (LocalAccounts::recordPurchase(accountName, purchase, newBalance)) {
    case Result::Success:
        showBalance(newBalance);
        emit balanceUpdated(newBalance);
        menuItem->setCurrentIndex(0);
        item->clear();
        points->setValue(0.0);
        showFeedback(tr("Purchase logged."));
        break;
    case Result::InvalidPurchase:
        showFeedback(tr("Enter a location, a date up to today, and an amount greater than zero."));
        break;
    case Result::InsufficientPoints:
        showFeedback(tr("This purchase costs more points than your saved balance."));
        break;
    default:
        showFeedback(tr("Could not save the purchase. Try again."));
        break;
    }
}

const LocalAccounts::HistoryEntry *PurchasesPage::selectedEntry() const {
    const auto selection = history->selectionModel()->selectedRows();
    if (selection.isEmpty() || selection.first().row() >= visibleEntries.size()) {
        return nullptr;
    }
    return &visibleEntries.at(selection.first().row());
}

void PurchasesPage::removeSelectedEntry() {
    const auto *selected = selectedEntry();
    if (!selected || selected->kind == Kind::Correction) {
        return;
    }
    const auto entry = *selected;
    const bool refund = entry.kind == Kind::Refund;
    const auto answer = QMessageBox::question(
        this, tr("Remove history entry?"),
        refund ? tr("Remove this refund and deduct its points from your balance?")
               : tr("Remove this purchase and its linked refunds? Only points still unrefunded are returned."),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes) {
        return;
    }

    double newBalance = 0.0;
    const auto result = refund
        ? LocalAccounts::removeRefund(accountName, entry.id, newBalance)
        : LocalAccounts::removePurchase(accountName,
            {entry.date, entry.location, entry.item, -entry.balanceChange, entry.id}, newBalance);
    if (result == Result::Success) {
        showBalance(newBalance);
        emit balanceUpdated(newBalance);
        showFeedback(refund ? tr("Refund removed.") : tr("Purchase removed."));
    } else {
        showFeedback(transactionError(result));
        refreshHistory();
    }
}

void PurchasesPage::editSelectedEntry() {
    const auto *selected = selectedEntry();
    if (!selected || selected->kind == Kind::Correction) {
        return;
    }
    const auto entry = *selected;
    if (entry.kind == Kind::Refund) {
        refundDialog({entry.date, entry.balanceChange, entry.purchaseId, entry.item, entry.id});
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit purchase"));
    QFormLayout fields(&dialog);
    QDateEdit when(entry.date, &dialog);
    when.setObjectName("editPurchaseDate");
    configurePurchaseDate(&when, entry.date);
    QLineEdit venue(entry.location, &dialog);
    venue.setObjectName("editPurchaseLocation");
    venue.setMaxLength(LocalAccounts::PurchaseTextLimit);
    QLineEdit name(entry.item, &dialog);
    name.setObjectName("editPurchaseItem");
    name.setMaxLength(LocalAccounts::PurchaseTextLimit);
    QDoubleSpinBox amount(&dialog);
    amount.setObjectName("editPurchasePoints");
    amount.setRange(0.01, LocalAccounts::MaxPurchasePoints);
    amount.setValue(-entry.balanceChange);
    QLabel error(&dialog);
    error.setObjectName("editPurchaseError");
    error.setTextFormat(Qt::PlainText);
    error.setWordWrap(true);
    error.hide();
    QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    fields.addRow(tr("Date"), &when);
    fields.addRow(tr("Location"), &venue);
    fields.addRow(tr("Item"), &name);
    fields.addRow(tr("Points spent"), &amount);
    fields.addRow(&error);
    fields.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    double newBalance = 0.0;
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        const auto result = LocalAccounts::editPurchase(accountName,
            {when.date(), venue.text(), name.text(), amount.value(), entry.id}, newBalance);
        if (result == Result::Success) {
            dialog.accept();
        } else {
            showDialogError(dialog, fields, error, result);
        }
    });
    if (dialog.exec() == QDialog::Accepted) {
        showBalance(newBalance);
        emit balanceUpdated(newBalance);
        showFeedback(tr("Purchase updated."));
    }
}

void PurchasesPage::refundSelectedPurchase() {
    const auto *entry = selectedEntry();
    if (!entry || entry->kind != Kind::Purchase) {
        return;
    }
    double available = -entry->balanceChange;
    for (const auto &row : entries) {
        if (row.purchaseId == entry->id) {
            available -= row.balanceChange;
        }
    }
    if (available < 0.005) {
        showFeedback(tr("This purchase has already been fully refunded."));
        return;
    }
    refundDialog({QDate::currentDate(), available, entry->id});
}

void PurchasesPage::refundDialog(const LocalAccounts::Refund &refund) {
    QDialog dialog(this);
    dialog.setWindowTitle(refund.id.isEmpty() ? tr("Log refund") : tr("Edit refund"));
    QFormLayout fields(&dialog);
    QDateEdit when(refund.date, &dialog);
    when.setObjectName("refundDate");
    when.setCalendarPopup(true);
    when.setMaximumDate(QDate::currentDate());
    // A linked refund can't come before its purchase.
    for (const auto &row : std::as_const(entries)) {
        if (row.kind == Kind::Purchase && row.id == refund.purchaseId) {
            when.setMinimumDate(row.date);
        }
    }
    QDoubleSpinBox amount(&dialog);
    amount.setObjectName("refundPoints");
    amount.setRange(0.01, LocalAccounts::MaxPurchasePoints);
    amount.setValue(refund.points);
    QLineEdit note(refund.note, &dialog);
    note.setObjectName("refundNote");
    note.setMaxLength(LocalAccounts::PurchaseTextLimit);
    QLabel error(&dialog);
    error.setObjectName("refundError");
    error.setTextFormat(Qt::PlainText);
    error.setWordWrap(true);
    error.hide();
    QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    fields.addRow(tr("Date"), &when);
    fields.addRow(tr("Points returned"), &amount);
    fields.addRow(tr("Note"), &note);
    fields.addRow(&error);
    fields.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    double newBalance = 0.0;
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        const LocalAccounts::Refund updated{
            when.date(), amount.value(), refund.purchaseId, note.text(), refund.id};
        const auto result = refund.id.isEmpty()
            ? LocalAccounts::recordRefund(accountName, updated, newBalance)
            : LocalAccounts::editRefund(accountName, updated, newBalance);
        if (result == Result::Success) {
            dialog.accept();
        } else {
            showDialogError(dialog, fields, error, result);
        }
    });
    if (dialog.exec() == QDialog::Accepted) {
        showBalance(newBalance);
        emit balanceUpdated(newBalance);
        showFeedback(refund.id.isEmpty() ? tr("Refund logged.") : tr("Refund updated."));
    }
}

void PurchasesPage::refreshHistory() {
    historyLoaded = LocalAccounts::loadHistory(accountName, entries);
    resolveRefundLocations(entries);
    const QString chosen = filterLocation->currentData().toString();
    const QSignalBlocker blocker(filterLocation);
    filterLocation->clear();
    filterLocation->addItem(tr("All locations"), QString());
    QSet<QString> locations;
    for (const auto &entry : entries) {
        if (!entry.location.isEmpty()) {
            locations.insert(entry.location);
        }
    }
    auto sorted = locations.values();
    sorted.sort(Qt::CaseInsensitive);
    for (const auto &venue : sorted) {
        filterLocation->addItem(venue, venue);
    }
    if (!chosen.isEmpty() && filterLocation->findData(chosen) < 0) {
        filterLocation->addItem(chosen, chosen);
    }
    filterLocation->setCurrentIndex(std::max(0, filterLocation->findData(chosen)));
    renderHistory();
}

void PurchasesPage::renderHistory() {
    history->clearSelection();
    visibleEntries.clear();
    const auto venue = filterLocation->currentData().toString();
    const bool dateRange = filterDates->isChecked();
    const bool reversed = dateRange && filterFrom->date() > filterTo->date();
    for (const auto &entry : entries) {
        if ((!venue.isEmpty() && entry.location != venue)
            || (dateRange && (entry.date < filterFrom->date() || entry.date > filterTo->date()))) {
            continue;
        }
        visibleEntries.append(entry);
    }
    history->setRowCount(visibleEntries.size());
    for (int row = 0; row < visibleEntries.size(); ++row) {
        const auto &entry = visibleEntries.at(row);
        QString description = entry.item;
        if (entry.kind == Kind::Correction) {
            description = tr("Balance correction");
        } else if (entry.kind == Kind::Refund) {
            description = entry.item.isEmpty() ? tr("Refund") : tr("Refund — %1").arg(entry.item);
        }
        const QString change = (entry.balanceChange > 0 ? QStringLiteral("+") : QString())
            + locale().toString(entry.balanceChange, 'f', 2);
        const QString values[] = {
            locale().toString(entry.date, QLocale::ShortFormat), entry.location, description, change};
        for (int column = 0; column < 4; ++column) {
            auto *cell = new QTableWidgetItem(values[column]);
            if (entry.kind == Kind::Correction) {
                auto font = cell->font();
                font.setItalic(true);
                cell->setFont(font);
            }
            if (column == 3) {
                cell->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            history->setItem(row, column, cell);
        }
    }
    historyStatus->setVisible(!historyLoaded || visibleEntries.isEmpty());
    if (!historyLoaded) {
        historyStatus->setText(tr("Could not load purchase history."));
    } else if (reversed) {
        historyStatus->setText(tr("Choose a start date on or before the end date."));
    } else if (dateRange || !venue.isEmpty()) {
        historyStatus->setText(tr("No history matches these filters."));
    } else {
        historyStatus->setText(tr("No purchases, refunds or balance corrections yet."));
    }
}
