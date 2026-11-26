#ifndef HISTORYLOCATIONS_H
#define HISTORYLOCATIONS_H

#include "localaccounts.h"

#include <QHash>
#include <utility>

inline void resolveRefundLocations(QList<LocalAccounts::HistoryEntry> &history) {
    using Kind = LocalAccounts::HistoryEntry::Kind;
    QHash<QString, QString> purchaseLocations;
    for (const auto &entry : std::as_const(history)) {
        if (entry.kind == Kind::Purchase) {
            purchaseLocations.insert(entry.id, entry.location);
        }
    }
    for (auto &entry : history) {
        if (entry.kind == Kind::Refund && !entry.purchaseId.isEmpty()) {
            entry.location = purchaseLocations.value(entry.purchaseId);
        }
    }
}

#endif // HISTORYLOCATIONS_H
