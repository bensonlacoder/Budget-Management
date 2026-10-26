#include "mainui.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainUI window;
    window.show();

    return app.exec();
}
