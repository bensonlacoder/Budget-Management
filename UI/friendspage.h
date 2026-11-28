#ifndef FRIENDSPAGE_H
#define FRIENDSPAGE_H

#include <QWidget>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QShowEvent;
class QTableWidget;

// Friends tab: the user's friend ID, sending and answering requests, pending sent
// requests, and accepted friends' balances, with removing a friend (T05).
class FriendsPage : public QWidget {
    Q_OBJECT

public:
    FriendsPage(const QString &accountName, const QString &friendId, QWidget *parent = nullptr);
    // Reloads requests and friends' balances.
    void refresh();

protected:
    // Friends' balances change in their own accounts, so the tab reloads each time
    // it is opened.
    void showEvent(QShowEvent *event) override;

private:
    void sendRequest();
    void respond(bool accept);
    void removeFriend();

    QString accountName;
    QLineEdit *requestId;
    QLabel *status;
    QListWidget *requests;
    QListWidget *sent;
    QTableWidget *friends;
    QLabel *noRequests;
    QLabel *noSent;
    QLabel *noFriends;
};

#endif // FRIENDSPAGE_H
