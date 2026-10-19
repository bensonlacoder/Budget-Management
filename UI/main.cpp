#include "mainui.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainUI w;
    w.show();
    return QCoreApplication::exec();
}
