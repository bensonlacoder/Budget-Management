# 08: Filter purchase history and edit purchases

**Requirements:** FR-10 (filtering), FR-33 (correcting purchases)

**What to build:** The user can filter purchase history by a date range and by dining location, and can edit a manually entered purchase. The balance and budget advice update after an edit.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] Date-range and location filters above the history narrow the list, and they can be cleared
- [ ] A message is shown when no purchases match the filters
- [ ] Selecting a purchase and choosing Edit lets the user change its date, location, item, and points
- [ ] Editing the points changes the balance by the difference, and an edit that would make the balance negative is rejected with the purchase left unchanged
- [ ] The balance and budget advice refresh after an edit
- [ ] Edits are still there after signing out and back in (FR-23)
- [ ] Tests cover each filter, the filters combined, a successful edit, and a rejected edit
