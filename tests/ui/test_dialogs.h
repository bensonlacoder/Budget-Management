#ifndef TEST_DIALOGS_H
#define TEST_DIALOGS_H

#include <QAbstractButton>
#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QTimer>

// Answers the next confirmation box by clicking the button labelled `choice` (without
// its '&'), and copies the box's question into `question`. Call it just before the
// click that opens the box: exec() blocks, so a timer waits for the box to appear.
inline void answerConfirmation(const QString &choice, QString &question) {
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

#endif // TEST_DIALOGS_H
