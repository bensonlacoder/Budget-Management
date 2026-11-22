# 14: Find affordable meals and save menu sections

**Requirements:** FR-44, FR-45

**What to build:** The user enters a maximum number of points and sees every available meal option at or below that price, with its dining location and price. They can also save menu sections for quick access and remove them later.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] A point-limit search lists matching items with their location and price, sorted by price
- [ ] Items without a fixed price (receipt total) are left out, or shown separately with a label
- [ ] The user can save a menu section (for example, a station at a location), see a list of saved sections, and remove any of them
- [ ] Saved sections are still there after signing out and back in (FR-23)
- [ ] Tests cover the price filter boundaries (equal to the limit, just above it) and saving and removing sections
