# 16: Move to a hosted backend (after the deadline)

**Owner:** Unassigned. Not part of the final week.

**Requirements:** FR-23 across devices; makes FR-24 to FR-30 (friends) work between different computers

**Files:** `UI/localaccounts.*` (internals only; the functions stay the same)

**What to build:** Accounts, purchases, and friends move from a SQLite file on one computer to a hosted database (for example, Supabase's free plan). Then two students on different laptops can be friends, and their data follows them to any computer.

**Blocked by:** 01 (SQLite storage), 15 (Email verification and password reset)

**Status:** backlog

- [ ] The team picks a provider that is free and doesn't need anyone's computer running, and writes the decision in SPECIFICATIONS.md
- [ ] Database rules ensure each user can read only their own data, and friends can see only display name and balance
- [ ] The app ships only the public key, never the admin key
- [ ] When the server can't be reached, the app shows "Can't reach the server. Check your internet connection." and offers to retry
- [ ] The `LocalAccounts` functions keep the same names and behaviour, so pages don't change
