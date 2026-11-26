#ifndef FRIENDSDIALOG_H
#define FRIENDSDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QTableWidget;

// Friend ID, sending and answering requests, pending sent requests, and accepted
// friends' balances, with removing a friend (T05).
class FriendsDialog : public QDialog {
    Q_OBJECT

public:
    FriendsDialog(const QString &accountName, const QString &friendId, QWidget *parent = nullptr);

private:
    void refresh();
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

#endif // FRIENDSDIALOG_H
