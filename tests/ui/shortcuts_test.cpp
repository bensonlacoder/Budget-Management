// Keyboard shortcut (mnemonic) checks for the main window: every "&" in a label
// becomes an underlined Alt+letter, and no two controls a user can reach at once share
// a letter. Author: Nathan Gu

#include "doctest.h"
#include "chartspage.h"
#include "friendspage.h"
#include "mainui.h"
#include "planpage.h"
#include "profilepage.h"
#include "purchasespage.h"
#include "test_account_store.h"

#include <QAbstractButton>
#include <QKeySequence>
#include <QLabel>
#include <QTabWidget>

namespace {
// The Alt+letter a label or button text gives, or an empty string if it has none.
QString mnemonicOf(const QString &text) {
    return QKeySequence::mnemonic(text).toString();
}

// Every Alt+letter on one page, mapped to the texts that use it.
QMultiHash<QString, QString> pageMnemonics(QWidget *page) {
    QMultiHash<QString, QString> found;
    for (auto *label : page->findChildren<QLabel *>()) {
        if (label->buddy()) {
            found.insert(mnemonicOf(label->text()), label->text());
        }
    }
    for (auto *button : page->findChildren<QAbstractButton *>()) {
        found.insert(mnemonicOf(button->text()), button->text());
    }
    found.remove(QString());
    return found;
}
}

TEST_CASE("No label shows a literal & and no two main window shortcuts share a letter") {
    TestAccountStore store;
    const auto created = store.createVerified("keys@scu.edu", "Keys");
    REQUIRE_FALSE(created.accountId.isEmpty());
    const QString account = created.accountId;

    // Built the same way as main.cpp.
    MainUI window([] { return std::optional<MainUI::RealNumbers>({0.0, 20}); });
    window.setPurchasePage(new PurchasesPage(account, &window));
    window.addPage(new PlanPage(account, &window), "P&lan");
    window.addPage(new ChartsPage(account, &window), "&Charts");
    window.addPage(new FriendsPage(account, created.friendId, &window), "&Friends");
    window.addPage(new ProfilePage(account, &window), "P&rofile");

    // Tab titles work from every page.
    QMultiHash<QString, QString> everywhere;
    auto *tabs = window.findChild<QTabWidget *>();
    REQUIRE(tabs);
    for (int index = 0; index < tabs->count(); ++index) {
        everywhere.insert(mnemonicOf(tabs->tabText(index)), tabs->tabText(index));
    }
    for (const QString &key : everywhere.uniqueKeys()) {
        const std::string shortcut = key.toStdString();
        CAPTURE(shortcut);
        CHECK(everywhere.count(key) == 1);
    }

    for (int index = 0; index < tabs->count(); ++index) {
        QWidget *page = tabs->widget(index);
        const std::string tab = tabs->tabText(index).toStdString();
        CAPTURE(tab);

        // A label with a mnemonic but no buddy shows the "&" itself.
        for (auto *label : page->findChildren<QLabel *>()) {
            if (!label->buddy() && label->textFormat() != Qt::RichText) {
                const std::string text = label->text().toStdString();
                CAPTURE(text);
                CHECK(mnemonicOf(label->text()).isEmpty());
            }
        }

        const auto letters = pageMnemonics(page);
        for (const QString &key : letters.uniqueKeys()) {
            const std::string shortcut = key.toStdString();
            CAPTURE(shortcut);
            const std::string uses = letters.values(key).join(" | ").toStdString();
            CAPTURE(uses);
            CHECK(letters.count(key) == 1);
            CHECK_FALSE(everywhere.contains(key));
        }
    }
}
