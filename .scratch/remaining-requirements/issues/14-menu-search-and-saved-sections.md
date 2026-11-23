# 14: Find affordable meals and save menu sections

**Owner:** Whoever finishes first (first ticket to cut)

**Requirements:** FR-44, FR-45

**Files:** new `UI/menusearchdialog.*`, `tests/ui/menu_search_test.cpp`; one announced line in `UI/main.cpp` to open it from a menu

**What to build:** The user enters a maximum number of points and sees every meal at or under that price, with its location and price. They can also save menu sections for quick access and remove them later.

**Blocked by:** 01 (SQLite storage), for saved sections

**Status:** ready-for-agent

- [ ] The point-limit search lists matching items sorted by price; items priced at the receipt total are left out or labelled
- [ ] The user can save a section (for example, a station at a location), see the saved list, and remove entries; the list is saved
- [ ] Tests cover a price exactly at the limit, a price just above it, and saving and removing sections
