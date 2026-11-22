#include "doctest.h"
#include "purchasespage.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QUuid>

namespace {
// Give the real purchase store its own disposable application directory.
struct TestAccountStore {
    QString previous = qApp->applicationName();
    TestAccountStore() {
        qApp->setApplicationName("DiningMenuTest-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    ~TestAccountStore() {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
        QFile::remove(dir.filePath("accounts.json"));
        QDir().rmdir(dir.absolutePath());
        qApp->setApplicationName(previous);
    }
};

void selectItem(QComboBox *menu, const QString &prefix) {
    const int index = menu->findText(prefix, Qt::MatchStartsWith);
    REQUIRE(index > 0);
    menu->setCurrentIndex(index);
}
}

TEST_CASE("Lunch purchases use station prices, selected options and receipt totals") {
    TestAccountStore store;
    LocalAccounts::Profile profile;
    REQUIRE(LocalAccounts::create("lunch_test", "test-password", "Student", "basic", profile)
            == LocalAccounts::Result::Success);
    PurchasesPage page("lunch_test");
    auto *location = page.findChild<QComboBox *>("purchaseLocation");
    auto *menu = page.findChild<QComboBox *>("scuMenuItem");
    auto *choice = page.findChild<QComboBox *>("menuChoice");
    auto *points = page.findChild<QDoubleSpinBox *>("purchasePoints");
    auto *date = page.findChild<QDateEdit *>("purchaseDate");
    auto *save = page.findChild<QPushButton *>("logPurchaseButton");
    REQUIRE(location);
    REQUIRE(menu);
    REQUIRE(choice);
    REQUIRE(points);
    REQUIRE(date);
    REQUIRE(save);

    location->setCurrentText("Marketplace");
    selectItem(menu, "Trattoria / Build your own pasta");
    CHECK(points->value() == doctest::Approx(8.25));
    choice->setCurrentIndex(1);
    int proteins = 0;
    for (auto *extra : page.findChildren<QCheckBox *>()) {
        if (extra->text().startsWith("Grilled chicken") || extra->text().startsWith("Shrimp")) {
            extra->click();
            ++proteins;
        }
    }
    REQUIRE(proteins == 2);
    CHECK(points->value() == doctest::Approx(18.10));
    date->setDate(QDate::currentDate().addDays(-1));
    CHECK(points->value() == doctest::Approx(18.10));
    double balance = profile.balance;
    QObject::connect(&page, &PurchasesPage::balanceUpdated, [&](double value) { balance = value; });
    save->click();
    QList<LocalAccounts::Purchase> history;
    REQUIRE(LocalAccounts::loadPurchases("lunch_test", history));
    REQUIRE(history.size() == 1);
    CHECK(history.first().location == "Marketplace / Trattoria");
    CHECK(history.first().item == "Build your own pasta (Full, Grilled chicken, Shrimp)");
    CHECK(history.first().points == doctest::Approx(18.10));
    CHECK(history.first().date == date->date());
    CHECK(balance == doctest::Approx(profile.balance - 18.10));

    location->setCurrentText("Marketplace / Global Grill");
    selectItem(menu, "BYO burger");
    choice->setCurrentIndex(2);
    CHECK(points->value() == 0);
    save->click();
    CHECK(balance == doctest::Approx(profile.balance - 18.10));
    points->setValue(16.33);
    save->click();
    REQUIRE(LocalAccounts::loadPurchases("lunch_test", history));
    REQUIRE(history.size() == 2);
    CHECK(history.first().item == "BYO burger (Curly fries)");
    CHECK(balance == doctest::Approx(profile.balance - 18.10 - 16.33));

    location->setCurrentText("Mission Bakery & Cafe");
    selectItem(menu, "Hot Coffee (");
    choice->setCurrentIndex(2);
    CHECK(points->value() == doctest::Approx(4.05));
    location->setCurrentText("Sidebar Cafe");
    CHECK(points->value() == 0);
    selectItem(menu, "Hot Coffee (");
    choice->setCurrentIndex(2);
    CHECK(points->value() == doctest::Approx(4.20));

    location->setCurrentText("Fresh Bytes");
    selectItem(menu, "Brown sugar boba");
    choice->setCurrentIndex(1);
    auto *topping = page.findChild<QCheckBox *>();
    REQUIRE(topping);
    topping->click();
    CHECK(points->value() == doctest::Approx(9.30));
    CHECK(page.findChild<QLabel *>("menuStatus")->text().contains("Nov 13"));
    selectItem(menu, "Chocolate chess pie");
    CHECK(points->value() == doctest::Approx(6.00));
    CHECK(page.findChild<QLabel *>("menuStatus")->text().contains("Nov 20"));

}
