# 12: "Last updated" indicator

**Requirements:** FR-15

**What to build:** The app shows when the balance and purchase history were last successfully updated, and whether that update was entered manually or imported.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] Every successful change to the balance or a purchase saves a timestamp and its source (manual or imported)
- [ ] The Budget and Purchases pages show "Last updated <date/time> (manual)" or "(imported)"
- [ ] A failed save does not change the timestamp
- [ ] The timestamp is still there after signing out and back in (FR-23)
- [ ] Tests cover the timestamp changing on success and staying the same on failure
