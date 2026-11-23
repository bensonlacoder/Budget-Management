#include "purchasespage.h"

#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
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
      network(new QNetworkAccessManager(this)) {
    QFont balanceFont = balanceLabel->font();
    balanceFont.setPointSize(14);
    balanceFont.setBold(true);
    balanceLabel->setFont(balanceFont);
    balanceLabel->setObjectName("purchaseBalance");

    date->setObjectName("purchaseDate");
    date->setCalendarPopup(true);
    date->setMaximumDate(QDate::currentDate());
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
    form->addRow(tr("&Location"), location);
    form->addRow(QString(), hoursLabel);
    form->addRow(tr("SCU &menu item"), menuItem);
    form->addRow(choiceLabel, choice);
    form->addRow(tr("Add-ons"), extras);
    form->addRow(tr("&Item name"), item);
    form->addRow(tr("&Points spent"), points);
    auto *source = new QLabel(tr(
        "<a href=\"https://scudining.cafebonappetit.com/\">SCU menus and hours</a>"), entry);
    source->setObjectName("scuMenusAndHoursLink");
    source->setWordWrap(false);
    source->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    source->setOpenExternalLinks(true);
    auto *reload = new QPushButton(tr("Refresh hours"), entry);
    auto *sources = new QHBoxLayout;
    sources->addWidget(source);
    sources->addStretch();
    sources->addWidget(reload);
    form->addRow(menuStatus);
    form->addRow(sources);
    auto *save = new QPushButton(tr("&Log purchase"), entry);
    save->setObjectName("logPurchaseButton");
    save->setAutoDefault(false);
    form->addRow(save);
    form->addRow(feedback);

    auto *historyGroup = new QGroupBox(tr("Purchase history"), this);
    history->setObjectName("purchaseHistory");
    history->setHorizontalHeaderLabels({tr("Date"), tr("Location"), tr("Item"), tr("Points spent")});
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
    historyLayout->addWidget(historyStatus);
    historyLayout->addWidget(history);
    historyLayout->addWidget(removePurchase, 0, Qt::AlignRight);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(balanceLabel);
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
        removePurchase->setEnabled(!history->selectedItems().isEmpty());
    });
    connect(removePurchase, &QPushButton::clicked, this, &PurchasesPage::removeSelectedPurchase);
    updateMenuItems();
    refreshHistory();
    location->setFocus();
    auto *clock = new QTimer(this);
    connect(clock, &QTimer::timeout, this, [this] { refreshHours(); });
    clock->start(60000);
    QTimer::singleShot(0, this, [this] { refreshHours(); });
}

void PurchasesPage::showBalance(double balance) {
    balanceLabel->setText(tr("Balance: %1 points").arg(locale().toString(balance, 'f', 2)));
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
    hoursLabel->setVisible(!name.isEmpty());
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
    menuStatus->setVisible(!selectedLocation().isEmpty());
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
        emit balanceUpdated(newBalance);
        refreshHistory();
        menuItem->setCurrentIndex(0);
        item->clear();
        points->setValue(0.0);
        feedback->setText(tr("Purchase logged."));
        break;
    case Result::InvalidPurchase:
        feedback->setText(tr("Enter a location, a date up to today, and an amount greater than zero."));
        break;
    case Result::InsufficientPoints:
        feedback->setText(tr("This purchase costs more points than your saved balance."));
        break;
    default:
        feedback->setText(tr("Could not save the purchase. Try again."));
        break;
    }
}

void PurchasesPage::removeSelectedPurchase() {
    const auto selected = history->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const auto purchase = purchases.at(selected.first()->row());
    const auto answer = QMessageBox::question(
        this, tr("Remove purchase?"),
        tr("Remove this purchase and return %1 points to your balance?")
            .arg(locale().toString(purchase.points, 'f', 2)),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes) {
        return;
    }

    double newBalance = 0.0;
    using LocalAccounts::Result;
    const Result result = LocalAccounts::removePurchase(accountName, purchase, newBalance);
    if (result == Result::Success) {
        emit balanceUpdated(newBalance);
        refreshHistory();
        feedback->setText(tr("Purchase removed. Points returned."));
    } else if (result == Result::PurchaseUnavailable) {
        refreshHistory();
        feedback->setText(tr("That purchase has already been removed."));
    } else {
        feedback->setText(tr("Could not remove the purchase. Try again."));
    }
}

void PurchasesPage::refreshHistory() {
    const bool loaded = LocalAccounts::loadPurchases(accountName, purchases);
    history->clearSelection();
    history->setRowCount(purchases.size());
    int row = 0;
    for (const auto &purchase : purchases) {
        history->setItem(row, 0, new QTableWidgetItem(locale().toString(purchase.date, QLocale::ShortFormat)));
        history->setItem(row, 1, new QTableWidgetItem(purchase.location));
        history->setItem(row, 2, new QTableWidgetItem(purchase.item));
        auto *cost = new QTableWidgetItem(locale().toString(purchase.points, 'f', 2));
        cost->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        history->setItem(row, 3, cost);
        ++row;
    }
    if (!loaded) {
        historyStatus->setText(tr("Could not load purchase history."));
    } else if (purchases.isEmpty()) {
        historyStatus->setText(tr("No purchases logged yet."));
    }
    historyStatus->setVisible(!loaded || purchases.isEmpty());
}
