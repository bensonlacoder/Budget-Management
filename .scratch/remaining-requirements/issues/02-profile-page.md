# 02: Profile page

**Owner:** P2

**Requirements:** FR-05, FR-15 (display), FR-19, FR-20, FR-21 (password change), FR-47

**Files:** `UI/profilepage.*`, `tests/ui/profile_page_test.cpp`

**What to build:** A Profile tab where the user sees their account and corrects their real balance and days left.

**Blocked by:** 00 (Day-one skeleton), 01 (SQLite storage) for real saving, 03 (Quarter calendar) for quarter dates and Reset days

**Status:** ready-for-agent

- [ ] The tab shows display name, email (the account ID), friend ID with a Copy button, dining plan, points left, days left with the current quarter's dates, and the last-updated time
- [ ] Display name and email can be edited; an invalid or already-used email is rejected with a clear message
- [ ] **Correct balance** offers Set, Add, and Subtract. Subtracting more than the balance is rejected. Each correction is saved as a "Balance correction" history entry, and the new balance appears everywhere
- [ ] **Correct days** sets a manual days-left count that keeps going down by one each day
- [ ] **Reset days** clears the manual count and returns to the calendar value
- [ ] **Change password** requires the current password and the new one entered twice; a wrong current password, a short new password, and two entries that don't match each show a clear message
- [ ] Tests cover each correction type, each rejection, and Reset days
