# 01: SQLite storage and email sign-in

**Owner:** P1

**Requirements:** FR-17, FR-18, FR-23

**Files:** `UI/localaccounts.*`, `UI/authdialog.*`, `tests/ui/test_account_store.h`, `tests/ui/local_accounts_test.cpp`

**What to build:** All account data moves from `accounts.json` to a local SQLite database, and users sign up and sign in with an email address and password instead of a username. Every stub from ticket 00 is filled in, so the other pages save real data.

**Blocked by:** 00 (Day-one skeleton)

**Status:** done (branch `p1-storage`)

- [x] The database file lives in the app's local data folder, and its tables are created on first run
- [x] Data has tables for accounts, purchases, refunds, balance corrections, friends, friend requests, settings (reserve, habits, alerts, days correction), and saved menu sections
- [x] Sign-up asks for email, display name, dining plan, and password; emails are unique regardless of upper or lower case, and an invalid email format is rejected
- [x] Sign-in uses email and password; usernames are removed everywhere
- [x] Passwords are still stored as salted PBKDF2 hashes
- [x] The Law Plan is removed from the dining plan list
- [x] Any write that changes more than one table (for example, a purchase and the balance) happens in a single transaction
- [x] If the database can't be opened, sign-in shows a clear error and the app does not crash
- [x] `accounts.json` is no longer read or written
- [x] Tests run against a temporary database, and every storage function has at least one success test and one failure test
- [x] **Day-3 checkpoint:** sign-up, sign-in, balance, and purchases work on SQLite
