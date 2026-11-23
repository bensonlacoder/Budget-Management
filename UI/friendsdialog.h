#ifndef FRIENDSDIALOG_H
#define FRIENDSDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QTableWidget;

class FriendsDialog : public QDialog {
    Q_OBJECT

public:
    FriendsDialog(const QString &accountName, const QString &friendId, QWidget *parent = nullptr);

private:
    void refresh();
    void sendRequest();
    void respond(bool accept);

    QString accountName;
    QLineEdit *requestId;
    QLabel *status;
    QListWidget *requests;
    QTableWidget *friends;
    QLabel *noRequests;
    QLabel *noFriends;
};

#endif // FRIENDSDIALOG_H
