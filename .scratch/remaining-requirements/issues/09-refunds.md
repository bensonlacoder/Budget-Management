# 09: Refunds

**Requirements:** FR-32, FR-10 (refund history), FR-33 (correcting refunds)

**What to build:** The user can record a full or partial refund, linked to the original purchase when there is one. The refunded points go back into the balance and are taken off spending totals. Refunds appear in the history and can be edited or removed.

**Blocked by:** 08 (Filter purchase history and edit purchases)

**Status:** ready-for-agent

- [ ] The user can record a refund from a selected purchase, which links it automatically, or on its own
- [ ] A refund linked to a purchase cannot be larger than what is left unrefunded on that purchase
- [ ] The balance goes up by the refund amount
- [ ] Refunds appear in the history, are visually different from purchases, and work with the filters from 08
- [ ] Refunds can be edited or removed, and the balance updates to match
- [ ] Removing a purchase that has refunds either removes or unlinks them; the team writes the rule in SPECIFICATIONS.md
- [ ] Refunds are still there after signing out and back in (FR-23)
- [ ] Tests cover full refunds, partial refunds, over-refunding being rejected, and editing and removing refunds
