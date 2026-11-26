// Tests for finding meals within a point limit and saving menu sections (T14).
// Author: Nathan Gu

#include "doctest.h"
#include "menusearchdialog.h"
#include "test_account_store.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QApplication>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>

namespace {
// A small catalog with known prices, in hundredths of a point.
QList<ScuMenu::Location> sampleCatalog() {
    using ScuMenu::ReceiptPrice;
    return {
        {"Cafe", {{"Coffee", 300, "Size", {{"Small", 300}, {"Large", 500}}},
                  {"Cookie", 250},
                  {"Daily special"}}}, // Receipt total only.
        {"Grill", {{"Burger", 1000},
                   {"Fries", 1001},
                   {"Combo", 900, "Side", {{"Fries", 900}, {"Onion rings", ReceiptPrice}}}}},
        {"Closed station", {}},
    };
}

// Clicks the button labeled `choice` on the next confirmation box, once it opens, and
// records the box's question in `question`.
void answerConfirmation(const QString &choice, QString &question) {
    auto *timer = new QTimer;
    QObject::connect(timer, &QTimer::timeout, timer, [timer, choice, &question] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (!box) {
            return; // Not open yet.
        }
        question = box->text();
        for (QAbstractButton *button : box->buttons()) {
            if (button->text().remove('&') == choice) {
                button->click();
            }
        }
        timer->deleteLater();
    });
    timer->start(10);
}

QStringList names(const QList<MenuSearch::Match> &found) {
    QStringList list;
    for (const auto &match : found) {
        list << match.item;
    }
    return list;
}
}

TEST_CASE("Meal search includes the limit itself and excludes anything above it") {
    const auto catalog = sampleCatalog();
    // Exactly 10.00 points includes the 10.00 burger; the 10.01 fries are left out.
    const auto found = MenuSearch::matches(catalog, 1000);
    CHECK(names(found) == QStringList{"Cookie", "Coffee (Small)", "Coffee (Large)", "Combo (Fries)",
                                      "Burger"});
    CHECK(found.last().location == "Grill");
    CHECK(found.last().priceInHundredths == 1000);
    CHECK(names(MenuSearch::matches(catalog, 1001)).contains("Fries"));
    CHECK(names(MenuSearch::matches(catalog, 999)).size() == 4);
    CHECK(MenuSearch::matches(catalog, 249).isEmpty());
    CHECK(names(MenuSearch::matches(catalog, 250)) == QStringList{"Cookie"});
}

TEST_CASE("Meal search can be limited to one location and counts receipt-priced items") {
    const auto catalog = sampleCatalog();
    CHECK(names(MenuSearch::matches(catalog, 10000, "Grill"))
          == QStringList{"Combo (Fries)", "Burger", "Fries"});
    CHECK(MenuSearch::matches(catalog, 10000, "No such place").isEmpty());
    // "Daily special" and "Combo (Onion rings)" are priced at the receipt total.
    CHECK(MenuSearch::receiptPricedCount(catalog) == 2);
    CHECK(MenuSearch::receiptPricedCount(catalog, "Cafe") == 1);
    CHECK(MenuSearch::receiptPricedCount(catalog, "Closed station") == 0);
}

TEST_CASE("The Find meals dialog lists the real catalog within the limit") {
    TestAccountStore store;
    const auto account = store.createVerified("meals@scu.edu", "Hungry").accountId;
    MenuSearchDialog dialog(account);
    auto *limit = dialog.findChild<QDoubleSpinBox *>("maxPoints");
    auto *location = dialog.findChild<QComboBox *>("locationFilter");
    auto *results = dialog.findChild<QTableWidget *>("mealResults");
    auto *status = dialog.findChild<QLabel *>("searchStatus");
    REQUIRE(limit);
    REQUIRE(location);
    REQUIRE(results);
    REQUIRE(status);

    limit->setValue(5.0);
    const auto expected = MenuSearch::matches(ScuMenu::locations(), 500);
    REQUIRE(results->rowCount() == expected.size());
    REQUIRE_FALSE(expected.isEmpty());
    CHECK(results->item(0, 0)->text() == expected.first().item);
    CHECK(status->text().startsWith(QString::number(expected.size()) + " meals cost 5.00 points"));
    for (int row = 1; row < results->rowCount(); ++row) {
        CHECK(results->item(row - 1, 2)->text().toDouble() <= results->item(row, 2)->text().toDouble());
    }

    // Locations without items aren't offered as filters.
    CHECK(location->findData("Marketplace") < 0);
    const int fireGrill = location->findData("Marketplace / Fire Grill");
    REQUIRE(fireGrill > 0);
    location->setCurrentIndex(fireGrill);
    for (int row = 0; row < results->rowCount(); ++row) {
        CHECK(results->item(row, 1)->text() == "Marketplace / Fire Grill");
    }

    limit->setValue(0.0);
    CHECK(results->rowCount() == 0);
    CHECK(status->text().startsWith("No meals cost 0.00 points or less."));
}

// Author: Nathan Gu
TEST_CASE("Menu sections are saved from a result, shown again, and removed after confirming") {
    TestAccountStore store;
    const auto account = store.createVerified("sections@scu.edu", "Planner").accountId;
    {
        MenuSearchDialog dialog(account);
        auto *limit = dialog.findChild<QDoubleSpinBox *>("maxPoints");
        auto *results = dialog.findChild<QTableWidget *>("mealResults");
        auto *save = dialog.findChild<QPushButton *>("saveSectionButton");
        auto *saved = dialog.findChild<QListWidget *>("savedSections");
        auto *feedback = dialog.findChild<QLabel *>("sectionFeedback");
        REQUIRE(save);
        REQUIRE(saved);
        CHECK(saved->count() == 0);
        CHECK_FALSE(save->isEnabled()); // Nothing selected and no location filter.

        limit->setValue(100.0);
        REQUIRE(results->rowCount() > 0);
        results->selectRow(0);
        const QString section = results->item(0, 1)->text();
        CHECK(save->isEnabled());
        save->click();
        CHECK(feedback->text() == "Saved " + section + ".");
        REQUIRE(saved->count() == 1);
        CHECK(saved->item(0)->text() == section);
        save->click(); // Saving the same section again doesn't duplicate it.
        CHECK(saved->count() == 1);
    }

    // Saved sections persist, and choosing one shows its items.
    MenuSearchDialog dialog(account);
    auto *location = dialog.findChild<QComboBox *>("locationFilter");
    auto *saved = dialog.findChild<QListWidget *>("savedSections");
    auto *show = dialog.findChild<QPushButton *>("showSectionButton");
    auto *remove = dialog.findChild<QPushButton *>("removeSectionButton");
    REQUIRE(saved->count() == 1);
    const QString section = saved->item(0)->text();
    CHECK_FALSE(show->isEnabled());
    saved->item(0)->setSelected(true);
    show->click();
    CHECK(location->currentData().toString() == section);

    // Removing asks first; Cancel keeps the section.
    auto *feedback = dialog.findChild<QLabel *>("sectionFeedback");
    QString question;
    answerConfirmation("Cancel", question);
    remove->click();
    CHECK(question == "Remove " + section + " from your saved sections? You can save it again later.");
    CHECK(saved->count() == 1);
    CHECK(feedback->text() == section + " is still saved.");

    saved->item(0)->setSelected(true);
    answerConfirmation("Remove", question);
    remove->click();
    CHECK(saved->count() == 0);
    CHECK(feedback->text() == "Removed " + section + ".");
    QStringList stored;
    REQUIRE(LocalAccounts::loadSavedSections(account, stored));
    CHECK(stored.isEmpty());
}
