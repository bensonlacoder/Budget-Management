# 13: Spending alerts

**Requirements:** FR-41, FR-42

**What to build:** The user can separately turn on or off two alerts: going over the daily budget, and running out of points before the quarter ends. Alerts that are on appear when their condition happens.

**Blocked by:** 04 (Quarter end date)

**Status:** ready-for-agent

- [ ] Settings have two separate on/off switches, one for each alert
- [ ] When today's recorded spending goes over today's daily budget and that alert is on, the user sees an alert once per day
- [ ] When projected spending would run out the balance before the quarter end date and that alert is on, the user sees an alert
- [ ] An alert that is off never appears
- [ ] Alert settings are still there after signing out and back in (FR-23)
- [ ] Unit tests cover each condition with its alert on and off
