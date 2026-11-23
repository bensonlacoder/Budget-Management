# 08: Filter history and edit purchases

**Owner:** P3

**Requirements:** FR-10 (filtering), FR-11, FR-33 (correcting purchases)

**Files:** `UI/purchasespage.*`, `tests/ui/purchase_history_test.cpp`

**What to build:** The history on the Purchases tab can be filtered by date range and location. It also shows balance corrections, and a purchase can be edited, with the balance updating to match.

**Blocked by:** 01 (SQLite storage), but the UI can be built against the stubs from 00

**Status:** ready-for-agent

- [ ] Date-range and location filters narrow the history and can be cleared; a message appears when nothing matches
- [ ] Balance corrections from the Profile page appear in history, labelled "Balance correction" and visually different from purchases
- [ ] Edit changes a purchase's date, location, item, or points; the balance changes by the difference
- [ ] An edit that would make the balance negative is rejected, and the purchase stays unchanged
- [ ] Purchase dates are limited to the current quarter (from 03)
- [ ] Tests cover each filter, the filters combined, a successful edit, and a rejected edit
