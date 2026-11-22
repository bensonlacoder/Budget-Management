# 04: Quarter end date

**Requirements:** FR-22, part of FR-39 (calculating time remaining)

**What to build:** The Budget page shows the quarter end date. It defaults to the last day of finals week, and the user can choose an earlier or later date. When a date is set, days remaining are calculated from it instead of being typed in.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The current quarter's finals-week end date is the default
- [ ] The user can pick a custom end date earlier or later than the default and can go back to the default
- [ ] When an end date is set, days remaining (counting today) are calculated from today and the end date, and the daily and weekly advice updates
- [ ] When no end date is set, the manually entered day count from FR-05 is used
- [ ] A date that would give more than 90 days, or a date in the past, is rejected with a clear message, and the previous value is kept
- [ ] The end date is still there after signing out and back in (FR-23)
- [ ] Unit tests cover counting days remaining, including the end date itself and an end date of today
