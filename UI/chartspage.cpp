#include "chartspage.h"

#include <QLabel>
#include <QVBoxLayout>

ChartsPage::ChartsPage(const QString &accountName, QWidget *parent)
    : QWidget(parent), accountName(accountName) {
    auto *layout = new QVBoxLayout(this);
    auto *placeholder = new QLabel(tr("Spending charts will appear here."), this);
    placeholder->setAlignment(Qt::AlignCenter);
    layout->addWidget(placeholder);
}

void ChartsPage::refresh() {}
