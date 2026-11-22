# 05: Reserve goal and shortfall warning

**Requirements:** FR-37, FR-43

**What to build:** The user enters how many points they want left over at the end of the quarter. Spending advice is based only on points above that reserve, and the app warns the user when projected spending would leave less than the reserve.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The Budget page has a reserve-goal field (default 0)
- [ ] Daily advice = (balance - reserve) / days remaining, with the weekly figure calculated from it the same way as now
- [ ] A reserve that is negative or larger than the balance is rejected with a clear message, and the previous value is kept
- [ ] If spending continues at the recent rate (the team defines the rate in SPECIFICATIONS.md), the app projects the end-of-quarter balance and shows a warning when that projection is below the reserve
- [ ] The reserve goal is still there after signing out and back in (FR-23)
- [ ] Unit tests cover advice with a reserve, rejecting an invalid reserve, and the projection triggering and not triggering the warning
