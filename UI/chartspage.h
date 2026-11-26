#ifndef CHARTSPAGE_H
#define CHARTSPAGE_H

#include <QString>
#include <QWidget>

// Charts tab: spending totals and charts from the saved history (T10, T11).
// Owner: P3. Placeholder until those tickets land.
class ChartsPage : public QWidget {
    Q_OBJECT

public:
    explicit ChartsPage(const QString &accountName, QWidget *parent = nullptr);
    // Reloads the saved history, for example after the balance changes on another tab.
    void refresh();

private:
    const QString accountName;
};

#endif // CHARTSPAGE_H
