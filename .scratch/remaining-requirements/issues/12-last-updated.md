# 12: Last updated time

**Owner:** P1 (storage); P2 shows it on the Profile page in ticket 02

**Requirements:** FR-15

**Files:** `UI/localaccounts.*`, `tests/ui/local_accounts_test.cpp`

**What to build:** Every successful change to the balance or history records when it happened, so the Profile page can show "Last updated …".

**Blocked by:** 01 (SQLite storage)

**Status:** done (branch `p1-storage`); P2 shows `Profile::lastUpdated` on the Profile page

- [x] Purchases, refunds, edits, removals, and balance corrections each save a last-updated timestamp when they succeed
- [x] A failed or rejected change leaves the timestamp unchanged
- [x] A storage function returns the timestamp, for the Profile page to display
- [x] Tests cover the timestamp changing on success and staying the same on failure
