#ifndef PLANPAGE_H
#define PLANPAGE_H

#include <QString>
#include <QWidget>

// Plan tab: real spending advice from the saved balance, reserve goal and dining
// habits (T06, T07, T13). Owner: P4. Placeholder until those tickets land.
class PlanPage : public QWidget {
    Q_OBJECT

public:
    explicit PlanPage(const QString &accountName, QWidget *parent = nullptr);
    // Reloads the saved data, for example after the balance changes on another tab.
    void refresh();

private:
    const QString accountName;
};

#endif // PLANPAGE_H
