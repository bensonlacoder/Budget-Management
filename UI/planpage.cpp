#include "planpage.h"

#include <QLabel>
#include <QVBoxLayout>

PlanPage::PlanPage(const QString &accountName, QWidget *parent)
    : QWidget(parent), accountName(accountName) {
    auto *layout = new QVBoxLayout(this);
    auto *placeholder = new QLabel(tr("Spending advice will appear here."), this);
    placeholder->setAlignment(Qt::AlignCenter);
    layout->addWidget(placeholder);
}

void PlanPage::refresh() {}
