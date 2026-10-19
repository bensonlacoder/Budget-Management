#ifndef MAINUI_H
#define MAINUI_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainUI;
}
QT_END_NAMESPACE

class MainUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainUI(QWidget *parent = nullptr);
    ~MainUI() override;

private:
    Ui::MainUI *ui;
};
#endif // MAINUI_H
