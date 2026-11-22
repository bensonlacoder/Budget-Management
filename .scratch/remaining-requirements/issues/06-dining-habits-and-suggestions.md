# 06: Dining habits and habit-aware suggestions

**Requirements:** FR-38, FR-39, FR-40

**What to build:** The user enters which days of the week they eat on campus and how many meals they eat on those days. Spending suggestions then use the balance, time remaining, reserve goal, and these habits, and refresh immediately after any of these changes.

**Blocked by:** 04 (Quarter end date), 05 (Reserve goal)

**Status:** ready-for-agent

- [ ] The user can select their on-campus days of the week and enter meals per day for each selected day
- [ ] The user can also upload their habits from a file; the team chooses and documents a simple format (for example, CSV)
- [ ] Suggestions spread the points available above the reserve across the remaining on-campus meals only, for example a per-meal amount and a per-campus-day amount
- [ ] Time remaining comes from the quarter end date when one is set, and from the manual day count otherwise
- [ ] Changing the balance, days remaining, end date, reserve, or habits updates the suggestions without a restart
- [ ] Habits are still there after signing out and back in (FR-23)
- [ ] Unit tests cover the suggestion math with and without habits, and with a week that has no campus days
