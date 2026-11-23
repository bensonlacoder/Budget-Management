# 03: Quarter calendar

**Owner:** P2

**Requirements:** FR-22, FR-39 (time remaining)

**Files:** `src/quarter.*`, `tests/quarter_test.cpp`

**What to build:** Days left are calculated from SCU's academic calendar instead of a fixed 77. This ticket is the calculation and its tests; the Profile page (02) and Plan page (06) use it.

**Blocked by:** 00 (Day-one skeleton)

**Status:** ready-for-agent

SCU 2026–27 undergraduate dates (first day of classes to last day of finals):

| Quarter | Start | End |
| --- | --- | --- |
| Fall 2026 | 2026-09-21 | 2026-12-11 |
| Winter 2027 | 2027-01-04 | 2027-03-19 |
| Spring 2027 | 2027-03-30 | 2027-06-10 |

Source: [SCU Undergraduate Bulletin 2026–27](https://www.scu.edu/bulletin/undergraduate/academic-calendar.html). Spring's start date differs by one day between SCU sources; March 30 is used here.

- [ ] Given today's date, the calculation returns the current quarter and the days left counting today (for example, 20 on 2026-11-22)
- [ ] Between quarters, it returns the next quarter's full length and start date (for example, Winter's 75 days on 2026-12-20)
- [ ] After the last listed quarter, it reports that the calendar needs updating, and the app falls back to the manual days count
- [ ] A manual days correction, when one exists, takes priority over the calendar
- [ ] SPECIFICATIONS.md documents the quarter dates and these rules
- [ ] Unit tests cover the first and last day of a quarter, a break, and a date after the calendar ends
