# Remaining work: plan for the final week

Four people, one week. Tickets are in [issues/](issues/), numbered in dependency order.

## Who does what

| Person | Area | Tickets, in order |
| --- | --- | --- |
| **Nathan Gu** | Storage and accounts | 00 (storage header part) → 01 SQLite storage and email sign-in → 12 Last updated → 15 Email verification and password reset |
| **Benson** | Profile, quarters, Budget tab | 00 (skeleton part) → 03 Quarter calendar → 02 Profile page → 04 Budget tab sandbox |
| **Nathan Jeong** | Purchases and charts | 08 Filter and edit history → 09 Refunds → 10 Location totals and charts → 11 Budget vs. actual |
| **Jackie** | Advice and friends | 06 Reserve goal → 07 Dining habits and suggestions → 05 Pending requests and remove friend → 13 Alerts |
| **Nathan Gu** (finished P1 first) | Menus | 14 Menu search and saved sections |
| After the deadline | Backend | 16 Move to a hosted backend |

## Timeline

- **Day 1:** ticket 00 merged by the end of the day. P1 adds every storage function as an empty stub, and P2 adds the empty tabs and lists every new file in CMake. Everyone else starts building against the stubs.
- **Day 3 checkpoint:** T01 (SQLite) done. Check progress against the cut order below.
- **Days 4 to 6:** features.
- **Day 7:** integration, bug fixes, demo practice. No new features.

## If we fall behind, cut in this order

1. T14 Menu search and saved sections
2. T13 Alerts
3. T11 Budget vs. actual
4. T04 Budget tab sandbox

Never cut: T01, T02, T03, T06, T07, T08, T09.

## File ownership (to avoid merge conflicts)

Only the owner edits these files. If you need a change in someone else's file, ask them.

| Files | Owner |
| --- | --- |
| `UI/localaccounts.*`, `UI/authdialog.*`, `UI/emailsender.*`, `apps-script/` | P1 |
| `src/quarter.*`, `UI/profilepage.*`, `UI/mainui.*` (Budget tab) | P2 |
| `UI/purchasespage.*`, `UI/chartspage.*` | P3 |
| `src/budget.*`, `UI/planpage.*`, `UI/friendsdialog.*` | P4 |
| `UI/main.cpp`, `CMakeLists.txt` | P2 on day 1 (ticket 00); afterwards only small, announced edits |

Rules:

- **Storage goes through `LocalAccounts` only.** Never open the database from a page. If you need a storage function that isn't in the header, ask P1 to add it.
- **Each person writes tests in their own files.** Tests use a temporary SQLite database, never a real one.
- **Put an `// Author:` comment above each test you write.**

## Decisions made

- **Storage:** local SQLite with sign-in by email; usernames are removed. Old `accounts.json` accounts are not carried over.
- **Email:** verification at sign-up and password reset send codes through a Google Apps Script web app on a dedicated Gmail account. A shared secret lives in an uncommitted config file.
- **Unreachable storage:** if the database can't be opened, show a clear error instead of crashing.
- **Days left:** come from SCU's 2026–27 quarter calendar, counting from the first day of classes through the last day of finals. Between quarters, show the next quarter's full length. A manual correction keeps counting down, and Reset returns to the calendar value. The fixed 77-day default is removed.
- **Plans:** quarter plans only; the Law Plan is dropped.
- **Pages:**
  - Budget tab: a sandbox that is never saved.
  - Plan tab: the real advice.
  - Profile page: real balance and days corrections.
- **Balance corrections:** Set, Add, or Subtract. Each is recorded in history as "Balance correction", and one that lowers the balance counts as "Unlogged" spending in charts.
- **Cut:** dining card sync.
