#include "friendspage.h"
#include "appstyle.h"
#include "localaccounts.h"

#include <QApplication>
#include <QClipboard>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QTableWidget>
#include <QVBoxLayout>

FriendsPage::FriendsPage(const QString &accountName, const QString &friendId, QWidget *parent)
    : QWidget(parent), accountName(accountName), requestId(new QLineEdit(this)),
      status(new QLabel(this)), requests(new QListWidget(this)), sent(new QListWidget(this)),
      friends(new QTableWidget(0, 3, this)), noRequests(new QLabel(tr("No incoming requests."))),
      noSent(new QLabel(tr("No sent requests are waiting for an answer."))),
      noFriends(new QLabel(tr("No friends yet."))) {
    status->setObjectName("friendsStatus");
    noSent->setObjectName("noSentRequests");
    status->setWordWrap(true);

    auto *ownId = new QLineEdit(friendId, this);
    ownId->setReadOnly(true);
    ownId->setObjectName("ownFriendId");
    requestId->setObjectName("requestFriendId");
    auto *copy = new QPushButton(tr("C&opy ID"), this);
    auto *send = new QPushButton(tr("&Send request"), this);
    AppStyle::markPrimary(send);
    auto *idRow = new QHBoxLayout;
    idRow->addWidget(ownId);
    idRow->addWidget(copy);
    auto *requestRow = new QHBoxLayout;
    requestRow->addWidget(requestId);
    requestRow->addWidget(send);
    auto *form = new QFormLayout;
    auto *ownIdLabel = new QLabel(tr("&Your friend ID"), this);
    ownIdLabel->setBuddy(ownId);
    auto *requestLabel = new QLabel(tr("Friend's I&D"), this);
    requestLabel->setBuddy(requestId);
    form->addRow(ownIdLabel, idRow);
    form->addRow(requestLabel, requestRow);

    auto *incoming = new QGroupBox(tr("Incoming requests"), this);
    requests->setObjectName("incomingRequests");
    auto *accept = new QPushButton(tr("&Accept"), incoming);
    AppStyle::markPrimary(accept);
    auto *decline = new QPushButton(tr("D&ecline"), incoming);
    accept->setEnabled(false);
    decline->setEnabled(false);
    auto *actions = new QHBoxLayout;
    actions->addStretch();
    actions->addWidget(accept);
    actions->addWidget(decline);
    auto *incomingLayout = new QVBoxLayout(incoming);
    incomingLayout->addWidget(noRequests);
    incomingLayout->addWidget(requests);
    incomingLayout->addLayout(actions);

    auto *outgoing = new QGroupBox(tr("Sent requests"), this);
    sent->setObjectName("sentRequests");
    sent->setSelectionMode(QAbstractItemView::NoSelection);
    auto *outgoingLayout = new QVBoxLayout(outgoing);
    outgoingLayout->addWidget(noSent);
    outgoingLayout->addWidget(sent);

    auto *accepted = new QGroupBox(tr("Friends"), this);
    friends->setObjectName("acceptedFriends");
    friends->setHorizontalHeaderLabels({tr("Display name"), tr("Friend ID"), tr("Balance")});
    friends->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    friends->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    friends->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    friends->verticalHeader()->hide();
    friends->setEditTriggers(QAbstractItemView::NoEditTriggers);
    friends->setSelectionBehavior(QAbstractItemView::SelectRows);
    friends->setSelectionMode(QAbstractItemView::SingleSelection);
    auto *remove = new QPushButton(tr("Re&move friend"), accepted);
    remove->setObjectName("removeFriendButton");
    remove->setEnabled(false);
    auto *friendActions = new QHBoxLayout;
    friendActions->addStretch();
    friendActions->addWidget(remove);
    auto *acceptedLayout = new QVBoxLayout(accepted);
    acceptedLayout->addWidget(noFriends);
    acceptedLayout->addWidget(friends);
    acceptedLayout->addLayout(friendActions);

    auto *reload = new QPushButton(tr("Refres&h"), this);
    auto *footer = new QHBoxLayout;
    footer->addWidget(reload);
    footer->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(status);
    layout->addWidget(incoming, 1);
    layout->addWidget(outgoing, 1);
    layout->addWidget(accepted, 2);
    layout->addLayout(footer);

    connect(copy, &QPushButton::clicked, this, [this, friendId] {
        QApplication::clipboard()->setText(friendId);
        status->setText(tr("Friend ID copied."));
    });
    connect(send, &QPushButton::clicked, this, &FriendsPage::sendRequest);
    connect(requestId, &QLineEdit::returnPressed, this, &FriendsPage::sendRequest);
    connect(requests, &QListWidget::itemSelectionChanged, this, [this, accept, decline] {
        const bool selected = !requests->selectedItems().isEmpty();
        accept->setEnabled(selected);
        decline->setEnabled(selected);
    });
    connect(friends, &QTableWidget::itemSelectionChanged, this, [this, remove] {
        remove->setEnabled(!friends->selectedItems().isEmpty());
    });
    connect(remove, &QPushButton::clicked, this, &FriendsPage::removeFriend);
    connect(accept, &QPushButton::clicked, this, [this] { respond(true); });
    connect(decline, &QPushButton::clicked, this, [this] { respond(false); });
    connect(reload, &QPushButton::clicked, this, [this] {
        status->clear();
        refresh();
    });

    refresh();
    requestId->setFocus();
}

void FriendsPage::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}

void FriendsPage::refresh() {
    LocalAccounts::Friends snapshot;
    const bool loaded = LocalAccounts::loadFriends(accountName, snapshot);
    requests->clear();
    sent->clear();
    friends->setRowCount(0);
    if (!loaded) {
        status->setText(tr("Could not load friends. Try refreshing."));
    }
    for (const auto &request : snapshot.incoming) {
        auto *item = new QListWidgetItem(tr("%1 (%2)").arg(request.displayName, request.friendId),
                                         requests);
        item->setData(Qt::UserRole, request.friendId);
    }
    for (const auto &request : snapshot.outgoing) {
        new QListWidgetItem(tr("%1 (%2), waiting for an answer")
                                .arg(request.displayName, request.friendId),
                            sent);
    }
    friends->setRowCount(snapshot.accepted.size());
    int row = 0;
    for (const auto &entry : snapshot.accepted) {
        auto *name = new QTableWidgetItem(entry.displayName);
        name->setData(Qt::UserRole, entry.friendId);
        friends->setItem(row, 0, name);
        friends->setItem(row, 1, new QTableWidgetItem(entry.friendId));
        auto *balance = new QTableWidgetItem(AppStyle::points(entry.balance));
        balance->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        friends->setItem(row, 2, balance);
        ++row;
    }
    noRequests->setVisible(loaded && snapshot.incoming.isEmpty());
    noSent->setVisible(loaded && snapshot.outgoing.isEmpty());
    requests->setVisible(!snapshot.incoming.isEmpty());
    sent->setVisible(!snapshot.outgoing.isEmpty());
    noFriends->setVisible(loaded && snapshot.accepted.isEmpty());
}

void FriendsPage::sendRequest() {
    using LocalAccounts::Result;
    const QString id = requestId->text().trimmed();
    if (id.isEmpty()) {
        status->setText(tr("Enter a friend ID."));
        return;
    }
    switch (LocalAccounts::sendRequest(accountName, id)) {
    case Result::Success:
        status->setText(tr("Friend request sent."));
        requestId->clear();
        refresh();
        break;
    case Result::UnknownFriendId:
        status->setText(tr("No local account matches that friend ID."));
        break;
    case Result::OwnFriendId:
        status->setText(tr("Enter someone else's friend ID."));
        break;
    case Result::AlreadyFriends:
        status->setText(tr("You are already friends."));
        break;
    case Result::RequestAlreadySent:
        status->setText(tr("Your friend request is already pending."));
        break;
    case Result::IncomingRequestExists:
        status->setText(tr("This person has already sent you a request."));
        refresh();
        break;
    default:
        status->setText(tr("Could not send the friend request. Try again."));
        break;
    }
}

void FriendsPage::respond(bool accept) {
    const auto selected = requests->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const QString id = selected.first()->data(Qt::UserRole).toString();
    using LocalAccounts::Result;
    const auto result = LocalAccounts::respondToRequest(accountName, id, accept);
    if (result == Result::Success) {
        status->setText(accept ? tr("Friend request accepted.") : tr("Friend request declined."));
        refresh();
    } else if (result == Result::RequestUnavailable) {
        status->setText(tr("This request is no longer pending."));
        refresh();
    } else {
        status->setText(tr("Could not update the friend request. Try again."));
    }
}

void FriendsPage::removeFriend() {
    const auto selected = friends->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const QTableWidgetItem *nameItem = friends->item(selected.first()->row(), 0);
    const QString name = nameItem->text();
    const QString id = nameItem->data(Qt::UserRole).toString();
    QMessageBox confirm(QMessageBox::Question, tr("Remove friend?"),
                        tr("Remove %1 from your friends? You'll no longer see each other's "
                           "balances. Either of you can send a new request later.")
                            .arg(name),
                        QMessageBox::Cancel, this);
    confirm.setObjectName("confirmRemoveFriend");
    QPushButton *removeChoice = confirm.addButton(tr("&Remove"), QMessageBox::DestructiveRole);
    confirm.setDefaultButton(QMessageBox::Cancel); // Enter alone doesn't remove anyone.
    confirm.exec();
    if (confirm.clickedButton() != removeChoice) {
        status->setText(tr("%1 is still your friend.").arg(name));
        return;
    }
    using LocalAccounts::Result;
    switch (LocalAccounts::removeFriend(accountName, id)) {
    case Result::Success:
        status->setText(tr("Removed %1. You no longer see each other's balances.").arg(name));
        break;
    case Result::FriendUnavailable:
        status->setText(tr("%1 is no longer your friend.").arg(name));
        break;
    default:
        status->setText(tr("Could not remove %1. Try again.").arg(name));
        return;
    }
    refresh();
}
