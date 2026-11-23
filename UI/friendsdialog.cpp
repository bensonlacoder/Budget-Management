#include "friendsdialog.h"
#include "localaccounts.h"

#include <QApplication>
#include <QClipboard>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

FriendsDialog::FriendsDialog(const QString &accountName, const QString &friendId, QWidget *parent)
    : QDialog(parent), accountName(accountName), requestId(new QLineEdit(this)),
      status(new QLabel(this)), requests(new QListWidget(this)),
      friends(new QTableWidget(0, 2, this)), noRequests(new QLabel(tr("No incoming requests."))),
      noFriends(new QLabel(tr("No friends yet."))) {
    setWindowTitle(tr("Friends"));
    resize(520, 500);
    status->setWordWrap(true);

    auto *ownId = new QLineEdit(friendId, this);
    ownId->setReadOnly(true);
    ownId->setObjectName("ownFriendId");
    requestId->setObjectName("requestFriendId");
    auto *copy = new QPushButton(tr("&Copy ID"), this);
    auto *send = new QPushButton(tr("&Send request"), this);
    auto *idRow = new QHBoxLayout;
    idRow->addWidget(ownId);
    idRow->addWidget(copy);
    auto *requestRow = new QHBoxLayout;
    requestRow->addWidget(requestId);
    requestRow->addWidget(send);
    auto *form = new QFormLayout;
    auto *ownIdLabel = new QLabel(tr("&Your friend ID"), this);
    ownIdLabel->setBuddy(ownId);
    auto *requestLabel = new QLabel(tr("&Friend's ID"), this);
    requestLabel->setBuddy(requestId);
    form->addRow(ownIdLabel, idRow);
    form->addRow(requestLabel, requestRow);

    auto *incoming = new QGroupBox(tr("Incoming requests"), this);
    requests->setObjectName("incomingRequests");
    auto *accept = new QPushButton(tr("&Accept"), incoming);
    auto *decline = new QPushButton(tr("&Decline"), incoming);
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

    auto *accepted = new QGroupBox(tr("Friends"), this);
    friends->setObjectName("acceptedFriends");
    friends->setHorizontalHeaderLabels({tr("Display name"), tr("Balance")});
    friends->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    friends->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    friends->verticalHeader()->hide();
    friends->setEditTriggers(QAbstractItemView::NoEditTriggers);
    friends->setSelectionMode(QAbstractItemView::NoSelection);
    auto *acceptedLayout = new QVBoxLayout(accepted);
    acceptedLayout->addWidget(noFriends);
    acceptedLayout->addWidget(friends);

    auto *reload = new QPushButton(tr("&Refresh"), this);
    auto *close = new QPushButton(tr("Close"), this);
    auto *footer = new QHBoxLayout;
    footer->addWidget(reload);
    footer->addStretch();
    footer->addWidget(close);
    for (auto *button : {copy, send, accept, decline, reload, close}) {
        button->setAutoDefault(false);
    }

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->addLayout(form);
    layout->addWidget(status);
    layout->addWidget(incoming, 1);
    layout->addWidget(accepted, 2);
    layout->addLayout(footer);

    connect(copy, &QPushButton::clicked, this, [this, friendId] {
        QApplication::clipboard()->setText(friendId);
        status->setText(tr("Friend ID copied."));
    });
    connect(send, &QPushButton::clicked, this, &FriendsDialog::sendRequest);
    connect(requestId, &QLineEdit::returnPressed, this, &FriendsDialog::sendRequest);
    connect(requests, &QListWidget::itemSelectionChanged, this, [this, accept, decline] {
        const bool selected = !requests->selectedItems().isEmpty();
        accept->setEnabled(selected);
        decline->setEnabled(selected);
    });
    connect(accept, &QPushButton::clicked, this, [this] { respond(true); });
    connect(decline, &QPushButton::clicked, this, [this] { respond(false); });
    connect(reload, &QPushButton::clicked, this, [this] {
        status->clear();
        refresh();
    });
    connect(close, &QPushButton::clicked, this, &QDialog::reject);

    refresh();
    requestId->setFocus();
}

void FriendsDialog::refresh() {
    LocalAccounts::Friends snapshot;
    const bool loaded = LocalAccounts::loadFriends(accountName, snapshot);
    requests->clear();
    friends->setRowCount(0);
    if (!loaded) {
        status->setText(tr("Could not load friends. Try refreshing."));
    }
    for (const auto &request : snapshot.incoming) {
        auto *item = new QListWidgetItem(tr("%1 (%2)").arg(request.displayName, request.friendId),
                                         requests);
        item->setData(Qt::UserRole, request.friendId);
    }
    friends->setRowCount(snapshot.accepted.size());
    int row = 0;
    for (const auto &entry : snapshot.accepted) {
        friends->setItem(row, 0, new QTableWidgetItem(entry.displayName));
        auto *balance = new QTableWidgetItem(tr("%1 points").arg(entry.balance, 0, 'f', 2));
        balance->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        friends->setItem(row, 1, balance);
        ++row;
    }
    noRequests->setVisible(loaded && snapshot.incoming.isEmpty());
    noFriends->setVisible(loaded && snapshot.accepted.isEmpty());
}

void FriendsDialog::sendRequest() {
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

void FriendsDialog::respond(bool accept) {
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
