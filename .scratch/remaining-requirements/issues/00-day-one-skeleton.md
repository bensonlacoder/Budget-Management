# 00: Day-one skeleton

**Owner:** P2 (tabs and build), with P1 (storage header)

**Files:** `UI/main.cpp`, `CMakeLists.txt`, new empty `UI/profilepage.*`, `UI/planpage.*`, `UI/chartspage.*`, `src/quarter.*`, `UI/emailsender.*`; P1: `UI/localaccounts.h`

**What to build:** The app's final structure, with empty pages and empty storage functions, so that every later ticket only touches its owner's files. This must be merged on day 1.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The main window has Purchases, Budget, Plan, Charts, and Profile tabs; the new tabs show a placeholder
- [ ] Every new source file is listed in CMake for both `app` and `ui_tests`, and `ui_tests` links Qt SQL
- [ ] `PurchasesPage::balanceUpdated` is connected to the Plan, Charts, and Profile pages so they can refresh when the balance changes
- [x] P1 adds every storage function the tickets need to `localaccounts.h` as a stub that compiles and returns a failure or empty result. The functions are:
  - profile update
  - password change
  - balance correction (set, add, subtract)
  - days correction and clearing it
  - reserve goal
  - dining habits
  - alert settings
  - purchase edit
  - refunds (record, edit, remove)
  - combined history (purchases, refunds, corrections)
  - outgoing friend requests and friend removal
  - saved menu sections
  - last-updated time
  - verification and reset codes
- [ ] The app builds, and all existing tests pass
