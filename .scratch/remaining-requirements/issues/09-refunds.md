# 09: Refunds

**Owner:** P3

**Requirements:** FR-10 (refund history), FR-32, FR-33 (correcting refunds)

**Files:** `UI/purchasespage.*`, `tests/ui/purchase_history_test.cpp`

**What to build:** The user records a full or partial refund, linked to the original purchase when there is one. The points return to the balance, and the refund appears in history, where it can be edited or removed.

**Blocked by:** 08 (Filter history and edit purchases)

**Status:** ready-for-agent

- [ ] The user can record a refund from a selected purchase, which links it automatically, or on its own
- [ ] A linked refund can't be larger than the part of the purchase that hasn't been refunded yet
- [ ] Refunds raise the balance, appear in history, and work with the filters
- [ ] Refunds can be edited or removed, and the balance updates to match
- [ ] Removing a purchase that has refunds follows a rule written in SPECIFICATIONS.md
- [ ] Tests cover full refunds, partial refunds, a rejected over-refund, editing, and removing
